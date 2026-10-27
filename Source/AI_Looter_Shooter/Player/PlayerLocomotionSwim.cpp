#include "Player/PlayerLocomotionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/UnderwaterView.h"
#include "Weapons/WaterRings.h"
#include "Weapons/WeaponFX.h"
#include "World/SwimWater.h"
#include "World/SwimWaterRules.h"
#include "World/SwimWaterSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"

// UPlayerLocomotionComponent's swimming (round 4, the user's pick 2026-10-27; note 30): deep water (an ASwimWater, more than
// chest deep over the feet) swims. The movement flies meanwhile (no gravity: the engine's own swimming needs a brush volume
// the build can't make) and this steers it up and down each frame; the rules and every number are SwimRules'
// (World/SwimWaterRules.h).

namespace
{
	/** The jump key's action, which the character binds itself (ALooterCharacter: IA_Jump); read raw here to rise while held. */
	const TCHAR* JumpActionPath = TEXT("/Game/Input/Actions/IA_Jump.IA_Jump");

	/** Seconds the body's swim pose takes in and out; the gun takes this long down, and this long back up wading out. */
	constexpr float SwimPoseSeconds = 0.3f;
	constexpr float SwimLowerSeconds = 0.28f;
	constexpr float SwimRaiseSeconds = 0.4f;
	/** How quickly the up-and-down speed follows what's asked (per second): firm, so a key let go stops the rise at once. */
	constexpr float VerticalResponse = 7.f;
	/** Rising with the key held slows over the last stretch below the float height (per second of the gap left). */
	constexpr float SurfaceEase = 6.f;
	/** The float's slow bob at the surface (full-size cm, radians a second): the water lifting the body a little. */
	constexpr float FloatBobDepth = 1.5f;
	constexpr float FloatBobRate = 1.7f;
	/** The bed is felt for this often (s): one short line under the feet. */
	constexpr float BedLookSeconds = 0.1f;
	/** A jump out doesn't fall straight back into a swim for this long (s), and only coming down. */
	constexpr double ReEnterSeconds = 0.25;
	/** Falling in: the swim starts once the feet are this share of a swim's depth under (the water has the body). */
	constexpr float FallInShare = 0.5f;
	/** The jump key works at the surface: the feet within this of their float height (full-size cm). */
	constexpr float JumpOutReach = 30.f;
	/** A stroke's push leaves its sound and rings only when swum this hard (0..1): treading water is quiet. */
	constexpr float StrokeEffortHeard = 0.25f;
	/** Going in from a fall this fast (cm/s) is the biggest splash; slower, smaller. */
	constexpr float SplashFullSpeed = 900.f;

	TAutoConsoleVariable<bool> CVarDebugSwim(TEXT("Looter.Swim.Debug"), false,
		TEXT("Print the swim each frame: water, depth over the feet, float height, vertical input, breath, stroke."));

	/** Whether a phase going from Before to After (0..1, wrapping) passed At. */
	bool Crossed(float Before, float After, float At)
	{
		return After >= Before ? (Before < At && After >= At) : (Before < At || After >= At);
	}
}

bool UPlayerLocomotionComponent::IsSwimmingActor(const AActor* Actor)
{
	const UPlayerLocomotionComponent* Moves = Actor ? Actor->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	return Moves && Moves->IsSwimming();
}

const ASwimWater* UPlayerLocomotionComponent::GetSwimWater() const
{
	return SwimWater.Get();
}

float UPlayerLocomotionComponent::GetSwimLower() const
{
	return FMath::SmoothStep(0.f, 1.f, SwimLowerLinear);
}

float UPlayerLocomotionComponent::GetSwimHandsShare() const
{
	// The hands leave the gun once it's most of the way down (out of view), and go back to it before it comes up.
	return FMath::SmoothStep(0.55f, 1.f, SwimLowerLinear);
}

