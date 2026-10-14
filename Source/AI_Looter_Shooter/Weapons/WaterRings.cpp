#include "Weapons/WaterRings.h"

// FWaterRings: the rules and the pool of rings and wakes on the water (the header says what they look like). The quads it
// draws are flat dashes of FWeaponFX's soft smoke quad laid along a circle (a ring) or along a V's arms (a wake).

namespace
{
	/** A shot's second ring: this share of the first's radius and opacity, a beat later, living a little shorter. */
	constexpr float InnerRadiusShare = 0.62f;
	constexpr float InnerOpacityShare = 0.7f;
	constexpr float InnerLifeShare = 0.9f;
	constexpr float FollowDelay = 0.1f;
	/** The ring's opacity at its strongest (before its dashes vary), and a hit smaller than this Size is a pellet's. */
	constexpr float RingOpacity = 0.55f;
	constexpr float PelletSize = 0.8f;
	constexpr float PelletOpacityShare = 0.85f;
	/** A dash is this much longer than the arc between dashes, so a ring breaks up rather than showing beads. */
	constexpr float DashOverlap = 1.3f;
	/** The share of a ring's dashes that are missing (a ripple is never whole). */
	constexpr float GapShare = 0.15f;
	/** The quads' height over the surface is the caller's; the second ring sits this much higher so two never share a plane. */
	constexpr float RingLift = 0.3f;

	/** A wake's arms leave the track at this angle (a boat's wake opens at about 19.5 degrees), drift outward and grow. */
	constexpr float WakeArmAngle = 22.f;
	constexpr float WakeArmLength = 85.f;
	constexpr float WakeDrift = 22.f;
	constexpr float WakeWidth = 7.f;
	constexpr float WakeOpacity = 0.5f;
	/** The speeds (cm/s) a wake goes from its faintest to its strongest between: a creep and a sprint. */
	constexpr float WakeSlowSpeed = 150.f;
	constexpr float WakeFastSpeed = 711.f;

	float WakeSpeedShare(float Speed)
	{
		return FMath::Clamp((Speed - WakeSlowSpeed) / (WakeFastSpeed - WakeSlowSpeed), 0.f, 1.f);
	}
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

float FWaterRings::MaxRadiusFor(float Size)
{
	return FMath::Lerp(RadiusSmall, RadiusLarge, FMath::Clamp((Size - 0.5f) / 1.f, 0.f, 1.f));
}

float FWaterRings::LifeFor(float MaxRadius)
{
	return FMath::Lerp(LifeShort, LifeLong, FMath::Clamp((MaxRadius - RadiusSmall) / (RadiusLarge - RadiusSmall), 0.f, 1.f));
}

int32 FWaterRings::RingsFor(float Size)
{
	return Size >= PelletSize ? 2 : 1;
}

float FWaterRings::RadiusAt(float T, float StartRadius, float MaxRadius)
{
	// Quick at first, slowing as it spreads: the water settles.
	const float Eased = 1.f - FMath::Square(1.f - FMath::Clamp(T, 0.f, 1.f));
	return FMath::Lerp(StartRadius, MaxRadius, Eased);
}

float FWaterRings::OpacityAt(float T)
{
	const float Clamped = FMath::Clamp(T, 0.f, 1.f);
	return FMath::Clamp(Clamped / 0.08f, 0.f, 1.f) * FMath::Pow(1.f - Clamped, 1.5f);
}

int32 FWaterRings::SegmentsFor(float MaxRadius)
{
	return FMath::Clamp(FMath::RoundToInt32(MaxRadius / 6.f), 10, 18);
}

bool FWaterRings::WakeDue(float Travelled, float Speed, float Scale)
{
	return Speed >= WakeMinSpeed && Travelled >= WakeSpacing * FMath::Max(Scale, 0.1f);
}

float FWaterRings::WakeOpacityFor(float Speed)
{
	return FMath::Lerp(0.45f, 1.f, WakeSpeedShare(Speed));
}

float FWaterRings::Hash01(uint32 Seed, uint32 Index)
{
	uint32 Hash = Seed * 0x9E3779B1u + (Index + 1u) * 0x85EBCA6Bu;
	Hash ^= Hash >> 15;
	Hash *= 0x2C1B3C6Du;
	Hash ^= Hash >> 12;
	Hash *= 0x297A2D39u;
	Hash ^= Hash >> 15;
	return static_cast<float>(Hash & 0xFFFFFFu) / 16777216.f;
}

// ---------------------------------------------------------------------------
// Pool
// ---------------------------------------------------------------------------

int32 FWaterRings::AddRings(const FVector& Center, const FVector& Up, float Size, int32 Count, const FLinearColor& Color)
{
	const FVector Normal = Up.GetSafeNormal();
	if (Count <= 0 || !LiesFlat(Normal))
	{
		return 0;
	}
	const float Scale = FMath::Clamp(Size, 0.4f, 1.6f);
	const float FullRadius = MaxRadiusFor(Scale);
	int32 Added = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (Budget < RingCost)
		{
			break;
		}
		Budget -= RingCost;
		if (Rings.Num() >= MaxRings)
		{
			Rings.RemoveAt(0, 1, EAllowShrinking::No); // the oldest is nearly gone anyway
		}
		const float Steps = static_cast<float>(Index);
		FRing& Ring = Rings.AddDefaulted_GetRef();
		Ring.Center = Center + Normal * (RingLift * Steps);
		Ring.Up = Normal;
		Ring.Color = Color;
		Ring.MaxRadius = FullRadius * FMath::Pow(InnerRadiusShare, Steps);
		Ring.StartRadius = FMath::Min(5.f * Scale, Ring.MaxRadius * 0.5f);
		Ring.Life = LifeFor(FullRadius) * FMath::Pow(InnerLifeShare, Steps);
		Ring.Age = -FollowDelay * Steps;
		Ring.Width = 6.f + 5.f * Scale;
		Ring.Opacity = RingOpacity * FMath::Pow(InnerOpacityShare, Steps) * (Scale < PelletSize ? PelletOpacityShare : 1.f);
		Ring.Segments = SegmentsFor(Ring.MaxRadius);
		Ring.Seed = NextSeed++;
		++Added;
	}
	return Added;
}

