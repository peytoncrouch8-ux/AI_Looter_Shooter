#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatRules.h"
#include "Elements/LooterElement.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/PartEffect.h"
#include "Weapons/BulletMarks.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponBrass.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponTypes.h"
#include "BulletSubsystem.generated.h"

class AController;
class AWeaponBase;
class UNiagaraSystem;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBulletHit, const FHitResult& /*Hit*/, float /*Damage*/, bool /*bCritical*/);

/** Everything a fired bullet needs to fly and to deal its hit, copied when it's fired (the gun may be gone by then). */
struct FBulletShot
{
	/** Where the bullet starts, on the shooter's aim line, and which way it flies. */
	FVector Start = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	/** Where the tracer leaves from: the muzzle as the player sees it. It closes onto the aim line as it flies. */
	FVector VisualStart = FVector::ZeroVector;
	/** cm/s; 0 = instant (hitscan). */
	float Speed = 30000.f;
	/** How far it can fly (cm). */
	float Range = 10000.f;
	/** Damage before the per-hit roll and the critical multiplier. */
	float Damage = 10.f;
	/** What a critical hit multiplies the damage by: the game's rule, or the gun's curse's (WeaponCurses::CritMultiplier). */
	float CritMultiplier = LooterCombat::CriticalHitMultiplier;
	/** The gun's stats, for its damage falloff with distance (FWeaponStats::DamageAtDistance); none = no falloff. */
	TOptional<FWeaponStats> FalloffStats;
	/** Push given to physics objects it hits. */
	float HitImpulse = 0.f;
	ECollisionChannel Channel = ECC_Visibility;

	FLinearColor TracerColor = FLinearColor(1.f, 0.62f, 0.25f);
	float TracerWidth = 3.f;
	float TracerLength = 380.f;
	/** Optional Niagara impact; empty = the built-in sparks, dust and splashes. */
	TWeakObjectPtr<UNiagaraSystem> ImpactFX;
	bool bDrawDebug = false;

	/** The gun (told about hits, for hit markers), who fired it (never hit by it) and who gets credit for the damage. */
	TWeakObjectPtr<AWeaponBase> Weapon;
	TWeakObjectPtr<AActor> Shooter;
	TWeakObjectPtr<AController> Instigator;

	/** Critical wherever it lands (Lucky Streak's every third round), at its CritMultiplier. */
	bool bSureCritical = false;
	/**
	 * Can't hurt (Double or Nothing's "nothing": thrown wide, it misses entirely): it flies and marks the world as any round
	 * does, but whatever has health that it meets takes nothing and shows nothing, and its gun is told no hit (no marker).
	 * OnBulletHit still reports where it struck, with 0 damage (the fauna scatter at a shot's landing all the same).
	 */
	bool bBlank = false;

	// --- The gun's parts' say (UPartEffectSubsystem stamps these as the shot is fired, from the gun's parts) ---

	/** The story element its rounds carry, and the chance (before the target's weakness) each hit sets its status. */
	ELooterElement Element = ELooterElement::None;
	float ElementChance = 0.f;
	/** What its gun's part makes it do (a bounce, a blast, a round through...). */
	EPartEffect Effect = EPartEffect::None;
	/** 0: a gun's own round; 1: one an effect made (a ricochet's bounce, a twin's second), which makes no more. */
	uint8 EffectGeneration = 0;
	/** Creatures it may still fly on through (Piercing). */
	int32 PierceLeft = 0;
	/** An actor it never hits: the creature a ricochet bounced off. */
	TWeakObjectPtr<AActor> IgnoreActor;
};

/** A bullet has dealt its hit (UBulletSubsystem::OnBulletStruck): what it dealt, and whether it flies on. */
struct FBulletStrike
{
	/** The damage it dealt (before the target's own defense), critical or not; 0 for a blank. */
	float Damage = 0.f;
	bool bCritical = false;
	/** How far it had flown (cm). */
	float Traveled = 0.f;
	/** A listener says it flies on through what it hit (a Piercing round through a creature). */
	bool bPassThrough = false;
	/** And with this share of its damage from here on. */
	float PassDamageScale = 1.f;
};

/** A bullet landing, before its damage is dealt (UBulletSubsystem::OnBulletLanding): what a power may change. */
struct FBulletLanding
{
	/** How far it flew (cm). */
	float Traveled = 0.f;
	/** It lands critical (the target's critical spot, or a sure critical): a power may make it so. */
	bool bCritical = false;
	/** Its damage times this (Long Shot's distance); 1 leaves it. */
	float DamageScale = 1.f;
};

