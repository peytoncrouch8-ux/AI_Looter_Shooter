#include "Player/AimAssist.h"
#include "Components/CapsuleComponent.h"
#include "Creatures/CreatureBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Settings/ControlSettingsSubsystem.h"

// The gamepad's look and its aim assist (AimAssist.h): the math first, then the cvars, then the one function that reads the world.

namespace
{
	TAutoConsoleVariable<float> CVarPadYaw(TEXT("Looter.Look.PadDegreesPerSecond"), AimAssist::Default::YawDegreesPerSecond,
		TEXT("Degrees a second a full push of the right stick turns the view round (times the Look sensitivity setting). 0: the old per-frame stick."));
	TAutoConsoleVariable<float> CVarPadPitch(TEXT("Looter.Look.PadPitchDegreesPerSecond"), AimAssist::Default::PitchDegreesPerSecond,
		TEXT("Degrees a second a full push of the right stick turns the view up or down."));
	TAutoConsoleVariable<float> CVarPadDeadZone(TEXT("Looter.Look.PadDeadZone"), AimAssist::Default::StickDeadZone,
		TEXT("The right stick's dead zone (its length, 0-1); past it the push starts from nothing."));
	TAutoConsoleVariable<float> CVarPadCurve(TEXT("Looter.Look.PadCurve"), AimAssist::Default::StickCurve,
		TEXT("The right stick's response power past the dead zone: 1 is linear, higher is finer for small pushes."));
	TAutoConsoleVariable<int32> CVarAssist(TEXT("Looter.AimAssist"), 1,
		TEXT("1: gamepad aim assist (friction and magnetism; the Aim assist setting turns it off for a player). 0: off for everyone."));
	TAutoConsoleVariable<float> CVarFriction(TEXT("Looter.AimAssist.Friction"), AimAssist::Default::Friction,
		TEXT("The share of the look kept while the crosshair is on a creature (0.6 = 40% slower). 1: no friction."));
	TAutoConsoleVariable<float> CVarMagnetism(TEXT("Looter.AimAssist.Magnetism"), AimAssist::Default::MagnetDegreesPerSecond,
		TEXT("Degrees a second, at most, that the view is pulled toward a creature the crosshair is nearly on, while the stick moves. 0: none."));
	TAutoConsoleVariable<float> CVarCone(TEXT("Looter.AimAssist.Cone"), AimAssist::Default::MagnetConeDegrees,
		TEXT("How close to a creature's middle (degrees) the crosshair must be for the magnet to work."));
	TAutoConsoleVariable<float> CVarMargin(TEXT("Looter.AimAssist.Margin"), AimAssist::Default::MarginCm,
		TEXT("How far past a creature's capsule (cm) the crosshair still counts as on it for the friction."));
	TAutoConsoleVariable<float> CVarRange(TEXT("Looter.AimAssist.Range"), AimAssist::Default::RangeCm,
		TEXT("How far away (cm) a creature is still assisted."));

	/** Components of a scale that is zero or unreadable count as 1, so a bad config can't divide by nothing. */
	double SafeScale(double Scale)
	{
		return FMath::Abs(Scale) > UE_KINDA_SMALL_NUMBER ? Scale : 1.0;
	}

	/** The creatures of one world, found by walking its actors at most every Default::RefreshSeconds. */
	struct FCreatureList
	{
		TWeakObjectPtr<UWorld> World;
		double Stamp = -1.0e9;
		TArray<TWeakObjectPtr<ACreatureBase>> Creatures;
	};
	FCreatureList GList;

	const TArray<TWeakObjectPtr<ACreatureBase>>& CreaturesOf(UWorld& World, float RefreshSeconds)
	{
		const double Now = FPlatformTime::Seconds();
		if (GList.World.Get() != &World || AimAssist::RefreshDue(Now, GList.Stamp, RefreshSeconds))
		{
			GList.World = &World;
			GList.Stamp = Now;
			GList.Creatures.Reset();
			for (TActorIterator<ACreatureBase> It(&World); It; ++It)
			{
				GList.Creatures.Add(*It);
			}
		}
		return GList.Creatures;
	}

	/** Nothing solid between the camera and Creature's middle (the player's pawn, what it holds and the creature itself aside). */
	bool SightClear(const UWorld& World, const APlayerController& Player, const FVector& Eye, const ACreatureBase& Creature, const FVector& Point)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(AimAssistSight), false, &Creature);
		if (const APawn* Pawn = Player.GetPawn())
		{
			Params.AddIgnoredActor(Pawn);
			TArray<AActor*> Held;
			Pawn->GetAttachedActors(Held, true, true);
			Params.AddIgnoredActors(Held);
		}
		return !World.LineTraceTestByChannel(Eye, Point, ECC_Visibility, Params);
	}
}

