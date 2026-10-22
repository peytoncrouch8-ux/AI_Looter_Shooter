#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/WeaponTypes.h"
#include "LootFanfareSubsystem.generated.h"

class AWeaponBase;
class UStaticMeshComponent;
class UWidgetComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLootAnnounced, const AWeaponBase* /*Weapon*/);

/**
 * The fanfare of a gun dropped by a kill or a chest (ULootLibrary::SpawnKillLoot, AChest's loot) as it lands: a sound by
 * its rarity (DropCue: only the landing's knock for a Common, a soft glint for an Uncommon, a rising chime for a Rare, a
 * brighter one for an Epic, a sting for a Legendary), a Rare or better's beam shooting up and flashing
 * (LightBeams::Flare; a cursed iron's guttering beam is left to gutter), and for an Epic or a Legendary the HUD's pickup
 * feed flashing its name (OnAnnounced). Guns put down any other way (a level's, a saved session's, the player's own)
 * land quietly. Does nothing while no drop is in the air and no beam flares.
 *
 * The bigger drop moments (Phase 12.3; LootDropMoments has every number): an Epic or a Legendary spins as it lands and
 * a ring of light in its colour runs out along the ground; a Legendary is felt before it's seen: its sting plays the
 * moment it drops and the world slows for a breath (0.25 s, never inside another slow), and as it lands a column of light
 * stands over it (seen from 60 m, as long as it lies there), its name pops up over it, motes rise and
 * Loot.Drop.Legendary.Land hits.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULootFanfareSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Weapon was just thrown as a drop: its fanfare plays as it lands. */
	static void ExpectLanding(AWeaponBase* Weapon);

	/** The sound a dropped gun of Rarity makes as it lands (none for a Common: its knock, Loot.Land, is enough). */
	static FName DropCue(EWeaponRarity Rarity);

	/** Whether that sound is heard flat, wherever the player looks (an Epic's and a Legendary's), not from where it fell. */
	static bool IsDropCueFlat(EWeaponRarity Rarity);

	/** Whether a dropped gun of Rarity flares its beam as it lands (Rare and up). */
	static bool FlaresBeam(EWeaponRarity Rarity);

	/** Whether a dropped gun of Rarity is announced in the pickup feed (Epic and up). */
	static bool IsAnnounced(EWeaponRarity Rarity);

	/** An Epic or Legendary drop landed (for the pickup feed's flash). */
	FOnLootAnnounced OnAnnounced;

	/** Drops still in the air, and beams flaring. */
	int32 NumExpected() const { return Expected.Num(); }
	int32 NumFlaring() const { return Flaring.Num(); }

	/** The landing shows playing (tests): spins, rings, Legendary columns and name pops; the world held slow by a drop. */
	int32 NumSpinning() const { return Spins.Num(); }
	int32 NumRings() const { return Rings.Num(); }
	int32 NumColumns() const { return Columns.Num(); }
	int32 NumNamePops() const { return NamePops.Num(); }
	bool IsSlowingWorld() const { return SlowEndsAt >= 0.0; }

	/** Moves the shows on (the tick calls it; a test level, which doesn't tick, calls it directly). */
	void Advance(float DeltaTime);

	/** A thrown drop not landed after this long is taken as landed (stuck on something). */
	static constexpr float LandingTimeout = 5.f;
	/** A landing this soon after another's sound is heard only if it's rarer (a chest's guns land together). */
	static constexpr float CueGap = 0.35f;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULootFanfareSubsystem, STATGROUP_Tickables); }
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Weapon has landed: its sound, its beam's flare, its announcement, its rarity's landing show. */
	void Land(AWeaponBase& Weapon, bool bStung);
	/** Weapon was just thrown: a Legendary's sting and the world's breath of slow. */
	void Dropped(AWeaponBase& Weapon, bool& bOutStung);
	/** The world back to full speed, if this slowed it and nothing has changed it since. */
	void EndSlowDown();
	// LootFanfareShows.cpp: the landing's spin, ring, column and name pop.
	void StartShows(AWeaponBase& Weapon);
	void AdvanceShows(float DeltaTime);
	void ClearShows();

	struct FExpected
	{
		TWeakObjectPtr<AWeaponBase> Weapon;
		float Waited = 0.f;
		/** Its sting played as it dropped (a Legendary): it lands with its impact instead. */
		bool bStung = false;
	};
	struct FSpin
	{
		TWeakObjectPtr<AWeaponBase> Weapon;
		float Time = 0.f;
		float Seconds = 0.f;
		float Rate = 0.f;
		/** The pickup's own spin, which it eases back to. */
		FRotator OwnRate = FRotator::ZeroRotator;
	};
	struct FRing
	{
		FVector Center = FVector::ZeroVector;
		FLinearColor Color = FLinearColor::White;
		float Time = 0.f;
		float Seconds = 0.f;
		float Radius = 0.f;
	};
	struct FColumn
	{
		TWeakObjectPtr<AWeaponBase> Weapon;
		TWeakObjectPtr<UStaticMeshComponent> Beam;
		float Time = 0.f;
		float Glow = 0.f;
		float Height = 0.f;
		float Radius = 0.f;
	};
	struct FNamePop
	{
		TWeakObjectPtr<AWeaponBase> Weapon;
		TWeakObjectPtr<UWidgetComponent> Widget;
		float Time = 0.f;
		float Seconds = 0.f;
	};
	TArray<FSpin> Spins;
	TArray<FRing> Rings;
	TArray<FColumn> Columns;
	TArray<FNamePop> NamePops;
	/**
	 * A drop's slow-down: when it ends (the world's real seconds; -1 none), the speed it set, and its time left counted
	 * from the slowed ticks put back to real time (a test level's real clock stands still; whichever ends first ends it).
	 */
	double SlowEndsAt = -1.0;
	float SlowDilation = 1.f;
	float SlowLeft = 0.f;
	struct FFlaring
	{
		TWeakObjectPtr<UStaticMeshComponent> Beam;
		float Time = 0.f;
		bool bGrand = false;
		/** The beam's steady numbers, as the gun set them up, which the flare is a multiple of. */
		float Glow = 0.f;
		float Height = 0.f;
		float Radius = 0.f;
	};
	TArray<FExpected> Expected;
	TArray<FFlaring> Flaring;

	double LastCueTime = -1000.0;
	EWeaponRarity LastCueRarity = EWeaponRarity::Common;
};
