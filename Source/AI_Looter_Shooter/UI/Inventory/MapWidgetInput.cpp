// UMapWidget: keys, the mouse and the sticks (held values applied each frame), the view's easing, and the pins and the
// travel card kept fresh while the page is open. Choosing a grave and travelling: MapWidgetSelect.cpp.

#include "UI/Inventory/MapWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "Components/CanvasPanel.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"

namespace
{
	/** Stick values under this are the stick at rest. */
	constexpr float StickDeadZone = 0.2f;

	float DeadZoned(float Value)
	{
		return FMath::Abs(Value) > StickDeadZone ? Value : 0.f;
	}

	/** How fast the view eases to a point it's sent to (a chosen grave, the player): the share of the way left per second. */
	constexpr double EaseRate = 10.0;
}

// ---------------------------------------------------------------------------
// Each frame: the sticks, the easing, the pins and the card kept fresh
// ---------------------------------------------------------------------------

void UMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	OpenTime += InDeltaTime;

	// Either stick pans (up looks north), the right trigger zooms in and the left out, about the frame's middle.
	FVector2D Stick = LeftStick + RightStick;
	if (Stick.SizeSquared() > 1.0)
	{
		Stick.Normalize();
	}
	if (!Stick.IsNearlyZero())
	{
		View.Pan(FVector2D(-Stick.X, Stick.Y) * MapLayout::StickPanSpeed * InDeltaTime);
		bEasing = false;
	}
	const float ZoomAxis = ZoomInAxis - ZoomOutAxis;
	if (FMath::Abs(ZoomAxis) > 0.05f)
	{
		View.ZoomAt(View.Frame * 0.5, FMath::Pow(2.0, ZoomAxis * MapLayout::TriggerZoomRate * InDeltaTime));
	}

	if (bEasing)
	{
		const FVector2D Before = View.Center;
		View.Center = FMath::Lerp(View.Center, EaseTarget, 1.0 - FMath::Exp(-EaseRate * InDeltaTime));
		View.Clamp();
		// There, or held by the map's edge short of it.
		bEasing = FVector2D::Distance(View.Center, EaseTarget) > 0.0005 && FVector2D::Distance(Before, View.Center) > 1e-6;
	}

	// The world moves on under the inventory: the pins twice a second, the travel card's verdict four times.
	PinRefreshLeft -= InDeltaTime;
	if (PinRefreshLeft <= 0.f)
	{
		PinRefreshLeft = 0.5f;
		const int32 Graves = MapPins::Count(Pins, EMapPinKind::Grave);
		RefreshPins();
		if (!SelectedGrave.IsNone() && !FindGravePin(SelectedGrave))
		{
			SelectedGrave = NAME_None;
		}
		RefreshLegend();
		if (Graves != MapPins::Count(Pins, EMapPinKind::Grave))
		{
			RebuildGraveList();
		}
	}
	CardRefreshLeft -= InDeltaTime;
	if (CardRefreshLeft <= 0.f)
	{
		CardRefreshLeft = 0.25f;
		RefreshTravelCard();
		RestyleGraveList();
	}
	if (StatusFlash > 0.f)
	{
		StatusFlash = FMath::Max(0.f, StatusFlash - InDeltaTime);
		RefreshTravelCard();
	}
	LayoutMap(InDeltaTime);
}

// ---------------------------------------------------------------------------
// Keys
// ---------------------------------------------------------------------------

