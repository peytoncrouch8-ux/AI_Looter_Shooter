// USettingsMenuWidget's tabs and pages (the user, 2026-10-27: tabbed pages instead of one long scroll; E2 "Big Dial",
// 2026-10-30: the tabs hang as the gun card's pennants off a long ruler across the page, the lit one gold on a gold tick,
// PGUP / PGDN at the ruler's ends). The tabs turn on a click, or LB / RB (Page Up / Down) round the ends; each page is its own
// scrolling list in a switcher. The help card beside the pages is SettingsMenuHelpCard.cpp.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuIcons.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsPageArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

using namespace SettingsMenu;
using namespace SettingsMenu::Layout;

namespace
{
	/** The gap between a tab's icon and its word (e.js: 12). */
	constexpr float TabIconGap = 12.f;
	/** The rows' list's top rule (e.js: 1.5 px of salt at .3). */
	constexpr float ListRuleHeight = 1.5f;
	constexpr float ListRuleOpacity = 0.3f;

	/** A tab's look: plain, pointed at, lit (its page on show). */
	enum class ETabLook : uint8 { Plain, Pointed, Lit };

	/** Page's icon (PauseMenuIcons' ids are the pages' names) in Tint, drawn TabIconSize at its own shape. */
	FSlateBrush TabIconBrush(int32 Page, ETabLook Look)
	{
		using namespace LooterUI;
		const FString Name = PageName(Page);
		const FPaintedIcon Icon = PauseMenuIcons::Get(FName(*Name));
		const FLinearColor Tint = Look == ETabLook::Lit ? Color::SoulGold() : Look == ETabLook::Pointed ? Color::SlateText() : Color::SlateIcon();
		const TCHAR* Shade = Look == ETabLook::Lit ? TEXT("Gold") : Look == ETabLook::Pointed ? TEXT("Text") : TEXT("Grey");
		const float Longest = FMath::Max(1.f, static_cast<float>(FMath::Max(Icon.ViewBox.X, Icon.ViewBox.Y)));
		const FVector2D Size = Icon.ViewBox * (TabIconSize / Longest);
		return PaintedIconBrush(FName(*FString::Printf(TEXT("SettingsTabIcon_%s_%s"), *Name, Shade)), SettingsPageArt::Recoloured(Icon, Tint),
			2.f * TabIconSize / Longest, Size);
	}

