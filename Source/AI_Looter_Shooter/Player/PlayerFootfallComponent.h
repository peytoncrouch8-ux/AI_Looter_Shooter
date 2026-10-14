#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "Player/DustLight.h"
#include "Player/FootfallDust.h"
#include "PlayerFootfallComponent.generated.h"

class UPlayerLocomotionComponent;

/** The sound cues the feet's dust brings (Art/Sounds/cues.json gives each its sounds; with none a cue plays nothing). */
namespace FootfallCue
{
	/** A hard landing's body thud under Player.Land: low, heavy, a little dust in it (3D at the feet). */
	inline constexpr const TCHAR* HardLand = TEXT("Player.Land.Hard");
	/** Feet coming down in water from a jump or a fall, or sprinting through it: a splash (3D at the feet). */
	inline constexpr const TCHAR* Splash = TEXT("Player.Splash");
}

/**
 * The dust the player's own feet throw up (concept F, "your feet"; FFootfallDust has the looks): a puff off every other
 * sprinting step, a ring of dust spreading out from a landing (bigger on a hard one, with a thud under the landing's
 * sound), a splash in water; by the surface under the foot (ImpactSurface, through the bullets' trace channel, which
 * water answers), lit by the light where it is (FDustLight). The slide has its own (FSlideDust). Seen in third person
 * and at the edge of the first-person view; capped by FFootfallDust's budget.
 *
 * ALooterCharacter makes it. It hears the landings (the character's LandedDelegate) and ticks for the sprint's stride
 * alone: a few numbers a frame, and one short trace per puffing step.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerFootfallComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerFootfallComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * A footfall now on the ground under the feet (Hardness 0-1 for a landing), or on Ground when given (a landing's hit):
	 * its dust or splash, as the budget allows. Returns the puffs thrown (a splash counts one). Public for the tests.
	 */
	int32 Footfall(EFootfall Kind, float Hardness, const FHitResult* Ground = nullptr);

	/** A landing's hardness for the speed it came down at (cm/s): 0 up to SoftLandSpeed, 1 from HardLandSpeed. */
	static float LandHardness(float FallSpeed);

	/** Every this many sprinting steps throws a puff. */
	static constexpr int32 StepsPerPuff = 2;
	/** Landings slower than this throw nothing (a curb's hop); at HardLandSpeed and over they're hard ones. */
	static constexpr float SoftLandSpeed = 300.f;
	static constexpr float HardLandSpeed = 1000.f;
	/** A landing at least this hard has the thud (FootfallCue::HardLand) under it. */
	static constexpr float ThudHardness = 0.6f;

	const FFootfallDust& GetDust() const { return Dust; }
	/** The surface the last footfall found (tests). */
	EImpactMaterial GetLastSurface() const { return LastSurface; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	/** The ground under the feet now (or water over it), on the bullets' channel, with its face for its material. */
	bool FindGround(FHitResult& OutHit) const;

	/** Sprinting on the ground now (not sliding, climbing or creeping). */
	bool IsSprinting(float& OutSpeed) const;

	FFootfallDust Dust;
	FDustLight Light;
	TWeakObjectPtr<UPlayerLocomotionComponent> Locomotion;
	/** Ground left to cover before the next sprinting step (cm), steps taken, and when the budget was last refilled. */
	float StrideLeft = 0.f;
	int32 Steps = 0;
	double LastRefill = 0.0;
	EImpactMaterial LastSurface = EImpactMaterial::Unknown;
};
