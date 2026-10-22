#pragma once

#include "CoreMinimal.h"
#include "Weapons/ImpactSurface.h"

class AActor;
class FWeaponFX;
class UObject;

/**
 * The ember powers' punch (the VFX pass, Saved/Vfx/PLAN.md; Looter.Vfx.Power.Scale): the beats every power shares, so each
 * reads as a power and not a puff, on FWeaponFX's shapes (rings, fireballs, starbursts; M_FX_Shape or its stand-ins). No
 * lights. Each power calls them at its moments (its use, its blow landing, its strike, its blast); EmberPowerBase::Hurt
 * calls BodyHit for every power's damage, so a blow always shows on the body it lands on.
 */
namespace EmberVfx
{
	/**
	 * A power's use, felt round the user's feet: a ring of its colour racing out over the ground, a slower fainter one after
	 * it, and motes of it rising. Strength sizes it (1 a power's use; 0.6 a small one).
	 */
	AI_LOOTER_SHOOTER_API void Activation(FWeaponFX& FX, const FVector& Feet, const FLinearColor& Color, float Strength = 1.f);

	/**
	 * A power's blow landing on Target at Where along Way: what a creature spills there (its blood, as a gun's hit of the
	 * blow's size: Damage 20 a rifle round's) and a pop of the power's colour. Nothing on what isn't a creature.
	 */
	AI_LOOTER_SHOOTER_API void BodyHit(FWeaponFX& FX, const AActor& Target, const FVector& Where, const FVector& Way, float Damage,
		const FLinearColor& Color);

	/** A strike: a starburst of Color and a ring snapping out round it, facing the eye (a hook biting, a wire tripping, a card's ricochet). */
	AI_LOOTER_SHOOTER_API void Strike(FWeaponFX& FX, const FVector& Where, const FLinearColor& Color, float Size = 1.f);

	/**
	 * A power's blast at Center (Up its ground's): SpawnExplosion in its colour (a fire's soot, or a soul blast's pale breath
	 * and rising motes), Radius its reach, Ground what it burst on (Unknown: in the air, no ground thrown).
	 */
	AI_LOOTER_SHOOTER_API void Blast(FWeaponFX& FX, const UObject* WorldContext, const FVector& Center, const FVector& Up, float Radius,
		const FLinearColor& Color, bool bSoul, EImpactMaterial Ground = EImpactMaterial::Dirt);

	/** A shaft of light coming down onto Where, Height cm tall (Last Rites' strike), with a ring where it lands. */
	AI_LOOTER_SHOOTER_API void Column(FWeaponFX& FX, const FVector& Where, float Height, const FLinearColor& Color);

	/** How big a blow's spill is for its damage (a gun hit's scale: Damage 20 = 1, 0.6 to 1.6). Tests read it. */
	AI_LOOTER_SHOOTER_API float BlowScale(float Damage);
}
