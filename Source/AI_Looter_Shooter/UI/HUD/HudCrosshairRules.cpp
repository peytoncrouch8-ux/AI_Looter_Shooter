// The crosshair shapes per gun kind, from the mockup's make.js (Docs/Work/2026-10-28-Core/crosshair), and the numbers the
// HUD runs them with. A shape's words in the mockup say why it looks so: they are kept beside each.

#include "UI/HUD/HudCrosshairRules.h"
#include "UI/Style/LooterUIStyle.h"

namespace
{
	using namespace HudCrosshair;

	const FVector2f PushUp(0.f, -1.f);
	const FVector2f PushDown(0.f, 1.f);
	const FVector2f PushLeft(-1.f, 0.f);
	const FVector2f PushRight(1.f, 0.f);

	FStroke Line(const FVector2f& From, const FVector2f& To, float Width, const FVector2f& Push)
	{
		FStroke Stroke;
		Stroke.Points = { From, To };
		Stroke.Width = Width;
		Stroke.Push = Push;
		return Stroke;
	}

	FStroke Path(TArray<FVector2f> Points, float Width, const FVector2f& Push, bool bClosed = false)
	{
		FStroke Stroke;
		Stroke.Points = MoveTemp(Points);
		Stroke.Width = Width;
		Stroke.Push = Push;
		Stroke.bClosed = bClosed;
		return Stroke;
	}

	/** Four ticks on the axes from Inner to Outer, each pushed out along its own axis as the gap opens. */
	void AddCross(FShape& Shape, float Inner, float Outer, float Width)
	{
		Shape.Strokes.Add(Line(FVector2f(0.f, -Outer), FVector2f(0.f, -Inner), Width, PushUp));
		Shape.Strokes.Add(Line(FVector2f(0.f, Inner), FVector2f(0.f, Outer), Width, PushDown));
		Shape.Strokes.Add(Line(FVector2f(-Outer, 0.f), FVector2f(-Inner, 0.f), Width, PushLeft));
		Shape.Strokes.Add(Line(FVector2f(Inner, 0.f), FVector2f(Outer, 0.f), Width, PushRight));
	}

	FShape MakeShape(EWeaponKind Kind)
	{
		FShape Shape;
		switch (Kind)
		{
		case EWeaponKind::SMG:
			// "A tight dot and four short ticks: fast, light, close."
			Shape.Dot.Radius = 1.6f;
			AddCross(Shape, 9.f, 14.f, 2.f);
			break;
		case EWeaponKind::Rifle:
			// "The classic cross with a gap that opens as spread grows." No dot from the hip, as drawn.
			AddCross(Shape, 8.f, 22.f, 2.f);
			Shape.bHipDot = false;
			break;
		case EWeaponKind::LMG:
		{
			// "Four corner brackets round a dot: wide, steady, built for long bursts." Each corner opens along its diagonal.
			Shape.Dot.Radius = 1.8f;
			const float Diagonal = UE_INV_SQRT_2;
			Shape.Strokes.Add(Path({ FVector2f(-18.f, -10.f), FVector2f(-18.f, -18.f), FVector2f(-10.f, -18.f) }, 2.f, FVector2f(-Diagonal, -Diagonal)));
			Shape.Strokes.Add(Path({ FVector2f(18.f, -10.f), FVector2f(18.f, -18.f), FVector2f(10.f, -18.f) }, 2.f, FVector2f(Diagonal, -Diagonal)));
			Shape.Strokes.Add(Path({ FVector2f(-18.f, 10.f), FVector2f(-18.f, 18.f), FVector2f(-10.f, 18.f) }, 2.f, FVector2f(-Diagonal, Diagonal)));
			Shape.Strokes.Add(Path({ FVector2f(18.f, 10.f), FVector2f(18.f, 18.f), FVector2f(10.f, 18.f) }, 2.f, FVector2f(Diagonal, Diagonal)));
			break;
		}
		case EWeaponKind::Revolver:
			// "A sharp chevron over a dot: one precise, heavy round." The chevron rises off the dot as the spread opens.
			Shape.Dot.At = FVector2f(0.f, 3.f);
			Shape.Dot.Radius = 2.f;
			Shape.Strokes.Add(Path({ FVector2f(-12.f, -4.f), FVector2f(0.f, -14.f), FVector2f(12.f, -4.f) }, 2.4f, PushUp));
			break;
		case EWeaponKind::Shotgun:
		{
			// "A broken ring the size of the pellet spread: where the pellets land."
			Shape.Dot.Radius = 1.6f;
			FRing Ring;
			Ring.Radius = 24.f;
			Ring.Width = 2.2f;
			Shape.Rings.Add(Ring);
			break;
		}
		case EWeaponKind::Sniper:
			// "Thin long hairs and an open middle: unsteady from the hip, aim it."
			Shape.Dot.Radius = 1.2f;
			AddCross(Shape, 14.f, 30.f, 1.4f);
			break;
		case EWeaponKind::Launcher:
			// "A diamond and drop marks below it: the rocket's arc, its blast." One rocket has no spread to show: it stays put.
			Shape.Strokes.Add(Path({ FVector2f(0.f, -12.f), FVector2f(12.f, 0.f), FVector2f(0.f, 12.f), FVector2f(-12.f, 0.f) }, 2.2f,
				FVector2f::ZeroVector, /*bClosed*/ true));
			Shape.Strokes.Add(Line(FVector2f(-6.f, 20.f), FVector2f(6.f, 20.f), 2.f, FVector2f::ZeroVector));
			Shape.Strokes.Add(Line(FVector2f(-4.f, 27.f), FVector2f(4.f, 27.f), 2.f, FVector2f::ZeroVector));
			Shape.Strokes.Add(Line(FVector2f(-2.f, 34.f), FVector2f(2.f, 34.f), 2.f, FVector2f::ZeroVector));
			Shape.bHipDot = false;
			break;
		case EWeaponKind::None:
		default:
			// The old cross: 9 px ticks with a 2 px core, 3 px off the middle at rest, round a 3 px dot.
			Shape.Dot.Radius = 1.2f;
			AddCross(Shape, 3.f / UnitScale, 12.f / UnitScale, 2.f / UnitScale);
			break;
		}
		return Shape;
	}