/** A bullet is about to fly (UBulletSubsystem::Fire): the shot as it will fly, for a power to change (its damage, way, look). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBulletFiring, FBulletShot& /*Shot*/);
/** A bullet has met something and its damage is about to be rolled: the shot, the hit and what may change. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBulletLanding, const FBulletShot& /*Shot*/, const FHitResult& /*Hit*/, FBulletLanding& /*Landing*/);
/** A bullet's hit has been dealt: the shot, the hit and what it dealt; a listener may let it fly on (FBulletStrike::bPassThrough). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnBulletStruck, const FBulletShot& /*Shot*/, const FHitResult& /*Hit*/, FBulletStrike& /*Strike*/);

/**
 * Bullets in flight. Each fired bullet is a real projectile: it travels along the shooter's aim at its gun's bullet
 * speed, is traced segment by segment every frame, and deals its damage to whatever is in its path when it gets there
 * (the same damage rules as ever: the per-hit roll and the x1.5 critical spots, or a cursed iron's own critical
 * multiplier). Its tracer is drawn from the muzzle the player sees, closing onto the aim line over the first few meters,
 * so shots still land exactly on the crosshair.
 *
 * Where it lands, the world shows what it hit (ImpactSurface finds the surface: dirt, grass, gravel, stone, wood, metal,
 * glass, water...): its own puff (FWeaponFX::SpawnSurfaceImpact; the chunks it throws land on the ground under the hit
 * and lie a moment), its own sound (ImpactCueOf), and a mark that fades away (FBulletMarks, at most MarksPerVolley from
 * one shotgun blast). Some glancing hits off stone and metal ricochet: a white-hot spark and a short tracer off at the
 * bounce (FWeaponFX::SpawnRicochet), and the whine. Creatures splash what they spill (CreatureBlood: ichor, gel, embers)
 * and cry their own hit, training dummies spark and ring, as before (BulletSubsystemImpacts.cpp).
 *
 * The tracer has a hot head, and the first pellets of a shot leave a faint smoke thread behind them for a moment. The
 * guns' spent brass lives here too (FWeaponBrass: AWeaponBase ejects it), ticked with the rest while any lies about.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBulletSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void Fire(const FBulletShot& Shot);

	/**
	 * Fires Shot as soon as it's safe: now, or (asked while the bullets are being moved: a hit's listener firing a
	 * ricochet's bounce) the moment their move is done, the same frame.
	 */
	void FireSoon(const FBulletShot& Shot);

	int32 NumBulletsInFlight() const;

	/** The effects drawn for bullets and their impacts. */
	FWeaponFX& GetEffects() { return Effects; }

	/** The marks bullets leave on the world. */
	FBulletMarks& GetMarks() { return Marks; }

	/** The guns' spent brass lying about. */
	FWeaponBrass& GetBrass() { return Brass; }

	/** A gun throws out a spent case (AWeaponBase: its port, or a revolver's empties on a reload). */
	void EjectCasing(const FCasingLaunch& Launch);

	/** How a hit on this actor looks: creatures splash, anything else with health sparks, the world sparks and dusts. */
	static EImpactSurface SurfaceOf(const AActor* Actor);

	/**
	 * The sound of a bullet (or a melee blow) meeting what it hit (LooterSoundCues): the world's by its surface
	 * (ImpactSurface: its own cue once it has sounds, else the old Impact.World, .Wood, .Metal or .Water); a training
	 * dummy's metal ring. None for a creature, whose own hit cry plays.
	 */
	static FName ImpactCueOf(const FHitResult& Hit, EImpactSurface Surface);

	/** Where the tracer of a bullet that has flown Distance is drawn (it starts at the visible muzzle, then joins the aim line). */
	static FVector TracerPoint(const FBulletShot& Shot, float Distance);

	/** Over how many cm a tracer closes from the muzzle onto the aim line. */
	static constexpr float ConvergeDistance = 1000.f;

	/**
	 * A component tagged so shows and sounds its own hits (the Gravemother's headstone sparks off granite with its own crack;
	 * her clod and the mounds burst as theirs): a bullet only damages it, with no puff, sound or mark of its own over it.
	 */
	static constexpr const TCHAR* OwnLookTag = TEXT("Impact.Own");

	/** Any bullet hit something (its damage has been dealt). */
	FOnBulletHit OnBulletHit;

	/**
	 * The ember powers' say over the shots (Embers/Powers: Double or Nothing, Lucky Streak, Long Shot): every shot passes
	 * OnBulletFiring as it's fired (a listener changes the shot it's handed: its damage, its way, its tracer, bSureCritical,
	 * bBlank) and OnBulletLanding as it meets something, before its damage is rolled (a listener may make it critical or
	 * scale it). Each listener checks the shot is its player's (Shooter). Nothing listens most of the game.
	 */
	FOnBulletFiring OnBulletFiring;
	FOnBulletLanding OnBulletLanding;

	/**
	 * The guns' parts' say after a hit is dealt (UPartEffectSubsystem: elements, ricochets, splashes, rounds flying on):
	 * every hit passes it, a blank's too (with 0 damage). A listener may let the bullet fly on through (bPassThrough);
	 * one that fires more bullets fires them with FireSoon.
	 */
	FOnBulletStruck OnBulletStruck;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBulletSubsystem, STATGROUP_Tickables); }
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FBullet
	{
		FBulletShot Shot;
		FVector Position = FVector::ZeroVector;
		float Traveled = 0.f;
		/** The shot it was fired with: a shotgun's pellets from one pull share it (for the cap on marks). */
		int32 Volley = 0;
		/** How big its marks are (its gun's: FBulletMarks::GunScale), and its puff. */
		float MarkScale = 1.f;
		/** Its gun's kind and the rounds left in its magazine as it was fired: how its hit lands (FWeaponFX::FLandingScope; GF-IMPACT). */
		EWeaponKind Kind = EWeaponKind::None;
		int32 RoundsLeft = 0;
		/** Hit something or flew its full range: drawn one last time, then removed. */
		bool bSpent = false;
		bool bDrawn = false;
		/** One of its volley's first pellets: it leaves a smoke thread, laid as far as ThreadLaid (cm along its flight). */
		bool bThread = false;
		float ThreadLaid = 0.f;
		/** What it has flown on through (Piercing): never hit again. */
		TArray<TWeakObjectPtr<AActor>, TInlineAllocator<2>> Passed;
	};

	/** How many marks one volley has left so far. */
	struct FVolleyMarks
	{
		int32 Volley = 0;
		int32 Count = 0;
	};

	void Advance(FBullet& Bullet, float DeltaSeconds);
	/** Deals the hit; returns what the hit's listeners said (it flies on, and how strongly). */
	FBulletStrike ResolveHit(const FBullet& Bullet, const FHitResult& Hit);
	/** Fires what FireSoon held back while the bullets were being moved. */
	void FireHeld();
	// BulletSubsystemImpacts.cpp: how a hit looks and sounds.
	/** A glancing hit off stone, gravel or metal that goes off (about half of those under RicochetSquare). */
	static bool RollRicochet(const FHitResult& Hit, EImpactMaterial Material, const FVector& ShotDirection);
	void ShowHit(const FBullet& Bullet, const FHitResult& Hit, EImpactSurface Surface, EImpactMaterial Material, bool bCritical, bool bRicochet);
	/** The ground the chunks of a world hit land on: the surface itself if it's floor, else what's below the hit. */
	bool FindDebrisFloor(const FBullet& Bullet, const FHitResult& Hit, FPlane& OutFloor);
	void PlayImpactSound(const FBullet& Bullet, const FHitResult& Hit, EImpactSurface Surface, EImpactMaterial Material, bool bRicochet);
	void LeaveMark(const FBullet& Bullet, const FHitResult& Hit, EImpactMaterial Material);
	/** Lays the smoke thread behind a bullet as far as it has flown (its first meters only). */
	void LayThread(FBullet& Bullet);
	void DrawTracer(const FBullet& Bullet, const FVector& Camera);
	FVector GetCameraLocation() const;

	TArray<FBullet> Bullets;
	/** Moving bullets now (Advance, however deep): FireSoon holds its shots back until it's done (the array may grow). */
	int32 AdvanceDepth = 0;
	TArray<FBulletShot> HeldShots;
	FWeaponFX Effects;
	FBulletMarks Marks;
	FWeaponBrass Brass;

	/** The last impact sound, so a shotgun's pellets landing together on one surface make one crack rather than nine. */
	double LastImpactSoundTime = -1000.0;
	FVector LastImpactSoundAt = FVector::ZeroVector;
	FName LastImpactCue;
	/** The last ricochet's whine (never two at once). */
	double LastRicochetTime = -1000.0;
	/** Ricochets shown from the latest volley (a blast's pellets show two at most). */
	int32 RicochetLookVolley = 0;
	int32 RicochetLooks = 0;
	/** The last look down for a wall hit's floor: a blast's pellets on one wall share it. */
	int32 FloorVolley = 0;
	FVector FloorAt = FVector::ZeroVector;
	FPlane LastFloor = FPlane(0.0, 0.0, 1.0, 0.0);
	bool bLastFloor = false;

	/** Shots fired in one frame by one gun are one volley (a shotgun's pellets). */
	uint64 LastFireFrame = TNumericLimits<uint64>::Max();
	TWeakObjectPtr<AWeaponBase> LastFireWeapon;
	TWeakObjectPtr<AActor> LastFireShooter;
	int32 VolleyCount = 0;
	/** Pellets fired so far in the latest volley (its first ones leave smoke threads). */
	int32 VolleyPellets = 0;
	TArray<FVolleyMarks, TInlineAllocator<4>> VolleyMarks;
};
