#include "Embers/Powers/EmberFiftyTwoPickup.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberVfx.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"

namespace
{
	const FName CardsName(TEXT("Cards"));
	const FName ConeName(TEXT("ConeDegrees"));
	const FName FanTallName(TEXT("FanTall"));

	/** A card stuck in the world glints this long, then fades over the last of it. */
	constexpr float StuckSeconds = 1.4f;
	constexpr float StuckFade = 0.4f;
	/** How bright a card is in the air, and how big it's drawn there (bigger than a real card, so a fan reads at range). */
	constexpr float CardGlow = 16.f;
	constexpr float FlyingCardSize = 1.35f;
	/** The red of a card's cut: a flick of its face's ink. */
	const FLinearColor CutRed(0.9f, 0.12f, 0.15f);

	struct FCutsOn
	{
		float Damage = 0.f;
		int32 Count = 0;
		FVector At = FVector::ZeroVector;
		FVector Along = FVector::ForwardVector;
	};
}

void UEmberFiftyTwoPickup::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Damage = ScaledDamage(Context, DefaultDamage);
	Range = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	const int32 Count = FMath::Clamp(FMath::RoundToInt(Def ? Def->Extra(CardsName, DefaultCards) : DefaultCards), 1, 128);
	const float Wide = FMath::Clamp(Def ? Def->Extra(ConeName, DefaultConeDegrees) : DefaultConeDegrees, 1.f, 120.f);
	const float Tall = FMath::Clamp(Def ? Def->Extra(FanTallName, DefaultFanTall) : DefaultFanTall, 1.f, 90.f);
	Cuts = 0;
	Random.Initialize(static_cast<int32>(GetTypeHash(Character->GetActorLocation())) ^ 0x52);

	// The deck fanned out evenly over the fan (rows and columns, a little give in each card's place), from the hand.
	FVector Eye;
	FVector Aim;
	GetAim(*Character, Eye, Aim);
	const FRotator AimRotation = Aim.Rotation();
	const FVector Hand = HandPoint(*Character, 12.f, 12.f, 40.f);
	const int32 Rows = FMath::Max(1, FMath::RoundToInt(FMath::Sqrt(Count * Tall / Wide)));
	const int32 Columns = FMath::DivideAndRoundUp(Count, Rows);
	Cards.Reset(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const int32 Row = Index / Columns;
		const int32 Column = Index % Columns;
		const float U = (Column + 0.5f + Random.FRandRange(-0.35f, 0.35f)) / Columns - 0.5f;
		const float V = (Row + 0.5f + Random.FRandRange(-0.35f, 0.35f)) / Rows - 0.5f;
		const FVector Way = (AimRotation + FRotator(V * Tall, U * Wide, 0.f)).Vector();
		FCard& Card = Cards.AddDefaulted_GetRef();
		Card.Location = Hand + Way * Random.FRandRange(0.f, 20.f);
		Card.Velocity = Way * CardSpeed * Random.FRandRange(0.9f, 1.1f);
		Card.Spin = Random.FRandRange(0.f, UE_TWO_PI);
		Card.SpinRate = Random.FRandRange(18.f, 30.f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
	}

	// Who may be cut: the living within its reach.
	Candidates.Reset();
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), Hand) <= Range + 400.f)
		{
			Candidates.Add(*It);
		}
	}

	LooterSound::Play2D(Character, LooterSoundCue::Ember::FiftyTwoPickupThrow);
	FViewKick Fling;
	Fling.Pitch = 0.5f;
	Fling.Yaw = -0.4f;
	Fling.Roll = 0.8f;
	Fling.Frequency = 9.f;
	Fling.Damping = 0.6f;
	Kick(Fling);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const FVector Out = (Aim + Random.GetUnitVector() * 0.6).GetSafeNormal();
		Sparks.Spawn(Hand, Out * Random.FRandRange(200.f, 450.f), Random.FRandRange(0.15f, 0.3f), 1.1f, Color, 16.f, 200.f, 4.f);
	}
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Fifty-Two Pickup: %d cards over %.0f x %.0f degrees, %.1f each, %d creature(s) near"), Count, Wide, Tall, Damage,
		Candidates.Num());
}

void UEmberFiftyTwoPickup::End(const FEmberContext& Context, bool bInterrupted)
{
	// Instant: over the moment it's thrown; the cards fly on their own (StepCards). Cut short, they fall where they are.
	if (bInterrupted)
	{
		for (FCard& Card : Cards)
		{
			Card.bDone = true;
		}
	}
}

int32 UEmberFiftyTwoPickup::NumFlying() const
{
	return Cards.FilterByPredicate([](const FCard& Card) { return !Card.bDone && !Card.bStuck; }).Num();
}

bool UEmberFiftyTwoPickup::Linger(float DeltaSeconds)
{
	StepCards(DeltaSeconds);
	const bool bShowing = Cards.ContainsByPredicate([](const FCard& Card) { return !Card.bDone; });
	return bShowing || !Sparks.IsEmpty();
}

