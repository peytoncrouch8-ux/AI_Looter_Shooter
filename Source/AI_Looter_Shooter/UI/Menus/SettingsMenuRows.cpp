// USettingsMenuWidget's rows: switches, choices, sliders, the key list, and the minimap's size preview. Each is a banded
// row (SettingsMenuLedger.cpp) in the full page's look: the name big at the left, the cut-corner switches or the ruler
// slider with its diamond knob and value at the right (mockups/e.js MODE settings).

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsPageArt.h"
#include "UI/Style/LooterButton.h"
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
using namespace SettingsMenu::Layout;

namespace
{
	/** The key list's switches, a little shorter than a setting's, so its shorter rows keep room round them. */
	constexpr float KeySwitchHeight = 50.f;
	/** A Hold / Toggle row's name sits in under its key's. */
	constexpr float ModeIndent = 28.f;
}

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
	// The HUD minimap's glass, a brighter rim so it reads over the shaded screen, and the player's arrow in the middle.
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
	// A row that belongs to a key binding sits in under it, a step quieter.
	const FLinearColor NameColor = bKeyListRow ? Color::SlateLabel() : Color::SlateText();
	UTextBlock* Name = MakeRowName(Label, bKeyListRow ? KeyNameSize - 2 : RowNameSize, NameColor);
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Name);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(bKeyListRow ? ModeIndent : 0.f, 0.f, 0.f, 0.f));

	// The two switches, the side that's the setting gold; each sounds a switch's click as the kit's segments did.
	const int32 FontSize = bKeyListRow ? DefaultFontSize : SwitchFontSize;
	OutFirst = MakeArtButton(FirstAction, Index, FirstText, FontSize, EButtonKind::Tab);
	OutSecond = MakeArtButton(SecondAction, Index, SecondText, FontSize, EButtonKind::Tab);
	UHorizontalBox* Switch = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* FirstSlot = Switch->AddChildToHorizontalBox(bKeyListRow ? static_cast<UWidget*>(MakeSized(WidgetTree, OutFirst, 0.f, KeySwitchHeight)) : OutFirst);
	FirstSlot->SetPadding(FMargin(0.f, 0.f, SettingsPageArt::SwitchGap, 0.f));
	UHorizontalBoxSlot* SecondSlot = Switch->AddChildToHorizontalBox(bKeyListRow ? static_cast<UWidget*>(MakeSized(WidgetTree, OutSecond, 0.f, KeySwitchHeight)) : OutSecond);
	if (bKeyListRow)
	{
		// In the key list the switch fills the keys' column, over an empty Default column.
		FirstSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SecondSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Switch, KeyColumnWide))->SetVerticalAlignment(VAlign_Center);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, DefaultColumnWide))->SetPadding(FMargin(SettingsPageArt::SwitchGap, 0.f, 0.f, 0.f));
	}
	else
	{
		UHorizontalBoxSlot* SwitchSlot = Line->AddChildToHorizontalBox(Switch);
		SwitchSlot->SetVerticalAlignment(VAlign_Center);
		SwitchSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	}
	return MakeLedgerRow(Line, Name, bKeyListRow ? SettingsPageArt::KeyRowHeight : SettingsPageArt::RowHeight, /*bNumbered*/ true,
		bKeyListRow, NameColor);
}

UWidget* USettingsMenuWidget::MakeChoiceRow(const FString& Label, const TArray<FString>& Choices, FName Action, float /*Width*/,
	TArray<ULooterButton*>& OutButtons)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Name = MakeRowName(Label, RowNameSize, Color::SlateText());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Name);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// The switches in a row, each as wide as its word, the chosen one gold.
	UHorizontalBox* Segments = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		ULooterButton* Button = MakeArtButton(Action, Index, Choices[Index], SwitchFontSize, EButtonKind::Tab);
		UHorizontalBoxSlot* SegmentSlot = Segments->AddChildToHorizontalBox(Button);
		SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Choices.Num() ? SettingsPageArt::SwitchGap : 0.f, 0.f));
		OutButtons.Add(Button);
	}
	UHorizontalBoxSlot* SegmentsSlot = Line->AddChildToHorizontalBox(Segments);
	SegmentsSlot->SetVerticalAlignment(VAlign_Center);
	SegmentsSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	return MakeLedgerRow(Line, Name, SettingsPageArt::RowHeight, /*bNumbered*/ true, false, Color::SlateText());
}

