// USettingsMenuWidget's help card (the user, 2026-10-27: what each setting does, shown as it's pointed at; E2 "Big Dial",
// 2026-10-30: the gun card's shape on the page's right, the page's icon in its gem on top): POINTED AT, the row's name, a
// rule, what it does in plain words (SettingsMenuHelp.cpp) and, gold and slanted, what it costs or its range. It follows
// the focus under keys or a gamepad, the row pointed at (else the row lit) under the mouse.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuIcons.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsPageArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIStyle.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Widgets/Layout/Anchors.h"

using namespace SettingsMenu;
using namespace SettingsMenu::Layout;

namespace
{
	/** The card's words as the mockup sets them (e.js: the label 16 px spaced .3em, the name 32 px spaced .12em, the words 23
	 *  and the cost 21 px); the card grows to fit them (SizeHelpCard). */
	constexpr int32 CardLabelSize = 16;
	constexpr int32 CardLabelSpacing = 300;
	constexpr int32 CardTitleSize = 32;
	constexpr int32 CardTitleSpacing = 120;
	constexpr int32 CardBodySize = 23;
	constexpr int32 CardDetailSize = 21;
	constexpr float BodyLineHeight = 1.1f;
	/** The rule's room above and below, the cost line's above, and the words' room above the card's cut foot. */
	constexpr float RuleTop = 12.f;
	constexpr float RuleBottom = 14.f;
	constexpr float DetailTop = 14.f;
	// 26 plus a line of the slanted cost text: Slate wraps the sheared line one line more than the measure (seen 2026-10-31).
	constexpr float WordsBottom = 66.f;
	/** Italic by shear: Chakra Petch ships no italic (the gun card's flavour line does the same); its lines wrap this much
	 *  shorter and sit in by half of it, so the slant's top and foot lines stay inside the card. */
	constexpr float ItalicShear = -12.f;
	constexpr float ShearRoom = 16.f;
	/** The card grows in these steps (each height is a picture of its own), and its top gem (e.js: 48 px). */
	constexpr float CardHeightStep = 24.f;
	constexpr float GemSize = 48.f;

	/** The card's glass or lines at Height (PauseMenuArt's card with its top gem's raised step), drawn once per height. */
	FSlateBrush CardBrush(float Height, PauseMenuArt::EPart Part)
	{
		const LooterUI::FPaintedIcon Picture = PauseMenuArt::Card(HelpCardWidth, Height, /*bTopGem*/ true, /*bPointLeft*/ false, 0.f, Part);
		const TCHAR* Kind = Part == PauseMenuArt::EPart::Fill ? TEXT("Fill") : TEXT("Lines");
		return LooterUI::PaintedIconBrush(FName(*FString::Printf(TEXT("SettingsHelpCard%s_%d"), Kind, FMath::RoundToInt32(Height))), Picture, 2.f,
			Picture.ViewBox);
	}
}

