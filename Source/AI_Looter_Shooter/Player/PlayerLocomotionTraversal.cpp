#include "Player/PlayerLocomotionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/SoundSurface.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"

// UPlayerLocomotionComponent's traversal: the jump key climbing onto ledges and vaulting fences (FTraversalProbe finds
// them, FPlayerTraversal plans the move, this carries the capsule along it), and a ledge caught in the air. The jump's
// forgiveness (coyote time, the buffer before landing) and nudging a wedged player free are in
// PlayerLocomotionJumpAssist.cpp; the eye's path through a move is read by UpdateCamera (PlayerLocomotionViewModel.cpp).

namespace
{
	/** The movement's custom mode while a move carries the capsule (it moves nothing in it; nothing else uses one). */
	constexpr uint8 TraversalMode = 1;

	/**
	 * The climb's own sounds (cloth, effort, the hands taking the weight). Not in LooterSoundCues.h yet: a cue with no
	 * sounds plays nothing, so these are safe to call before Main adds them.
	 */
	const TCHAR* const MantleCue = TEXT("Player.Mantle");
	const TCHAR* const VaultCue = TEXT("Player.Vault");

	/** How hard the gun dips on the kick spring as a move lands (cm/s): on the ledge, or down on the floor beyond. */
	constexpr float MantleLandKick = 14.f;
	constexpr float VaultLandKick = 30.f;

	TAutoConsoleVariable<bool> CVarDebugTraversal(TEXT("Looter.DebugTraversal"), false,
		TEXT("Draw what the jump key's ledge probe finds (the wall, the top, where the body would end) and the planned path, and log why a ledge was refused."));

	const TCHAR* KindName(ETraversalKind Kind)
	{
		switch (Kind)
		{
		case ETraversalKind::Mantle: return TEXT("mantle");
		case ETraversalKind::Vault: return TEXT("vault");
		case ETraversalKind::Unstick: return TEXT("unstick");
		default: return TEXT("none");
		}
	}
}

void UPlayerLocomotionComponent::JumpOrTraverse()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move || Traversal.IsActive())
	{
		return;
	}
	// Something to climb or vault in front comes first: the key at a ledge means "get up there".
	if (TryStartTraversal(true))
	{
		return;
	}
	if (Move->IsFalling())
	{
		if (CanCoyoteJump())
		{
			CoyoteJump();
		}
		else if (Owner->JumpMaxCount > 1)
		{
			// A character allowed more jumps in the air takes this one.
			Owner->Jump();
		}
		else
		{
			// Too late for an edge: kept for the landing, which jumps if it comes soon enough (UpdateJumpAssist).
			JumpBufferedClock = Clock;
		}
		return;
	}
	Owner->Jump();
}

bool UPlayerLocomotionComponent::TryStartTraversal(bool bJumpKey)
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move || Traversal.IsActive() || Owner->bIsCrouched || Slide.IsActive() || IsMovementHeld())
	{
		return false;
	}
	const bool bInAir = Move->IsFalling();
	if (!bInAir && !Move->IsMovingOnGround())
	{
		// Flying (a debug ghost) or swimming.
		return false;
	}
	const float Speed = static_cast<float>(Move->Velocity.Size2D());
	const bool bForward = IsMovingForward();
	FTraversalAsk Ask;
	Ask.Character = Owner;
	Ask.Facing = Owner->GetActorForwardVector();
	Ask.Heading = GetTraversalHeading();
	Ask.bInAir = bInAir;
	// A sprint vaults a low wall it could have climbed onto, and keeps its speed over it.
	Ask.bRunning = bSprinting && Speed >= BaseWalkSpeed * LooterTraversal::VaultRunShare;
	Ask.LastFloorZ = bHaveFloor ? LastFloorZ : static_cast<float>(GetFeetHeight());
	const float Rising = bInAir ? static_cast<float>(FMath::Max(Move->Velocity.Z, 0.0)) : 0.f;
	Ask.RiseLeft = Rising * Rising / (2.f * FMath::Max(FMath::Abs(Move->GetGravityZ()), 1.f));
	// After a mantle the player steps on at a walk if they push on, else stands; a vault keeps the run (or the walk).
	Ask.MantleExitSpeed = bForward ? FMath::Clamp(Speed * 0.6f, BaseWalkSpeed * 0.45f, BaseWalkSpeed) : 0.f;
	Ask.VaultExitSpeed = bForward ? FMath::Max(Speed, BaseWalkSpeed * 0.8f) : Speed * 0.5f;
	const FTraversalFind Found = FTraversalProbe::Find(Ask);
	LastTraversalRefusal = Found.Refusal;
	if (bJumpKey)
	{
		// Only for the key: in the air the catch looks every frame.
		DrawTraversalDebug(Found);
	}
	if (Found.Kind == ETraversalKind::None)
	{
		if (bJumpKey)
		{
			UE_LOG(LogLooter, Verbose, TEXT("Jump key: nothing to climb (%s)."), FTraversalProbe::RefusalName(Found.Refusal));
		}
		return false;
	}
	return StartTraversal(Found);
}