UWidget* USettingsMenuWidget::MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue,
	float StepSize, TFunction<FString(float)> EndLabel)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Name = MakeRowName(Label, RowNameSize, Color::SlateText());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Name);
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// The ruler slider: no bar of the slider's own (the ruler under it is the track, its gold run out to the knob), a gold
	// diamond knob that snaps to its steps.
	FSliderStyle Style;
	const FSlateBrush NoTrack = RectBrush(FLinearColor::Transparent);
	Style.SetNormalBarImage(NoTrack);
	Style.SetHoveredBarImage(NoTrack);
	Style.SetDisabledBarImage(NoTrack);
	constexpr float Knob = SettingsPageArt::KnobSize;
	const FSlateBrush KnobBrush = PaintedIconBrush(TEXT("SettingsPageKnob"), SettingsPageArt::Knob(), 2.f, FVector2D(Knob, Knob));
	Style.SetNormalThumbImage(KnobBrush);
	Style.SetHoveredThumbImage(KnobBrush);
	Style.SetDisabledThumbImage(KnobBrush);
	Style.SetBarThickness(2.f);

	OutSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	OutSlider->SetWidgetStyle(Style);
	OutSlider->SetMinValue(MinValue);
	OutSlider->SetMaxValue(MaxValue);
	OutSlider->SetStepSize(StepSize);
	OutSlider->MouseUsesStep = true;
	// The page keeps keyboard focus (Esc goes back) and steps its sliders itself for the keys and a controller
	// (SettingsMenuNav.cpp); and the world behind keeps redrawing while the handle is dragged.
	OutSlider->IsFocusable = false;
	OutSlider->bPreventThrottling = true;

	// The ruler fills the cell; the slider lies over its top part, so the knob rides the ruler's line.
	UOverlay* Cell = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	USettingsPageArt* Track = USettingsPageArt::Make(WidgetTree, ESettingsPageArt::SliderTrack, ESettingsPagePart::Front);
	auto Words = [&EndLabel](float Value) { return EndLabel ? EndLabel(Value) : PercentText(Value); };
	Track->SetSlider(OutSlider, Words(MinValue), Words(MaxValue), Knob);
	FillOverlaySlot(Cell->AddChildToOverlay(Track));
	UOverlaySlot* SliderSlot = Cell->AddChildToOverlay(MakeSized(WidgetTree, OutSlider, 0.f, SettingsPageArt::SliderLineY * 2.f));
	SliderSlot->SetHorizontalAlignment(HAlign_Fill);
	SliderSlot->SetVerticalAlignment(VAlign_Top);
	UHorizontalBoxSlot* CellSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Cell, SettingsPageArt::SliderTrackWidth + Knob * 2.f,
		SettingsPageArt::SliderHeight));
	CellSlot->SetVerticalAlignment(VAlign_Center);
	CellSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));

	OutValue = MakeText(WidgetTree, TEXT(""), ValueSize, Color::SlateText());
	OutValue->SetJustification(ETextJustify::Right);
	UHorizontalBoxSlot* ValueSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, OutValue, ValueWidth));
	ValueSlot->SetVerticalAlignment(VAlign_Center);
	// Clear of the ruler: the cell keeps a knob's width past the ruler's end, and the end value reaches at most
	// EndLabelOverhang past it, so the value starts ValueGap or more beyond the end value (the mockup's gap).
	ValueSlot->SetPadding(FMargin(FMath::Max(0.f, ValueGap - (Knob - SettingsPageArt::EndLabelOverhang)), 0.f, 0.f, 0.f));
	return MakeLedgerRow(Line, Name, SettingsPageArt::RowHeight, /*bNumbered*/ true, false, Color::SlateText());
}

