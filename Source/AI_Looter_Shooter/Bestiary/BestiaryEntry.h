#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/StoryCondition.h"
#include "BestiaryEntry.generated.h"

class AActor;
class UAnimationAsset;
class UMaterialInterface;
class UMissionRunner;
class USkeletalMesh;
class UStaticMesh;
struct FCampaignRecord;

/** The bestiary's sections, in the order it lists them. */
UENUM(BlueprintType)
enum class EBestiaryCategory : uint8
{
	/** Wildlife and monsters: spiders and whatever else lives out there. */
	Creature,
	/** Hostile people: bandits, raiders, their bosses. */
	Enemy,
	/** People and things who aren't against you: traders, quest givers, training dummies. */
	NPC,
	/** Allies who fight or travel with you. */
	Friend,
};

/**
 * What a page is about (Docs/Areas/RansomsRest.md, "The Ledger"): something met in the world, a character of the story
 * with nothing to meet, or one of the Ledger's seven names.
 */
UENUM(BlueprintType)
enum class EBestiaryPage : uint8
{
	/**
	 * An actor met in the world (a creature, an enemy): its numbers read from ActorClass, its kills counted, its model on
	 * the stand. Open once the player has met one, or once KnownWhen holds.
	 */
	Actor,
	/**
	 * A character of the story who is never fought: Grandma Delia behind her screen door, Tilly at her window, Father
	 * Aldana, Mister Sexton, Ranger Calder, Hob. Words, and a model on the stand when the page names one (PreviewMesh:
	 * Hob's), but no actor, numbers or kills; open once KnownWhen holds (empty: from the start).
	 */
	StoryCharacter,
	/**
	 * One of the seven names Sexton writes in the Ledger: who they are and what they carry, with their whereabouts
	 * (Habitat) blank until FoundWhen holds (the Keeper's Lantern has found them). Written in the Ledger only, and open
	 * from the start: the name is the debt.
	 */
	LedgerName,
};

/** What the bestiary shows about an entry, read from its actor class's defaults so it never drifts from the game. */
struct FBestiaryStats
{
	int32 Level = 0;
	float Health = 0.f;
	/** Average damage of one attack; 0 when it doesn't fight. */
	float AttackDamage = 0.f;
	int32 XPReward = 0;
	/** It has stats at all (a creature or something with health), as opposed to a story character. */
	bool bHasHealth = false;
	bool bAttacks = false;
};

/** A mesh the actor wears on a bone of its body besides the body itself (the Unpaid's hat), as the stand puts it on. */
struct FBestiaryStandPart
{
	UStaticMesh* Mesh = nullptr;
	/** The bone it's worn on, and where on that bone. */
	FName Bone;
	FTransform Relative;
	/** Its materials as the actor wears them, slot by slot. */
	TArray<UMaterialInterface*> Materials;
};

