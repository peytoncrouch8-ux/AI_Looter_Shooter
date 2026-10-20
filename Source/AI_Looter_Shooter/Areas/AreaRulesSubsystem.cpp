#include "Areas/AreaRulesSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	const UAreaRulesSubsystem* FindRules(const UObject* WorldContextObject)
	{
		// A class default (the tests ask about a spider's) has no level: no rules.
		const UWorld* World = WorldContextObject && GEngine
			? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
		return World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	}
}

bool UAreaRulesSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAreaRulesSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Before any actor begins play: the creatures ask for their levels and ranks as they do.
	Rolls.GenerateNewSeed();
	Area = UAreaDefinition::FindByMap(USessionSubsystem::MapOf(&InWorld));
	bPromotionsRoll = false;
	if (Area && Area->HasPromotions())
	{
		// The session keeps the cooldown; a game without the session subsystem (none should be) rolls every start.
		USessionSubsystem* Sessions = USessionSubsystem::Get(&InWorld);
		bPromotionsRoll = !Sessions || Sessions->ClaimPromotionRoll(&InWorld);
	}
	InWorld.OnWorldBeginPlay.AddUObject(this, &UAreaRulesSubsystem::HandleLevelBegun);
}

const UAreaDefinition* UAreaRulesSubsystem::GetArea() const
{
	return Area;
}

int32 UAreaRulesSubsystem::RollLevelFor(const ACreatureBase& Creature, int32 OwnLevel)
{
	// A level other than its class's was chosen for it where it was placed (a spawned one asked for a level keeps it without
	// asking). Where it stands: as it begins play its spot, as it comes back its home (it may have died far from it).
	const ACreatureBase* Defaults = Creature.GetClass()->GetDefaultObject<ACreatureBase>();
	const bool bOwnLevelChosen = Defaults && FMath::Max(OwnLevel, 1) != FMath::Max(Defaults->Level, 1);
	const FVector Spot = Creature.HasActorBegunPlay() ? Creature.GetHome().GetLocation() : Creature.GetActorLocation();
	LevelsGiven += Area && Area->HasLevelBand() ? 1 : 0;
	return RollLevelIn(Area, Spot, Creature.GetClass(), OwnLevel, bOwnLevelChosen, Rolls);
}

ECreatureRank UAreaRulesSubsystem::RollPromotion(const AActor* Asking, int32* OutRoll)
{
	if (!bPromotionsRoll)
	{
		return ECreatureRank::Basic;
	}
	float Roll = 0.f;
	const ECreatureRank Rolled = RollPromotionIn(Area, Rolls, &Roll);
	const ECreatureRank Kept = KeepUnderCaps(Rolled, Asking);
	++PromotionsAsked;
	PromotionsCapped += Kept != Rolled ? 1 : 0;
	if (const int32 Tier = LooterRanks::TierIndex(Kept); Tier != INDEX_NONE)
	{
		++Promoted[Tier];
	}
	if (OutRoll)
	{
		// The roll's own bits: two creatures promoted alike still get traits of their own.
		*OutRoll = static_cast<int32>(GetTypeHash(Roll));
	}
	return Kept;
}

ECreatureRank UAreaRulesSubsystem::KeepUnderCaps(ECreatureRank Rolled, const AActor* Except, TConstArrayView<ECreatureRank> Pending) const
{
	// Counted only when a capped tier comes up (a few in a hundred rolls), so an arrival's hundreds of creatures stay cheap.
	return UCreatureRankSettings::KeepUnderCaps(Rolled, [this, Except, Pending](ECreatureRank Tier)
	{
		int32 Coming = 0;
		for (const ECreatureRank Each : Pending)
		{
			Coming += Each == Tier ? 1 : 0;
		}
		return CountAlive(Tier, Except) + Coming;
	});
}

int32 UAreaRulesSubsystem::CountAlive(ECreatureRank Rank, const AActor* Except) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}
	int32 Alive = 0;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		const ACreatureBase* Creature = *It;
		if (Creature == Except || Creature->IsDead() || Creature->IsActorBeingDestroyed())
		{
			continue;
		}
		// One still to begin play hasn't taken its rank yet: the one it will start with.
		const ECreatureRank Its = Creature->HasActorBegunPlay() ? Creature->GetRank() : Creature->StartingRank;
		Alive += Its == Rank ? 1 : 0;
	}
	return Alive;
}

