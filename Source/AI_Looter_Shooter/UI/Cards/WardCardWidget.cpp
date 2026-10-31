#include "UI/Cards/WardCardWidget.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UI/Cards/GunCardIcons.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "UI/Cards/LooterHollowKey.h"
#include "UI/Cards/WardCardIcons.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

// The ward card's build and words (its intro: WardCardWidgetMotion.cpp). The gun card's tag card (GunCardWidget.cpp) at its
// sizes, so a ward and a gun read as one family on the ground and in the inventory.

using namespace LooterUI;

namespace
{
	/** The ward box's mark over the name, its icon at the foot (px). */
	constexpr float TopMarkSize = 20.f;
	constexpr float KindMarkSize = 30.f;
	constexpr float IconSize = 18.f;
	constexpr float ArrowWidth = 18.f;
	/** The gap between the stats and the build and trait rows (the gun card's before its element's). */
	constexpr float ExtraRowsGap = 14.f;

	UTextBlock* Words(UWidgetTree* Tree, int32 Size, const FLinearColor& Tint, bool bBold = true, int32 Spacing = 0)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetFont(Font(Size, bBold, Spacing));
		Text->SetColorAndOpacity(FSlateColor(Tint));
		return Text;
	}

	void SetWords(UTextBlock* Text, const FString& Value)
	{
		if (Text && !Text->GetText().ToString().Equals(Value))
		{
			Text->SetText(FText::FromString(Value));
		}
	}

	void SetShown(UWidget* Widget, bool bShown)
	{
		const ESlateVisibility Wanted = bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Widget && Widget->GetVisibility() != Wanted)
		{
			Widget->SetVisibility(Wanted);
		}
	}

	UImage* Rule(UWidgetTree* Tree)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(RectBrush(Color::SoulGold().CopyWithNewOpacity(0.55f)));
		Image->SetDesiredSizeOverride(FVector2D(10.f, 2.f));
		return Image;
	}

	FName WardIconName(const FShieldInstanceData& Ward)
	{
		return Ward.Definition ? Ward.Definition->IconName : FName(TEXT("WardBox"));
	}
}

TSharedRef<SWidget> UWardCardWidget::RebuildWidget()
{
	EnsureBuilt();
	return Super::RebuildWidget();
}

void UWardCardWidget::EnsureBuilt()
{
	if (Root || !WidgetTree)
	{
		return;
	}
	UWidgetTree* Tree = WidgetTree;

	UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// Over the name, where a gun card names its maker: the ward box's mark and "RARE WARD", no box round them.
	UHorizontalBox* TopLine = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TopMark = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	TopLine->AddChildToHorizontalBox(MakeSized(Tree, TopMark, TopMarkSize, TopMarkSize))->SetVerticalAlignment(VAlign_Center);
	TopText = Words(Tree, 15, Color::SlateLabel(), true, 280);
	TopLine->AddChildToHorizontalBox(TopText)->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	UVerticalBoxSlot* TopSlot = Content->AddChildToVerticalBox(TopLine);
	TopSlot->SetHorizontalAlignment(HAlign_Center);
	TopSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	NameText = Words(Tree, 21, Color::SlateText(), true, 50);
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(NameText);

	StatsBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(StatsBox)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// The ornament: a rule, a diamond, a rule (the gun card's).
	UHorizontalBox* Orn = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* LeftRule = Orn->AddChildToHorizontalBox(Rule(Tree));
	LeftRule->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LeftRule->SetVerticalAlignment(VAlign_Center);
	Orn->AddChildToHorizontalBox(MakeImage(Tree, IconBrush(TEXT("GunCardDiamond"), GunCardIcons::Diamond(), 3.f, FVector2D(16.f, 16.f),
		Color::SoulGold())))->SetPadding(FMargin(8.f, 0.f));
	UHorizontalBoxSlot* RightRule = Orn->AddChildToHorizontalBox(Rule(Tree));
	RightRule->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RightRule->SetVerticalAlignment(VAlign_Center);
	Ornament = Orn;
	Content->AddChildToVerticalBox(Orn)->SetPadding(FMargin(0.f, 10.f, 0.f, 8.f));

	LinesBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(LinesBox);

	// The foot: the ward box in its rarity's colour, as a gun card's foot shows the gun's kind.
	KindMark = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	UVerticalBoxSlot* FootSlot = Content->AddChildToVerticalBox(MakeSized(Tree, KindMark, KindMarkSize, KindMarkSize));
	FootSlot->SetHorizontalAlignment(HAlign_Center);
	FootSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// The tag: glass and line under the content; the gem's number (the ward's level) over the notch.
	ULooterCardBackdrop* OutFill = nullptr;
	ULooterCardBackdrop* OutLines = nullptr;
	UOverlay* Card = ULooterCardBackdrop::MakeLayered(Tree, ELooterBackdropShape::Tag, Content,
		FMargin(24.f, LooterBackdrop::TagTop + 32.f, 24.f, LooterBackdrop::TagPoint + 16.f), OutFill, OutLines);
	Fill = OutFill;
	Lines = OutLines;
	GemText = Words(Tree, 15, Color::SoulGold(), true);
	GemText->SetJustification(ETextJustify::Center);
	UOverlay* GemMiddle = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UOverlaySlot* NumberSlot = GemMiddle->AddChildToOverlay(GemText);
	NumberSlot->SetHorizontalAlignment(HAlign_Center);
	NumberSlot->SetVerticalAlignment(VAlign_Center);
	USizeBox* GemBox = MakeSized(Tree, GemMiddle, LooterBackdrop::TagGem, LooterBackdrop::TagGem);
	Gem = GemBox;
	UOverlaySlot* GemSlot = Card->AddChildToOverlay(GemBox);
	GemSlot->SetHorizontalAlignment(HAlign_Center);
	GemSlot->SetVerticalAlignment(VAlign_Top);
	GemSlot->SetPadding(FMargin(0.f, LooterBackdrop::TagTop + LooterBackdrop::TagNotchDepth * 0.5f - LooterBackdrop::TagGem * 0.5f, 0.f, 0.f));
	Box = MakeSized(Tree, Card, CardWidth());

	// The take keys float under the card, outside its glass, as a gun card's.
	UVerticalBox* Stack = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Stack->AddChildToVerticalBox(Box);
	KeysRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* KeysSlot = Stack->AddChildToVerticalBox(KeysRow);
	KeysSlot->SetHorizontalAlignment(HAlign_Center);
	KeysSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	Root = Stack;
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	Tree->RootWidget = Root;
	if (bHasWard)
	{
		ApplyWard();
	}
	ApplyIntro();
}