void USettingsMenuWidget::MakeHelpCard()
{
	using namespace LooterUI;
	// The gun card's shape (PauseMenuArt): its glass (marked, the UI transparency fades it) under its lines. Its view box
	// keeps Pad round the card, whose top (the gem's raised step) is at Pad. SizeHelpCard makes it as tall as its words.
	const float Pad = LooterReticle::Pad;
	UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	HelpCardFill = MakeImage(WidgetTree, CardBrush(HelpCardHeight, PauseMenuArt::EPart::Fill));
	MarkBackground(HelpCardFill);
	FillOverlaySlot(Card->AddChildToOverlay(HelpCardFill));
	HelpCardLines = MakeImage(WidgetTree, CardBrush(HelpCardHeight, PauseMenuArt::EPart::Lines));
	FillOverlaySlot(Card->AddChildToOverlay(HelpCardLines));
	HelpCardShownHeight = HelpCardHeight;
	bHelpCardSized = false;

	// The gem on top (e.js: 48 px, lit gold), the page's icon in its middle (ShowHelpIcon).
	UOverlay* Gem = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	const FPaintedIcon GemFill = PauseMenuArt::Gem(GemSize, true, PauseMenuArt::EPart::Fill);
	const FPaintedIcon GemLines = PauseMenuArt::Gem(GemSize, true, PauseMenuArt::EPart::Lines);
	UImage* GemGlass = MakeImage(WidgetTree, PaintedIconBrush(TEXT("SettingsHelpGemFill"), GemFill, 2.f, GemFill.ViewBox));
	MarkBackground(GemGlass);
	FillOverlaySlot(Gem->AddChildToOverlay(GemGlass));
	FillOverlaySlot(Gem->AddChildToOverlay(MakeImage(WidgetTree, PaintedIconBrush(TEXT("SettingsHelpGemLines"), GemLines, 2.f, GemLines.ViewBox))));
	HelpIcon = MakeImage(WidgetTree, FSlateBrush());
	UOverlaySlot* IconSlot = Gem->AddChildToOverlay(HelpIcon);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	UOverlaySlot* GemSlot = Card->AddChildToOverlay(MakeSized(WidgetTree, Gem, GemSize, GemSize));
	GemSlot->SetHorizontalAlignment(HAlign_Center);
	GemSlot->SetVerticalAlignment(VAlign_Top);
	GemSlot->SetPadding(FMargin(0.f, Pad + HelpGemCentreY - GemSize * 0.5f, 0.f, 0.f));

	// The words, wrapped at the card's inner width (a set width, so they can be measured before they are drawn).
	const float WordsWidth = HelpCardWidth - HelpPadLeft - HelpPadRight;
	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	HelpLabel = MakeText(WidgetTree, TEXT("POINTED AT"), CardLabelSize, Color::SlateFlavor(), true, CardLabelSpacing);
	Words->AddChildToVerticalBox(HelpLabel);
	HelpTitle = MakeText(WidgetTree, TEXT(""), CardTitleSize, Color::SlateText(), true, CardTitleSpacing);
	HelpTitle->SetWrapTextAt(WordsWidth);
	Words->AddChildToVerticalBox(HelpTitle)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	Words->AddChildToVerticalBox(MakeSized(WidgetTree, USettingsPageArt::Make(WidgetTree, ESettingsPageArt::Ornament, ESettingsPagePart::Front), 0.f,
		SettingsPageArt::OrnamentHeight))->SetPadding(FMargin(0.f, RuleTop, 0.f, RuleBottom));
	HelpBody = MakeText(WidgetTree, TEXT(""), CardBodySize, Color::SlateText());
	HelpBody->SetFont(Font(CardBodySize, false));
	HelpBody->SetWrapTextAt(WordsWidth);
	HelpBody->SetLineHeightPercentage(BodyLineHeight);
	Words->AddChildToVerticalBox(HelpBody);
	HelpDetail = MakeText(WidgetTree, TEXT(""), CardDetailSize, Color::SlateFlavor());
	HelpDetail->SetFont(Font(CardDetailSize, false));
	HelpDetail->SetWrapTextAt(WordsWidth - ShearRoom);
	HelpDetail->SetLineHeightPercentage(BodyLineHeight);
	FWidgetTransform Slant;
	Slant.Shear = FVector2D(ItalicShear, 0.f);
	HelpDetail->SetRenderTransform(Slant);
	Words->AddChildToVerticalBox(HelpDetail)->SetPadding(FMargin(ShearRoom * 0.5f, DetailTop, 0.f, 0.f));
	HelpWords = Words;
	UOverlaySlot* WordsSlot = Card->AddChildToOverlay(Words);
	WordsSlot->SetHorizontalAlignment(HAlign_Fill);
	WordsSlot->SetVerticalAlignment(VAlign_Top);
	WordsSlot->SetPadding(FMargin(Pad + HelpPadLeft, Pad + HelpPadTop, Pad + HelpPadRight, 0.f));

	Card->SetVisibility(ESlateVisibility::HitTestInvisible);
	HelpCardBox = MakeSized(WidgetTree, Card, HelpCardWidth + Pad * 2.f, HelpCardHeight + Pad * 2.f);
	HelpCard = HelpCardBox;
	// Its right edge on the page's right margin, its top (the gem's step) at the mockup's 424; it grows downward.
	PlaceOnPage(HelpCardBox, FAnchors(1.f, 0.f), FVector2D(Pad - PageMargin, HelpCardTop - Pad), FVector2D(1.f, 0.f));
}

