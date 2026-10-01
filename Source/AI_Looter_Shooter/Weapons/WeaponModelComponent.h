#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponModelComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * A rolled gun's model: its definition's parts (static meshes made in Blender) as the roll's seed picks them, in the
 * gun's paint and its rarity's glow. This component is the gun's own space (origin at the back of the receiver, +X
 * toward the muzzle): the parts hang in it, or from sockets on earlier parts. It knows where the muzzle and the hands
 * go, and moves the part a reload works on.
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

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

private:
	/** A socket of any part, in the gun's space; false if no part has it. */
	bool FindSocket(FName Socket, FVector& OutLocation) const;
	void FindAimPoint();

	/** The slot (or the socket it hangs from) whose part is the sight. */
	const FName SightSocket = TEXT("Sight");
	/** On a sight: the point the eye lines up with when aiming (the dot, an optic's center, the irons' notch). */
	const FName AimSocket = TEXT("Aim");

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ReloadPartMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SightPart;

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
