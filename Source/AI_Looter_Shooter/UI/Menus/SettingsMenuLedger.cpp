// USettingsMenuWidget's rows (E2 "Big Dial", mockups/e.js MODE settings): each a big band of glass, the bands alternating
// light and dark, a salt hairline along each foot; the number in its own column, the name, the control at the right; the
// row marked lit by a gold wash with a gold edge at its left (SettingsMenuLedgerMotion.cpp marks it). The switches are
// boxes cut at two corners, the gold one the setting. SettingsPageArt.h paints the pieces.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsPageArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace SettingsMenu;
using namespace SettingsMenu::Layout;

namespace
{
	/** A key list heading's room above and below, and its indent (lined up with the rows' numbers). */
	constexpr float SectionTop = 26.f;
	constexpr float SectionBottom = 10.f;
	constexpr float SectionIndent = 34.f;
}

ULooterButton* USettingsMenuWidget::MakeArtButton(FName Action, int32 Index, const FString& Label, int32 FontSize,
	LooterUI::EButtonKind SoundKind, USettingsPageArt** OutLabel)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	USettingsPageArt* Back = nullptr;
	USettingsPageArt* Front = nullptr;
	UOverlay* Look = SettingsPageArt::MakeLayered(WidgetTree, ESettingsPageArt::Switch, Button, Back, Front);
	Front->SetLabel(Label.ToUpper(), FontSize, SwitchSpacing);
	Button->SetupContent(Look, Action, Index, LooterUI::EButtonKind::Bare);
	// The look is the painting's; the sound stays what the kit's button of that kind made (a switch's or a tab's click).
	Button->ClickCue = Action == ActionClose ? FName(LooterSoundCue::Close) : ULooterButton::ClickCueFor(Action, SoundKind);
	Button->OnButtonClicked.BindUObject(this, &USettingsMenuWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &USettingsMenuWidget::HandleButtonHovered);
	if (OutLabel)
	{
		*OutLabel = Front;
	}
	return Button;
}

UTextBlock* USettingsMenuWidget::MakeRowName(const FString& Label, int32 Size, const FLinearColor& Tint)
{
	// One line, always (the user's check, 2026-10-31: no name wraps): the controls leave room for the longest, "Field of
	// view (1st person)" beside the slider; clipped at its column, should a longer one ever come, rather than run under them.
	UTextBlock* Name = LooterUI::MakeText(WidgetTree, Label, Size, Tint);
	Name->SetAutoWrapText(false);
	Name->SetClipping(EWidgetClipping::ClipToBounds);
	return Name;
}

UWidget* USettingsMenuWidget::MakeLedgerRow(UWidget* Content, UTextBlock* Name, float Height, bool bNumbered, bool bKeyList,
	const FLinearColor& NameColor)
{
	using namespace LooterUI;
	UOverlay* Row = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	// The band: the page's rows alternate a lighter and a darker glass. Its hairline takes the pointer for the whole row,
	// so pointing anywhere on it marks it.
	const bool bOdd = PageBandCount.IsValidIndex(BuildPage) && (PageBandCount[BuildPage]++ % 2 == 1);
	USettingsPageArt* BandBack = nullptr;
	USettingsPageArt* BandFront = nullptr;
	UOverlay* Band = SettingsPageArt::MakeLayered(WidgetTree, ESettingsPageArt::RowBand, nullptr, BandBack, BandFront);
	BandBack->SetOdd(bOdd);
	FillOverlaySlot(Row->AddChildToOverlay(Band));

	// Over it, hidden until the row is marked (or has the keys' focus): the gold wash and the gold edge at its left.
	USettingsPageArt* LightBack = nullptr;
	USettingsPageArt* LightFront = nullptr;
	UOverlay* Light = SettingsPageArt::MakeLayered(WidgetTree, ESettingsPageArt::RowLight, nullptr, LightBack, LightFront);
	Light->SetVisibility(ESlateVisibility::Collapsed);
	FillOverlaySlot(Row->AddChildToOverlay(Light));

	// The number in its column, then the row itself, clear of the band's right end.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* Number = nullptr;
	if (bNumbered && PageRowNumbers.IsValidIndex(BuildPage))
	{
		Number = MakeText(WidgetTree, FString::Printf(TEXT("%02d"), PageRowNumbers[BuildPage]++), bKeyList ? KeyNumberSize : RowNumberSize,
			Color::SlateIcon());
		Number->SetJustification(ETextJustify::Center);
	}
	UHorizontalBoxSlot* NumberSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Number, SettingsPageArt::NumberWidth));
	NumberSlot->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* ContentSlot = Line->AddChildToHorizontalBox(Content);
	ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ContentSlot->SetVerticalAlignment(VAlign_Center);
	ContentSlot->SetPadding(FMargin(0.f, 0.f, SettingsPageArt::RowPadRight, 0.f));
	USizeBox* Sized = MakeSized(WidgetTree, Line, 0.f);
	Sized->SetMinDesiredHeight(Height);
	FillOverlaySlot(Row->AddChildToOverlay(Sized));

	FSettingsLedgerLine& Entry = LedgerLines.AddDefaulted_GetRef();
	Entry.Row = Row;
	Entry.Light = Light;
	Entry.Number = Number;
	Entry.Name = Name;
	Entry.NameColor = NameColor;
	Entry.Page = BuildPage;
	Entry.bKeyList = bKeyList;
	return Row;
}

UWidget* USettingsMenuWidget::LedgerLightOf(const UWidget* Row) const
{
	const FSettingsLedgerLine* Found = LedgerLines.FindByPredicate([Row](const FSettingsLedgerLine& Line) { return Line.Row.Get() == Row; });
	return Found ? Found->Light.Get() : nullptr;
}

UWidget* USettingsMenuWidget::MakeLedgerSection(const FString& Label)
{
	using namespace LooterUI;
	// A heading in from the rows' numbers, gold and spaced like the help card's label.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* TitleSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, SectionSize, Color::SlateFlavor(), true, SectionSpacing));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	TitleSlot->SetPadding(FMargin(SectionIndent, SectionTop, 0.f, SectionBottom));
	return Line;
}