void UPlayerLocomotionComponent::EnsureUnderwaterView()
{
	AActor* Owner = GetOwner();
	if (!Owner || Owner->FindComponentByClass<UUnderwaterViewComponent>())
	{
		return;
	}
	UUnderwaterViewComponent* View = NewObject<UUnderwaterViewComponent>(Owner, TEXT("UnderwaterView"), RF_Transient);
	View->RegisterComponent();
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::UpdateSwim(float DeltaTime)
{
	const float Dt = FMath::Clamp(DeltaTime, 0.f, 0.1f);
	SwimClock += Dt;
	if (bSwimming)
	{
		MoveSwim(Dt);
	}
	else if (!Traversal.IsActive() && !IsMovementHeld())
	{
		LookForSwim();
	}
	SwimAlpha = FMath::FInterpConstantTo(SwimAlpha, bSwimming ? 1.f : 0.f, Dt, 1.f / SwimPoseSeconds);
	SwimLowerLinear = FMath::FInterpConstantTo(SwimLowerLinear, bSwimming ? 1.f : 0.f, Dt, 1.f / (bSwimming ? SwimLowerSeconds : SwimRaiseSeconds));
	UpdateBreath(Dt);

#if !UE_BUILD_SHIPPING
	if (CVarDebugSwim.GetValueOnGameThread())
	{
		const ASwimWater* Water = SwimWater.Get();
		const float Feet = static_cast<float>(GetFeetHeight());
		const FString Line = FString::Printf(TEXT("Swim: %d water %s depth %.0f vz %.0f input %.2f breath %.2f%s head %s stroke %.2f effort %.2f lower %.2f"),
			bSwimming, Water ? *Water->WaterId.ToString() : TEXT("-"), Water ? Water->GetSurfaceZ() - Feet : 0.f,
			Movement.IsValid() ? Movement->Velocity.Z : 0.f, bSwimming ? GetSwimVerticalInput() : 0.f, Breath, bBreathSpent ? TEXT(" (out)") : TEXT(""),
			bHeadUnder ? TEXT("under") : TEXT("up"), StrokePhase, SwimEffort, SwimLowerLinear);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(reinterpret_cast<uint64>(this) + 1, 0.f, FColor::Cyan, Line);
		}
	}
#endif
}

void UPlayerLocomotionComponent::LookForSwim()
{
	const USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(this);
	ACharacter* Owner = Character.Get();
	const UCharacterMovementComponent* Move = Movement.Get();
	if (!Waters || Waters->NumWaters() == 0 || !Owner || !Move || Slide.IsActive())
	{
		return;
	}
	const bool bWalking = Move->IsMovingOnGround();
	if (!bWalking && !Move->IsFalling())
	{
		return;
	}
	const FVector Here = Owner->GetActorLocation();
	const float FeetZ = static_cast<float>(GetFeetHeight());
	ASwimWater* Water = Waters->FindAt(FVector(Here.X, Here.Y, FeetZ), 0.f, USwimWaterSubsystem::DeepReachBelow);
	if (!Water)
	{
		return;
	}
	const float Scale = GetBodyScale();
	const float Depth = Water->GetSurfaceZ() - FeetZ;
	if (bWalking)
	{
		// Walked in until the water is over the chest.
		if (SwimRules::ShouldSwim(Depth, Scale))
		{
			StartSwim(*Water, false);
		}
		return;
	}
	// Falling in: coming down, the feet well under, and no bed near enough to land on (a shallow pool takes the landing).
	const float Start = SwimRules::StartDepth(Scale);
	if (Move->Velocity.Z > 0.0 || Clock - SwimEndedClock < ReEnterSeconds || Depth < Start * FallInShare)
	{
		return;
	}
	float BedZ = 0.f;
	if (Depth < Start && FindSwimBed(Start - Depth, BedZ))
	{
		return;
	}
	StartSwim(*Water, true);
}

bool UPlayerLocomotionComponent::TrySwimIn(ASwimWater* Water)
{
	const UCharacterMovementComponent* Move = Movement.Get();
	if (!Water || bSwimming || !Move || !Character.IsValid())
	{
		return bSwimming;
	}
	const float Depth = Water->GetSurfaceZ() - static_cast<float>(GetFeetHeight());
	if (SwimRules::ShouldSwim(Depth, GetBodyScale()))
	{
		StartSwim(*Water, Move->IsFalling());
	}
	return bSwimming;
}

