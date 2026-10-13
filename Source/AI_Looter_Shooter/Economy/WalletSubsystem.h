#pragma once

#include "CoreMinimal.h"
#include "Economy/VendorOffer.h"
#include "Subsystems/WorldSubsystem.h"
#include "WalletSubsystem.generated.h"

/** Why the silver carried changed (UWalletSubsystem::OnSilverChanged), so a display can say so. */
enum class ESilverChange : uint8
{
	/** Picked up: a kill's, a container's, a crate's (ASilverPickup). */
	Found,
	/** A mission's purse, at its turn-in. */
	Reward,
	/** A gun sold to a vendor. */
	Sold,
	/** Paid to a vendor. */
	Spent,
	/** Put back from a saved session (no fanfare). */
	Restored
};

/** The silver changed: the new total, by how much, and why. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSilverChanged, int32 /*Total*/, int32 /*Delta*/, ESilverChange /*Why*/);

/**
 * The player's purse in the level being played (Docs/Progression.md, "Economy"): the silver carried and the vendors' gun
 * of the day (FVendorOffer). A world subsystem: the session gives it what it saved as the level starts and keeps it as the
 * level ends or a trip leaves (USessionSubsystem::CaptureWorld, RestoreWorld; ULooterSessionSave::Silver and VendorOffer),
 * so it carries from level to level like the guns do. Without a session it starts empty, kept for that play only.
 * One player: the game is single-player.
 *
 * Silver comes in by ASilverPickup (kills, containers, crates), a mission's turn-in (MissionRewards::GiveSilver) and
 * selling guns, and goes out at the vendors (VendorTrade). The HUD's readout and the screens poll it or listen to
 * OnSilverChanged.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UWalletSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The wallet of WorldContextObject's world, or null (no world, a world kind without one). */
	static UWalletSubsystem* Get(const UObject* WorldContextObject);

	/** The silver carried in WorldContextObject's world (0 without a wallet). */
	static int32 SilverOf(const UObject* WorldContextObject);

	/** The local player's level, 1 without one (a test level), as the vendors price by it. */
	static int32 PlayerLevelOf(const UObject* WorldContextObject);

	/** Played levels, play in the editor and the tests' levels. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	int32 GetSilver() const { return Silver; }

	bool CanAfford(int32 Price) const { return Price >= 0 && Price <= Silver; }

	/** Adds up to Amount (held to LooterEconomy::MaxSilver); returns what went in. Tells OnSilverChanged when it changed. */
	int32 Add(int32 Amount, ESilverChange Why = ESilverChange::Found);

	/** Pays Price: false, nothing taken, when there isn't enough. A price of 0 is always paid. */
	bool Spend(int32 Price);

	/** Sets the silver carried (a session put back; the console). Tells OnSilverChanged as Restored. */
	void SetSilver(int32 Amount);

	// --- The gun of the day ---

	/** The gun of the day as last rolled (it may be due a new one: RefreshOffer). */
	const FVendorOffer& GetOffer() const { return Offer; }

	/**
	 * The gun of the day for a player at PlayerLevel now: rolled again first when due (none yet, a level gained, a day's trade
	 * gone by). Random: the roll's numbers (a test's seed); null: fresh ones.
	 */
	const FVendorOffer& RefreshOffer(int32 PlayerLevel, FRandomStream* Random = nullptr);

	/** The gun of the day was bought: sold out until the next. */
	void MarkOfferSold();

	/** Puts back a session's gun of the day as it was saved. */
	void SetOffer(const FVendorOffer& InOffer) { Offer = InOffer; }

	/** Now, as the gun of the day counts its day: the session's time played; without a session, this level's play time. */
	double GetClock() const;

	/** Every change of the silver carried. */
	FOnSilverChanged OnSilverChanged;

private:
	int32 Silver = 0;

	/** Transient: the session keeps it (its gun's kind is an asset reference, held for the collector here). */
	UPROPERTY(Transient)
	FVendorOffer Offer;
};