FVector UPlayerLocomotionComponent::GetTraversalHeading() const
{
	const ACharacter* Owner = Character.Get();
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = Owner->GetActorRightVector().GetSafeNormal2D();
	// The keys first: they say where the player means to go, even pressed up against the wall where the run has stopped.
	const FVector2D Keys = bHasMoveInput ? GetHeldMoveInput() : FVector2D::ZeroVector;
	if (!Keys.IsNearlyZero())
	{
		return (Forward * Keys.Y + Right * Keys.X).GetSafeNormal2D();
	}
	const FVector Run = Owner->GetVelocity();
	if (Run.SizeSquared2D() > FMath::Square(100.0))
	{
		return Run.GetSafeNormal2D();
	}
	if (!bHasMoveInput)
	{
		const FVector Input = Owner->GetLastMovementInputVector().GetSafeNormal2D();
		if (!Input.IsNearlyZero())
		{
			return Input;
		}
	}
	return Forward;
}

bool UPlayerLocomotionComponent::StartTraversal(const FTraversalFind& Found)
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	const UCapsuleComponent* Capsule = Owner ? Owner->GetCapsuleComponent() : nullptr;
	if (!Owner || !Move || !Capsule || Found.Kind == ETraversalKind::None)
	{
		return false;
	}
	const bool bVault = Found.Kind == ETraversalKind::Vault;
	const bool bUnstick = Found.Kind == ETraversalKind::Unstick;
	const FVector Direction = FVector(Found.Direction.X, Found.Direction.Y, 0.0).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Direction);

	FTraversalSetup Setup;
	Setup.Kind = Found.Kind;
	Setup.Origin = Owner->GetActorLocation();
	Setup.Direction = Direction;
	Setup.Radius = Capsule->GetScaledCapsuleRadius();
	Setup.HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	Setup.Velocity = Move->Velocity;
	Setup.GravityZ = Move->IsFalling() ? Move->GetGravityZ() : 0.f;
	Setup.NearFace = Found.NearFace;
	Setup.FarFace = Found.FarFace;
	Setup.TopZ = Found.TopZ;
	const FVector Offset = Found.End - Setup.Origin;
	Setup.EndAlong = static_cast<float>(Offset | Direction);
	Setup.EndSide = static_cast<float>(Offset | Right);
	Setup.EndFeetZ = Found.EndFeetZ;
	Setup.ExitSpeed = Found.ExitSpeed;
	Setup.Duration = Found.Duration;
	Setup.Tuck = bVault ? LooterTraversal::VaultTuck : LooterTraversal::MantleTuck;

	// The eye sets off from where it is, at the speed and acceleration it has (the body's and its own easing's), and ends
	// a little under standing (the give), rising back after. Before the eye has ever been placed, it's the camera itself.
	const float Feet = static_cast<float>(GetFeetHeight());
	const float Standing = StandingEye > 1.f ? StandingEye : Setup.HalfHeight * 1.72f;
	const float EyeNow = GetEyeHeight();
	const UCameraComponent* View = Camera.Get();
	if (bHaveEye)
	{
		Setup.EyeZ = Feet + EyeNow;
		Setup.EyeSpeed = static_cast<float>(Move->Velocity.Z) + EyeHeight.GetVelocity();
		Setup.EyeAcceleration = Setup.GravityZ + EyeHeight.GetAcceleration();
	}
	else
	{
		Setup.EyeZ = View ? static_cast<float>(View->GetComponentLocation().Z) : Feet + Standing;
		Setup.EyeSpeed = static_cast<float>(Move->Velocity.Z);
		Setup.EyeAcceleration = Setup.GravityZ;
	}
	Setup.EyeEndHeight = bUnstick ? Setup.EyeZ - Feet : Standing - (bVault ? LooterTraversal::VaultEyeDip : LooterTraversal::MantleEyeDip);
	Setup.EyeForward = View ? static_cast<float>((View->GetComponentLocation() - Setup.Origin) | Direction) : 0.f;
	if (!Traversal.Plan(Setup))
	{
		LastTraversalRefusal = ETraversalRefusal::Blocked;
		UE_LOG(LogLooter, Verbose, TEXT("No clear %s over the edge."), KindName(Found.Kind));
		return false;
	}

	// The move has the capsule now: the movement stands aside in its custom mode, which moves nothing, and lets go of the
	// floor it stood on (a moving one would carry the body off the path).
	EndSlide();
	if (!bVault)
	{
		bSprinting = false;
	}
	Owner->SetBase(static_cast<UPrimitiveComponent*>(nullptr));
	Move->SetMovementMode(MOVE_Custom, TraversalMode);
	Move->Velocity = Setup.Velocity;
	TraversalLocation = Owner->GetActorLocation();
	TraversalEyeHeight = Setup.EyeZ - Feet;
	TraversalHeight = Found.TopZ - Feet;
	TraversalTop = Found.TopHit;
	TraversalFloor = Found.FloorHit;
	PendingFeetJump = 0.f;
	bWasGrounded = false;
	bLeftByWalking = false;
	JumpBufferedClock = -100.0;
	if (!bUnstick)
	{
		PlayTraversalSounds(false);
		OnTraversalStarted.Broadcast(Found.Kind, TraversalHeight);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Traversal: %s, %.0f cm up, %.2f s, ending %.0f cm on at %.0f cm/s."), KindName(Found.Kind),
		TraversalHeight, Traversal.GetDuration(), Setup.EndAlong, Setup.ExitSpeed);