void UWardCardWidget::EnsureRows(int32 Count)
{
	UWidgetTree* Tree = WidgetTree;
	while (RowWidgets.Num() < Count)
	{
		const int32 Index = RowWidgets.Num();
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UImage* Icon = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Row->AddChildToHorizontalBox(MakeSized(Tree, Icon, IconSize, IconSize))->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Label = Words(Tree, 14, Color::SlateLabel(), true, 50);
		Row->AddChildToHorizontalBox(Label)->SetPadding(FMargin(9.f, 0.f, 0.f, 0.f));
		Row->AddChildToHorizontalBox(Tree->ConstructWidget<USpacer>(USpacer::StaticClass()))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UTextBlock* Value = Words(Tree, 17, Color::SlateText(), true);
		Value->SetJustification(ETextJustify::Right);
		Row->AddChildToHorizontalBox(Value)->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Arrow = Words(Tree, 12, Color::Better(), true);
		Arrow->SetJustification(ETextJustify::Right);
		Row->AddChildToHorizontalBox(MakeSized(Tree, Arrow, ArrowWidth))->SetVerticalAlignment(VAlign_Center);

		// Every other row on a faint band, as the gun card's (a band is part of the card's ink, not a background).
		UBorder* Band = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Band->SetBrush(FSlateRoundedBoxBrush(Index % 2 == 0 ? Color::SlateBand() : FLinearColor::Transparent, 6.f));
		Band->SetPadding(FMargin(8.f, 3.f, 6.f, 3.f));
		Band->SetContent(Row);
		StatsBox->AddChildToVerticalBox(Band);
		RowWidgets.Add(Band);
		RowIcons.Add(Icon);
		RowLabels.Add(Label);
		RowValues.Add(Value);
		RowArrows.Add(Arrow);
	}
	for (int32 Index = 0; Index < RowWidgets.Num(); ++Index)
	{
		SetShown(RowWidgets[Index], Index < Count);
	}
}

bool UWardCardWidget::SetWard(const FShieldInstanceData& Ward, const FShieldInstanceData* Worn, EWardCardMode InMode)
{
	const bool bNew = !bHasWard || ShieldRules::Identity(Ward) != ShieldRules::Identity(Shown) || Mode != InMode;
	const bool bCompareChanged = (Worn != nullptr) != bHasCompare || (Worn && ShieldRules::Identity(*Worn) != ShieldRules::Identity(Compared));
	if (!bNew && !bCompareChanged)
	{
		return false;
	}
	Shown = Ward;
	bHasWard = true;
	bHasCompare = Worn != nullptr;
	Compared = Worn ? *Worn : FShieldInstanceData();
	Mode = InMode;
	EnsureBuilt();
	if (Root)
	{
		ApplyWard();
	}
	return bNew;
}

