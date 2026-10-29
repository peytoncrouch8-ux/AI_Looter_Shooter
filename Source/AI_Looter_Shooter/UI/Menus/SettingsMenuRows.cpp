// USettingsMenuWidget's rows: switches, choices, sliders, the key list, and the minimap's size preview. Each is a ledger
// row (SettingsMenuLedger.cpp) in round 3's look: c3's hollow choices and ruler sliders on c1's ruled, numbered lines.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

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

namespace
{
	/** A row's name (c1/c3: the label grey, set bold), and a key list row's, a step smaller and quieter. */
	constexpr int32 RowLabelSize = 20;
	constexpr int32 KeyLabelSize = 16;
	/** A choice's word (c3: 15 px, spaced 2) and the gap between choices. */
	constexpr int32 ChipFontSize = 14;
	constexpr int32 ChipSpacing = 120;
	constexpr float ChipGap = 6.f;
	/** The value beside a slider (c3: 21 px, white). */
	constexpr int32 ValueSize = 20;
	constexpr float ValueWidth = 64.f;

	/** A row's name; a long one ("Crosshair colour" beside five choices) wraps onto a second line rather than run under them. */
	UTextBlock* RowLabel(UWidgetTree* Tree, const FString& Label, int32 Size, const FLinearColor& Tint)
	{
		UTextBlock* Text = LooterUI::MakeText(Tree, Label, Size, Tint);
		Text->SetAutoWrapText(true);
		return Text;
	}
}

UWidget* USettingsMenuWidget::MakeToggleRow(const FString& Label, const FString& FirstText, const FString& SecondText, FName FirstAction,
	FName SecondAction, int32 Index, bool bKeyListRow, ULooterButton*& OutFirst, ULooterButton*& OutSecond)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// Rows that belong to a key binding sit indented under it, a step quieter.
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(RowLabel(WidgetTree, Label, bKeyListRow ? KeyLabelSize : RowLabelSize,
		bKeyListRow ? Color::SlateIcon() : Color::SlateLabel()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(bKeyListRow ? 16.f : 0.f, 0.f, 0.f, 0.f));

	// c3's two hollow choices, the side that's the setting gold; each sounds a switch's click as the kit's segments did.
	UHorizontalBox* Switch = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	OutFirst = MakeArtButton(FirstAction, Index, FirstText, ESettingsArt::Chip, ChipFontSize, ChipSpacing, EButtonKind::Tab);
	OutSecond = MakeArtButton(SecondAction, Index, SecondText, ESettingsArt::Chip, ChipFontSize, ChipSpacing, EButtonKind::Tab);
	UHorizontalBoxSlot* FirstSlot = Switch->AddChildToHorizontalBox(OutFirst);
	FirstSlot->SetPadding(FMargin(0.f, 0.f, ChipGap, 0.f));
	UHorizontalBoxSlot* SecondSlot = Switch->AddChildToHorizontalBox(OutSecond);
	if (bKeyListRow)
	{
		// In the key list the switch sits in the keys' column, above an empty "Default" column.
		FirstSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SecondSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Switch, KeyColumnWidth))->SetPadding(FMargin(8.f, 0.f));
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, DefaultColumnWidth));
	}
	else
	{
		UHorizontalBoxSlot* SwitchSlot = Line->AddChildToHorizontalBox(Switch);
		SwitchSlot->SetVerticalAlignment(VAlign_Center);
		SwitchSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	}
	return MakeLedgerRow(Line, bKeyListRow ? KeyRowHeight : LedgerRowHeight, /*bNumbered*/ true, bKeyListRow);
}

