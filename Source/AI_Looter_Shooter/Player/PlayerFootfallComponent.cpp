#include "Player/PlayerFootfallComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/PlayerSoundComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Weapons/WeaponFX.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** The bullets' trace channel (Weapon): what a shot would hit under the feet, the water's surface included. */
	constexpr ECollisionChannel FootfallChannel = ECC_GameTraceChannel2;
	/** How far over and under the feet the ground is looked for (cm): over, so a creek's surface round the shins is found. */
	constexpr double FootfallProbeAbove = 40.0;
	constexpr double FootfallProbeBelow = 30.0;
	/** Running this fast or more, without the locomotion's word, is a sprint (cm/s; the player's walk is about 459). */
	constexpr float FootfallSprintSpeed = 600.f;
	/** The light is judged this far over the ground (cm), where the dust hangs. */
	constexpr double FootfallLightHeight = 40.0;
}

UPlayerFootfallComponent::UPlayerFootfallComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// After the movement, so a step goes by this frame's speed and floor (as the footsteps' sounds do).
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

float UPlayerFootfallComponent::LandHardness(float FallSpeed)
{
	return FMath::Clamp((FallSpeed - SoftLandSpeed) / (HardLandSpeed - SoftLandSpeed), 0.f, 1.f);
}

void UPlayerFootfallComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		Character->LandedDelegate.AddUniqueDynamic(this, &UPlayerFootfallComponent::HandleLanded);
	}
	Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const UWorld* World = GetWorld();
	LastRefill = World ? World->GetTimeSeconds() : 0.0;
	StrideLeft = UPlayerSoundComponent::StrideLength(0.f);
}

void UPlayerFootfallComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->LandedDelegate.RemoveDynamic(this, &UPlayerFootfallComponent::HandleLanded);
	}
	Super::EndPlay(EndPlayReason);
}

bool UPlayerFootfallComponent::IsSprinting(float& OutSpeed) const
{
	OutSpeed = 0.f;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !Movement->IsMovingOnGround())
	{
		return false;
	}
	OutSpeed = static_cast<float>(Movement->Velocity.Size2D());
	if (const UPlayerLocomotionComponent* Moves = Locomotion.Get())
	{
		// The slide throws its own dust; a climb's feet aren't running.
		return !Moves->IsSliding() && !Moves->IsTraversing() && Moves->GetSprintAlpha() > 0.5f
			&& OutSpeed > 0.8f * Movement->MaxWalkSpeed;
	}
	return OutSpeed >= FootfallSprintSpeed;
}

void UPlayerFootfallComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	float Speed = 0.f;
	if (!IsSprinting(Speed))
	{
		// Not sprinting: the next sprint's first puff comes a couple of strides in, not at once.
		StrideLeft = FMath::Min(StrideLeft, UPlayerSoundComponent::StrideLength(Speed));
		Steps = 0;
		return;
	}
	// The footsteps' own stride (UPlayerSoundComponent), so the puffs come down with the steps heard.
	StrideLeft -= Speed * DeltaTime;
	if (StrideLeft <= 0.f)
	{
		StrideLeft = FMath::Max(StrideLeft + UPlayerSoundComponent::StrideLength(Speed), 0.f);
		++Steps;
		if (Steps % StepsPerPuff == 0)
		{
			Footfall(EFootfall::Step, 0.f);
		}
	}
}

void UPlayerFootfallComponent::HandleLanded(const FHitResult& Hit)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	// Still the speed it came down at: the movement tells the character before it takes the fall's speed away.
	const float FallSpeed = Movement ? FMath::Max(0.f, static_cast<float>(-Movement->Velocity.Z)) : 0.f;
	Steps = 0;
	if (FallSpeed >= SoftLandSpeed)
	{
		Footfall(EFootfall::Land, LandHardness(FallSpeed), &Hit);
	}
}

bool UPlayerFootfallComponent::FindGround(FHitResult& OutHit) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	const UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return false;
	}
	const FVector Feet = Capsule->GetComponentLocation() - FVector(0.0, 0.0, Capsule->GetScaledCapsuleHalfHeight());
	// Complex, with the face, so the material under the foot says the surface (a terrain's tiles have several).
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootfallGround), /*bTraceComplex*/ true, Character);
	Params.bReturnFaceIndex = true;
	Params.bReturnPhysicalMaterial = true;
	return World->LineTraceSingleByChannel(OutHit, Feet + FVector(0.0, 0.0, FootfallProbeAbove), Feet - FVector(0.0, 0.0, FootfallProbeBelow),
		FootfallChannel, Params);
}

int32 UPlayerFootfallComponent::Footfall(EFootfall Kind, float Hardness, const FHitResult* Ground)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Character || !Bullets)
	{
		return 0;
	}
	const double Now = World->GetTimeSeconds();
	Dust.Refill(static_cast<float>(Now - LastRefill));
	LastRefill = Now;

	FHitResult Hit;
	if (!FindGround(Hit))
	{
		if (!Ground || !Ground->bBlockingHit)
		{
			return 0;
		}
		Hit = *Ground;
	}
	LastSurface = ImpactSurface::Of(Hit);

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const FVector Velocity = Movement ? Movement->Velocity : Character->GetVelocity();
	FFootfallSpot Spot;
	Spot.Ground = Hit.ImpactPoint;
	Spot.Up = Hit.ImpactNormal.IsNearlyZero() ? FVector::UpVector : Hit.ImpactNormal;
	Spot.Along = FVector(Velocity.X, Velocity.Y, 0.0).GetSafeNormal();
	Spot.Speed = static_cast<float>(Velocity.Size2D());
	Spot.Hardness = Hardness;
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	Spot.Scale = Capsule ? FMath::Max(static_cast<float>(Capsule->GetComponentScale().Z), 0.1f) : 1.f;
	// Each puffing step comes off the other foot.
	Spot.Side = (Steps / StepsPerPuff) % 2 == 0 ? 1.f : -1.f;

	const FLinearColor Lit = Light.At(*World, Spot.Ground + FVector(0.0, 0.0, FootfallLightHeight), *Character);
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const int32 SplashesBefore = Dust.GetSplashCount();
	const int32 Thrown = Dust.Throw(Effects, Kind, LastSurface, Spot, Lit);

	// What's heard with it: a splash in water; a hard landing's heavy thud under the landing's own sound.
	const float Hard = FMath::Clamp(Hardness, 0.f, 1.f);
	if (FFootfallDust::Splashes(LastSurface))
	{
		// Heard even when the budget held the splash's look back, for a landing; a step's only with its look.
		if (Kind == EFootfall::Land || Dust.GetSplashCount() > SplashesBefore)
		{
			LooterSound::PlayAt(this, FootfallCue::Splash, Spot.Ground, Kind == EFootfall::Land ? FMath::Lerp(0.6f, 1.f, Hard) : 0.5f);
		}
	}
	else if (Kind == EFootfall::Land && Hard >= ThudHardness)
	{
		LooterSound::PlayAt(this, FootfallCue::HardLand, Spot.Ground, FMath::Lerp(0.7f, 1.f, (Hard - ThudHardness) / (1.f - ThudHardness)));
	}
	return Thrown;
}
