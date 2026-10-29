#include "UI/Cards/GunCardIcons.h"

// The tag card's glyphs, drawn from the mockup's SVGs (mockup/make_compact.js: "Our own small glyphs for each stat") as the
// kit's vector icons: convex fills clockwise on screen, strokes as polylines, in a 16-unit box.

using LooterUI::FVectorIcon;
using LoadoutRules::EStat;

namespace
{
	TArray<FVector2D> Ring(float X, float Y, float R, int32 Steps = 20, float From = 0.f, float To = 360.f)
	{
		TArray<FVector2D> Points;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, static_cast<float>(Step) / Steps));
			Points.Add(FVector2D(X + R * FMath::Cos(Angle), Y + R * FMath::Sin(Angle)));
		}
		return Points;
	}

	FVectorIcon Make(float Stroke)
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(16.0, 16.0);
		Icon.StrokeWidth = Stroke;
		return Icon;
	}

	FVectorIcon MakeDamage()
	{
		// A starburst: four strokes crossing, and a small solid core.
		FVectorIcon Icon = Make(2.f);
		Icon.Strokes = { { {8, 1}, {8, 15} }, { {1, 8}, {15, 8} }, { {3, 3}, {13, 13} }, { {13, 3}, {3, 13} } };
		Icon.Fills = { Ring(8, 8, 3, 10) };
		return Icon;
	}

	FVectorIcon MakeAccuracy()
	{
		FVectorIcon Icon = Make(1.8f);
		Icon.Strokes = { Ring(8, 8, 5.2f, 24), { {8, 0}, {8, 4.5} }, { {8, 11.5}, {8, 16} }, { {0, 8}, {4.5, 8} }, { {11.5, 8}, {16, 8} } };
		return Icon;
	}

	FVectorIcon MakeHandling()
	{
		FVectorIcon Icon = Make(2.2f);
		Icon.Strokes = { { {2, 12}, {8, 3}, {14, 12} }, { {5, 14.5}, {11, 14.5} } };
		return Icon;
	}

	FVectorIcon MakeReload()
	{
		// Three quarters of a circle and the arrowhead that turns it.
		FVectorIcon Icon = Make(1.9f);
		Icon.Strokes = { Ring(8, 8.5f, 5.5f, 18, -90.f + 30.f, 270.f) };
		Icon.Fills = { { {6.5, 0.5}, {11, 3}, {6.5, 6} } };
		return Icon;
	}

	FVectorIcon MakeFireRate()
	{
		FVectorIcon Icon = Make(1.9f);
		Icon.Strokes = { { {1.5, 4}, {9.5, 4} }, { {1.5, 8}, {12.5, 8} }, { {1.5, 12}, {9.5, 12} } };
		Icon.Fills = { { {11.5, 2}, {15, 4}, {11.5, 6} }, { {11.5, 10}, {15, 12}, {11.5, 14} } };
		return Icon;
	}

	FVectorIcon MakeMagazine()
	{
		FVectorIcon Icon = Make(1.7f);
		Icon.Strokes = { { {5, 1.5}, {11, 1.5}, {11, 14.5}, {5, 14.5}, {5, 1.5} }, { {5, 5.5}, {11, 5.5} }, { {5, 9.5}, {11, 9.5} } };
		return Icon;
	}

	FVectorIcon MakeZoom()
	{
		FVectorIcon Icon = Make(1.8f);
		Icon.Strokes = { Ring(6.5f, 6.5f, 4.8f, 22), { {10.2, 10.2}, {15, 15} } };
		return Icon;
	}

	FVectorIcon MakeEmber()
	{
		// A flame: a tall teardrop and a smaller bright one inside (two convex fills; the tint colours both).
		FVectorIcon Icon = Make(1.f);
		TArray<FVector2D> Outer = { {8, 0.5} };
		for (int32 Step = 0; Step <= 12; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(-30.f + 240.f * Step / 12.f);
			Outer.Add(FVector2D(8.0 + 5.6 * FMath::Cos(Angle), 10.0 + 5.4 * FMath::Sin(Angle)));
		}
		Icon.Fills = { Outer };
		return Icon;
	}

	FVectorIcon MakeSoulLight()
	{
		// A four-point star: two thin diamonds crossed.
		FVectorIcon Icon = Make(1.f);
		Icon.Fills = { { {8, 0.5}, {10, 8}, {8, 15.5}, {6, 8} }, { {0.5, 8}, {8, 6}, {15.5, 8}, {8, 10} } };
		return Icon;
	}

	FVectorIcon MakeGraveSalt()
	{
		// A salt crystal: a diamond outline round a small solid one.
		FVectorIcon Icon = Make(1.8f);
		Icon.Strokes = { { {8, 1}, {15, 8}, {8, 15}, {1, 8}, {8, 1} } };
		Icon.Fills = { { {8, 4.5}, {11.5, 8}, {8, 11.5}, {4.5, 8} } };
		return Icon;
	}
}

const FVectorIcon& GunCardIcons::Stat(EStat Stat)
{
	static const FVectorIcon Damage = MakeDamage();
	static const FVectorIcon Accuracy = MakeAccuracy();
	static const FVectorIcon Handling = MakeHandling();
	static const FVectorIcon Reload = MakeReload();
	static const FVectorIcon FireRate = MakeFireRate();
	static const FVectorIcon Magazine = MakeMagazine();
	static const FVectorIcon Zoom = MakeZoom();
	switch (Stat)
	{
	case EStat::Damage: return Damage;
	case EStat::Accuracy: return Accuracy;
	case EStat::Handling: return Handling;
	case EStat::Reload: return Reload;
	case EStat::FireRate: return FireRate;
	case EStat::Magazine: return Magazine;
	default: return Zoom;
	}
}

FName GunCardIcons::StatName(EStat Stat)
{
	return FName(*FString::Printf(TEXT("GunCardStat_%s"), LoadoutRules::StatName(Stat)));
}

const FVectorIcon* GunCardIcons::Element(ELooterElement Element)
{
	static const FVectorIcon Ember = MakeEmber();
	static const FVectorIcon SoulLight = MakeSoulLight();
	static const FVectorIcon GraveSalt = MakeGraveSalt();
	switch (Element)
	{
	case ELooterElement::Ember: return &Ember;
	case ELooterElement::SoulLight: return &SoulLight;
	case ELooterElement::GraveSalt: return &GraveSalt;
	default: return nullptr;
	}
}

FName GunCardIcons::ElementName(ELooterElement Element)
{
	return FName(*FString::Printf(TEXT("GunCardElement_%d"), static_cast<int32>(Element)));
}

const FVectorIcon& GunCardIcons::Diamond()
{
	static const FVectorIcon Icon = []
	{
		FVectorIcon Made = Make(2.f);
		Made.Strokes = { { {8, 1}, {15, 8}, {8, 15}, {1, 8}, {8, 1} } };
		Made.Fills = { { {8, 5}, {11, 8}, {8, 11}, {5, 8} } };
		return Made;
	}();
	return Icon;
}
