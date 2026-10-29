#include "UI/Cards/GunCardWidget.h"
#include "UI/Cards/GunCardStyle.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "UI/Cards/LooterHollowKey.h"
#include "UI/Cards/LooterSegmentBar.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

// The gun card's build and words (its intro: GunCardWidgetMotion.cpp). Sizes are the mockup's px (Docs/Work/
// 2026-10-28-UICharacter/mockup/wide.png), used as Slate sizes the way the HUD's are (CLAUDE.md, UI rules).

using namespace LooterUI;

namespace
{
	constexpr float LeftColumn = 140.f;
	constexpr float StatLabelWidth = 86.f;
	/** The value after each bar ("45 RPM", "5", "3.1 s": the user's pick A, 2026-10-28). */
	constexpr float ValueWidth = 62.f;
	constexpr float ArrowWidth = 14.f;

	UTextBlock* Words(UWidgetTree* Tree, int32 Size, const FLinearColor& Tint, bool bBold, int32 Spacing = 0)
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

	/** The same gun, as the card sees it: a new seed, kind, level or rarity is another gun (its intro plays again). */
	bool SameGun(const FWeaponInstanceData& A, const FWeaponInstanceData& B)
	{
		return A.Seed == B.Seed && A.Definition == B.Definition && A.Level == B.Level && A.Rarity == B.Rarity && A.Named == B.Named;
	}
}

float UGunCardWidget::CardWidth(EGunCardMode InMode)
{
	return InMode == EGunCardMode::InHand ? 430.f : 470.f;
}

float UGunCardWidget::UnfoldPivotY(EGunCardMode InMode)
{
	return InMode == EGunCardMode::World ? 1.f : 0.f;
}

TSharedRef<SWidget> UGunCardWidget::RebuildWidget()
{
	EnsureBuilt();
	return Super::RebuildWidget();
}

