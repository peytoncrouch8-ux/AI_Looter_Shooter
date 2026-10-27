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
#include "HAL/IConsoleManager.h"
#include "Math/RotationMatrix.h"

namespace
{
	const FName CardsName(TEXT("Cards"));
	const FName ConeName(TEXT("ConeDegrees"));
	const FName FanTallName(TEXT("FanTall"));

	TAutoConsoleVariable<float> CVarWallStick(TEXT("Looter.Embers.FiftyTwo.WallStick"), 1.4f,
		TEXT("Seconds a Fifty-Two Pickup card stays stuck in the world before it's gone (it shrinks away over the last 0.35)."));
	TAutoConsoleVariable<float> CVarFall(TEXT("Looter.Embers.FiftyTwo.Fall"), 2.f,
		TEXT("Seconds a card shaken loose from a body that died falls and lies where it lands before it's gone (it shrinks away "
			 "over the last third)."));
	TAutoConsoleVariable<float> CVarBodyStick(TEXT("Looter.Embers.FiftyTwo.BodyStick"), 1.2f,
		TEXT("Seconds a Fifty-Two Pickup card stays stuck in the creature it cut (riding its body) before it's gone."));
	TAutoConsoleVariable<float> CVarSink(TEXT("Looter.Embers.FiftyTwo.Sink"), 0.35f,
		TEXT("How far a stuck card is sunk in, edge first (a share of its half length)."));
	TAutoConsoleVariable<float> CVarTrail(TEXT("Looter.Embers.FiftyTwo.Trail"), 3.f,
		TEXT("The glow of the faint trail of Odessa's colour behind each flying card (0: none)."));
	TAutoConsoleVariable<float> CVarHandRight(TEXT("Looter.Embers.FiftyTwo.HandRight"), -12.f,
		TEXT("Where the deck leaves the body in first person, cm right of the eye (negative: left, the casting hand's side)."));

	/** A stuck card shrinks away over this last share of its time. */
	constexpr float StuckFade = 0.35f;
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

	// The deck fanned out evenly over the fan (rows and columns, a little give in each card's place), from the casting hand.
	FVector Eye;
	FVector Aim;
	GetAim(*Character, Eye, Aim);
	const FRotator AimRotation = Aim.Rotation();
	const FVector Hand = HandPoint(*Character, CVarHandRight.GetValueOnGameThread(), 12.f, 40.f);
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
		Card.Flutter = Random.FRandRange(0.f, UE_TWO_PI);
		Card.FlutterRate = Random.FRandRange(8.f, 14.f);
		// The fan's cards slant with their place across it, as a fanned hand does.
		Card.Roll = U * 0.9f + Random.FRandRange(-0.2f, 0.2f);
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
		Deck.Begin();
		Deck.Flush(GetBodyWorld(), Color);
	}
}

int32 UEmberFiftyTwoPickup::NumFlying() const
{
	return Cards.FilterByPredicate([](const FCard& Card) { return !Card.bDone && !Card.bStuck && !Card.bFalling; }).Num();
}

bool UEmberFiftyTwoPickup::Linger(float DeltaSeconds)
{
	StepCards(DeltaSeconds);
	const bool bShowing = Cards.ContainsByPredicate([](const FCard& Card) { return !Card.bDone; });
	return bShowing || !Sparks.IsEmpty();
}

void UEmberFiftyTwoPickup::StickCard(FCard& Card, const FVector& At, const FVector& Way, ACreatureBase* Struck) const
{
	// Edge first: its long side along the way it flew, sunk in a share of its half length; it stops spinning there.
	const float Size = EmberFX::CardSize();
	const FQuat Turn = EmberFX::FlyingCardTurn(Way, 0.f, Card.Flutter, Card.Roll);
	const FVector Center = At - Way * (EmberFX::CardLong * Size * 0.5f * (1.f - FMath::Clamp(CVarSink.GetValueOnGameThread(), 0.f, 0.9f)));
	const FTransform World(Turn, Center);
	Card.bStuck = true;
	Card.bOnBody = Struck != nullptr;
	Card.StuckAge = 0.f;
	Card.StuckIn = Struck;
	Card.StuckAt = Struck ? World.GetRelativeTransform(Struck->GetActorTransform()) : World;
	Card.Location = Center;
}

void UEmberFiftyTwoPickup::StepStuck(FCard& Card, float DeltaSeconds)
{
	Card.StuckAge += DeltaSeconds;
	const float Life = FMath::Max(Card.bOnBody ? CVarBodyStick.GetValueOnGameThread() : CVarWallStick.GetValueOnGameThread(), 0.05f);
	ACreatureBase* RiddenBody = Card.StuckIn.Get();
	// The body it rode died or went: it's shaken loose and falls, rather than vanishing.
	if (Card.bOnBody && (!RiddenBody || RiddenBody->IsDead()))
	{
		LooseCard(Card);
		StepFalling(Card, 0.f);
		return;
	}
	// Gone once its time is up.
	if (Card.StuckAge >= Life)
	{
		Card.bDone = true;
		return;
	}
	const FTransform Place = RiddenBody ? Card.StuckAt * RiddenBody->GetActorTransform() : Card.StuckAt;
	Card.Location = Place.GetLocation();
	// Shrinks away over the last of its time (the cards are lit and solid: no fading them out).
	const float Left = FMath::Clamp((Life - Card.StuckAge) / (Life * StuckFade), 0.f, 1.f);
	Deck.Add(Card.Location, Place.GetRotation(), EmberFX::CardSize() * FMath::SmoothStep(0.f, 1.f, Left));
}

