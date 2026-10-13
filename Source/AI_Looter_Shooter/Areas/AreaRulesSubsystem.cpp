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

ECreatureRank UAreaRulesSubsystem::RollPromotion()
{
	if (!bPromotionsRoll)
	{
		return ECreatureRank::Basic;
	}
	const ECreatureRank Promoted = RollPromotionIn(Area, Rolls);
	++PromotionsAsked;
	PromotedRare += Promoted == ECreatureRank::Rare ? 1 : 0;
	PromotedEpic += Promoted == ECreatureRank::Epic ? 1 : 0;
	return Promoted;
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

ECreatureRank UAreaRulesSubsystem::RollPromotionIn(const UAreaDefinition* InArea, const FRandomStream& Random)
{
	return InArea && InArea->HasPromotions() ? InArea->PickPromotion(Random.FRand()) : ECreatureRank::Basic;
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
		UE_LOG(LogLooter, Log, TEXT("%s: this arrival promoted %d of %d placed Basic creatures: %d %s, %d %s"), *AreaName,
			PromotedRare + PromotedEpic, PromotionsAsked, PromotedRare, *UCreatureRankSettings::Get(ECreatureRank::Rare).Word.ToString(),
			PromotedEpic, *UCreatureRankSettings::Get(ECreatureRank::Epic).Word.ToString());
	}
}
