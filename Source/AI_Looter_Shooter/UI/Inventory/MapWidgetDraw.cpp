// UMapWidget: the map, its pins, the places' names and the player placed on the frame each frame, and the grid, the
// route to the chosen grave and the scale bar painted under the pins.

#include "UI/Inventory/MapWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/MinimapSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Rendering/DrawElements.h"

using namespace LooterUI;

namespace
{
	/** Grid lines at least this far apart (page units): finer steps come in as the map zooms. */
	constexpr double GridSpacing = 90.0;
	/** The scale bar's step is the grid's, at least this long on the page. */
	constexpr double ScaleSpacing = 70.0;
	/** A pin kept on the frame's edge when it lies beyond it (the objective, a turn-in, the player) stays this far in. */
	constexpr double EdgeInset = 18.0;
	/** The chosen grave's ring breathes at this rate (radians per second). */
	constexpr float PulseRate = 4.f;

	/** A page point for the painted lines (Slate draws in floats). */
	FVector2f Point2f(double X, double Y)
	{
		return FVector2f(static_cast<float>(X), static_cast<float>(Y));
	}

	void ShowIf(UWidget* Widget, bool bShow)
	{
		const ESlateVisibility Wanted = bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		if (Widget && Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}

	void PlaceAt(UWidget* Widget, const FVector2D& Position, const FVector2D& Size = FVector2D::ZeroVector)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetPosition(Position);
			if (!Size.IsZero())
			{
				CanvasSlot->SetSize(Size);
			}
		}
	}

	bool InFrame(const FVector2D& Point, double Margin)
	{
		return Point.X >= -Margin && Point.Y >= -Margin && Point.X <= MapLayout::MapSize.X + Margin && Point.Y <= MapLayout::MapSize.Y + Margin;
	}

	/** A point beyond the frame brought back onto its edge, EdgeInset in; true when it was beyond. */
	bool KeepOnFrame(FVector2D& Point)
	{
		const FVector2D Clamped(FMath::Clamp(Point.X, EdgeInset, MapLayout::MapSize.X - EdgeInset),
			FMath::Clamp(Point.Y, EdgeInset, MapLayout::MapSize.Y - EdgeInset));
		const bool bMoved = !Clamped.Equals(Point, 0.01);
		Point = Clamped;
		return bMoved;
	}
}

FVector2D UMapWidget::WorldToUV(const FVector& World) const
{
	return UMinimapSubsystem::WorldToMapUV(MapBounds, World);
}

FVector2D UMapWidget::WorldToFrame(const FVector& World) const
{
	return View.UVToFrame(WorldToUV(World));
}

// ---------------------------------------------------------------------------
// Each frame
// ---------------------------------------------------------------------------

void UMapWidget::LayoutMap(float DeltaTime)
{
	if (!MapCanvas)
	{
		return;
	}

	// The picture: the bake may finish after the page opened (its square was known as it started).
	UWorld* World = GetWorld();
	UMinimapSubsystem* Minimap = World ? World->GetSubsystem<UMinimapSubsystem>() : nullptr;
	if (!MapTexture.IsValid() && Minimap)
	{
		if (UTexture2D* Picture = Minimap->GetMapTexture())
		{
			MapTexture = Picture;
			MapBounds = Minimap->GetMapBounds();
			MapBrush.SetResourceObject(Picture);
			MapBrush.ImageSize = FVector2D(Picture->GetSizeX(), Picture->GetSizeY());
			MapImage->SetBrush(MapBrush);
		}
	}
	const bool bPicture = MapTexture.IsValid();
	ShowIf(MapImage, bPicture);
	ShowIf(LoadingText, !bPicture && Minimap != nullptr);
	if (bPicture)
	{
		PlaceAt(MapImage, View.MapTopLeft(), FVector2D(View.Side()));
	}

	LayoutPlaces();
	LayoutPins();
	LayoutPlayer();
	LayoutHover();

	// The scale bar's words and the zoom, rewritten only when they change.
	const double Step = View.GridStep(MapBounds.GetSize().X, ScaleSpacing);
	const FString Scale = FString::Printf(TEXT("%d M"), FMath::RoundToInt32(Step / 100.0));
	if (ScaleText && !ScaleText->GetText().ToString().Equals(Scale))
	{
		ScaleText->SetText(FText::FromString(Scale));
	}
	const FString Zoom = FString::Printf(TEXT("ZOOM %.1fX"), View.Zoom);
	if (ZoomText && !ZoomText->GetText().ToString().Equals(Zoom))
	{
		ZoomText->SetText(FText::FromString(Zoom));
	}
}

void UMapWidget::LayoutPlaces()
{
	const bool bMinor = View.Zoom >= MapPlaces::MinorZoom;
	for (int32 Index = 0; Index < PlaceLabels.Num() && Index < Places.Num(); ++Index)
	{
		const FMapPlace& Place = Places[Index];
		const FVector2D At = WorldToFrame(FVector(Place.At.X, Place.At.Y, 0.0));
		const bool bShow = (Place.bMajor || bMinor) && InFrame(At, 40.0);
		ShowIf(PlaceLabels[Index], bShow);
		if (bShow)
		{
			PlaceAt(PlaceLabels[Index], At);
		}
	}
}