void UEmberFiftyTwoPickup::StepCards(float DeltaSeconds)
{
	UWorld* World = GetBodyWorld();
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	FWeaponFX* FX = Effects();
	if (!World)
	{
		return;
	}
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("FiftyTwoPickup"), Body.Get());
	TMap<TWeakObjectPtr<ACreatureBase>, FCutsOn> CutsOn;
	for (FCard& Card : Cards)
	{
		if (Card.bDone)
		{
			continue;
		}
		if (Card.bStuck)
		{
			// Stuck in the world: a glint that fades.
			Card.StuckAge += Dt;
			if (Card.StuckAge >= StuckSeconds)
			{
				Card.bDone = true;
				continue;
			}
			if (FX)
			{
				const float Fade = 1.f - FMath::Clamp((Card.StuckAge - (StuckSeconds - StuckFade)) / StuckFade, 0.f, 1.f);
				EmberFX::DrawCard(*FX, Card.Location, Card.StuckAlong, Card.Spin, Color, CardGlow * 0.5f * Fade);
			}
			continue;
		}
		const FVector From = Card.Location;
		FVector To = From + Card.Velocity * Dt;
		Card.Spin += Card.SpinRate * Dt;

		// A body in its way first (the nearest along the card's path), then the world.
		ACreatureBase* Cut = nullptr;
		float CutAlong = TNumericLimits<float>::Max();
		FVector CutAt = To;
		for (const TWeakObjectPtr<ACreatureBase>& Each : Candidates)
		{
			ACreatureBase* Creature = Each.Get();
			if (!Creature || Creature->IsDead())
			{
				continue;
			}
			FVector OnLine;
			FVector OnBody;
			if (DistanceToBody(*Creature, From, To, OnLine, OnBody) > CardRadius)
			{
				continue;
			}
			const float Along = static_cast<float>(FVector::Dist(From, OnLine));
			if (Along < CutAlong)
			{
				CutAlong = Along;
				Cut = Creature;
				CutAt = OnLine;
			}
		}
		FHitResult Wall;
		const bool bWall = World->LineTraceSingleByObjectType(Wall, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		if (bWall && (!Cut || static_cast<float>(Wall.Distance) < CutAlong))
		{
			// Into the world: it sticks there, edge first, with a puff of what it struck.
			Card.bStuck = true;
			Card.Location = Wall.ImpactPoint + Wall.ImpactNormal * 1.5;
			Card.StuckAlong = Card.Velocity.GetSafeNormal();
			if (FX && Random.FRand() < 0.35f)
			{
				FX->SpawnGrit(Wall.ImpactPoint, (Wall.ImpactNormal + Random.GetUnitVector() * 0.5).GetSafeNormal() * 200.f, 0.6f, 0.4f);
			}
			continue;
		}
		if (Cut)
		{
			Card.bDone = true;
			++Cuts;
			FCutsOn& On = CutsOn.FindOrAdd(Cut);
			On.Damage += Damage;
			++On.Count;
			On.At = CutAt;
			On.Along = Card.Velocity.GetSafeNormal();
			continue;
		}
		Card.Location = To;
		Card.Travelled += static_cast<float>(FVector::Dist(From, To));
		if (Card.Travelled >= Range)
		{
			// Its reach spent: it flutters away (a few sparks of its colour) and is gone.
			Card.bDone = true;
			Sparks.Spawn(To, Card.Velocity * 0.05f, 0.25f, 1.f, Color, 10.f, 200.f, 3.f);
			continue;
		}
		if (FX)
		{
			EmberFX::DrawCard(*FX, Card.Location, Card.Velocity, Card.Spin, Color, CardGlow, FlyingCardSize);
		}
	}

	// Each body's cuts this frame as one hit: one number, one sound, a spray of torn card and its ink.
	for (const TPair<TWeakObjectPtr<ACreatureBase>, FCutsOn>& Each : CutsOn)
	{
		ACreatureBase* Creature = Each.Key.Get();
		if (!Creature)
		{
			continue;
		}
		const FCutsOn& On = Each.Value;
		Hurt(*Creature, On.Damage * Random.FRandRange(0.9f, 1.1f), On.At, On.Along);
		LooterSound::PlayAt(Creature, LooterSoundCue::Ember::FiftyTwoPickupHit, On.At, FMath::Clamp(0.55f + 0.08f * On.Count, 0.55f, 1.f),
			Random.FRandRange(0.95f, 1.06f));
		for (int32 Index = 0; Index < FMath::Min(3 + On.Count, 12); ++Index)
		{
			const FVector Out = (-On.Along + Random.GetUnitVector()).GetSafeNormal();
			Sparks.Spawn(On.At, Out * Random.FRandRange(150.f, 380.f), Random.FRandRange(0.15f, 0.35f), 1.2f, Index % 3 ? Color : CutRed, 14.f, 500.f, 3.f);
		}
		// (The VFX pass) a hand of cards landing together cuts deep: a strike of its colour, bigger the more cards.
		if (FX && On.Count >= 3)
		{
			EmberVfx::Strike(*FX, On.At, Color, FMath::Min(0.45f + 0.06f * On.Count, 1.f));
		}
	}
	Sparks.Tick(Dt, FX);
}