void UGunCardWidget::EnsureBuilt()
{
	if (Root || !WidgetTree)
	{
		return;
	}
	UWidgetTree* Tree = WidgetTree;

	// The sweep's row: the maker's mark and word on the left, the verdict or the quirk on the right.
	UOverlay* Top = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UHorizontalBox* MakerRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MakerMark = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	MakerRow->AddChildToHorizontalBox(MakeSized(Tree, MakerMark, 15.f, 15.f))->SetVerticalAlignment(VAlign_Center);
	MakerText = Words(Tree, 11, Color::Text(), true, 250);
	MakerRow->AddChildToHorizontalBox(MakerText)->SetPadding(FMargin(7.f, 0.f, 0.f, 0.f));
	UOverlaySlot* MakerSlot = Top->AddChildToOverlay(MakerRow);
	MakerSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
	MakerSlot->SetVerticalAlignment(VAlign_Center);
	CornerText = Words(Tree, 12, Color::Better(), true, 200);
	UOverlaySlot* CornerSlot = Top->AddChildToOverlay(CornerText);
	CornerSlot->SetHorizontalAlignment(HAlign_Right);
	CornerSlot->SetVerticalAlignment(VAlign_Center);
	CornerSlot->SetPadding(FMargin(0.f, 0.f, 22.f, 0.f));

	UVerticalBox* Body = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	NameText = Words(Tree, 20, Color::Text(), true, 50);
	Body->AddChildToVerticalBox(NameText);
	KindText = Words(Tree, 13, Color::TextDim(), false, 50);
	Body->AddChildToVerticalBox(KindText);

	// Two columns: the damage on the left, the three bars on the right.
	UHorizontalBox* Columns = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBox* Damage = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBox* Number = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	DamageText = Words(Tree, 40, Color::Text(), true);
	Number->AddChildToHorizontalBox(DamageText)->SetVerticalAlignment(VAlign_Bottom);
	PelletText = Words(Tree, 18, Color::Text(), true);
	Number->AddChildToHorizontalBox(PelletText)->SetPadding(FMargin(2.f, 0.f, 0.f, 6.f));
	Damage->AddChildToVerticalBox(Number);
	UHorizontalBox* Under = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	DamageLabel = Words(Tree, 11, Color::TextDim(), false, 200);
	DamageLabel->SetText(FText::FromString(TEXT("DAMAGE")));
	Under->AddChildToHorizontalBox(DamageLabel)->SetVerticalAlignment(VAlign_Center);
	DamageDelta = Words(Tree, 15, Color::Better(), true);
	Under->AddChildToHorizontalBox(DamageDelta)->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
	Damage->AddChildToVerticalBox(Under)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	Columns->AddChildToHorizontalBox(MakeSized(Tree, Damage, LeftColumn));

	UVerticalBox* Stats = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	for (const LoadoutRules::EStat Stat : GunCardRules::CardStats())
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Label = Words(Tree, 11, Color::TextDim(), false, 100);
		Label->SetText(FText::FromString(GunCardRules::StatLabel(Stat)));
		Row->AddChildToHorizontalBox(MakeSized(Tree, Label, StatLabelWidth))->SetVerticalAlignment(VAlign_Center);
		ULooterSegmentBar* Bar = Tree->ConstructWidget<ULooterSegmentBar>(ULooterSegmentBar::StaticClass());
		UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(Bar);
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		BarSlot->SetPadding(FMargin(0.f, 0.f, 7.f, 0.f));
		UTextBlock* Value = Words(Tree, 12, Color::Text(), true);
		Value->SetJustification(ETextJustify::Right);
		Row->AddChildToHorizontalBox(MakeSized(Tree, Value, ValueWidth))->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Arrow = Words(Tree, 12, Color::Better(), true);
		Arrow->SetJustification(ETextJustify::Right);
		Row->AddChildToHorizontalBox(MakeSized(Tree, Arrow, ArrowWidth))->SetVerticalAlignment(VAlign_Center);
		Stats->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, StatLabels.Num() == 0 ? 4.f : 5.f, 0.f, 0.f));
		StatLabels.Add(Label);
		Bars.Add(Bar);
		Values.Add(Value);
		Arrows.Add(Arrow);
	}
	UHorizontalBoxSlot* StatsSlot = Columns->AddChildToHorizontalBox(Stats);
	StatsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	StatsSlot->SetPadding(FMargin(18.f, 0.f, 0.f, 0.f));
	Body->AddChildToVerticalBox(Columns)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

	// The one special line: its word in its colour, what it does after it.
	UHorizontalBox* Special = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	SpecialWord = Words(Tree, 13, Color::Accent(), true, 50);
	Special->AddChildToHorizontalBox(SpecialWord);
	SpecialWords = Words(Tree, 13, Color::TextDim(), false);
	SpecialWords->SetAutoWrapText(true);
	UHorizontalBoxSlot* WordsSlot = Special->AddChildToHorizontalBox(SpecialWords);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
	SpecialRow = Special;
	Body->AddChildToVerticalBox(Special)->SetPadding(FMargin(0.f, 7.f, 0.f, 0.f));


	UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(MakeSized(Tree, Top, 0.f, LooterBackdrop::SweepHeight));
	Content->AddChildToVerticalBox(Body)->SetPadding(FMargin(18.f, 8.f, 18.f, 14.f));

	ULooterCardBackdrop* OutFill = nullptr;
	ULooterCardBackdrop* OutLines = nullptr;
	UOverlay* Card = ULooterCardBackdrop::MakeLayered(Tree, ELooterBackdropShape::Card, Content, FMargin(0.f), OutFill, OutLines);
	Fill = OutFill;
	Lines = OutLines;
	Box = MakeSized(Tree, Card, CardWidth(Mode));
	// The take keys float under the card, outside its glass (the user, 2026-10-28: "like Borderlands"), centred on it.
	UVerticalBox* Stack = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Stack->AddChildToVerticalBox(Box);
	KeysRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* KeysSlot = Stack->AddChildToVerticalBox(KeysRow);
	KeysSlot->SetHorizontalAlignment(HAlign_Center);
	KeysSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	Root = Stack;
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	Tree->RootWidget = Root;
	if (bHasGun)
	{
		ApplyGun();
	}
	ApplyIntro();
}

