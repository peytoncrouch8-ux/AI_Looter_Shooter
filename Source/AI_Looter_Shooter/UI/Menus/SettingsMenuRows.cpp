// USettingsMenuWidget's rows: switches, choices, sliders, the key list, and the minimap's size preview.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

using namespace SettingsMenu;

UWidget* USettingsMenuWidget::MakeMinimapPreview()
{
	using namespace LooterUI;
	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	auto AddLayer = [this, Stack](const FSlateBrush& Brush, bool bFill)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(Brush);
		UOverlaySlot* LayerSlot = Stack->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(bFill ? HAlign_Fill : HAlign_Center);
		LayerSlot->SetVerticalAlignment(bFill ? VAlign_Fill : VAlign_Center);
		return Image;
	};
	// The HUD minimap's glass, a brighter rim so it reads over the dimmed screen, and the player's arrow in the middle.
	const FLinearColor Glass = Color::ScreenBg();
	MarkBackground(AddLayer(CircleBrush(FLinearColor(Glass.R, Glass.G, Glass.B, 0.62f)), true));
	AddLayer(CircleBrush(FLinearColor::Transparent, Color::TileLine(), 2.f), true);
	// And the bezel's outer edge, which is the minimap's real footprint: the box is the map's circle, so this layer reaches
	// out of it by the bezel's width (a negative padding that ShowMinimapScale sets for the size).
	MinimapPreviewBezel = AddLayer(CircleBrush(FLinearColor::Transparent, Color::TileLine(), 2.f), true);
	AddLayer(MarkerBrush(EMarker::Arrow, FLinearColor::White), false)->SetDesiredSizeOverride(FVector2D(22.f, 22.f));
	// The caption sits in the lower part of the circle at any size (ShowMinimapScale places it).
	MinimapPreviewCaption = MakeText(WidgetTree, TEXT("Minimap"), 10, Color::Title(), true, 200);
	UOverlaySlot* CaptionSlot = Stack->AddChildToOverlay(MinimapPreviewCaption);
	CaptionSlot->SetHorizontalAlignment(HAlign_Center);
	CaptionSlot->SetVerticalAlignment(VAlign_Top);

	MinimapPreviewSize = MakeSized(WidgetTree, Stack, 0.f);
	return MinimapPreviewSize;
}

UWidget* USettingsMenuWidget::MakeToggleRow(const FString& Label, const FString& FirstText, const FString& SecondText, FName FirstAction,
	FName SecondAction, int32 Index, bool bKeyListRow, ULooterButton*& OutFirst, ULooterButton*& OutSecond)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// Rows that belong to a key binding sit indented under it, a step quieter.
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, bKeyListRow ? 13 : 14, bKeyListRow ? Color::TextDim() : Color::Text()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(bKeyListRow ? 16.f : 0.f, 0.f, 0.f, 0.f));

	// Two-segment switch; the active side is highlighted like a selected tab.
	UHorizontalBox* Switch = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	OutFirst = MakeButton(FirstAction, Index, FirstText, 12, EButtonKind::Tab);
	OutSecond = MakeButton(SecondAction, Index, SecondText, 12, EButtonKind::Tab);
	UHorizontalBoxSlot* FirstSlot = Switch->AddChildToHorizontalBox(OutFirst);
	FirstSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	FirstSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
	Switch->AddChildToHorizontalBox(OutSecond)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Same width as the key buttons; in the key list it sits in their column, above an empty "Default" column.
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Switch, KeyColumnWidth))->SetPadding(bKeyListRow ? FMargin(8.f, 0.f) : FMargin(8.f, 0.f, 0.f, 0.f));
	if (bKeyListRow)
	{
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, DefaultColumnWidth));
	}
	return MakeRow(WidgetTree, Line);
}

UWidget* USettingsMenuWidget::MakeChoiceRow(const FString& Label, const TArray<FString>& Choices, FName Action, float Width,
	TArray<ULooterButton*>& OutButtons)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, 14, Color::Text()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// Segments like the two-way switch's, the chosen one highlighted like a selected tab.
	UHorizontalBox* Segments = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		ULooterButton* Button = MakeButton(Action, Index, Choices[Index], 12, EButtonKind::Tab);
		UHorizontalBoxSlot* SegmentSlot = Segments->AddChildToHorizontalBox(Button);
		SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Choices.Num() ? 4.f : 0.f, 0.f));
		OutButtons.Add(Button);
	}
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Segments, Width))->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	return MakeRow(WidgetTree, Line);
}