// ---------------------------------------------------------------------------
// In and out
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::StartSwim(ASwimWater& Water, bool bFromAir)
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move || bSwimming)
	{
		return;
	}
	EndSlide();
	Intent.StandUp();
	if (Owner->bIsCrouched || Move->bWantsToCrouch)
	{
		Owner->UnCrouch();
	}
	bSprinting = false;
	if (!JumpKeyAction.IsValid())
	{
		JumpKeyAction = LoadObject<UInputAction>(nullptr, JumpActionPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	// The movement flies for the swim (its own numbers kept to give back): no gravity, the water's drag, the swim's pace.
	const float EntryFallSpeed = static_cast<float>(FMath::Max(-Move->Velocity.Z, 0.0));
	FVector Velocity = Move->Velocity;
	if (bFromAir)
	{
		// The water takes most of a fall at once, and some of the run.
		Velocity.Z *= SwimRules::EntryKeep();
		Velocity.X *= 0.6;
		Velocity.Y *= 0.6;
	}
	SavedMaxFlySpeed = Move->MaxFlySpeed;
	SavedBrakingFlying = Move->BrakingDecelerationFlying;
	Move->MaxFlySpeed = SwimRules::SwimSpeed(BaseWalkSpeed, false);
	Move->BrakingDecelerationFlying = SwimRules::Braking();
	Move->SetMovementMode(MOVE_Flying);
	Move->Velocity = Velocity;

	SwimWater = &Water;
	bSwimming = true;
	bSwimFast = false;
	SwimBedClock = BedLookSeconds;
	SwimWakeTravel = 0.f;
	StrokePhase = 0.f;
	// No gun under water: a held trigger lets go now (the weapon manager refuses new presses while swimming).
	if (UWeaponManagerComponent* Manager = WeaponManager.Get())
	{
		Manager->StopFire();
	}
	if (bFromAir || EntryFallSpeed > 200.f)
	{
		SwimSplash(true, FMath::Clamp(EntryFallSpeed / SplashFullSpeed, 0.25f, 1.f));
	}
	else
	{
		// Waded in until it lifted the feet: a soft surge, no splash.
		LooterSound::PlayAt(this, SwimCue::SplashIn, FVector(Owner->GetActorLocation().X, Owner->GetActorLocation().Y, Water.GetSurfaceZ()), 0.35f);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Swim: in %s (%s)."), *Water.WaterId.ToString(), bFromAir ? TEXT("from the air") : TEXT("waded in"));
}

void UPlayerLocomotionComponent::EndSwim(ESwimExit Exit)
{
	if (!bSwimming)
	{
		return;
	}
	bSwimming = false;
	bSwimFast = false;
	SwimEndedClock = Clock;
	if (UCharacterMovementComponent* Move = Movement.Get())
	{
		Move->MaxFlySpeed = SavedMaxFlySpeed;
		Move->BrakingDecelerationFlying = SavedBrakingFlying;
		// Out by the feet or into the air, the movement falls: it lands on the bed in a frame, or carries the jump out.
		// (Something else that took the movement over already, a climb or a respawn, keeps it.)
		if (Move->MovementMode == MOVE_Flying)
		{
			Move->SetMovementMode(MOVE_Falling);
		}
	}
	UE_LOG(LogLooter, Verbose, TEXT("Swim: out (%s)."), Exit == ESwimExit::Foot ? TEXT("on foot") : TEXT("into the air"));
}

void UPlayerLocomotionComponent::SwimJump()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	const ASwimWater* Water = SwimWater.Get();
	if (!Owner || !Move || !Water)
	{
		return;
	}
	// Only at the surface: under water the key held rises instead (MoveSwim).
	const float Scale = GetBodyScale();
	const float Eye = bHaveEye && StandingEye > 1.f ? StandingEye : Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 1.8f;
	const float TargetFeet = SwimRules::FloatFeetZ(Water->GetSurfaceZ(), Eye, Scale);
	if (bHeadUnder || static_cast<float>(GetFeetHeight()) < TargetFeet - JumpOutReach * Scale)
	{
		return;
	}
	EndSwim(ESwimExit::Air);
	Move->Velocity.Z = SwimRules::JumpOutSpeed(Scale);
	SwimSplash(false, 0.6f);
	// A bank's lip, a jetty or a rock in front: climbed (the catch in the air tries again as the body rises).
	TryStartTraversal(true);
}

// ---------------------------------------------------------------------------
// The swim
// ---------------------------------------------------------------------------

void UPlayerLocomotionComponent::UpdateSwimStance(float DeltaTime)
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	bSprinting = false;
	if (Move->bWantsToCrouch)
	{
		Owner->UnCrouch();
	}
	// The sprint key swims faster, forward only.
	bSwimFast = Intent.WantsSprint() && IsMovingForward();
	const float Top = SwimRules::SwimSpeed(BaseWalkSpeed, bSwimFast);
	if (!FMath::IsNearlyEqual(Move->MaxFlySpeed, Top))
	{
		Move->MaxFlySpeed = Top;
	}
}