/** A still model on one of the still stand model's sockets (Sexton on his rail's Sit, his ledger on its Ledger). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FBestiaryStillPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** The stand model's socket it goes on, snapped (its SOCKET_ prefix left off, as Unreal names sockets). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	FName Socket;
};

/**
 * One page of the bestiary (the inventory's second tab; Sexton's Ledger from Main 2): a creature, enemy, NPC or friend.
 * Make one per kind of character in /Game/Data/Bestiary (right-click > Miscellaneous > Data Asset > Bestiary Entry, or
 * duplicate one; Tools/Unreal/create_bestiary_pages.py writes the story's); the bestiary finds them all by itself. An
 * actor page's level, health, attack and experience come from ActorClass, and kills of it are counted, so only the words
 * and the picture are written here. A story character's page and a Ledger name need no actor at all (Page).
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UBestiaryEntry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	EBestiaryCategory Category = EBestiaryCategory::Creature;

	/** What it is, in a few words: "Arachnid", "Training target". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	FText Kind;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary", meta = (MultiLine = true))
	FText Description;

	/** Where it's found: "Tutorial Island: the forest". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	FText Habitat;

	/** Short field notes, one line each: weak spots, habits, how to fight it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	TArray<FText> Notes;

	/**
	 * The actor in the world (a creature class, a Blueprint): its stats are read from it and kills of it are counted. An
	 * actor page needs one; a story's page (Page) has none.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	TSoftClassPtr<AActor> ActorClass;

	/** The model on the bestiary's stand. Empty: the actor class's own skeletal mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	TSoftObjectPtr<USkeletalMesh> PreviewMesh;

	/**
	 * A still model on the stand instead, for a figure that isn't skinned (Mister Sexton, seated on his rail as
	 * SM_MisterSexton): shown when there's no skeletal model (PreviewMesh or the actor's).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	TSoftObjectPtr<UStaticMesh> PreviewStaticMesh;

	/**
	 * Still models on PreviewStaticMesh's sockets: a seated figure on a stand piece that grounds him (SM_SextonStand, a
	 * length of the lookout's rail, with Sexton on its Sit socket and his ledger on its Ledger socket).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	TArray<FBestiaryStillPart> PreviewStillParts;

	/** Loops on the stand, if set. Without one the model stands in its modeled pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	TSoftObjectPtr<UAnimationAsset> PreviewAnimation;

	/** Turns the model on the stand (degrees) when its front isn't the mesh's +X. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Stand")
	float PreviewYaw = 0.f;

	/** Order within its section; lower comes first, ties by name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary")
	int32 SortOrder = 0;

	// --- The story's pages (the Ledger) ---

	/** What the page is about: an actor met in the world, a story character, or a Ledger name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Story")
	EBestiaryPage Page = EBestiaryPage::Actor;

	/**
	 * Its page is open once this holds, as if the player had met one: a story character's once the story has introduced
	 * them (Tilly after Main 3, Hob after Main 1). Empty: an actor page waits to be met in the world; a story character's
	 * is open from the start.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Story")
	FStoryCondition KnownWhen;

	/** Written in the Ledger: listed only once Sexton has handed it over (Ledger::IsOpen). A Ledger name always is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Story")
	bool bLedgerOnly = false;

	/**
	 * A Ledger name's whereabouts (Habitat) are written in once this holds: the Keeper's Lantern has found them (Lucky Ned
	 * after Main 7). Until then they read blank. Empty: blank until a mission says.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bestiary|Story")
	FStoryCondition FoundWhen;

	/**
	 * It's about an actor in the world, whose class and model it needs (an actor page). A story's page needs neither, and
	 * shows the model it names, if any.
	 */
	bool NeedsActor() const { return Page == EBestiaryPage::Actor; }

	/** It's in the book now: every page but the Ledger's own, and with the Ledger open (bLedgerOpen) those too. */
	bool IsListed(bool bLedgerOpen) const;

	/**
	 * Its page is open: an actor page once the player has met one (bMet) or KnownWhen holds; a story character's once
	 * KnownWhen holds (empty: always); a Ledger name always.
	 */
	bool IsKnown(bool bMet, const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr) const;

	/** A Ledger name's whereabouts are written in: FoundWhen holds, and there's a Habitat to show. */
	bool IsFound(const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr) const;

	/** Level, health, attack and experience from ActorClass's defaults (loads the class). */
	FBestiaryStats ReadStats() const;

	/** PreviewMesh, or the first skeletal mesh on ActorClass's defaults. */
	USkeletalMesh* LoadPreviewMesh() const;

	/** The still model the stand shows when there's no skeletal one (PreviewStaticMesh), or null. */
	UStaticMesh* LoadPreviewStaticMesh() const;

	/**
	 * The materials the actor wears on this mesh in the world, slot by slot (its Blueprint may override the mesh's own),
	 * so the stand shows it as it looks out there. Empty when the actor doesn't use this mesh.
	 */
	TArray<UMaterialInterface*> GetPreviewMaterials(const USkeletalMesh* Mesh) const;

	/**
	 * What the actor wears on this mesh's bones besides the body (a hat), as its class's defaults put it on: the stand
	 * puts it on too, so the actor looks whole. Empty when the actor doesn't use this mesh (another may lack the bones).
	 */
	TArray<FBestiaryStandPart> GetPreviewParts(const USkeletalMesh* Mesh) const;

	/** The actor is of this entry's kind (ActorClass or a child of it). */
	bool Describes(const UClass* ActorType) const;

	/** "Creatures", "Enemies", "NPCs", "Friends". */
	static FText CategoryName(EBestiaryCategory Category);

	/** Every entry in the project (loaded), by section, then SortOrder, then name. */
	static TArray<UBestiaryEntry*> LoadAll();
};
