#include "Player/FootfallDust.h"
#include "Weapons/WeaponFX.h"

namespace
{
	/** A sprinting step's puffs and grains at full amount (dirt), and a landing's: a hop's worth, plus a hard fall's more. */
	constexpr float StepPuffs = 2.f;
	constexpr float StepGrit = 2.f;
	constexpr float LandPuffs = 4.f;
	constexpr float HardLandPuffs = 8.f;
	constexpr float LandGrit = 3.f;
	constexpr float HardLandGrit = 9.f;
	/** A grain costs this share of a puff from the budget (a lit chip is cheap; a puff is a soft quad over the view). */
	constexpr float GrainCost = 0.25f;
	/** The thickest a footfall's dust is (a slide's is 0.4: a step's is lighter, being shorter-lived). */
	constexpr float FootfallOpacity = 0.36f;
}

float FFootfallDust::DustAmount(EImpactMaterial Surface)
{
	switch (Surface)
	{
	case EImpactMaterial::Dirt:
	case EImpactMaterial::Sand: return 1.f;
	case EImpactMaterial::Gravel: return 0.7f;
	case EImpactMaterial::Unknown: return 0.6f;
	case EImpactMaterial::Hay: return 0.4f;
	case EImpactMaterial::Grass: return 0.35f;
	case EImpactMaterial::Stone: return 0.15f;
	case EImpactMaterial::Bone: return 0.12f;
	case EImpactMaterial::Wood: return 0.1f;
	default: return 0.f; // metal, sheet metal, glass, cloth; water splashes instead
	}
}

float FFootfallDust::GritAmount(EImpactMaterial Surface)
{
	switch (Surface)
	{
	case EImpactMaterial::Gravel: return 1.f;
	case EImpactMaterial::Dirt: return 0.45f;
	case EImpactMaterial::Sand: return 0.35f;
	case EImpactMaterial::Unknown: return 0.3f;
	case EImpactMaterial::Stone: return 0.25f;
	case EImpactMaterial::Grass: return 0.12f;
	default: return 0.f;
	}
}

FLinearColor FFootfallDust::DustAlbedo(EImpactMaterial Surface)
{
	switch (Surface)
	{
	case EImpactMaterial::Sand: return FLinearColor(0.55f, 0.48f, 0.36f);
	case EImpactMaterial::Gravel: return FLinearColor(0.45f, 0.43f, 0.40f);
	case EImpactMaterial::Grass: return FLinearColor(0.36f, 0.36f, 0.24f);
	case EImpactMaterial::Hay: return FLinearColor(0.55f, 0.50f, 0.32f);
	case EImpactMaterial::Stone: return FLinearColor(0.46f, 0.44f, 0.40f);
	case EImpactMaterial::Wood: return FLinearColor(0.42f, 0.37f, 0.30f);
	case EImpactMaterial::Bone: return FLinearColor(0.60f, 0.58f, 0.52f);
	default: return FLinearColor(0.40f, 0.33f, 0.25f); // dirt, and ground nothing names
	}
}

int32 FFootfallDust::PuffsFor(EFootfall Kind, EImpactMaterial Surface, float Hardness)
{
	if (Splashes(Surface))
	{
		return 0;
	}
	const float Most = Kind == EFootfall::Step ? StepPuffs : LandPuffs + HardLandPuffs * FMath::Clamp(Hardness, 0.f, 1.f);
	return FMath::FloorToInt32(Most * DustAmount(Surface) + 0.5f);
}

int32 FFootfallDust::GritFor(EFootfall Kind, EImpactMaterial Surface, float Hardness)
{
	// A step flicks grit only off loose ground (gravel); a landing knocks some out of most.
	if (Kind == EFootfall::Step)
	{
		return FMath::FloorToInt32(StepGrit * GritAmount(Surface));
	}
	return FMath::FloorToInt32((LandGrit + HardLandGrit * FMath::Clamp(Hardness, 0.f, 1.f)) * GritAmount(Surface) + 0.5f);
}

void FFootfallDust::Refill(float Seconds)
{
	Budget = FMath::Min(BudgetCap, Budget + BudgetRate * FMath::Max(Seconds, 0.f));
}

