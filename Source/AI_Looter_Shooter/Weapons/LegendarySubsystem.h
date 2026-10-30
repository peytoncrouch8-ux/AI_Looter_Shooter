#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/LegendaryRules.h"
#include "LegendarySubsystem.generated.h"

class ACreatureBase;
class ALegendaryLantern;
class APawn;
class AWeaponBase;
class UBulletSubsystem;
struct FBulletLanding;
struct FBulletShot;
struct FBulletStrike;
struct FHitResult;

/**
 * The unique legendaries in play (LegendaryRules; the user's call, 2026-10-29 night): listens to UBulletSubsystem's
 * hooks as UPartEffectSubsystem does, for the guns whose named gun carries a signature.
 *  - Firing: the gun notes the round (ULegendaryEffectComponent: Company Scrip's burst, Lamplighter's scope).
 *  - Landing and struck: whether this round's hit killed what it struck (alive as it landed, dead once dealt).
 *  - Gravesilk: every hit lays silk (LaySilk); the stack that roots it holds it in place (the movement slow at its floor)
 *    and pins it with strands drawn each frame while it lasts. Never a boss.
 *  - A kill (NoteKill): Company Scrip stacks its fire rate; Lamplighter's scoped one lights a lantern at the body's feet
 *    (ALegendaryLantern).
 * Every number: FLegendaryTuning (Looter.Legendary.*). It ticks for the rooted creatures' strands, nothing else.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULegendarySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static ULegendarySubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** A Gravesilk hit on Creature now: a stack laid, and the root when it's the one that roots it. */
	LegendaryRules::ESilk LaySilk(ACreatureBase& Creature);

	/** Gravesilk's stacks on Creature now (0 for none, or fallen away). */
	int32 SilkOn(const ACreatureBase* Creature) const;

	/** Creature is held by Gravesilk's root now. */
	bool IsRooted(const ACreatureBase* Creature) const;

	/** Gun's bullet killed Victim: its signature's kill (Company Scrip's stack, Lamplighter's lantern when scoped). */
	void NoteKill(AWeaponBase& Gun, ACreatureBase& Victim);

	/** Lights a lantern at Ground that heals Healer (oldest out past Looter.Legendary.Lamplighter.MaxLanterns). */
	ALegendaryLantern* LightLantern(const FVector& Ground, APawn* Healer);

	/**
	 * Lantern may heal Pawn this frame: the first lantern to ask this frame does (and may again), the rest don't, so
	 * lanterns never stack on one player.
	 */
	bool ClaimHeal(const APawn* Pawn, const ALegendaryLantern* Lantern);

	/** The lanterns burning now. */
	int32 NumLanterns() const;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void NoteFiring(FBulletShot& Shot);
	void NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing);
	void NoteStruck(const FBulletShot& Shot, const FHitResult& Hit, FBulletStrike& Strike);

	/** Holds Creature where it stands for the root's seconds, with its look and sound. */
	void Root(ACreatureBase& Creature, double Ends);
	/** The strands pinning a rooted creature to the ground, this frame. */
	void DrawStrands(const ACreatureBase& Creature, float Strength) const;

	double Now() const;

	TWeakObjectPtr<UBulletSubsystem> Bullets;
	FDelegateHandle FiringHandle;
	FDelegateHandle LandingHandle;
	FDelegateHandle StruckHandle;

	TMap<TWeakObjectPtr<ACreatureBase>, LegendaryRules::FSilk> Silk;

	struct FRooted
	{
		TWeakObjectPtr<ACreatureBase> Creature;
		double Starts = 0.0;
		double Ends = 0.0;
	};
	TArray<FRooted> Rooted;

	TArray<TWeakObjectPtr<ALegendaryLantern>> Lanterns;
	/** Who healed each player last, and in which frame. */
	struct FHealClaim
	{
		uint64 Frame = 0;
		TWeakObjectPtr<const ALegendaryLantern> Lantern;
	};
	TMap<TWeakObjectPtr<const APawn>, FHealClaim> HealClaims;

	/** What the round landing now struck, and whether it was alive as it landed (the struck hook says if it died). */
	TWeakObjectPtr<AActor> Landed;
	bool bLandedAlive = false;
};
