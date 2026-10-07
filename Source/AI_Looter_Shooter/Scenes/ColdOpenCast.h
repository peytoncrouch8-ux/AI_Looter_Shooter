#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ColdOpenCast.generated.h"

class AColdOpenSet;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMeshComponent;
class UPointLightComponent;
class UPoseableMeshComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * Everything the cold open shows that the level doesn't have, staged cheaply on purpose (Docs/Story.md: Cold open): the
 * gang's skiff in its dark paint with the gang on its deck, the gang again on the lookout's deck, Saint Ada's ember in the
 * Deacon's gloved hand, Abel running up the bluff path with his lantern, the muzzle flashes, and Mister Sexton on the far
 * rail. The people are the UE mannequin posed by code and drawn flat black, with no costumes; Sexton is his seated model
 * (SM_MisterSexton, on the lookout's Sit socket, with his ledger on its Ledger socket) drawn black, or a stand-in of plain
 * shapes in a checkout that hasn't imported him yet.
 *
 * ColdOpen moves it beat by beat. Everything is built hidden as the scene starts and goes with the scene.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AColdOpenCast : public AActor
{
	GENERATED_BODY()

public:
	AColdOpenCast();

	/** The cast for Set, in Set's world, everything built and hidden; null when it couldn't be spawned. */
	static AColdOpenCast* Spawn(const AColdOpenSet& Set);

	virtual void Tick(float DeltaSeconds) override;

	// --- Seven days ago: the gang's skiff ---

	/** The gang's skiff and its crew shown (and the Point's people hidden), or hidden. */
	void ShowSkiff(bool bShow);

	/** The skiff's keel at Pose (the course's). */
	void PlaceSkiff(const FTransform& Pose);

	/** Where Ellis's eyes are on the skiff's deck now (world): over its Deck socket, where the player stood on the packet skiff. */
	FVector GetDeckEye() const;

	/** The skiff's crew member's head (world), 0 the Deacon at Ellis's shoulder. */
	FVector GetCrewHead(int32 Index) const;

	// --- Dusk on Ransom's Point ---

	/** The gang on the lookout's deck shown, the Deacon holding the ember (and the skiff hidden), or hidden. */
	void ShowPoint(bool bShow);

	/** Abel along his run up the bluff path: Alpha 0 where Ellis first sees him, 1 where the shot finds him. */
	void RunAbel(float Alpha);

	/** Abel shot: he pitches forward onto the path over Alpha (0-1). */
	void DropAbel(float Alpha);

	/** His lantern rolling off the path into the dark over Alpha (0-1), and going out. */
	void RollLantern(float Alpha);

	/** A gang member's gun arm raised at Target (world). */
	void Aim(int32 GangIndex, const FVector& Target);

	/** A shot from a gang member's gun hand at Target: the flash, its light on the deck, the shot's sound when there is one. */
	void Fire(int32 GangIndex, const FVector& Target);

	/** Mister Sexton on the far rail, shown or not. */
	void ShowSexton(bool bShow);

	/** Where a gang member's head is (world): 0 the Deacon, 1 Lucky Ned. */
	FVector GetHead(int32 GangIndex) const;

	FVector GetAbelChest() const;
	FVector GetSextonHead() const;

	/** How many of the gang stand on the deck, and on the skiff. */
	int32 GetGangCount() const { return Gang.Num(); }
	int32 GetCrewCount() const { return SkiffCrew.Num(); }

	/** Sexton is his own model (SM_MisterSexton) rather than the stand-in. */
	bool HasSextonModel() const { return SextonModel != nullptr; }

	bool IsSkiffShown() const { return bSkiffShown; }
	bool IsPointShown() const { return bPointShown; }
	bool IsSextonShown() const { return bSextonShown; }

private:
	void Build(const AColdOpenSet& Set);
	void BuildSkiff(const AColdOpenSet& Set);
	void BuildPoint(const AColdOpenSet& Set);

	/** The ember and the lantern facing the camera, and their lights where they are. */
	void UpdateGlows();

	// --- The figures and Sexton (ColdOpenCastFigures.cpp) ---

	/** The silhouettes' black: an unlit instance of the set's master, made once. Null without the master. */
	UMaterialInterface* GetSilhouette();

	/** Every slot of Mesh drawn in the silhouettes' black. */
	void PaintBlack(UMeshComponent& Mesh);

	/** A mannequin standing on Feet (relative to Parent, facing its +X), drawn black, posed at ease. */
	UPoseableMeshComponent* MakeFigure(USceneComponent* Parent, const FTransform& Feet);

	/** Mister Sexton on the lookout's Sit socket (or the set's seat): his model and ledger drawn black, or the stand-in. */
	void BuildSexton(const AColdOpenSet& Set);
	void BuildSextonStandIn(USceneComponent* Seat);

	/** Arms hanging at ease; a gun arm raised at a target (world); a left hand held at the chest (the ember). */
	static void PoseAtEase(UPoseableMeshComponent& Figure);
	static void PoseAim(UPoseableMeshComponent& Figure, const FVector& Target);
	static void PoseHoldingEmber(UPoseableMeshComponent& Figure);

	/** A bone's place in the world (a hand, a head) on a posed or animated figure, or a spot over its feet without the bone. */
	static FVector BoneInWorld(USceneComponent* Figure, FName Bone);

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Skiff;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPoseableMeshComponent>> SkiffCrew;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPoseableMeshComponent>> Gang;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Abel;

	/** Sexton's seat (the Sit socket's place): his model or the stand-in's parts hang from it. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SextonSeat;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SextonModel;

	/** The ember (instance 0) and Abel's lantern (instance 1): round glows that face the camera. */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Glows;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> EmberLight;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> LanternLight;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> FlashLight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SilhouetteBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Silhouette;

	TWeakObjectPtr<const AColdOpenSet> SetActor;
	FLinearColor SilhouetteColor = FLinearColor::Black;

	/** The skiff's Deck socket in its own frame, and Ellis's eyes over it (cm). */
	FVector DeckLocal = FVector(120.0, 0.0, 43.0);
	float EyeHeight = 162.f;

	/** Abel's run (feet, world, on the ground), his facing, and where his lantern left his hand. */
	FVector AbelFrom = FVector::ZeroVector;
	FVector AbelTo = FVector::ZeroVector;
	float AbelYaw = 0.f;
	FVector LanternFrom = FVector::ZeroVector;
	FVector LanternAt = FVector::ZeroVector;
	float LanternGlow = 0.f;
	bool bLanternLoose = false;

	float FlashLeft = 0.f;
	bool bSkiffShown = false;
	bool bPointShown = false;
	bool bSextonShown = false;
	bool bAbelShown = false;
};