bool FWaterRings::AddWake(const FVector& Origin, const FVector& Up, const FVector& Direction, float Speed, float Scale, const FLinearColor& Color)
{
	const FVector Normal = Up.GetSafeNormal();
	if (!LiesFlat(Normal))
	{
		return false;
	}
	// Back along the surface from the way the body is going.
	FVector Back = -(Direction - Normal * FVector::DotProduct(Direction, Normal));
	if (!Back.Normalize())
	{
		return false;
	}
	if (Wakes.Num() >= MaxWakes)
	{
		Wakes.RemoveAt(0, 1, EAllowShrinking::No);
	}
	const float Body = FMath::Clamp(Scale, 0.1f, 2.f);
	FWake& Wake = Wakes.AddDefaulted_GetRef();
	Wake.Origin = Origin;
	Wake.Up = Normal;
	Wake.Back = Back;
	Wake.Side = FVector::CrossProduct(Normal, Back).GetSafeNormal();
	Wake.Color = Color;
	Wake.ArmLength = WakeArmLength * Body * FMath::Lerp(0.8f, 1.15f, WakeSpeedShare(Speed));
	Wake.Width = WakeWidth * Body;
	Wake.Opacity = WakeOpacity * WakeOpacityFor(Speed);
	return true;
}

void FWaterRings::Tick(float DeltaSeconds)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	Budget = FMath::Min(BudgetCap, Budget + BudgetRate * Dt);
	for (int32 Index = Rings.Num() - 1; Index >= 0; --Index)
	{
		FRing& Ring = Rings[Index];
		Ring.Age += Dt;
		if (Ring.Age >= Ring.Life)
		{
			Rings.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}
	for (int32 Index = Wakes.Num() - 1; Index >= 0; --Index)
	{
		FWake& Wake = Wakes[Index];
		Wake.Age += Dt;
		if (Wake.Age >= WakeLife)
		{
			Wakes.RemoveAt(Index, 1, EAllowShrinking::No);
		}
	}
}

