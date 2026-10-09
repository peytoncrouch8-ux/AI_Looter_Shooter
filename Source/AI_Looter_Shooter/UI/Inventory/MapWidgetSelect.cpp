// UMapWidget: pins under the pointer, choosing a grave (a click, G, the D-pad, the list), sending the view somewhere,
// travelling, and the page's buttons.

#include "UI/Inventory/MapWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterButton.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RespawnMarker.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Centring on the player zooms in at least this far, so it shows where they stand rather than the whole valley. */
	constexpr double PlayerZoom = 2.2;

	/** An edge-kept pin (the objective, a turn-in) stays this far inside the frame, as it's drawn. */
	constexpr double EdgeInset = 18.0;
}

// ---------------------------------------------------------------------------
// Pins under the pointer
// ---------------------------------------------------------------------------

int32 UMapWidget::FindPinAt(const FVector2D& FramePoint) const
{
	// The nearest whose badge (and a little round it) is under the point; the edge-kept pins where they're drawn.
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Pins.Num(); ++Index)
	{
		const FMapPin& Pin = Pins[Index];
		FVector2D At = WorldToFrame(Pin.Location);
		if (Pin.Kind == EMapPinKind::Objective || Pin.Kind == EMapPinKind::TurnIn)
		{
			At.X = FMath::Clamp(At.X, EdgeInset, MapLayout::MapSize.X - EdgeInset);
			At.Y = FMath::Clamp(At.Y, EdgeInset, MapLayout::MapSize.Y - EdgeInset);
		}
		const double Distance = FVector2D::Distance(At, FramePoint);
		if (Distance <= MapIcons::Size(Pin.Kind) * 0.5 + MapLayout::PinReach && Distance < BestDistance)
		{
			Best = Index;
			BestDistance = Distance;
		}
	}
	return Best;
}

void UMapWidget::SetHovered(int32 PinIndex)
{
	const int32 Wanted = Pins.IsValidIndex(PinIndex) ? PinIndex : INDEX_NONE;
	if (Wanted == Hovered)
	{
		return;
	}
	Hovered = Wanted;
	if (Hovered != INDEX_NONE)
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
	}
	// A grave under the pointer can be clicked: the hand says so.
	if (!bDragging)
	{
		SetCursor(Hovered != INDEX_NONE && Pins[Hovered].Kind == EMapPinKind::Grave ? EMouseCursor::Hand : EMouseCursor::Default);
	}
}

const FMapPin* UMapWidget::FindGravePin(FName GraveId) const
{
	if (GraveId.IsNone())
	{
		return nullptr;
	}
	return Pins.FindByPredicate([GraveId](const FMapPin& Pin) { return Pin.Kind == EMapPinKind::Grave && Pin.Id == GraveId; });
}

// ---------------------------------------------------------------------------
// Choosing a grave
// ---------------------------------------------------------------------------

void UMapWidget::SelectGrave(FName GraveId, bool bCenter, bool bSound)
{
	const FMapPin* Pin = FindGravePin(GraveId);
	const FName Wanted = Pin ? GraveId : FName(NAME_None);
	if (Pin && bCenter)
	{
		CenterOn(WorldToUV(Pin->Location));
	}
	if (Wanted == SelectedGrave)
	{
		return;
	}
	SelectedGrave = Wanted;
	if (Pin && bSound)
	{
		LooterSound::Play2D(this, LooterSoundCue::Click);
	}
	ShownStatus.Reset();
	RestyleGraveList();
	RefreshTravelCard();
	RefreshPrompts();
}

void UMapWidget::SelectNextGrave()
{
	TArray<FName> Graves;
	for (const FMapPin& Pin : Pins)
	{
		if (Pin.Kind == EMapPinKind::Grave)
		{
			Graves.Add(Pin.Id);
		}
	}
	if (Graves.IsEmpty())
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		return;
	}
	const int32 Current = Graves.IndexOfByKey(SelectedGrave);
	SelectGrave(Graves[(Current + 1) % Graves.Num()], true, true);
}

