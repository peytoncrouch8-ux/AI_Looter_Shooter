#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGGroundFitFilter.generated.h"

/**
 * Meadow filter (Tools/Unreal/build_meadow.py): keeps a ground cover point only where the patch spawned on it lies on
 * the ground. A patch is one rigid mesh laid on the slope under its middle, so this samples the ground under its edge
 * (Radius, in the point's own tilted and scaled frame). A patch floating a little over a bump is pressed down until
 * no part of its edge floats; one that would need more than MaxSink, or that has no ground under part of its edge (a
 * cliff top's edge, the island's rim), is dropped. It only runs while the meadow generates in the editor; it lives in
 * the game module because the meadow's graph ships with the level.
 */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class AI_LOOTER_SHOOTER_API UPCGGroundFitFilterSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return TEXT("GroundFitFilter"); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("LooterPCG", "GroundFitFilterTitle", "Ground Fit Filter"); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Filter; }
#endif

	/** The patch's radius at scale 1 (cm). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "1", PCG_Overridable))
	float Radius = 170.f;

	/** How far a patch may be pressed into the ground to close a gap under its edge (cm). Needing more drops it. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (ClampMin = "0", PCG_Overridable))
	float MaxSink = 30.f;

	/** Only actors with this tag count as ground. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FName GroundTag = TEXT("Ground");

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return Super::DefaultPointInputPinProperties(); }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override { return Super::DefaultPointOutputPinProperties(); }
	virtual FPCGElementPtr CreateElement() const override;
};

namespace LooterGroundFit
{
	/**
	 * How far a patch has to go down so no part of its edge floats (cm; 0 when it already lies on the ground), or unset
	 * when it can't lie there: part of its edge has no ground under it, or it would have to go down more than MaxSink.
	 * GroundHeight gives the ground's height under a point, unset where there's none nearby.
	 */
	AI_LOOTER_SHOOTER_API TOptional<double> SinkToFit(const FTransform& Patch, double Radius, double MaxSink,
		TFunctionRef<TOptional<double>(const FVector&)> GroundHeight);
}
