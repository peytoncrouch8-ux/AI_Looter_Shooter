#include "UI/HUD/HudMinimapWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Creatures/CreatureBase.h"
#include "Loot/AmmoPickup.h"
#include "Missions/MissionSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "World/MinimapSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	/** How much of the world the minimap shows (radius, cm) at zoom 1, at any size. */
	constexpr float BaseRange = 3500.f;
	/** How many things it can mark at once. */
	constexpr int32 MaxMarkers = 32;
	constexpr float ArrowSize = 22.f;
	/** The mission waypoint's ring (a bit bigger than a loot dot), and the arrow on the rim that points to it. */
	constexpr float WaypointSize = 16.f;
	constexpr float NeedleSize = 14.f;
	/** Gap between the rim and the waypoint's ring, as the other markers keep. */
	constexpr float RimGap = 3.f;
	/**
	 * Gap between the rim and the arrow: clear of the facing notch at the top (10 px deep), where the arrow sits most
	 * often (walking toward the waypoint).
	 */
	constexpr float NeedleGap = 13.f;

	/**
	 * The mission waypoint on the map: a ring around a dot, so it reads apart from the loot dots (an orange one is
	 * legendary loot) and from the hostiles' diamonds.
	 */
	const FVectorIcon& WaypointIcon()
	{
		static const FVectorIcon Icon = []
		{
			FVectorIcon Result;
			Result.ViewBox = FVector2D(24.f, 24.f);
			Result.StrokeWidth = 2.5f;
			TArray<FVector2D>& Ring = Result.Strokes.AddDefaulted_GetRef();
			TArray<FVector2D>& Dot = Result.Fills.AddDefaulted_GetRef();
			constexpr int32 Sides = 32;
			for (int32 Step = 0; Step <= Sides; ++Step)
			{
				const double Angle = UE_TWO_PI * Step / Sides;
				Ring.Add(FVector2D(12.0 + 9.5 * FMath::Cos(Angle), 12.0 + 9.5 * FMath::Sin(Angle)));
				if (Step < Sides)
				{
					Dot.Add(FVector2D(12.0 + 4.0 * FMath::Cos(Angle), 12.0 + 4.0 * FMath::Sin(Angle)));
				}
			}
			return Result;
		}();
		return Icon;
	}

	/** Changes a widget's visibility only when it differs (the minimap updates every frame). */
	void ShowIf(UWidget* Widget, bool bShow)
	{
		const ESlateVisibility Wanted = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		if (Widget && Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}

	/** Centers Widget on Position inside Canvas; a zero WidgetSize lets it size itself. */
	void AddToCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& WidgetSize)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(WidgetSize.IsZero());
		if (!WidgetSize.IsZero())
		{
			CanvasSlot->SetSize(WidgetSize);
		}
	}
}