void FWaterRings::Reset()
{
	Rings.Reset();
	Wakes.Reset();
	Budget = BudgetCap;
	NextSeed = 1;
}

void FWaterRings::Draw(const FVector& Camera, TArray<FTransform>& Transforms, TArray<float>& SmokeData) const
{
	// No camera known (the origin): everything is near enough.
	const double RangeSquared = Camera.IsZero() ? TNumericLimits<double>::Max() : FMath::Square(static_cast<double>(DrawRange));

	for (const FRing& Ring : Rings)
	{
		if (Ring.Age < 0.f || FVector::DistSquared(Ring.Center, Camera) > RangeSquared)
		{
			continue;
		}
		const float T = Ring.Age / Ring.Life;
		const float Strength = Ring.Opacity * OpacityAt(T);
		const float Radius = RadiusAt(T, Ring.StartRadius, Ring.MaxRadius);
		const float Step = UE_TWO_PI / static_cast<float>(Ring.Segments);
		const float Length = Radius * Step * DashOverlap;
		const float Width = Ring.Width * (1.f + 0.5f * T);
		// Two axes along the water to measure the circle against.
		const FVector Right = FVector::CrossProduct(Ring.Up, FMath::Abs(Ring.Up.X) < 0.9 ? FVector::ForwardVector : FVector::RightVector).GetSafeNormal();
		const FVector Forward = FVector::CrossProduct(Ring.Up, Right).GetSafeNormal();
		const float Phase = Hash01(Ring.Seed, 9999u) * UE_TWO_PI;
		for (int32 Index = 0; Index < Ring.Segments; ++Index)
		{
			const uint32 Dash = static_cast<uint32>(Index);
			if (Hash01(Ring.Seed, Dash) < GapShare)
			{
				continue; // a gap in the ripple (fixed for the ring's life, so the instance count holds)
			}
			const float Angle = Phase + Step * static_cast<float>(Index);
			const float Cosine = FMath::Cos(Angle);
			const float Sine = FMath::Sin(Angle);
			const FVector Out = Right * Cosine + Forward * Sine;
			const FVector Tangent = Right * -Sine + Forward * Cosine;
			Transforms.Add(FTransform(FRotationMatrix::MakeFromZX(Ring.Up, Tangent).ToQuat(), Ring.Center + Out * Radius,
				FVector(Length / 100.f, Width / 100.f, 1.f)));
			SmokeData.Append({ Ring.Color.R, Ring.Color.G, Ring.Color.B, Strength * FMath::Lerp(0.55f, 1.f, Hash01(Ring.Seed, Dash + 1000u)) });
		}
	}

	const float ArmCosine = FMath::Cos(FMath::DegreesToRadians(WakeArmAngle));
	const float ArmSine = FMath::Sin(FMath::DegreesToRadians(WakeArmAngle));
	for (const FWake& Wake : Wakes)
	{
		if (FVector::DistSquared(Wake.Origin, Camera) > RangeSquared)
		{
			continue;
		}
		const float T = Wake.Age / WakeLife;
		const float Strength = Wake.Opacity * OpacityAt(T);
		// The arms lengthen from half to full as the wake spreads, and drift apart.
		const float Length = Wake.ArmLength * (0.5f + 0.5f * (1.f - FMath::Square(1.f - FMath::Clamp(T, 0.f, 1.f))));
		for (int32 Arm = 0; Arm < 2; ++Arm)
		{
			const float Sign = Arm == 0 ? 1.f : -1.f;
			const FVector Along = (Wake.Back * ArmCosine + Wake.Side * (Sign * ArmSine)).GetSafeNormal();
			const FVector Middle = Wake.Origin + Along * (Length * 0.5f) + Wake.Side * (Sign * WakeDrift * Wake.Age);
			Transforms.Add(FTransform(FRotationMatrix::MakeFromZX(Wake.Up, Along).ToQuat(), Middle, FVector(Length / 100.f, Wake.Width / 100.f, 1.f)));
			SmokeData.Append({ Wake.Color.R, Wake.Color.G, Wake.Color.B, Strength });
		}
	}
}
