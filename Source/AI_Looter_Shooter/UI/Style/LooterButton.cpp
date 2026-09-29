#include "UI/Style/LooterButton.h"
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
	OnButtonHovered.ExecuteIfBound(this);
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
}

void ULooterButton::HandleClicked()
{
	// Hand keyboard focus straight back to the game so hotkeys keep working.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
	OnButtonClicked.ExecuteIfBound(this);
}
