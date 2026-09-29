#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "SpiderCreature.generated.h"

class UCapsuleComponent;
class UDynamicMeshComponent;
class UMaterialInterface;
class USceneComponent;
class USphereComponent;

/**
 * Human-sized brown hunting spider (wolf-spider look, not a black widow). 150 health; the head is the
 * critical spot (x1.5 per the game-wide rule), legs, thorax, abdomen and fangs take base damage.
 *
 * Everything is procedural: the body is generated with the stylized mesh kit, and the eight legs are
 * two-bone IK chains driven by a stepping gait (alternating tetrapod groups, feet planted on the ground,
 * steps triggered by distance from each foot's rest spot and led by velocity). Attacks rear up and lunge,
 * hits make it flinch, and death curls the legs in.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASpiderCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	ASpiderCreature();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Thorax height above the ground when standing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spider")
	float RideHeight = 62.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spider")
	FLinearColor BodyColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spider")
	FLinearColor MarkingColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spider")
	FLinearColor BellyColor;

protected:
	virtual void BeginPlay() override;
	virtual void OnAttackStarted() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;

private:
	struct FLeg
	{
		float Side = 1.f;          // +1 right, -1 left
		int32 Pair = 0;            // 0 = front ... 3 = back
		int32 Group = 0;           // gait group (alternating tetrapod)
		FVector Hip;               // body space
		FVector Rest;              // resting foot, body space (Z ignored; feet sit on the ground)
		float FemurLength = 90.f;
		float TibiaLength = 115.f;

		FVector Foot = FVector::ZeroVector;  // world
		FVector StepFrom = FVector::ZeroVector;
		FVector StepTo = FVector::ZeroVector;
		float StepAlpha = 1.f;
		bool bStepping = false;
		bool bNeedsReset = false;
		float LastStepTime = 0.f;
	};

	void BuildMeshes();
	void PlantLegs();
	void AnimateBody(float DeltaSeconds);
	void AnimateLegs(float DeltaSeconds);
	void PoseSegment(int32 Segment, const FVector& From, const FVector& To, const FVector& Pole, float Radius);
	FVector GroundUnder(const FVector& Point) const;

	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TObjectPtr<USceneComponent> BodyRoot;

	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TObjectPtr<UDynamicMeshComponent> ThoraxMesh;

	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TObjectPtr<UDynamicMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TObjectPtr<USceneComponent> AbdomenPivot;

	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TObjectPtr<UDynamicMeshComponent> AbdomenMesh;

	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TObjectPtr<UDynamicMeshComponent> FangLeft;

	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TObjectPtr<UDynamicMeshComponent> FangRight;

	UPROPERTY(VisibleAnywhere, Category = "Spider|Hit Zones")
	TObjectPtr<USphereComponent> HeadHit;

	UPROPERTY(VisibleAnywhere, Category = "Spider|Hit Zones")
	TObjectPtr<USphereComponent> ThoraxHit;

	UPROPERTY(VisibleAnywhere, Category = "Spider|Hit Zones")
	TObjectPtr<UCapsuleComponent> AbdomenHit;

	/** Left then right; each rides on its fang mesh, so it follows the fangs as they spread and snap shut. */
	UPROPERTY(VisibleAnywhere, Category = "Spider|Hit Zones")
	TArray<TObjectPtr<UCapsuleComponent>> FangHits;

	/** Two per leg: femur then tibia. */
	UPROPERTY(VisibleAnywhere, Category = "Spider")
	TArray<TObjectPtr<UDynamicMeshComponent>> LegMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Spider|Hit Zones")
	TArray<TObjectPtr<UCapsuleComponent>> LegHits;

	/** One shared material set for every body part. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> BodyMaterials;

	TArray<FLeg> Legs;
	float AnimTime = 0.f;
	float BodyZ = 0.f;
	float BodyPitch = 0.f;
	float BodyRoll = 0.f;
	bool bBodyInitialized = false;
	FVector HurtOffset = FVector::ZeroVector;
	float AbdomenKick = 0.f;
	float FangOpen = 0.f;
};
