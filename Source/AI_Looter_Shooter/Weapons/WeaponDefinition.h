#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponDefinition.generated.h"

class AWeaponBase;
class USkeletalMesh;
class UStaticMesh;
class USoundBase;
class UNiagaraSystem;

/**
 * Designer-authored archetype for a weapon (e.g. "Assault Rifle", "Pump Shotgun").
 * Create one per weapon type via Content Browser > Miscellaneous > Data Asset > WeaponDefinition.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UWeaponDefinition();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	/** Actor class to spawn. Leave as AWeaponBase unless the weapon needs custom Blueprint logic. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AWeaponBase> WeaponClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EWeaponFireMode FireMode = EWeaponFireMode::FullAuto;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "2", EditCondition = "FireMode == EWeaponFireMode::Burst"))
	int32 BurstCount = 3;

	/** Stats before variance, rarity and level are applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FWeaponStats BaseStats;

	/** Each stat is randomly scaled by up to +/- this fraction (0.1 = +/-10%). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0", ClampMax = "1"))
	float StatVariance = 0.1f;

	/**
	 * Damage growth a level above 1, compounding: x(1 + this) ^ (Level - 1) (0.09: x1.68 at 7; the user, 2026-10-29 night; was
	 * +8% of level 1's, linear). Keep it equal to UProgressionSettings' EnemyGrowthPerLevel (FLevelRules::Compounded).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float DamagePerLevel = 0.09f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	TMap<EWeaponRarity, FWeaponRarityInfo> RarityTable;

	/** Which ammo pool this weapon reloads from (shared by every weapon of the same class). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	EAmmoType AmmoType = EAmmoType::AssaultRifle;

	/** When given as a starting weapon, this many magazines of its ammo are added to the player's pool. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 StartingReserveMagazines = 4;

	/** Impulse applied to physics-simulating objects that are hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
	float HitImpulse = 20000.f;

	/**
	 * How fast the bullets fly (cm/s). Bullets are real: each one travels, and hits whatever is in its path when it gets
	 * there. 0 = instant (hitscan).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0"))
	float BulletSpeed = 30000.f;

	/**
	 * A launcher's rocket (SM_PowderkegRocket, from Art/Models/Weapons/Powderkeg.py): what flies when it fires (AWeaponRocket)
	 * and what its reload slides into the mouth. Only a Launcher kind fires rockets; without one it flies as a plain shape.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	/** Glow color of the bullets' tracers (linear). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing")
	FLinearColor TracerColor = FLinearColor(1.f, 0.62f, 0.25f);

	/** Width of a tracer streak (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0.1"))
	float TracerWidth = 3.f;

	/** Length of a tracer streak (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "10"))
	float TracerLength = 380.f;

	/** Size of the built-in muzzle flash (1 = rifle). Not used when MuzzleFlashFX is set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing", meta = (ClampMin = "0"))
	float MuzzleFlashScale = 1.f;

	/** How the gun and the shooter's aim kick with each shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Firing")
	FWeaponRecoilProfile Recoil;

	/** What kind of gun it is: its icons, its shot's sound and kick, its nicknames (WeaponSounds, ViewKicks, WeaponNotches). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	EWeaponKind Kind = EWeaponKind::None;

	/**
	 * How far ahead of a long gun's hold this one is held out (cm, along its barrel): a six-gun is pushed out toward
	 * arm's length, where a rifle is tucked in. The third-person arms reach out by it (the stance layer, LooterStancePose).
	 * 0 for long guns. (First person has its own hip hold: FirstPersonGrip.)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals", meta = (ClampMin = "0", ClampMax = "40"))
	float HoldReach = 0.f;

	/**
	 * A first-person hip hold of its own (a six-gun's): where its grip sits in the view (cm, the hold's space: X ahead,
	 * Y right, Z up), turned by FirstPersonRotation. Zero: the long guns' hold (the weapon manager's AttachOffset, lined up
	 * by the grip). Aiming still puts the sight's SOCKET_Aim on the line of sight, whatever this is.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FVector FirstPersonGrip = FVector::ZeroVector;

	/** The hip hold's turn (with FirstPersonGrip): a cant in toward the middle, the barrel toed in to meet the crosshair at range. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FRotator FirstPersonRotation = FRotator::ZeroRotator;

	/**
	 * The gun's parts, in order: each rolled gun picks its own by its seed (UWeaponModelComponent assembles them).
	 * Takes priority over the meshes below. Sockets the gun reads from its parts: Muzzle, Grip and Foregrip.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Parts")
	TArray<FWeaponPartSlot> Parts;

	/** Materials the parts share that each rolled gun colors its own way. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Parts")
	TArray<FWeaponPaint> Paints;

	/** The parts' material slot that glows in the gun's rarity color. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Parts")
	FName RarityGlowSlot = TEXT("GunAccentGlow");

	/**
	 * The part slot a reload moves: a magazine slides out along the part's -Z, a pump back along its -X, a cylinder swings
	 * out to the left on its crane (its SOCKET_Crane) and turns a chamber per shot. The SMG's helix, the LMG's box and the
	 * sniper's box slide out as a magazine does (the sniper's bolt is its Bolt slot's part); the launcher's is the loaded
	 * rocket on the Rocket socket (EWeaponReloadPart).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Parts")
	FName ReloadSlot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Parts")
	EWeaponReloadPart ReloadPart = EWeaponReloadPart::None;

	/** Without parts: either a skeletal or static mesh. Skeletal wins if both are set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	FName MuzzleSocket = TEXT("Muzzle");

	/** Optional Niagara muzzle flash. Empty = the built-in flash. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UNiagaraSystem> MuzzleFlashFX;

	/** Optional Niagara impact, spawned where bullets hit. Empty = the built-in sparks, dust and splashes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals")
	TObjectPtr<UNiagaraSystem> ImpactFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> DryFireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundBase> ReloadSound;

	const FWeaponRarityInfo& GetRarityInfo(EWeaponRarity Rarity) const;
};
