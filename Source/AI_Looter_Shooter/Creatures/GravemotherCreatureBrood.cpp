// AGravemotherCreature's brood (Docs/Areas/RansomsRest.md: "Calls 4 spiderlings at 66% and 33%"; a spiderling is "a
// configuration of the existing spider, not a new class: 0.45x size, 20% health (60 HP at level 1)", on the Basic table,
// sharing the spider's Ledger page). They claw up out of the ground round her, on spots chosen by an encounter's rules
// (EncounterRules and FEncounterGroundProbe: her own level of ground, standable and roomy, out of safe zones and off
// nobody's head; in her den, its floor), kept to the level's cap near the player, and go for her target.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterGroundProbe.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/PlayableArea.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** How hard the ground breaks where one claws up (FWeaponFX::SpawnDirt, 0 to 1). */
	constexpr float ClawOutDirt = 0.7f;
}

// ---------------------------------------------------------------------------
// Spiderlings
// ---------------------------------------------------------------------------

ACreatureBase::FRuntimeSpawn AGravemotherCreature::MakeSpiderlingSpawn(int32 Level)
{
	FRuntimeSpawn Spawn;
	Spawn.Rank = ECreatureRank::Basic;
	Spawn.Level = FMath::Max(Level, 0);
	Spawn.BodyScale = Gravemother::SpiderlingSize;
	Spawn.HealthScale = Gravemother::SpiderlingHealthShare;
	return Spawn;
}

FText AGravemotherCreature::SpiderlingName()
{
	return FText::FromString(TEXT("Spiderling"));
}

ASpiderCreature* AGravemotherCreature::SpawnSpiderling(UWorld* World, const FVector& Feet, float Yaw, int32 Level)
{
	ASpiderCreature* Spiderling = Cast<ASpiderCreature>(
		ACreatureBase::SpawnAtRuntime(World, ASpiderCreature::StaticClass(), Feet, Yaw, MakeSpiderlingSpawn(Level)));
	if (!Spiderling)
	{
		return nullptr;
	}
	// Named for what it is on its tag; it's still a brown spider, so its kills and its page are the spider's.
	Spiderling->DisplayName = SpiderlingName();
	// In a level that isn't playing yet (a test level), it starts as play would start it.
	if (!Spiderling->HasActorBegunPlay())
	{
		Spiderling->DispatchBeginPlay();
	}
	return Spiderling;
}

// ---------------------------------------------------------------------------
// Her calls
// ---------------------------------------------------------------------------

void AGravemotherCreature::PruneBrood()
{
	Brood.RemoveAll([](const TWeakObjectPtr<ASpiderCreature>& Each) { return !Each.IsValid() || Each->IsDead(); });
}

TArray<ASpiderCreature*> AGravemotherCreature::GetBrood() const
{
	TArray<ASpiderCreature*> Alive;
	for (const TWeakObjectPtr<ASpiderCreature>& Each : Brood)
	{
		ASpiderCreature* Spiderling = Each.Get();
		if (Spiderling && !Spiderling->IsDead())
		{
			Alive.Add(Spiderling);
		}
	}
	return Alive;
}