// ---------------------------------------------------------------------------
// Tunables
// ---------------------------------------------------------------------------

AimAssist::FTuning AimAssist::LiveTuning()
{
	FTuning Tuning;
	Tuning.bEnabled = CVarAssist.GetValueOnAnyThread() != 0;
	Tuning.StickDeadZone = FMath::Clamp(CVarPadDeadZone.GetValueOnAnyThread(), 0.f, 0.9f);
	Tuning.StickCurve = FMath::Clamp(CVarPadCurve.GetValueOnAnyThread(), 0.5f, 4.f);
	Tuning.YawDegreesPerSecond = FMath::Clamp(CVarPadYaw.GetValueOnAnyThread(), 0.f, 1000.f);
	Tuning.PitchDegreesPerSecond = FMath::Clamp(CVarPadPitch.GetValueOnAnyThread(), 0.f, 1000.f);
	Tuning.Friction = FMath::Clamp(CVarFriction.GetValueOnAnyThread(), 0.05f, 1.f);
	Tuning.MagnetDegreesPerSecond = FMath::Clamp(CVarMagnetism.GetValueOnAnyThread(), 0.f, 30.f);
	Tuning.MagnetConeDegrees = FMath::Clamp(CVarCone.GetValueOnAnyThread(), 0.f, 10.f);
	Tuning.MarginCm = FMath::Clamp(CVarMargin.GetValueOnAnyThread(), 0.f, 200.f);
	Tuning.RangeCm = FMath::Clamp(CVarRange.GetValueOnAnyThread(), 500.f, 20000.f);
	return Tuning;
}

// ---------------------------------------------------------------------------
// The stick
// ---------------------------------------------------------------------------

FVector2D AimAssist::ShapeStick(const FVector2D& Stick, float DeadZone, float Curve)
{
	const double Length = Stick.Size();
	if (!FMath::IsFinite(Length) || Length <= 0.0)
	{
		return FVector2D::ZeroVector;
	}
	// Round, not per axis: a diagonal push keeps its angle, and a thumb resting off-centre doesn't drag one axis.
	const double Zone = FMath::Clamp(static_cast<double>(DeadZone), 0.0, 0.95);
	if (Length <= Zone)
	{
		return FVector2D::ZeroVector;
	}
	const double Past = (FMath::Min(Length, 1.0) - Zone) / (1.0 - Zone);
	const double Turn = FMath::Pow(Past, FMath::Max(static_cast<double>(Curve), 0.1));
	return Stick / Length * Turn;
}

FVector2D AimAssist::StickTurn(const FVector2D& Shaped, float YawPerSecond, float PitchPerSecond, float DeltaSeconds, float MaxStepSeconds)
{
	if (!FMath::IsFinite(DeltaSeconds))
	{
		return FVector2D::ZeroVector;
	}
	const float Step = FMath::Clamp(DeltaSeconds, 0.f, FMath::Max(MaxStepSeconds, 0.f));
	return FVector2D(Shaped.X * YawPerSecond * Step, Shaped.Y * PitchPerSecond * Step);
}

FVector2D AimAssist::InvertY(const FVector2D& Input, bool bInvert)
{
	return bInvert ? FVector2D(Input.X, -Input.Y) : Input;
}

FVector2D AimAssist::LegacyInputScales(const APlayerController& Player)
{
	// The engine's own rule (APlayerController::AddYawInput): the controller's scales only count with the setting on.
	if (!GetDefault<UInputSettings>()->bEnableLegacyInputScales)
	{
		return FVector2D(1.0, 1.0);
	}
	return FVector2D(Player.InputYawScale_DEPRECATED, Player.InputPitchScale_DEPRECATED);
}

FVector2D AimAssist::DegreesToInput(const FVector2D& StickDegrees, const FVector2D& Scales)
{
	// Only the size: the stick's own signs were set against the engine's (the input mapping negates Y for the -2.5).
	return FVector2D(StickDegrees.X / FMath::Abs(SafeScale(Scales.X)), StickDegrees.Y / FMath::Abs(SafeScale(Scales.Y)));
}

FVector2D AimAssist::RotationToInput(const FVector2D& Rotation, const FVector2D& Scales)
{
	return FVector2D(Rotation.X / SafeScale(Scales.X), Rotation.Y / SafeScale(Scales.Y));
}

