#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTierLook.h"
#include "CreatureTierFXSubsystem.generated.h"

class AActor;
class ACreatureBase;
class FWeaponFX;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * The enemy tiers' looks that belong to the level rather than to one creature (Saved/Tiers/DESIGN.md, "Readable and
 * rewarding"):
 *  - the tier shells' looks: M_CreatureTier (Tools/Unreal/build_tier_material.py), one material instance per kind and
 *    tier shared by every creature that looks alike (a horde of Ethereal spiders wears one), so a crowd costs no more
 *    uniform buffers than one (GetShellLook);
 *  - a top tier's death (PlayDeath; UCreatureTierLookComponent asks for it), on top of the kind's own burst
 *    (UCreatureHitReactionComponent): its death cue (Creature.Tier.Death.<Tier>) at the body, a soul-light burst in its
 *    colour bigger than the body's (FWeaponFX::SpawnDeathBurst), a ring of light racing out over the ground, a short
 *    pillar of soul-light shooting up from it (the loot beams' glow on the engine's cylinder: LightBeams; an Ethereal's
 *    flickers, and a second ring follows the first), motes rising round it, and a beat later the loot's pop where the
 *    drops fly out (Loot.Tier.Pop.<Tier>, a flash and a fountain of glints) unless it drops nothing.
 * No light anywhere (the Medium budget): the pillar is an emissive surface, the rest FWeaponFX's pooled streaks and
 * particles. At most MaxShows at once (the oldest gives way); it ticks only while one plays.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureTierFXSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UCreatureTierFXSubsystem* Get(const UObject* WorldContextObject);

	/** M_CreatureTier, the shells' material. */
	static const TCHAR* const ShellMaterialPath;

	/** The shared look for Shell at Rank (made the first time it's asked for); null without M_CreatureTier in the game. */
	UMaterialInstanceDynamic* GetShellLook(const FCreatureTierShell& Shell, ECreatureRank Rank);

	/** Creature, a top tier, has just died: its show plays (false: not a top tier, or no world). */
	bool PlayDeath(const ACreatureBase& Creature);

	/**
	 * The same show at a spot: Center the body's middle, Feet the ground under it, Size the body's size (cm, 100 for a
	 * spider at size 1), LootSpot where its drops fly out from, bLootPop whether they do.
	 */
	bool PlayDeathAt(ECreatureRank Rank, const FVector& Center, const FVector& Feet, float Size, const FVector& LootSpot, bool bLootPop);

	/** Moves every show on and draws it (the tick; a test level, which doesn't tick, calls it directly). */
	void Advance(float DeltaSeconds);

	/** Shows playing, pillars lit, streaks drawn last frame, loot pops played (tests). */
	int32 NumShows() const { return Shows.Num(); }
	int32 NumPillarsLit() const;
	int32 GetStreaksDrawn() const { return StreaksDrawn; }
	int32 GetPopsPlayed() const { return PopsPlayed; }
	int32 NumShellLooks() const { return ShellLooks.Num(); }

	/** The most shows at once, and the ring's pieces (and the spokes its ground is looked up along). */
	static constexpr int32 MaxShows = 6;
	static constexpr int32 RingSegments = 24;

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** A mote rising round the pillar, or a glint of the loot's pop: born at a moment of the show, glowing for its life. */
	struct FTierSpark
	{
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Born = 0.f;
		float Life = 1.f;
		float Phase = 0.f;
		float Width = 2.f;
		float Glow = 1.f;
		bool bLaunched = false;
		bool bHot = false;
	};

	struct FTierShow
	{
		ECreatureRank Rank = ECreatureRank::Hallowed;
		FCreatureTierDeathShow Look;
		FLinearColor Color = FLinearColor::White;
		FVector Center = FVector::ZeroVector;
		FVector Feet = FVector::ZeroVector;
		FVector LootSpot = FVector::ZeroVector;
		float Size = 100.f;
		float Time = 0.f;
		bool bLootPop = false;
		bool bPopped = false;
		/** The ground's height over Feet at the ring's end along each spoke (looked up once). */
		TArray<float> EdgeHeight;
		TArray<FTierSpark> Motes;
		TArray<FTierSpark> Glints;
		/** Its pillars' pair in the pool. */
		int32 Slot = 0;
	};

	// CreatureTierFXSubsystem.cpp: starting, the ground, the pool.
	void FindGround(FTierShow& Show) const;
	void SeedSparks(FTierShow& Show);
	/** The pool's pillar pair Slot (the glow, and its white-hot core), made the first time. */
	UStaticMeshComponent* PillarFor(int32 Slot, bool bCore);
	void HidePillars(int32 Slot);
	int32 FreeSlot() const;
	FVector CameraLocation() const;

	// CreatureTierFXSubsystemDraw.cpp: the show, frame by frame.
	void PlayPop(FTierShow& Show);
	void DrawPillars(FTierShow& Show);
	void DrawRings(FWeaponFX& Effects, const FTierShow& Show);
	void DrawSparks(FWeaponFX& Effects, FTierShow& Show, float DeltaSeconds, const FVector& Camera);

	TArray<FTierShow> Shows;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> ShellLooks;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ShellMaterial;
	bool bShellMaterialLooked = false;

	/** The pillars' pool: two cylinders a slot (MaxShows slots), on one transient actor. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> PillarHolder;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pillars;

	int32 StreaksDrawn = 0;
	int32 PopsPlayed = 0;
	FRandomStream Random{ 0x71e85 };
};
