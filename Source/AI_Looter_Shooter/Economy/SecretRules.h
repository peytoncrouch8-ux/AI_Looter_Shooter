#pragma once

#include "CoreMinimal.h"
#include "Story/StoryCondition.h"
#include "SecretRules.generated.h"

class UMissionRunner;
struct FCampaignRecord;

/**
 * What a secret Mister Sexton sells is (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 12): it sets the price tier
 * (SecretRules::BasePrice) and the row's mark. A find's kind is the find's own (FindRules); this is the shop's.
 */
UENUM(BlueprintType)
enum class ESecretKind : uint8
{
	/** A footlocker, a tin, a hoard: the common finds (40 silver at level 1). */
	Cache,
	/** A strongbox from the gang's bank years (100). */
	Strongbox,
	/** A find that holds a named gun (100). */
	Named,
	/** A named wanderer's beat (the Tollman; 150). */
	Elite,
	/** Pages to read (the gang's Ledger pages, all marked by one secret; 60). */
	Lore,
	/** Not a find: a place in the story (the Gravemother's barrow), marked by a tag until the story is done with it (60). */
	Story,
};

/**
 * One secret for sale (USecretCatalog::Secrets; Tools/Unreal/create_secret_assets.py from story_texts.SEXTON_SECRETS):
 * what it is called and says, what it costs, what it marks and when it's on the list. Bought, it's the story flag
 * SecretSold.<Id> (SecretRules::SoldFlag): never rename an id.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSecretOffer
{
	GENERATED_BODY()

	/** Saved as SecretSold.<Id>: never renamed once sold in a session. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FName Id;

	/** Its name on the list and on its map pin (the find's banner name: "The Widow's Strongbox"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FText Title;

	/** What the list says before it's bought ("A strongbox from the gang's bank years."). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FText Teaser;

	/** His line, once bought: on the row and the map pin ("Under the middle Widow, friend. Mind your back."). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FText Line;

	/** The map's region it lies in ("Farm & Point"; "The Rest" for one across the level). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FText Region;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	ESecretKind Kind = ESecretKind::Cache;

	/** Silver at level 1 (grown with the player's level as the vendors' prices are); 0: the kind's (SecretRules::BasePrice). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret", meta = (ClampMin = "0"))
	int32 Price = 0;

	/** The finds it marks (AFindSpot::FindId): it's done once every one is found (Find.<Id>). Empty for a story secret. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	TArray<FName> FindIds;

	/** A story secret's place: the actors carrying this tag are marked (Place_SinkFloor). Unused with finds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FName MarkTag;

	/** It's on the list from then (a region as the story reaches it); empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FStoryCondition AvailableWhen;

	/** A story secret's end (after Side3): off the list and the map then. Empty: only its finds end it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FStoryCondition DoneWhen;

	/** A story flag set as it's sold, for a line that knows (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FName SetsFlag;
};

/** His line over the list at a point in the story (USecretCatalog::Greetings: the first that holds). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSecretGreeting
{
	GENERATED_BODY()

	/** When it's said; empty: always (the last, a fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FStoryCondition When;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
	FText Line;
};

/** Where a secret stands in a story (SecretRules::StateOf). */
enum class ESecretState : uint8
{
	/** Not on the list yet (AvailableWhen not met). */
	Hidden,
	/** On the list, to buy. */
	ForSale,
	/** Bought and not found yet: marked on the map, shown on the list under MARKED. */
	Sold,
	/** Found (every find), or its story is done: off the list and the map. */
	Done,
};

/** One row of the list as the screen shows it (USecretSubsystem::GetRows), its words copied out of the catalog. */
struct AI_LOOTER_SHOOTER_API FSecretRow
{
	FName Id;
	FText Title;
	FText Teaser;
	FText Line;
	FText Region;
	ESecretKind Kind = ESecretKind::Cache;
	/** ForSale or Sold (the list shows nothing else). */
	ESecretState State = ESecretState::ForSale;
	/** What it costs the player now (level and discount counted); paid already when Sold. */
	int32 Price = 0;
	/** How many places it marks in this level (unfound ones; 6 for the Ledger pages at first). */
	int32 Places = 0;
};