// ---------------------------------------------------------------------------
// Aim assist
// ---------------------------------------------------------------------------

bool AimAssist::CanAssist(bool bDead, bool bPassive, bool bHidden, bool bSolid)
{
	return !bDead && !bPassive && !bHidden && bSolid;
}

float AimAssist::RayGap(const FVector& Eye, const FVector& Aim, const FTarget& Target, float RangeCm)
{
	constexpr float Far = 1.0e6f;
	const FVector Direction = Aim.GetSafeNormal();
	if (Direction.IsNearlyZero() || RangeCm <= 0.f)
	{
		return Far;
	}
	// Behind the eye, or beside it: not what the crosshair is on, however close it stands.
	if (((Target.Center - Eye) | Direction) <= 0.0)
	{
		return Far;
	}
	// The capsule is its axis (a vertical segment) swelled by its radius.
	const double Reach = FMath::Max(Target.HalfHeight - Target.Radius, 0.f);
	const FVector Low = Target.Center - FVector(0.0, 0.0, Reach);
	const FVector High = Target.Center + FVector(0.0, 0.0, Reach);
	FVector OnRay = FVector::ZeroVector;
	FVector OnAxis = FVector::ZeroVector;
	FMath::SegmentDistToSegmentSafe(Eye, Eye + Direction * RangeCm, Low, High, OnRay, OnAxis);
	return static_cast<float>(FVector::Dist(OnRay, OnAxis)) - Target.Radius;
}

bool AimAssist::OnTarget(const FVector& Eye, const FVector& Aim, const FTarget& Target, float MarginCm, float RangeCm)
{
	return RayGap(Eye, Aim, Target, RangeCm) <= FMath::Max(MarginCm, 0.f);
}

float AimAssist::FrictionScale(bool bOnTarget, float Friction)
{
	// Never to nothing: a stick on a creature slows, it doesn't stick.
	return bOnTarget ? FMath::Clamp(Friction, 0.05f, 1.f) : 1.f;
}

float AimAssist::AngleTo(const FVector& Eye, const FVector& Aim, const FVector& Point)
{
	const FVector To = (Point - Eye).GetSafeNormal();
	const FVector Direction = Aim.GetSafeNormal();
	if (To.IsNearlyZero() || Direction.IsNearlyZero())
	{
		return 180.f;
	}
	return static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(To | Direction, -1.0, 1.0))));
}

FVector2D AimAssist::TurnTo(const FVector& Eye, const FVector& Aim, const FVector& Point)
{
	const FVector To = (Point - Eye).GetSafeNormal();
	const FVector Direction = Aim.GetSafeNormal();
	if (To.IsNearlyZero() || Direction.IsNearlyZero())
	{
		return FVector2D::ZeroVector;
	}
	const FRotator Delta = (To.Rotation() - Direction.Rotation()).GetNormalized();
	return FVector2D(Delta.Yaw, Delta.Pitch);
}

int32 AimAssist::PickMagnetTarget(const FVector& Eye, const FVector& Aim, TArrayView<const FTarget> Targets, float ConeDegrees, float RangeCm)
{
	int32 Best = INDEX_NONE;
	float BestAngle = 0.f;
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		const FTarget& Target = Targets[Index];
		if (FVector::DistSquared(Eye, Target.Center) > FMath::Square(static_cast<double>(RangeCm)))
		{
			continue;
		}
		const float Angle = AngleTo(Eye, Aim, Target.Center);
		// Strictly nearer wins, so the first listed keeps a tie (the pull never flips between two equals frame to frame).
		if (Angle <= ConeDegrees && (Best == INDEX_NONE || Angle < BestAngle))
		{
			Best = Index;
			BestAngle = Angle;
		}
	}
	return Best;
}

FVector2D AimAssist::MagnetPull(const FVector& Eye, const FVector& Aim, const FVector& Point, bool bStickMoving, float MagnetDps,
	float ConeDegrees, float SettleDegrees, float DeltaSeconds)
{
	if (!bStickMoving || MagnetDps <= 0.f || DeltaSeconds <= 0.f || !FMath::IsFinite(DeltaSeconds))
	{
		return FVector2D::ZeroVector;
	}
	const float Angle = AngleTo(Eye, Aim, Point);
	if (Angle > ConeDegrees)
	{
		return FVector2D::ZeroVector;
	}
	const FVector2D Way = TurnTo(Eye, Aim, Point);
	const double WayLength = Way.Size();
	if (WayLength < UE_KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}
	// Full strength from the settle angle out, easing to nothing at the middle: the crosshair arrives and stays, and the pull
	// never carries it past (a step is at most the way left).
	const double Ease = FMath::Clamp(static_cast<double>(Angle) / FMath::Max(static_cast<double>(SettleDegrees), 0.01), 0.0, 1.0);
	const double Step = FMath::Min(static_cast<double>(MagnetDps) * Ease * DeltaSeconds, WayLength);
	return Way / WayLength * Step;
}

