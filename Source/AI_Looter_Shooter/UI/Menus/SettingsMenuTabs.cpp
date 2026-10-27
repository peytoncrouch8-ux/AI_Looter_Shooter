// USettingsMenuWidget's tabs and description column (the user, 2026-10-27: tabbed pages instead of one long scroll, and
// what each setting does shown as it's pointed at). The tabs turn on a click, or LB / RB (Page Up / Down) round the ends;
// each page is its own scrolling list in a switcher. The column beside the page says what the row under the pointer does,
// or the row with the focus while keys or a gamepad drive the menu; its words are SettingsMenuHelp.cpp's.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Audio/LooterSound.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
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
	/** The gap between two tabs. */
	constexpr float TabGap = 4.f;
	/** A key cap at an end of the tabs: its height, and the room between it and the tabs. */
	constexpr float TabKeyHeight = 22.f;
	constexpr float TabKeyGap = 10.f;
	/** The cyan line under the tabs and under the column's title. */
	constexpr float HairlineThickness = 1.f;
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

UWidget* USettingsMenuWidget::MakeTabBar()
{
	using namespace LooterUI;
	UHorizontalBox* Strip = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// A key cap at each end, the inventory's key hints' look (RefreshTabKeys names the keys for the device).
	auto AddKeyCap = [this, Strip](TObjectPtr<UTextBlock>& OutText, const FMargin& CapPadding)
	{
		OutText = MakeText(WidgetTree, TEXT(""), 9, Color::Title(), true, 40);
		UWidget* Cap = MakeShapeBox(WidgetTree, EShape::Control, Color::Plate(), Color::ScreenLine(), OutText, FMargin(6.f, 2.f));
		UHorizontalBoxSlot* CapSlot = Strip->AddChildToHorizontalBox(MakeSized(WidgetTree, Cap, 0.f, TabKeyHeight));
		CapSlot->SetVerticalAlignment(VAlign_Center);
		CapSlot->SetPadding(CapPadding);
	};
	AddKeyCap(TabKeyPrevious, FMargin(0.f, 0.f, TabKeyGap, 0.f));
	TabButtons.Reset();
	for (int32 Page = 0; Page < NumPages; ++Page)
	{
		ULooterButton* Tab = MakeButton(ActionTab, Page, PageName(Page), 13, EButtonKind::Tab);
		TabButtons.Add(Tab);
		UHorizontalBoxSlot* TabSlot = Strip->AddChildToHorizontalBox(MakeSized(WidgetTree, Tab, TabWidth));
		TabSlot->SetPadding(FMargin(Page > 0 ? TabGap : 0.f, 0.f, 0.f, 0.f));
		// Pointed at, a tab says what its page holds.
		Describe(Tab, PageHelp(Page), /*bEveryPage*/ true);
	}
	AddKeyCap(TabKeyNext, FMargin(TabKeyGap, 0.f, 0.f, 0.f));
	RefreshTabKeys();

	// A cyan line under the tabs, the pages' edge.
	UVerticalBox* Bar = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Bar->AddChildToVerticalBox(Strip);
	Bar->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::ScreenLine())), 0.f, HairlineThickness))
		->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	return Bar;
}

UWidget* USettingsMenuWidget::MakeHelpColumn()
{
	using namespace LooterUI;
	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	HelpTitle = MakeText(WidgetTree, TEXT(""), 13, Color::Accent(), true, 150);
	HelpTitle->SetAutoWrapText(true);
	Lines->AddChildToVerticalBox(HelpTitle);
	Lines->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::ScreenLine())), 0.f, HairlineThickness))
		->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	HelpBody = MakeText(WidgetTree, TEXT(""), 14, Color::Text());
	HelpBody->SetAutoWrapText(true);
	Lines->AddChildToVerticalBox(HelpBody)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	HelpDetail = MakeText(WidgetTree, TEXT(""), 12, Color::TextDim());
	HelpDetail->SetAutoWrapText(true);
	Lines->AddChildToVerticalBox(HelpDetail)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// A glass plate the height of the page (its fill follows the UI transparency setting; MakePlate marks it).
	UWidget* Plate = MakePlate(WidgetTree, Lines, FMargin(14.f, 12.f));
	Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
	return MakeSized(WidgetTree, Plate, HelpColumnWidth);
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
		PageNav[BuildPage].Add(FMenuNavRow{ FMenuNavItem::MakeSlider(Slider, LooterUI::AddFocusFrame(Row), Row, BuildPage) });
	}
	return Row;
}

UWidget* USettingsMenuWidget::NavChoices(UWidget* Row, const TArray<ULooterButton*>& Choices)
{
	if (Row && !Choices.IsEmpty() && !Choices.Contains(nullptr) && PageNav.IsValidIndex(BuildPage))
	{
		PageNav[BuildPage].Add(FMenuNavRow{ FMenuNavItem::MakeChoices(Choices, LooterUI::AddFocusFrame(Row), Row, BuildPage) });
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
		TabKeyPrevious->SetText(FText::FromString(bPad ? TEXT("LB") : TEXT("PGUP")));
	}
	if (TabKeyNext)
	{
		TabKeyNext->SetText(FText::FromString(bPad ? TEXT("RB") : TEXT("PGDN")));
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
