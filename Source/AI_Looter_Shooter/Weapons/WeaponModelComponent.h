#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponModelComponent.generated.h"

class UMaterialInstanceDynamic;
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

	/** Key points in the gun's space. */
	FVector GetMuzzle() const { return Muzzle; }
	FVector GetGrip() const { return Grip; }
	FVector GetForegrip() const { return Foregrip; }
	/** Where the eye lines up when aiming down the sights: just over the sight (the gun's top without one), along its middle. */
	FVector GetAimPoint() const { return AimPoint; }

	/** The middle of the gun's parts, in its space. */
	FVector GetCenter() const;

	EWeaponReloadPart GetReloadPart() const { return ReloadPartMesh ? ReloadPart : EWeaponReloadPart::None; }

	/** Moves the reload's part this far out of place (cm, along its way out) and shows or hides it. */
	void SetReloadTravel(float Travel, bool bShow);

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

	/** The slot (or the socket it hangs from) whose part is the sight. */
	const FName SightSocket = TEXT("Sight");
	/** On a sight: the point the eye lines up with when aiming (the dot, an optic's center, the irons' notch). */
	const FName AimSocket = TEXT("Aim");
	/** The slots whose parts can take the tally: the stock's, else the body's. */
	const FName StockSlot = TEXT("Stock");
	const FName BodySlot = TEXT("Body");

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

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
	FVector Muzzle = FVector::ZeroVector;
	FVector Grip = FVector::ZeroVector;
	FVector Foregrip = FVector::ZeroVector;
	FVector AimPoint = FVector::ZeroVector;
	EFirstPersonPrimitiveType FirstPersonType = EFirstPersonPrimitiveType::None;
};
