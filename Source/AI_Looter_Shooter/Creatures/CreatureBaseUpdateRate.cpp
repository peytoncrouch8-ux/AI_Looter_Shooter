#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<bool> CVarCreatureUpdateRates(TEXT("Looter.Creatures.UpdateRates"), true,
		TEXT("Distant creatures update less often, and stop posing off screen (0: every creature updates every frame, to compare)."));

	/** Drawn in a view this lately (seconds) counts as on screen. */
	constexpr float CreatureOnScreenTolerance = 0.25f;
}

bool ACreatureBase::NeedsFullRate(ECreatureState CreatureState, bool bHasTarget, bool bRecentlyHurt)
{
	return CreatureState == ECreatureState::Chase || CreatureState == ECreatureState::Attack || bHasTarget || bRecentlyHurt;
}

void ACreatureBase::TickUpdateRate(float DeltaSeconds)
{
	FullRateTime = FMath::Max(0.f, FullRateTime - DeltaSeconds);
	UpdateRateCheckTime -= DeltaSeconds;
	// A frozen body wakes as soon as a view comes near it, not at the next check: it would show the pose it froze in.
	if (UpdateRateCheckTime <= 0.f || (bPoseFrozen && MeasureViewers().bOnScreen))
	{
		RefreshUpdateRate();
	}
}

void ACreatureBase::WakeUpdateRate()
{
	if (UpdateInterval > 0.f || bPoseFrozen)
	{
		RefreshUpdateRate();
	}
}

void ACreatureBase::RefreshUpdateRate()
{
	UpdateRateCheckTime = UpdateRate.CheckInterval;

	float Interval = 0.f;
	bool bFreeze = false;
	const bool bEngaged = NeedsFullRate(State, Target.IsValid(), FullRateTime > 0.f);
	if (CVarCreatureUpdateRates.GetValueOnGameThread() && !bEngaged)
	{
		const FViewerMeasure Viewers = MeasureViewers();
		// No player to measure from (simulating in the editor): every frame, as it always was.
		if (Viewers.Distance < TNumericLimits<float>::Max())
		{
			Interval = UpdateRate.IntervalFor(Viewers.Distance, bEngaged, Viewers.bOnScreen);
			bFreeze = UpdateRate.ShouldFreezePose(Viewers.Distance, bEngaged, Viewers.bOnScreen);
		}
	}
	ApplyUpdateInterval(Interval);
	SetPoseFrozen(bFreeze);
}

void ACreatureBase::ApplyUpdateInterval(float Interval)
{
	if (FMath::IsNearlyEqual(Interval, UpdateInterval))
	{
		return;
	}
	UE_LOG(LogLooter, VeryVerbose, TEXT("%s: updates every %.2f s"), *GetName(), Interval);
	UpdateInterval = Interval;

	// The brain, the movement, the body's pose and the AI controller all change rate together, and "AndCoolDown" restarts
	// each one's wait from now: a faster rate starts at once instead of after the slow wait already queued, and they stay
	// on the same frames, so the movement takes the input the brain gave it last update and the pose shows this update's
	// position. Movement sub-steps long updates itself (the walking and falling physics split them into short moves).
	PrimaryActorTick.UpdateTickIntervalAndCoolDown(Interval);
	GetCharacterMovement()->SetComponentTickIntervalAndCooldown(Interval);
	GetMesh()->SetComponentTickIntervalAndCooldown(Interval);
	AController* Brain = GetController();
	if (Brain && !Brain->IsA<APlayerController>())
	{
		Brain->PrimaryActorTick.UpdateTickIntervalAndCoolDown(Interval);
	}
}

void ACreatureBase::SetPoseFrozen(bool bFrozen)
{
	if (bFrozen == bPoseFrozen)
	{
		return;
	}
	bPoseFrozen = bFrozen;
	USkeletalMeshComponent* Body = GetMesh();
	if (bFrozen)
	{
		// Out of view and far: the subclass stops posing, and the mesh stops refreshing its bones (the same frozen pose
		// over and over) unless it's drawn, counting only views: a shadow map draws many creatures behind the player.
		// The hit zones keep following the capsule in the pose it froze in.
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		Body->bUseScreenRenderStateForUpdate = true;
	}
	else
	{
		Body->VisibilityBasedAnimTickOption = AwakeAnimTickOption;
		Body->bUseScreenRenderStateForUpdate = bAwakeUsesScreenRenderState;
		OnPoseThawed();
	}
}

ACreatureBase::FViewerMeasure ACreatureBase::MeasureViewers() const
{
	FViewerMeasure Result;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Result;
	}
	// The pawn and the camera both: the third-person camera trails the player, and the menu and the view tour have a
	// camera but no player.
	const FVector Here = GetActorLocation();
	const FBoxSphereBounds& Body = GetMesh()->Bounds;
	const float BodyRadius = static_cast<float>(Body.SphereRadius);
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Player = It->Get();
		if (!Player || !Player->IsLocalController())
		{
			continue;
		}
		if (const APawn* Pawn = Player->GetPawn())
		{
			Result.Distance = FMath::Min(Result.Distance, static_cast<float>(FVector::Dist(Here, Pawn->GetActorLocation())));
		}
		const APlayerCameraManager* Camera = Player->PlayerCameraManager.Get();
		if (!Camera)
		{
			continue;
		}
		const FVector Eye = Camera->GetCameraLocation();
		const FVector Looking = Camera->GetCameraRotation().Vector();
		const float FieldOfView = Camera->GetFOVAngle();
		float FromCamera = static_cast<float>(FVector::Dist(Here, Eye));
		// In view whether or not a wall hides it: drawn-on-screen time stops behind cover too, and a body frozen there would
		// show its stale pose as it stepped out. A scope's view counts as wide as an unzoomed one, so lowering the gun
		// doesn't uncover frozen bodies all around the sight.
		Result.bOnScreen |= FCreatureUpdateRate::IsInView(Eye, Looking, FMath::Max(FieldOfView, UpdateRate.ReferenceFieldOfView),
			Body.Origin, BodyRadius, UpdateRate.ViewMargin);
		// Through a scope it looks as near as the zoom makes it, and must move as smoothly as a creature that near.
		const float Zoom = UpdateRate.ZoomFor(FieldOfView);
		if (Zoom > 1.f && FCreatureUpdateRate::IsInView(Eye, Looking, FieldOfView, Body.Origin, BodyRadius, UpdateRate.ViewMargin))
		{
			FromCamera /= Zoom;
		}
		Result.Distance = FMath::Min(Result.Distance, FromCamera);
	}
	// Drawn on screen lately counts too, for views that aren't a player's camera. On-screen time only: GetLastRenderTime
	// also counts shadow maps, which draw plenty of creatures out of view.
	Result.bOnScreen |= World->TimeSince(GetMesh()->GetLastRenderTimeOnScreen()) <= CreatureOnScreenTolerance;
	return Result;
}
