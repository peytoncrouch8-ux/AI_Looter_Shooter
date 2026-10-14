// AChickenYard's birds: what each does next (stand and look about, peck, scratch, walk, sit, the rooster's crow), the
// flapping run from a scare, walking and turning inside its pen, and where its head looks.

#include "World/ChickenYard.h"
#include "World/FaunaRules.h"

namespace
{
	/** How far a scratch's step back goes (cm). */
	constexpr float StepBackDistance = 7.f;

	/** How fast a bird turns walking, and running (degrees a second); a walk turns on the spot past this much off its way. */
	constexpr float WalkTurnRate = 270.f;
	constexpr float FleeTurnRate = 900.f;
	constexpr float TurnFirstDegrees = 35.f;

	/** A walk's length (cm), and how many spots are tried for one. */
	constexpr float WalkMin = 40.f;
	constexpr float WalkMax = 150.f;
	constexpr int32 WalkTries = 12;

	/** A cornered bird flaps on the spot this long (s). */
	constexpr float CorneredSeconds = 0.6f;

	/** The head: how far it looks about (degrees), how quickly it snaps there, how far it turns to watch someone. */
	constexpr float LookTurn = 75.f;
	constexpr float LookUp = 12.f;
	constexpr float LookDown = -15.f;
	constexpr float HeadSnap = 22.f;
	constexpr float WatchTurn = 110.f;
	constexpr float WatchPitch = 30.f;

	/** How quickly a bird stands tall, and settles again (rates for FInterpTo). */
	constexpr float AlertRise = 8.f;
	constexpr float AlertFall = 1.5f;
}

void AChickenYard::UpdateBird(int32 Index, float DeltaSeconds)
{
	FChicken& Bird = Birds[Index];
	Bird.Clock += DeltaSeconds;
	Bird.Moving = 0.f;
	const float Goal = (Bird.bWatching || bScared) && Bird.Doing != EChickenDoing::Flee ? 1.f : 0.f;
	Bird.Alert = FMath::FInterpTo(Bird.Alert, Goal, DeltaSeconds, Goal > Bird.Alert ? AlertRise : AlertFall);

	switch (Bird.Doing)
	{
	case EChickenDoing::Stand:
		if (Bird.Clock >= Bird.Length)
		{
			ChooseNext(Index);
		}
		break;
	case EChickenDoing::Peck:
		if (Bird.Clock >= Bird.Count * ChickenTimes::Peck || bScared)
		{
			StartStand(Bird, 0.3f, 1.2f);
		}
		break;
	case EChickenDoing::Scratch:
	{
		// Two rakes, a step back, then pecks at the spot.
		const float Back = 2.f * ChickenTimes::Rake;
		if (Bird.Clock >= Back && Bird.Clock < Back + ChickenTimes::StepBack)
		{
			StepBack(Bird, StepBackDistance * DeltaSeconds / ChickenTimes::StepBack);
		}
		if (Bird.Clock >= Back + ChickenTimes::StepBack + Bird.Count * ChickenTimes::Peck || bScared)
		{
			StartStand(Bird, 0.3f, 1.2f);
		}
		break;
	}
	case EChickenDoing::Walk:
		if (MoveOn(Bird, DeltaSeconds, WalkTurnRate, true) || bScared)
		{
			StartStand(Bird, 0.2f, 0.9f);
		}
		break;
	case EChickenDoing::Sit:
		// Up again when its time is over, or someone comes near.
		if (Bird.Clock >= Bird.Length || Bird.bWatching || bScared)
		{
			StartStand(Bird, 0.5f, 1.5f);
		}
		break;
	case EChickenDoing::Crow:
		if (Bird.Clock >= ChickenTimes::Crow)
		{
			StartStand(Bird, 0.6f, 1.5f);
		}
		break;
	case EChickenDoing::Flee:
		if (Bird.Delay > 0.f)
		{
			// The moment before it bolts: standing tall, looking at what scared it.
			Bird.Delay -= DeltaSeconds;
			Bird.Clock = 0.f;
			break;
		}
		if (MoveOn(Bird, DeltaSeconds, FleeTurnRate, false) && Bird.Clock >= CorneredSeconds)
		{
			// There: standing tall, ready to go again if it's still too near (CheckAlarm).
			StartStand(Bird, 0.4f, 1.f);
			Bird.Alert = 1.f;
		}
		break;
	}
	const float Ground = GroundAt(Bird.Position);
	Bird.Height = DeltaSeconds > 0.f ? FMath::FInterpTo(Bird.Height, Ground, DeltaSeconds, 15.f) : Ground;
	UpdateHead(Bird, DeltaSeconds);
}

