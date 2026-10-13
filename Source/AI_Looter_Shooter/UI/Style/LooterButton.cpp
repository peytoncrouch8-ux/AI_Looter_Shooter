#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"

ULooterButton::ULooterButton()
{
	InitIsFocusable(false);
}

void ULooterButton::Setup(UTextBlock* InLabel, FName InAction, int32 InIndex, const FText& Text, int32 FontSize, LooterUI::EButtonKind InKind)
{
	Action = InAction;
	Index = InIndex;
	Kind = InKind;
	Label = InLabel;
	// Key names ("Left Mouse Button") read better in their natural case; everything else is uppercase.
	bUppercase = Kind != LooterUI::EButtonKind::Key;

	UWidgetTree* Tree = Cast<UWidgetTree>(GetOuter());
	UOverlay* Content = Tree ? Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass()) : nullptr;
	Body = Content;
	if (Content)
	{
		Outline = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Outline->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* OutlineSlot = Content->AddChildToOverlay(Outline);
		OutlineSlot->SetHorizontalAlignment(HAlign_Fill);
		OutlineSlot->SetVerticalAlignment(VAlign_Fill);

		if (Label)
		{
			Label->SetFont(LooterUI::Font(FontSize, true, bUppercase ? 150 : 0));
			Label->SetJustification(ETextJustify::Center);
			UOverlaySlot* LabelSlot = Content->AddChildToOverlay(Label);
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(12.f, 6.f));
		}

		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(AddChild(Content)))
		{
			ContentSlot->SetPadding(FMargin(0.f));
			ContentSlot->SetHorizontalAlignment(HAlign_Fill);
			ContentSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	SetLabel(Text);
	ApplyStyle();
	OnClicked.AddDynamic(this, &ULooterButton::HandleClicked);
	OnHovered.AddDynamic(this, &ULooterButton::HandleHovered);
}

void ULooterButton::SetupContent(UWidget* Content, FName InAction, int32 InIndex, LooterUI::EButtonKind InKind)
{
	Action = InAction;
	Index = InIndex;
	Kind = InKind;
	if (UButtonSlot* ContentSlot = Content ? Cast<UButtonSlot>(AddChild(Content)) : nullptr)
	{
		ContentSlot->SetPadding(FMargin(0.f));
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}
	ApplyStyle();
	OnClicked.AddDynamic(this, &ULooterButton::HandleClicked);
	OnHovered.AddDynamic(this, &ULooterButton::HandleHovered);
}

void ULooterButton::HandleHovered()
{
	if (bPlaysSounds && GetIsEnabled())
	{
		LooterSound::Play2D(this, LooterSoundCue::Hover);
	}
	OnButtonHovered.ExecuteIfBound(this);
}

FName ULooterButton::ClickCueFor(FName InAction, LooterUI::EButtonKind InKind)
{
	static const FName Close(TEXT("Close"));
	static const FName Back(TEXT("Back"));
	if (InAction == Close || InAction == Back || InAction.ToString().StartsWith(TEXT("Cancel")))
	{
		return LooterSoundCue::Back;
	}
	return InKind == LooterUI::EButtonKind::Tab ? FName(LooterSoundCue::Tab) : FName(LooterSoundCue::Click);
}

void ULooterButton::SetLabel(const FText& Text)
{
	RawText = Text;
	if (Label)
	{
		Label->SetText(bUppercase ? FText::FromString(Text.ToString().ToUpper()) : Text);
	}
}

void ULooterButton::SetHighlighted(bool bHighlighted)
{
	if (bIsHighlighted != bHighlighted)
	{
		bIsHighlighted = bHighlighted;
		ApplyStyle();
	}
}

void ULooterButton::ApplyStyle()
{
	SetStyle(LooterUI::ButtonStyle(Kind, bIsHighlighted));
	if (Outline)
	{
		Outline->SetBrush(LooterUI::ShapeBrush(LooterUI::EShape::Control, true, LooterUI::ButtonLineColor(Kind, bIsHighlighted)));
	}
	if (Label)
	{
		Label->SetColorAndOpacity(FSlateColor(LooterUI::ButtonTextColor(Kind, bIsHighlighted)));
	}
	if (FocusFrame)
	{
		// Lit or not, the look may need its other colour (a key button lit orange while it waits for a key).
		UpdateFocusFrame();
	}
}

bool ULooterButton::Press()
{
	if (!GetIsEnabled())
	{
		return false;
	}
	// Heard before the handler runs, as a click's is. Unlike a click, the keyboard focus isn't handed to the game: the menu
	// that pressed it keeps it.
	if (bPlaysSounds)
	{
		LooterSound::Play2D(this, ClickCue.IsNone() ? ClickCueFor(Action, Kind) : ClickCue);
	}
	OnButtonClicked.ExecuteIfBound(this);
	return true;
}

void ULooterButton::SetFocusLit(bool bLit)
{
	if (bFocusLit != bLit || (bLit && !FocusFrame))
	{
		bFocusLit = bLit;
		UpdateFocusFrame();
	}
}

void ULooterButton::SetFocusGlow(float Glow)
{
	if (FocusFrame)
	{
		LooterUI::SetFocusGlow(FocusFrame, Glow);
	}
}

void ULooterButton::UpdateFocusFrame()
{
	// Orange buttons (Primary, or lit) take the warm white look; the rest the orange one.
	const bool bOnAccent = bIsHighlighted || Kind == LooterUI::EButtonKind::Primary;
	if (FocusFrame && bFocusFrameOnAccent != bOnAccent)
	{
		FocusFrame->RemoveFromParent();
		FocusFrame = nullptr;
	}
	if (bFocusLit && !FocusFrame && Body)
	{
		// Made the first time it's needed, so the many buttons never focused (the HUD's, the inventory's) carry none. Over
		// the label (a live overlay only takes children on top), but only at the edges, clear of the centred text.
		FocusFrame = LooterUI::MakeFocusFrame(Cast<UWidgetTree>(GetOuter()), bOnAccent, false);
		if (FocusFrame)
		{
			LooterUI::FillOverlaySlot(Body->AddChildToOverlay(FocusFrame));
			bFocusFrameOnAccent = bOnAccent;
		}
	}
	if (FocusFrame)
	{
		LooterUI::SetFocusLit(FocusFrame, bFocusLit);
	}
}

void ULooterButton::HandleClicked()
{
	// Heard before the handler runs: it may close the menu, or leave the level.
	if (bPlaysSounds)
	{
		LooterSound::Play2D(this, ClickCue.IsNone() ? ClickCueFor(Action, Kind) : ClickCue);
	}
	// Hand keyboard focus straight back to the game so hotkeys keep working.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
	OnButtonClicked.ExecuteIfBound(this);
}
