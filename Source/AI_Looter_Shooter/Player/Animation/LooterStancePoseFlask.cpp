#include "Player/Animation/LooterStancePose.h"
#include "Player/PlayerFlaskMotion.h"
#include "Player/PlayerFlaskRules.h"

// The stance pose's drink from the soul flask: a code pose like the grenade throw's, played through the throw's own stages
// (ApplyThrowBody, ApplyThrowArm) on FFlaskRules' and FlaskMotion's clock, so it shows from behind (third person) and in
// the body's shadow in first person as the first-person flask moves: the left hand brings the flask up to the mouth, the
// elbow lifts as it's tipped up through the pour, the chest leaning back a touch, then it's lowered away. The gun in the
// right hand dips as for a throw. Offsets are the full-size mannequin's, from the left shoulder: X ahead, Y out to the
// body's left, Z up.

namespace
{
	/** One moment of the drink: the left wrist and where its elbow points, and the chest's turn and lean (degrees). */
	struct FDrinkKey
	{
		FVector Wrist;
		FVector Elbow;
		float Twist;
		float Lean;
	};

	/**
	 * At the mouth: the wrist under the chin and a little ahead (the flask's neck at the lips, 8 cm past the wrist), the
	 * elbow low and out, the left shoulder brought a touch forward.
	 */
	FDrinkKey AtMouthKey()
	{
		return { FVector(18.f, -12.f, 8.f), FVector(6.f, 20.f, -18.f), 6.f, 0.f };
	}

	/** Tipped up: the wrist risen and turned, the elbow lifted out to the side, leaning back to drink. */
	FDrinkKey TippedKey()
	{
		return { FVector(20.f, -11.f, 15.f), FVector(8.f, 24.f, -4.f), 8.f, -6.f };
	}

	FDrinkKey Blend(const FDrinkKey& From, const FDrinkKey& To, float Alpha)
	{
		return { FMath::Lerp(From.Wrist, To.Wrist, Alpha), FMath::Lerp(From.Elbow, To.Elbow, Alpha),
			FMath::Lerp(From.Twist, To.Twist, Alpha), FMath::Lerp(From.Lean, To.Lean, Alpha) };
	}
}

FLooterThrowStance LooterStancePose::DescribeDrink(float DrinkTime)
{
	FLooterThrowStance Drink;
	const float End = FFlaskRules::DrinkSeconds;
	if (DrinkTime <= 0.f || DrinkTime >= End)
	{
		return Drink;
	}
	Drink.GunDip = FlaskMotion::GunDipShare(DrinkTime);

	FDrinkKey Key = AtMouthKey();
	const float Raise = FlaskMotion::RaiseSeconds;
	const float Sip = FFlaskRules::SipAt;
	const float TipEnd = FlaskMotion::TipEndSeconds;
	const float Lower = FlaskMotion::LowerSeconds;
	if (DrinkTime < Raise)
	{
		// Up to the mouth off whatever the arm was doing (the foregrip, or the walk's swing), slowing as it gets there.
		Drink.Arm = FMath::InterpEaseOut(0.f, 1.f, DrinkTime / Raise, 2.f);
	}
	else if (DrinkTime < TipEnd)
	{
		// Tipped up through the pour, as the first-person flask is.
		Drink.Arm = 1.f;
		Key = Blend(AtMouthKey(), TippedKey(), FMath::SmoothStep(Sip, TipEnd, DrinkTime));
	}
	else
	{
		// Lowered away (the flask back at the belt by Lower) and the arm back to the foregrip (or the walk) by the end.
		Drink.Arm = 1.f - FMath::SmoothStep(TipEnd, End, DrinkTime);
		Key = Blend(TippedKey(), AtMouthKey(), FMath::SmoothStep(TipEnd, Lower, DrinkTime));
	}
	Drink.Hand = Key.Wrist;
	Drink.Elbow = Key.Elbow;
	// The chest comes into it with the arm and goes back with it.
	Drink.Twist = Key.Twist * Drink.Arm;
	Drink.Lean = Key.Lean * Drink.Arm;
	return Drink;
}