int32 UAreaRulesSubsystem::RollLevelIn(const UAreaDefinition* InArea, const FVector& Spot, const UClass* Kind, int32 OwnLevel,
	bool bOwnLevelChosen, const FRandomStream& Random)
{
	if (!InArea || !InArea->HasLevelBand())
	{
		return FMath::Max(OwnLevel, 1);
	}
	if (bOwnLevelChosen)
	{
		return InArea->KeepInBand(OwnLevel);
	}
	// Drawn whatever it is, so a boss among them doesn't shift the rolls of the creatures after it.
	const float Roll = Random.FRand();
	return InArea->PickLevel(Spot, Kind, Roll);
}

ECreatureRank UAreaRulesSubsystem::RollPromotionIn(const UAreaDefinition* InArea, const FRandomStream& Random, float* OutRoll)
{
	if (!InArea || !InArea->HasPromotions())
	{
		return ECreatureRank::Basic;
	}
	const float Roll = Random.FRand();
	if (OutRoll)
	{
		*OutRoll = Roll;
	}
	return InArea->PickPromotion(Roll);
}

// ---------------------------------------------------------------------------
// Practice
// ---------------------------------------------------------------------------

bool UAreaRulesSubsystem::GivesKillExperience() const
{
	return GivesKillExperienceIn(Area);
}

bool UAreaRulesSubsystem::DropsGuns() const
{
	// Asked at each death, so the first cast-off counts from the moment it's recorded.
	const UWorld* World = GetWorld();
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	return DropsGunsIn(Area, Runner ? &Runner->GetCampaign() : nullptr);
}

bool UAreaRulesSubsystem::GivesKillExperienceIn(const UAreaDefinition* InArea)
{
	return !InArea || InArea->GivesKillExperience();
}

bool UAreaRulesSubsystem::DropsGunsIn(const UAreaDefinition* InArea, const FCampaignRecord* Campaign)
{
	return !InArea || InArea->DropsGuns(Campaign && Campaign->bFirstCastOff);
}

bool UAreaRulesSubsystem::GivesKillExperienceAt(const UObject* WorldContextObject)
{
	const UAreaRulesSubsystem* Rules = FindRules(WorldContextObject);
	return !Rules || Rules->GivesKillExperience();
}

bool UAreaRulesSubsystem::DropsGunsAt(const UObject* WorldContextObject)
{
	const UAreaRulesSubsystem* Rules = FindRules(WorldContextObject);
	return !Rules || Rules->DropsGuns();
}

void UAreaRulesSubsystem::HandleLevelBegun()
{
	if (!Area)
	{
		return;
	}
	const FString AreaName = Area->DisplayName.ToString();
	if (Area->HasLevelBand())
	{
		// The count shows the creatures ask where they stand (none would mean they keep their classes' levels).
		UE_LOG(LogLooter, Log, TEXT("%s: creatures at levels %d-%d by where they stand (%d zones, %d kinds at set levels; their ranks' ")
			TEXT("levels on top), never the player's: %d placed ones given theirs."), *AreaName, Area->MinLevel, Area->GetBandTop(),
			Area->LevelZones.Num(), Area->KindLevels.Num(), LevelsGiven);
	}
	if (bPromotionsRoll)
	{
		FString Tiers;
		int32 Total = 0;
		for (int32 Tier = 1; Tier < LooterRanks::NumTiers; ++Tier)
		{
			const ECreatureRank Rank = LooterRanks::Tiers[Tier];
			Tiers += FString::Printf(TEXT("%s%d %s (%.2f%%)"), Tiers.IsEmpty() ? TEXT("") : TEXT(", "), Promoted[Tier],
				*UCreatureRankSettings::GetTierName(Rank), UCreatureRankSettings::SpawnChance(Rank, Area->GetCampaignRise()) * 100.f);
			Total += Promoted[Tier];
		}
		UE_LOG(LogLooter, Log, TEXT("%s: this arrival promoted %d of %d placed Basic creatures (its place in the campaign %.2f): %s; %d stepped down by the caps."),
			*AreaName, Total, PromotionsAsked, Area->GetCampaignRise(), *Tiers, PromotionsCapped);
	}
}
