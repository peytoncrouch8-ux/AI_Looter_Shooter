#pragma once

#include "CoreMinimal.h"
#include "ShroudChain.generated.h"

/** How an Unpaid's shroud moves (FShroudChain). Angles in degrees. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FShroudChainSettings
{
	GENERATED_BODY()

	/** It swings back this far for each meter a second it's carried forward (and aside for each sideways), up to MaxTrail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0"))
	float TrailPerMeter = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0", ClampMax = "100"))
	float MaxTrail = 70.f;

	/** It swings aside this far for each 100 degrees a second the body turns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0"))
	float TurnTrail = 10.f;

	/**
	 * The ripple running down it: this many degrees at RippleFullSpeed (cm/s) and above, less the slower it goes, this
	 * many times a second, each link this much later than the one above (radians).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0"))
	float RippleDegrees = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0"))
	float RippleHz = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0"))
	float RippleLag = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "1", Units = "cm/s"))
	float RippleFullSpeed = 450.f;

	/** At rest it sways slowly, this far, this many times a second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0"))
	float SwayDegrees = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0"))
	float SwayHz = 0.3f;

	/** How quickly the top link follows where it's put (spring stiffness, 1/s^2); each link down is LowerSlower less quick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "1"))
	float Stiffness = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0", ClampMax = "0.15"))
	float LowerSlower = 0.12f;

	/** Its springs' damping against critical (1): a little under, so it flows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0.1", ClampMax = "2"))
	float DampingRatio = 0.55f;

	/** A lunge snaps it straight: this stiff, critically damped, every link StraightAngle from hanging down (90 is level). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "1"))
	float SnapStiffness = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shroud", meta = (ClampMin = "0", ClampMax = "100"))
	float StraightAngle = 80.f;
};

/**
 * An Unpaid's shroud, or one of the strips at its sides: a chain of links hanging from the hips, with no physics. Each
 * link swings from where it hangs at rest by two angles in the body's frame, Back (toward the back and up, about the
 * body's right-hand axis) and Side (to the right), on a damped spring toward where the body's motion puts it: trailing its
 * velocity (lower links further, and later, as they're slower to follow), with a ripple running down the chain that grows
 * with speed, a slow sway at rest, and on a lunge snapped straight back. Chains of the same body run out of phase
 * (PhaseOffset), so the strips never flap in step. Plain math, so the tests drive it; AUnpaidCreature turns the angles
 * into its tail bones' pose.
 */
struct AI_LOOTER_SHOOTER_API FShroudChain
{
	struct FLink
	{
		/** Where it hangs at rest: degrees from straight down toward the back (HangAngle). */
		float RestAngle = 0.f;
		float Back = 0.f;
		float Side = 0.f;
		float BackSpeed = 0.f;
		float SideSpeed = 0.f;
	};

	TArray<FLink> Links;

	/** Sets this chain apart from the others on the body (radians): the side strips run out of phase with the middle. */
	float PhaseOffset = 0.f;

	/** Where the ripple is in its cycle (radians), and the sway's clock (s). */
	float RipplePhase = 0.f;
	float Time = 0.f;

	/** A chain of links hanging at these angles (degrees from straight down toward the back), all at rest. */
	void Init(TConstArrayView<float> RestAngles, float InPhaseOffset);

	/** Every link back where it hangs at rest, and still. */
	void Settle();

	/**
	 * Moves it on by DeltaSeconds for a body moving at LocalVelocity (cm/s in its own frame: X forward, Y right) and turning
	 * at YawRate (degrees a second, positive to the right), snapped straight by Snap (0 to 1: a lunge). In short even steps,
	 * so a distant Unpaid updating a few times a second swings as one nearby does; an update longer than
	 * FCreatureUpdateRate::MaxInterval (a hitch) moves it that far.
	 */
	void Step(const FShroudChainSettings& Settings, const FVector& LocalVelocity, float YawRate, float Snap, float DeltaSeconds);

	/** Link Index's turn from rest in the body's frame, for its bone (RestDirection: the link's own direction at rest). */
	FQuat LinkRotation(int32 Index, const FVector& RestDirection) const;

	/** A direction's angle from straight down toward the back (degrees), in the body's frame (X forward, Z up). */
	static float HangAngle(const FVector& Direction);

	/** The Back swing that lines a link hanging at RestAngle up with the others straight behind the body (a lunge). */
	static float StraightBack(const FShroudChainSettings& Settings, float RestAngle);
};
