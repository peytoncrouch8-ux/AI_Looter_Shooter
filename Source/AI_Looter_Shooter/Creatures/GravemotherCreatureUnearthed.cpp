// AGravemotherCreature's brood, Unearthed (the user's call, 2026-10-19: "at 66% and 33% she slams the floor and 4 spiderlings
// dig out of mounds that bulge for 1 s first; a mound shot while it bulges kills the spiderling in it"). A spiderling is "a
// configuration of the existing spider, not a new class: 0.45x size, 20% health (60 HP at level 1)", on the Basic table,
// sharing the spider's Ledger page. Her health's calls wait for her roar's slam (GravemotherCreatureMoves.cpp), which raises
// the mounds here, on spots an encounter's rules choose (EncounterRules and FEncounterGroundProbe: her own level of ground,
// standable and roomy, out of safe zones, off nobody's head and off her berms and rubble), kept to the level's cap near the
// player; cracks race out to them. Each mound (AGravemotherMound) bulges, then its spiderling claws out (RaiseFromMound).

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterGroundProbe.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherMound.h"
#include "Creatures/GroundCrack.h"
#include "World/PlayableArea.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The cracks racing out to the mounds: the crack decal this wide (cm), or its stand-in (a dark AGroundCrack). */
	constexpr float RaceCrackWidth = 40.f;
	constexpr float StandInRaceWidth = 16.f;
	const FLinearColor StandInSeam(0.035f, 0.028f, 0.02f);
	/** They lie a moment after the mounds burst, then close (s). */
	constexpr float RaceCrackLingers = 2.f;
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
	Mounds.RemoveAll([](const TWeakObjectPtr<AGravemotherMound>& Each) { return !Each.IsValid(); });
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

TArray<AGravemotherMound*> AGravemotherCreature::GetMounds() const
{
	TArray<AGravemotherMound*> Shown;
	for (const TWeakObjectPtr<AGravemotherMound>& Each : Mounds)
	{
		if (AGravemotherMound* Mound = Each.Get(); Mound && !Mound->IsActorBeingDestroyed())
		{
			Shown.Add(Mound);
		}
	}
	return Shown;
}

int32 AGravemotherCreature::CallBrood()
{
	return RaiseMounds(1);
}