#if ENABLE_DRAW_DEBUG
	if (CVarDebugTraversal.GetValueOnGameThread())
	{
		// The body's path (green) and the eye's (cyan).
		const double Length = Traversal.GetDuration();
		for (int32 Index = 0; Index < 24; ++Index)
		{
			const double From = Length * Index / 24.0;
			const double To = Length * (Index + 1) / 24.0;
			DrawDebugLine(GetWorld(), Traversal.CenterAt(From), Traversal.CenterAt(To), FColor::Green, false, 3.f);
			const FVector EyeFrom = Traversal.CenterAt(From);
			const FVector EyeTo = Traversal.CenterAt(To);
			DrawDebugLine(GetWorld(), FVector(EyeFrom.X, EyeFrom.Y, Traversal.EyeAt(From)), FVector(EyeTo.X, EyeTo.Y, Traversal.EyeAt(To)),
				FColor::Cyan, false, 3.f);
		}
	}
#endif
	return true;
}

void UPlayerLocomotionComponent::UpdateTraversal(float DeltaTime)
{
	Clock += DeltaTime;
	if (Traversal.IsActive())
	{
		AdvanceTraversal(DeltaTime);
		return;
	}
	UpdateJumpAssist();
	// A ledge met in the air with the keys pushing on toward it is caught.
	const UCharacterMovementComponent* Move = Movement.Get();
	if (!Traversal.IsActive() && Move->IsFalling() && IsMovingForward() && TryStartTraversal(false))
	{
		return;
	}
	if (!Traversal.IsActive())
	{
		UpdateStuck();
	}
}

void UPlayerLocomotionComponent::AdvanceTraversal(float DeltaTime)
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	// Something else moved the player or took the movement over (a respawn, fall recovery, a teleport): let go there.
	if (Move->MovementMode != MOVE_Custom || Move->CustomMovementMode != TraversalMode || !Owner->GetActorLocation().Equals(TraversalLocation, 1.0))
	{
		AbortTraversal();
		return;
	}
	Traversal.Advance(DeltaTime);
	// Straight to the planned place: the path was checked clear before it began, and a sweep catching the ledge's corner
	// would leave the body short of it.
	Owner->SetActorLocation(Traversal.GetCenter(), false, nullptr, ETeleportType::None);
	TraversalLocation = Owner->GetActorLocation();
	Move->Velocity = Traversal.GetVelocity();
	Move->UpdateComponentVelocity();
	if (!Traversal.IsActive())
	{
		FinishTraversal();
	}
}