bool UGunCardWidget::SetGun(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Compare, EGunCardMode InMode)
{
	const bool bNew = !bHasGun || !SameGun(Gun, Shown) || Mode != InMode;
	const bool bCompareChanged = (Compare != nullptr) != bHasCompare || (Compare && !SameGun(*Compare, Compared));
	if (!bNew && !bCompareChanged && Gun.Kills == Shown.Kills && Gun.bCurseLifted == Shown.bCurseLifted)
	{
		return false;
	}
	Shown = Gun;
	bHasGun = true;
	bHasCompare = Compare != nullptr;
	if (Compare)
	{
		Compared = *Compare;
	}
	Mode = InMode;
	EnsureBuilt();
	if (Root)
	{
		ApplyGun();
	}
	return bNew;
}

void UGunCardWidget::ApplyGun()
{
	const GunCardStyle::FStyle& Style = GunCardStyle::For(Shown);
	if (Box)
	{
		Box->SetWidthOverride(CardWidth(Mode));
	}
	Lines->SetSweep(&Style);

	// The sweep: the maker's mark and word in its ink.
	SetWords(MakerText, Style.Word.ToString());
	MakerText->SetColorAndOpacity(FSlateColor(Style.Ink));
	const FInkedIcon* Mark = Style.MarkIcon.IsNone() ? nullptr : FindInkedIcon(Style.MarkIcon);
	if (Mark)
	{
		MakerMark->SetBrush(InkedIconBrush(Style.MarkIcon, *Mark, FVector2D(15.f, 15.f), Style.Ink));
	}
	SetShown(MakerMark, Mark != nullptr);

	// The verdict on a card that compares (only ever boasting: "UPGRADE"), else the maker's quirk.
	const FWeaponInstanceData* Against = bHasCompare ? &Compared : nullptr;
	const FString Verdict = Against ? GunCardRules::VerdictWord(LoadoutRules::Compare(Shown, Against)) : FString();
	if (!Verdict.IsEmpty())
	{
		SetWords(CornerText, FString(TEXT("▲ ")) + Verdict);
		CornerText->SetFont(Font(12, true, 200));
		CornerText->SetColorAndOpacity(FSlateColor(Color::Better()));
	}
	else
	{
		SetWords(CornerText, Style.Quirk.ToString());
		CornerText->SetFont(Font(11, false, 150));
		CornerText->SetColorAndOpacity(FSlateColor(Color::TextDim()));
	}

	SetWords(NameText, LooterWeaponText::Name(Shown).ToUpper());
	NameText->SetColorAndOpacity(FSlateColor(LooterWeaponText::Color(Shown)));
	SetWords(KindText, GunCardRules::KindLine(Shown));

	DamageValue = Shown.Stats.Damage;
	PelletWords = GunCardRules::PelletText(Shown);
	SetWords(PelletText, PelletWords);
	SetShown(PelletText, !PelletWords.IsEmpty());
	FString Delta;
	if (Against)
	{
		const LoadoutRules::EChange Change = LoadoutRules::CompareStat(LoadoutRules::EStat::Damage, Shown.Stats, Against->Stats);
		if (Change != LoadoutRules::EChange::Same)
		{
			Delta = (Change == LoadoutRules::EChange::Better ? TEXT("▲ ") : TEXT("▼ ")) +
				LoadoutRules::DeltaText(LoadoutRules::EStat::Damage, Shown.Stats, Against->Stats);
			DamageDelta->SetColorAndOpacity(FSlateColor(Change == LoadoutRules::EChange::Better ? Color::Better() : Color::Worse()));
		}
	}
	SetWords(DamageDelta, Delta);
	SetShown(DamageDelta, !Delta.IsEmpty());

	const TConstArrayView<LoadoutRules::EStat> Stats = GunCardRules::CardStats();
	for (int32 Index = 0; Index < Stats.Num() && Index < Bars.Num(); ++Index)
	{
		Bars[Index]->SetFill(LoadoutRules::StatRating(Stats[Index], Shown.Stats));
		Bars[Index]->SetTick(Against ? LoadoutRules::StatRating(Stats[Index], Against->Stats) : -1.f);
		SetWords(Values[Index], GunCardRules::StatValueText(Stats[Index], Shown.Stats));
		FString Arrow;
		if (Against)
		{
			const LoadoutRules::EChange Change = LoadoutRules::CompareStat(Stats[Index], Shown.Stats, Against->Stats);
			if (Change != LoadoutRules::EChange::Same)
			{
				Arrow = Change == LoadoutRules::EChange::Better ? TEXT("▲") : TEXT("▼");
				Arrows[Index]->SetColorAndOpacity(FSlateColor(Change == LoadoutRules::EChange::Better ? Color::Better() : Color::Worse()));
			}
		}
		SetWords(Arrows[Index], Arrow);
	}

	const GunCardRules::FSpecialLine Special = GunCardRules::SpecialLine(Shown);
	SetShown(SpecialRow, Special.IsSet());
	if (Special.IsSet())
	{
		SetWords(SpecialWord, FString(TEXT("◆ ")) + Special.Word);
		SpecialWord->SetColorAndOpacity(FSlateColor(Special.Color));
		SetWords(SpecialWords, Special.Words);
	}
	SetShown(KeysRow, Mode == EGunCardMode::World && ShownKeys.Num() > 0);
}

