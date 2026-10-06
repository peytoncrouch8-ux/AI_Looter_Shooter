#include "Areas/AreaRulesSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureRankSettings.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
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

int32 UAreaRulesSubsystem::RollLevel(int32 OwnLevel, ECreatureRank Rank)
{
	return RollLevelIn(Area, GetPlayerLevel(), OwnLevel, Rank, Rolls);
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

int32 UAreaRulesSubsystem::GetPlayerLevel() const
{
	// The progression the session gave the player as the level started (before anything began play).
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return Progression ? Progression->GetLevel() : 1;
}

int32 UAreaRulesSubsystem::RollLevelIn(const UAreaDefinition* InArea, int32 PlayerLevel, int32 OwnLevel, ECreatureRank Rank,
	const FRandomStream& Random)
{
	if (!InArea || !InArea->HasLevelBand())
	{
		return FMath::Max(OwnLevel, 1);
	}
	// A boss is the player's level kept inside the band; anything else a level either side of it, or the same.
	const int32 Spread = Rank == ECreatureRank::Boss ? 0 : Random.RandRange(-1, 1);
	return InArea->LevelFor(PlayerLevel, Spread, OwnLevel);
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
		UE_LOG(LogLooter, Log, TEXT("%s: creatures at levels %d-%d, following the player's %d (their ranks' levels on top)"), *AreaName,
			Area->MinLevel, Area->GetBandTop(), GetPlayerLevel());
	}
	if (bPromotionsRoll)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: this arrival promoted %d of %d placed Basic creatures: %d %s, %d %s"), *AreaName,
			PromotedRare + PromotedEpic, PromotionsAsked, PromotedRare, *UCreatureRankSettings::Get(ECreatureRank::Rare).Word.ToString(),
			PromotedEpic, *UCreatureRankSettings::Get(ECreatureRank::Epic).Word.ToString());
	}
}