void AChickenYard::ChooseNext(int32 Index)
{
	FChicken& Bird = Birds[Index];
	if (bScared)
	{
		StartStand(Bird, 0.3f, 0.8f);
		return;
	}
	// Hens potter (peck, scratch, walk) and sometimes sit; the rooster struts about and keeps watch.
	const float Roll = Bird.Random.FRand();
	const float PeckShare = Bird.bRooster ? 0.15f : 0.34f;
	const float ScratchShare = Bird.bRooster ? 0.05f : 0.16f;
	const float WalkShare = Bird.bRooster ? 0.45f : 0.28f;
	const float SitShare = Bird.bRooster || Bird.bWatching ? 0.f : 0.06f;
	if (Roll < PeckShare)
	{
		Bird.Doing = EChickenDoing::Peck;
		Bird.Clock = 0.f;
		Bird.Count = Bird.Random.RandRange(2, 4);
		return;
	}
	if (Roll < PeckShare + ScratchShare)
	{
		Bird.Doing = EChickenDoing::Scratch;
		Bird.Clock = 0.f;
		Bird.Count = Bird.Random.RandRange(2, 3);
		Bird.bMirror = Bird.Random.FRand() < 0.5f;
		return;
	}
	if (Roll < PeckShare + ScratchShare + WalkShare && StartWalk(Index))
	{
		return;
	}
	if (Roll > 1.f - SitShare)
	{
		Bird.Doing = EChickenDoing::Sit;
		Bird.Clock = 0.f;
		Bird.Length = Bird.Random.FRandRange(SitSecondsMin, FMath::Max(SitSecondsMin, SitSecondsMax));
		return;
	}
	StartStand(Bird, 0.8f, 2.5f);
}

void AChickenYard::StartStand(FChicken& Bird, float MinSeconds, float MaxSeconds)
{
	Bird.Doing = EChickenDoing::Stand;
	Bird.Clock = 0.f;
	Bird.Length = Bird.Random.FRandRange(MinSeconds, MaxSeconds);
	Bird.Speed = 0.f;
	Bird.bVia = false;
}

bool AChickenYard::StartWalk(int32 Index)
{
	FChicken& Bird = Birds[Index];
	const TArray<FVector2D>& Spots = PenSpots[Bird.Pen];
	const FChickenPen& Pen = Pens[Bird.Pen];
	for (int32 Try = 0; Try < WalkTries && Spots.Num() > 0; ++Try)
	{
		const FVector2D Spot = Spots[Bird.Random.RandRange(0, Spots.Num() - 1)];
		const double Distance = FVector2D::Distance(Spot, Bird.Position);
		if (Distance < WalkMin || Distance > WalkMax || !ChickenPens::IsWayClear(Pen, Bird.Position, Spot, BodyRoom))
		{
			continue;
		}
		// Not onto another bird.
		bool bTaken = false;
		for (int32 Other = 0; Other < Birds.Num(); ++Other)
		{
			bTaken |= Other != Index && Birds[Other].Pen == Bird.Pen && FVector2D::Distance(Spot, Birds[Other].Position) < 30.0;
		}
		if (bTaken)
		{
			continue;
		}
		Bird.Doing = EChickenDoing::Walk;
		Bird.Clock = 0.f;
		Bird.Target = Spot;
		Bird.bVia = false;
		Bird.Speed = Bird.bRooster ? StrutSpeed : WalkSpeed;
		return true;
	}
	return false;
}

void AChickenYard::StartFlee(int32 Index, const FVector2D& Source)
{
	FChicken& Bird = Birds[Index];
	TArray<FVector2D, TInlineAllocator<8>> Others;
	for (int32 Other = 0; Other < Birds.Num(); ++Other)
	{
		if (Other != Index && Birds[Other].Pen == Bird.Pen)
		{
			Others.Add(Birds[Other].Doing == EChickenDoing::Flee ? Birds[Other].Target : Birds[Other].Position);
		}
	}
	const ChickenPens::FFleeRun Run = ChickenPens::FindFleeRun(Pens[Bird.Pen], PenSpots[Bird.Pen], PenCorners[Bird.Pen],
		Bird.Position, Source, Others, BodyRoom, Bird.Random);
	Bird.Doing = EChickenDoing::Flee;
	Bird.Clock = 0.f;
	Bird.Target = Run.bFound ? Run.Target : Bird.Position;
	Bird.Via = Run.Via;
	Bird.bVia = Run.bFound && Run.bVia;
	Bird.Speed = Bird.Random.FRandRange(FleeSpeedMin, FMath::Max(FleeSpeedMin, FleeSpeedMax));
	// The nearest go first: a ripple through the yard.
	const double Distance = FVector2D::Distance(Bird.Position, Source);
	Bird.Delay = FMath::Min(static_cast<float>(Distance) / 4000.f, 0.25f) + Bird.Random.FRandRange(0.f, 0.12f);
	Bird.WatchAt = FVector(Source, Bird.Height + 100.f);
}

