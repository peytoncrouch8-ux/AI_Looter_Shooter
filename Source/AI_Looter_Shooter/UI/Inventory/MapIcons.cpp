#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/MissionMarkIcons.h"

using namespace LooterUI;

namespace
{
	/** Texture texels per view-box unit: a 24-unit icon is drawn into 72 px, crisp at the page's sizes on a 1080p screen. */
	constexpr float PixelsPerUnit = 3.f;

	/** A circle's outline as a polygon, clockwise on screen (y down), as the kit's fills want. */
	TArray<FVector2D> Circle(const FVector2D& Center, double Radius, int32 Sides = 24)
	{
		TArray<FVector2D> Points;
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Sides;
			Points.Add(Center + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		return Points;
	}

	/**
	 * Hob's roost, the same on the map, the minimap and its rim: the crow's-foot mark burnt in a board on every roost (look
	 * sheet 41), a bird's track: three toes spread up from the pad, the hind toe down. Four bold strokes and the pad: on the
	 * minimap's rim it is hardly ten pixels across and still reads.
	 */
	const FVectorIcon& RoostIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.StrokeWidth = 2.6f;
			const FVector2D Pad(12.0, 13.2);
			Result.Strokes.Add(TArray<FVector2D>{Pad, FVector2D(12.0, 2.4) });
			Result.Strokes.Add(TArray<FVector2D>{Pad, FVector2D(4.4, 5.0) });
			Result.Strokes.Add(TArray<FVector2D>{Pad, FVector2D(19.6, 5.0) });
			Result.Strokes.Add(TArray<FVector2D>{Pad, FVector2D(12.0, 21.8) });
			Result.Fills.Add(Circle(Pad, 2.4, 16));
			return Result;
		}();
		return Icon;
	}

	/** A locomotive on its rail: where trips leave from. */
	const FVectorIcon& StationIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.StrokeWidth = 1.3f;
			Result.Fills.Add(TArray<FVector2D>{FVector2D(5.0, 10.0), FVector2D(15.0, 10.0), FVector2D(15.0, 15.0), FVector2D(5.0, 15.0) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(14.5, 6.0), FVector2D(20.5, 6.0), FVector2D(20.5, 15.0), FVector2D(14.5, 15.0) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(13.5, 4.8), FVector2D(21.5, 4.8), FVector2D(21.5, 6.3), FVector2D(13.5, 6.3) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(6.4, 5.2), FVector2D(10.1, 5.2), FVector2D(9.3, 10.0), FVector2D(7.2, 10.0) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(5.0, 12.4), FVector2D(5.0, 16.4), FVector2D(1.6, 16.4) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(3.0, 15.0), FVector2D(21.5, 15.0), FVector2D(21.5, 16.4), FVector2D(3.0, 16.4) });
			Result.Fills.Add(Circle(FVector2D(8.2, 18.0), 2.3));
			Result.Fills.Add(Circle(FVector2D(16.8, 18.0), 2.3));
			Result.Strokes.Add(TArray<FVector2D>{FVector2D(1.0, 21.2), FVector2D(23.0, 21.2) });
			return Result;
		}();
		return Icon;
	}

	/** An anvil: the gunsmith's bench. */
	const FVectorIcon& BenchIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.Fills.Add(TArray<FVector2D>{FVector2D(1.5, 8.2), FVector2D(7.0, 7.0), FVector2D(7.0, 10.6) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(7.0, 7.0), FVector2D(21.5, 7.0), FVector2D(21.5, 10.6), FVector2D(7.0, 10.6) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(10.0, 10.6), FVector2D(18.0, 10.6), FVector2D(16.3, 15.2), FVector2D(11.7, 15.2) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(9.8, 15.2), FVector2D(18.2, 15.2), FVector2D(20.6, 19.0), FVector2D(7.4, 19.0) });
			return Result;
		}();
		return Icon;
	}

	/**
	 * A small stele with a candle burning on it: the vigil stone (a peaked slab on a plinth, a candle stub and its flame). The
	 * shapes are bold and few: in a badge the icon is hardly fourteen pixels across. Every fill is convex.
	 */
	const FVectorIcon& ShrineIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			// The flame: a teardrop, its tip up.
			Result.Fills.Add(TArray<FVector2D>{FVector2D(12.0, 0.8), FVector2D(14.2, 3.9), FVector2D(14.7, 5.8), FVector2D(13.5, 7.5),
				FVector2D(12.0, 8.1), FVector2D(10.5, 7.5), FVector2D(9.3, 5.8), FVector2D(9.8, 3.9) });
			// The candle stub, standing in the stele's peak.
			Result.Fills.Add(TArray<FVector2D>{FVector2D(10.9, 8.1), FVector2D(13.1, 8.1), FVector2D(13.1, 11.0), FVector2D(10.9, 11.0) });
			// The stele: a slab with sloped shoulders.
			Result.Fills.Add(TArray<FVector2D>{FVector2D(7.6, 19.0), FVector2D(7.6, 13.2), FVector2D(12.0, 10.4), FVector2D(16.4, 13.2),
				FVector2D(16.4, 19.0) });
			// The plinth it stands on.
			Result.Fills.Add(TArray<FVector2D>{FVector2D(4.6, 19.0), FVector2D(19.4, 19.0), FVector2D(19.4, 22.0), FVector2D(4.6, 22.0) });
			return Result;
		}();
		return Icon;
	}

	/** A chest, its lid apart from its body by a dark seam. */
	const FVectorIcon& ChestIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.Fills.Add(TArray<FVector2D>{FVector2D(5.0, 5.8), FVector2D(19.0, 5.8), FVector2D(20.5, 7.9), FVector2D(20.5, 10.2), FVector2D(3.5, 10.2),
				FVector2D(3.5, 7.9) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(3.5, 11.5), FVector2D(10.6, 11.5), FVector2D(10.6, 19.5), FVector2D(3.5, 19.5) });
			Result.Fills.Add(TArray<FVector2D>{FVector2D(13.4, 11.5), FVector2D(20.5, 11.5), FVector2D(20.5, 19.5), FVector2D(13.4, 19.5) });
			// The hasp between the body's halves.
			Result.Fills.Add(TArray<FVector2D>{FVector2D(10.6, 12.6), FVector2D(13.4, 12.6), FVector2D(13.4, 15.6), FVector2D(10.6, 15.6) });
			return Result;
		}();
		return Icon;
	}

	/** A five-pointed star, top point up, as fills (its five tips and the pentagon between them: each one convex). */
	void AddStar(FVectorIcon& Icon, const FVector2D& Center, double Outer, double Inner)
	{
		auto At = [&Center](double Degrees, double Radius)
		{
			const double Radians = FMath::DegreesToRadians(Degrees);
			return Center + Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
		};
		TArray<FVector2D> Middle;
		for (int32 Tip = 0; Tip < 5; ++Tip)
		{
			const double Angle = -90.0 + 72.0 * Tip;
			Icon.Fills.Add(TArray<FVector2D>{At(Angle - 36.0, Inner), At(Angle, Outer), At(Angle + 36.0, Inner) });
			Middle.Add(At(Angle + 36.0, Inner));
		}
		Icon.Fills.Add(MoveTemp(Middle));
	}

	/** A coin: a heavy rim round a small star struck in its middle, as the silver coin beside every amount has it. A store. */
	const FVectorIcon& VendorIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.StrokeWidth = 2.2f;
			TArray<FVector2D>& Rim = Result.Strokes.AddDefaulted_GetRef();
			constexpr int32 Sides = 32;
			for (int32 Step = 0; Step <= Sides; ++Step)
			{
				const double Angle = UE_TWO_PI * Step / Sides;
				Rim.Add(FVector2D(12.0 + 9.3 * FMath::Cos(Angle), 12.0 + 9.3 * FMath::Sin(Angle)));
			}
			AddStar(Result, FVector2D(12.0, 12.6), 5.4, 2.3);
			return Result;
		}();
		return Icon;
	}

	/** The waypoint's ring round a dot, as on the HUD's minimap. */
	const FVectorIcon& ObjectiveIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.StrokeWidth = 2.5f;
			TArray<FVector2D>& Ring = Result.Strokes.AddDefaulted_GetRef();
			constexpr int32 Sides = 32;
			for (int32 Step = 0; Step <= Sides; ++Step)
			{
				const double Angle = UE_TWO_PI * Step / Sides;
				Ring.Add(FVector2D(12.0 + 9.5 * FMath::Cos(Angle), 12.0 + 9.5 * FMath::Sin(Angle)));
			}
			Result.Fills.Add(Circle(FVector2D(12.0, 12.0), 4.0, 20));
			return Result;
		}();
		return Icon;
	}
}

