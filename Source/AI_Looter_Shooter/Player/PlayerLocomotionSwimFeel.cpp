#include "Player/PlayerLocomotionComponent.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Player/UnderwaterView.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponFX.h"
#include "World/SwimWater.h"
#include "World/SwimWaterRules.h"
#include "World/SwimWaterSubsystem.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EnhancedPlayerInput.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"

// UPlayerLocomotionComponent's swim, what it senses and what it gives back: the keys held to rise and dive, the bed under
// the feet, the breath, a stroke's push and the splashes going in and out (the swim itself: PlayerLocomotionSwim.cpp).

namespace
{
	/** Where the hands are at a stroke's push, ahead of the body's middle (full-size cm). */
	constexpr float StrokeHandsAhead = 45.f;
	/** A stroke's rings on the water, a wading step's size (FWaterRings) at full effort. */
	constexpr float StrokeRingSize = 0.5f;
	/** The surface splash's size as SpawnSurfaceImpact's Scale (a rifle shot's is 1), from small to the biggest. */
	constexpr float SplashScaleSmall = 1.3f;
	constexpr float SplashScaleBig = 2.6f;
}

// ---------------------------------------------------------------------------
// The keys and the bed
// ---------------------------------------------------------------------------

bool UPlayerLocomotionComponent::IsSwimKeyHeld(bool bJumpKey) const
{
	const APlayerController* PC = InputBinding.GetController();
	const UEnhancedPlayerInput* PlayerInput = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
	if (!PlayerInput)
	{
		return false;
	}
	const UInputAction* Action = nullptr;
	if (bJumpKey)
	{
		Action = JumpKeyAction.Get();
	}
	else if (const UKeyBindingSubsystem* Bindings = GetBindings())
	{
		Action = Bindings->GetCrouchAction();
	}
	return Action && PlayerInput->GetActionValue(Action).Get<bool>();
}

float UPlayerLocomotionComponent::GetSwimVerticalInput() const
{
	// Out of breath the water carries the body up, whatever the keys say: no drowning damage (the contract's "only if fun").
	if (bBreathSpent)
	{
		return 1.f;
	}
	float Input = 0.f;
	if (IsSwimKeyHeld(true))
	{
		Input += 1.f;
	}
	if (IsSwimKeyHeld(false))
	{
		Input -= 1.f;
	}
	// Swimming forward goes the look's way once it's pitched past the dead zone.
	const FVector2D Keys = GetHeldMoveInput();
	const ACharacter* Owner = Character.Get();
	if (Owner && Keys.Y > 0.1)
	{
		Input += SwimRules::DiveShare(static_cast<float>(Owner->GetControlRotation().GetNormalized().Pitch)) * static_cast<float>(Keys.Y);
	}
	return FMath::Clamp(Input, -1.f, 1.f);
}

bool UPlayerLocomotionComponent::FindSwimBed(float Reach, float& OutBedZ) const
{
	const ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	const UCapsuleComponent* Capsule = Owner ? Owner->GetCapsuleComponent() : nullptr;
	const UWorld* World = GetWorld();
	if (!Capsule || !Move || !World)
	{
		return false;
	}
	// What the body itself would stand on (its own channel and responses): the pond's bed, a rock, a jetty's step.
	const FVector Feet = Capsule->GetComponentLocation() - FVector(0.0, 0.0, Capsule->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SwimBed), false, Owner);
	const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Feet + FVector(0.0, 0.0, 5.0), Feet - FVector(0.0, 0.0, FMath::Max(Reach, 1.f)),
		Capsule->GetCollisionObjectType(), Params, Responses))
	{
		return false;
	}
	if (Hit.ImpactNormal.Z < Move->GetWalkableFloorZ())
	{
		return false;
	}
	OutBedZ = static_cast<float>(Hit.ImpactPoint.Z);
	return true;
}


// ---------------------------------------------------------------------------
// A stroke, a splash
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::StrokePushed()
{
	++StrokeCount;
	const ACharacter* Owner = Character.Get();
	const ASwimWater* Water = SwimWater.Get();
	if (!Owner || !Water)
	{
		return;
	}
	const float Scale = GetBodyScale();
	const float Loud = FMath::Lerp(0.55f, 1.f, SwimEffort);
	if (bHeadUnder)
	{
		LooterSound::Play2D(this, SwimCue::StrokeUnder, Loud);
		return;
	}
	const FVector Hands = Owner->GetActorLocation() + Owner->GetActorForwardVector().GetSafeNormal2D() * (StrokeHandsAhead * Scale);
	const FVector OnWater(Hands.X, Hands.Y, Water->GetSurfaceZ());
	LooterSound::PlayAt(this, SwimCue::Stroke, OnWater, Loud);
	if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		Effects.SpawnWaterRings(OnWater, FVector::UpVector, StrokeRingSize * Scale * SwimEffort);
	}
}

void UPlayerLocomotionComponent::SwimSplash(bool bIn, float Strength) const
{
	const ACharacter* Owner = Character.Get();
	const ASwimWater* Water = SwimWater.Get();
	UWorld* World = GetWorld();
	if (!Owner || !Water || !World)
	{
		return;
	}
	const float Share = FMath::Clamp(Strength, 0.f, 1.f);
	const FVector Point(Owner->GetActorLocation().X, Owner->GetActorLocation().Y, Water->GetSurfaceZ());
	LooterSound::PlayAt(this, bIn ? SwimCue::SplashIn : SwimCue::SplashOut, Point, FMath::Lerp(0.55f, 1.f, Share));
	if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
	{
		// The water's own hit look (splash, mist and rings), at a body's size.
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnSurfaceImpact(Point, FVector::UpVector, FVector(0.0, 0.0, bIn ? -1.0 : 1.0), EImpactMaterial::Water,
			FMath::Lerp(SplashScaleSmall, SplashScaleBig, Share), false);
	}
}

// ---------------------------------------------------------------------------
// Breath
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::UpdateBreath(float DeltaTime)
{
	const ACharacter* Owner = Character.Get();
	if (!Owner)
	{
		return;
	}
	// The eye's place (whatever the camera does in third person): the head under counts, swimming or crouched in a pond.
	const FVector Here = Owner->GetActorLocation();
	const FVector Eye(Here.X, Here.Y, GetFeetHeight() + GetEyeHeight());
	const ASwimWater* Water = SwimWater.Get();
	if (!bSwimming || !Water)
	{
		const USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(this);
		Water = Waters && Waters->NumWaters() > 0 ? Waters->FindAt(Eye, 0.f, USwimWaterSubsystem::DeepReachBelow) : nullptr;
	}
	const bool bUnder = Water && Eye.Z < Water->GetSurfaceZ() - SwimRules::HeadUnderMargin * GetBodyScale();
	if (bHeadUnder && !bUnder && SwimRules::GaspOnSurfacing(Breath))
	{
		LooterSound::Play2D(this, SwimCue::Gasp, FMath::Lerp(1.f, 0.6f, Breath));
	}
	bHeadUnder = bUnder;
	Breath = SwimRules::BreathAfter(Breath, DeltaTime, bUnder);
	if (SwimRules::OutOfBreath(Breath))
	{
		bBreathSpent = true;
	}
	else if (bBreathSpent && Breath >= SwimRules::BreathBack())
	{
		bBreathSpent = false;
	}
}
