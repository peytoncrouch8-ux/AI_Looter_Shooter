// USettingsMenuWidget's tabs and description column (the user, 2026-10-27: tabbed pages instead of one long scroll, and
// what each setting does shown as it's pointed at). The tabs turn on a click, or LB / RB (Page Up / Down) round the ends;
// each page is its own scrolling list in a switcher. The column beside the page says what the row under the pointer does,
// or the row with the focus while keys or a gamepad drive the menu; its words are SettingsMenuHelp.cpp's.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Cards/LooterHollowKey.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
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

namespace
{
	/** The room between a hollow key at an end of the tabs and the tabs. */
	constexpr float TabKeyGap = 12.f;
	/** A tab's height (its word in the middle). */
	constexpr float TabHeight = 40.f;
	/** The ruler under the tabs (c3: a tick every 10 px, every fifth long). */
	constexpr float TabRulerStep = 10.f;
	constexpr int32 TabRulerLongEvery = 5;
	/** The description column's short ruler at its left (c3: about three rows tall), and the words' room from it. */
	constexpr float ColumnRulerWidth = 14.f;
	constexpr float ColumnRulerHeight = 220.f;
	constexpr float ColumnTextGap = 10.f;
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

UWidget* USettingsMenuWidget::MakeTabBar()
{
	using namespace LooterUI;
	// c3's tab strip: a hollow key at each end (RefreshTabKeys names them for the device), the words spaced between them,
	// the lit one gold; a ruler under them, and the gold mark under the lit tab that slides to it (AdvanceLook).
	UHorizontalBox* Strip = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	auto AddKeyCap = [this, Strip](TObjectPtr<ULooterHollowKey>& OutKey, const FMargin& CapPadding)
	{
		OutKey = WidgetTree->ConstructWidget<ULooterHollowKey>(ULooterHollowKey::StaticClass());
		OutKey->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBoxSlot* CapSlot = Strip->AddChildToHorizontalBox(OutKey);
		CapSlot->SetVerticalAlignment(VAlign_Center);
		CapSlot->SetPadding(CapPadding);
	};
	AddKeyCap(TabKeyPrevious, FMargin(0.f, 0.f, TabKeyGap, 0.f));
	TabButtons.Reset();
	for (int32 Page = 0; Page < NumPages; ++Page)
	{
		// The tab's sound stays a tab's (UI.Tab), its look the painted word.
		ULooterButton* Tab = MakeArtButton(ActionTab, Page, PageName(Page), ESettingsArt::Tab, 18, 167, EButtonKind::Tab);
		TabButtons.Add(Tab);
		UHorizontalBoxSlot* TabSlot = Strip->AddChildToHorizontalBox(MakeSized(WidgetTree, Tab, TabWidth, TabHeight));
		TabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TabSlot->SetVerticalAlignment(VAlign_Center);
		// Pointed at, a tab says what its page holds.
		Describe(Tab, PageHelp(Page), /*bEveryPage*/ true);
	}
	AddKeyCap(TabKeyNext, FMargin(TabKeyGap, 0.f, 0.f, 0.f));
	RefreshTabKeys();

	UVerticalBox* Bar = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Bar->AddChildToVerticalBox(Strip);
	const float RulerLength = PanelWidth - PlatePadX * 2.f - LooterReticle::Pad * 2.f;
	const FPaintedIcon Ruler = LooterReticle::Ruler(RulerLength, TabRulerStep, TabRulerLongEvery, false);
	UImage* RulerImage = MakeImage(WidgetTree, PaintedIconBrush(TEXT("SettingsTabRuler"), Ruler, 2.f, Ruler.ViewBox));
	TabRuler = RulerImage;
	UVerticalBoxSlot* RulerSlot = Bar->AddChildToVerticalBox(RulerImage);
	RulerSlot->SetHorizontalAlignment(HAlign_Left);
	RulerSlot->SetPadding(FMargin(-LooterReticle::Pad, 6.f, 0.f, 0.f));

	TabBar = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(TabBar->AddChildToOverlay(Bar));
	UCanvasPanel* Marks = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Marks->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillOverlaySlot(TabBar->AddChildToOverlay(Marks));
	TabMark = USettingsMenuArt::Make(WidgetTree, ESettingsArt::TabMark);
	// Hidden until the tabs have been drawn once and the mark knows where the lit one is.
	TabMark->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* MarkSlot = Marks->AddChildToCanvas(TabMark);
	MarkSlot->SetAutoSize(false);
	MarkSlot->SetSize(FVector2D(SettingsMenuArt::TabMarkWidth, SettingsMenuArt::TabMarkHeight));
	return TabBar;
}

UWidget* USettingsMenuWidget::MakeHelpColumn()
{
	using namespace LooterUI;
	// c3's description column: no glass, a short vertical ruler at its left, the row's name gold and spaced, its words.
	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	HelpTitle = MakeText(WidgetTree, TEXT(""), 17, Color::SoulGold(), true, 235);
	HelpTitle->SetAutoWrapText(true);
	Lines->AddChildToVerticalBox(HelpTitle);
	HelpBody = MakeText(WidgetTree, TEXT(""), 16, Color::SlateText());
	HelpBody->SetFont(Font(16, false));
	HelpBody->SetAutoWrapText(true);
	Lines->AddChildToVerticalBox(HelpBody)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	HelpDetail = MakeText(WidgetTree, TEXT(""), 14, Color::SlateLabel());
	HelpDetail->SetFont(Font(14, false));
	HelpDetail->SetAutoWrapText(true);
	Lines->AddChildToVerticalBox(HelpDetail)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	UHorizontalBox* Column = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Column->SetVisibility(ESlateVisibility::HitTestInvisible);
	UHorizontalBoxSlot* RulerSlot = Column->AddChildToHorizontalBox(MakeSized(WidgetTree,
		USettingsMenuArt::Make(WidgetTree, ESettingsArt::ColumnRuler), ColumnRulerWidth, ColumnRulerHeight));
	RulerSlot->SetVerticalAlignment(VAlign_Top);
	UHorizontalBoxSlot* LinesSlot = Column->AddChildToHorizontalBox(Lines);
	LinesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LinesSlot->SetPadding(FMargin(ColumnTextGap, 4.f, 0.f, 0.f));
	return MakeSized(WidgetTree, Column, HelpColumnWidth);
}

UVerticalBox* USettingsMenuWidget::BeginPage()
{
	UScrollBox* List = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	LooterUI::StyleScrollBox(List);
	BuildRows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	List->AddChild(BuildRows);
	if (Pages)
	{
		Pages->AddChild(List);
	}
	PageLists.Add(List);
	PageNav.AddDefaulted();
	// The ledger numbers each page's rows from 01.
	PageRowNumbers.Add(1);
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
	CurrentPage = Page;
	Pages->SetActiveWidgetIndex(Page);
	Body = PageLists[Page];
	Body->ScrollToStart();
	for (int32 Index = 0; Index < TabButtons.Num(); ++Index)
	{
		TabButtons[Index]->SetHighlighted(Index == Page);
	}
	// The column says what the page is for until a row is pointed at; the focus goes to the page's first setting.
	ShowHelp(PageHelp(Page));
	RebuildNavigation(FirstPageStop());
}

void USettingsMenuWidget::StepPage(int32 Step)
{
	if (Step == 0)
	{
		return;
	}
	// Round the ends: RB from the last tab is the first.
	const int32 Next = (CurrentPage + (Step > 0 ? 1 : NumPages - 1)) % NumPages;
	LooterSound::Play2D(this, LooterSoundCue::Tab);
	ShowPage(Next);
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
	// Word key names in capitals, as the prompts' key caps show them (the kit's font has no arrow glyphs).
	if (TabKeyPrevious)
	{
		TabKeyPrevious->SetLabel(bPad ? TEXT("LB") : TEXT("PGUP"));
	}
	if (TabKeyNext)
	{
		TabKeyNext->SetLabel(bPad ? TEXT("RB") : TEXT("PGDN"));
	}
}

// ---------------------------------------------------------------------------
// The description column
// ---------------------------------------------------------------------------

const FHelpEntry* USettingsMenuWidget::FindHelp(const UWidget* Widget) const
{
	if (!Widget)
	{
		return nullptr;
	}
	return HelpEntries.FindByPredicate([this, Widget](const FHelpEntry& Entry)
	{
		return Entry.Target.Get() == Widget && (Entry.Page == INDEX_NONE || Entry.Page == CurrentPage);
	});
}

void USettingsMenuWidget::AdvanceHelp()
{
	if (!HelpBody)
	{
		return;
	}
	const FHelpEntry* Want = nullptr;
	const bool bKeysDrive = Nav.IsShown();
	if (bKeysDrive)
	{
		// Keys or a gamepad: the stop with the focus (its row, else its button), or the page's own words.
		if (const FMenuNavItem* Here = Nav.GetFocused())
		{
			Want = FindHelp(Here->Row.Get());
			if (!Want)
			{
				Want = FindHelp(Here->GetWidget());
			}
		}
		if (!Want)
		{
			ShowHelp(PageHelp(CurrentPage));
			return;
		}
	}
	else
	{
		// The mouse: the row or button under the pointer. Over nothing (a gap between rows, the column itself) the last
		// words stay, so they don't flicker as the pointer crosses the page.
		for (const FHelpEntry& Entry : HelpEntries)
		{
			if ((Entry.Page == INDEX_NONE || Entry.Page == CurrentPage) && Entry.Target.IsValid() && Entry.Target->IsHovered())
			{
				Want = &Entry;
				break;
			}
		}
		if (!Want)
		{
			return;
		}
	}

	FSettingHelp Help = Want->Help;
	if (Want->bQuality)
	{
		// The preset pointed at, else the one chosen (which keys pick as they move along the row), else Medium.
		int32 Preset = INDEX_NONE;
		for (int32 Index = 0; Index < QualityButtons.Num() && Preset == INDEX_NONE; ++Index)
		{
			if (!bKeysDrive && QualityButtons[Index]->IsHovered())
			{
				Preset = Index;
			}
		}
		for (int32 Index = 0; Index < QualityButtons.Num() && Preset == INDEX_NONE; ++Index)
		{
			if (QualityButtons[Index]->IsHighlighted())
			{
				Preset = Index;
			}
		}
		const EGraphicsQuality Quality = Preset >= 0 && Preset < static_cast<int32>(UE_ARRAY_COUNT(Qualities)) ? Qualities[Preset] : EGraphicsQuality::Medium;
		Help.Detail = FString::Printf(TEXT("%s: %s"), *UGraphicsSettingsSubsystem::QualityName(Quality), *QualityPresetHelp(Quality));
	}
	ShowHelp(Help);
}

void USettingsMenuWidget::AdvanceHoverLook()
{
	// Keys or a gamepad in charge (the focus look is the navigator's), or a key being waited for: no pointer look.
	UWidget* Wanted = nullptr;
	const FMenuNavItem* Hovered = nullptr;
	if (!Nav.IsShown() && ListeningIndex == INDEX_NONE && PageNav.IsValidIndex(CurrentPage))
	{
		// Rows with a focus look of their own (sliders and choices): a button wears its own hover look already.
		for (const FMenuNavRow& NavRow : PageNav[CurrentPage])
		{
			for (const FMenuNavItem& Item : NavRow)
			{
				const UWidget* Row = Item.Row.Get();
				if (!Item.IsButton() && Item.Light.IsValid() && Row && Row->IsHovered())
				{
					Wanted = Item.Light.Get();
					Hovered = &Item;
					break;
				}
			}
			if (Wanted)
			{
				break;
			}
		}
	}

	UWidget* Lit = HoverLit.Get();
	if (Lit && Lit != Wanted)
	{
		// Keys that took over on the very row the pointer lit own its look now (the navigator lit it): it stays.
		const FMenuNavItem* Here = Nav.GetFocused();
		const bool bNavOwnsIt = Nav.IsShown() && Here && Here->Light.Get() == Lit;
		if (!bNavOwnsIt)
		{
			LooterUI::SetFocusLit(Lit, false);
		}
	}
	if (Wanted && (Wanted != Lit || !LooterUI::IsFocusLit(Wanted)))
	{
		LooterUI::SetFocusLit(Wanted, true);
		// A steady look under the pointer: it doesn't breathe as the keys' does, which waits for a press.
		LooterUI::SetFocusGlow(Wanted, 1.f);
		// The keys carry on from here once they are used.
		if (Nav.FocusOn(Hovered->GetWidget()))
		{
			RefreshPrompts();
		}
	}
	HoverLit = Wanted;
}

void USettingsMenuWidget::ShowHelp(const FSettingHelp& Help)
{
	if (!HelpTitle || !HelpBody || !HelpDetail)
	{
		return;
	}
	if (Help.Title == ShownHelp.Title && Help.Body == ShownHelp.Body && Help.Detail == ShownHelp.Detail)
	{
		return;
	}
	ShownHelp = Help;
	// Set in capitals, as the kit's section titles are (MakeText capitalizes only the text it's made with).
	HelpTitle->SetText(FText::FromString(Help.Title.ToUpper()));
	HelpBody->SetText(FText::FromString(Help.Body));
	HelpDetail->SetText(FText::FromString(Help.Detail));
	HelpDetail->SetVisibility(Help.Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

FString USettingsMenuWidget::GetShownHelpTitle() const
{
	return ShownHelp.Title;
}

FString USettingsMenuWidget::GetShownHelpText() const
{
	return ShownHelp.Body;
}