void UMapWidget::LayoutPins()
{
	int32 Used = 0;
	bool bSelectedShown = false;
	for (int32 Index = 0; Index < Pins.Num() && Used < PinSlots.Num(); ++Index)
	{
		const FMapPin& Pin = Pins[Index];
		FVector2D At = WorldToFrame(Pin.Location);
		const float PinSize = MapIcons::Size(Pin.Kind);
		// Where the story points stays on the map's edge when it's beyond it, so the way to it is never lost.
		const bool bKeep = Pin.Kind == EMapPinKind::Objective || Pin.Kind == EMapPinKind::TurnIn;
		if (bKeep)
		{
			KeepOnFrame(At);
		}
		else if (!InFrame(At, PinSize))
		{
			continue;
		}

		FPinSlot& PinSlot = PinSlots[Used++];
		const bool bSelected = Pin.Kind == EMapPinKind::Grave && !SelectedGrave.IsNone() && Pin.Id == SelectedGrave;
		const bool bLit = bSelected || Index == Hovered;
		const float Grow = bLit ? 1.18f : 1.f;
		const FLinearColor Tint = bSelected ? Color::Accent() : MapIcons::Color(Pin.Kind);
		const bool bBadge = MapIcons::HasBadge(Pin.Kind);
		const float BadgeSize = PinSize * Grow;
		const float IconSize = bBadge ? BadgeSize * MapIcons::IconShare : BadgeSize;
		// What the slot shows: its kind, lit or not, chosen or not (never 0, the empty slot's key).
		const uint32 Key = 0x100u | (static_cast<uint32>(Pin.Kind) << 2) | (bLit ? 2u : 0u) | (bSelected ? 1u : 0u);
		if (PinSlot.StyleKey != Key)
		{
			PinSlot.Badge->SetBrush(MapIcons::BadgeBrush(Tint, bLit));
			PinSlot.Icon->SetBrush(MapIcons::IconBrush(Pin.Kind, IconSize, Tint));
			PinSlot.StyleKey = Key;
		}
		ShowIf(PinSlot.Badge, bBadge);
		ShowIf(PinSlot.Icon, true);
		PlaceAt(PinSlot.Badge, At, FVector2D(BadgeSize));
		PlaceAt(PinSlot.Icon, At, FVector2D(IconSize));

		if (bSelected)
		{
			bSelectedShown = true;
			const float Pulse = 0.5f + 0.5f * FMath::Sin(OpenTime * PulseRate);
			PlaceAt(SelectGlow, At);
			PlaceAt(SelectRing, At, FVector2D(BadgeSize + 10.f + 8.f * Pulse));
			if (SelectRing)
			{
				SelectRing->SetRenderOpacity(0.9f - 0.5f * Pulse);
			}
		}
	}
	for (int32 Index = Used; Index < PinSlots.Num(); ++Index)
	{
		ShowIf(PinSlots[Index].Badge, false);
		ShowIf(PinSlots[Index].Icon, false);
	}
	ShowIf(SelectGlow, bSelectedShown);
	ShowIf(SelectRing, bSelectedShown);
}

void UMapWidget::LayoutPlayer()
{
	const APawn* Player = GetOwningPlayerPawn();
	ShowIf(PlayerArrow, Player != nullptr);
	ShowIf(PlayerGlow, Player != nullptr);
	if (!Player)
	{
		return;
	}
	// North is up, so the arrow turns by the view's yaw (0 faces north; east, to the right, is clockwise).
	FVector2D At = WorldToFrame(Player->GetActorLocation());
	KeepOnFrame(At);
	PlaceAt(PlayerArrow, At);
	PlaceAt(PlayerGlow, At);
	PlayerArrow->SetRenderTransformAngle(static_cast<float>(Player->GetControlRotation().Yaw));
	PlayerGlow->SetRenderOpacity(0.7f + 0.3f * FMath::Sin(OpenTime * 2.5f));
}

