#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MapDiscoverySubsystem.generated.h"

/**
 * What the player has come across in this level, for the map page's pins: the loot chests they've been near (within the
 * minimap's reach, DiscoverRadius) are marked from then on, opened or not. A look twice a second, cheap: a level has a
 * handful of chests. The session keeps what is found with the map's world, so the pins survive a Save & Quit and Continue:
 * an opened chest is kept open (FSavedMapWorld::OpenedChests) and stays on the map as looted; one found but not opened is
 * kept too (FSavedMapWorld::FoundChests: USessionSubsystem::CaptureWorld takes GetUnopenedFound, RestoreWorld gives it back
 * through RestoreFound).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMapDiscoverySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** How near the player comes for a chest to count as found (cm): what the HUD's minimap shows around them. */
	static constexpr double DiscoverRadius = 3500.0;

	/** Seconds between looks. */
	static constexpr float LookInterval = 0.5f;

	/** The chests found so far, by their save key (AChest::GetSaveKey). */
	const TSet<FName>& GetFoundChests() const { return FoundChests; }

	/** Counts a chest as found (the look; tests). */
	void MarkChestFound(FName ChestKey) { FoundChests.Add(ChestKey); }

	/**
	 * The chests found and not yet opened, by save key: what the session keeps. Only chests this level has, that the map shows
	 * (a grave or a mailbox never gets a pin) and whose loot is not out; an opened one is the session's OpenedChests instead.
	 */
	TArray<FName> GetUnopenedFound() const;

	/** Counts these chests as found, besides those found so far (the session, as a saved world is put back). */
	void RestoreFound(const TArray<FName>& ChestKeys);

	/** Looks round the local player now: every chest within DiscoverRadius is found. */
	void LookAround();

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMapDiscoverySubsystem, STATGROUP_Tickables); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	TSet<FName> FoundChests;
	float SinceLook = 0.f;
};
