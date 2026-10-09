#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterUIStyle.h"

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

	/** A headstone with a cross on it, standing on the ground: the respawn grave. */
	const FVectorIcon& GraveIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.StrokeWidth = 1.9f;
			TArray<FVector2D>& Stone = Result.Strokes.AddDefaulted_GetRef();
			Stone.Add(FVector2D(6.5, 20.0));
			constexpr int32 ArcSteps = 14;
			for (int32 Step = 0; Step <= ArcSteps; ++Step)
			{
				// Over the top from the left side to the right (angles from pi to 2 pi run up and over, y down).
				const double Angle = UE_PI + UE_PI * Step / ArcSteps;
				Stone.Add(FVector2D(12.0 + 5.5 * FMath::Cos(Angle), 10.0 + 5.5 * FMath::Sin(Angle)));
			}
			Stone.Add(FVector2D(17.5, 20.0));
			Result.Strokes.Add(TArray<FVector2D>{FVector2D(4.0, 20.5), FVector2D(20.0, 20.5) });
			Result.Strokes.Add(TArray<FVector2D>{FVector2D(12.0, 7.8), FVector2D(12.0, 16.6) });
			Result.Strokes.Add(TArray<FVector2D>{FVector2D(9.1, 10.8), FVector2D(14.9, 10.8) });
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

	/** A five-pointed star: a mission waiting to be turned in (the mission-complete banner's medal). */
	const FVectorIcon& StarIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			const FVector2D Center(12.0, 12.8);
			constexpr double Outer = 10.0;
			constexpr double Inner = 4.1;
			auto At = [&Center](double Degrees, double Radius)
			{
				const double Radians = FMath::DegreesToRadians(Degrees);
				return Center + Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
			};
			TArray<FVector2D> Middle;
			for (int32 Tip = 0; Tip < 5; ++Tip)
			{
				const double Angle = -90.0 + 72.0 * Tip;
				Result.Fills.Add(TArray<FVector2D>{At(Angle - 36.0, Inner), At(Angle, Outer), At(Angle + 36.0, Inner) });
				Middle.Add(At(Angle + 36.0, Inner));
			}
			Result.Fills.Add(MoveTemp(Middle));
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
		case EMapPinKind::TurnIn:      return LooterUI::Color::Better();
		case EMapPinKind::Grave:       return LooterUI::Color::CyanText();
		case EMapPinKind::Station:     return LooterUI::Color::Title();
		case EMapPinKind::Bench:       return LooterUI::Color::IconLight();
		case EMapPinKind::Chest:       return LooterUI::Color::AccentLight();
		case EMapPinKind::ChestLooted: return LooterUI::Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.75f);
		}
		return LooterUI::Color::Text();
	}

	float Size(EMapPinKind Kind)
	{
		switch (Kind)
		{
		case EMapPinKind::Objective:   return 22.f;
		case EMapPinKind::TurnIn:      return 24.f;
		case EMapPinKind::Grave:       return 26.f;
		case EMapPinKind::Station:     return 24.f;
		case EMapPinKind::Bench:       return 22.f;
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
		case EMapPinKind::TurnIn:      return LooterUI::IconBrush(TEXT("MapTurnIn"), StarIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Grave:       return LooterUI::IconBrush(TEXT("MapGrave"), GraveIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Station:     return LooterUI::IconBrush(TEXT("MapStation"), StationIcon(), PixelsPerUnit, Box, Tint);
		case EMapPinKind::Bench:       return LooterUI::IconBrush(TEXT("MapBench"), BenchIcon(), PixelsPerUnit, Box, Tint);
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
}
