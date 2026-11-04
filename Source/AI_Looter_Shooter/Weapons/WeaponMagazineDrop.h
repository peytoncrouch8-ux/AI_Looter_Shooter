#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponMagazineDrop.generated.h"

class AActor;
class AWeaponBase;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UWeaponDefinition;
class UWeaponModelComponent;

/** One old magazine leaving a gun (AWeaponBase, where its reload throws it: WeaponBaseReload.cpp). */
struct FMagazineDropLaunch
{
	/** The part's own mesh, its materials and custom primitive data (wear) as the gun wears them. */
	UStaticMesh* Mesh = nullptr;
	TArray<UMaterialInterface*> Materials;
	TArray<float> CustomData;
	/** Where it leaves from, in the world, at the size it's drawn there (a first-person gun is drawn bigger and nearer). */
	FTransform Transform = FTransform::Identity;
	/** Its size in the world, which it takes over its first GrowSeconds (the first-person size is the view model's). */
	FVector WorldScale = FVector::OneVector;
	FVector Velocity = FVector::ZeroVector;
	/** Tumble: a world axis times its speed (radians a second). */
	FVector Spin = FVector::ZeroVector;
	/** What it lands on is traced on this channel (the gun's own, as its brass). */
	ECollisionChannel Channel = ECC_Visibility;
	/** Never landed on: whoever threw it, and the gun. */
	TWeakObjectPtr<const AActor> Holder;
	TWeakObjectPtr<const AActor> Gun;
};

/** The thrown magazine's plain rules (the tests' too; WeaponMagazineDropRules.cpp). */
namespace MagazineDrop
{
	/** At most this many lie about at once: past it the oldest goes. */
	inline constexpr int32 PoolSize = 8;
	inline constexpr float Gravity = 980.f;
	/** Bounces after the first landing (a box of rounds hardly bounces). */
	inline constexpr int32 MaxBounces = 1;
	/** How long it takes to sink away once its rest is up (s), and to grow from its first-person size to its own. */
	inline constexpr float SinkSeconds = 1.2f;
	inline constexpr float GrowSeconds = 0.25f;
	/** How far it may fall before it's given up (off a cliff), and the longest a leg of its arc may last. */
	inline constexpr float MaxFall = 800.f;
	inline constexpr float MaxLegSeconds = 2.5f;

	/** A reload of Part throws its old magazine: the rifle's magazine, the SMG's helix, the LMG's box, the sniper's box. */
	AI_LOOTER_SHOOTER_API bool Drops(EWeaponReloadPart Part);

	/** Where in a reload of Part (its choreography progress) the old one leaves the gun: clear of its well (-1: never). */
	AI_LOOTER_SHOOTER_API float DropAt(EWeaponReloadPart Part);

	/** Whether the gun's own part is hidden at Progress because the thrown one has it: from DropAt until the fresh one shows. */
	AI_LOOTER_SHOOTER_API bool Hides(EWeaponReloadPart Part, float Progress);

	/** Its arc: Seconds after leaving Start at Velocity. */
	AI_LOOTER_SHOOTER_API FVector Arc(const FVector& Start, const FVector& Velocity, float Seconds);

	/**
	 * Its bounce off a face of Normal it hit at Velocity: some of its speed along the face kept, a little of what went into
	 * it given back (a dull knock in soft ground, a clatter off stone or steel).
	 */
	AI_LOOTER_SHOOTER_API FVector Bounce(const FVector& Velocity, const FVector& Normal, bool bSoft);

	/** The slot a new one takes in a pool whose slots have been out Ages seconds (negative: free): a free one, else the oldest. */
	AI_LOOTER_SHOOTER_API int32 PickSlot(TConstArrayView<float> Ages);

	/** How far it has sunk away (0 lying, 1 gone) RestAge seconds after it came to rest, lying Rest seconds before it sinks. */
	AI_LOOTER_SHOOTER_API float SinkShare(float RestAge, float Rest);

	/** Lying on Normal: its flattest side down (Extent: its half size along its own axes), its turn kept about the normal. */
	AI_LOOTER_SHOOTER_API FQuat RestRotation(const FQuat& Now, const FVector& Normal, const FVector& Extent);

	/** How far its middle lies off the ground in RestRotation (cm): its half size along that flattest side. */
	AI_LOOTER_SHOOTER_API float RestLift(const FVector& Extent);

	/** The reload's part of a gun's model (the part filling its definition's ReloadSlot), null for a gun without one. */
	AI_LOOTER_SHOOTER_API UStaticMeshComponent* FindReloadPart(const UWeaponModelComponent& Model, const UWeaponDefinition& Definition);

	/**
	 * Gun's reloads throw the old magazine (Looter.Gunfeel.Reload.DropMag on, a magazine-like part on its model, a pool in
	 * its world). Only then is its own part hidden from DropAt until the fresh one shows (else it falls out of sight, as before).
	 */
	AI_LOOTER_SHOOTER_API bool Throws(const AWeaponBase& Gun, const UWeaponModelComponent& Model);