	/** A pennant's look as two images, its glass (PauseMenuArt's Fill part, a background) under its lines. */
	UOverlay* MakePennantLook(UWidgetTree* Tree, PauseMenuArt::ELook Shape, const TCHAR* Name)
	{
		using namespace LooterUI;
		const FPaintedIcon Fill = PauseMenuArt::Pennant(SettingsPageArt::PennantWidth, SettingsPageArt::PennantHeight, Shape, PauseMenuArt::EPart::Fill);
		const FPaintedIcon Lines = PauseMenuArt::Pennant(SettingsPageArt::PennantWidth, SettingsPageArt::PennantHeight, Shape, PauseMenuArt::EPart::Lines);
		UOverlay* Look = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* Back = MakeImage(Tree, PaintedIconBrush(FName(*FString::Printf(TEXT("%sFill"), Name)), Fill, 2.f, Fill.ViewBox));
		MarkBackground(Back);
		FillOverlaySlot(Look->AddChildToOverlay(Back));
		FillOverlaySlot(Look->AddChildToOverlay(MakeImage(Tree, PaintedIconBrush(FName(*FString::Printf(TEXT("%sLines"), Name)), Lines, 2.f,
			Lines.ViewBox))));
		Look->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Look;
	}
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

UWidget* USettingsMenuWidget::MakeTabBar()
{
	using namespace LooterUI;
	UOverlay* Strip = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// The ruler across the whole strip; under it the PGUP cap, the four tabs hanging from it, the PGDN cap.
	UOverlaySlot* RulerSlot = Strip->AddChildToOverlay(USettingsPageArt::Make(WidgetTree, ESettingsPageArt::TabRuler, ESettingsPagePart::Front));
	RulerSlot->SetHorizontalAlignment(HAlign_Fill);
	RulerSlot->SetVerticalAlignment(VAlign_Top);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	auto AddKeyCap = [this, Row](TObjectPtr<USettingsPageArt>& OutKey)
	{
		USettingsPageArt* Front = nullptr;
		UHorizontalBoxSlot* CapSlot = Row->AddChildToHorizontalBox(SettingsPageArt::MakeKeyCap(WidgetTree, TEXT(""), nullptr, Front));
		CapSlot->SetVerticalAlignment(VAlign_Top);
		CapSlot->SetPadding(FMargin(0.f, TabKeyTop, 0.f, 0.f));
		OutKey = Front;
	};
	AddKeyCap(TabKeyPrevious);

	// The pennant, plain and lit (PauseMenuArt). Its view box keeps Pad round its lines and may hold its hook above the
	// pennant's own height: the pennant is anchored by its foot, so its top lands TabHangBottom down the strip, on the tick.
	const FPaintedIcon PlainPicture = PauseMenuArt::Pennant(SettingsPageArt::PennantWidth, SettingsPageArt::PennantHeight, PauseMenuArt::ELook::Plain);
	const FVector2D Box = PlainPicture.ViewBox;
	const float BodyTop = static_cast<float>(Box.Y) - LooterReticle::Pad - SettingsPageArt::PennantHeight;

	TabButtons.Reset();
	TabCells.Reset();
	TabPlainLooks.Reset();
	TabLitLooks.Reset();
	TabIcons.Reset();
	TabWords.Reset();
	for (int32 Page = 0; Page < NumPages; ++Page)
	{
		ULooterButton* Tab = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		UOverlay* Look = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UOverlay* Plain = MakePennantLook(WidgetTree, PauseMenuArt::ELook::Plain, TEXT("SettingsPennantPlain"));
		UOverlay* Lit = MakePennantLook(WidgetTree, PauseMenuArt::ELook::Lit, TEXT("SettingsPennantLit"));
		Lit->SetVisibility(ESlateVisibility::Collapsed);
		FillOverlaySlot(Look->AddChildToOverlay(Plain));
		FillOverlaySlot(Look->AddChildToOverlay(Lit));

		// The page's icon and name across the pennant's upper part (its point is below).
		UHorizontalBox* Label = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UImage* Icon = MakeImage(WidgetTree, TabIconBrush(Page, ETabLook::Plain));
		Label->AddChildToHorizontalBox(Icon)->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Word = MakeText(WidgetTree, PageName(Page), TabWordSize, Color::SlateLabel(), true, TabWordSpacing);
		UHorizontalBoxSlot* WordSlot = Label->AddChildToHorizontalBox(Word);
		WordSlot->SetVerticalAlignment(VAlign_Center);
		WordSlot->SetPadding(FMargin(TabIconGap, 0.f, 0.f, 0.f));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* LabelSlot = Look->AddChildToOverlay(Label);
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Top);
		LabelSlot->SetPadding(FMargin(0.f, BodyTop + TabLabelTop, 0.f, 0.f));

		Tab->SetupContent(MakeSized(WidgetTree, Look, static_cast<float>(Box.X), static_cast<float>(Box.Y)), ActionTab, Page, EButtonKind::Bare);
		// The tab's sound stays a tab's (UI.Tab).
		Tab->ClickCue = ULooterButton::ClickCueFor(ActionTab, EButtonKind::Tab);
		Tab->OnButtonClicked.BindUObject(this, &USettingsMenuWidget::HandleButton);
		Tab->OnButtonHovered.BindUObject(this, &USettingsMenuWidget::HandleButtonHovered);
		// Pointed at, a tab says what its page holds.
		Describe(Tab, PageHelp(Page), /*bEveryPage*/ true);

		// The cell: the tick it hangs on from the ruler, the pennant under it.
		UOverlay* Cell = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USettingsPageArt* Tick = USettingsPageArt::Make(WidgetTree, ESettingsPageArt::HangTick, ESettingsPagePart::Front);
		Tick->SetButton(Tab);
		FillOverlaySlot(Cell->AddChildToOverlay(Tick));
		UOverlaySlot* TabSlot = Cell->AddChildToOverlay(Tab);
		TabSlot->SetHorizontalAlignment(HAlign_Center);
		TabSlot->SetVerticalAlignment(VAlign_Top);
		TabSlot->SetPadding(FMargin(0.f, SettingsPageArt::TabHangBottom - BodyTop, 0.f, 0.f));
		UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Cell);
		CellSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CellSlot->SetVerticalAlignment(VAlign_Fill);