void AChickenYard::StartCrow(int32 Index)
{
	FChicken& Bird = Birds[Index];
	Bird.Doing = EChickenDoing::Crow;
	Bird.Clock = 0.f;
	Bird.Speed = 0.f;
}

bool AChickenYard::MoveOn(FChicken& Bird, float DeltaSeconds, float TurnRate, bool bTurnFirst)
{
	const float StrideSeconds = Bird.Doing == EChickenDoing::Flee ? ChickenTimes::RunStride
		: (Bird.bRooster ? ChickenTimes::RoosterStride : ChickenTimes::HenStride);
	Bird.Stride = FMath::Fmod(Bird.Stride + DeltaSeconds / StrideSeconds, 1.f);
	const FVector2D Heading = Bird.bVia ? Bird.Via : Bird.Target;
	const FVector2D Way = Heading - Bird.Position;
	const double Left = Way.Size();
	if (Left < 1.0)
	{
		if (Bird.bVia)
		{
			Bird.bVia = false;
			return false;
		}
		return true;
	}
	const float WayYaw = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Way.Y, Way.X)));
	Bird.Yaw = FaunaRules::StepAngle(Bird.Yaw, WayYaw, TurnRate * DeltaSeconds);
	if (bTurnFirst && FMath::Abs(FMath::FindDeltaAngleDegrees(Bird.Yaw, WayYaw)) > TurnFirstDegrees)
	{
		// Little steps turning on the spot before it sets off.
		return false;
	}
	const double Step = FMath::Min(static_cast<double>(Bird.Speed * DeltaSeconds), Left);
	const FVector2D Next = Bird.Position + Way / Left * Step;
	if (!ChickenPens::IsInside(Pens[Bird.Pen], Next, BodyRoom * 0.9f))
	{
		// Never out of the pen: it stops where it is.
		Bird.bVia = false;
		Bird.Target = Bird.Position;
		return true;
	}
	Bird.Position = Next;
	Bird.Moving = 1.f;
	return Step >= Left && !Bird.bVia;
}

void AChickenYard::StepBack(FChicken& Bird, float Distance)
{
	const FVector2D Back = -FVector2D(FRotator(0.f, Bird.Yaw, 0.f).Vector()) * Distance;
	if (ChickenPens::IsInside(Pens[Bird.Pen], Bird.Position + Back, BodyRoom))
	{
		Bird.Position += Back;
		Bird.Moving = 1.f;
	}
}

void AChickenYard::UpdateHead(FChicken& Bird, float DeltaSeconds)
{
	float Rate = HeadSnap;
	if (Bird.bWatching || (Bird.Doing == EChickenDoing::Flee && Bird.Delay > 0.f))
	{
		// Watching someone: the head turned to them as far as it goes, the body round if they're behind.
		const FVector Head = HeadOf(Bird);
		const FVector To = Bird.WatchAt - Head;
		const float Yaw = FMath::FindDeltaAngleDegrees(Bird.Yaw, FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(To.Y, To.X))));
		Bird.LookYaw = FMath::Clamp(Yaw, -WatchTurn, WatchTurn);
		Bird.LookPitch = FMath::Clamp(FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(To.Z, To.Size2D()))), -WatchPitch, WatchPitch);
		if (Bird.Doing == EChickenDoing::Stand && FMath::Abs(Yaw) > 80.f)
		{
			Bird.Yaw = FaunaRules::StepAngle(Bird.Yaw, Bird.Yaw + Yaw, 120.f * DeltaSeconds);
		}
		Rate = 14.f;
	}
	else if (Bird.Doing == EChickenDoing::Stand || Bird.Doing == EChickenDoing::Walk || Bird.Doing == EChickenDoing::Sit)
	{
		// Looking about in quick turns: still a while, then a snap somewhere else.
		Bird.LookClock -= DeltaSeconds;
		if (Bird.LookClock <= 0.f)
		{
			Bird.LookClock = Bird.Random.FRandRange(0.3f, Bird.Doing == EChickenDoing::Sit ? 2.5f : 1.3f);
			Bird.LookYaw = Bird.Random.FRandRange(-LookTurn, LookTurn) * (Bird.Doing == EChickenDoing::Walk ? 0.4f : 1.f);
			Bird.LookPitch = Bird.Random.FRandRange(LookDown, LookUp);
		}
	}
	else
	{
		// Pecking, scratching, crowing, running: the head goes with the body.
		Bird.LookYaw = 0.f;
		Bird.LookPitch = 0.f;
	}
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	Bird.HeadYaw = FMath::FInterpTo(Bird.HeadYaw, Bird.LookYaw, DeltaSeconds, Rate);
	Bird.HeadPitch = FMath::FInterpTo(Bird.HeadPitch, Bird.LookPitch, DeltaSeconds, Rate);
}