FReply UMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	return HandleKey(InKeyEvent.GetKey()) ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UMapWidget::HandleKey(const FKey& Key)
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	ALooterHUD* HUD = OwningHUD.Get();

	// Esc and B let go of a chosen grave first, then close; the inventory's own key closes at once.
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (!SelectedGrave.IsNone())
		{
			SelectGrave(NAME_None, false, false);
			LooterSound::Play2D(this, LooterSoundCue::Back);
			return true;
		}
		Close();
		return true;
	}
	if (Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I))
	{
		Close();
		return true;
	}

	// The other pages.
	const struct { FKey PageKey; EInventoryPage Page; } PageKeys[] = {
		{ EKeys::One, EInventoryPage::Loadout },
		{ EKeys::Two, EInventoryPage::Bestiary },
		{ EKeys::Three, EInventoryPage::Missions },
		{ EKeys::Gamepad_LeftShoulder, EInventoryPage::Missions },
	};
	for (const auto& PageKey : PageKeys)
	{
		if (Key == PageKey.PageKey)
		{
			if (HUD)
			{
				HUD->ShowInventoryPage(PageKey.Page);
			}
			return true;
		}
	}
	if (Key == EKeys::Four)
	{
		return true;
	}

	// Panning a step at a time (held, the key repeats): up looks north.
	const double Step = MapLayout::KeyPanStep;
	const struct { FKey PanKey; FVector2D Delta; } PanKeys[] = {
		{ EKeys::W, FVector2D(0.0, Step) }, { EKeys::Up, FVector2D(0.0, Step) },
		{ EKeys::S, FVector2D(0.0, -Step) }, { EKeys::Down, FVector2D(0.0, -Step) },
		{ EKeys::A, FVector2D(Step, 0.0) }, { EKeys::Left, FVector2D(Step, 0.0) },
		{ EKeys::D, FVector2D(-Step, 0.0) }, { EKeys::Right, FVector2D(-Step, 0.0) },
	};
	for (const auto& PanKey : PanKeys)
	{
		if (Key == PanKey.PanKey)
		{
			View.Pan(PanKey.Delta);
			bEasing = false;
			return true;
		}
	}
	if (Key == EKeys::Add || Key == EKeys::Equals || Key == EKeys::PageUp)
	{
		ZoomBy(MapLayout::WheelZoom);
		return true;
	}
	if (Key == EKeys::Subtract || Key == EKeys::Hyphen || Key == EKeys::PageDown)
	{
		ZoomBy(1.0 / MapLayout::WheelZoom);
		return true;
	}
	if (Key == EKeys::Home)
	{
		ResetView();
		return true;
	}

	// Graves: G cycles them, the D-pad picks the one that way; the player's own spot with C or Y.
	if (Key == EKeys::G)
	{
		SelectNextGrave();
		return true;
	}
	const struct { FKey PadKey; FVector2D Direction; } PadKeys[] = {
		{ EKeys::Gamepad_DPad_Up, FVector2D(0.0, -1.0) }, { EKeys::Gamepad_DPad_Down, FVector2D(0.0, 1.0) },
		{ EKeys::Gamepad_DPad_Left, FVector2D(-1.0, 0.0) }, { EKeys::Gamepad_DPad_Right, FVector2D(1.0, 0.0) },
	};
	for (const auto& PadKey : PadKeys)
	{
		if (Key == PadKey.PadKey)
		{
			SelectGraveToward(PadKey.Direction);
			return true;
		}
	}
	if (Key == EKeys::C || Key == EKeys::Gamepad_FaceButton_Top)
	{
		CenterOnPlayer();
		return true;
	}
	const bool bInteractKey = Bindings && Bindings->GetKey(TEXT("Interact")) == Key;
	if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom || bInteractKey)
	{
		ConfirmTravel();
		return true;
	}
	return false;
}

// ---------------------------------------------------------------------------
// The mouse
// ---------------------------------------------------------------------------

bool UMapWidget::ToFrame(const FVector2D& ScreenPosition, FVector2D& OutFramePoint) const
{
	if (!MapCanvas)
	{
		return false;
	}
	const FGeometry& Geometry = MapCanvas->GetCachedGeometry();
	OutFramePoint = FVector2D(Geometry.AbsoluteToLocal(ScreenPosition));
	return Geometry.IsUnderLocation(ScreenPosition);
}