void UPlayerLocomotionComponent::MoveSwim(float DeltaTime)
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	ASwimWater* Water = SwimWater.Get();
	if (!Owner || !Move || !Water || Move->MovementMode != MOVE_Flying)
	{
		// The water went, or something else took the movement over (a climb, a respawn, a scene): the swim lets go.
		EndSwim(ESwimExit::Air);
		return;
	}
	const FVector Here = Owner->GetActorLocation();
	const float Surface = Water->GetSurfaceZ();
	const float FeetZ = static_cast<float>(GetFeetHeight());
	if (!Water->ContainsXY(FVector2D(Here.X, Here.Y)) || FeetZ > Surface + 5.f)
	{
		// Off over the water's edge, or lifted out of it.
		EndSwim(ESwimExit::Air);
		return;
	}
	const float Scale = GetBodyScale();

	// Up and down: the keys (and the look) at the vertical speed, never past the float height; no key, the float spring
	// (a gentle drift up from under, the eye eased onto the surface with a slow bob).
	const float Eye = bHaveEye && StandingEye > 1.f ? StandingEye : Owner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 1.8f;
	const float Bob = FMath::Sin(SwimClock * FloatBobRate) * FloatBobDepth * Scale;
	const float TargetFeet = SwimRules::FloatFeetZ(Surface, Eye, Scale) + Bob;
	const float Input = GetSwimVerticalInput();
	float WantUp = SwimRules::FloatSpeed(FeetZ, TargetFeet, Scale);
	if (Input > 0.f)
	{
		WantUp = FMath::Min(Input * SwimRules::VerticalSpeed(Scale), FMath::Max((TargetFeet - FeetZ) * SurfaceEase, WantUp));
	}
	else if (Input < 0.f)
	{
		WantUp = Input * SwimRules::VerticalSpeed(Scale);
	}
	Move->Velocity.Z = FMath::FInterpTo(static_cast<float>(Move->Velocity.Z), WantUp, DeltaTime, VerticalResponse);

	// The bed under the feet, a few times a second: shallow enough there, back on foot.
	SwimBedClock -= DeltaTime;
	if (SwimBedClock <= 0.f)
	{
		SwimBedClock = BedLookSeconds;
		float BedZ = 0.f;
		if (Input >= 0.f && FindSwimBed(SwimRules::BedReach(Scale), BedZ) && SwimRules::CanStand(Surface - BedZ, Scale))
		{
			EndSwim(ESwimExit::Foot);
			return;
		}
	}

	// The stroke: quicker the faster the swim, its push heard and seen.
	const float Speed = static_cast<float>(Move->Velocity.Size());
	const float Share = Speed / FMath::Max(SwimRules::SwimSpeed(BaseWalkSpeed, false), 1.f);
	SwimEffort = FMath::FInterpTo(SwimEffort, FMath::Clamp(Share, 0.f, 1.f), DeltaTime, 4.f);
	const float Before = StrokePhase;
	StrokePhase = FMath::Fmod(StrokePhase + DeltaTime * SwimRules::StrokeRate(SwimEffort), 1.f);
	if (SwimEffort >= StrokeEffortHeard && Crossed(Before, StrokePhase, SwimRules::StrokePushAt))
	{
		StrokePushed();
	}

	// A wake at the surface behind a swimmer on the move (FWaterRings' spacing, as a wading walk's).
	const float Flat = static_cast<float>(Move->Velocity.Size2D());
	if (bHeadUnder || Flat < FWaterRings::WakeMinSpeed)
	{
		SwimWakeTravel = 0.f;
		return;
	}
	SwimWakeTravel += Flat * DeltaTime;
	if (!FWaterRings::WakeDue(SwimWakeTravel, Flat, Scale))
	{
		return;
	}
	SwimWakeTravel = 0.f;
	if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		const FVector Along = FVector(Move->Velocity.X, Move->Velocity.Y, 0.0).GetSafeNormal();
		Effects.SpawnWake(FVector(Here.X, Here.Y, Surface), FVector::UpVector, Along, Flat, Scale);
	}
}

