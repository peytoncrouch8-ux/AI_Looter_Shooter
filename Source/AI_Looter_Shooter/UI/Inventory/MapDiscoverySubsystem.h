#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MapDiscoverySubsystem.generated.h"

/**
 * What the player has come across in this level, for the map page's pins: the loot chests they've been near (within the
 * minimap's reach, DiscoverRadius) are marked from then on, opened or not. A look twice a second, cheap: a level has a
 * handful of chests. Kept for this visit only; an opened chest is kept open by the session, so it stays on the map.
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
