#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/WeaponTypes.h"
#include "MissionDefinition.generated.h"

class UMissionObjective;
class UWeaponDefinition;

/** Main or side story, or Skyreach's own missions outside it. */
UENUM(BlueprintType)
enum class EMissionKind : uint8
{
	/** The story: one at a time, kept with its step in the campaign record so a session goes on from it. */
	Main,
	/** Side stories, beside the main one. */
	Side,
	/** Skyreach's own, outside the story (the tutorial, boarding the skiff): their owners keep their steps. */
	Tutorial,
};

/** What starts a mission. */
UENUM(BlueprintType)
enum class EMissionStart : uint8
{
	/** As soon as its prerequisites are finished, in its area's level. */
	Automatic,
	/** When StartEvent happens there (a notice board read, someone spoken to), once its prerequisites are finished. */
	OnEvent,
	/** Only when code or the console starts it (the tutorial director's mission, test missions); listed once started. */
	Manual,
};

/** One step of a mission: objectives that are all done before the next step begins. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionStep
{
	GENERATED_BODY()

	/** Done in any order; the first not done yet is what the tracker shows and where its arrow points. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Mission")
	TArray<TObjectPtr<UMissionObjective>> Objectives;
};

/** What finishing a mission gives, once per session (a mission played again gives nothing more). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionRewards
{
	GENERATED_BODY()

	/**
	 * Experience: this share of what the player's current level takes, so it's worth the same part of a level at any
	 * level. 0.3 for a main mission, 0.2 for a side one (Docs/Areas/RansomsRest.md).
	 */
	UPROPERTY(EditAnywhere, Category = "Rewards", meta = (ClampMin = "0", ClampMax = "5"))
	float ExperienceShare = 0.f;

	/** A gun, dropped at the player's feet. */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	bool bGun = false;

	/** Its kind; none: one picked from the default loot table by its weights. */
	UPROPERTY(EditAnywhere, Category = "Rewards", meta = (EditCondition = "bGun"))
	TObjectPtr<UWeaponDefinition> GunKind;

	/** At least this rare: its rarity is rolled as loot's is, then raised to this when it came out lower. */
	UPROPERTY(EditAnywhere, Category = "Rewards", meta = (EditCondition = "bGun"))
	EWeaponRarity GunRarityFloor = EWeaponRarity::Common;

	/** A named gun (Heirloom), by its name. Named guns come in step 23: until then it's only written down here. */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	FName NamedGun;

	/** Areas opened to travel (the station boards list them), by area id (UAreaDefinition::GetAreaId). */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	TArray<FName> UnlockAreas;

	bool IsEmpty() const { return ExperienceShare <= 0.f && !bGun && NamedGun.IsNone() && UnlockAreas.IsEmpty(); }
};

/**
 * One mission as a data asset in /Game/Data/Missions (DA_Mission_<Id>; Tools/Unreal/create_mission_assets.py makes the
 * first): its words, whether it's main or side, what must be finished first, the area it's played in, what starts it,
 * its steps of objectives and its rewards. The objectives are instanced objects, one C++ class per kind
 * (UMissionObjective). The mission runner of each level (UMissionRunner) finds every mission by itself, starts the ones
 * that are due, follows their objectives and records them in the session's campaign record by Id.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UMissionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** "DA_Mission_Main1" is the mission "Main1" unless it names its own Id. */
	static constexpr const TCHAR* AssetPrefix = TEXT("DA_Mission_");

	/** Where the mission assets are made and kept. */
	static constexpr const TCHAR* AssetFolder = TEXT("/Game/Data/Missions");

	/**
	 * What saves call it (the campaign record, other missions' prerequisites, the console): never change or reuse one once
	 * a session may have saved it. None: the asset's name without DA_Mission_.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FText Title;

	/** What it's about, for the Missions page. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission", meta = (MultiLine = true))
	FText Summary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	EMissionKind Kind = EMissionKind::Main;

	/** Missions to finish first, by id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TArray<FName> Prerequisites;

	/** The area it's played in, by area id (UAreaDefinition::GetAreaId): it starts and runs only in that area's level. None: anywhere. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	FName Area;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Start")
	EMissionStart Start = EMissionStart::Automatic;

	/** The event that starts it (OnEvent): a name a speaker point or a notice board sends. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Start", meta = (EditCondition = "Start == EMissionStart::OnEvent", EditConditionHides))
	FName StartEvent;

	UPROPERTY(EditAnywhere, Category = "Mission")
	TArray<FMissionStep> Steps;

	UPROPERTY(EditAnywhere, Category = "Mission")
	FMissionRewards Rewards;

	/** Order in lists, lower first (within main, side and tutorial), then by id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 SortOrder = 0;

	/** Its id: Id, or the asset's name without DA_Mission_. */
	FName GetMissionId() const;

	/** The objective at Index of step StepIndex, or null. */
	const UMissionObjective* GetObjective(int32 StepIndex, int32 Index) const;

	/** The words name this mission: its id, its asset's name or its title, ignoring case, spaces and punctuation. */
	bool IsNamed(const FString& Words) const;

	/** Every mission asset in the project (loaded), by kind (main, side, tutorial), then SortOrder, then id. */
	static TArray<UMissionDefinition*> LoadAll();

	/** "Main", "Side", "Tutorial". */
	static FText KindName(EMissionKind InKind);
};
