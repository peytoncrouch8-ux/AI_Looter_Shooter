#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Missions/MissionTargets.h"
#include "Story/StoryLine.h"
#include "Weapons/WeaponTypes.h"
#include "MissionDefinition.generated.h"

class UMissionObjective;
class UStaticMesh;
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
	/**
	 * Picked up from its giver (Offer), Borderlands' way: someone at a speaker point, a notice board or an artifact found in
	 * the world offers it once its prerequisites are finished, with a ? over them, and it starts only when the player accepts
	 * it on the offer card. Accepted, it runs from then on wherever the session goes on (FCampaignRecord::AcceptedMissions).
	 * A main mission is never offered (the user's call, 2026-10-22): it starts by itself the moment it's due, its giver's
	 * words said on the move, or, with FMissionOffer::bMeetFirst, it's listed with "Talk to <giver>" and starts as they're
	 * talked to (no card either way).
	 */
	Offered,
};

/**
 * Who offers a mission picked up from its giver (EMissionStart::Offered) and what the offer says: the giver is found by a
 * tag its actor carries (someone's speaker point, a notice board, a mission artifact), as turn-ins find theirs. The offer
 * card opens at once with the Briefing, a speaker saying Lines beside it; Accept goes to AcceptLines (2026-10-22). Who
 * offers it nothing before its prerequisites (its story gate) are finished: no mark, no card, no pin.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionOffer
{
	GENERATED_BODY()

	/** The giver actor's tag: Speaker_Delia, NoticeBoard_Rangers, Artifact_MinersTag. None: nobody offers it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offer")
	FName GiverTag;

	/** The giver's name for people ("Delia", "The Rim Rangers' board", "A miner's tag"). Empty: from the tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offer")
	FText GiverName;

	/**
	 * What a speaker says as they offer it, said instead of their lines at this point in the story, while the card is open
	 * beside them (the card opens at once: Accept never waits for the words). A line with no speaker is said in their speaker
	 * point's name. Empty: their own lines (boards and artifacts say nothing). A main mission's are said on the move as it
	 * starts by itself, or are the talk that starts it (bMeetFirst).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offer")
	TArray<FStoryLine> Lines;

	/**
	 * What the giver says once it's taken, Borderlands' way: Accept on the card cuts the offer's Lines short and goes
	 * straight to these. A main mission says them after its Lines. A line with no speaker is said in the giver's name. Empty:
	 * nothing more (the offer's words stop as it's accepted).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offer")
	TArray<FStoryLine> AcceptLines;

	/**
	 * A main mission whose story needs the player at its giver first (Main 3 and Main 6: Delia's word): from the moment it's
	 * due it's on the Missions page and tracked with "Talk to <giver>" (the arrow on them), and talking to them says Lines and
	 * starts it, no card. Off, a main mission starts by itself the moment it's due, Lines said on the move. Side missions
	 * ignore it: they're always offered on the card.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offer")
	bool bMeetFirst = false;

	/** The offer card's story, two to four sentences in the giver's voice; an artifact's own words when it's one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offer", meta = (MultiLine = true))
	FText Briefing;
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
	 * The silver its purse holds at the turn-in, the same at any level (Back Pay's payroll). 0: the giver's purse kept by
	 * mission id (MissionRewards::SilverFor), as the first missions' are.
	 */
	UPROPERTY(EditAnywhere, Category = "Rewards", meta = (ClampMin = "0"))
	int32 Silver = 0;

	/**
	 * Silver for each optional objective done (UMissionObjective::bOptional), paid at the turn-in: this at level 1, growing
	 * with the player's level as the economy's amounts do (LooterEconomy::AtLevel). Never experience.
	 */
	UPROPERTY(EditAnywhere, Category = "Rewards", meta = (ClampMin = "0"))
	int32 OptionalSilver = 10;

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
	 * through her door, ADoorHandoff): it's listed among the rewards, and the hand-off gives it, once. The turn-in makes sure
	 * of it (MissionRewards::EnsureNamedGun): one never taken is brought to the player, one never handed out is given, and
	 * one they carry gets no second.
	 */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	bool bNamedGunByHand = false;

	/** Areas opened to travel (the station boards list them), by area id (UAreaDefinition::GetAreaId). */
	UPROPERTY(EditAnywhere, Category = "Rewards")
	TArray<FName> UnlockAreas;

	bool GivesExperience() const { return Experience > 0 || ExperienceShare > 0.f; }

	bool IsEmpty() const { return !GivesExperience() && !bGun && NamedGun.IsNone() && UnlockAreas.IsEmpty() && Silver <= 0; }
};

