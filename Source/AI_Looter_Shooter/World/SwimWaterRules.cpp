#include "World/SwimWaterRules.h"
#include "HAL/IConsoleManager.h"

// Swimming's numbers, every one a cvar (full-size body, cm and seconds; SwimRules takes the body's scale).

namespace
{
	TAutoConsoleVariable<float> CVarStartDepth(TEXT("Looter.Swim.StartDepth"), 135.f,
		TEXT("Water this deep over the feet swims (full-size cm; chest deep on the full-size body, 115 cm on the player)."));
	TAutoConsoleVariable<float> CVarStandMargin(TEXT("Looter.Swim.StandMargin"), 20.f,
		TEXT("A swimmer stands again where the bed is this much shallower than the swim's start (full-size cm): no flicker at a pool's edge."));
	TAutoConsoleVariable<float> CVarBedReach(TEXT("Looter.Swim.BedReach"), 30.f,
		TEXT("How far under the feet a swimmer feels for the bed to stand on (full-size cm)."));
	TAutoConsoleVariable<float> CVarFloatEye(TEXT("Looter.Swim.FloatEye"), 12.f,
		TEXT("Floating, the eye rides this far over the surface (full-size cm): the water's line low in the view, never across it."));
	TAutoConsoleVariable<float> CVarFloatStiffness(TEXT("Looter.Swim.FloatStiffness"), 4.f,
		TEXT("How quickly the float spring closes on its height (per second)."));
	TAutoConsoleVariable<float> CVarFloatRise(TEXT("Looter.Swim.FloatRise"), 70.f,
		TEXT("Buoyancy: the fastest the body drifts up with no key held (full-size cm/s); gentle, so a dive isn't undone at once."));
	TAutoConsoleVariable<float> CVarFloatSink(TEXT("Looter.Swim.FloatSink"), 300.f,
		TEXT("The fastest the float spring sinks a body that came in high (a jump in; full-size cm/s)."));
	TAutoConsoleVariable<float> CVarSpeed(TEXT("Looter.Swim.Speed"), 0.5f,
		TEXT("Swimming speed as a share of the walk."));
	TAutoConsoleVariable<float> CVarFastSpeed(TEXT("Looter.Swim.FastSpeed"), 0.72f,
		TEXT("Swimming speed with the sprint key held, as a share of the walk."));
	TAutoConsoleVariable<float> CVarVerticalSpeed(TEXT("Looter.Swim.VerticalSpeed"), 230.f,
		TEXT("Up or down at full key (full-size cm/s)."));
	TAutoConsoleVariable<float> CVarBraking(TEXT("Looter.Swim.Braking"), 450.f,
		TEXT("The water's drag with no key held (cm/s^2): a glide of about half a metre from a full swim."));
	TAutoConsoleVariable<float> CVarEntryKeep(TEXT("Looter.Swim.EntryKeep"), 0.3f,
		TEXT("Share of a fall's speed kept on hitting the water."));
	TAutoConsoleVariable<float> CVarJumpOut(TEXT("Looter.Swim.JumpOut"), 520.f,
		TEXT("The jump key at the surface sends the body up this fast (full-size cm/s); a bank's lip in front is climbed."));
	TAutoConsoleVariable<float> CVarDiveDeadZone(TEXT("Looter.Swim.DiveDeadZone"), 25.f,
		TEXT("Looking up or down less than this (degrees), swimming forward stays level."));
	TAutoConsoleVariable<float> CVarDiveFull(TEXT("Looter.Swim.DiveFull"), 65.f,
		TEXT("Looking this far up or down (degrees), the stroke goes fully that way."));
	TAutoConsoleVariable<float> CVarStrokeRateMin(TEXT("Looter.Swim.StrokeRateMin"), 0.45f,
		TEXT("Strokes a second treading water (the hands' slow sculling)."));
	TAutoConsoleVariable<float> CVarStrokeRateMax(TEXT("Looter.Swim.StrokeRateMax"), 1.05f,
		TEXT("Strokes a second at full swimming speed."));
	TAutoConsoleVariable<float> CVarBreathSeconds(TEXT("Looter.Swim.BreathSeconds"), 22.f,
		TEXT("Seconds a full breath lasts with the head under water."));
	TAutoConsoleVariable<float> CVarRefillSeconds(TEXT("Looter.Swim.RefillSeconds"), 3.5f,
		TEXT("Seconds in the air to fill the breath again from empty."));
	TAutoConsoleVariable<float> CVarBreathBack(TEXT("Looter.Swim.BreathBack"), 0.25f,
		TEXT("Out of breath, the dive comes back once the breath is this full again."));
	TAutoConsoleVariable<float> CVarGaspBelow(TEXT("Looter.Swim.GaspBelow"), 0.45f,
		TEXT("Surfacing with less breath than this, the swimmer gasps."));

	float Get(const TAutoConsoleVariable<float>& Variable)
	{
		return Variable.GetValueOnAnyThread();
	}
}