	/**
	 * The launch of Gun's old magazine as it clears the well now: Part as the model has it this frame, where it shows
	 * (bFirstPerson: the view model's field of view and size), moving as its slide had it (the reload's motion MotionSeconds
	 * long) plus the holder's run, flicked off the gun's left, tumbling end over end.
	 */
	AI_LOOTER_SHOOTER_API FMagazineDropLaunch LaunchFrom(const AWeaponBase& Gun, const UStaticMeshComponent& Part, bool bFirstPerson,
		float MotionSeconds);
}

/**
 * The old magazines a reload throws out (Phase 3 change 12; Looter.Gunfeel.Reload.DropMag): the gun's own part copied as
 * it leaves the well, falling on a simple arc with gravity and a tumble, landing (traced ahead in a few chords, never a
 * trace a frame, as FWeaponBrass's cases), a bounce with a clatter, then lying Looter.Gunfeel.Reload.DropMag.Rest seconds
 * before it sinks away. The revolver's empties already fall this way (FWeaponBrass, DumpEmpties).
 *
 * Drawn on instanced meshes, one per mesh and paint (a gun's magazines share one draw), at most PoolSize lying about: past
 * it the oldest goes. Redrawn only on frames something moved; ticks only while one is out.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UWeaponMagazineDrop : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UWeaponMagazineDrop* Get(const UObject* WorldContextObject);

	/** Throws one (traced to where it lands at once). Past the pool the oldest goes. */
	void Drop(const FMagazineDropLaunch& Launch);

	/** Moves every one on, lands and bounces them, ages the lying ones, and redraws what changed (the tick calls it; a test calls it). */
	void Advance(float DeltaSeconds);

	int32 Num() const;
	int32 NumFlying() const;
	/** Where each one's middle is now (tests). */
	TArray<FVector> GetLocations() const;
	/** The instanced meshes made so far (reused; never more than PoolSize). */
	int32 NumDrawers() const { return Drawers.Num(); }

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FThrown
	{
		/** Out (in use), how long since it was thrown, and which throw it was (the pool evicts the earliest throw: ages tie within a frame). */
		bool bOut = false;
		float Age = 0.f;
		uint32 Serial = 0;
		/** Which drawer draws it, its half size along its own axes (at its world size), and the mesh's middle off its origin. */
		int32 Drawer = INDEX_NONE;
		FVector Extent = FVector::OneVector;
		FVector MeshMiddle = FVector::ZeroVector;
		FVector DrawnScale = FVector::OneVector;
		FVector WorldScale = FVector::OneVector;
		/** Its arc now (its middle's): from LegStart at LegVelocity, LegTime seconds in, landing LegLanding in (-1: nothing under it). */
		FVector LegStart = FVector::ZeroVector;
		FVector LegVelocity = FVector::ZeroVector;
		float LegTime = 0.f;
		float LegLanding = -1.f;
		float LegHorizon = 1.f;
		FVector LandPoint = FVector::ZeroVector;
		FVector LandNormal = FVector::UpVector;
		EImpactMaterial LandSurface = EImpactMaterial::Unknown;
		FVector Location = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector Spin = FVector::ZeroVector;
		int32 Contacts = 0;
		bool bResting = false;
		float RestAge = 0.f;
		ECollisionChannel Channel = ECC_Visibility;
		TWeakObjectPtr<const AActor> Holder;
		TWeakObjectPtr<const AActor> Gun;
	};

	/** One instanced mesh: the mesh and paint it draws (the gun's own materials, as they were when it was made), reused once empty. */
	struct FDrawerKey
	{
		TWeakObjectPtr<UStaticMesh> Mesh;
		TArray<TWeakObjectPtr<UMaterialInterface>> Materials;
		TArray<float> CustomData;
	};

	/** The drawer for Launch's mesh and paint: one already drawing it, else an empty one taken over, else a new one. */
	int32 DrawerFor(const FMagazineDropLaunch& Launch);
	/** Starts a leg of its arc from From at Velocity, traced ahead to where it lands. */
	void StartLeg(FThrown& Thrown, const FVector& From, const FVector& Velocity);
	/** It reached its landing: a sound, a bounce or it lies still. False: it's gone (into water). */
	bool Land(FThrown& Thrown);
	void Redraw();
	/** The actor that holds the drawers (made once; again if the level took it). */
	AActor* GetHolder();

	UPROPERTY(Transient)
	TObjectPtr<AActor> Holder;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Drawers;

	/** One per drawer, in the same order. */
	TArray<FDrawerKey> DrawerKeys;

	/** PoolSize slots. */
	TArray<FThrown> Pool;
	/** Throws so far (each FThrown's Serial), so the slot given up is the earliest throw even among throws of one frame. */
	uint32 Throws = 0;
	bool bDirty = false;
	bool bDrawnAnything = false;
	FRandomStream Random{ 0x3a6d0b };
};