/** One place a bought secret marks (USecretSubsystem::GetMarks): the map page's pin and the minimap's keyhole. */
struct AI_LOOTER_SHOOTER_API FSecretMark
{
	FName Id;
	FVector Location = FVector::ZeroVector;
	FText Title;
	FText Line;
	ESecretKind Kind = ESecretKind::Cache;
};

/** How a buy went (USecretSubsystem::Buy). */
enum class ESecretBuy : uint8
{
	Bought,
	/** No such secret here, or it isn't on the list (not yet, or done). */
	NotForSale,
	/** Bought before. */
	AlreadySold,
	/** The purse is short. */
	NotEnoughSilver,
	/** No story or no wallet to buy with (a level with no session's record, no catalog). */
	NoTrade,
};

/** Sexton's secrets as plain rules (Looter.Story.Secrets.Rules tests them). */
namespace SecretRules
{
	/** A quarter off once the catalog's DiscountMission (Paid on Delivery) is done, unless the catalog says otherwise. */
	inline constexpr float DefaultDiscount = 0.25f;

	/** "SecretSold.<Id>": the story flag a secret is sold as (FCampaignRecord::StoryFlags). None for None. */
	AI_LOOTER_SHOOTER_API FName SoldFlag(FName Id);

	/** A kind's price at level 1: Cache 40, Strongbox 100, Named 100, Elite 150, Lore 60, Story 60. */
	AI_LOOTER_SHOOTER_API int32 BasePrice(ESecretKind Kind);

	/** Base at PlayerLevel (LooterEconomy::AtLevel), then Discount (0-1) off, rounded, at least 1 (Base 0 stays 0). */
	AI_LOOTER_SHOOTER_API int32 PriceAt(int32 Base, int32 PlayerLevel, float Discount);

	/** An offer's price now: its own Price (else its kind's) through PriceAt. */
	AI_LOOTER_SHOOTER_API int32 PriceFor(const FSecretOffer& Offer, int32 PlayerLevel, float Discount);

	/** Every one of its finds is found in Record (Find.<Id>); false for an offer with none. */
	AI_LOOTER_SHOOTER_API bool IsFound(const FSecretOffer& Offer, const FCampaignRecord& Record);

	/**
	 * Where Offer stands in Record's story: Done (found, or DoneWhen met), else Sold (its flag), else ForSale (AvailableWhen
	 * met or empty), else Hidden. Runner answers conditions on a mission under way (DuringMission); null is fine otherwise.
	 */
	AI_LOOTER_SHOOTER_API ESecretState StateOf(const FSecretOffer& Offer, const FCampaignRecord& Record, const UMissionRunner* Runner = nullptr);

	/** The list shows it (ForSale or Sold). */
	AI_LOOTER_SHOOTER_API bool IsListed(ESecretState State);

	/** A level's catalog: "Lvl_RansomsRest" (or "UEDPIE_0_Lvl_RansomsRest") -> "/Game/Data/Secrets/DA_Secrets_RansomsRest.DA_Secrets_RansomsRest". */
	AI_LOOTER_SHOOTER_API FString CatalogPathFor(const FString& LevelName);

	/** The row's small word for a kind: "CACHE", "STRONGBOX", "NAMED GUN", "ELITE", "LORE", "STORY". */
	AI_LOOTER_SHOOTER_API FText KindWord(ESecretKind Kind);

	/** "Marked on your map: The Widow's Strongbox." (Places > 1: "6 places marked on your map: ..."). */
	AI_LOOTER_SHOOTER_API FText MarkedWords(const FText& Title, int32 Places);

	/** "Not enough silver: 40 needed, you carry 12." */
	AI_LOOTER_SHOOTER_API FText ShortWords(int32 Price, int32 Carried);
}
