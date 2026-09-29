#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/WeaponFX.h"
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
 * (the same damage rules as ever: the per-hit roll and the x1.5 critical spots). Its tracer is drawn from the muzzle the
 * player sees, closing onto the aim line over the first few meters, so shots still land exactly on the crosshair.
 * Impacts throw sparks and dust off the world, sparks off training dummies, and ichor out of creatures (FWeaponFX).
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

	/** How a hit on this actor looks: creatures splash, anything else with health sparks, the world sparks and dusts. */
	static EImpactSurface SurfaceOf(const AActor* Actor);

	/** Where the tracer of a bullet that has flown Distance is drawn (it starts at the visible muzzle, then joins the aim line). */
	static FVector TracerPoint(const FBulletShot& Shot, float Distance);

	/** Over how many cm a tracer closes from the muzzle onto the aim line. */
	static constexpr float ConvergeDistance = 1000.f;

	/** Any bullet hit something (its damage has been dealt). */
	FOnBulletHit OnBulletHit;

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
		/** Hit something or flew its full range: drawn one last time, then removed. */
		bool bSpent = false;
		bool bDrawn = false;
	};

	void Advance(FBullet& Bullet, float DeltaSeconds);
	void ResolveHit(const FBullet& Bullet, const FHitResult& Hit);
	void DrawTracer(const FBullet& Bullet, const FVector& Camera);
	FVector GetCameraLocation() const;

	TArray<FBullet> Bullets;
	FWeaponFX Effects;
};