void UWardCardWidget::ApplyWard()
{
	const FLinearColor Rarity = ShieldText::Color(Shown);
	SetWords(GemText, FString::FromInt(Shown.Level));
	SetWords(TopText, WardCardRules::TopWord(Shown));
	SetWords(NameText, ShieldText::Name(Shown).ToUpper());
	NameText->SetColorAndOpacity(FSlateColor(Rarity));
	const FName IconName = WardIconName(Shown);
	const FInkedIcon* Inked = FindInkedIcon(IconName);
	if (Inked)
	{
		TopMark->SetBrush(InkedIconBrush(IconName, *Inked, FVector2D(TopMarkSize, TopMarkSize)));
		KindMark->SetBrush(InkedIconBrush(IconName, *Inked, FVector2D(KindMarkSize, KindMarkSize), Rarity));
	}
	SetShown(TopMark, Inked != nullptr);
	SetShown(KindMark, Inked != nullptr);

	// The stat rows against the ward worn, then the build and the trait (the gun card's element rows), after a gap.
	const TConstArrayView<WardCardRules::EStat> Stats = WardCardRules::CardStats();
	const TArray<WardCardRules::FExtraRow> Extras = WardCardRules::ExtraRows(Shown);
	EnsureRows(Stats.Num() + Extras.Num());
	for (int32 Index = 0; Index < RowWidgets.Num(); ++Index)
	{
		if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(RowWidgets[Index]->Slot))
		{
			RowSlot->SetPadding(FMargin(0.f, Index == Stats.Num() ? ExtraRowsGap : 0.f, 0.f, 0.f));
		}
	}
	const FShieldInstanceData* Against = bHasCompare ? &Compared : nullptr;
	for (int32 Index = 0; Index < Stats.Num(); ++Index)
	{
		const WardCardRules::EStat Stat = Stats[Index];
		RowIcons[Index]->SetBrush(IconBrush(WardCardIcons::StatName(Stat), WardCardIcons::Stat(Stat), 3.f, FVector2D(IconSize, IconSize), Color::SlateIcon()));
		SetWords(RowLabels[Index], WardCardRules::StatLabel(Stat));
		RowLabels[Index]->SetColorAndOpacity(FSlateColor(Color::SlateLabel()));
		SetWords(RowValues[Index], WardCardRules::StatValueText(Stat, Shown.Stats));
		RowValues[Index]->SetColorAndOpacity(FSlateColor(Color::SlateText()));
		const LoadoutRules::EChange Change = Against ? WardCardRules::CompareStat(Stat, Shown.Stats, Against->Stats) : LoadoutRules::EChange::Same;
		SetWords(RowArrows[Index], WardCardRules::ArrowText(Change));
		RowArrows[Index]->SetColorAndOpacity(FSlateColor(Change == LoadoutRules::EChange::Better ? Color::Better() : Color::Worse()));
	}
	const FLinearColor TraitTint = ShieldText::TraitColor(Shown);
	for (int32 Extra = 0; Extra < Extras.Num(); ++Extra)
	{
		const int32 Index = Stats.Num() + Extra;
		const WardCardRules::FExtraRow& Row = Extras[Extra];
		RowIcons[Index]->SetBrush(Row.bTrait ? IconBrush(TEXT("WardCardTrait"), WardCardIcons::Trait(), 3.f, FVector2D(IconSize, IconSize), TraitTint)
			: IconBrush(TEXT("WardCardBuild"), WardCardIcons::Build(), 3.f, FVector2D(IconSize, IconSize), Color::SlateIcon()));
		SetWords(RowLabels[Index], Row.Label);
		RowLabels[Index]->SetColorAndOpacity(FSlateColor(Row.bTrait ? TraitTint : Color::SlateLabel()));
		SetWords(RowValues[Index], Row.Value);
		RowValues[Index]->SetColorAndOpacity(FSlateColor(Row.bTrait ? TraitTint : Color::SlateText()));
		SetWords(RowArrows[Index], FString());
	}

	// The bullet lines: the trait's effect first, in its colour (as the gun card's element line), then the build's lean.
	const TArray<FString> Bullets = WardCardRules::Lines(Shown);
	while (LineTexts.Num() < Bullets.Num())
	{
		UTextBlock* Line = Words(WidgetTree, 13, Color::SlateText(), true);
		Line->SetAutoWrapText(true);
		LinesBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 1.f, 0.f, 1.f));
		LineTexts.Add(Line);
	}
	const bool bTraitLine = Shown.Trait != EShieldTrait::None;
	for (int32 Index = 0; Index < LineTexts.Num(); ++Index)
	{
		const bool bShownLine = Index < Bullets.Num();
		SetShown(LineTexts[Index], bShownLine);
		if (bShownLine)
		{
			SetWords(LineTexts[Index], FString(TEXT("• ")) + Bullets[Index]);
			LineTexts[Index]->SetColorAndOpacity(FSlateColor(Index == 0 && bTraitLine ? TraitTint : Color::SlateText()));
		}
	}
	SetShown(Ornament, Bullets.Num() > 0);
	SetShown(KeysRow, Mode == EWardCardMode::World && ShownKeys.Num() > 0);
}

