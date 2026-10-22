#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponModelComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A rolled gun's model: its definition's parts (static meshes made in Blender) as the roll's seed picks them, in the
 * gun's paint and its rarity's glow. This component is the gun's own space (origin at the back of the receiver, +X
 * toward the muzzle): the parts hang in it, or from sockets on earlier parts. It knows where the muzzle and the hands
 * go, and moves the part a reload works on.
 *
 * Its notches show on it (WeaponModelNotches.cpp): the gun master (M_Gun) cuts the tally into one part from that part's
 * custom primitive data, and lights a soul-forged gun's soul-light the same way, so no gun needs meshes or materials of
 * its own.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UWeaponModelComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Builds the gun, replacing the one it showed. False (and empty) when its definition has no parts. */
	bool Assemble(const FWeaponInstanceData& Instance);

	/** Removes the parts. */
	void Clear();

	bool IsAssembled() const { return !Parts.IsEmpty(); }

	/**
	 * How much bigger than modeled a gun kind is drawn (the user's call, 2026-10-14: the rifle and the shotgun 10%, the
	 * six-gun 20%), everywhere its model shows. The parts hang at this scale inside the component, so every point it gives
	 * in its space (grip, foregrip, aim, muzzle, ejection port) and a reload part's travel come out at it too; whoever holds
	 * the component never scales it.
	 */
	static float ScaleFor(EWeaponKind Kind);

	/** The scale this gun was built at (ScaleFor its kind; 1 before it's built). */
	float GetModelScale() const { return ModelScale; }

	/** Key points in the gun's space. */
	FVector GetMuzzle() const { return Muzzle; }
	FVector GetGrip() const { return Grip; }
	FVector GetForegrip() const { return Foregrip; }
	/** Where the eye lines up when aiming down the sights: just over the sight (the gun's top without one), along its middle. */
	FVector GetAimPoint() const { return AimPoint; }

	/** The middle of the gun's parts, in its space. */
	FVector GetCenter() const;

	/** The ejection port its spent cases leave from, in its space, when a part marks it (SOCKET_Eject); false if none does. */
	bool GetEject(FVector& OutLocation) const
	{
		OutLocation = Eject;
		return bHasEject;
	}

	/**
	 * A revolver's chambers where they open at the back of its cylinder, in the world as the cylinder is now (swung out,
	 * turned), and the way out of them (OutBack: the cylinder's back): where its empties tumble from. False without one.
	 */
	bool GetChamberRears(TArray<FVector>& OutLocations, FVector& OutBack) const;

	EWeaponReloadPart GetReloadPart() const { return ReloadPartMesh ? ReloadPart : EWeaponReloadPart::None; }

	/**
	 * Moves the reload's part this far out of place (cm, along its way out) and shows or hides it. A revolver's cylinder
	 * swings instead: Travel is how far it has swung out on its crane (degrees, to the gun's left).
	 */
	void SetReloadTravel(float Travel, bool bShow);

	/**
	 * A revolver's cylinder turns on by Chambers (the next round under the hammer), eased over CylinderTurnSeconds after a
	 * short wait (the recoil first); nothing for a gun without one. UpdateCylinder plays it out.
	 */
	void TurnCylinder(int32 Chambers);

	/** Advances the cylinder's turn. True while it's still turning (the gun keeps ticking for it). */
	bool UpdateCylinder(float DeltaSeconds);

	bool IsCylinderTurning() const { return CylinderTurnAge < CylinderTurnDelay + CylinderTurnSeconds; }

	/** The chambers in this gun's cylinder (its part's capacity), 0 without one. */
	int32 GetCylinderChambers() const { return ReloadPart == EWeaponReloadPart::Cylinder && ReloadPartMesh ? CylinderChambers : 0; }

	/** How far the cylinder has turned since the gun was built, in chambers (fractions mid-turn). */
	float GetCylinderTurn() const { return CylinderTurnShown; }

	/** The cylinder's turn after a shot: the wait (the hammer's fall and the kick) and the turn itself (seconds). */
	static constexpr float CylinderTurnDelay = 0.04f;
	static constexpr float CylinderTurnSeconds = 0.12f;

	// --- The frontier-tech guns' moving and glowing parts (Phase 12.3; WeaponModelKinds.cpp) ---

	/** The sniper's bolt (its Bolt slot's part): Open 0 home, 1 turned up and drawn fully back (LooterReload::BoltPose). */
	void SetBoltOpen(float Open);
	bool HasBolt() const { return BoltMesh != nullptr; }

	/**
	 * The launcher's rocket: the next one (the definition's ProjectileMesh) Travel cm in front of its seat in the mouth,
	 * shown while it's in the hand (bIncomingShown); the loaded rocket's nose (the magazine slot's part) shown while one is
	 * in (bLoaded).
	 */
	void SetRocketLoad(float Travel, bool bIncomingShown, bool bLoaded);

	/**
	 * The glows the game drives (material slots GunAmmoGlow, the SMG helix's rounds left; GunHeatGlow, the LMG barrel's
	 * heat): lit from the part's feed or breech end (its UV0.x 0) up to Fill (0-1), at Strength. On the gauge master
	 * (M_GunGauge, Tools/Unreal/build_gun_gauge_material.py) the lit length follows Fill; without it the whole slot
	 * glows at Strength times Fill's share, so it still dims and brightens.
	 */
	void SetAmmoGauge(float Fill, float Strength);
	void SetHeatGauge(float Fill, float Strength);
	bool HasAmmoGauge() const { return AmmoGauge != nullptr; }
	bool HasHeatGauge() const { return HeatGauge != nullptr; }

	/** A launcher's exhaust mouth (SOCKET_Backblast) and the way it faces, in the gun's space; false without one. */
	bool GetBackblast(FVector& OutLocation, FVector& OutDirection) const
	{
		OutLocation = Backblast;
		OutDirection = BackblastWay;
		return bHasBackblast;
	}

	/** The material slots the game lights part of the way, and the gauge master that can (null until it's built). */
	static const FName AmmoGlowSlot;
	static const FName HeatGlowSlot;
	static UMaterialInterface* FindGaugeMaster();

	/** Draws the parts like first-person arms (own field of view, no clipping) or like the world. */
	void SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType Type);
	EFirstPersonPrimitiveType GetFirstPersonPrimitiveType() const { return FirstPersonType; }

	/** Every part's mesh, for rendering settings. */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetParts() const { return Parts; }

	/**
	 * Shows the gun's notches: one tally mark per WeaponNotches::KillsPerMark kills cut into its stock (into the body's
	 * butt on a gun whose stock is only a pad, the bullpup), and a faint soul-light in its rarity's color once it's
	 * soul-forged. Assemble shows them; call again when its kills change.
	 */
	void ShowNotches(const FWeaponInstanceData& Instance);

	/** The part the tally is cut into (null when there's none to cut). */
	UStaticMeshComponent* GetNotchPart() const { return NotchPart; }

	/** Whether a part has a tally row laid out for it (WeaponModelNotches.cpp); any other gets one fitted to its size. */
	static bool HasTallyRow(const UStaticMesh* Mesh);

	/**
	 * The gun master's custom primitive data (WeaponParts::WearDataIndex, 0, is the wear): the marks cut (0 to 25); the
	 * row they run along, from (start X, start Z) to (end X, end Z) in the part's own space (cm); how tall a cut is and how
	 * far from the part's middle (|Y|, cm) a surface must be to take one, so only the outside of its sides is cut; the
	 * soul-light's color (RGB) and strength.
	 */
	static constexpr int32 NotchMarksDataIndex = 1;
	static constexpr int32 NotchRowDataIndex = 2;
	static constexpr int32 NotchHeightDataIndex = 6;
	static constexpr int32 NotchDepthDataIndex = 7;
	static constexpr int32 SoulLightDataIndex = 8;

	/**
	 * How bright a soul-forged gun's soul-light is (M_Gun's rim glow, times its rarity's color). Faint, as the spec says:
	 * at 1.6 the whole gun in hand glowed blue-white (2026-10-08).
	 */
	static constexpr float SoulLightGlow = 0.3f;

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	/** A socket of any part, in the gun's space; false if no part has it. */
	bool FindSocket(FName Socket, FVector& OutLocation) const;
	void FindAimPoint();

	/** Picks the part the tally goes on and where on it: the stock's or the body's row (WeaponModelNotches.cpp). */
	void ChooseNotchPart(UStaticMeshComponent* Stock, UStaticMeshComponent* Body);

	/** Puts the cylinder where its swing and its turn have it: turned about its own axis, swung about its crane. */
	void PoseCylinder();

	/** A gauge slot's material for this gun (WeaponModelKinds.cpp): the gauge master's instance in Source's color, else Source's own. */
	UMaterialInstanceDynamic* MakeGauge(FName Slot, UMaterialInterface* Source);

	/** The launcher's next rocket for its reload, beside the loaded one (WeaponModelKinds.cpp; none without a projectile mesh). */
	void MakeRocketLoad(const UWeaponDefinition& Definition, UStaticMeshComponent& Loaded);

	/** The back-blast socket's place and facing, from whichever part has it (the launcher's body). */
	void FindBackblast();

	/** The slot (or the socket it hangs from) whose part is the sight. */
	const FName SightSocket = TEXT("Sight");
	/** On a sight: the point the eye lines up with when aiming (the dot, an optic's center, the irons' notch). */
	const FName AimSocket = TEXT("Aim");
	/** The slots whose parts can take the tally: the stock's (a revolver's grip, where gunfighters cut theirs), else the body's. */
	const FName StockSlot = TEXT("Stock");
	const FName GripSlot = TEXT("Grip");
	const FName BodySlot = TEXT("Body");
	/** On a revolver's cylinder: the crane's hinge it swings out on, in the cylinder's own space. */
	const FName CraneSocket = TEXT("Crane");
	/** The sniper's bolt slot. */
	const FName BoltSlot = TEXT("Bolt");

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	/** The sniper's bolt, and the launcher's next rocket (not one of Parts: it only shows through a reload). */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BoltMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> RocketLoadMesh;

	/** This gun's instances of its gauge slots (null for a gun without them). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AmmoGauge;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeatGauge;

	FVector Backblast = FVector::ZeroVector;
	FVector BackblastWay = -FVector::ForwardVector;
	bool bHasBackblast = false;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ReloadPartMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SightPart;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> NotchPart;

	/** The tally's row on NotchPart (start X, start Z, end X, end Z; cm), and its cuts' height and least depth. */
	FVector4f NotchRow = FVector4f(0.f, 0.f, 0.f, 0.f);
	FVector2f NotchCut = FVector2f(0.f, 0.f);

	/** This gun's colors of the parts' shared materials. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Paints;

	EWeaponReloadPart ReloadPart = EWeaponReloadPart::None;

	/** The scale the parts were hung at (ScaleFor the definition's kind). */
	float ModelScale = 1.f;

	/** A revolver's cylinder: its chambers, its crane's hinge (its own space), its swing (degrees) and its turn (chambers). */
	int32 CylinderChambers = 0;
	FVector CranePivot = FVector::ZeroVector;
	float CylinderSwing = 0.f;
	float CylinderTurnShown = 0.f;
	float CylinderTurnFrom = 0.f;
	float CylinderTurnTo = 0.f;
	/** Seconds since the last shot's turn began (past its wait and turn: still). */
	float CylinderTurnAge = 1000.f;

	FVector Muzzle = FVector::ZeroVector;
	FVector Grip = FVector::ZeroVector;
	FVector Foregrip = FVector::ZeroVector;
	FVector AimPoint = FVector::ZeroVector;
	FVector Eject = FVector::ZeroVector;
	bool bHasEject = false;
	EFirstPersonPrimitiveType FirstPersonType = EFirstPersonPrimitiveType::None;
};
