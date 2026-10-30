#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FoliageBlastSubsystem.generated.h"

class UMaterialParameterCollection;

/** A blast's push on the plants at an age: its front's radius (cm) and its strength (springs back past 0, then 0). */
struct FFoliageBlastState
{
	float Front = 0.f;
	float Strength = 0.f;
};

/**
 * A blast through the grass (2026-10-30, the playtest: "grass doesn't bend in the shriek"): a ring rolls out from a point,
 * throwing the blades and plants flat away from it at its front, and they spring back behind it with a small overshoot.
 * One blast at a time (the newest wins): it writes MPC_Wind's BlastCenter (xyz, w = the front's radius in cm) and
 * BlastStrength, which M_WorldFoliage's wind (Tools/Unreal/build_world_materials.py WIND_CODE) reads. It ticks only while
 * a blast is on. The Unpaid's shriek wave (UUnpaidPackSubsystem::StartWave) starts one; anything else may.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UFoliageBlastSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UFoliageBlastSubsystem* Get(const UObject* WorldContextObject);

	/** A blast from Center: its front rolls out to Radius (cm) over GrowSeconds at Strength (0 to 1), then the plants settle. */
	static void Blast(const UObject* WorldContextObject, const FVector& Center, float Radius, float GrowSeconds, float Strength);

	/** The blast at Age seconds (pure: the tests read it). */
	static FFoliageBlastState StateAt(float Age, float Radius, float GrowSeconds, float Strength);
	/** How long the plants take to settle once the front stops (s). */
	static constexpr float SettleSeconds = 1.2f;

	/** MPC_Wind's blast parameters (Tools/Unreal/wind_collection.py). */
	static const FName CenterParameter;
	static const FName StrengthParameter;

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	bool IsBlasting() const { return bActive; }

private:
	void Write(const FVector& Center, const FFoliageBlastState& State);

	FVector Center = FVector::ZeroVector;
	float Radius = 0.f;
	float GrowSeconds = 0.5f;
	float Strength = 0.f;
	float Age = 0.f;
	bool bActive = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> WindCollection;
	bool bCollectionLooked = false;
};