namespace SwimRules
{
	float StartDepth(float BodyScale)
	{
		return FMath::Max(Get(CVarStartDepth), 1.f) * BodyScale;
	}

	float StandDepth(float BodyScale)
	{
		return FMath::Max(Get(CVarStartDepth) - FMath::Max(Get(CVarStandMargin), 0.f), 0.f) * BodyScale;
	}

	bool ShouldSwim(float WaterOverFeet, float BodyScale)
	{
		return WaterOverFeet > StartDepth(BodyScale);
	}

	bool CanStand(float WaterOverBed, float BodyScale)
	{
		return WaterOverBed < StandDepth(BodyScale);
	}

	float BedReach(float BodyScale)
	{
		return FMath::Max(Get(CVarBedReach), 1.f) * BodyScale;
	}

	float FloatFeetZ(float SurfaceZ, float EyeAboveFeet, float BodyScale)
	{
		return SurfaceZ + Get(CVarFloatEye) * BodyScale - EyeAboveFeet;
	}

	float FloatSpeed(float FeetZ, float TargetFeetZ, float BodyScale)
	{
		const float Wanted = (TargetFeetZ - FeetZ) * FMath::Max(Get(CVarFloatStiffness), 0.f);
		return FMath::Clamp(Wanted, -FMath::Max(Get(CVarFloatSink), 0.f) * BodyScale, FMath::Max(Get(CVarFloatRise), 0.f) * BodyScale);
	}

	float SwimSpeed(float WalkSpeed, bool bFast)
	{
		return WalkSpeed * FMath::Max(bFast ? Get(CVarFastSpeed) : Get(CVarSpeed), 0.05f);
	}

	float VerticalSpeed(float BodyScale)
	{
		return FMath::Max(Get(CVarVerticalSpeed), 0.f) * BodyScale;
	}

	float Braking()
	{
		return FMath::Max(Get(CVarBraking), 0.f);
	}

	float EntryKeep()
	{
		return FMath::Clamp(Get(CVarEntryKeep), 0.f, 1.f);
	}

	float JumpOutSpeed(float BodyScale)
	{
		return FMath::Max(Get(CVarJumpOut), 0.f) * BodyScale;
	}

	float DiveShare(float PitchDegrees)
	{
		const float Dead = FMath::Clamp(Get(CVarDiveDeadZone), 0.f, 89.f);
		const float Full = FMath::Clamp(Get(CVarDiveFull), Dead + 1.f, 90.f);
		const float Past = FMath::Clamp((FMath::Abs(PitchDegrees) - Dead) / (Full - Dead), 0.f, 1.f);
		return FMath::Sign(PitchDegrees) * FMath::SmoothStep(0.f, 1.f, Past);
	}

	float StrokeRate(float SpeedShare)
	{
		return FMath::Lerp(Get(CVarStrokeRateMin), Get(CVarStrokeRateMax), FMath::Clamp(SpeedShare, 0.f, 1.f));
	}

	float BreathAfter(float Breath, float Dt, bool bHeadUnder)
	{
		const float Step = bHeadUnder ? -Dt / FMath::Max(Get(CVarBreathSeconds), 0.1f) : Dt / FMath::Max(Get(CVarRefillSeconds), 0.1f);
		return FMath::Clamp(Breath + Step, 0.f, 1.f);
	}

	bool OutOfBreath(float Breath)
	{
		return Breath <= 0.f;
	}

	float BreathBack()
	{
		return FMath::Clamp(Get(CVarBreathBack), 0.f, 1.f);
	}

	bool GaspOnSurfacing(float Breath)
	{
		return Breath < Get(CVarGaspBelow);
	}
}

namespace SwimShore
{
	bool Contains(const TArray<FVector2D>& Polygon, const FVector2D& Point)
	{
		const int32 Count = Polygon.Num();
		if (Count < 3)
		{
			return false;
		}
		// Count the edges a ray from the point toward +X crosses: an odd count is inside.
		bool bInside = false;
		for (int32 Index = 0, Last = Count - 1; Index < Count; Last = Index++)
		{
			const FVector2D& A = Polygon[Index];
			const FVector2D& B = Polygon[Last];
			if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < A.X + (Point.Y - A.Y) * (B.X - A.X) / (B.Y - A.Y))
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	FBox2D BoundsOf(const TArray<FVector2D>& Polygon)
	{
		FBox2D Box(ForceInit);
		for (const FVector2D& Corner : Polygon)
		{
			Box += Corner;
		}
		return Box;
	}

	TArray<FVector2D> Ellipse(const FVector2D& Center, const FVector2D& Radii, int32 Count)
	{
		TArray<FVector2D> Corners;
		const int32 Points = FMath::Max(Count, 3);
		Corners.Reserve(Points);
		for (int32 Index = 0; Index < Points; ++Index)
		{
			const double Angle = 2.0 * UE_DOUBLE_PI * Index / Points;
			Corners.Add(Center + FVector2D(FMath::Cos(Angle) * Radii.X, FMath::Sin(Angle) * Radii.Y));
		}
		return Corners;
	}
}