int32 AGravemotherCreature::CallBrood()
{
	UWorld* World = GetWorld();
	if (!World || IsDead())
	{
		return 0;
	}
	PruneBrood();

	// A call into a crowded fight brings fewer: the level's cap near the player holds for her brood too.
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	int32 Wanted = Gravemother::BroodSize;
	if (Encounters && Player)
	{
		const int32 Near = Encounters->CountAliveNear(Player->GetActorLocation(), Settings.NearPlayerRadius);
		Wanted = FMath::Min(Wanted, EncounterRules::Room(Near, Settings.MaxCreaturesNearPlayer));
	}
	if (Wanted <= 0)
	{
		UE_LOG(LogLooter, Log, TEXT("%s calls her brood, but the fight is full (%d creatures near the player): none come."), *GetName(),
			Settings.MaxCreaturesNearPlayer);
		return 0;
	}

	// Spots round her on her own level of ground, as a spawner chooses them: standable, with room for a spiderling, not on
	// top of a rock or a coffin (in her den, the den's floor in Den Rock is ground), out of safe zones, inside the playable
	// area and her hunting ground, clear of the player, and apart from her and each other. With no ground under her at all
	// (a test level), level with her feet.
	const float Scale = GetSizeScale();
	const FVector Middle = GetActorLocation();
	const FVector Feet = GetFeet();
	const FEncounterBody SpiderlingBody = EncounterRules::CreatureBody(ASpiderCreature::StaticClass(), Gravemother::SpiderlingSize,
		UCreatureRankSettings::Get(ECreatureRank::Basic).Size);
	FEncounterGroundProbe Probe(World, this, TEXT("GravemotherBrood"), SpiderlingBody, BroodMaxStep);
	Probe.FindHome(Feet, 50.0 * Scale, 150.0 * Scale);
	const double HomeZ = Probe.GetHomeZ();
	const float Spacing = FMath::Max(BroodSpacing, GetCapsuleComponent()->GetScaledCapsuleRadius() + SpiderlingBody.Radius + 20.f);
	const TArray<FVector> Candidates = EncounterRules::CandidateSpots(FVector(Feet.X, Feet.Y, HomeZ), BroodRadius * Scale,
		TArray<FVector>(), Wanted, Spacing, FMath::FRandRange(0.f, 360.f));
	TArray<FVector> Occupied = { Middle };
	for (const ASpiderCreature* Spiderling : GetBrood())
	{
		Occupied.Add(Spiderling->GetActorLocation());
	}

	auto GroundAt = [&Probe](const FVector& Spot)
	{
		return Probe.Look(Spot);
	};
	const APlayableArea* Playable = APlayableArea::Find(World);
	const APawn* Victim = GetTarget();
	const APawn* Watched = Victim ? Victim : Player;
	const double ClearSquared = FMath::Square(static_cast<double>(BroodClearOfPlayer));
	const FVector HomeSpot = GetHome().GetLocation();
	auto IsBlocked = [this, Encounters, Playable, Watched, ClearSquared, &HomeSpot](const FVector& Spot)
	{
		return (Encounters && Encounters->IsInSafeZone(Spot))
			|| (Playable && !Playable->Contains(Spot))
			|| !HuntingGround.ContainsSpot(Spot, HomeSpot)
			|| (Watched && FVector::DistSquared2D(Spot, Watched->GetActorLocation()) < ClearSquared);
	};
	const TArray<FVector> Spots = EncounterRules::ChooseSpots(Candidates, Wanted, HomeZ, BroodMaxStep, Spacing, Occupied, GroundAt, IsBlocked);

	// Hers before her rank's levels: her brood is of the place, not Soulfed.
	const int32 OwnLevel = FMath::Max(Level - UCreatureRankSettings::Get(GetRank()).LevelOffset, 1);
	int32 Came = 0;
	for (const FVector& Spot : Spots)
	{
		const FVector Facing = Victim ? Victim->GetActorLocation() - Spot : Spot - Middle;
		ASpiderCreature* Spiderling = SpawnSpiderling(World, Spot, static_cast<float>(Facing.Rotation().Yaw), OwnLevel);
		if (!Spiderling)
		{
			continue;
		}
		// They fight on her ground and go for whoever she's after; her boss counts them as its adds (gone with a fight that
		// starts over, dead with her).
		Spiderling->HuntingGround = HuntingGround;
		if (APawn* Hunted = GetTarget())
		{
			Spiderling->AlertTo(Hunted);
		}
		Brood.Add(Spiderling);
		if (Boss)
		{
			Boss->AdoptAdd(Spiderling);
		}
		KickDirt(Spot, ClawOutDirt);
		++Came;
	}
	UE_LOG(LogLooter, Log, TEXT("%s calls her brood (%d of %d calls): %d of %d spiderlings claw up round her."), *GetName(), BroodCallsMade,
		Gravemother::NumBroodCalls, Came, Gravemother::BroodSize);
	return Came;
}
