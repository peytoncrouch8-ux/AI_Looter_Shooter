#pragma once

#include "CoreMinimal.h"
#include "DeadMinerRules.generated.h"

struct FCampaignRecord;

/**
 * A dead miner's numbers (ADeadMinerCreature; Docs/Areas/RansomsRest_Expansion.md, section 6 "The Providence"): its pick's
 * overhead swing, its head lamp, its work at the walls. Distances at size 1 (they grow with it), seconds as they are.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FDeadMinerTuning
{
	GENERATED_BODY()

	// --- The swing ---

	/** The pick comes down on a target this far off at most (cm, flat), in front of it (the cosine off its facing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "50", Units = "cm"))
	float SwingReach = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "-1", ClampMax = "1"))
	float SwingFront = 0.35f;

	/** The pick's head bites the ground this far in front of it (cm): the sparks, the grit, a Gravebound one's ring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "0", Units = "cm"))
	float SwingImpactAhead = 150.f;

	/** Shot in the lamp during its wind-up, the swing is lost and it reels this long (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (ClampMin = "0.1", Units = "s"))
	float ReelSeconds = 1.3f;

	// --- The lamp ---

	/** The lamp's weak spot: a ball this big round its lens (cm). Small: a skill shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "2", Units = "cm"))
	float LampHitRadius = 10.f;

	/** Its light at rest and at the wind-up's flare (candela), and how far it reaches (cm). Small, warm, shadowless. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "0"))
	float LampCandela = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "0"))
	float FlareCandela = 24.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "50", Units = "cm"))
	float LampRadius = 420.f;

	/** The lamp's glow cards (cm across) and their strength; the flare swells them by this share. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "1", Units = "cm"))
	float LampGlowSize = 9.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "0"))
	float LampGlowStrength = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "0"))
	float FlareSwell = 1.2f;

	/**
	 * The area's light budget (at most 3 shadowless point lights in a view, Docs/Areas/RansomsRest_Expansion.md "Performance"):
	 * only this many dead miners nearest the player light up round them; the rest show their lamp's glow alone. Past
	 * LampLightRange (cm) none does.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "0", ClampMax = "6"))
	int32 MaxLitLamps = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp", meta = (ClampMin = "0", Units = "cm"))
	float LampLightRange = 2600.f;

	// --- Work ---

	/** Unaware, it works the nearest wall within this (cm, from its middle) at chest height; none near: its lunch pail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Work", meta = (ClampMin = "50", Units = "cm"))
	float WorkReach = 220.f;

	/** One blow at the wall, from the pick raised to the pick raised again (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Work", meta = (ClampMin = "0.5", Units = "s"))
	float WorkSwingSeconds = 2.4f;
};

/** The dead miners' rules as plain functions, so the tests check them without a level. */
namespace DeadMinerRules
{
	/** Back Pay (Docs/Areas/RansomsRest_Expansion.md, section 4) and the choice that keeps the dead at work. */
	inline const FName BackPayMission(TEXT("BackPay"));
	inline const FName KeptFlag(TEXT("BackPay.Kept"));

	/**
	 * Whether the Providence's shift still works, so its dead come back on an arrival: Back Pay isn't finished, or it was
	 * finished keeping the payroll (the flag). Paid, the eleven rest.
	 */
	AI_LOOTER_SHOOTER_API bool ShiftStillWorks(bool bMissionFinished, bool bPayrollKept);
	AI_LOOTER_SHOOTER_API bool ShiftStillWorks(const FCampaignRecord& Campaign, FName Mission = BackPayMission, FName Flag = KeptFlag);

	/**
	 * Whether the pick lands on a target ToTarget from it (world, from its middle), facing Forward: within Reach flat,
	 * MaxRise up or down, and in front (Front, the cosine off its facing).
	 */
	AI_LOOTER_SHOOTER_API bool IsSwingHit(const FVector& Forward, const FVector& ToTarget, float Reach, float Front, float MaxRise);

	/** How far into its wind-up's flare the lamp is (0-1): it swells over the wind-up, brightest at the blow. */
	AI_LOOTER_SHOOTER_API float WindupFlare(float StateTime, float Windup);

	/**
	 * Whether a dead miner DistanceToPlayer (cm) off lights up round it: within Range, and fewer than MaxLit lit lamps
	 * nearer the player (NearerLit). The rest keep only their glow.
	 */
	AI_LOOTER_SHOOTER_API bool ShouldLightLamp(float DistanceToPlayer, int32 NearerLit, int32 MaxLit, float Range);

	/** Where in a blow at the wall it is (0-1 over WorkSwingSeconds), and whether the blow lands between two times. */
	AI_LOOTER_SHOOTER_API float WorkPhase(float WorkTime, float SwingSeconds);
	AI_LOOTER_SHOOTER_API bool WorkBlowBetween(float FromTime, float ToTime, float SwingSeconds);

	/** The share of a blow at which the pick meets the wall. */
	inline constexpr float WorkBlowShare = 0.58f;

	/** A lamp's light: a carbide flame's warm white. */
	AI_LOOTER_SHOOTER_API FLinearColor LampColor();
}