TSharedRef<SWidget> UHudMinimapWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// Layers, bottom to top: dark glass disc, the map (clipped to the circle), loot/hostile markers, the mission
		// waypoint and the player arrow, the rim, then the facing notch, the N, and the waypoint's arrow and distance.
		const float Radius = Diameter * 0.5f;
		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		SizeBox = MakeSized(WidgetTree, Stack, Diameter, Diameter);
		SizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = SizeBox;
		auto AddLayer = [Stack](UWidget* Layer)
		{
			FillOverlaySlot(Stack->AddChildToOverlay(Layer));
		};

		const FLinearColor Glass = Color::ScreenBg();
		UImage* GlassDisc = MakeImage(WidgetTree, CircleBrush(FLinearColor(Glass.R, Glass.G, Glass.B, 0.62f)));
		MarkBackground(GlassDisc);
		AddLayer(GlassDisc);

		MapBrush = CircleBrush(FLinearColor::White);
		MapImage = MakeImage(WidgetTree, MapBrush);
		MapImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		MapImage->SetVisibility(ESlateVisibility::Hidden);
		AddLayer(MapImage);

		UCanvasPanel* MarkerLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		AddLayer(MarkerLayer);
		for (int32 Index = 0; Index < MaxMarkers; ++Index)
		{
			UImage* Marker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Marker->SetVisibility(ESlateVisibility::Hidden);
			AddToCanvas(MarkerLayer, Marker, FVector2D(Radius), FVector2D(10.f));
			Markers.Add(Marker);
			MarkerKeys.Add(0);
		}
		// The mission's waypoint, over every loot and hostile marker; only your own arrow covers it, so standing on it
		// still shows which way you face.
		Waypoint = MakeImage(WidgetTree, IconBrush(TEXT("MinimapWaypoint"), WaypointIcon(), 2.f, FVector2D(WaypointSize), Color::Accent()));
		Waypoint->SetVisibility(ESlateVisibility::Hidden);
		AddToCanvas(MarkerLayer, Waypoint, FVector2D(Radius), FVector2D(WaypointSize));
		// You: an arrow in the middle that always points up (the map turns under it).
		Arrow = MakeImage(WidgetTree, MarkerBrush(EMarker::Arrow, FLinearColor::White));
		AddToCanvas(MarkerLayer, Arrow, FVector2D(Radius), FVector2D(ArrowSize));

		AddLayer(MakeImage(WidgetTree, CircleBrush(FLinearColor::Transparent, Color::ScreenLine(), 2.f)));

		UCanvasPanel* Rim = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		AddLayer(Rim);
		Notch = MakeImage(WidgetTree, RectBrush(Color::Accent()));
		AddToCanvas(Rim, Notch, FVector2D(Radius, 5.f), FVector2D(3.f, 10.f));
		North = MakeFloatingText(WidgetTree, 12, Color::Accent(), 0, ETextJustify::Center);
		North->SetText(FText::FromString(TEXT("N")));
		AddToCanvas(Rim, North, FVector2D(Radius, 12.f), FVector2D::ZeroVector);

		// A waypoint past the rim: a compass needle just inside the rim, turned toward it, with how far it is.
		WaypointArrow = MakeImage(WidgetTree, MarkerBrush(EMarker::Arrow, Color::Accent()));
		WaypointArrow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		WaypointArrow->SetVisibility(ESlateVisibility::Hidden);
		AddToCanvas(Rim, WaypointArrow, FVector2D(Radius), FVector2D(NeedleSize));
		WaypointDistance = MakeFloatingText(WidgetTree, 10, Color::Accent(), 0, ETextJustify::Center);
		WaypointDistance->SetVisibility(ESlateVisibility::Hidden);
		AddToCanvas(Rim, WaypointDistance, FVector2D(Radius), FVector2D::ZeroVector);
	}
	return Super::RebuildWidget();
}

void UHudMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UWorld* World = GetWorld();
	UMinimapSubsystem* Minimap = World ? World->GetSubsystem<UMinimapSubsystem>() : nullptr;
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!SizeBox)
	{
		return;
	}
	// The player's settings, live (the settings menu changes them while the game is paused). Turned off, the map
	// hides and skips all its work below.
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
	if (!Minimap || !Pawn || (Settings && !Settings->IsMinimapShown()))
	{
		SizeBox->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	SizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	const float WantedScale = Settings ? Settings->GetMinimapScale() : 1.f;
	const float Range = BaseRange / (Settings ? Settings->GetMinimapZoom() : 1.f);
	if (!FMath::IsNearlyEqual(WantedScale, Scale))
	{
		ApplyScale(WantedScale);
	}

	const FVector Location = Pawn->GetActorLocation();
	const float Yaw = Pawn->GetControlRotation().Yaw;
	const float Radius = GetDiameter() * 0.5f;
	const float PixelsPerCm = (Radius - 4.f) / Range;
	// Markers grow more gently than the map, so a small map stays readable and a big one uncluttered.
	const float MarkerScale = FMath::Sqrt(Scale);

	// The island: the part of the baked picture around you, turned so the way you look points up.
	UTexture2D* Map = Minimap->GetMapTexture();
	if (Map != MapTexture)
	{
		MapTexture = Map;
		MapBrush.SetResourceObject(Map);
		MapImage->SetVisibility(Map ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (Map)
	{
		const FVector2D Center = Minimap->WorldToMapUV(Location);
		const double Half = Range / FMath::Max(Minimap->GetMapBounds().GetSize().X, 1.0);
		MapBrush.SetUVRegion(FBox2d(Center - FVector2D(Half), Center + FVector2D(Half)));
		MapImage->SetBrush(MapBrush);
		MapImage->SetRenderTransformAngle(-Yaw);
	}

	// Markers: ammo and loot first, hostiles last so they draw on top. Anything past the rim is left off.
	int32 Used = 0;
	auto Mark = [&](const FVector& Where, EMarker Kind, const FLinearColor& Tint, float BaseSize)
	{
		if (Used >= Markers.Num())
		{
			return;
		}
		const float Size = BaseSize * MarkerScale;
		const FVector2D Offset = UMinimapSubsystem::ViewOffset(Where - Location, Yaw, PixelsPerCm);
		if (Offset.Size() > Radius - Size * 0.5f - 3.f)
		{
			return;
		}
		UImage* Marker = Markers[Used];
		const uint32 Key = HashCombine(GetTypeHash(static_cast<uint8>(Kind)), GetTypeHash(Tint.ToFColor(true))) | 1u;
		if (MarkerKeys[Used] != Key)
		{
			Marker->SetBrush(MarkerBrush(Kind, Tint));
			MarkerKeys[Used] = Key;
		}
		if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
		{
			MarkerSlot->SetSize(FVector2D(Size));
			MarkerSlot->SetPosition(FVector2D(Radius) + Offset);
		}
		Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
		++Used;
	};
	const FLinearColor AmmoColor(FColor(230, 220, 192));
	for (TActorIterator<AAmmoPickup> It(World); It; ++It)
	{
		Mark(It->GetActorLocation(), EMarker::Dot, AmmoColor, 6.f);
	}
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		if (It->IsPickup())
		{
			Mark(It->GetActorLocation(), EMarker::Dot, LooterWeaponText::Color(It->GetInstance()), 9.f);
		}
	}
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		if (!It->IsDead())
		{
			Mark(It->GetActorLocation(), EMarker::Diamond, Color::Health(), 12.f);
		}
	}
	for (int32 Index = Used; Index < Markers.Num(); ++Index)
	{
		Markers[Index]->SetVisibility(ESlateVisibility::Hidden);
	}

	// North circles the rim as you turn.
	if (UCanvasPanelSlot* NorthSlot = Cast<UCanvasPanelSlot>(North->Slot))
	{
		const FVector2D NorthOffset = UMinimapSubsystem::ViewOffset(FVector::ForwardVector, Yaw, 1.f).GetSafeNormal() * (Radius - 13.f);
		NorthSlot->SetPosition(FVector2D(Radius) + NorthOffset);
	}

	UpdateWaypoint(Location, Yaw, Radius, PixelsPerCm);
}

FMinimapWaypoint UHudMinimapWidget::PlaceWaypoint(const FVector& WorldDelta, float ViewYaw, float PixelsPerCm, float InsideRadius, float RimRadius)
{
	FMinimapWaypoint Result;
	const FVector2D Offset = UMinimapSubsystem::ViewOffset(WorldDelta, ViewYaw, PixelsPerCm);
	// Up on screen is -Y, so the angle clockwise from up is atan2(right, up); Slate turns widgets clockwise too.
	Result.Angle = Offset.IsNearlyZero() ? 0.f : static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Offset.X, -Offset.Y)));
	Result.bInside = Offset.Size() <= InsideRadius;
	Result.Position = Result.bInside ? Offset : Offset.GetSafeNormal() * RimRadius;
	return Result;
}

