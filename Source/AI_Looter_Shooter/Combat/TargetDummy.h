#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/CriticalSpotTarget.h"
#include "TargetDummy.generated.h"

class AController;
class UAnimationAsset;
class UHealthComponent;
class ULootDropComponent;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

/**
 * Practice target. Uses a skeletal mesh so headshots register on the real head bone, the same way
 * they will on enemies. Flashes when hit (brighter and golden for headshots). Heals back to full when
 * left alone; when "killed" it drops loot, vanishes briefly, and respawns.
 */
UCLASS(Blueprintable)
class AI_LOOTER_SHOOTER_API ATargetDummy : public AActor, public ICriticalSpotTarget
{
	GENERATED_BODY()

public:
	ATargetDummy();

	// ICriticalSpotTarget: the head bone is the critical spot.
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	/** Bones whose name contains this (case-insensitive) are critical spots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dummy")
	FString CriticalBoneKeyword = TEXT("head");

	/** Looping pose/animation to play so the dummy isn't stuck in T-pose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dummy")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	/** Seconds without taking damage before health snaps back to full. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dummy", meta = (ClampMin = "0"))
	float HealDelay = 4.f;

	/** Seconds the dummy stays gone after being destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dummy", meta = (ClampMin = "0"))
	float RespawnDelay = 3.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULootDropComponent> Loot;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	void HealToFull();
	void Respawn();
	void SetPresent(bool bPresent);

	/** A quick glow over the whole body (an overlay material), fading out. */
	void FlashHit(bool bCritical);
	void UpdateHitFlash();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HitFlashMaterial;

	FTimerHandle HealTimer;
	FTimerHandle RespawnTimer;
	FTimerHandle HitFlashTimer;
	double HitFlashStart = 0.0;
	float HitFlashStrength = 0.f;
};
