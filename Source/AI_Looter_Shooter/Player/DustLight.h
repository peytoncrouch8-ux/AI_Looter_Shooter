#pragma once

#include "CoreMinimal.h"

class AActor;
class UDirectionalLightComponent;
class UWorld;

/**
 * The light a puff of dust drawn in the unlit smoke material is given, so it shows as dust in that light would and
 * never glows in shade or at dusk: the sun's (its hue, lower over the horizon) where a trace toward it is clear, the
 * sky's in shade, both times the lighting state's tint (MPC_Lighting's BackdropTint, as the unlit backdrop is). FSlideDust
 * works the same out every frame of a slide, eased; this is for dust thrown now and then (the feet's footfalls): one
 * trace toward the sun per ask, the sun and the tint looked up again every RefindSeconds.
 */
class AI_LOOTER_SHOOTER_API FDustLight
{
public:
	/** The light at Where (Ignore: the body throwing the dust). */
	FLinearColor At(const UWorld& World, const FVector& Where, const AActor& Ignore);

	/** The sun and the lighting state's tint are looked up again after this long (the time of day moves on). */
	static constexpr float RefindSeconds = 10.f;

private:
	void FindLight(const UWorld& World);

	TWeakObjectPtr<const UDirectionalLightComponent> Sun;
	FLinearColor StateTint = FLinearColor::White;
	double FoundAt = -1.0e9;
};
