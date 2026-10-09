#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "World/FaunaActor.h"
#include "FaunaDustDevils.generated.h"

class UAudioComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Now and then a dust devil on Ransom's Rest's open dusty ground (World/FaunaActor.h): a short-lived spinning column of
 * dust (SM_DustDevil, translucent cards on the smoke master with no glow of its own) that rises out of nothing, wanders
 * with the wind over the ground (a trace down keeps its foot on it), and thins away after a few seconds, with its whirl
 * playing while it lives. One at a time, somewhere 20 to 60 m from the player at one of its spots (roads, yards and the
 * fields, from Tools/Unreal/build_area_fauna.py). It touches nothing.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFaunaDustDevils : public AFaunaActor
{
	GENERATED_BODY()

public:
	AFaunaDustDevils();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override { return bActive; }

	/** Raises one at a spot now (tests, Looter.Fauna.DustDevil). */
	bool Raise(int32 SpotIndex);

	bool IsActive() const { return bActive; }
	FVector GetDevilLocation() const { return Position; }
	/** How far through its life it is (0 to 1) and how strong it is now (0 to 1). */
	float GetLifeAlpha() const { return Life > 0.f ? Age / Life : 0.f; }
	float GetStrength() const;

	/** Builds it now (as play begins it), for the tests' worlds. */
	void SetupDevils();

	/** Open dusty ground it may rise on, each wandering within its radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils")
	TArray<FFaunaZone> Spots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils")
	TObjectPtr<UStaticMesh> Mesh;

	/** Seconds between one ending and the next rising (Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "1", Units = "s"))
	float SpawnSecondsMin = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "1", Units = "s"))
	float SpawnSecondsMax = 75.f;

	/** It rises between these distances from the nearest player (cm): far enough to watch, near enough to see. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "0", Units = "cm"))
	float SpawnRangeMin = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "0", Units = "cm"))
	float SpawnRangeMax = 6000.f;

	/** How long one lives (s, Min to Max), rising over GrowSeconds and thinning away over FadeSeconds of it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "2", Units = "s"))
	float LifeSecondsMin = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "2", Units = "s"))
	float LifeSecondsMax = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "0.1", Units = "s"))
	float GrowSeconds = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "0.1", Units = "s"))
	float FadeSeconds = 2.5f;

	/** How fast it spins (degrees a second) and wanders (cm/s); the wind's way (degrees, the smoke's lean). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (Units = "Degrees"))
	float SpinDegreesPerSecond = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "0", Units = "cm/s"))
	float WanderSpeed = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (Units = "Degrees"))
	float WindYaw = 19.f;

	/** Its height against the model's (Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "0.1"))
	float ScaleMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils", meta = (ClampMin = "0.1"))
	float ScaleMax = 1.3f;

	/** The material's parameter that thins it (M_Smoke's Opacity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils")
	FName OpacityParameter = FName(TEXT("Opacity"));

	/** Its whirl, looping while it lives (FaunaCues.h). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dust Devils|Sound")
	FName WhirlCue;

protected:
	virtual void BeginPlay() override;
	virtual void OnFaunaShownChanged(bool bShown) override;

private:
	void TryRaise(const FFaunaContext& Context);
	void End();
	void PushTransform();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Dust;

	TWeakObjectPtr<UAudioComponent> Whirl;
	FCollisionQueryParams GroundParams;
	FRandomStream Random;

	bool bReady = false;
	bool bActive = false;
	int32 Spot = INDEX_NONE;
	FVector Position = FVector::ZeroVector;
	FVector Goal = FVector::ZeroVector;
	float Age = 0.f;
	float Life = 0.f;
	float Spin = 0.f;
	float Scale = 1.f;
	float BaseOpacity = 0.28f;
	float SpawnTimer = 0.f;
	float GoalTimer = 0.f;
	FVector Center = FVector::ZeroVector;
	float Reach = 0.f;
};
