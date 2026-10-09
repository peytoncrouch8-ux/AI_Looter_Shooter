#include "World/FaunaFlock.h"
#include "World/FaunaRules.h"
#include "Audio/LooterSound.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** How far (cm) a settled bird looks for another perch to hop to. */
	constexpr float HopReach = 900.f;

	/** A threat this many flee radii (plus the perches' spread) from the flock's middle keeps it from calming. */
	constexpr float CalmRadiusScale = 1.6f;

	/** A perch is safe to land on with no threat within this many flee radii of it. */
	constexpr float LandingSafeScale = 1.3f;

	/** How long a call's moment lasts on the bird that calls (s): the head thrown up, a bob. */
	constexpr float CallMoment = 0.45f;

	/** A startled bird waits up to this long (s) per cm from what startled it, and up to StaggerSeconds more. */
	constexpr float DelayPerCm = 1.f / 8000.f;
	constexpr float MaxDistanceDelay = 0.35f;
	constexpr float StaggerSeconds = 0.25f;

	/** A shrunken piece (not in use for its bird): drawn at a thousandth, so its transform stays invertible. */
	constexpr float HiddenScale = 0.001f;
}

AFaunaFlock::AFaunaFlock()
{
	CullDistance = 9000.f;
}

void AFaunaFlock::BeginPlay()
{
	Super::BeginPlay();
	SetupBirds();
}

void AFaunaFlock::SetupBirds()
{
	if (bBirdsReady)
	{
		return;
	}
	bBirdsReady = true;
	FlockRandom.Initialize(Seed);
	MakeComponents();
	ReadSockets();

	// Its middle and spread: the perches', or the aerial flock's sky.
	if (Mode == EFaunaFlockMode::Perching && Perches.Num() > 0)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FFaunaPerch& Perch : Perches)
		{
			Sum += Perch.Location;
		}
		Center = Sum / Perches.Num();
		Reach = 0.f;
		for (const FFaunaPerch& Perch : Perches)
		{
			Reach = FMath::Max(Reach, static_cast<float>(FVector::Dist(Center, Perch.Location)));
		}
	}
	else
	{
		Center = AerialArea.Center + FVector(0.0, 0.0, 0.5 * (AerialArea.MinHeight + AerialArea.MaxHeight));
		Reach = AerialArea.Radius * 1.4f + 0.5f * FMath::Abs(AerialArea.MaxHeight - AerialArea.MinHeight);
	}

	Birds.Reset();
	PerchHolder.Init(INDEX_NONE, Perches.Num());
	if (Mode == EFaunaFlockMode::Perching)
	{
		// The perches that hold a bird as play begins: a shuffle of all of them by the seed.
		TArray<int32> Order;
		for (int32 Index = 0; Index < Perches.Num(); ++Index)
		{
			Order.Add(Index);
		}
		for (int32 Index = Order.Num() - 1; Index > 0; --Index)
		{
			Order.Swap(Index, FlockRandom.RandRange(0, Index));
		}
		const int32 Count = FMath::Min(BirdCount, Perches.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FFaunaBird& Bird = Birds.AddDefaulted_GetRef();
			Bird.Random.Initialize(Seed * 131 + Index * 7919 + 1);
			Bird.Scale = BirdScale * (1.f + Bird.Random.FRandRange(-ScaleJitter, ScaleJitter));
			Bird.LandYaw = Perches[Order[Index]].Yaw + (Bird.Random.FRand() < 0.5f ? 0.f : 180.f);
			Bird.FlapRate = WingbeatsPerSecond;
			PerchHolder[Order[Index]] = Index;
			Settle(Bird, Order[Index]);
			Bird.HeadTimer = Bird.Random.FRandRange(0.f, HeadSecondsMax);
			Bird.IdleTimer = Bird.Random.FRandRange(0.5f, 5.f);
		}
	}
	else
	{
		for (int32 Index = 0; Index < BirdCount; ++Index)
		{
			FFaunaBird& Bird = Birds.AddDefaulted_GetRef();
			Bird.Random.Initialize(Seed * 131 + Index * 7919 + 1);
			Bird.Scale = BirdScale * (1.f + Bird.Random.FRandRange(-ScaleJitter, ScaleJitter));
			Bird.State = EFaunaBirdState::Soaring;
			Bird.AerialPhase = FVector(Bird.Random.FRandRange(0.f, UE_TWO_PI), Bird.Random.FRandRange(0.f, UE_TWO_PI),
				Bird.Random.FRandRange(0.f, UE_TWO_PI));
			Bird.AerialTurn = Bird.Random.FRandRange(0.f, 360.f);
			// Each its own pace and way round, so two never fly in step.
			Bird.AerialRate = Bird.Random.FRandRange(0.8f, 1.2f) * (Bird.Random.FRand() < 0.5f ? -1.f : 1.f);
			Bird.AerialTime = Bird.Random.FRandRange(0.f, 200.f);
			Bird.FlapPhase = Bird.Random.FRandRange(0.f, UE_TWO_PI);
			Bird.FlapRate = WingbeatsPerSecond;
			Bird.FlapBout = Bird.Random.FRandRange(0.f, 2.f);
			Bird.CircleRadius = AerialArea.Radius * Bird.Random.FRandRange(0.75f, 1.2f);
			FlyAerial(Bird, 0.f);
		}
	}

	// One instance of every piece per bird, each piece not in use shrunk away.
	const FTransform Hidden(FQuat::Identity, Center, FVector(HiddenScale));
	for (UInstancedStaticMeshComponent* Pieces : { PerchedInstances.Get(), HeadInstances.Get(), FlyingInstances.Get(),
		WingInstancesL.Get(), WingInstancesR.Get(), OuterInstancesL.Get(), OuterInstancesR.Get() })
	{
		if (Pieces)
		{
			TArray<FTransform> Start;
			Start.Init(Hidden, Birds.Num());
			Pieces->AddInstances(Start, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/true, /*bUpdateNavigation=*/false);
		}
	}
	HopTimer = FlockRandom.FRandRange(HopSecondsMin, FMath::Max(HopSecondsMin, HopSecondsMax));
	CallTimer = FlockRandom.FRandRange(0.f, FMath::Max(CallSecondsMin, CallSecondsMax));
	PushTransforms();
}

