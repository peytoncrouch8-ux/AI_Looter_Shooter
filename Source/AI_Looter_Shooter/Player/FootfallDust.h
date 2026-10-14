#pragma once

#include "CoreMinimal.h"
#include "Weapons/ImpactSurface.h"

class FWeaponFX;

/** What the feet just did. */
enum class EFootfall : uint8
{
	/** A sprinting step (every few of them). */
	Step,
	/** Coming down from a jump or a fall: the harder, the more. */
	Land
};

/** One footfall's dust: where, which way the body was going, and how hard. */
struct FFootfallSpot
{
	/** The ground under the foot (on it), its up, and the body's way along the ground (unit, or zero). */
	FVector Ground = FVector::ZeroVector;
	FVector Up = FVector::UpVector;
	FVector Along = FVector::ZeroVector;
	/** The body's speed along the ground (cm/s), a landing's hardness (0 a hop's worth, 1 a hard fall) and the body's size. */
	float Speed = 0.f;
	float Hardness = 0.f;
	float Scale = 1.f;
	/** Which foot (a step's puff comes off that side): -1 left, 1 right. */
	float Side = 1.f;
};

/**
 * The dust the player's feet throw up (concept F, "your feet"), by the surface under them (ImpactSurface, its 13): dirt
 * and sand a full brown or pale puff, gravel a grey one with grit, grass a little, hay a little straw dust, stone and bone
 * a wisp on a landing only, wood, metal, glass and cloth nothing; water a splash (FWeaponFX's water hit, small, with the
 * rings it leaves: one for a step, two for a hard landing; the wading's own rings and wake are the component's). A
 * sprinting step throws a couple of puffs back off the heel; a landing a ring of them spreading out over the ground
 * (seen at the edge of the first-person view as it reaches past the feet), more and wider the harder it was, with grit
 * thrown out of it. Drawn by FWeaponFX's pooled, instanced puffs and chips (the bullet subsystem's): no draws of its own.
 *
 * Capped: puffs come out of a budget that refills over time (BudgetCap at most, BudgetRate a second), so a run of hops or
 * a scramble down rocks never piles dust up; what the budget can't pay for is simply not thrown.
 */
class AI_LOOTER_SHOOTER_API FFootfallDust
{
public:
	/** How much dust and grit each surface throws (shares of the most), and the dust's own color before the light. */
	static float DustAmount(EImpactMaterial Surface);
	static float GritAmount(EImpactMaterial Surface);
	static FLinearColor DustAlbedo(EImpactMaterial Surface);
	/** Water splashes rather than dusts. */
	static bool Splashes(EImpactMaterial Surface) { return Surface == EImpactMaterial::Water; }

	/** Puffs and grains a footfall of Kind throws on Surface (Hardness for a landing), before the budget. */
	static int32 PuffsFor(EFootfall Kind, EImpactMaterial Surface, float Hardness);
	static int32 GritFor(EFootfall Kind, EImpactMaterial Surface, float Hardness);

	/** The most puffs in hand, and how many a second come back. A splash costs SplashCost of them. */
	static constexpr float BudgetCap = 16.f;
	static constexpr float BudgetRate = 10.f;
	static constexpr float SplashCost = 4.f;

	/**
	 * Throws one footfall's dust (or splash) at Spot on Surface, lit by Light, as far as the budget pays. Returns the puffs
	 * thrown (a splash counts as one).
	 */
	int32 Throw(FWeaponFX& Effects, EFootfall Kind, EImpactMaterial Surface, const FFootfallSpot& Spot, const FLinearColor& Light);

	/** Time gone by: the budget refills. */
	void Refill(float Seconds);

	float GetBudget() const { return Budget; }
	/** Puffs, grains and splashes thrown so far (the tests). */
	int32 GetPuffsThrown() const { return PuffsThrown; }
	int32 GetGritThrown() const { return GritThrown; }
	int32 GetSplashCount() const { return SplashCount; }

private:
	void ThrowStepPuff(FWeaponFX& Effects, const FFootfallSpot& Spot, const FVector& Side, const FLinearColor& Color, float Opacity);
	void ThrowLandPuff(FWeaponFX& Effects, const FFootfallSpot& Spot, const FVector& Out, const FLinearColor& Color, float Opacity);

	FRandomStream Random{ 0xf007 };
	float Budget = BudgetCap;
	int32 PuffsThrown = 0;
	int32 GritThrown = 0;
	int32 SplashCount = 0;
};