void USettingsMenuWidget::SizeHelpCard()
{
	if (!HelpWords || !HelpCardBox || !HelpCardFill || !HelpCardLines || !HelpWords->GetCachedWidget().IsValid())
	{
		return;
	}
	// The words as they now read (wrapped at their set width), measured now rather than after they're drawn.
	HelpWords->ForceLayoutPrepass();
	const float WordsHeight = static_cast<float>(HelpWords->GetDesiredSize().Y);
	if (WordsHeight <= 1.f)
	{
		return;
	}
	bHelpCardSized = true;
	// As tall as its words need (never shorter than the mockup's three rows), and never down into the key hints: from the
	// card's top to the rows' list's foot.
	const float Needed = HelpPadTop + WordsHeight + WordsBottom;
	const float Most = ListRoom() + (RowsTop - HelpCardTop);
	const float Height = FMath::Min(FMath::Max(HelpCardHeight, FMath::CeilToFloat(Needed / CardHeightStep) * CardHeightStep), Most);
	if (FMath::IsNearlyEqual(Height, HelpCardShownHeight, 0.5f))
	{
		return;
	}
	HelpCardShownHeight = Height;
	HelpCardFill->SetBrush(CardBrush(Height, PauseMenuArt::EPart::Fill));
	HelpCardLines->SetBrush(CardBrush(Height, PauseMenuArt::EPart::Lines));
	HelpCardBox->SetHeightOverride(Height + LooterReticle::Pad * 2.f);
}

void USettingsMenuWidget::ShowHelpIcon(int32 Page)
{
	using namespace LooterUI;
	if (!HelpIcon)
	{
		return;
	}
	const FString Name = PageName(Page);
	const FPaintedIcon Icon = PauseMenuIcons::Get(FName(*Name));
	const float Longest = FMath::Max(1.f, static_cast<float>(FMath::Max(Icon.ViewBox.X, Icon.ViewBox.Y)));
	HelpIcon->SetBrush(PaintedIconBrush(FName(*FString::Printf(TEXT("SettingsHelpIcon_%s"), *Name)),
		SettingsPageArt::Recoloured(Icon, Color::SoulGold()), 2.f * HelpGemIconSize / Longest, Icon.ViewBox * (HelpGemIconSize / Longest)));
}

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
			ShowHelp(PageHelp(CurrentPage), /*bPageWords*/ true);
			return;
		}
	}
	else
	{
		// The mouse: the row, tab or button under the pointer; off them, the row lit (the mockup: row 01 lit, its words on
		// the card), so the card and the gold row always agree.
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
			Want = FindHelp(MarkedRow.Get());
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
	// A tab pointed at says what its page holds: those are a page's words, not a row's.
	const bool bTab = TabButtons.Contains(Cast<ULooterButton>(Want->Target.Get()));
	ShowHelp(Help, bTab);
}

void USettingsMenuWidget::ShowHelp(const FSettingHelp& Help, bool bPageWords)
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
	if (HelpLabel)
	{
		HelpLabel->SetText(FText::FromString(bPageWords ? TEXT("ON THIS PAGE") : TEXT("POINTED AT")));
	}
	// Set in capitals, as the kit's titles are (MakeText capitalizes only the text it's made with).
	HelpTitle->SetText(FText::FromString(Help.Title.ToUpper()));
	HelpBody->SetText(FText::FromString(Help.Body));
	HelpDetail->SetText(FText::FromString(Help.Detail));
	HelpDetail->SetVisibility(Help.Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	// The new words rise in (SettingsMenuLedgerMotion.cpp), and the card fits them before it is next drawn.
	bHelpCardSized = false;
	SizeHelpCard();
	HelpAge = 0.f;
}

FString USettingsMenuWidget::GetShownHelpTitle() const
{
	return ShownHelp.Title;
}

FString USettingsMenuWidget::GetShownHelpText() const
{
	return ShownHelp.Body;
}