	/** The shapes by EWeaponKind's value (None to Launcher), made once. */
	constexpr int32 NumKinds = static_cast<int32>(EWeaponKind::Launcher) + 1;
}

float HudCrosshair::GapBoxFor(float Spread)
{
	return FMath::Clamp(GapBoxBase + Spread * GapBoxPerDegree, GapBoxMin, GapBoxMax);
}

float HudCrosshair::OpenOf(float GapBox)
{
	return FMath::Clamp((GapBox - GapBoxMin) * 0.5f, 0.f, MaxOpen);
}

const HudCrosshair::FShape& HudCrosshair::ShapeFor(EWeaponKind Kind)
{
	static const TArray<FShape> Shapes = []()
	{
		TArray<FShape> All;
		for (int32 Index = 0; Index < NumKinds; ++Index)
		{
			All.Add(MakeShape(static_cast<EWeaponKind>(Index)));
		}
		return All;
	}();
	const int32 Index = static_cast<int32>(Kind);
	return Shapes.IsValidIndex(Index) ? Shapes[Index] : Shapes[0];
}

float HudCrosshair::ReachOf(const FShape& Shape, float Open)
{
	float Reach = 0.f;
	for (const FStroke& Stroke : Shape.Strokes)
	{
		const float Edge = (Stroke.Width + OutlineExtra) * UnitScale * 0.5f;
		for (const FVector2f& Point : Stroke.Points)
		{
			Reach = FMath::Max(Reach, (Point * UnitScale + Stroke.Push * Open).Size() + Edge);
		}
	}
	for (const FRing& Ring : Shape.Rings)
	{
		Reach = FMath::Max(Reach, Ring.Radius * UnitScale + Open + (Ring.Width + OutlineExtra) * UnitScale * 0.5f);
	}
	return FMath::Max(Reach, (Shape.Dot.At.Size() + Shape.Dot.Radius + DotOutlineExtra) * UnitScale);
}

float HudCrosshair::OpenLimitFor(const FShape& Shape)
{
	if (ReachOf(Shape, MaxOpen) <= MaxReach)
	{
		return MaxOpen;
	}
	if (ReachOf(Shape, 0.f) >= MaxReach)
	{
		return 0.f;
	}
	// Reach only grows as the gap opens: halve the way to the most that stays inside.
	float Low = 0.f;
	float High = MaxOpen;
	for (int32 Step = 0; Step < 20; ++Step)
	{
		const float Mid = (Low + High) * 0.5f;
		if (ReachOf(Shape, Mid) <= MaxReach)
		{
			Low = Mid;
		}
		else
		{
			High = Mid;
		}
	}
	return Low;
}

HudCrosshair::FKick HudCrosshair::KickFor(EWeaponKind Kind)
{
	switch (Kind)
	{
	case EWeaponKind::SMG:      return { 1.1f, 0.08f };
	case EWeaponKind::Rifle:    return { 1.3f, 0.12f };
	case EWeaponKind::LMG:      return { 1.2f, 0.1f };
	case EWeaponKind::Revolver: return { 1.5f, 0.16f };
	case EWeaponKind::Shotgun:  return { 1.55f, 0.2f };
	case EWeaponKind::Sniper:   return { 1.6f, 0.22f };
	case EWeaponKind::Launcher: return { 1.6f, 0.25f };
	case EWeaponKind::None:
	default:                    return { 1.45f, 0.16f };
	}
}

float HudCrosshair::KickScaleAt(const FKick& Kick, float Age)
{
	if (Kick.Seconds <= 0.f)
	{
		return 1.f;
	}
	const float Settled = FMath::Clamp(Age / Kick.Seconds, 0.f, 1.f);
	const float Remaining = 1.f - FMath::InterpEaseOut(0.f, 1.f, Settled, 2.f);
	return 1.f + (Kick.Scale - 1.f) * Remaining;
}

float HudCrosshair::SwitchFadeAt(float Age)
{
	return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Age / SwitchSeconds, 0.f, 1.f), 2.f);
}

float HudCrosshair::SwitchScaleAt(float Age)
{
	return FMath::Lerp(SwitchFromScale, 1.f, SwitchFadeAt(Age));
}

FLinearColor HudCrosshair::ColourOf(ECrosshairColour Colour)
{
	using namespace LooterUI;
	switch (Colour)
	{
	case ECrosshairColour::Salt:  return Color::SaltLine();
	case ECrosshairColour::Gold:  return Color::AccentLight();
	case ECrosshairColour::Cyan:  return Color::CyanText();
	case ECrosshairColour::Green: return Color::Better();
	case ECrosshairColour::White:
	default:
		// The mockup's #f2f6f8: the tag card's text white.
		return Color::SlateText();
	}
}

FLinearColor HudCrosshair::EdgeColour()
{
	// The mockup's edge is near-black at 85%; the HUD's floating text edge is the same weight in its dark blue.
	return LooterUI::Color::Outline();
}
