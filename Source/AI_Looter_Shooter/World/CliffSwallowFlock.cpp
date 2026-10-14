// ACliffSwallowFlock: the colony at its nests: the mouths read off the nests' model, the birds given a mouth each, the
// alarm (a gunshot, a strike, a player under the nests) and the calm after it, and their calls.

#include "World/CliffSwallowFlock.h"
#include "World/FaunaCues.h"
#include "World/FaunaRules.h"
#include "Audio/LooterSound.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

namespace
{
	/**
	 * The mouths of a cluster without the nests' sockets: about where SM_SwallowNests has them (Swallow.py's NESTS: in front
	 * of its pivot and under it, along the wall), facing out and down (cm, degrees).
	 */
	const FVector FallbackMouths[] = { FVector(14.0, 19.5, -12.6), FVector(16.3, 4.8, -13.3), FVector(14.1, -8.8, -11.7),
		FVector(12.4, -18.8, -11.5) };
	constexpr float FallbackMouthPitch = -30.f;

	/** A threat this many flee radii from a nest keeps them from calming. */
	constexpr float SwallowCalmScale = 1.5f;

	/** Startled at the nest, a bird bursts out within this (s). */
	constexpr float BurstSeconds = 0.5f;
}

ACliffSwallowFlock::ACliffSwallowFlock()
{
	CullDistance = 6000.f;
	CallCue = FName(FaunaCue::SwallowTwitter);
}

void ACliffSwallowFlock::BeginPlay()
{
	Super::BeginPlay();
	SetupBirds();
}

void ACliffSwallowFlock::SetupBirds()
{
	if (bBirdsReady)
	{
		return;
	}
	bBirdsReady = true;
	FlockRandom.Initialize(Seed);
	MakeComponents();
	ReadSockets();
	ReadMouths();

	// Its middle and reach: the mouths', and the loops out from them.
	FVector Sum = FVector::ZeroVector;
	for (const FTransform& Mouth : Mouths)
	{
		Sum += Mouth.GetLocation();
	}
	Center = Mouths.Num() > 0 ? Sum / Mouths.Num() : GetActorLocation();
	float Spread = 0.f;
	FVector Out = FVector::ZeroVector;
	for (int32 Index = 0; Index < Mouths.Num(); ++Index)
	{
		Spread = FMath::Max(Spread, static_cast<float>(FVector::Dist(Center, Mouths[Index].GetLocation())));
		Out += OutFrom(Index);
	}
	Reach = Spread + LoopOutMax * 0.5f;
	Out = Out.GetSafeNormal2D(UE_SMALL_NUMBER, FVector::ForwardVector);
	CircleCenter = Center + Out * CircleOut + FVector(0.0, 0.0, CircleHeight);

	// Each cluster's birds, each at a mouth of its own (the cluster's first mouths in a shuffled order).
	Birds.Reset();
	const int32 PerNest = Nests.Num() > 0 ? Mouths.Num() / Nests.Num() : 0;
	for (int32 Nest = 0; Nest < Nests.Num(); ++Nest)
	{
		TArray<int32> Order;
		for (int32 Index = 0; Index < PerNest; ++Index)
		{
			Order.Add(Nest * PerNest + Index);
		}
		for (int32 Index = Order.Num() - 1; Index > 0; --Index)
		{
			Order.Swap(Index, FlockRandom.RandRange(0, Index));
		}
		for (int32 Index = 0; Index < FMath::Min(BirdsPerNest, Order.Num()); ++Index)
		{
			FCliffSwallow& Bird = Birds.AddDefaulted_GetRef();
			Bird.Random.Initialize(Seed * 131 + Birds.Num() * 7919 + 7);
			Bird.Mouth = Order[Index];
			Bird.Scale = BirdScale * Bird.Random.FRandRange(0.94f, 1.06f);
			Bird.Speed = Bird.Random.FRandRange(SpeedMin, FMath::Max(SpeedMin, SpeedMax));
			Bird.FlapRate = Bird.Random.FRandRange(WingbeatsMin, FMath::Max(WingbeatsMin, WingbeatsMax));
			Bird.FlapPhase = Bird.Random.FRandRange(0.f, UE_TWO_PI);
			Settle(Bird);
			// Staggered from the start: some about to leave, some clinging, some long settled.
			Bird.Timer = Bird.Random.FRandRange(0.f, StaySecondsMax);
		}
	}

	// One instance of every piece per bird, each piece not in use shrunk away.
	const FTransform Hidden(FQuat::Identity, Center, FVector(0.001));
	for (UInstancedStaticMeshComponent* Pieces : { PerchedInstances.Get(), HeadInstances.Get(), FlyingInstances.Get(),
		InnerInstancesL.Get(), InnerInstancesR.Get(), OuterInstancesL.Get(), OuterInstancesR.Get() })
	{
		if (Pieces)
		{
			TArray<FTransform> Start;
			Start.Init(Hidden, Birds.Num());
			Pieces->AddInstances(Start, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/true, /*bUpdateNavigation=*/false);
		}
	}
	CallTimer = FlockRandom.FRandRange(0.f, FMath::Max(CallSecondsMin, CallSecondsMax));
	PushTransforms();
}