namespace MapIcons
{
	FLinearColor Color(EMapPinKind Kind)
	{
		switch (Kind)
		{
		case EMapPinKind::Objective:   return LooterUI::Color::Accent();
		case EMapPinKind::Offer:       return MissionMarkIcons::Color(true);
		case EMapPinKind::TurnIn:      return MissionMarkIcons::Color(true);
		// Hob's roost in its beacon's own colour, the one the world shows (URoostSettings::BeaconColor).
		case EMapPinKind::Grave:       return LooterUI::Color::Roost();
		case EMapPinKind::Station:     return LooterUI::Color::Title();
		case EMapPinKind::Vendor:      return LooterUI::Color::IconShade();
		case EMapPinKind::Bench:       return LooterUI::Color::IconLight();
		// The vigil stone's candlelit brass (the cursed irons' tarnished gold): apart from the chest's orange-gold and the bench's ivory.
		case EMapPinKind::Shrine:      return LooterUI::Color::Curse();
		case EMapPinKind::Chest:      return LooterUI::Color::AccentLight();
		case EMapPinKind::ChestLooted: return LooterUI::Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.75f);
		}
		return LooterUI::Color::Text();
	}

	float Size(EMapPinKind Kind)
	{
		switch (Kind)
		{
		case EMapPinKind::Objective:   return 22.f;
		case EMapPinKind::Offer:       return 24.f;
		case EMapPinKind::TurnIn:      return 24.f;
		case EMapPinKind::Grave:       return 26.f;
		case EMapPinKind::Station:     return 24.f;
		case EMapPinKind::Vendor:      return 24.f;
		case EMapPinKind::Bench:       return 22.f;
		case EMapPinKind::Shrine:      return 26.f;
		case EMapPinKind::Chest:       return 20.f;
		case EMapPinKind::ChestLooted: return 13.f;
		}
		return 20.f;
	}

	bool HasBadge(EMapPinKind Kind)
	{
		return Kind != EMapPinKind::Objective && Kind != EMapPinKind::ChestLooted;
	}

	FSlateBrush IconBrush(EMapPinKind Kind, float IconSize, const FLinearColor& Tint)
	{
		const FVector2D Box(IconSize, IconSize);
		switch (Kind)
		{
		case EMapPinKind::Objective:   return LooterUI::IconBrush(TEXT("MapObjective"), ObjectiveIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Offer:       return MissionMarkIcons::MarkBrush(MissionMarkIcons::EMark::Offer, true, Box);
		case EMapPinKind::TurnIn:      return MissionMarkIcons::MarkBrush(MissionMarkIcons::EMark::TurnIn, true, Box);
		case EMapPinKind::Grave:       return RoostBrush(IconSize, Tint);
		case EMapPinKind::Station:     return LooterUI::IconBrush(TEXT("MapStation"), StationIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Vendor:      return LooterUI::IconBrush(TEXT("MapVendor"), VendorIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Bench:       return LooterUI::IconBrush(TEXT("MapBench"), BenchIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Shrine:      return LooterUI::IconBrush(TEXT("MapShrine"), ShrineIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Chest:
		case EMapPinKind::ChestLooted: return LooterUI::IconBrush(TEXT("MapChest"), ChestIcon(), PixelsPerUnit, Box, Tint);
		}
		return LooterUI::IconBrush(TEXT("MapObjective"), ObjectiveIcon(), PixelsPerUnit, Box, Tint);
	}

	FSlateBrush BadgeBrush(const FLinearColor& Ring, bool bLit)
	{
		const FLinearColor Glass = LooterUI::Color::ScreenBg();
		return CircleBrush(FLinearColor(Glass.R, Glass.G, Glass.B, 0.9f), Ring, bLit ? 2.2f : 1.4f);
	}

	FSlateBrush RoostBrush(float IconSize, const FLinearColor& Tint)
	{
		// Small on the minimap, so the texture's resolution follows the size it's drawn at.
		const float Pixels = IconSize < 16.f ? 4.f : PixelsPerUnit;
		return LooterUI::IconBrush(TEXT("MapRoost"), RoostIcon(), Pixels, FVector2D(IconSize, IconSize), Tint);
	}
}
