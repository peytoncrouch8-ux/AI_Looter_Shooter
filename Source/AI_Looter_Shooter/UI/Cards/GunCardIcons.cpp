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

	/** A white picture in the 24-unit box from even-odd polygons (a polygon inside another cuts a hole), for the tint. */
	LooterUI::FPaintedIcon Glyph(TArray<TArray<FVector2D>> Fills)
	{
		LooterUI::FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(24.0, 24.0);
		LooterUI::FPaintLayer Layer;
		Layer.Fills = MoveTemp(Fills);
		Icon.Layers.Add(MoveTemp(Layer));
		return Icon;
	}

	/** Ember, "Hearth": a round belly rising to one curling tip, a small flame cut from its heart. */
	LooterUI::FPaintedIcon MakeEmber()
	{
		return Glyph({ { {12, 23}, {10.45, 22.9}, {9.06, 22.61}, {7.82, 22.15}, {6.75, 21.52}, {5.85, 20.75}, {5.13, 19.85}, {4.6, 18.83}, {4.26, 17.7}, {4.13, 16.49}, {4.2, 15.2}, {4.47, 14.06}, {4.89, 13.03}, {5.44, 12.08}, {6.06, 11.17}, {6.72, 10.3}, {7.4, 9.43}, {8.04, 8.52}, {8.61, 7.57}, {9.08, 6.54}, {9.4, 5.4}, {9.5, 4.87}, {9.55, 4.37}, {9.55, 3.88}, {9.52, 3.42}, {9.45, 2.98}, {9.34, 2.55}, {9.2, 2.14}, {9.03, 1.75}, {8.83, 1.37}, {8.6, 1}, {10.28, 1.55}, {11.94, 2.34}, {13.53, 3.36}, {15.03, 4.59}, {16.4, 6}, {17.6, 7.58}, {18.6, 9.31}, {19.35, 11.17}, {19.83, 13.14}, {20, 15.2}, {19.91, 16.54}, {19.63, 17.78}, {19.18, 18.91}, {18.57, 19.93}, {17.8, 20.83}, {16.89, 21.58}, {15.84, 22.18}, {14.68, 22.63}, {13.39, 22.91} },
			{ {12, 21}, {11.26, 20.95}, {10.59, 20.8}, {10, 20.56}, {9.49, 20.24}, {9.08, 19.85}, {8.75, 19.4}, {8.51, 18.9}, {8.37, 18.36}, {8.33, 17.79}, {8.4, 17.2}, {8.57, 16.63}, {8.81, 16.1}, {9.12, 15.59}, {9.47, 15.1}, {9.85, 14.6}, {10.24, 14.08}, {10.63, 13.51}, {11, 12.89}, {11.33, 12.19}, {11.6, 11.4}, {12.21, 11.95}, {12.82, 12.51}, {13.43, 13.08}, {14, 13.67}, {14.53, 14.27}, {14.98, 14.9}, {15.36, 15.54}, {15.63, 16.21}, {15.79, 16.89}, {15.8, 17.6}, {15.7, 18.23}, {15.52, 18.8}, {15.27, 19.3}, {14.95, 19.75}, {14.58, 20.13}, {14.14, 20.44}, {13.67, 20.68}, {13.14, 20.86}, {12.59, 20.96} } });
	}

	/** Soul-light, "Sunburst": a bright core throwing eight rays, the four straight ones longer. */
	LooterUI::FPaintedIcon MakeSoulLight()
	{
		return Glyph({ { {16.2, 12}, {16.14, 12.73}, {15.95, 13.44}, {15.64, 14.1}, {15.22, 14.7}, {14.7, 15.22}, {14.1, 15.64}, {13.44, 15.95}, {12.73, 16.14}, {12, 16.2}, {11.27, 16.14}, {10.56, 15.95}, {9.9, 15.64}, {9.3, 15.22}, {8.78, 14.7}, {8.36, 14.1}, {8.05, 13.44}, {7.86, 12.73}, {7.8, 12}, {7.86, 11.27}, {8.05, 10.56}, {8.36, 9.9}, {8.78, 9.3}, {9.3, 8.78}, {9.9, 8.36}, {10.56, 8.05}, {11.27, 7.86}, {12, 7.8}, {12.73, 7.86}, {13.44, 8.05}, {14.1, 8.36}, {14.7, 8.78}, {15.22, 9.3}, {15.64, 9.9}, {15.95, 10.56}, {16.14, 11.27} },
			{ {18, 13.4}, {23.6, 12}, {18, 10.6} },
			{ {15.46, 17.02}, {19.07, 19.07}, {17.02, 15.46} },
			{ {10.6, 18}, {12, 23.6}, {13.4, 18} },
			{ {6.98, 15.46}, {4.93, 19.07}, {8.54, 17.02} },
			{ {6, 10.6}, {0.4, 12}, {6, 13.4} },
			{ {8.54, 6.98}, {4.93, 4.93}, {6.98, 8.54} },
			{ {13.4, 6}, {12, 0.4}, {10.6, 6} },
			{ {17.02, 8.54}, {19.07, 4.93}, {15.46, 6.98} } });
	}

	/** Grave-salt, "Salt Cluster": three salt crystals growing up from the ground. */
	LooterUI::FPaintedIcon MakeGraveSalt()
	{
		return Glyph({ { {10, 22.4}, {10, 7}, {12, 2.6}, {14, 7}, {14, 22.4} },
			{ {4.6, 22.4}, {4, 13.4}, {5.8, 10.2}, {8.2, 12.6}, {8.8, 22.4} },
			{ {15.2, 22.4}, {15.8, 11.6}, {18.2, 8.8}, {20, 12.4}, {19.4, 22.4} } });
	}

	/** No element, "Empty Ring": a hollow ring, nothing loaded. */
	LooterUI::FPaintedIcon MakeNormal()
	{
		return Glyph({ { {21.4, 12}, {21.28, 13.47}, {20.94, 14.9}, {20.38, 16.27}, {19.6, 17.53}, {18.65, 18.65}, {17.53, 19.6}, {16.27, 20.38}, {14.9, 20.94}, {13.47, 21.28}, {12, 21.4}, {10.53, 21.28}, {9.1, 20.94}, {7.73, 20.38}, {6.47, 19.6}, {5.35, 18.65}, {4.4, 17.53}, {3.62, 16.27}, {3.06, 14.9}, {2.72, 13.47}, {2.6, 12}, {2.72, 10.53}, {3.06, 9.1}, {3.62, 7.73}, {4.4, 6.47}, {5.35, 5.35}, {6.47, 4.4}, {7.73, 3.62}, {9.1, 3.06}, {10.53, 2.72}, {12, 2.6}, {13.47, 2.72}, {14.9, 3.06}, {16.27, 3.62}, {17.53, 4.4}, {18.65, 5.35}, {19.6, 6.47}, {20.38, 7.73}, {20.94, 9.1}, {21.28, 10.53} },
			{ {18.8, 12}, {18.72, 13.06}, {18.47, 14.1}, {18.06, 15.09}, {17.5, 16}, {16.81, 16.81}, {16, 17.5}, {15.09, 18.06}, {14.1, 18.47}, {13.06, 18.72}, {12, 18.8}, {10.94, 18.72}, {9.9, 18.47}, {8.91, 18.06}, {8, 17.5}, {7.19, 16.81}, {6.5, 16}, {5.94, 15.09}, {5.53, 14.1}, {5.28, 13.06}, {5.2, 12}, {5.28, 10.94}, {5.53, 9.9}, {5.94, 8.91}, {6.5, 8}, {7.19, 7.19}, {8, 6.5}, {8.91, 5.94}, {9.9, 5.53}, {10.94, 5.28}, {12, 5.2}, {13.06, 5.28}, {14.1, 5.53}, {15.09, 5.94}, {16, 6.5}, {16.81, 7.19}, {17.5, 8}, {18.06, 8.91}, {18.47, 9.9}, {18.72, 10.94} } });
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

bool GunCardIcons::ElementBrush(ELooterElement Element, const FVector2D& Size, const FLinearColor& Tint, FSlateBrush& OutBrush)
{
	static const LooterUI::FPaintedIcon Ember = MakeEmber();
	static const LooterUI::FPaintedIcon SoulLight = MakeSoulLight();
	static const LooterUI::FPaintedIcon GraveSalt = MakeGraveSalt();
	static const LooterUI::FPaintedIcon Normal = MakeNormal();
	const LooterUI::FPaintedIcon* Icon = &Normal;
	FName Name = TEXT("GunCardElement_Normal");
	switch (Element)
	{
	case ELooterElement::Ember: Icon = &Ember; Name = TEXT("GunCardElement_Ember"); break;
	case ELooterElement::SoulLight: Icon = &SoulLight; Name = TEXT("GunCardElement_SoulLight"); break;
	case ELooterElement::GraveSalt: Icon = &GraveSalt; Name = TEXT("GunCardElement_GraveSalt"); break;
	default: break;
	}
	// Its texture at about twice the drawn size, shared by sizes that round alike.
	const float PixelsPerUnit = FMath::RoundToFloat(static_cast<float>(Size.Y) * 2.f / 24.f * 4.f) / 4.f;
	OutBrush = LooterUI::PaintedIconBrush(Name, *Icon, FMath::Max(PixelsPerUnit, 1.f), Size, Tint);
	return Element != ELooterElement::None;
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
