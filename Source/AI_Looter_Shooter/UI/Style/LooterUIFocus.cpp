#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "InputCoreTypes.h"

namespace
{
	/** The menus start under the mouse: the focus look waits for the first key or gamepad press. */
	LooterUI::EMenuDevice GMenuDevice = LooterUI::EMenuDevice::Mouse;

	/** The lit edge down the left side, and how far it stays clear of the chamfer's cut top-left corner. */
	constexpr float LitEdgeWidth = 3.f;
	constexpr float LitEdgeTop = 8.f;
	constexpr float LitEdgeBottom = 3.f;
	/** The outline's double, this far inside it and this much fainter: together they read as a lit frame. */
	constexpr float InnerOutlineInset = 2.f;
	constexpr float InnerOutlineAlpha = 0.45f;
	/** The breath: full for a moment as the focus arrives, then down to BreathLow and back, BreathHz times a second. */
	constexpr float ArriveSeconds = 0.18f;
	constexpr float BreathHz = 0.75f;
	constexpr float BreathLow = 0.72f;
}

// ---------------------------------------------------------------------------
// Which device drives the menus
// ---------------------------------------------------------------------------

LooterUI::EMenuDevice LooterUI::GetMenuDevice()
{
	return GMenuDevice;
}

void LooterUI::SetMenuDevice(EMenuDevice Device)
{
	GMenuDevice = Device;
}

bool LooterUI::NoteMenuKey(const FKey& Key)
{
	const EMenuDevice Device = Key.IsGamepadKey() ? EMenuDevice::Gamepad : EMenuDevice::Keyboard;
	if (Device == GMenuDevice)
	{
		return false;
	}
	GMenuDevice = Device;
	return true;
}

bool LooterUI::NoteMenuPointer()
{
	if (GMenuDevice == EMenuDevice::Mouse)
	{
		return false;
	}
	GMenuDevice = EMenuDevice::Mouse;
	return true;
}

bool LooterUI::IsMenuFocusShown()
{
	return GMenuDevice != EMenuDevice::Mouse;
}

bool LooterUI::IsMenuGamepad()
{
	return GMenuDevice == EMenuDevice::Gamepad;
}

// ---------------------------------------------------------------------------
// The focus look
// ---------------------------------------------------------------------------

UWidget* LooterUI::MakeFocusFrame(UWidgetTree* Tree, bool bOnAccent, bool bWash)
{
	if (!Tree)
	{
		return nullptr;
	}
	const FLinearColor Line = bOnAccent ? Color::FocusOnAccent() : Color::Focus();
	UOverlay* Frame = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Frame->SetVisibility(ESlateVisibility::Collapsed);

	// The wash is a background (it fades with the UI transparency); on orange it wouldn't show, so there is none.
	if (bWash && !bOnAccent)
	{
		UImage* Wash = MakeImage(Tree, ShapeBrush(EShape::Control, false, Color::FocusWash()));
		MarkBackground(Wash);
		FillOverlaySlot(Frame->AddChildToOverlay(Wash));
	}
	// The outline on the stop's own edge, so nothing that clips (a scrolling list) cuts it, and its fainter double inside.
	FillOverlaySlot(Frame->AddChildToOverlay(MakeImage(Tree, ShapeBrush(EShape::Control, true, Line))));
	FillOverlaySlot(Frame->AddChildToOverlay(MakeImage(Tree, ShapeBrush(EShape::Control, true, Line * FLinearColor(1.f, 1.f, 1.f, InnerOutlineAlpha)))),
		FMargin(InnerOutlineInset));
	UOverlaySlot* EdgeSlot = Frame->AddChildToOverlay(MakeSized(Tree, MakeImage(Tree, RectBrush(Line)), LitEdgeWidth));
	EdgeSlot->SetHorizontalAlignment(HAlign_Left);
	EdgeSlot->SetVerticalAlignment(VAlign_Fill);
	EdgeSlot->SetPadding(FMargin(1.f, LitEdgeTop, 0.f, LitEdgeBottom));
	return Frame;
}

UWidget* LooterUI::AddFocusFrame(UWidget* Box)
{
	UOverlay* Overlay = Cast<UOverlay>(Box);
	UWidgetTree* Tree = Overlay ? Cast<UWidgetTree>(Overlay->GetOuter()) : nullptr;
	UWidget* Frame = MakeFocusFrame(Tree, false, true);
	if (!Frame)
	{
		return nullptr;
	}
	// MakeShapeBox's layers are the fill, the outline and then the content: the look goes under the content, so the wash
	// never tints its text. A row already on screen only takes a child on top (inserting needs a rebuild to show).
	const int32 UnderContent = FMath::Min(2, Overlay->GetChildrenCount());
	UPanelSlot* NewSlot = Overlay->GetCachedWidget().IsValid() ? Overlay->AddChild(Frame) : Overlay->InsertChildAt(UnderContent, Frame);
	if (UOverlaySlot* FrameSlot = Cast<UOverlaySlot>(NewSlot))
	{
		FillOverlaySlot(FrameSlot);
	}
	return Frame;
}

void LooterUI::SetFocusLit(UWidget* Light, bool bLit)
{
	if (ULooterButton* Button = Cast<ULooterButton>(Light))
	{
		Button->SetFocusLit(bLit);
		return;
	}
	if (Light)
	{
		const ESlateVisibility Wanted = bLit ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Light->GetVisibility() != Wanted)
		{
			Light->SetVisibility(Wanted);
		}
	}
}

bool LooterUI::IsFocusLit(const UWidget* Light)
{
	if (const ULooterButton* Button = Cast<ULooterButton>(Light))
	{
		return Button->IsFocusLit();
	}
	return Light && Light->GetVisibility() != ESlateVisibility::Collapsed;
}

void LooterUI::SetFocusGlow(UWidget* Light, float Glow)
{
	if (ULooterButton* Button = Cast<ULooterButton>(Light))
	{
		Button->SetFocusGlow(Glow);
	}
	else if (Light && Light->GetRenderOpacity() != Glow)
	{
		Light->SetRenderOpacity(Glow);
	}
}

float LooterUI::FocusGlowAt(float Seconds)
{
	if (Seconds <= ArriveSeconds)
	{
		return 1.f;
	}
	const float Wave = 0.5f * (1.f + FMath::Cos(2.f * UE_PI * BreathHz * (Seconds - ArriveSeconds)));
	return FMath::Lerp(BreathLow, 1.f, Wave);
}