void AFaunaFlock::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bBirdsReady)
	{
		SetupBirds();
	}
	const float DeltaSeconds = Tick.DeltaSeconds;
	if (Mode == EFaunaFlockMode::Perching)
	{
		CheckAlarm(Tick, Context);
		if (bFlushed)
		{
			UpdateFlushed(DeltaSeconds, Context);
		}
	}
	UpdateSettledLife(DeltaSeconds, Tick, Context);
	for (int32 Index = 0; Index < Birds.Num(); ++Index)
	{
		UpdateBird(Birds[Index], Index, DeltaSeconds);
	}
	if (bFlushed && Mode == EFaunaFlockMode::Perching)
	{
		bool bAllDown = true;
		for (const FFaunaBird& Bird : Birds)
		{
			bAllDown &= Bird.State == EFaunaBirdState::Perched;
		}
		if (bAllDown)
		{
			bFlushed = false;
			bLeaving = false;
			HopTimer = FlockRandom.FRandRange(HopSecondsMin, FMath::Max(HopSecondsMin, HopSecondsMax));
		}
	}
	// Moving the pieces is the cost: only while someone can see them (coming into view updates at once).
	if (Tick.bOnScreen && IsFaunaShown())
	{
		PushTransforms();
	}
}

FVector AFaunaFlock::GetFaunaCenter() const
{
	return Center;
}

float AFaunaFlock::GetFaunaRadius() const
{
	// Up, the birds wheel round the circle over the perches (or are off over the hills, hidden).
	return bFlushed ? Reach + CircleRadius * 1.3f + CircleHeight : Reach + 100.f;
}

bool AFaunaFlock::IsBusy() const
{
	if (Mode == EFaunaFlockMode::Aerial)
	{
		return true;
	}
	for (const FFaunaBird& Bird : Birds)
	{
		if (Bird.State != EFaunaBirdState::Perched && Bird.State != EFaunaBirdState::Away)
		{
			return true;
		}
	}
	return false;
}

int32 AFaunaFlock::CountPerched() const
{
	int32 Count = 0;
	for (const FFaunaBird& Bird : Birds)
	{
		Count += Bird.State == EFaunaBirdState::Perched ? 1 : 0;
	}
	return Count;
}