void UGunCardWidget::SetKeys(const FGunCardKeys& Keys)
{
	EnsureBuilt();
	if (!KeysRow)
	{
		return;
	}
	const TArray<GunCardRules::FKeyHint> Hints = GunCardRules::KeyHints(Keys);
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
		for (const GunCardRules::FKeyHint& Hint : Hints)
		{
			ULooterHollowKey* Cap = nullptr;
			if (!Hint.Cap.IsEmpty())
			{
				Cap = WidgetTree->ConstructWidget<ULooterHollowKey>(ULooterHollowKey::StaticClass());
				Cap->SetLabel(Hint.Cap);
				Cap->SetDim(Hint.bDim && !Hint.bHold);
				KeysRow->AddChildToHorizontalBox(Cap)->SetVerticalAlignment(VAlign_Center);
			}
			// Outlined floating text: no glass behind it out here.
			UTextBlock* Word = MakeFloatingText(WidgetTree, 14, Hint.bDim ? Color::TextDim() : Color::Text(), 150);
			Word->SetText(FText::FromString(Hint.Word));
			const bool bLast = &Hint == &Hints.Last();
			KeysRow->AddChildToHorizontalBox(Word)->SetPadding(FMargin(Cap ? 7.f : 0.f, 0.f, bLast ? 0.f : 22.f, 0.f));
			KeyCaps.Add(Cap);
			KeyWords.Add(Word);
		}
	}
	// The hold's cap fills as the hold runs.
	ShownHold = FMath::Clamp(Keys.Hold, 0.f, 1.f);
	for (int32 Index = 0; Index < ShownKeys.Num() && Index < KeyCaps.Num(); ++Index)
	{
		if (KeyCaps[Index] && ShownKeys[Index].bHold)
		{
			KeyCaps[Index]->SetFill(ShownHold);
		}
	}
	SetShown(KeysRow, Mode == EGunCardMode::World && ShownKeys.Num() > 0);
}

FText UGunCardWidget::GetNameText() const { return NameText ? NameText->GetText() : FText::GetEmpty(); }
FText UGunCardWidget::GetMakerText() const { return MakerText ? MakerText->GetText() : FText::GetEmpty(); }
FText UGunCardWidget::GetDamageText() const { return DamageText ? DamageText->GetText() : FText::GetEmpty(); }

FText UGunCardWidget::GetVerdictText() const
{
	const FWeaponInstanceData* Against = bHasCompare ? &Compared : nullptr;
	return FText::FromString(bHasGun && Against ? GunCardRules::VerdictWord(LoadoutRules::Compare(Shown, Against)) : FString());
}

FText UGunCardWidget::GetSpecialText() const
{
	return FText::FromString(bHasGun ? GunCardRules::SpecialLine(Shown).Word : FString());
}

TArray<FString> UGunCardWidget::GetKeyCaps() const
{
	TArray<FString> Caps;
	if (Mode == EGunCardMode::World)
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

float UGunCardWidget::GetHoldFill() const
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

FName UGunCardWidget::GetMakerCue() const
{
	return bHasGun ? GunCardStyle::For(Shown).OpenCue : NAME_None;
}

FText UGunCardWidget::GetStatValueText(int32 Index) const
{
	return Values.IsValidIndex(Index) && Values[Index] ? Values[Index]->GetText() : FText::GetEmpty();
}