void ACliffSwallowFlock::ReadMouths()
{
	// The nests' mouths in their model (SOCKET_Nest1..4, in name order so every run is the same), else one per cluster.
	TArray<TPair<FName, FTransform>> Local;
	if (NestMesh)
	{
		for (const UStaticMeshSocket* Socket : NestMesh->Sockets)
		{
			if (Socket && Socket->SocketName.ToString().StartsWith(TEXT("Nest")))
			{
				Local.Add({ Socket->SocketName, FTransform(Socket->RelativeRotation, Socket->RelativeLocation) });
			}
		}
		Local.Sort([](const TPair<FName, FTransform>& A, const TPair<FName, FTransform>& B) { return A.Key.LexicalLess(B.Key); });
	}
	if (Local.IsEmpty())
	{
		for (const FVector& Mouth : FallbackMouths)
		{
			Local.Add({ NAME_None, FTransform(FRotator(FallbackMouthPitch, 0.f, 0.f), Mouth) });
		}
	}
	Mouths.Reset();
	for (const FTransform& Nest : Nests)
	{
		for (const TPair<FName, FTransform>& Each : Local)
		{
			// Where the nest's scale puts the mouth, but a bird of its own size.
			FTransform Mouth = Each.Value * Nest;
			Mouth.SetScale3D(FVector::OneVector);
			Mouths.Add(Mouth);
		}
	}
}

FVector ACliffSwallowFlock::OutFrom(int32 Mouth) const
{
	if (!Mouths.IsValidIndex(Mouth))
	{
		return FVector::ForwardVector;
	}
	return Mouths[Mouth].GetUnitAxis(EAxis::X).GetSafeNormal2D(UE_SMALL_NUMBER, FVector::ForwardVector);
}

FVector ACliffSwallowFlock::GetFaunaCenter() const
{
	return Center;
}

float ACliffSwallowFlock::GetFaunaRadius() const
{
	return Reach;
}

bool ACliffSwallowFlock::IsBusy() const
{
	for (const FCliffSwallow& Bird : Birds)
	{
		if (Bird.State != ESwallowState::Inside && Bird.State != ESwallowState::Clinging)
		{
			return true;
		}
	}
	return false;
}

int32 ACliffSwallowFlock::CountHome() const
{
	int32 Count = 0;
	for (const FCliffSwallow& Bird : Birds)
	{
		Count += Bird.State == ESwallowState::Inside || Bird.State == ESwallowState::Clinging ? 1 : 0;
	}
	return Count;
}

void ACliffSwallowFlock::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bBirdsReady)
	{
		SetupBirds();
	}
	const float DeltaSeconds = Tick.DeltaSeconds;
	CheckAlarm(Tick, Context);
	if (bAlarmed)
	{
		UpdateCalm(DeltaSeconds, Context);
	}
	for (FCliffSwallow& Bird : Birds)
	{
		UpdateBird(Bird, DeltaSeconds);
	}
	UpdateCalls(DeltaSeconds, Context);
	// Moving the pieces is the cost: only while someone can see them (coming into view updates at once).
	if (Tick.bOnScreen && IsFaunaShown())
	{
		PushTransforms();
	}
}