void AFaunaFlock::CheckAlarm(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	// Any bird down (perched, coming back, hopping) with a threat in its flee radius, or a noise the flock hears.
	for (const FFaunaBird& Bird : Birds)
	{
		const bool bDown = Bird.State == EFaunaBirdState::Perched || Bird.State == EFaunaBirdState::Returning
			|| Bird.State == EFaunaBirdState::Landing || Bird.State == EFaunaBirdState::Hopping;
		if (!bDown)
		{
			continue;
		}
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			if (FaunaRules::IsThreatened(Bird.Position, Threat, FleeRadius))
			{
				Startle(Threat.Location);
				return;
			}
		}
	}
	// Strikes are judged at the perches' spread: a bullet into the fence under one end startles the far end too.
	const int32 Heard = FaunaRules::FirstHeard(Center, Context.Noises, Tick.Since, GunfireRadius, ImpactRadius + Reach);
	if (Heard != INDEX_NONE)
	{
		Startle(Context.Noises[Heard].Location);
	}
}

void AFaunaFlock::Startle(const FVector& Source)
{
	if (Mode != EFaunaFlockMode::Perching || Birds.Num() == 0)
	{
		return;
	}
	CalmTime = 0.f;
	if (!bFlushed)
	{
		bFlushed = true;
		bLeaving = false;
		UpTime = 0.f;
		CalmNeeded = FlockRandom.FRandRange(CalmSecondsMin, FMath::Max(CalmSecondsMin, CalmSecondsMax));
		// The circle hangs over the perches, pushed away from what startled them.
		FVector Away = Center - Source;
		Away.Z = 0.0;
		Away = Away.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		CircleCenter = Center + Away * (CircleRadius * 0.5f) + FVector(0.0, 0.0, CircleHeight);
		CircleDirection = FlockRandom.FRand() < 0.5f ? -1.f : 1.f;
		PlayCue(TakeOffCue, Center);
	}
	for (FFaunaBird& Bird : Birds)
	{
		const bool bPerched = Bird.State == EFaunaBirdState::Perched;
		const bool bLow = Bird.State == EFaunaBirdState::Returning || Bird.State == EFaunaBirdState::Landing
			|| Bird.State == EFaunaBirdState::Hopping;
		if (!bPerched && !bLow)
		{
			continue;
		}
		// The nearest go first, a ripple through the flock; one already in the air turns away at once.
		const float Distance = static_cast<float>(FVector::Dist(Bird.Position, Source));
		Bird.Delay = bLow ? 0.f : FMath::Min(Distance * DelayPerCm, MaxDistanceDelay) + Bird.Random.FRandRange(0.f, StaggerSeconds);
		Bird.State = EFaunaBirdState::Startled;
		Bird.PecksLeft = 0;
		Bird.WalkDuration = 0.f;
	}
}

void AFaunaFlock::UpdateFlushed(float DeltaSeconds, const FFaunaContext& Context)
{
	UpTime += DeltaSeconds;
	const float CalmRadius = FleeRadius * CalmRadiusScale + Reach;
	bool bThreatNear = false;
	for (const FFaunaThreat& Threat : Context.Threats)
	{
		bThreatNear |= FVector::Dist2D(Threat.Location, Center) < CalmRadius * FMath::Max(Threat.Fear, 0.5f);
	}
	CalmTime = bThreatNear ? 0.f : CalmTime + DeltaSeconds;

	// Troubled too long: off over the hills, out of the player's way, until it's calm.
	if (!bLeaving && UpTime > LeaveAfterSeconds && CalmTime < CalmNeeded)
	{
		bLeaving = true;
		for (FFaunaBird& Bird : Birds)
		{
			if (Bird.State == EFaunaBirdState::Circling || Bird.State == EFaunaBirdState::TakingOff)
			{
				StartLeave(Bird);
			}
		}
	}
	if (CalmTime < CalmNeeded)
	{
		return;
	}
	for (int32 Index = 0; Index < Birds.Num(); ++Index)
	{
		FFaunaBird& Bird = Birds[Index];
		if (Bird.State != EFaunaBirdState::Circling && Bird.State != EFaunaBirdState::Away)
		{
			continue;
		}
		const int32 Perch = FindLandingPerch(Index, Context);
		if (Perch != INDEX_NONE)
		{
			PerchHolder[Perch] = Index;
			StartReturn(Bird, Perch);
		}
	}
}