void UMapWidget::SelectGraveToward(const FVector2D& Direction)
{
	// From the chosen grave, or else from the player, or the frame's middle: the open grave that way on the map.
	TArray<FVector2D> Points;
	TArray<FName> Ids;
	int32 From = INDEX_NONE;
	for (const FMapPin& Pin : Pins)
	{
		if (Pin.Kind == EMapPinKind::Grave)
		{
			if (Pin.Id == SelectedGrave)
			{
				From = Points.Num();
			}
			Points.Add(WorldToFrame(Pin.Location));
			Ids.Add(Pin.Id);
		}
	}
	if (Points.IsEmpty())
	{
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		return;
	}
	FVector2D Origin = View.Frame * 0.5;
	if (Points.IsValidIndex(From))
	{
		Origin = Points[From];
	}
	else if (const APawn* Player = GetOwningPlayerPawn())
	{
		Origin = WorldToFrame(Player->GetActorLocation());
	}
	const int32 Picked = FMapView::PickInDirection(Points, Origin, Direction, From);
	if (Ids.IsValidIndex(Picked))
	{
		SelectGrave(Ids[Picked], true, true);
		return;
	}
	if (From == INDEX_NONE)
	{
		// Nothing chosen and nothing that way: the nearest grave.
		int32 Nearest = 0;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Nearest = FVector2D::Distance(Points[Index], Origin) < FVector2D::Distance(Points[Nearest], Origin) ? Index : Nearest;
		}
		SelectGrave(Ids[Nearest], true, true);
		return;
	}
	// No open grave that way from the chosen one.
	LooterSound::Play2D(this, LooterSoundCue::Denied);
}

// ---------------------------------------------------------------------------
// Sending the view somewhere
// ---------------------------------------------------------------------------

void UMapWidget::CenterOn(const FVector2D& UV)
{
	EaseTarget = UV;
	bEasing = true;
}

void UMapWidget::CenterOnPlayer()
{
	if (const APawn* Player = GetOwningPlayerPawn())
	{
		if (View.Zoom < PlayerZoom)
		{
			View.ZoomAt(View.Frame * 0.5, PlayerZoom / View.Zoom);
		}
		CenterOn(WorldToUV(Player->GetActorLocation()));
		LooterSound::Play2D(this, LooterSoundCue::Click);
	}
}

void UMapWidget::ZoomBy(double Factor)
{
	View.ZoomAt(View.Frame * 0.5, Factor);
}

// ---------------------------------------------------------------------------
// Travelling
// ---------------------------------------------------------------------------

void UMapWidget::ConfirmTravel()
{
	const FMapPin* Pin = FindGravePin(SelectedGrave);
	const ARespawnMarker* Grave = Pin ? Pin->Grave.Get() : nullptr;
	UGraveTravelSubsystem* Travel = UGraveTravelSubsystem::Get(this);
	EGraveTravelBlock Block = EGraveTravelBlock::None;
	if (!Grave || !Travel || !Travel->TravelTo(GetOwningPlayerPawn(), *Grave, &Block))
	{
		// Not now: the card says why, and flashes it.
		LooterSound::Play2D(this, LooterSoundCue::Denied);
		FlashStatus();
		return;
	}
	// On the way: the inventory closes as the screen fades (the travel's whoosh is the sound of it).
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseInventory();
	}
}

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------

void UMapWidget::HandleTabClicked(ULooterButton* Button)
{
	ALooterHUD* HUD = OwningHUD.Get();
	if (HUD && Button && Button->Index != static_cast<int32>(EInventoryPage::Map))
	{
		HUD->ShowInventoryPage(static_cast<EInventoryPage>(Button->Index));
	}
	else
	{
		SetKeyboardFocus();
	}
}

void UMapWidget::HandleTravelClicked(ULooterButton* Button)
{
	ConfirmTravel();
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the page's keys.
	SetKeyboardFocus();
}

void UMapWidget::HandleGraveRowClicked(ULooterButton* Button)
{
	if (Button && GraveRows.IsValidIndex(Button->Index))
	{
		SelectGrave(GraveRows[Button->Index].GraveId, true, true);
	}
	SetKeyboardFocus();
}

void UMapWidget::HandleGraveRowHovered(ULooterButton* Button)
{
	if (!Button || !GraveRows.IsValidIndex(Button->Index))
	{
		return;
	}
	// The row's grave lights up on the map too, with its name beside it.
	const FName GraveId = GraveRows[Button->Index].GraveId;
	const int32 PinIndex = Pins.IndexOfByPredicate([GraveId](const FMapPin& Pin) { return Pin.Kind == EMapPinKind::Grave && Pin.Id == GraveId; });
	bHoverFromList = PinIndex != INDEX_NONE;
	SetHovered(PinIndex);
}
