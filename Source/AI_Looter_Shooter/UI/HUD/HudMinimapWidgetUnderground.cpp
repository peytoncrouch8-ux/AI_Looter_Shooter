// The minimap underground: inside a space marked Underground (the Providence's galleries, Tilly's cellar) the top-down bake
// shows only the ground overhead, so a dark plate marked UNDERGROUND fades in over it and the markers (creatures, givers,
// loot, the waypoint) go on over the plate, the player's arrow in the middle as ever.

#include "UI/HUD/HudMinimapWidget.h"
#include "Audio/LooterSound.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

#define LOCTEXT_NAMESPACE "HudMinimap"

using namespace LooterUI;

const FName UHudMinimapWidget::UndergroundTag(TEXT("Underground"));

namespace
{
	/** How often the underground spaces are looked for again (s): they are placed with the level and never move, so the walk over every actor is rare. */
	constexpr float UndergroundLookSeconds = 10.f;
	/** The plate's fade in or out (s): a breath, so a doorway's edge never flickers. */
	constexpr float UndergroundFadeSeconds = 0.45f;
	/** The word sits this far up from the middle (a share of the radius), clear of the player's arrow. */
	constexpr float LabelRise = 0.5f;
	constexpr int32 LabelSize = 10;
	constexpr int32 LabelSpacing = 333;
	constexpr float PlateBox = 172.f;

	TArray<FVector2D> PlateCircle(const FVector2D& Center, float Radius)
	{
		constexpr int32 Sides = 96;
		TArray<FVector2D> Points;
		Points.Reserve(Sides + 1);
		for (int32 Step = 0; Step <= Sides; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Sides;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	/** Faint rings and a cross through the middle, like a surveyor's plan of a working: there's a map, just not this one. */
	const FPaintedIcon& PlateLines()
	{
		static const FPaintedIcon Picture = []
		{
			const FVector2D Center(PlateBox * 0.5f);
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(PlateBox);
			FPaintLayer Rings;
			for (const float Share : { 0.3f, 0.6f, 0.88f })
			{
				Rings.Strokes.Add(PlateCircle(Center, PlateBox * 0.5f * Share));
			}
			Rings.StrokeWidth = 1.f;
			Rings.Color = Color::Hairline();
			Rings.Opacity = 0.16f;
			Result.Layers.Add(Rings);
			FPaintLayer Cross;
			Cross.Strokes.Add({ FVector2D(Center.X, 8.f), FVector2D(Center.X, PlateBox - 8.f) });
			Cross.Strokes.Add({ FVector2D(8.f, Center.Y), FVector2D(PlateBox - 8.f, Center.Y) });
			Cross.StrokeWidth = 1.f;
			Cross.Color = Color::Hairline();
			Cross.Opacity = 0.1f;
			Result.Layers.Add(Cross);
			return Result;
		}();
		return Picture;
	}
}

bool UHudMinimapWidget::IsUnderground(const TArray<FBox>& Zones, const FVector& Where)
{
	return Zones.ContainsByPredicate([&Where](const FBox& Zone) { return Zone.IsValid && Zone.IsInsideOrOn(Where); });
}

void UHudMinimapWidget::GatherUndergroundZones(const UWorld* World, TArray<FBox>& OutZones)
{
	OutZones.Reset();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(UndergroundTag))
		{
			continue;
		}
		// Its shape's bounds, colliding or not (a trigger box's, a volume's brush).
		const FBox Zone = It->GetComponentsBoundingBox(/*bNonColliding*/ true);
		if (Zone.IsValid)
		{
			OutZones.Add(Zone);
		}
	}
}

void UHudMinimapWidget::BuildUnderground(UOverlay* Stack)
{
	if (!WidgetTree || !Stack)
	{
		return;
	}
	UOverlay* Plate = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// The dark under the ground: a background, so it fades with the UI transparency setting like the map's glass.
	const FLinearColor Ink = Color::Ink();
	UImage* Disc = MakeImage(WidgetTree, CircleBrush(FLinearColor(Ink.R, Ink.G, Ink.B, 0.93f)));
	MarkBackground(Disc);
	FillOverlaySlot(Plate->AddChildToOverlay(Disc));
	FillOverlaySlot(Plate->AddChildToOverlay(MakeImage(WidgetTree, PaintedIconBrush(TEXT("MinimapUndergroundLines"), PlateLines(), 2.f,
		FVector2D(PlateBox)))));
	UndergroundLabel = MakeFloatingText(WidgetTree, LabelSize, Color::CyanText(), LabelSpacing, ETextJustify::Center);
	UndergroundLabel->SetText(LOCTEXT("Underground", "UNDERGROUND"));
	UOverlaySlot* LabelSlot = Plate->AddChildToOverlay(UndergroundLabel);
	LabelSlot->SetHorizontalAlignment(HAlign_Center);
	LabelSlot->SetVerticalAlignment(VAlign_Center);
	Plate->SetVisibility(ESlateVisibility::Collapsed);
	Plate->SetRenderOpacity(0.f);
	FillOverlaySlot(Stack->AddChildToOverlay(Plate));
	UndergroundPlate = Plate;
}

void UHudMinimapWidget::UpdateUnderground(const FVector& PlayerLocation, float DeltaSeconds)
{
	UndergroundLookLeft -= DeltaSeconds;
	if (UndergroundLookLeft <= 0.f)
	{
		UndergroundLookLeft = UndergroundLookSeconds;
		GatherUndergroundZones(GetWorld(), UndergroundZones);
	}
	const bool bBelow = IsUnderground(UndergroundZones, PlayerLocation);
	if (bBelow && !bUnderground && !bPreview)
	{
		// Going under: a muffled hush as the map goes dark.
		LooterSound::Play2D(this, LooterSoundCue::MissionFeatures::Underground);
	}
	bUnderground = bBelow;
	const float Target = bUnderground ? 1.f : 0.f;
	if (UndergroundAlpha == Target || !UndergroundPlate)
	{
		return;
	}
	UndergroundAlpha = FMath::Clamp(UndergroundAlpha + (bUnderground ? 1.f : -1.f) * DeltaSeconds / UndergroundFadeSeconds, 0.f, 1.f);
	UndergroundPlate->SetRenderOpacity(UndergroundAlpha);
	UndergroundPlate->SetVisibility(UndergroundAlpha > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	// The bake goes as the plate comes: what it shows is the ground overhead.
	if (MapImage)
	{
		MapImage->SetRenderOpacity(1.f - UndergroundAlpha);
	}
	if (UndergroundLabel)
	{
		// Up from the middle by half the radius, as big as the map is now, clear of the player's arrow.
		UndergroundLabel->SetRenderTranslation(FVector2D(0.f, -GetDiameter() * 0.5f * LabelRise));
	}
}

#undef LOCTEXT_NAMESPACE