/**
 * One of a turn-in's two choices (FMissionTurnIn::Choices): what it's called on the card, what it means in a line, what it
 * gives in place of the mission's own rewards, the story flag it sets for other code to read, and what's said once it's
 * chosen. Back Pay's: "Pay the eleven" (BackPay.Paid, a Rare gun) and "Keep the payroll" (BackPay.Kept, 400 silver).
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionTurnInChoice
{
	GENERATED_BODY()

	/** The choice on its button ("Pay the eleven"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FText Title;

	/** What it means, a sentence or two under it ("The tags go on OUT with their wages. The Providence's dead rest."). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice", meta = (MultiLine = true))
	FText Line;

	/**
	 * What it gives instead of the mission's own rewards: its silver, gun, named gun and areas. Its experience left at 0,
	 * the mission's own experience is given either way (the card says so above the two).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FMissionRewards Rewards;

	/** The campaign flag it sets (FCampaignRecord::StoryFlags): BackPay.Paid. None: none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FName Flag;

	/** What's said once it's chosen; a line with no speaker in the giver's speaker point's name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	TArray<FStoryLine> Lines;

	/** The word stamped over the card as it's chosen ("PAID", "KEPT"). Empty: "SETTLED". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Choice")
	FText Stamp;
};

/**
 * How a mission whose objectives are all done is finished. Borderlands' way: it's turned in to someone (the one who gave
 * it, or who it's for), and only then are its rewards given. Until then it's "ready to turn in": the tracker says "Turn in
 * to <who>" with its arrow on them, and a session saves it so. Anyone at a speaker point can take it (a door, a window, a
 * talking coffin, a speaking tube, a tag board), and so can a notice board or a mission artifact carrying the tag.
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

	/**
	 * A choice of two at the turn-in (Back Pay: pay the eleven, or keep the payroll): with two choices the talk that turns it
	 * in opens the offer card in its choice mode (ALooterHUD::OpenTurnInChoice) once Lines are said, and the mission finishes
	 * only when the player holds one of them down: its flag set, its own rewards given, its own lines said. Only the first
	 * two count; with fewer than two there's no choice.
	 */
	UPROPERTY(EditAnywhere, Category = "Turn-in", meta = (EditCondition = "!bAutomatic"))
	TArray<FMissionTurnInChoice> Choices;

	/** The choice card's question over the two ("Their wages, or the company's silver?"). Empty: the card's own words. */
	UPROPERTY(EditAnywhere, Category = "Turn-in", meta = (EditCondition = "!bAutomatic", MultiLine = true))
	FText ChoiceQuestion;

	/**
	 * What the giver hands over as it's turned in: items of a later mission (their ids, FMissionItemInfo::Id), carried
	 * from then on as if picked up (UMissionRunner::GiveItem), so nobody walks back for them. Tilly hands Pa's lantern bail
	 * (Main 6's) over at Main 3's turn-in (2026-10-21, the user's cut of the fetches); its sill pickup goes as it's carried.
	 */
	UPROPERTY(EditAnywhere, Category = "Turn-in")
	TArray<FName> HandItems;
};

