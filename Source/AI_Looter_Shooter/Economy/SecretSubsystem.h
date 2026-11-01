#pragma once

#include "CoreMinimal.h"
#include "Economy/SecretRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "SecretSubsystem.generated.h"

class AActor;
class AFindSpot;
class UMissionRunner;
class USecretCatalog;
struct FCampaignRecord;

/** A secret was sold (its id): the map and the minimap look again. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSecretSold, FName /*Id*/);

/**
 * Sexton's secrets in the level being played (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 12; lanes/SECRETS.md):
 * the level's catalog (USecretCatalog, found by the level's name: SecretRules::CatalogPathFor, loaded once), its rows for
 * the screen, buying one (the wallet's silver, the story flag SecretSold.<Id> saved with the campaign record, the
 * catalog's SetsFlag), and the places the bought ones mark for the map page (MapPins) and the minimap until they're found.
 *
 * A secret marks its finds' spots (AFindSpot by FindId: the ones not found yet) or, a story secret, the actors carrying
 * its MarkTag. A secret with nowhere to mark in this level (its find not built here) isn't listed. The level's spots and
 * tagged actors are looked up once and kept; a lookup that comes back short is tried again at most every LookAgainSeconds.
 *
 * Game worlds, play in the editor and the tests' editor preview worlds (SetCatalogForTesting gives a test its catalog;
 * the story is the mission runner's, or a test's record given to the calls that take one). Never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USecretSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static USecretSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** The level's catalog (loaded the first time it's asked for), or null: none for this level. */
	const USecretCatalog* GetCatalog() const;

	/** A test's catalog in place of the level's (null: look for the level's again). */
	void SetCatalogForTesting(USecretCatalog* InCatalog);

	/** He trades now: a catalog, a story (the runner's), its OpenWhen met. */
	bool IsOpen() const;

	/** The share off now (the catalog's discount once its mission is done). */
	float GetDiscount() const;

	/** His line over the list now; SoldOut when nothing is left for sale. */
	FText GetGreeting() const;

	/**
	 * The list as the screen shows it, for the player's level now: every secret for sale (the catalog's order), then the
	 * ones bought and not found yet; none Hidden or Done, none with nowhere to mark here. Record: the runner's without one.
	 */
	TArray<FSecretRow> GetRows() const;
	TArray<FSecretRow> GetRows(const FCampaignRecord& Record, const UMissionRunner* Runner, int32 PlayerLevel) const;

	/**
	 * Buys secret Id for the player: its price out of the wallet, its flag set (saved soon after, listeners told), its
	 * SetsFlag, OnSold. bFree (the console): no silver taken. Nothing changes unless it's Bought.
	 */
	ESecretBuy Buy(FName Id, bool bFree = false);

	/** Unsells Id (the console): its flag cleared, its marks gone. False when it wasn't sold. */
	bool Forget(FName Id);

	/** Unsells every secret here. Returns how many were sold. */
	int32 ForgetAll();

	/** The places the bought, unfound secrets mark now (the runner's record; none without a story). */
	TArray<FSecretMark> GetMarks() const;
	TArray<FSecretMark> GetMarks(const FCampaignRecord& Record, const UMissionRunner* Runner = nullptr) const;

	/** How many places Offer marks in this level now (its unfound finds' spots, or its tagged actors). */
	int32 CountPlaces(const FSecretOffer& Offer, const FCampaignRecord& Record) const;

	FOnSecretSold OnSold;

	/** A lookup that came back short is tried again no sooner than this (s, world time). */
	static constexpr double LookAgainSeconds = 5.0;

private:
	/** The runner's record while its story is active, else null. */
	const FCampaignRecord* GetRecord() const;
	UMissionRunner* GetRunner() const;
	/** Offer's places now, added to Out. */
	void GatherPlaces(const FSecretOffer& Offer, const FCampaignRecord& Record, TArray<FVector>& Out) const;
	AFindSpot* FindSpot(FName FindId) const;
	void FindTagged(FName Tag, TArray<AActor*>& Out) const;

	/** Loaded on the first ask (GetCatalog is const: it writes these through a cast, once). */
	UPROPERTY(Transient)
	TObjectPtr<USecretCatalog> Catalog;
	bool bCatalogLooked = false;
	bool bTestCatalog = false;

	mutable TMap<FName, TWeakObjectPtr<AFindSpot>> Spots;
	mutable double SpotsLookedAt = -1.0e9;
	mutable TMap<FName, TArray<TWeakObjectPtr<AActor>>> Tagged;
	mutable TMap<FName, double> TaggedLookedAt;
};