void UWardCardWidget::SetKeys(const FWardCardKeys& Keys)
{
	EnsureBuilt();
	if (!KeysRow)
	{
		return;
	}
	const TArray<GunCardRules::FKeyHint> Hints = WardCardRules::KeyHints(Keys);
	bool bSame = Hints.Num() == ShownKeys.Num();
	for (int32 Index = 0; bSame && Index < Hints.Num(); ++Index)
	{
		bSame = Hints[Index].Cap == ShownKeys[Index].Cap && Hints[Index].Word == ShownKeys[Index].Word;
	}
	if (!bSame)
	{
		ShownKeys = Hints;
		KeysRow->ClearChildren();
		KeyCaps.Reset();
		KeyWords.Reset();
		for (int32 Index = 0; Index < Hints.Num(); ++Index)
		{
			const GunCardRules::FKeyHint& Hint = Hints[Index];
			ULooterHollowKey* Cap = nullptr;
			if (!Hint.Cap.IsEmpty())
			{
				Cap = WidgetTree->ConstructWidget<ULooterHollowKey>(ULooterHollowKey::StaticClass());
				Cap->SetLabel(Hint.Cap);
				KeysRow->AddChildToHorizontalBox(Cap)->SetVerticalAlignment(VAlign_Center);
			}
			// Outlined floating text: no glass behind it out here.
			UTextBlock* Word = MakeFloatingText(WidgetTree, 15, Hint.bDim ? Color::SlateLabel() : Color::SlateText(), 150);
			Word->SetText(FText::FromString(Hint.Word));
			const bool bLast = Index == Hints.Num() - 1;
			KeysRow->AddChildToHorizontalBox(Word)->SetPadding(FMargin(Cap ? 8.f : 0.f, 0.f, bLast ? 0.f : 24.f, 0.f));
			KeyCaps.Add(Cap);
			KeyWords.Add(Word);
		}
	}
	ShownHold = FMath::Clamp(Keys.Hold, 0.f, 1.f);
	for (int32 Index = 0; Index < ShownKeys.Num() && Index < KeyCaps.Num(); ++Index)
	{
		if (KeyCaps[Index] && ShownKeys[Index].bHold)
		{
			KeyCaps[Index]->SetFill(ShownHold);
		}
	}
	SetShown(KeysRow, Mode == EWardCardMode::World && ShownKeys.Num() > 0);
}

FText UWardCardWidget::GetNameText() const { return NameText ? NameText->GetText() : FText::GetEmpty(); }
FText UWardCardWidget::GetTopText() const { return TopText ? TopText->GetText() : FText::GetEmpty(); }

FText UWardCardWidget::GetVerdictText() const
{
	return FText::FromString(bHasWard ? WardCardRules::VerdictWord(Shown, bHasCompare ? &Compared : nullptr) : FString());
}

FText UWardCardWidget::GetStatValueText(int32 Index) const
{
	return Index < GetStatCount() && RowValues.IsValidIndex(Index) ? RowValues[Index]->GetText() : FText::GetEmpty();
}

FText UWardCardWidget::GetStatArrowText(int32 Index) const
{
	return Index < GetStatCount() && RowArrows.IsValidIndex(Index) ? RowArrows[Index]->GetText() : FText::GetEmpty();
}

TArray<FString> UWardCardWidget::GetLines() const
{
	return bHasWard ? WardCardRules::Lines(Shown) : TArray<FString>();
}

TArray<FString> UWardCardWidget::GetKeyCaps() const
{
	TArray<FString> Caps;
	if (Mode == EWardCardMode::World)
	{
		for (const GunCardRules::FKeyHint& Hint : ShownKeys)
		{
			if (!Hint.Cap.IsEmpty())
			{
				Caps.Add(Hint.Cap);
			}
		}
	}
	return Caps;
}

TArray<FString> UWardCardWidget::GetKeyWords() const
{
	TArray<FString> Out;
	if (Mode == EWardCardMode::World)
	{
		for (const GunCardRules::FKeyHint& Hint : ShownKeys)
		{
			Out.Add(Hint.Word);
		}
	}
	return Out;
}

float UWardCardWidget::GetHoldFill() const
{
	for (int32 Index = 0; Index < ShownKeys.Num() && Index < KeyCaps.Num(); ++Index)
	{
		if (KeyCaps[Index] && ShownKeys[Index].bHold)
		{
			return KeyCaps[Index]->GetFill();
		}
	}
	return 0.f;
}
