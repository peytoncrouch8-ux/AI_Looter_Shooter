#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/WeaponTypes.h"
#include "LootFanfareSubsystem.generated.h"

class AWeaponBase;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLootAnnounced, const AWeaponBase* /*Weapon*/);

/**
 * The fanfare of a gun dropped by a kill or a chest (ULootLibrary::SpawnKillLoot, AChest's loot) as it lands: a sound by
 * its rarity (DropCue: only the landing's knock for a Common, a soft glint for an Uncommon, a rising chime for a Rare, a
 * brighter one for an Epic, a sting for a Legendary), a Rare or better's beam shooting up and flashing
 * (LightBeams::Flare; a cursed iron's guttering beam is left to gutter), and for an Epic or a Legendary the HUD's pickup
 * feed flashing its name (OnAnnounced). Guns put down any other way (a level's, a saved session's, the player's own)
 * land quietly. Does nothing while no drop is in the air and no beam flares.
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

	/** A thrown drop not landed after this long is taken as landed (stuck on something). */
	static constexpr float LandingTimeout = 5.f;
	/** A landing this soon after another's sound is heard only if it's rarer (a chest's guns land together). */
	static constexpr float CueGap = 0.35f;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULootFanfareSubsystem, STATGROUP_Tickables); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Weapon has landed: its sound, its beam's flare, its announcement. */
	void Land(AWeaponBase& Weapon);

	struct FExpected
	{
		TWeakObjectPtr<AWeaponBase> Weapon;
		float Waited = 0.f;
	};
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
