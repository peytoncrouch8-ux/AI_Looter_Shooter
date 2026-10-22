#include "Weapons/PartEffectRules.h"

FString PartEffectRules::Word(EPartEffect Effect)
{
	switch (Effect)
	{
	case EPartEffect::Ricochet: return TEXT("RICOCHET");
	case EPartEffect::Burst: return TEXT("BURST");
	case EPartEffect::Explosive: return TEXT("EXPLOSIVE");
	case EPartEffect::Piercing: return TEXT("PIERCING");
	case EPartEffect::Overcharge: return TEXT("OVERCHARGE");
	case EPartEffect::Twin: return TEXT("TWIN");
	default: return FString();
	}
}

FString PartEffectRules::Description(EPartEffect Effect)
{
	switch (Effect)
	{
	case EPartEffect::Ricochet: return TEXT("Rounds that hit bounce once to the nearest enemy.");
	case EPartEffect::Burst: return TEXT("Each pull fires a three-round burst.");
	case EPartEffect::Explosive: return TEXT("Rounds burst where they land.");
	case EPartEffect::Piercing: return TEXT("Rounds punch through an enemy into the next.");
	case EPartEffect::Overcharge: return TEXT("Each pull winds up: double damage, half the fire rate.");
	case EPartEffect::Twin: return TEXT("Two bullets a shot for one round.");
	default: return FString();
	}
}

EPartEffect PartEffectRules::Parse(const FString& Text)
{
	const FString Key = Text.TrimStartAndEnd();
	for (const EPartEffect Effect : { EPartEffect::Ricochet, EPartEffect::Burst, EPartEffect::Explosive, EPartEffect::Piercing,
		EPartEffect::Overcharge, EPartEffect::Twin })
	{
		if (Key.Equals(Word(Effect), ESearchCase::IgnoreCase))
		{
			return Effect;
		}
	}
	return EPartEffect::None;
}

float PartEffectRules::SplashDamage(float DirectDamage, const FPartEffectTuning& Tuning)
{
	// What it hit took Direct of the round, so the round was DirectDamage / Direct; the splash is Splash of that.
	if (Tuning.ExplosiveDirect <= 0.f || DirectDamage <= 0.f)
	{
		return 0.f;
	}
	return DirectDamage / Tuning.ExplosiveDirect * Tuning.ExplosiveSplash;
}

float PartEffectRules::SplashShareAt(float Distance, const FPartEffectTuning& Tuning)
{
	if (Tuning.ExplosiveRadius <= 0.f || Distance > Tuning.ExplosiveRadius)
	{
		return 0.f;
	}
	return FMath::Lerp(1.f, Tuning.ExplosiveEdgeShare, FMath::Clamp(Distance / Tuning.ExplosiveRadius, 0.f, 1.f));
}

float PartEffectRules::ChargeAfter(float HeldSeconds, const FPartEffectTuning& Tuning)
{
	if (Tuning.OverchargeSeconds <= 0.f)
	{
		return 1.f;
	}
	return FMath::Clamp(HeldSeconds / Tuning.OverchargeSeconds, 0.f, 1.f);
}

float PartEffectRules::ChargeDamageScale(float Charge, const FPartEffectTuning& Tuning)
{
	const float Held = Charge > 0.f ? FMath::Max(Charge, Tuning.OverchargeMinCharge) : 0.f;
	return FMath::Lerp(1.f, Tuning.OverchargeDamage, FMath::Clamp(Held, 0.f, 1.f));
}

void PartEffectRules::TwinDirections(const FVector& Direction, const FVector& Up, const FPartEffectTuning& Tuning, FVector& OutLeft,
	FVector& OutRight)
{
	const FVector Way = Direction.GetSafeNormal();
	FVector Axis = Up.GetSafeNormal();
	if (Axis.IsNearlyZero() || FMath::Abs(FVector::DotProduct(Axis, Way)) > 0.99f)
	{
		Axis = FMath::Abs(Way.Z) > 0.99f ? FVector::ForwardVector : FVector::UpVector;
	}
	const float Half = FMath::DegreesToRadians(Tuning.TwinAngle * 0.5f);
	OutLeft = Way.RotateAngleAxisRad(-Half, Axis);
	OutRight = Way.RotateAngleAxisRad(Half, Axis);
}

int32 PartEffectRules::PickNearest(const FVector& From, TConstArrayView<FVector> Candidates, float Radius)
{
	int32 Best = INDEX_NONE;
	double BestSquared = FMath::Square(static_cast<double>(Radius));
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const double Squared = FVector::DistSquared(From, Candidates[Index]);
		if (Squared <= BestSquared)
		{
			BestSquared = Squared;
			Best = Index;
		}
	}
	return Best;
}
