// USettingsMenuWidget's header (E2 "Big Dial", mockups/e.js MODE settings): the pause menu's dial, smaller and cut by the
// screen's top-left corner, its needle pointing right at the title; SETTINGS big over a ruler; the breadcrumb under it,
// PAUSED / SETTINGS over the game or MAIN MENU / SETTINGS from the main menu. The dial turns as the page opens and clicks a
// tick as a tab turns (SettingsMenuLedgerMotion.cpp).

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsPageArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Widgets/Layout/Anchors.h"

using namespace SettingsMenu;
using namespace SettingsMenu::Layout;

namespace
{
	/** The breadcrumb's slash sits a full em clear of its words either side: the mockup's .5em margin on top of its .5em
	 *  letter spacing, which Slate doesn't add after a word's last letter ("PAUSED/SETTINGS" read as one word in game). */
	constexpr float CrumbSlashGap = CrumbSize * 1.f;
	/** The ruler 12 px under the title, the crumb 4 px under the ruler (e.js lockup). */
	constexpr float TitleRulerGap = 12.f;
	constexpr float CrumbGap = 4.f;
}

void USettingsMenuWidget::MakeHeader()
{
	using namespace LooterUI;

	// The dial: the main menu's salt ring and its ticks, the fine inner ring, the gold needle at three o'clock on its pivot.
	// Only the corner of it shows; its middle is just inside the screen's left edge, level with the title.
	const FPaintedIcon DialPicture = PauseMenuArt::Dial(DialRadius, 0.f);
	Dial = MakeImage(WidgetTree, PaintedIconBrush(TEXT("SettingsPageDial"), DialPicture, 2.f, DialPicture.ViewBox));
	PlaceOnPage(Dial, FAnchors(0.f, 0.f), FVector2D(DialCentreX, DialCentreY), FVector2D(0.5f, 0.5f));

	// SETTINGS in the floating outlined type the HUD uses, spaced wide, over the ruler and the crumb.
	UVerticalBox* Block = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Title = MakeFloatingText(WidgetTree, TitleSize, Color::SlateText(), TitleSpacing);
	Title->SetText(FText::FromString(TEXT("SETTINGS")));
	Block->AddChildToVerticalBox(Title);
	USettingsPageArt* Ruler = USettingsPageArt::Make(WidgetTree, ESettingsPageArt::TitleRuler, ESettingsPagePart::Front);
	UVerticalBoxSlot* RulerSlot = Block->AddChildToVerticalBox(Ruler);
	RulerSlot->SetHorizontalAlignment(HAlign_Left);
	RulerSlot->SetPadding(FMargin(0.f, TitleRulerGap, 0.f, 0.f));

	// The breadcrumb: where the page was opened from in the label grey, the slash quieter, SETTINGS gold (RefreshCrumb words it).
	UHorizontalBox* Crumb = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	CrumbFirst = MakeText(WidgetTree, TEXT("PAUSED"), CrumbSize, Color::SlateLabel(), true, CrumbSpacing);
	Crumb->AddChildToHorizontalBox(CrumbFirst);
	UHorizontalBoxSlot* SlashSlot = Crumb->AddChildToHorizontalBox(MakeText(WidgetTree, TEXT("/"), CrumbSize, Color::SlateIcon(), false, CrumbSpacing));
	SlashSlot->SetPadding(FMargin(CrumbSlashGap, 0.f, CrumbSlashGap, 0.f));
	Crumb->AddChildToHorizontalBox(MakeText(WidgetTree, TEXT("SETTINGS"), CrumbSize, Color::SoulGold(), true, CrumbSpacing));
	Block->AddChildToVerticalBox(Crumb)->SetPadding(FMargin(0.f, CrumbGap, 0.f, 0.f));

	Block->SetVisibility(ESlateVisibility::HitTestInvisible);
	TitleBlock = Block;
	PlaceOnPage(Block, FAnchors(0.f, 0.f), FVector2D(TitleLeft, TitleTop), FVector2D::ZeroVector);
}

void USettingsMenuWidget::RefreshCrumb()
{
	if (CrumbFirst)
	{
		CrumbFirst->SetText(FText::FromString(Mode == ESettingsMenuMode::Pause ? TEXT("PAUSED") : TEXT("MAIN MENU")));
	}
}

FString USettingsMenuWidget::GetBreadcrumb() const
{
	return CrumbFirst ? FString::Printf(TEXT("%s / SETTINGS"), *CrumbFirst->GetText().ToString()) : FString();
}