FReply UMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FKey Button = InMouseEvent.GetEffectingButton();
	FVector2D Point;
	if ((Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton) && ToFrame(InMouseEvent.GetScreenSpacePosition(), Point))
	{
		// A click until it moves, then a drag that pans.
		bPressing = true;
		bDragging = false;
		PressScreen = InMouseEvent.GetScreenSpacePosition();
		LastFramePoint = Point;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FVector2D Screen = InMouseEvent.GetScreenSpacePosition();
	FVector2D Point;
	const bool bOverMap = ToFrame(Screen, Point);
	if (bPressing)
	{
		if (!bDragging && FVector2D::Distance(Screen, PressScreen) >= LoadoutParts::DragStartDistance)
		{
			bDragging = true;
			SetHovered(INDEX_NONE);
			// The map in hand while it's dragged.
			SetCursor(EMouseCursor::GrabHandClosed);
		}
		if (bDragging)
		{
			View.Pan(Point - LastFramePoint);
			bEasing = false;
		}
		LastFramePoint = Point;
		return FReply::Handled();
	}
	if (bOverMap)
	{
		bHoverFromList = false;
		SetHovered(FindPinAt(Point));
	}
	else if (bHoverFromList)
	{
		// A grave's row keeps its pin lit while the pointer stays on the row.
		const bool bOnRow = GraveRows.ContainsByPredicate([&Screen](const FGraveRow& Row)
		{
			return Row.Button && Row.Button->GetCachedGeometry().IsUnderLocation(Screen);
		});
		if (!bOnRow)
		{
			bHoverFromList = false;
			SetHovered(INDEX_NONE);
		}
	}
	else
	{
		SetHovered(INDEX_NONE);
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FKey Button = InMouseEvent.GetEffectingButton();
	if (bPressing && (Button == EKeys::LeftMouseButton || Button == EKeys::RightMouseButton))
	{
		const bool bClick = !bDragging && Button == EKeys::LeftMouseButton;
		bPressing = false;
		bDragging = false;
		SetCursor(EMouseCursor::Default);
		FVector2D Point;
		if (bClick && ToFrame(InMouseEvent.GetScreenSpacePosition(), Point))
		{
			// A click on a grave chooses it; on another pin it only shows its name (pointing at it does that).
			const int32 Pin = FindPinAt(Point);
			if (Pins.IsValidIndex(Pin) && Pins[Pin].Kind == EMapPinKind::Grave)
			{
				SelectGrave(Pins[Pin].Id, false, true);
			}
			SetHovered(Pin);
		}
		SetKeyboardFocus();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UMapWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// A double click on a grave travels there (the rules allowing), as choosing it and pressing Travel would.
	FVector2D Point;
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && ToFrame(InMouseEvent.GetScreenSpacePosition(), Point))
	{
		const int32 Pin = FindPinAt(Point);
		if (Pins.IsValidIndex(Pin) && Pins[Pin].Kind == EMapPinKind::Grave)
		{
			SelectGrave(Pins[Pin].Id, false, false);
			ConfirmTravel();
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply UMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D Point;
	if (ToFrame(InMouseEvent.GetScreenSpacePosition(), Point))
	{
		// About the pointer: what's under it stays under it.
		View.ZoomAt(Point, FMath::Pow(MapLayout::WheelZoom, static_cast<double>(InMouseEvent.GetWheelDelta())));
		bEasing = false;
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply UMapWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	const float Value = InAnalogEvent.GetAnalogValue();
	if (Key == EKeys::Gamepad_LeftX)       { LeftStick.X = DeadZoned(Value); return FReply::Handled(); }
	if (Key == EKeys::Gamepad_LeftY)       { LeftStick.Y = DeadZoned(Value); return FReply::Handled(); }
	if (Key == EKeys::Gamepad_RightX)      { RightStick.X = DeadZoned(Value); return FReply::Handled(); }
	if (Key == EKeys::Gamepad_RightY)      { RightStick.Y = DeadZoned(Value); return FReply::Handled(); }
	if (Key == EKeys::Gamepad_RightTriggerAxis) { ZoomInAxis = Value > 0.1f ? Value : 0.f; return FReply::Handled(); }
	if (Key == EKeys::Gamepad_LeftTriggerAxis)  { ZoomOutAxis = Value > 0.1f ? Value : 0.f; return FReply::Handled(); }
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

void UMapWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bPressing = false;
	bDragging = false;
	SetCursor(EMouseCursor::Default);
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UMapWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (!bPressing)
	{
		bHoverFromList = false;
		SetHovered(INDEX_NONE);
	}
	Super::NativeOnMouseLeave(InMouseEvent);
}