		TabButtons.Add(Tab);
		TabCells.Add(Cell);
		TabPlainLooks.Add(Plain);
		TabLitLooks.Add(Lit);
		TabIcons.Add(Icon);
		TabWords.Add(Word);
	}
	AddKeyCap(TabKeyNext);
	RefreshTabKeys();
	TabLookShown.Init(MAX_uint8, NumPages);

	FillOverlaySlot(Strip->AddChildToOverlay(Row));
	return Strip;
}

UVerticalBox* USettingsMenuWidget::BeginPage()
{
	using namespace LooterUI;
	UScrollBox* List = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	StyleScrollBox(List);
	// The wheel scrolls it a whole row at a time (NativeOnMouseWheel, SettingsMenuScroll.cpp), never by its own amount.
	List->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
	BuildRows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// The list's top rule, a salt hairline over its first band.
	BuildRows->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::SaltLine().CopyWithNewOpacity(ListRuleOpacity))),
		0.f, ListRuleHeight));
	List->AddChild(BuildRows);
	if (Pages)
	{
		Pages->AddChild(List);
	}
	PageLists.Add(List);
	PageNav.AddDefaulted();
	// Each page numbers its rows from 01 and starts its bands on the lighter glass.
	PageRowNumbers.Add(1);
	PageBandCount.Add(0);
	BuildPage = PageLists.Num() - 1;
	return BuildRows;
}

UWidget* USettingsMenuWidget::AddPageRow(UWidget* Row, float Top)
{
	if (BuildRows && Row)
	{
		BuildRows->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
	}
	return Row;
}

UWidget* USettingsMenuWidget::NavSlider(UWidget* Row, USlider* Slider)
{
	if (Row && Slider && PageNav.IsValidIndex(BuildPage))
	{
		PageNav[BuildPage].Add(FMenuNavRow{ FMenuNavItem::MakeSlider(Slider, LedgerLightOf(Row), Row, BuildPage) });
	}
	return Row;
}

UWidget* USettingsMenuWidget::NavChoices(UWidget* Row, const TArray<ULooterButton*>& Choices)
{
	if (Row && !Choices.IsEmpty() && !Choices.Contains(nullptr) && PageNav.IsValidIndex(BuildPage))
	{
		PageNav[BuildPage].Add(FMenuNavRow{ FMenuNavItem::MakeChoices(Choices, LedgerLightOf(Row), Row, BuildPage) });
	}
	return Row;
}

void USettingsMenuWidget::NavButton(ULooterButton* Button, UWidget* Row)
{
	if (Button && PageNav.IsValidIndex(BuildPage))
	{
		PageNav[BuildPage].Add(FMenuNavRow{ FMenuNavItem::MakeButton(Button, BuildPage, Row) });
	}
}

UWidget* USettingsMenuWidget::Describe(UWidget* Target, const FSettingHelp& Help, bool bEveryPage, bool bQuality, bool bKeyList)
{
	if (Target)
	{
		FHelpEntry& Entry = HelpEntries.AddDefaulted_GetRef();
		Entry.Target = Target;
		Entry.Help = Help;
		Entry.Page = bEveryPage ? INDEX_NONE : BuildPage;
		Entry.bQuality = bQuality;
		Entry.bKeyList = bKeyList;
	}
	return Target;
}

// ---------------------------------------------------------------------------
// The tabs
// ---------------------------------------------------------------------------

void USettingsMenuWidget::ShowPage(int32 Page)
{
	if (!PageLists.IsValidIndex(Page) || !Pages)
	{
		return;
	}
	// Nothing held by a key carries over to the other page.
	StopNavHold();
	const int32 Before = CurrentPage;
	CurrentPage = Page;
	Pages->SetActiveWidgetIndex(Page);
	Body = PageLists[Page];
	Body->ScrollToStart();
	// The list from its top, nothing of the last page's scroll carried over (SettingsMenuScroll.cpp).
	ScrollSetOffset = 0.f;
	ScrollTarget = -1.f;
	ScrollIdle = 0.f;
	for (int32 Index = 0; Index < TabButtons.Num(); ++Index)
	{
		TabButtons[Index]->SetHighlighted(Index == Page);
	}
	RefreshTabLooks();
	// The card says what the page is for until a row is pointed at; the focus goes to the page's first setting.
	ShowHelpIcon(Page);
	ShowHelp(PageHelp(Page), /*bPageWords*/ true);
	RebuildNavigation(FirstPageStop());
	if (Page != Before)
	{
		BeginPageMotion(Page > Before ? 1 : -1);
	}
}