int32 AGravemotherCreature::RaiseMounds(int32 Calls)
{
	UWorld* World = GetWorld();
	if (!World || IsDead() || Calls <= 0)
	{
		return 0;
	}
	PruneBrood();

	// A call into a crowded fight brings fewer: the level's cap near the player holds for her brood too (bulging mounds
	// count as the spiderlings they'll be).
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	int32 Wanted = Gravemother::BroodSize * Calls;
	if (Encounters && Player)
	{
		int32 Near = Encounters->CountAliveNear(Player->GetActorLocation(), Settings.NearPlayerRadius);
		for (const AGravemotherMound* Mound : GetMounds())
		{
			Near += Mound->IsBulging() ? 1 : 0;
		}
		Wanted = FMath::Min(Wanted, EncounterRules::Room(Near, Settings.MaxCreaturesNearPlayer));
	}
	if (Wanted <= 0)
	{
		UE_LOG(LogLooter, Log, TEXT("%s slams the floor, but the fight is full (%d creatures near the player): no mounds."), *GetName(),
			Settings.MaxCreaturesNearPlayer);
		return 0;
	}

	// Spots round her on her own level of ground, as a spawner chooses them: standable, with room for a spiderling, not on
	// a rock, a coffin, a berm or rubble (in her den, the den's floor in Den Rock is ground), out of safe zones, inside the
	// playable area and her hunting ground, clear of the player, and apart from her and each other. With no ground under her
	// at all (a test level), level with her feet.
	const float Scale = GetSizeScale();
	const FVector Middle = GetActorLocation();
	const FVector Feet = GetFeet();
	const FEncounterBody SpiderlingBody = EncounterRules::CreatureBody(ASpiderCreature::StaticClass(), Gravemother::SpiderlingSize,
		UCreatureRankSettings::Get(ECreatureRank::Basic).Size);
	FEncounterGroundProbe Probe(World, this, TEXT("GravemotherMounds"), SpiderlingBody, Unearth.MaxStep);
	Probe.FindHome(Feet, 50.0 * Scale, 150.0 * Scale);
	const double HomeZ = Probe.GetHomeZ();
	const float Spacing = FMath::Max(Unearth.Spacing, GetCapsuleComponent()->GetScaledCapsuleRadius() + SpiderlingBody.Radius + 20.f);
	const TArray<FVector> Candidates = EncounterRules::CandidateSpots(FVector(Feet.X, Feet.Y, HomeZ), Unearth.Radius * Scale,
		TArray<FVector>(), Wanted, Spacing, FMath::FRandRange(0.f, 360.f));
	TArray<FVector> Occupied = { Middle };
	for (const ASpiderCreature* Spiderling : GetBrood())
	{
		Occupied.Add(Spiderling->GetActorLocation());
	}
	for (const AGravemotherMound* Mound : GetMounds())
	{
		Occupied.Add(Mound->GetFeet());
	}
	auto GroundAt = [&Probe](const FVector& Spot)
	{
		return Probe.Look(Spot);
	};
	const APlayableArea* Playable = APlayableArea::Find(World);
	const APawn* Victim = GetTarget();
	const APawn* Watched = Victim ? Victim : Player;
	const double ClearSquared = FMath::Square(static_cast<double>(Unearth.ClearOfPlayer));
	const FVector HomeSpot = GetHome().GetLocation();
	auto IsBlocked = [this, Encounters, Playable, Watched, ClearSquared, &HomeSpot](const FVector& Spot)
	{
		return (Encounters && Encounters->IsInSafeZone(Spot))
			|| (Playable && !Playable->Contains(Spot))
			|| !HuntingGround.ContainsSpot(Spot, HomeSpot)
			|| (Watched && FVector::DistSquared2D(Spot, Watched->GetActorLocation()) < ClearSquared);
	};
	const TArray<FVector> Spots = EncounterRules::ChooseSpots(Candidates, Wanted, HomeZ, Unearth.MaxStep, Spacing, Occupied, GroundAt, IsBlocked);

	// Hers before her rank's levels: her brood is of the place, not Soulfed.
	const int32 OwnLevel = FMath::Max(Level - UCreatureRankSettings::Get(GetRank()).LevelOffset, 1);
	const float Yaw = static_cast<float>(GetActorRotation().Yaw);
	int32 Raised = 0;
	for (const FVector& Spot : Spots)
	{
		const FVector Facing = Victim ? Victim->GetActorLocation() - Spot : Spot - Middle;
		AGravemotherMound* Mound = AGravemotherMound::Spawn(*this, Spot, static_cast<float>(Facing.Rotation().Yaw), Unearth.BulgeSeconds, OwnLevel);
		if (!Mound)
		{
			continue;
		}
		Mounds.Add(Mound);
		++Raised;
		// A crack races out from under her to it: the crack's decal, or a dark crack where it isn't imported.
		const FVector From = Feet + (Spot - Feet).GetSafeNormal2D() * GetCapsuleComponent()->GetScaledCapsuleRadius();
		const FVector Line = Spot - From;
		const float Length = static_cast<float>(Line.Size2D());
		if (Length > 20.f && !GravemotherAssets::IsImported(GravemotherAssets::DecalHeaveCrack))
		{
			if (AGroundCrack* Crack = AGroundCrack::Spawn(World, From, Spot, StandInRaceWidth, StandInSeam, FMath::Rand()))
			{
				Crack->BurstSeconds = Unearth.CrackRaceSeconds;
				Crack->SetOpen(1.f);
				Crack->Close(Unearth.BulgeSeconds + RaceCrackLingers);
				MoundCracks.Add(Crack);
			}
		}
		else if (Length > 20.f)
		{
			// Laid on the mound's own actor, so it goes with it.
			if (UDecalComponent* Decal = GravemotherAssets::AddGroundDecal(*Mound, GravemotherAssets::DecalHeaveCrack, (From + Spot) * 0.5f,
				static_cast<float>(Line.Rotation().Yaw), Length, RaceCrackWidth, 1))
			{
				GravemotherAssets::SetDecalOpacity(Decal, 1.f);
			}
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s slams the floor (%d of %d calls): %d of %d mounds bulge round her (yaw %.0f)."), *GetName(), BroodCallsMade,
		Gravemother::NumBroodCalls, Raised, Gravemother::BroodSize * Calls, Yaw);
	return Raised;
}

ASpiderCreature* AGravemotherCreature::RaiseFromMound(AGravemotherMound& Mound)
{
	UWorld* World = GetWorld();
	if (!World || IsDead())
	{
		return nullptr;
	}
	ASpiderCreature* Spiderling = SpawnSpiderling(World, Mound.GetFeet(), Mound.GetYaw(), Mound.GetSpiderlingLevel());
	if (!Spiderling)
	{
		return nullptr;
	}
	// It fights on her ground and goes for whoever she's after; her boss counts it as her adds (gone with a fight that starts
	// over, dead with her). It climbs out of the burst (the spider's entrance) where a player sees it.
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
	Spiderling->PlayEntranceIfSeen();
	return Spiderling;
}

void AGravemotherCreature::NoteMoundPopped(AGravemotherMound& Mound, AController* By)
{
	UE_LOG(LogLooter, Log, TEXT("%s: a mound shot while it bulged caves in on its spiderling (by %s)."), *GetName(), *GetNameSafe(By));
}

void AGravemotherCreature::TickUnearth(float DeltaSeconds)
{
	if (PendingUnearths <= 0)
	{
		return;
	}
	// A call her roar hasn't answered in time (nobody to roar at, reeling) raises its mounds anyway, with the slam's dust.
	PendingFor += DeltaSeconds;
	if (PendingFor >= Unearth.LateRaiseSeconds && !IsDead() && Move != EGravemotherMove::Roar)
	{
		const int32 Calls = PendingUnearths;
		PendingUnearths = 0;
		bRoarWanted = false;
		KickDirtRing(GetFeet(), 90.f * GetSizeScale(), 5, 0.7f);
		RaiseMounds(Calls);
	}
}

void AGravemotherCreature::CollapseMounds()
{
	for (const TWeakObjectPtr<AGravemotherMound>& Each : Mounds)
	{
		if (AGravemotherMound* Mound = Each.Get())
		{
			Mound->Collapse();
		}
	}
	for (const TWeakObjectPtr<AGroundCrack>& Each : MoundCracks)
	{
		if (AGroundCrack* Crack = Each.Get())
		{
			Crack->Close(0.f);
		}
	}
	MoundCracks.Reset();
}
