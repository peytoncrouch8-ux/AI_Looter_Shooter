#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/StoryLine.h"
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

/**
 * How a mission whose objectives are all done is finished. Borderlands' way: it's turned in to someone (the one who gave
 * it, or who it's for), and only then are its rewards given. Until then it's "ready to turn in": the tracker says "Turn in
 * to <who>" with its arrow on them, and a session saves it so.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionTurnIn
{
	GENERATED_BODY()

	/**
	 * It finishes by itself the moment its last objective is done (Skyreach's tutorial and skiff, the test mission, a boss
	 * fight that ends in its own scene): no one to turn it in to.
	 */
	UPROPERTY(EditAnywhere, Category = "Turn-in")
	bool bAutomatic = false;

	/**
	 * Who it's turned in to, by the tag their speaker point's actor carries (Speaker_Delia, Speaker_Tilly): talking to them
	 * there turns it in, and the tracker's arrow and the minimap point at them meanwhile. When the last objective would be
	 * talking to them, that talk is the turn-in instead: the steps end one earlier.
	 */
	UPROPERTY(EditAnywhere, Category = "Turn-in", meta = (EditCondition = "!bAutomatic"))
	FName SpeakerTag;

	/** Their name for people: "Delia" ("Turn in to Delia"). Empty: from the tag (Speaker_Delia: "Delia"). */
	UPROPERTY(EditAnywhere, Category = "Turn-in", meta = (EditCondition = "!bAutomatic"))
	FText GiverName;

	/**
	 * What they say as it's turned in, said instead of what they'd say there otherwise (their speaker point's lines at
	 * this point in the story). Empty: their own lines, for a talk the story already wrote as the mission's end (Delia's,
	 * Tilly's, Aldana's, Amos's). A line with no speaker is said in their speaker point's name.
	 */
	UPROPERTY(EditAnywhere, Category = "Turn-in", meta = (EditCondition = "!bAutomatic"))
	TArray<FStoryLine> Lines;
};

/** What finishing a mission gives, once per session (a mission played again gives nothing more). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionRewards
{
	GENERATED_BODY()

	/**
	 * Experience, a fixed amount: what the mission is worth, tuned with the area's kills to the level its story should end
	 * on (Docs/Progression.md). Above 0 it's what's given, and ExperienceShare is left out.
	 */
	UPROPERTY(EditAnywhere, Category = "Rewards", meta = (ClampMin = "0"))
	int32 Experience = 0;

	/**
	 * Experience as this share of what the player's current level takes, so it's worth the same part of a level at any
	 * level; only when Experience is 0 (the test mission's 10%).
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

	/**
	 * A named gun by its id (UNamedWeaponDefinition, DA_Named_<Id> in /Game/Data/Weapons: "Heirloom"), dropped at the
	 * player's feet at their level as the gun above is: its own parts, rarity, name and line.
	 */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	FName NamedGun;

	/**
	 * The named gun is handed over in the story rather than dropped as the mission ends (Main 7: Delia hands Heirloom out
	 * through her door, ADoorHandoff): it's listed among the rewards, and the hand-off gives it, once.
	 */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	bool bNamedGunByHand = false;

	/** Areas opened to travel (the station boards list them), by area id (UAreaDefinition::GetAreaId). */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	TArray<FName> UnlockAreas;

	bool GivesExperience() const { return Experience > 0 || ExperienceShare > 0.f; }

	bool IsEmpty() const { return !GivesExperience() && !bGun && NamedGun.IsNone() && UnlockAreas.IsEmpty(); }
};

/**
 * One mission as a data asset in /Game/Data/Missions (DA_Mission_<Id>; Tools/Unreal/create_mission_assets.py makes the
 * first): its words, whether it's main or side, what must be finished first, the area it's played in, what starts it,
 * its steps of objectives, who it's turned in to and its rewards. The objectives are instanced objects, one C++ class per
 * kind (UMissionObjective). The mission runner of each level (UMissionRunner) finds every mission by itself, starts the
 * ones that are due, follows their objectives, waits for the turn-in and records them in the session's campaign record
 * by Id.
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

	/** Who it's turned in to once its objectives are done, or that it finishes by itself. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	FMissionTurnIn TurnIn;

	UPROPERTY(EditAnywhere, Category = "Mission")
	FMissionRewards Rewards;

	/** Order in lists, lower first (within main, side and tutorial), then by id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	int32 SortOrder = 0;

	/** Its id: Id, or the asset's name without DA_Mission_. */
	FName GetMissionId() const;

	/** The objective at Index of step StepIndex, or null. */
	const UMissionObjective* GetObjective(int32 StepIndex, int32 Index) const;

	/**
	 * It waits to be turned in once its objectives are done: it names a giver and isn't automatic. A mission made in code
	 * with no giver (a test's) finishes by itself, as missions did before turn-ins.
	 */
	bool NeedsTurnIn() const { return !TurnIn.bAutomatic && !TurnIn.SpeakerTag.IsNone(); }

	/** The giver's name for people: TurnIn.GiverName, else the speaker tag without "Speaker_" as words. Empty without a giver. */
	FText GetGiverName() const;

	/** "Turn in to Delia": the HUD tracker's line while it waits. */
	FString GetTurnInShortText() const;

	/** "Ready to turn in: talk to Delia": the Missions page's and the full objective line while it waits. */
	FString GetTurnInText() const;

	/** The words name this mission: its id, its asset's name or its title, ignoring case, spaces and punctuation. */
	bool IsNamed(const FString& Words) const;

	/** Every mission asset in the project (loaded), by kind (main, side, tutorial), then SortOrder, then id. */
	static TArray<UMissionDefinition*> LoadAll();

	/** "Main", "Side", "Tutorial". */
	static FText KindName(EMissionKind InKind);
};
