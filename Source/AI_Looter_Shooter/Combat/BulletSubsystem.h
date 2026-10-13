#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/BulletMarks.h"
#include "Weapons/ImpactSurface.h"
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
};

/**
 * Bullets in flight. Each fired bullet is a real projectile: it travels along the shooter's aim at its gun's bullet
 * speed, is traced segment by segment every frame, and deals its damage to whatever is in its path when it gets there
 * (the same damage rules as ever: the per-hit roll and the x1.5 critical spots, or a cursed iron's own critical
 * multiplier). Its tracer is drawn from the muzzle the player sees, closing onto the aim line over the first few meters,
 * so shots still land exactly on the crosshair.
 *
 * Where it lands, the world shows what it hit (ImpactSurface finds the surface: dirt, grass, gravel, stone, wood, metal,
 * glass, water...): its own puff (FWeaponFX::SpawnSurfaceImpact), its own sound (ImpactCueOf, with a ricochet's whine on
 * some glancing hits off stone and metal) and a mark that fades away (FBulletMarks, at most MarksPerVolley from one
 * shotgun blast). Creatures splash and cry their own hit, training dummies spark and ring, as before.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBulletSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void Fire(const FBulletShot& Shot);

	int32 NumBulletsInFlight() const;

	/** The effects drawn for bullets and their impacts. */
	FWeaponFX& GetEffects() { return Effects; }

	/** The marks bullets leave on the world. */
	FBulletMarks& GetMarks() { return Marks; }

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

	/** Any bullet hit something (its damage has been dealt). */
	FOnBulletHit OnBulletHit;

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
		/** Hit something or flew its full range: drawn one last time, then removed. */
		bool bSpent = false;
		bool bDrawn = false;
	};

	/** How many marks one volley has left so far. */
	struct FVolleyMarks
	{
		int32 Volley = 0;
		int32 Count = 0;
	};

	void Advance(FBullet& Bullet, float DeltaSeconds);
	void ResolveHit(const FBullet& Bullet, const FHitResult& Hit);
	void PlayImpactSound(const FHitResult& Hit, EImpactSurface Surface, EImpactMaterial Material, const FVector& ShotDirection);
	void LeaveMark(const FBullet& Bullet, const FHitResult& Hit, EImpactMaterial Material);
	void DrawTracer(const FBullet& Bullet, const FVector& Camera);
	FVector GetCameraLocation() const;

	TArray<FBullet> Bullets;
	FWeaponFX Effects;
	FBulletMarks Marks;

	/** The last impact sound, so a shotgun's pellets landing together on one surface make one crack rather than nine. */
	double LastImpactSoundTime = -1000.0;
	FVector LastImpactSoundAt = FVector::ZeroVector;
	FName LastImpactCue;
	/** The last ricochet's whine (never two at once). */
	double LastRicochetTime = -1000.0;

	/** Shots fired in one frame by one gun are one volley (a shotgun's pellets). */
	uint64 LastFireFrame = TNumericLimits<uint64>::Max();
	TWeakObjectPtr<AWeaponBase> LastFireWeapon;
	TWeakObjectPtr<AActor> LastFireShooter;
	int32 VolleyCount = 0;
	TArray<FVolleyMarks, TInlineAllocator<4>> VolleyMarks;
};