/**
 * A thing that belongs to a mission (Pa's lantern bail, the winch crank, the Gravemother's fang): picked up from an
 * AMissionItem in the world, or dropped by the creature that carries it as it dies, kept by the campaign while its mission
 * runs (FCampaignRecord::CarriedItems), listed under Carried on the Missions page and taken away as the mission finishes.
 * A UMissionCollectItemObjective counts it carried.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FMissionItemInfo
{
	GENERATED_BODY()

	/** What the campaign keeps it by: never change or reuse one once a session may have saved it. Unique across missions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName Id;

	/** Its name for people ("Pa's lantern bail"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Name;

	/** A short line about it, for the Carried list ("The ring it hangs by, snapped off in his fist."). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Line;

	/** Its model, for a pickup that names none and for one dropped by a creature. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/**
	 * Who carries it, when a creature does (Abe Haskett, tagged; the Gravemother, by class): as one of them dies with the
	 * mission not finished and the item neither carried nor dropped before, it falls at the body as a pickup. Empty: none.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FMissionActorFilter DroppedBy;
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

	/**
	 * Missions to finish first, by id: a side mission's story gate (Side 1 after Main 3; the "After" column of
	 * Docs/Areas/RansomsRest_Expansion.md section 2). Until they're done nobody offers it: no mark, no card, no pin.
	 */
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

	/** Who offers it and what the offer says (Offered). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission|Start", meta = (EditCondition = "Start == EMissionStart::Offered", EditConditionHides))
	FMissionOffer Offer;

	UPROPERTY(EditAnywhere, Category = "Mission")
	TArray<FMissionStep> Steps;

	/** Who it's turned in to once its objectives are done, or that it finishes by itself. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	FMissionTurnIn TurnIn;

	UPROPERTY(EditAnywhere, Category = "Mission")
	FMissionRewards Rewards;

	/** The things that belong to it (mission items), picked up or dropped by a creature, carried until it finishes. */
	UPROPERTY(EditAnywhere, Category = "Mission")
	TArray<FMissionItemInfo> Items;

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

	/** It's picked up from a giver: it starts Offered and names its giver's tag. */
	bool IsOffered() const { return Start == EMissionStart::Offered && !Offer.GiverTag.IsNone(); }

	/** A main mission with a giver that starts by itself the moment it's due (main missions are never offered on a card). */
	bool IsMainByItself() const { return Kind == EMissionKind::Main && IsOffered() && !Offer.bMeetFirst; }

	/** A main mission that starts as its giver is talked to (Offer.bMeetFirst), listed with "Talk to <giver>" until then. */
	bool IsMetFirst() const { return Kind == EMissionKind::Main && IsOffered() && Offer.bMeetFirst; }

	/** Its turn-in asks the player to choose between two (FMissionTurnIn::Choices): it waits for a giver and has two. */
	bool HasTurnInChoice() const { return NeedsTurnIn() && TurnIn.Choices.Num() >= 2; }

	/** One of the turn-in's two choices (0 or 1), or null (no choice, or out of range). */
	const FMissionTurnInChoice* GetTurnInChoice(int32 Choice) const;

	/**
	 * What finishing it gives with a choice (0 or 1): the choice's rewards in place of its own, with its own experience when
	 * the choice names none, and its optional bonus always. Without a choice (INDEX_NONE, or none to make): Rewards.
	 */
	FMissionRewards RewardsFor(int32 Choice) const;

	/** One of its items by id, or null. */
	const FMissionItemInfo* FindItem(FName ItemId) const;

	/** How many optional objectives it has, in every step. */
	int32 CountOptional() const;

	/**
	 * The offering giver's name for people: Offer.GiverName, else the tag as words without its kind (Speaker_Delia: "Delia",
	 * NoticeBoard_Rangers: "Rangers", Artifact_MinersTag: "Miners Tag"). Empty when nobody offers it.
	 */
	FText GetOfferGiverName() const;

	/** The words name this mission: its id, its asset's name or its title, ignoring case, spaces and punctuation. */
	bool IsNamed(const FString& Words) const;

	/** Every mission asset in the project (loaded), by kind (main, side, tutorial), then SortOrder, then id. */
	static TArray<UMissionDefinition*> LoadAll();

	/** "Main", "Side", "Tutorial". */
	static FText KindName(EMissionKind InKind);
};