void USettingsMenuWidget::StepPage(int32 Step)
{
	if (Step == 0)
	{
		return;
	}
	// Round the ends: RB from the last tab is the first (and the dial still clicks forward).
	const int32 Next = (CurrentPage + (Step > 0 ? 1 : NumPages - 1)) % NumPages;
	LooterSound::Play2D(this, LooterSoundCue::Tab);
	ShowPage(Next);
	BeginPageMotion(Step > 0 ? 1 : -1);
}

void USettingsMenuWidget::SelectPage(int32 Page)
{
	ShowPage(FMath::Clamp(Page, 0, NumPages - 1));
}

ULooterButton* USettingsMenuWidget::GetTabButton(int32 Page) const
{
	return TabButtons.IsValidIndex(Page) ? TabButtons[Page].Get() : nullptr;
}

void USettingsMenuWidget::RefreshTabKeys()
{
	const bool bPad = LooterUI::IsMenuGamepad();
	// Word key names in capitals, as the hints' caps show them (the kit's font has no arrow glyphs).
	if (TabKeyPrevious)
	{
		TabKeyPrevious->SetLabel(bPad ? TEXT("LB") : TEXT("PGUP"), SettingsPageArt::KeyCapFontSize, SettingsPageArt::KeyCapSpacing);
	}
	if (TabKeyNext)
	{
		TabKeyNext->SetLabel(bPad ? TEXT("RB") : TEXT("PGDN"), SettingsPageArt::KeyCapFontSize, SettingsPageArt::KeyCapSpacing);
	}
}

void USettingsMenuWidget::RefreshTabLooks()
{
	using namespace LooterUI;
	if (TabLookShown.Num() != TabButtons.Num())
	{
		TabLookShown.Init(MAX_uint8, TabButtons.Num());
	}
	for (int32 Page = 0; Page < TabButtons.Num(); ++Page)
	{
		const ULooterButton* Tab = TabButtons[Page];
		const ETabLook Look = Tab && Tab->IsHighlighted() ? ETabLook::Lit : Tab && Tab->IsHovered() ? ETabLook::Pointed : ETabLook::Plain;
		if (TabLookShown[Page] == static_cast<uint8>(Look))
		{
			continue;
		}
		TabLookShown[Page] = static_cast<uint8>(Look);
		const bool bLit = Look == ETabLook::Lit;
		if (TabPlainLooks.IsValidIndex(Page) && TabPlainLooks[Page])
		{
			TabPlainLooks[Page]->SetVisibility(bLit ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
		if (TabLitLooks.IsValidIndex(Page) && TabLitLooks[Page])
		{
			TabLitLooks[Page]->SetVisibility(bLit ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (TabIcons.IsValidIndex(Page) && TabIcons[Page])
		{
			TabIcons[Page]->SetBrush(TabIconBrush(Page, Look));
		}
		if (TabWords.IsValidIndex(Page) && TabWords[Page])
		{
			TabWords[Page]->SetColorAndOpacity(FSlateColor(bLit ? Color::SoulGold() : Look == ETabLook::Pointed ? Color::SlateText() : Color::SlateLabel()));
		}
	}
}

// ---------------------------------------------------------------------------
// The mouse over the rows
// ---------------------------------------------------------------------------

void USettingsMenuWidget::AdvanceHoverLook()
{
	// Under the mouse, a slider or choices row pointed at takes the focus (its light is the row marked: AdvanceLook), so the
	// keys carry on from it once they are used. A button takes it as it's pointed at (HandleButtonHovered).
	if (Nav.IsShown() || ListeningIndex != INDEX_NONE || !PageNav.IsValidIndex(CurrentPage))
	{
		return;
	}
	auto TryRows = [this](const TArray<FMenuNavRow>& Rows)
	{
		for (const FMenuNavRow& NavRow : Rows)
		{
			for (const FMenuNavItem& Item : NavRow)
			{
				const UWidget* Row = Item.Row.Get();
				if (!Item.IsButton() && Row && Row->IsHovered())
				{
					if (Nav.GetFocusedWidget() != Item.GetWidget() && Nav.FocusOn(Item.GetWidget()))
					{
						RefreshPrompts();
					}
					return true;
				}
			}
		}
		return false;
	};
	if (!TryRows(PageNav[CurrentPage]) && CurrentPage == static_cast<int32>(ESettingsPage::Controls))
	{
		TryRows(KeyNav);
	}
}
