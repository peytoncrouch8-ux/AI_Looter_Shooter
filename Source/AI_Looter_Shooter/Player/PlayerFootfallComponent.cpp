#include "Player/PlayerFootfallComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/PlayerSoundComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Weapons/WaterRings.h"
#include "Weapons/WeaponFX.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
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

	/** Wading looks for water this far over the feet (cm): the surface of a pool round the thighs, not only the shins. */
	constexpr double WadeProbeAbove = 90.0;
	/** On foot, water is looked for this often on dry ground, and its surface checked again this often in it (a trace against the water's own mesh), in seconds. */
	constexpr float WadeLookSeconds = 0.2f;
	constexpr float WadeRecheckSeconds = 0.1f;
	/** A wading step's ring is a hit of this Size (the sprint splash's) times the body's scale, the foot this far (cm) to its side. */
	constexpr float WadeRingSize = 0.6f;
	constexpr double WadeFootSide = 9.0;
	/** A step this soon after a sprint's splash (which has its own ring) leaves no second one (seconds). */
	constexpr double WadeSplashGap = 0.15;
	/** The first wading step after setting off comes this share of a stride in (the footsteps' own). */
	constexpr float WadeFirstStepShare = 0.35f;
	/** The foam's light is looked up again this often (seconds): one trace toward the sun. */
	constexpr double WadeLightSeconds = 0.5;
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
	WadeStride = StrideLeft * WadeFirstStepShare;
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
	TickWading(DeltaTime);
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
	// Water found here is water the wading goes on in (its mesh is traced alone from now on).
	NoteWater(LastSurface == EImpactMaterial::Water ? &Hit : nullptr);

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
	if (Dust.GetSplashCount() > SplashesBefore)
	{
		LastSplashTime = Now;
	}

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

// ---------------------------------------------------------------------------
// Wading
// ---------------------------------------------------------------------------

void UPlayerFootfallComponent::NoteWater(const FHitResult* WaterHit)
{
	if (!WaterHit || !WaterHit->GetComponent())
	{
		WaterBody.Reset();
		return;
	}
	WaterBody = WaterHit->GetComponent();
	WaterPoint = WaterHit->ImpactPoint;
	WaterUp = WaterHit->ImpactNormal.IsNearlyZero() ? FVector::UpVector : FVector(WaterHit->ImpactNormal).GetSafeNormal();
	WadeRecheck = WadeRecheckSeconds;
}

bool UPlayerFootfallComponent::LookForWater(const FVector& Feet)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	// Complex, with the face, as the footfall's ground look: the water answers by the material it wears.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootfallWade), /*bTraceComplex*/ true, GetOwner());
	Params.bReturnFaceIndex = true;
	Params.bReturnPhysicalMaterial = true;
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Feet + FVector(0.0, 0.0, WadeProbeAbove), Feet - FVector(0.0, 0.0, FootfallProbeBelow),
		FootfallChannel, Params) || ImpactSurface::Of(Hit) != EImpactMaterial::Water)
	{
		return false;
	}
	NoteWater(&Hit);
	return WaterBody.IsValid();
}

void UPlayerFootfallComponent::TickWading(float DeltaTime)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!World || !Movement || !Capsule)
	{
		return;
	}
	const UPlayerLocomotionComponent* Moves = Locomotion.Get();
	const bool bOnFoot = Movement->IsMovingOnGround() && !(Moves && (Moves->IsSliding() || Moves->IsTraversing()));
	const float Speed = bOnFoot ? static_cast<float>(Movement->Velocity.Size2D()) : 0.f;
	if (Speed < FWaterRings::WakeMinSpeed)
	{
		// Standing, creeping or in the air: no wake, and the next wade's first step comes soon after setting off.
		WadeStride = FMath::Min(WadeStride, UPlayerSoundComponent::StrideLength(0.f) * WadeFirstStepShare);
		WakeTravel = 0.f;
		WaterBody.Reset();
		return;
	}

	const FVector Feet = Capsule->GetComponentLocation() - FVector(0.0, 0.0, Capsule->GetScaledCapsuleHalfHeight());
	// The footsteps' own stride, so a ring comes down with each step heard.
	WadeStride -= Speed * DeltaTime;
	const bool bStep = WadeStride <= 0.f;
	if (bStep)
	{
		WadeStride = FMath::Max(WadeStride + UPlayerSoundComponent::StrideLength(Speed), 0.f);
		++WadeSteps;
	}

	WadeRecheck -= DeltaTime;
	if (WadeRecheck <= 0.f)
	{
		WadeRecheck = WaterBody.IsValid() ? WadeRecheckSeconds : WadeLookSeconds;
		if (WaterBody.IsValid())
		{
			// Still in it? A line against the water's own mesh, nothing else.
			FCollisionQueryParams Params(SCENE_QUERY_STAT(FootfallWadeSurface), /*bTraceComplex*/ true);
			FHitResult Surface;
			if (WaterBody->LineTraceComponent(Surface, Feet + FVector(0.0, 0.0, WadeProbeAbove), Feet - FVector(0.0, 0.0, FootfallProbeBelow), Params))
			{
				WaterPoint = Surface.ImpactPoint;
				if (!Surface.ImpactNormal.IsNearlyZero())
				{
					WaterUp = FVector(Surface.ImpactNormal).GetSafeNormal();
				}
			}
			else
			{
				WaterBody.Reset();
			}
		}
		else
		{
			// On dry ground: a look five times a second, so a creek is noticed within a stride or so of its bank.
			LookForWater(Feet);
		}
	}
	else if (WaterBody.IsValid())
	{
		// Between looks the surface is where it was, under the feet's new place.
		WaterPoint = FVector(Feet.X, Feet.Y, WaterPoint.Z);
	}
	if (!WaterBody.IsValid())
	{
		WakeTravel = 0.f;
		return;
	}

	const float Scale = FMath::Max(static_cast<float>(Capsule->GetComponentScale().Z), 0.1f);
	WakeTravel += Speed * DeltaTime;
	const double Now = World->GetTimeSeconds();
	const bool bWake = FWaterRings::WakeDue(WakeTravel, Speed, Scale);
	const bool bRing = bStep && Now - LastSplashTime > WadeSplashGap;
	UBulletSubsystem* Bullets = (bWake || bRing) ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	if (Now - WadeLitAt >= WadeLightSeconds || Now < WadeLitAt)
	{
		WadeLit = Light.At(*World, WaterPoint + FVector(0.0, 0.0, FootfallLightHeight), *Character);
		WadeLitAt = Now;
	}
	const FVector Along = FVector(Movement->Velocity.X, Movement->Velocity.Y, 0.0).GetSafeNormal();
	if (bWake)
	{
		WakeTravel = 0.f;
		Effects.SpawnWake(WaterPoint, WaterUp, Along, Speed, Scale, &WadeLit);
	}
	if (bRing)
	{
		// Off the foot that came down, the other one next step.
		const FVector Side = FVector::CrossProduct(WaterUp, Along).GetSafeNormal() * (WadeSteps % 2 == 0 ? 1.0 : -1.0);
		Effects.SpawnWaterRings(WaterPoint + Side * (WadeFootSide * Scale), WaterUp, WadeRingSize * Scale, &WadeLit);
	}
}
