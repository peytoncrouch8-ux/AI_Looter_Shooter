#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/WeaponTypes.h"
#include "LootFanfareSubsystem.generated.h"

class AActor;
class AWeaponBase;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
struct FLegendaryBurst;

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
 * moment it drops, and as it lands it bursts (FLegendaryBurst: a crack, a star, a core, rings, a galaxy's spiral of star
 * sparks; the user, 2026-10-27: "a galactic explosion instead of a game stutter"), a column of light stands over it (seen
 * from 60 m, as long as it lies there), its name pops up over it, motes rise and Loot.Drop.Legendary.Land hits. The world's
 * clock is never slowed (the drop's old 0.25 s at 0.3 read as a stutter).
 *
 * Nothing heavy is made as it lands (the hitch the user felt): the column and the name pop come from a pool built when
 * play begins (PoolSize of each, more only if more Legendaries lie at once), on an actor of the fanfare's own; a landing
 * attaches one to the gun, colours it and shows it, and gives it back when the gun is taken or gone.
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

	/** The landing shows playing (tests): spins, rings, Legendary columns, name pops and the burst's star sparks in flight. */
	int32 NumSpinning() const { return Spins.Num(); }
	int32 NumRings() const { return Rings.Num(); }
	int32 NumColumns() const { return Columns.Num(); }
	int32 NumNamePops() const { return NamePops.Num(); }
	int32 NumBurstSparks() const { return ArmSparks.Num() + FountainSparks.Num(); }

	/** The pooled columns and name pops waiting for a landing (tests). */
	int32 NumPooledColumns() const { return FreeColumns.Num(); }
	int32 NumPooledNamePops() const { return FreeNamePops.Num(); }

	/** Builds the pools now (play beginning does it; a test level, which doesn't begin play, calls it directly). */
	void WarmPools();

	/** Moves the shows on (the tick calls it; a test level, which doesn't tick, calls it directly). */
	void Advance(float DeltaTime);

	/** A thrown drop not landed after this long is taken as landed (stuck on something). */
	static constexpr float LandingTimeout = 5.f;
	/** A landing this soon after another's sound is heard only if it's rarer (a chest's guns land together). */
	static constexpr float CueGap = 0.35f;
	/** Columns and name pops built when play begins: two Legendaries lying at once is already rare (a boss's shower). */
	static constexpr int32 PoolSize = 2;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULootFanfareSubsystem, STATGROUP_Tickables); }
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Weapon has landed: its sound, its beam's flare, its announcement, its rarity's landing show. */
	void Land(AWeaponBase& Weapon, bool bStung);
	/** Weapon was just thrown: a Legendary's sting. */
	void Dropped(AWeaponBase& Weapon, bool& bOutStung);
	// LootFanfareShows.cpp: the landing's spin, ring, column and name pop, and the pools.
	void StartShows(AWeaponBase& Weapon);
	void AdvanceShows(float DeltaTime);
	void ClearShows();
	USceneComponent* PoolRoot();
	UStaticMeshComponent* MakeColumn();
	UWidgetComponent* MakeNamePop();
	UStaticMeshComponent* TakeColumn(AWeaponBase& Weapon, const FLinearColor& Color);
	UWidgetComponent* TakeNamePop(AWeaponBase& Weapon);
	void GiveBack(UStaticMeshComponent* Beam);
	void GiveBack(UWidgetComponent* Widget);
	// LootFanfareBurst.cpp: a Legendary's landing burst, and its star sparks each frame.
	void StartBurst(const FVector& Ground, const FLinearColor& Color, const FLegendaryBurst& Burst);
	void AdvanceBurst(float DeltaTime);

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
	/** A star spark of a burst's spiral arm: its path worked out from its start (it winds out, slowing, as the arm turns). */
	struct FArmSpark
	{
		FVector Center = FVector::ZeroVector;
		FVector Up = FVector::UpVector;
		FLinearColor Color = FLinearColor::White;
		/** Its arm's way at the heart (radians), how far it gets (cm), its height in the disc there (cm), its wind (radians). */
		float Angle = 0.f;
		float Reach = 0.f;
		float Height = 0.f;
		float Wind = 0.f;
		float Time = 0.f;
		float Seconds = 1.f;
		/** Its twinkle's phase. */
		float Seed = 0.f;
	};
	/** A star spark thrown up by a burst's fountain: flown each frame (drag and gravity). */
	struct FFountainSpark
	{
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FLinearColor Color = FLinearColor::White;
		float Time = 0.f;
		float Seconds = 1.f;
		float Seed = 0.f;
	};
	TArray<FSpin> Spins;
	TArray<FRing> Rings;
	TArray<FColumn> Columns;
	TArray<FNamePop> NamePops;
	TArray<FArmSpark> ArmSparks;
	TArray<FFountainSpark> FountainSparks;
	/** The burst's numbers as it began (its sparks draw by them to their end). */
	float SparkWidth = 6.f;
	float SparkTrail = 0.08f;
	float SparkHead = 18.f;
	float SparkMinAngle = 0.f;
	float SparkGlow = 1.f;

	/** The pools: an actor of the fanfare's own holds the columns and name pops not in use, hidden. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> PoolHolder;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> FreeColumns;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidgetComponent>> FreeNamePops;

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