int32 FFootfallDust::Throw(FWeaponFX& Effects, EFootfall Kind, EImpactMaterial Surface, const FFootfallSpot& Spot, const FLinearColor& Light)
{
	const float Scale = FMath::Max(Spot.Scale, 0.1f);
	const float Hardness = FMath::Clamp(Spot.Hardness, 0.f, 1.f);
	const FVector Up = Spot.Up.IsNearlyZero() ? FVector::UpVector : Spot.Up.GetSafeNormal();
	if (Splashes(Surface))
	{
		// Water: a splash and its mist (the water hit's look, small), a hard landing's bigger.
		if (Budget < SplashCost)
		{
			return 0;
		}
		Budget -= SplashCost;
		const float Size = Kind == EFootfall::Land ? FMath::Lerp(0.8f, 1.6f, Hardness) : 0.6f;
		Effects.SpawnSurfaceImpact(Spot.Ground + Up * 2.0, Up, -Up, EImpactMaterial::Water, Size * Scale, false);
		++SplashCount;
		return 1;
	}

	const int32 Puffs = FMath::Min(PuffsFor(Kind, Surface, Hardness), FMath::FloorToInt32(Budget));
	const int32 Grains = FMath::Max(FMath::Min(GritFor(Kind, Surface, Hardness), FMath::FloorToInt32((Budget - Puffs) / GrainCost)), 0);
	if (Puffs <= 0 && Grains <= 0)
	{
		return 0;
	}
	Budget = FMath::Max(Budget - Puffs - Grains * GrainCost, 0.f);

	// The body's way along the ground, and across it (any, for a landing straight down).
	FVector Along = Spot.Along - Up * FVector::DotProduct(Spot.Along, Up);
	if (!Along.Normalize())
	{
		Along = FVector::CrossProduct(Up, FMath::Abs(Up.X) < 0.9 ? FVector::ForwardVector : FVector::RightVector).GetSafeNormal();
	}
	const FVector Across = FVector::CrossProduct(Up, Along);
	const FLinearColor Color = DustAlbedo(Surface) * Light;
	const float Amount = DustAmount(Surface);
	const float Opacity = FootfallOpacity * FMath::Lerp(0.55f, 1.f, Amount) * (Kind == EFootfall::Land ? FMath::Lerp(0.85f, 1.15f, Hardness) : 0.9f);
	FFootfallSpot Flat = Spot;
	Flat.Up = Up;
	Flat.Along = Along;

	if (Kind == EFootfall::Step)
	{
		// Off the heel on the foot's side, left behind as the run carries on.
		const FVector Side = Across * (Spot.Side < 0.f ? -1.f : 1.f);
		for (int32 Index = 0; Index < Puffs; ++Index)
		{
			ThrowStepPuff(Effects, Flat, Side, Color, Opacity);
		}
		for (int32 Index = 0; Index < Grains; ++Index)
		{
			const FVector Where = Spot.Ground + Side * (8.0 * Scale) + Up * 2.0;
			const FVector Velocity = -Along * (Spot.Speed * Random.FRandRange(0.15f, 0.35f)) + Side * Random.FRandRange(-40.f, 60.f)
				+ Up * Random.FRandRange(120.f, 260.f);
			Effects.SpawnGrit(Where, Velocity, Random.FRandRange(0.7f, 1.5f), Random.FRandRange(0.4f, 0.7f));
		}
	}
	else
	{
		// A ring spreading out over the ground from the feet, round evenly with a little jitter; grit knocked out of it.
		for (int32 Index = 0; Index < Puffs; ++Index)
		{
			const float Angle = UE_TWO_PI * (static_cast<float>(Index) + Random.FRandRange(-0.3f, 0.3f)) / static_cast<float>(FMath::Max(Puffs, 1));
			const FVector Out = Along * FMath::Cos(Angle) + Across * FMath::Sin(Angle);
			ThrowLandPuff(Effects, Flat, Out, Color, Opacity);
		}
		for (int32 Index = 0; Index < Grains; ++Index)
		{
			const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
			const FVector Out = Along * FMath::Cos(Angle) + Across * FMath::Sin(Angle);
			const FVector Velocity = Out * (Random.FRandRange(150.f, 380.f) * FMath::Lerp(0.7f, 1.4f, Hardness)) + Up * Random.FRandRange(140.f, 320.f);
			Effects.SpawnGrit(Spot.Ground + Out * (10.0 * Scale) + Up * 2.0, Velocity, Random.FRandRange(0.7f, 1.6f), Random.FRandRange(0.45f, 0.75f));
		}
	}
	PuffsThrown += Puffs;
	GritThrown += Grains;
	return Puffs;
}

void FFootfallDust::ThrowStepPuff(FWeaponFX& Effects, const FFootfallSpot& Spot, const FVector& Side, const FLinearColor& Color, float Opacity)
{
	const float Scale = FMath::Max(Spot.Scale, 0.1f);
	const FVector Where = Spot.Ground + (Side * Random.FRandRange(4.f, 14.f) + Spot.Along * Random.FRandRange(-10.f, 15.f)) * Scale
		+ Spot.Up * Random.FRandRange(3.f, 8.f);
	const FVector Velocity = -Spot.Along * (Spot.Speed * Random.FRandRange(0.1f, 0.25f)) + Side * Random.FRandRange(10.f, 45.f)
		+ Spot.Up * Random.FRandRange(25.f, 70.f);
	const float Size = Random.FRandRange(10.f, 15.f) * Scale;
	Effects.SpawnDustPuff(Where, Velocity, Color, Opacity * Random.FRandRange(0.8f, 1.1f), Size, Size * Random.FRandRange(3.f, 4.f),
		Random.FRandRange(0.6f, 0.95f));
}

void FFootfallDust::ThrowLandPuff(FWeaponFX& Effects, const FFootfallSpot& Spot, const FVector& Out, const FLinearColor& Color, float Opacity)
{
	// Out over the ground past the feet, wider and bigger the harder it came down: far enough that the first-person view
	// sees it roll out at the bottom of the screen.
	const float Scale = FMath::Max(Spot.Scale, 0.1f);
	const float Hardness = FMath::Clamp(Spot.Hardness, 0.f, 1.f);
	const FVector Where = Spot.Ground + Out * (Random.FRandRange(12.f, 25.f) * Scale) + Spot.Up * Random.FRandRange(3.f, 8.f);
	const FVector Velocity = Out * (Random.FRandRange(120.f, 240.f) * FMath::Lerp(0.7f, 1.5f, Hardness)) + Spot.Along * (Spot.Speed * 0.3f)
		+ Spot.Up * Random.FRandRange(15.f, 55.f);
	const float Size = Random.FRandRange(12.f, 19.f) * Scale * FMath::Lerp(1.f, 1.3f, Hardness);
	Effects.SpawnDustPuff(Where, Velocity, Color, Opacity * Random.FRandRange(0.8f, 1.1f), Size, Size * Random.FRandRange(3.2f, 4.4f),
		Random.FRandRange(0.8f, 1.25f) * FMath::Lerp(1.f, 1.3f, Hardness));
}