void UPlayerLocomotionComponent::FinishTraversal()
{
	ACharacter* Owner = Character.Get();
	UCharacterMovementComponent* Move = Movement.Get();
	if (!Owner || !Move)
	{
		Traversal.Stop();
		return;
	}
	const ETraversalKind Kind = Traversal.GetKind();
	const bool bUnstick = Kind == ETraversalKind::Unstick;
	const FVector Exit = Traversal.GetVelocity();
	// The movement takes over: walking on the top or the floor beyond (finding the floor as it does), or falling the last
	// little way after an unstick.
	Move->SetMovementMode(bUnstick ? MOVE_Falling : MOVE_Walking);
	Move->Velocity = bUnstick ? FVector::ZeroVector : FVector(Exit.X, Exit.Y, 0.0);
	// The eye's own curve takes over where the move left it: at rest, a little under standing, rising back (the give).
	EyeHeight.Reset(Traversal.GetEyeZ() - static_cast<float>(GetFeetHeight()));
	TraversalEyeHeight = EyeHeight.GetValue();
	LastFloorZ = static_cast<float>(GetFeetHeight());
	bHaveFloor = true;
	FallSpeed = 0.f;
	StuckAnchor = Owner->GetActorLocation();
	StuckSinceClock = Clock;
	if (!bUnstick)
	{
		KickVelocity -= Kind == ETraversalKind::Vault ? VaultLandKick : MantleLandKick;
		PlayTraversalSounds(true);
		OnTraversalLanded.Broadcast(Kind, TraversalHeight);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Traversal over: %s, on at %.0f cm/s."), KindName(Kind), Move->Velocity.Size2D());
}

void UPlayerLocomotionComponent::AbortTraversal()
{
	if (!Traversal.IsActive())
	{
		return;
	}
	Traversal.Stop();
	if (UCharacterMovementComponent* Move = Movement.Get(); Move && Move->MovementMode == MOVE_Custom && Move->CustomMovementMode == TraversalMode)
	{
		Move->SetMovementMode(MOVE_Falling);
	}
	EyeHeight.Reset(TraversalEyeHeight);
	UE_LOG(LogLooter, Verbose, TEXT("Traversal let go: something else moved the player."));
}

void UPlayerLocomotionComponent::PlayTraversalSounds(bool bLanding) const
{
	ACharacter* Owner = Character.Get();
	if (!Owner)
	{
		return;
	}
	const bool bVault = Traversal.GetKind() == ETraversalKind::Vault;
	if (!bLanding)
	{
		// The body's effort and cloth, and the hands meeting the top, on its own surface.
		LooterSound::PlayAttached(bVault ? VaultCue : MantleCue, Owner->GetRootComponent());
		if (TraversalTop.GetComponent())
		{
			LooterSound::PlayAt(this, SoundSurface::FootstepCue(SoundSurface::Of(TraversalTop)), TraversalTop.ImpactPoint, 0.55f, 1.1f);
		}
		return;
	}
	// The feet on the top or the floor beyond; a vault comes down with a soft landing as well.
	const FVector Feet(Owner->GetActorLocation().X, Owner->GetActorLocation().Y, GetFeetHeight());
	const ESoundSurface Surface = TraversalFloor.GetComponent() ? SoundSurface::Of(TraversalFloor) : ESoundSurface::Dirt;
	LooterSound::PlayAt(this, SoundSurface::FootstepCue(Surface), Feet, bVault ? 0.9f : 0.7f);
	if (bVault)
	{
		LooterSound::PlayAt(this, LooterSoundCue::Land, Feet, 0.45f, 1.04f);
	}
}

void UPlayerLocomotionComponent::DrawTraversalDebug(const FTraversalFind& Found) const
{
#if ENABLE_DRAW_DEBUG
	const ACharacter* Owner = Character.Get();
	UWorld* World = GetWorld();
	if (!CVarDebugTraversal.GetValueOnGameThread() || !Owner || !World)
	{
		return;
	}
	constexpr float Seconds = 3.f;
	if (Found.WallHit.bBlockingHit)
	{
		DrawDebugPoint(World, Found.WallHit.ImpactPoint, 12.f, FColor::Orange, false, Seconds);
		DrawDebugLine(World, Found.WallHit.ImpactPoint, Found.WallHit.ImpactPoint + FVector(Found.WallHit.ImpactNormal) * 40.0, FColor::Orange, false, Seconds);
	}
	if (Found.TopHit.bBlockingHit)
	{
		DrawDebugPoint(World, Found.TopHit.ImpactPoint, 12.f, FColor::Cyan, false, Seconds);
	}
	if (Found.Kind != ETraversalKind::None)
	{
		const UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();
		DrawDebugCapsule(World, Found.End, Capsule->GetScaledCapsuleHalfHeight(), Capsule->GetScaledCapsuleRadius(), FQuat::Identity, FColor::Green,
			false, Seconds);
	}
	UE_LOG(LogLooter, Log, TEXT("Ledge probe: %s (%s), top %.0f cm over the feet, faces at %.0f and %.0f cm."), KindName(Found.Kind),
		FTraversalProbe::RefusalName(Found.Refusal), Found.TopZ - GetFeetHeight(), Found.NearFace, Found.FarFace);
#endif
}
