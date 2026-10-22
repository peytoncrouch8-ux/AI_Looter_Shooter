#pragma once

#include "CoreMinimal.h"
#include "Weapons/PartEffect.h"
#include "Weapons/PartEffectTuning.h"

/**
 * The part effects' plain rules (numbers in, numbers out; the tests drive them without a world): words for the cards,
 * the Explosive round's split and falloff, the Overcharge's wind-up, the Twin's two ways, the Ricochet's pick. The bullets'
 * side plays them (UPartEffectSubsystem), the trigger's is WeaponTrigger::PlanFor.
 */
namespace PartEffectRules
{
	/** "RICOCHET", "BURST", "EXPLOSIVE", "PIERCING", "OVERCHARGE", "TWIN"; empty for None. */
	AI_LOOTER_SHOOTER_API FString Word(EPartEffect Effect);

	/** What it does, in a line for the loot card. Empty for None. */
	AI_LOOTER_SHOOTER_API FString Description(EPartEffect Effect);

	/** The effect a CSV cell names (its word, any case); None for anything else. */
	AI_LOOTER_SHOOTER_API EPartEffect Parse(const FString& Text);

	/** An Explosive round's splash, from what it dealt to what it hit (Direct of the round): Splash of the round. */
	AI_LOOTER_SHOOTER_API float SplashDamage(float DirectDamage, const FPartEffectTuning& Tuning);

	/** The splash's share Distance cm from its middle: 1 there, EdgeShare at its Radius, nothing past it. */
	AI_LOOTER_SHOOTER_API float SplashShareAt(float Distance, const FPartEffectTuning& Tuning);

	/** How charged an Overcharge pull is after HeldSeconds (0 to 1). */
	AI_LOOTER_SHOOTER_API float ChargeAfter(float HeldSeconds, const FPartEffectTuning& Tuning);

	/** A round's damage at Charge (0 to 1): 1 to OverchargeDamage times (MinCharge at least once anything's held). */
	AI_LOOTER_SHOOTER_API float ChargeDamageScale(float Charge, const FPartEffectTuning& Tuning);

	/** A Twin shot's two ways about Direction: TwinAngle apart, turned about Up (the view's up), in OutLeft and OutRight. */
	AI_LOOTER_SHOOTER_API void TwinDirections(const FVector& Direction, const FVector& Up, const FPartEffectTuning& Tuning,
		FVector& OutLeft, FVector& OutRight);

	/** Of Candidates, the nearest to From within Radius; INDEX_NONE for none (a Ricochet's bounce). */
	AI_LOOTER_SHOOTER_API int32 PickNearest(const FVector& From, TConstArrayView<FVector> Candidates, float Radius);
}