bool AimAssist::RefreshDue(double Now, double LastRefresh, float IntervalSeconds)
{
	// A clock that went backwards (a new world, a level change) is due too.
	return Now < LastRefresh || Now - LastRefresh >= static_cast<double>(IntervalSeconds);
}

// ---------------------------------------------------------------------------
// The player's frame
// ---------------------------------------------------------------------------

AimAssist::FFrame AimAssist::ForPlayer(const APlayerController& Player, bool bStickMoving, float DeltaSeconds)
{
	FFrame Frame;
	UWorld* World = Player.GetWorld();
	const FTuning Tuning = LiveTuning();
	// Friction and magnetism both work only while the stick turns the view, so a still stick costs nothing here.
	if (!Tuning.bEnabled || !bStickMoving || !World || !UControlSettingsSubsystem::IsAimAssistFor(&Player))
	{
		return Frame;
	}

	FVector Eye = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	Player.GetPlayerViewPoint(Eye, ViewRotation);
	const FVector Aim = ViewRotation.Vector();

	// The creatures that can be assisted now and are in range, from the list found at most ten times a second.
	TArray<FTarget, TInlineAllocator<24>> Targets;
	TArray<const ACreatureBase*, TInlineAllocator<24>> Creatures;
	for (const TWeakObjectPtr<ACreatureBase>& Weak : CreaturesOf(*World, Tuning.RefreshSeconds))
	{
		const ACreatureBase* Creature = Weak.Get();
		const UCapsuleComponent* Capsule = Creature ? Creature->GetCapsuleComponent() : nullptr;
		if (!Capsule || !CanAssist(Creature->IsDead(), Creature->IsPassive(), Creature->IsHidden(),
			Capsule->GetCollisionEnabled() != ECollisionEnabled::NoCollision))
		{
			continue;
		}
		FTarget Target;
		Target.Center = Capsule->GetComponentLocation();
		Target.Radius = Capsule->GetScaledCapsuleRadius();
		Target.HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		if (FVector::DistSquared(Eye, Target.Center) <= FMath::Square(static_cast<double>(Tuning.RangeCm + Target.HalfHeight)))
		{
			Targets.Add(Target);
			Creatures.Add(Creature);
		}
	}
	if (Targets.IsEmpty())
	{
		return Frame;
	}

	// The creature the crosshair is on (the nearest, if it crosses several) and the one the magnet would pull toward.
	int32 OnIndex = INDEX_NONE;
	double OnDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		const double Distance = FVector::DistSquared(Eye, Targets[Index].Center);
		if (Distance < OnDistance && OnTarget(Eye, Aim, Targets[Index], Tuning.MarginCm, Tuning.RangeCm))
		{
			OnIndex = Index;
			OnDistance = Distance;
		}
	}
	const int32 MagnetIndex = PickMagnetTarget(Eye, Aim, Targets, Tuning.MagnetConeDegrees, Tuning.RangeCm);

	// One sight line per creature that would be assisted (usually the same one for both), so a wall in between gives no help.
	int32 CheckedIndex = INDEX_NONE;
	bool bCheckedClear = false;
	auto InSight = [&](int32 Index)
	{
		if (Index != CheckedIndex)
		{
			CheckedIndex = Index;
			bCheckedClear = SightClear(*World, Player, Eye, *Creatures[Index], Targets[Index].Center);
		}
		return bCheckedClear;
	};

	if (OnIndex != INDEX_NONE && InSight(OnIndex))
	{
		Frame.bOnTarget = true;
		Frame.FrictionScale = FrictionScale(true, Tuning.Friction);
	}
	if (MagnetIndex != INDEX_NONE && InSight(MagnetIndex))
	{
		const float Step = FMath::Clamp(DeltaSeconds, 0.f, Tuning.MaxStepSeconds);
		Frame.Pull = MagnetPull(Eye, Aim, Targets[MagnetIndex].Center, true, Tuning.MagnetDegreesPerSecond, Tuning.MagnetConeDegrees,
			Tuning.MagnetSettleDegrees, Step);
	}
	return Frame;
}