void UHudMinimapWidget::UpdateWaypoint(const FVector& PlayerLocation, float Yaw, float Radius, float PixelsPerCm)
{
	if (!Waypoint || !WaypointArrow || !WaypointDistance)
	{
		return;
	}
	const UWorld* World = GetWorld();
	const UMissionSubsystem* Missions = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	FVector Target = FVector::ZeroVector;
	if (!Missions || !Missions->GetTrackedWaypoint(Target))
	{
		ShowIf(Waypoint, false);
		ShowIf(WaypointArrow, false);
		ShowIf(WaypointDistance, false);
		return;
	}

	// Sizes as ApplyScale sets them: markers grow more gently than the map.
	const float MarkerScale = FMath::Sqrt(Scale);
	const float RingSize = WaypointSize * MarkerScale;
	const float NeedleExtent = NeedleSize * MarkerScale;
	const FMinimapWaypoint Placed = PlaceWaypoint(Target - PlayerLocation, Yaw, PixelsPerCm,
		Radius - RingSize * 0.5f - RimGap, Radius - NeedleExtent * 0.5f - NeedleGap);
	const FVector2D Center(Radius);
	ShowIf(Waypoint, Placed.bInside);
	ShowIf(WaypointArrow, !Placed.bInside);
	ShowIf(WaypointDistance, !Placed.bInside);
	if (Placed.bInside)
	{
		if (UCanvasPanelSlot* WaypointSlot = Cast<UCanvasPanelSlot>(Waypoint->Slot))
		{
			WaypointSlot->SetPosition(Center + Placed.Position);
		}
		return;
	}

	if (UCanvasPanelSlot* NeedleSlot = Cast<UCanvasPanelSlot>(WaypointArrow->Slot))
	{
		NeedleSlot->SetPosition(Center + Placed.Position);
	}
	WaypointArrow->SetRenderTransformAngle(Placed.Angle);

	// How far, in whole meters across the map (height doesn't count); the text only changes with the number.
	const int32 Meters = FMath::RoundToInt32(FVector::Dist2D(Target, PlayerLocation) / 100.0);
	if (Meters != WaypointMeters)
	{
		WaypointMeters = Meters;
		WaypointDistance->SetText(FText::FromString(FString::Printf(TEXT("%d m"), Meters)));
	}
	// Just inside the needle, toward the center, and pulled in by half the label's own extent that way so it never
	// crosses the rim, whichever side the needle is on.
	FVector2D Half = WaypointDistance->GetDesiredSize() * 0.5;
	if (Half.IsNearlyZero())
	{
		// Not laid out yet (its first frame): about the size of "85 m".
		Half = FVector2D(14.0, 7.0);
	}
	const FVector2D Direction = Placed.Position.GetSafeNormal();
	const double Inset = NeedleExtent * 0.5 + 2.0 + FMath::Abs(Direction.X) * Half.X + FMath::Abs(Direction.Y) * Half.Y;
	if (UCanvasPanelSlot* LabelSlot = Cast<UCanvasPanelSlot>(WaypointDistance->Slot))
	{
		LabelSlot->SetPosition(Center + Direction * (Placed.Position.Size() - Inset));
	}
}

void UHudMinimapWidget::ApplyScale(float NewScale)
{
	Scale = NewScale;
	if (!SizeBox)
	{
		return;
	}
	// The glass, the map and the rim fill the box, so they follow it; the arrow and the notch are placed by hand.
	const float Size = GetDiameter();
	const float Radius = Size * 0.5f;
	SizeBox->SetWidthOverride(Size);
	SizeBox->SetHeightOverride(Size);
	if (UCanvasPanelSlot* ArrowSlot = Cast<UCanvasPanelSlot>(Arrow->Slot))
	{
		ArrowSlot->SetPosition(FVector2D(Radius));
		ArrowSlot->SetSize(FVector2D(ArrowSize * FMath::Sqrt(NewScale)));
	}
	if (UCanvasPanelSlot* NotchSlot = Cast<UCanvasPanelSlot>(Notch->Slot))
	{
		NotchSlot->SetPosition(FVector2D(Radius, 5.f));
	}
	// The waypoint's ring and needle are markers: they grow like the others (placed every frame by UpdateWaypoint).
	const float MarkerScale = FMath::Sqrt(NewScale);
	if (UCanvasPanelSlot* WaypointSlot = Waypoint ? Cast<UCanvasPanelSlot>(Waypoint->Slot) : nullptr)
	{
		WaypointSlot->SetSize(FVector2D(WaypointSize * MarkerScale));
	}
	if (UCanvasPanelSlot* NeedleSlot = WaypointArrow ? Cast<UCanvasPanelSlot>(WaypointArrow->Slot) : nullptr)
	{
		NeedleSlot->SetSize(FVector2D(NeedleSize * MarkerScale));
	}
}