void USettingsMenuWidget::RebuildControls()
{
	using namespace LooterUI;
	if (!KeyList)
	{
		return;
	}
	KeyList->ClearChildren();
	KeyButtons.Reset();
	KeyLabels.Reset();
	ModeButtons.Reset();
	DefaultButtons.Reset();
	KeyNav.Reset();
	// The key list is the Controls page's, after its sliders: each key and its Default a row of two stops, each hold/toggle
	// switch one. Each row has its words for the help card; the old rows' go with them.
	const int32 KeySection = static_cast<int32>(ESettingsPage::Controls);
	HelpEntries.RemoveAll([](const FHelpEntry& Entry) { return Entry.bKeyList; });
	// The old rows leave the page too; the key rows are numbered and banded on from the page's sliders.
	LedgerLines.RemoveAll([](const FSettingsLedgerLine& Line) { return Line.bKeyList; });
	if (PageRowNumbers.IsValidIndex(KeySection))
	{
		PageRowNumbers[KeySection] = KeyNumberStart;
	}
	if (PageBandCount.IsValidIndex(KeySection))
	{
		PageBandCount[KeySection] = KeyBandStart;
	}
	TGuardValue<int32> DescribeOnControls(BuildPage, KeySection);

	const UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings)
	{
		KeyList->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Key bindings unavailable."), KeyNameSize, Color::Worse()))
			->SetPadding(FMargin(SettingsPageArt::NumberWidth, 16.f, 0.f, 0.f));
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
			KeyList->AddChildToVerticalBox(MakeLedgerSection(LastCategory));
		}

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Name = MakeRowName(Binding.DisplayName.ToString(), KeyNameSize, Color::SlateText());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Name);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);

		// The key, a switch showing its name (RefreshKeyLabels: it changes as it's rebound), gold while it waits for a key.
		USettingsPageArt* KeyLabel = nullptr;
		ULooterButton* KeyButton = MakeArtButton(ActionRebind, Index, TEXT(""), KeyFontSize, EButtonKind::Key, &KeyLabel);
		UHorizontalBoxSlot* KeySlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, KeyButton, KeyColumnWide, KeySwitchHeight));
		KeySlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
		KeySlot->SetVerticalAlignment(VAlign_Center);
		KeyButtons.Add(KeyButton);
		KeyLabels.Add(KeyLabel);

		ULooterButton* DefaultButton = MakeArtButton(ActionResetOne, Index, TEXT("Default"), DefaultFontSize, EButtonKind::Mini);
		UHorizontalBoxSlot* DefaultSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, DefaultButton, DefaultColumnWide, KeySwitchHeight));
		DefaultSlot->SetPadding(FMargin(SettingsPageArt::SwitchGap, 0.f, 0.f, 0.f));
		DefaultSlot->SetVerticalAlignment(VAlign_Center);
		DefaultButtons.Add(DefaultButton);

		UWidget* KeyRow = MakeLedgerRow(Line, Name, SettingsPageArt::KeyRowHeight, /*bNumbered*/ true, /*bKeyList*/ true, Color::SlateText());
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
	for (int32 KeyIndex = 0; KeyIndex < KeyButtons.Num(); ++KeyIndex)
	{
		ULooterButton* Button = KeyButtons[KeyIndex];
		if (!Button || !List.IsValidIndex(Button->Index))
		{
			continue;
		}
		const bool bListening = Button->Index == ListeningIndex;
		// Key names keep their own case ("Left Mouse Button" reads better than in capitals, and fits the column).
		const FKey Key = Bindings->GetKey(List[Button->Index].Id);
		const FString Words = bListening ? FString(TEXT("Press a key...")) : Key.GetDisplayName().ToString();
		if (KeyLabels.IsValidIndex(KeyIndex) && KeyLabels[KeyIndex])
		{
			KeyLabels[KeyIndex]->SetLabel(Words, KeyFontSize, SwitchSpacing);
		}
		Button->SetHighlighted(bListening);
	}

	for (ULooterButton* Button : ModeButtons)
	{
		if (Button && List.IsValidIndex(Button->Index))
		{
			const bool bToggle = Bindings->IsToggleMode(List[Button->Index].Id);
			Button->SetHighlighted((Button->Action == ActionToggleMode) == bToggle);
		}
	}
}