UWidget* USettingsMenuWidget::MakeChoiceRow(const FString& Label, const TArray<FString>& Choices, FName Action, float /*Width*/,
	TArray<ULooterButton*>& OutButtons)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(RowLabel(WidgetTree, Label, RowLabelSize, Color::SlateLabel()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// c3's hollow choices in a row, each as wide as its word (Width, the kit's fixed column, no longer applies), the chosen
	// one gold.
	UHorizontalBox* Segments = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		ULooterButton* Button = MakeArtButton(Action, Index, Choices[Index], ESettingsArt::Chip, ChipFontSize, ChipSpacing, EButtonKind::Tab);
		UHorizontalBoxSlot* SegmentSlot = Segments->AddChildToHorizontalBox(Button);
		SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Choices.Num() ? ChipGap : 0.f, 0.f));
		OutButtons.Add(Button);
	}
	UHorizontalBoxSlot* SegmentsSlot = Line->AddChildToHorizontalBox(Segments);
	SegmentsSlot->SetVerticalAlignment(VAlign_Center);
	SegmentsSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	return MakeLedgerRow(Line, LedgerRowHeight, /*bNumbered*/ true);
}

UWidget* USettingsMenuWidget::MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue,
	float StepSize, TFunction<FString(float)> EndLabel)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(RowLabel(WidgetTree, Label, RowLabelSize, Color::SlateLabel()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// c3's range scale: no bar of the slider's own (the ruler under it is the track, its gold run up to the knob), a gold
	// ring knob that snaps to its steps.
	FSliderStyle Style;
	const FSlateBrush NoTrack = RectBrush(FLinearColor::Transparent);
	Style.SetNormalBarImage(NoTrack);
	Style.SetHoveredBarImage(NoTrack);
	Style.SetDisabledBarImage(NoTrack);
	const FVector2D KnobBox(SettingsMenuArt::KnobSize, SettingsMenuArt::KnobSize);
	const FSlateBrush Knob = PaintedIconBrush(TEXT("SettingsSliderKnob"), SettingsMenuArt::Knob(), 2.f, KnobBox);
	Style.SetNormalThumbImage(Knob);
	Style.SetHoveredThumbImage(Knob);
	Style.SetDisabledThumbImage(Knob);
	Style.SetBarThickness(2.f);

	OutSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	OutSlider->SetWidgetStyle(Style);
	OutSlider->SetMinValue(MinValue);
	OutSlider->SetMaxValue(MaxValue);
	OutSlider->SetStepSize(StepSize);
	OutSlider->MouseUsesStep = true;
	// The menu keeps keyboard focus (Esc closes it) and steps its sliders itself for the keys and a controller
	// (SettingsMenuNav.cpp); and the world behind keeps redrawing while the handle is dragged.
	OutSlider->IsFocusable = false;
	OutSlider->bPreventThrottling = true;

	// The ruler under the slider, the same box: its knob runs over the ruler's line, its ends' values under the ends.
	UOverlay* Cell = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	USettingsMenuArt* Ruler = USettingsMenuArt::Make(WidgetTree, ESettingsArt::SliderRuler);
	auto Words = [&EndLabel](float Value) { return EndLabel ? EndLabel(Value) : PercentText(Value); };
	Ruler->SetSlider(OutSlider, Words(MinValue), Words(MaxValue), SettingsMenuArt::KnobSize);
	FillOverlaySlot(Cell->AddChildToOverlay(Ruler));
	FillOverlaySlot(Cell->AddChildToOverlay(OutSlider));
	UHorizontalBoxSlot* CellSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Cell, SliderWidth, SliderHeight));
	CellSlot->SetVerticalAlignment(VAlign_Center);
	CellSlot->SetPadding(FMargin(8.f, 0.f));

	OutValue = MakeText(WidgetTree, TEXT(""), ValueSize, Color::SlateText());
	OutValue->SetJustification(ETextJustify::Right);
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, OutValue, ValueWidth))->SetVerticalAlignment(VAlign_Center);
	return MakeLedgerRow(Line, LedgerRowHeight, /*bNumbered*/ true);
}