void UMapWidget::LayoutHover()
{
	if (!HoverCard || !HoverName || !HoverDetail)
	{
		return;
	}
	if (!Pins.IsValidIndex(Hovered))
	{
		ShowIf(HoverCard, false);
		return;
	}
	const FMapPin& Pin = Pins[Hovered];
	const FText Name = Pin.Name.ToUpper();
	if (!HoverName->GetText().EqualTo(Name))
	{
		HoverName->SetText(Name);
		HoverDetail->SetText(Pin.Detail);
		HoverDetail->SetVisibility(Pin.Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	// Beside the pin, on whichever side has room.
	FVector2D At = WorldToFrame(Pin.Location);
	if (Pin.Kind == EMapPinKind::Objective || Pin.Kind == EMapPinKind::TurnIn)
	{
		KeepOnFrame(At);
	}
	const double Gap = MapIcons::Size(Pin.Kind) * 0.6 + 10.0;
	const bool bLeft = At.X > MapLayout::MapSize.X - 260.0;
	if (UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(HoverCard->Slot))
	{
		CardSlot->SetAlignment(FVector2D(bLeft ? 1.f : 0.f, 0.5f));
		CardSlot->SetPosition(At + FVector2D(bLeft ? -Gap : Gap, 0.0));
	}
	ShowIf(HoverCard, true);
}

// ---------------------------------------------------------------------------
// Painted under the pins: the grid, the route to the chosen grave, the scale bar
// ---------------------------------------------------------------------------

void UMapWidget::PaintMap(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const FVector2D Size = MapLayout::MapSize;
	const FVector2D MapSizeCm = MapBounds.GetSize();
	if (MapSizeCm.X <= 0.0 || MapSizeCm.Y <= 0.0)
	{
		return;
	}

	// The grid: world-aligned lines (north-south and east-west) a whole number of metres apart, faint over the land.
	const double Step = View.GridStep(MapSizeCm.X, GridSpacing);
	const FLinearColor GridColor = Color::ScreenLine() * FLinearColor(1.f, 1.f, 1.f, 0.16f);
	const FVector2D TopLeft = View.MapTopLeft();
	const double Left = FMath::Max(0.0, TopLeft.X);
	const double Top = FMath::Max(0.0, TopLeft.Y);
	const double Right = FMath::Min(Size.X, TopLeft.X + View.Side());
	const double Bottom = FMath::Min(Size.Y, TopLeft.Y + View.Side());
	constexpr int32 MaxLines = 160;
	int32 Lines = 0;
	// East-west lines (a fixed X, north) run across; their V is (Max.X - X) / size.
	for (double X = FMath::CeilToDouble(MapBounds.Min.X / Step) * Step; X <= MapBounds.Max.X && Lines < MaxLines; X += Step, ++Lines)
	{
		const double Y = View.UVToFrame(FVector2D(0.0, (MapBounds.Max.X - X) / MapSizeCm.X)).Y;
		if (Y > 0.0 && Y < Size.Y)
		{
			LoadoutParts::DrawLines(Elements, LayerId, Geometry, { Point2f(Left, Y), Point2f(Right, Y) }, GridColor, 1.f);
		}
	}
	// North-south lines (a fixed Y, east) run up and down; their U is (Y - Min.Y) / size.
	for (double Y = FMath::CeilToDouble(MapBounds.Min.Y / Step) * Step; Y <= MapBounds.Max.Y && Lines < MaxLines * 2; Y += Step, ++Lines)
	{
		const double X = View.UVToFrame(FVector2D((Y - MapBounds.Min.Y) / MapSizeCm.Y, 0.0)).X;
		if (X > 0.0 && X < Size.X)
		{
			LoadoutParts::DrawLines(Elements, LayerId, Geometry, { Point2f(X, Top), Point2f(X, Bottom) }, GridColor, 1.f);
		}
	}

	// The route to the chosen grave: a dashed line from the player, marching toward it.
	const APawn* Player = GetOwningPlayerPawn();
	const FMapPin* Grave = FindGravePin(SelectedGrave);
	if (Player && Grave)
	{
		const FVector2D From = WorldToFrame(Player->GetActorLocation());
		const FVector2D To = WorldToFrame(Grave->Location);
		const FVector2D Along = To - From;
		const double Length = Along.Size();
		constexpr double Dash = 10.0;
		constexpr double Gap = 7.0;
		const FLinearColor RouteColor = Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.75f);
		if (Length > 30.0)
		{
			const FVector2D Direction = Along / Length;
			// Clear of the arrow and the badge at either end.
			const double Start = 16.0;
			const double End = Length - MapIcons::Size(EMapPinKind::Grave) * 0.7;
			const double Offset = FMath::Fmod(static_cast<double>(OpenTime) * 28.0, Dash + Gap);
			for (double S = Start + Offset - (Dash + Gap); S < End; S += Dash + Gap)
			{
				const double A = FMath::Max(S, Start);
				const double B = FMath::Min(S + Dash, End);
				if (B > A)
				{
					const FVector2D P = From + Direction * A;
					const FVector2D Q = From + Direction * B;
					LoadoutParts::DrawLines(Elements, LayerId, Geometry, { Point2f(P.X, P.Y), Point2f(Q.X, Q.Y) }, RouteColor, 2.f);
				}
			}
		}
	}

	// The scale bar, bottom left: one grid step long, dark under light so it reads on any ground.
	const double ScaleStep = View.GridStep(MapSizeCm.X, ScaleSpacing);
	const double BarLength = ScaleStep * View.Side() / MapSizeCm.X;
	const float BarY = static_cast<float>(Size.Y - 18.0);
	const float X0 = 22.f;
	const float X1 = static_cast<float>(22.0 + BarLength);
	const TArray<FVector2f> Bar = { FVector2f(X0, BarY - 6.f), FVector2f(X0, BarY), FVector2f(X1, BarY), FVector2f(X1, BarY - 6.f) };
	LoadoutParts::DrawLines(Elements, LayerId, Geometry, Bar, Color::Outline(), 4.f);
	LoadoutParts::DrawLines(Elements, LayerId + 1, Geometry, Bar, Color::Text(), 2.f);
}