void ACliffSwallowFlock::CheckAlarm(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (Mouths.IsEmpty())
	{
		return;
	}
	const float Spread = Reach - LoopOutMax * 0.5f;
	const int32 Heard = FaunaRules::FirstHeard(Center, Context.Noises, Tick.Since, GunfireRadius + Spread, ImpactRadius + Spread);
	if (Heard != INDEX_NONE)
	{
		Alarm(Context.Noises[Heard].Location);
		return;
	}
	for (const FFaunaThreat& Threat : Context.Threats)
	{
		for (const FTransform& Mouth : Mouths)
		{
			if (FaunaRules::IsThreatened(Mouth.GetLocation(), Threat, FleeRadius))
			{
				Alarm(Threat.Location);
				return;
			}
		}
	}
}

void ACliffSwallowFlock::Alarm(const FVector& Source)
{
	CalmTime = 0.f;
	if (bAlarmed || Birds.IsEmpty())
	{
		return;
	}
	bAlarmed = true;
	CalmNeeded = FlockRandom.FRandRange(CalmSecondsMin, FMath::Max(CalmSecondsMin, CalmSecondsMax));
	CircleDirection = FlockRandom.FRand() < 0.5f ? -1.f : 1.f;
	PlayCue(CallCue, Center);
	for (FCliffSwallow& Bird : Birds)
	{
		if (Bird.State == ESwallowState::Inside || Bird.State == ESwallowState::Clinging)
		{
			// Out of the nests in a burst, not all at once.
			Bird.State = ESwallowState::Startled;
			Bird.Timer = Bird.Random.FRandRange(0.f, BurstSeconds);
		}
		else if (Bird.State != ESwallowState::Startled && Bird.State != ESwallowState::Circling)
		{
			StartCircle(Bird);
		}
	}
}

void ACliffSwallowFlock::UpdateCalm(float DeltaSeconds, const FFaunaContext& Context)
{
	bool bNear = false;
	for (const FFaunaThreat& Threat : Context.Threats)
	{
		for (const FTransform& Mouth : Mouths)
		{
			bNear |= FaunaRules::IsThreatened(Mouth.GetLocation(), Threat, FleeRadius * SwallowCalmScale);
		}
	}
	CalmTime = bNear ? 0.f : CalmTime + DeltaSeconds;
	if (CalmTime < CalmNeeded)
	{
		return;
	}
	bAlarmed = false;
	for (FCliffSwallow& Bird : Birds)
	{
		if (Bird.State == ESwallowState::Circling)
		{
			// Home one by one over a few seconds.
			Bird.Timer = Bird.Random.FRandRange(0.f, 3.f);
		}
	}
}

void ACliffSwallowFlock::UpdateCalls(float DeltaSeconds, const FFaunaContext& Context)
{
	CallTimer -= DeltaSeconds;
	if (CallTimer > 0.f || Birds.IsEmpty())
	{
		return;
	}
	CallTimer = FlockRandom.FRandRange(CallSecondsMin, FMath::Max(CallSecondsMin, CallSecondsMax));
	bool bListener = false;
	for (const FFaunaThreat& Threat : Context.Threats)
	{
		bListener |= Threat.bPlayer && FVector::Dist(Threat.Location, Center) < CallRange + Reach;
	}
	const FCliffSwallow& Caller = Birds[FlockRandom.RandRange(0, Birds.Num() - 1)];
	if (bListener && !(Caller.State == ESwallowState::Inside && !Caller.bPeeking))
	{
		PlayCue(CallCue, Caller.Position);
	}
}

void ACliffSwallowFlock::PlayCue(FName Cue, const FVector& Where) const
{
	if (!Cue.IsNone() && IsFaunaShown())
	{
		LooterSound::PlayAt(this, Cue, Where);
	}
}
