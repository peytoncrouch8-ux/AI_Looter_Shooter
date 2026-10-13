// AOreLift: the run under way, a call's or a ride's: its shape from the cable, the cage hung where the motion has it, each
// moment's sound as it passes (the whistle and take-up, the clanks over the tower, the landing), the loops while it runs,
// and the cable's shiver after a clank.

#include "World/OreLift.h"
#include "Audio/LooterSound.h"
#include "Algo/Reverse.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	/** The cable's shiver after a clank: how fast it shivers (Hz) and dies away (per s). */
	constexpr float ShiverHz = 5.5f;
	constexpr float ShiverDecay = 2.5f;
}

FOreLiftRunShape AOreLift::MakeRun(EOreLiftEnd From, bool bRide) const
{
	FOreLiftRunShape Shape;
	Shape.TakeUpSeconds = bRide ? RideTakeUpSeconds : CallTakeUpSeconds;
	Shape.TravelSeconds = FMath::Max(bRide ? RideTravelSeconds : CallTravelSeconds, 0.1f);
	Shape.EaseSeconds = bRide ? RideEaseSeconds : CallEaseSeconds;
	Shape.SettleSeconds = bRide ? RideSettleSeconds : CallSettleSeconds;

	TArray<FVector> Points = GetCablePoints();
	if (From == EOreLiftEnd::Top)
	{
		Algo::Reverse(Points);
	}
	// Each span's way as shares of the run's heading over the ground and its right: what the cage feels at each bend.
	const FVector Heading = FVector(Points.Last().X - Points[0].X, Points.Last().Y - Points[0].Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Heading);
	double Length = 0.0;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		const FVector Span = Points[Index] - Points[Index - 1];
		const FVector Way = Span.GetSafeNormal();
		Shape.Spans.Add(FVector2D(FVector::DotProduct(Way, Heading), FVector::DotProduct(Way, Right)));
		Length += Span.Size();
		if (Index + 1 < Points.Num())
		{
			Shape.Bends.Add(static_cast<float>(Length));
		}
	}
	Shape.Length = static_cast<float>(Length);
	return Shape;
}

void AOreLift::BeginRun(EOreLiftEnd From, bool bRide)
{
	RunFrom = From;
	Run = MakeRun(From, bRide);
	ClankTimes = OreLiftMotion::ClankTimes(Run);
	SwingState = FOreLiftSwingState();
	Sway = FOreLiftSway();
	RunTime = 0.f;
	ClanksHeard = 0;
	bTakeUpHeard = false;
	bLandHeard = false;
	bRiding = bRide;
	bCalled = !bRide;
}

void AOreLift::SetRunTime(float Seconds, bool bQuiet)
{
	if (!IsMoving())
	{
		return;
	}
	const float Time = FMath::Clamp(Seconds, 0.f, Run.GetDuration());
	FOreLiftSwing Swing;
	Swing.PendulumLength = PendulumLength;
	Swing.Damping = SwingDamping;
	Swing.MaxDegrees = MaxSwingDegrees;
	Swing.WindDegrees = WindDegrees;
	Swing.ClankBob = ClankBob;
	Sway = OreLiftMotion::SwayAt(Run, Swing, Time, SwingState);
	FQuat Swung = FQuat::Identity;
	FVector Heading = FVector::ForwardVector;
	Cage->SetRelativeTransform(CageAt(RunFrom, OreLiftMotion::DistanceAt(Run, Time), Sway, Swung, Heading));
	const FQuat ToWorld = GetActorQuat();
	SwayTurn = ToWorld * Swung * ToWorld.Inverse();
	RunHeading = ToWorld.RotateVector(Heading);

	// Each moment's sound as the run passes it, once: the whistle and the take-up, each wheel's clank, the landing.
	if (!bTakeUpHeard && Time > 0.f)
	{
		bTakeUpHeard = true;
		if (!bQuiet)
		{
			LooterSound::PlayAt(this, LooterSoundCue::OreLift::Whistle, Winch->GetComponentLocation());
			LooterSound::PlayAttached(LooterSoundCue::OreLift::TakeUp, Cage);
			if (Time < Run.GetArrival())
			{
				StartLoops();
			}
		}
	}
	while (ClanksHeard < ClankTimes.Num() && Time >= ClankTimes[ClanksHeard])
	{
		if (!bQuiet)
		{
			LooterSound::PlayAt(this, LooterSoundCue::OreLift::SheaveClank, GetActorTransform().TransformPosition(TowerSheave));
		}
		++ClanksHeard;
	}
	if (!bLandHeard && Time >= Run.GetArrival())
	{
		bLandHeard = true;
		StopLoops();
		if (!bQuiet)
		{
			LooterSound::PlayAttached(LooterSoundCue::OreLift::CageLand, Cage);
		}
	}

	// The cable shivers from the last clank, dying away.
	float Shiver = 0.f;
	if (ClanksHeard > 0 && !bLandHeard)
	{
		const float Since = Time - ClankTimes[ClanksHeard - 1];
		Shiver = CableShiver * FMath::Exp(-ShiverDecay * Since) * FMath::Sin(UE_TWO_PI * ShiverHz * Since);
	}
	if (!FMath::IsNearlyZero(Shiver, 0.05f) || !FMath::IsNearlyZero(LastShiver, 0.05f))
	{
		LayCable(Shiver);
		LastShiver = Shiver;
	}
	RunTime = Time;
}

void AOreLift::StartLoops()
{
	// The cable singing over the grip rides with the cage; the winch works in its house.
	if (!CableLoop.IsValid())
	{
		CableLoop = LooterSound::Start(this, LooterSoundCue::OreLift::CableRun, Cage);
	}
	if (!WinchLoop.IsValid())
	{
		WinchLoop = LooterSound::Start(this, LooterSoundCue::OreLift::WinchRun, Winch);
	}
}

void AOreLift::StopLoops()
{
	LooterSound::Stop(CableLoop.Get(), 0.5f);
	LooterSound::Stop(WinchLoop.Get(), 0.8f);
	CableLoop.Reset();
	WinchLoop.Reset();
}