void UEmberFiftyTwoPickup::LooseCard(FCard& Card)
{
	// From where it last showed, turned as it was, with a little push off the body and up.
	Card.bStuck = false;
	Card.bOnBody = false;
	Card.bFalling = true;
	Card.bResting = false;
	Card.FallAge = 0.f;
	Card.StuckIn.Reset();
	Card.Velocity = Random.GetUnitVector() * 60.f + FVector(0.0, 0.0, 80.0);
	Card.SpinRate = Random.FRandRange(4.f, 9.f) * (Random.FRand() < 0.5f ? -1.f : 1.f);
}

void UEmberFiftyTwoPickup::StepFalling(FCard& Card, float DeltaSeconds)
{
	const float Life = FMath::Max(CVarFall.GetValueOnGameThread(), 0.1f);
	Card.FallAge += DeltaSeconds;
	if (Card.FallAge >= Life)
	{
		Card.bDone = true;
		return;
	}
	if (!Card.bResting && DeltaSeconds > 0.f)
	{
		// Paper on the air: pulled down, held back hard (it drifts down at about a metre and a half a second), fluttering.
		Card.Velocity.Z -= 980.0 * DeltaSeconds;
		Card.Velocity *= FMath::Max(1.f - 4.f * DeltaSeconds, 0.f);
		const FVector From = Card.Location;
		const FVector To = From + Card.Velocity * DeltaSeconds;
		Card.Spin += Card.SpinRate * DeltaSeconds;
		Card.Flutter += Card.FlutterRate * DeltaSeconds;
		FHitResult Ground;
		UWorld* World = GetBodyWorld();
		const FCollisionQueryParams Params = World ? LooterWorld::StaticGeometryParams(World, TEXT("FiftyTwoFall"), Body.Get())
			: FCollisionQueryParams();
		if (World && World->LineTraceSingleByObjectType(Ground, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			// Landed: it lies there, flat on what it fell on.
			Card.bResting = true;
			Card.Location = Ground.ImpactPoint + Ground.ImpactNormal * 0.3;
			Card.StuckAt = FTransform(FRotationMatrix::MakeFromZX(Ground.ImpactNormal, FQuat(Ground.ImpactNormal, Card.Spin).RotateVector(
				FVector::CrossProduct(Ground.ImpactNormal, FVector::RightVector).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector))).ToQuat(),
				Card.Location);
		}
		else
		{
			Card.Location = To;
		}
	}
	const FQuat Turn = Card.bResting ? Card.StuckAt.GetRotation()
		: EmberFX::FlyingCardTurn(Card.Velocity.IsNearlyZero() ? FVector::DownVector : Card.Velocity, Card.Spin, Card.Flutter, Card.Roll);
	// Shrinks away over the last of its time.
	const float Left = FMath::Clamp((Life - Card.FallAge) / (Life * 0.3f), 0.f, 1.f);
	Deck.Add(Card.Location, Turn, EmberFX::CardSize() * FMath::SmoothStep(0.f, 1.f, Left));
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
	Deck.Begin();
	const float Size = EmberFX::CardSize();
	const float Trail = FMath::Max(CVarTrail.GetValueOnGameThread(), 0.f);
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
			StepStuck(Card, Dt);
			continue;
		}
		if (Card.bFalling)
		{
			StepFalling(Card, Dt);
			continue;
		}
		const FVector From = Card.Location;
		FVector To = From + Card.Velocity * Dt;
		Card.Spin += Card.SpinRate * Dt;
		Card.Flutter += Card.FlutterRate * Dt;
		const FVector Way = Card.Velocity.GetSafeNormal();

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
			StickCard(Card, Wall.ImpactPoint, Way, nullptr);
			if (FX && Random.FRand() < 0.35f)
			{
				FX->SpawnGrit(Wall.ImpactPoint, (Wall.ImpactNormal + Random.GetUnitVector() * 0.5).GetSafeNormal() * 200.f, 0.6f, 0.4f);
			}
			StepStuck(Card, 0.f);
			continue;
		}
		if (Cut)
		{
			// Into the body: it sticks there and rides it.
			++Cuts;
			FCutsOn& On = CutsOn.FindOrAdd(Cut);
			On.Damage += Damage;
			++On.Count;
			On.At = CutAt;
			On.Along = Way;
			StickCard(Card, CutAt, Way, Cut);
			StepStuck(Card, 0.f);
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
		Deck.Add(Card.Location, EmberFX::FlyingCardTurn(Card.Velocity, Card.Spin, Card.Flutter, Card.Roll), Size);
		if (FX && Trail > 0.f)
		{
			// A faint streak of her colour behind it, so the fan's paths read against a bright sky.
			FX->AddStreak(Card.Location - Way * (EmberFX::CardLong * Size * 1.5f), Card.Location, EmberFX::CardShort * Size * 0.25f, Color, Trail);
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
	Deck.Flush(World, Color);
}