UWidget* USettingsMenuWidget::MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue,
	float StepSize)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, 14, Color::Text()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// A thin cyan track with an orange handle that snaps to its steps.
	FSliderStyle Style;
	const FSlateBrush Track = RectBrush(Color::SegmentOff(), Color::RowLine(), 1.f);
	Style.SetNormalBarImage(Track);
	Style.SetHoveredBarImage(Track);
	Style.SetDisabledBarImage(Track);
	FSlateBrush Handle = RectBrush(Color::Accent(), Color::AccentDark(), 1.f);
	Handle.ImageSize = FVector2D(12.f, 22.f);
	FSlateBrush HandleHovered = RectBrush(FLinearColor::FromSRGBColor(FColor(255, 190, 90)), Color::AccentDark(), 1.f);
	HandleHovered.ImageSize = Handle.ImageSize;
	Style.SetNormalThumbImage(Handle);
	Style.SetHoveredThumbImage(HandleHovered);
	Style.SetDisabledThumbImage(Handle);
	Style.SetBarThickness(6.f);

	OutSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	OutSlider->SetWidgetStyle(Style);
	OutSlider->SetMinValue(MinValue);
	OutSlider->SetMaxValue(MaxValue);
	OutSlider->SetStepSize(StepSize);
	OutSlider->MouseUsesStep = true;
	// The menu keeps keyboard focus (Esc closes it); and the world behind keeps redrawing while the handle is dragged.
	OutSlider->IsFocusable = false;
	OutSlider->bPreventThrottling = true;

	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, OutSlider, KeyColumnWidth, 30.f))->SetPadding(FMargin(8.f, 0.f));

	OutValue = MakeText(WidgetTree, TEXT(""), 14, Color::Title());
	OutValue->SetJustification(ETextJustify::Right);
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, OutValue, 48.f))->SetVerticalAlignment(VAlign_Center);
	return MakeRow(WidgetTree, Line);
}

void USettingsMenuWidget::RebuildControls()
{
	using namespace LooterUI;
	ControlsList->ClearChildren();
	KeyButtons.Reset();
	ModeButtons.Reset();

	const UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings)
	{
		ControlsList->AddChild(MakeText(WidgetTree, TEXT("Key bindings unavailable."), 13, LooterUI::Color::Worse()));
		return;
	}

	FString LastCategory;
	const TArray<FRebindableKey>& List = Bindings->GetBindings();
	for (int32 Index = 0; Index < List.Num(); ++Index)
	{
		const FRebindableKey& Binding = List[Index];
		if (Binding.Category.ToString() != LastCategory)
		{
			LastCategory = Binding.Category.ToString();
			if (UScrollBoxSlot* HeaderSlot = Cast<UScrollBoxSlot>(ControlsList->AddChild(MakeSection(WidgetTree, LastCategory))))
			{
				HeaderSlot->SetPadding(FMargin(0.f, Index == 0 ? 0.f : 10.f, 0.f, 4.f));
			}
		}

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Binding.DisplayName.ToString(), 14, Color::Text()));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);

		ULooterButton* KeyButton = MakeButton(ActionRebind, Index, TEXT(""), 13, EButtonKind::Key);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, KeyButton, KeyColumnWidth))->SetPadding(FMargin(8.f, 0.f));
		KeyButtons.Add(KeyButton);

		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeButton(ActionResetOne, Index, TEXT("Default"), 10, EButtonKind::Mini), DefaultColumnWidth));

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(ControlsList->AddChild(MakeRow(WidgetTree, Line))))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		if (Binding.bSupportsToggle)
		{
			ULooterButton* HoldButton = nullptr;
			ULooterButton* ToggleButton = nullptr;
			UWidget* ModeRow = MakeToggleRow(FString::Printf(TEXT("%s mode"), *Binding.DisplayName.ToString()), TEXT("Hold"), TEXT("Toggle"),
				ActionHoldMode, ActionToggleMode, Index, true, HoldButton, ToggleButton);
			ModeButtons.Add(HoldButton);
			ModeButtons.Add(ToggleButton);
			if (UScrollBoxSlot* ModeSlot = Cast<UScrollBoxSlot>(ControlsList->AddChild(ModeRow)))
			{
				ModeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
			}
		}
	}
	RefreshKeyLabels();
}

void USettingsMenuWidget::RefreshKeyLabels()
{
	const UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings)
	{
		return;
	}

	const TArray<FRebindableKey>& List = Bindings->GetBindings();
	for (ULooterButton* Button : KeyButtons)
	{
		if (!List.IsValidIndex(Button->Index))
		{
			continue;
		}
		const bool bListening = Button->Index == ListeningIndex;
		const FKey Key = Bindings->GetKey(List[Button->Index].Id);
		Button->SetLabel(bListening ? FText::FromString(TEXT("Press a key...")) : Key.GetDisplayName());
		Button->SetHighlighted(bListening);
	}

	for (ULooterButton* Button : ModeButtons)
	{
		if (List.IsValidIndex(Button->Index))
		{
			const bool bToggle = Bindings->IsToggleMode(List[Button->Index].Id);
			Button->SetHighlighted((Button->Action == ActionToggleMode) == bToggle);
		}
	}
}