void USettingsMenuWidget::RebuildControls()
{
	using namespace LooterUI;
	KeyList->ClearChildren();
	KeyButtons.Reset();
	ModeButtons.Reset();
	DefaultButtons.Reset();
	KeyNav.Reset();
	// The key list is the Controls page's, after its sliders: each key and its Default a row of two stops, each hold/toggle
	// switch one. Each row has its words in the description column; the old rows' go with them.
	const int32 KeySection = static_cast<int32>(ESettingsPage::Controls);
	HelpEntries.RemoveAll([](const FHelpEntry& Entry) { return Entry.bKeyList; });
	// The old rows leave the ledger too, and the key rows are numbered on from the page's sliders.
	LedgerLines.RemoveAll([](const FSettingsLedgerLine& Line) { return Line.bKeyList; });
	if (PageRowNumbers.IsValidIndex(KeySection))
	{
		PageRowNumbers[KeySection] = KeyNumberStart;
	}
	TGuardValue<int32> DescribeOnControls(BuildPage, KeySection);

	const UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings)
	{
		KeyList->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Key bindings unavailable."), 13, LooterUI::Color::Worse()));
		RebuildNavigation();
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
			KeyList->AddChildToVerticalBox(MakeLedgerSection(LastCategory))->SetPadding(FMargin(0.f, Index == 0 ? 0.f : 8.f, 0.f, 0.f));
		}

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(RowLabel(WidgetTree, Binding.DisplayName.ToString(), KeyLabelSize + 1, Color::SlateLabel()));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);

		// The key keeps the kit's key button (it shows the key's name, which changes as it's rebound).
		ULooterButton* KeyButton = MakeButton(ActionRebind, Index, TEXT(""), 13, EButtonKind::Key);
		UHorizontalBoxSlot* KeySlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, KeyButton, KeyColumnWidth, SettingsMenuArt::ChipHeight));
		KeySlot->SetPadding(FMargin(8.f, 0.f));
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeyButtons.Add(KeyButton);

		ULooterButton* DefaultButton = MakeArtButton(ActionResetOne, Index, TEXT("Default"), ESettingsArt::Chip, 12, ChipSpacing, EButtonKind::Mini);
		UHorizontalBoxSlot* DefaultSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, DefaultButton, DefaultColumnWidth));
		DefaultSlot->SetVerticalAlignment(VAlign_Center);
		DefaultButtons.Add(DefaultButton);

		UWidget* KeyRow = MakeLedgerRow(Line, KeyRowHeight, /*bNumbered*/ true, /*bKeyList*/ true);
		KeyList->AddChildToVerticalBox(KeyRow);
		KeyNav.Add(FMenuNavRow{ FMenuNavItem::MakeButton(KeyButton, KeySection, KeyRow), FMenuNavItem::MakeButton(DefaultButton, KeySection, KeyRow) });
		Describe(KeyRow, KeyHelp(Binding.Id, Binding.DisplayName.ToString(), Binding.DefaultKey.IsValid() ? Binding.DefaultKey.GetDisplayName().ToString() : FString()),
			false, false, /*bKeyList*/ true);

		if (Binding.bSupportsToggle)
		{
			ULooterButton* HoldButton = nullptr;
			ULooterButton* ToggleButton = nullptr;
			UWidget* ModeRow = MakeToggleRow(FString::Printf(TEXT("%s mode"), *Binding.DisplayName.ToString()), TEXT("Hold"), TEXT("Toggle"),
				ActionHoldMode, ActionToggleMode, Index, true, HoldButton, ToggleButton);
			ModeButtons.Add(HoldButton);
			ModeButtons.Add(ToggleButton);
			KeyList->AddChildToVerticalBox(ModeRow);
			KeyNav.Add(FMenuNavRow{ FMenuNavItem::MakeChoices({ HoldButton, ToggleButton }, LedgerLightOf(ModeRow), ModeRow, KeySection) });
			Describe(ModeRow, KeyModeHelp(Binding.DisplayName.ToString()), false, false, /*bKeyList*/ true);
		}
	}
	RefreshKeyLabels();
	RebuildNavigation();
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