int32 AFaunaFlock::FindLandingPerch(int32 BirdIndex, const FFaunaContext& Context) const
{
	const FFaunaBird& Bird = Birds[BirdIndex];
	auto IsSafe = [&](int32 Perch)
	{
		return PerchHolder[Perch] == INDEX_NONE
			&& !FaunaRules::IsThreatenedByAny(Perches[Perch].Location, Context.Threats, FleeRadius * LandingSafeScale);
	};
	if (Perches.IsValidIndex(Bird.Perch) && IsSafe(Bird.Perch))
	{
		return Bird.Perch;
	}
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 Perch = 0; Perch < Perches.Num(); ++Perch)
	{
		const double Distance = FVector::DistSquared(Bird.Position, Perches[Perch].Location);
		if (Distance < BestDistance && IsSafe(Perch))
		{
			BestDistance = Distance;
			Best = Perch;
		}
	}
	return Best;
}

void AFaunaFlock::UpdateSettledLife(float DeltaSeconds, const FFaunaTick& Tick, const FFaunaContext& Context)
{
	// A call now and then while a player is near enough to hear it: a perched bird (or a soaring one) throws its head up.
	CallTimer -= DeltaSeconds;
	if (CallTimer <= 0.f)
	{
		CallTimer = FlockRandom.FRandRange(CallSecondsMin, FMath::Max(CallSecondsMin, CallSecondsMax));
		bool bListener = false;
		for (const FFaunaThreat& Threat : Context.Threats)
		{
			bListener |= Threat.bPlayer && FVector::Dist(Threat.Location, Center) < CallRange + Reach;
		}
		if (bListener && !CallCue.IsNone() && Birds.Num() > 0)
		{
			const EFaunaBirdState Caller = Mode == EFaunaFlockMode::Aerial ? EFaunaBirdState::Soaring : EFaunaBirdState::Perched;
			const int32 Start = FlockRandom.RandRange(0, Birds.Num() - 1);
			for (int32 Step = 0; Step < Birds.Num(); ++Step)
			{
				FFaunaBird& Bird = Birds[(Start + Step) % Birds.Num()];
				if (Bird.State == Caller)
				{
					Bird.CallTime = CallMoment;
					PlayCue(CallCue, Bird.Position);
					break;
				}
			}
		}
	}

	// A hop to another perch now and then, only while settled and seen (nobody watches it otherwise).
	if (Mode != EFaunaFlockMode::Perching || bFlushed || Birds.Num() == 0)
	{
		return;
	}
	HopTimer -= DeltaSeconds;
	if (HopTimer > 0.f)
	{
		return;
	}
	HopTimer = FlockRandom.FRandRange(HopSecondsMin, FMath::Max(HopSecondsMin, HopSecondsMax));
	if (!Tick.bOnScreen)
	{
		return;
	}
	const int32 Index = FlockRandom.RandRange(0, Birds.Num() - 1);
	FFaunaBird& Bird = Birds[Index];
	if (Bird.State != EFaunaBirdState::Perched || Bird.PecksLeft > 0 || Bird.WalkDuration > 0.f)
	{
		return;
	}
	TArray<int32> Near;
	for (int32 Perch = 0; Perch < Perches.Num(); ++Perch)
	{
		if (Perch != Bird.Perch && PerchHolder[Perch] == INDEX_NONE
			&& FVector::DistSquared(Perches[Perch].Location, Bird.Position) < FMath::Square(HopReach)
			&& !FaunaRules::IsThreatenedByAny(Perches[Perch].Location, Context.Threats, FleeRadius * LandingSafeScale))
		{
			Near.Add(Perch);
		}
	}
	if (Near.Num() > 0)
	{
		const int32 Target = Near[FlockRandom.RandRange(0, Near.Num() - 1)];
		if (PerchHolder.IsValidIndex(Bird.Perch) && PerchHolder[Bird.Perch] == Index)
		{
			PerchHolder[Bird.Perch] = INDEX_NONE;
		}
		PerchHolder[Target] = Index;
		StartHop(Bird, Target);
	}
}

void AFaunaFlock::PlayCue(FName Cue, const FVector& Where) const
{
	if (!Cue.IsNone() && IsFaunaShown())
	{
		LooterSound::PlayAt(this, Cue, Where);
	}
}
