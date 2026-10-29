#include "UI/Cards/GunCardWidget.h"
#include "Elements/ElementRules.h"
#include "UI/Cards/GunCardIcons.h"
#include "UI/Cards/GunCardStyle.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "UI/Cards/LooterHollowKey.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
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

// The tag card's build and words (its intro: GunCardWidgetMotion.cpp). Sizes follow the mockup (Docs/Work/
// 2026-10-28-UICharacter/mockup/dist.png concept C, colors.png theme 6) at the HUD's text scale.

using namespace LooterUI;

namespace
{
	/** The maker's mark on the foot's badge (px). */
	constexpr float MakerMarkSize = 20.f;
	/** The element's glyph beside the badge, and the box the gun's kind fits in (px). */
	constexpr float ElementMarkSize = 22.f;
	const FVector2D KindMarkBox(58.f, 26.f);

	constexpr float IconSize = 18.f;
	constexpr float ArrowWidth = 18.f;
	/** Italic by shear: Chakra Petch ships no italic. */
	constexpr float ItalicShear = -12.f;

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

	void Italic(UWidget* Widget)
	{
		FWidgetTransform Transform;
		Transform.Shear = FVector2D(ItalicShear, 0.f);
		Widget->SetRenderTransform(Transform);
	}

	/** The same gun, as the card sees it: a new seed, kind, level or rarity is another gun (its intro plays again). */
	bool SameGun(const FWeaponInstanceData& A, const FWeaponInstanceData& B)
	{
		return A.Seed == B.Seed && A.Definition == B.Definition && A.Level == B.Level && A.Rarity == B.Rarity && A.Named == B.Named;
	}

	/** A thin rule of the ornament, fading out at its far end. */
	UImage* Rule(UWidgetTree* Tree)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(RectBrush(Color::SoulGold().CopyWithNewOpacity(0.55f)));
		Image->SetDesiredSizeOverride(FVector2D(10.f, 2.f));
		return Image;
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

	UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	NameText = Words(Tree, 21, Color::SlateText(), true, 50);
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetAutoWrapText(true);
	// No maker-and-kind line under the name (the user, 2026-10-29: removed; the maker's badge at the foot names the maker).
	Content->AddChildToVerticalBox(NameText);

	StatsBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(StatsBox)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// The ornament: a rule, a diamond, a rule.
	UHorizontalBox* Orn = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* LeftRule = Orn->AddChildToHorizontalBox(Rule(Tree));
	LeftRule->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LeftRule->SetVerticalAlignment(VAlign_Center);
	UImage* Diamond = MakeImage(Tree, IconBrush(TEXT("GunCardDiamond"), GunCardIcons::Diamond(), 3.f, FVector2D(16.f, 16.f), Color::SoulGold()));
	Orn->AddChildToHorizontalBox(Diamond)->SetPadding(FMargin(8.f, 0.f));
	UHorizontalBoxSlot* RightRule = Orn->AddChildToHorizontalBox(Rule(Tree));
	RightRule->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RightRule->SetVerticalAlignment(VAlign_Center);
	Ornament = Orn;
	Content->AddChildToVerticalBox(Orn)->SetPadding(FMargin(0.f, 10.f, 0.f, 8.f));

	LinesBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(LinesBox);
	FlavorText = Words(Tree, 13, Color::SlateFlavor(), true);
	FlavorText->SetAutoWrapText(true);
	Italic(FlavorText);
	Content->AddChildToVerticalBox(FlavorText)->SetPadding(FMargin(4.f, 4.f, 0.f, 0.f));

	// The foot: the maker's badge, centred (the element is the stat list's last row). The badge is sized to fill the tag's narrowing foot (the
	// user, 2026-10-29: "the manufacturer at the bottom should be slightly bigger and fit the open space more").
	UHorizontalBox* Foot = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MakerBadge = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
	MakerBadge->SetPadding(FMargin(16.f, 6.f, 20.f, 7.f));
	UHorizontalBox* BadgeRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	MakerMark = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	BadgeRow->AddChildToHorizontalBox(MakeSized(Tree, MakerMark, MakerMarkSize, MakerMarkSize))->SetVerticalAlignment(VAlign_Center);
	MakerText = Words(Tree, 15, Color::Text(), true, 280);
	BadgeRow->AddChildToHorizontalBox(MakerText)->SetPadding(FMargin(9.f, 0.f, 0.f, 0.f));
	MakerBadge->SetContent(BadgeRow);
	Foot->AddChildToHorizontalBox(MakerBadge)->SetVerticalAlignment(VAlign_Center);
	// Then the element's glyph and the gun's kind, like the flame and the rifle beside Borderlands' maker (the user, 2026-10-29).
	ElementMark = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	UHorizontalBoxSlot* ElementSlot = Foot->AddChildToHorizontalBox(MakeSized(Tree, ElementMark, ElementMarkSize, ElementMarkSize));
	ElementSlot->SetVerticalAlignment(VAlign_Center);
	ElementSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	KindMark = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	UHorizontalBoxSlot* KindSlot = Foot->AddChildToHorizontalBox(KindMark);
	KindSlot->SetVerticalAlignment(VAlign_Center);
	KindSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UVerticalBoxSlot* FootSlot = Content->AddChildToVerticalBox(Foot);
	FootSlot->SetHorizontalAlignment(HAlign_Center);
	FootSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));

	// The tag: glass and line under the content; the gem's number over the notch.
	ULooterCardBackdrop* OutFill = nullptr;
	ULooterCardBackdrop* OutLines = nullptr;
	const float Top = LooterBackdrop::TagTop + 32.f;
	UOverlay* Card = ULooterCardBackdrop::MakeLayered(Tree, ELooterBackdropShape::Tag, Content,
		FMargin(24.f, Top, 24.f, LooterBackdrop::TagPoint + 16.f), OutFill, OutLines);
	Fill = OutFill;
	Lines = OutLines;
	GemText = Words(Tree, 15, Color::SoulGold(), true);
	GemText->SetJustification(ETextJustify::Center);
	// The number centred in the gem's box.
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
	Box = MakeSized(Tree, Card, CardWidth(Mode));

	// The take keys float under the card, outside its glass (the user, 2026-10-28: "like Borderlands"), centred on it.
	UVerticalBox* Stack = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Stack->AddChildToVerticalBox(Box);
	KeysRow = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* KeysSlot = Stack->AddChildToVerticalBox(KeysRow);
	KeysSlot->SetHorizontalAlignment(HAlign_Center);
	KeysSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	Root = Stack;
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	Tree->RootWidget = Root;
	if (bHasGun)
	{
		ApplyGun();
	}
	ApplyIntro();
}

void UGunCardWidget::EnsureRows(int32 Count)
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

		// Every other row on a faint band, rounded, so the eye keeps to its line.
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
	if (Box)
	{
		Box->SetWidthOverride(CardWidth(Mode));
	}
	const GunCardStyle::FStyle& Style = GunCardStyle::For(Shown);

	SetWords(GemText, FString::FromInt(Shown.Level));
	SetWords(NameText, LooterWeaponText::Name(Shown).ToUpper());
	NameText->SetColorAndOpacity(FSlateColor(LooterWeaponText::Color(Shown)));

	// The stat list, each against the compared gun, then the element's rows: its glyph, label and value in its colour, as
	// Borderlands shows a burn (the user, 2026-10-29), or "Element: Normal" for a gun without one (the user, same day).
	ShownStats = GunCardRules::CardStats(Shown.Stats);
	const ELooterElement Element = WeaponParts::ElementOf(Shown);
	const FVectorIcon* ElementGlyph = GunCardIcons::Element(Element);
	const FLinearColor ElementTint = ElementGlyph ? ElementRules::UiColor(Element) : Color::SlateText();
	const TArray<GunCardRules::FElementRow> ElementRows = GunCardRules::ElementRows(Shown, FElementTuning::Live());
	EnsureRows(ShownStats.Num() + ElementRows.Num());
	const FWeaponInstanceData* Against = bHasCompare ? &Compared : nullptr;
	for (int32 Index = 0; Index < ShownStats.Num(); ++Index)
	{
		const LoadoutRules::EStat Stat = ShownStats[Index];
		RowIcons[Index]->SetBrush(IconBrush(GunCardIcons::StatName(Stat), GunCardIcons::Stat(Stat), 3.f, FVector2D(IconSize, IconSize), Color::SlateIcon()));
		SetWords(RowLabels[Index], GunCardRules::StatLabel(Stat));
		RowLabels[Index]->SetColorAndOpacity(FSlateColor(Color::SlateLabel()));
		SetWords(RowValues[Index], GunCardRules::StatValueText(Stat, Shown.Stats));
		RowValues[Index]->SetColorAndOpacity(FSlateColor(Color::SlateText()));
		FString Arrow;
		if (Against)
		{
			const LoadoutRules::EChange Change = LoadoutRules::CompareStat(Stat, Shown.Stats, Against->Stats);
			if (Change != LoadoutRules::EChange::Same)
			{
				Arrow = Change == LoadoutRules::EChange::Better ? TEXT("▲") : TEXT("▼");
				RowArrows[Index]->SetColorAndOpacity(FSlateColor(Change == LoadoutRules::EChange::Better ? Color::Better() : Color::Worse()));
			}
		}
		SetWords(RowArrows[Index], Arrow);
	}
	for (int32 Row = 0; Row < ElementRows.Num(); ++Row)
	{
		const int32 Index = ShownStats.Num() + Row;
		if (ElementGlyph)
		{
			RowIcons[Index]->SetBrush(IconBrush(GunCardIcons::ElementName(Element), *ElementGlyph, 3.f, FVector2D(IconSize, IconSize), ElementTint));
		}
		else
		{
			RowIcons[Index]->SetBrush(IconBrush(TEXT("GunCardDiamondRow"), GunCardIcons::Diamond(), 3.f, FVector2D(IconSize, IconSize), Color::SlateIcon()));
		}
		SetWords(RowLabels[Index], ElementRows[Row].Label);
		RowLabels[Index]->SetColorAndOpacity(FSlateColor(ElementGlyph ? ElementTint : Color::SlateLabel()));
		SetWords(RowValues[Index], ElementRows[Row].Value);
		RowValues[Index]->SetColorAndOpacity(FSlateColor(ElementTint));
		SetWords(RowArrows[Index], FString());
	}

	// The bullet lines and the flavour line.
	const TArray<FString> Bullets = GunCardRules::Lines(Shown);
	while (LineTexts.Num() < Bullets.Num())
	{
		UTextBlock* Line = Words(WidgetTree, 13, Color::SlateText(), true);
		Line->SetAutoWrapText(true);
		LinesBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 1.f, 0.f, 1.f));
		LineTexts.Add(Line);
	}
	for (int32 Index = 0; Index < LineTexts.Num(); ++Index)
	{
		const bool bShownLine = Index < Bullets.Num();
		SetShown(LineTexts[Index], bShownLine);
		if (bShownLine)
		{
			SetWords(LineTexts[Index], FString(TEXT("• ")) + Bullets[Index]);
			// The element's line (always first) in its colour, like Borderlands' orange burn lines.
			LineTexts[Index]->SetColorAndOpacity(FSlateColor(Index == 0 && ElementGlyph ? ElementTint : Color::SlateText()));
		}
	}
	const FString Flavor = GunCardRules::FlavorLine(Shown);
	SetWords(FlavorText, Flavor);
	SetShown(FlavorText, !Flavor.IsEmpty());
	SetShown(Ornament, Bullets.Num() > 0 || !Flavor.IsEmpty());

	// The foot: the maker's badge in its first plate's colour, its trim round it; the element's glyph in its colour.
	const FLinearColor Plate = Style.Plates.Num() > 0 ? Style.Plates[0] : Color::MakerPlain();
	MakerBadge->SetBrush(FSlateRoundedBoxBrush(Plate, 18.f, Style.Trim, 2.5f));
	SetWords(MakerText, Style.Word.ToString());
	MakerText->SetColorAndOpacity(FSlateColor(Style.Ink));
	const FInkedIcon* Mark = Style.MarkIcon.IsNone() ? nullptr : FindInkedIcon(Style.MarkIcon);
	if (Mark)
	{
		MakerMark->SetBrush(InkedIconBrush(Style.MarkIcon, *Mark, FVector2D(MakerMarkSize, MakerMarkSize), Style.Ink));
	}
	SetShown(MakerMark, Mark != nullptr);
	if (ElementGlyph)
	{
		ElementMark->SetBrush(IconBrush(FName(*(GunCardIcons::ElementName(Element).ToString() + TEXT("Foot"))), *ElementGlyph, 3.f,
			FVector2D(ElementMarkSize, ElementMarkSize), ElementTint));
	}
	SetShown(ElementMark, ElementGlyph != nullptr);
	const EWeaponKind Kind = Shown.Definition ? Shown.Definition->Kind : EWeaponKind::None;
	if (Kind != EWeaponKind::None)
	{
		KindMark->SetBrush(InkedIconBrush(LoadoutParts::GunIconName(Kind), LoadoutParts::GunIcon(Kind), KindMarkBox, LooterWeaponText::Color(Shown)));
	}
	SetShown(KindMark, Kind != EWeaponKind::None);
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

FText UGunCardWidget::GetDamageText() const
{
	return RowValues.Num() > 0 && RowValues[0] ? RowValues[0]->GetText() : FText::GetEmpty();
}

FText UGunCardWidget::GetStatValueText(int32 Index) const
{
	return Index < ShownStats.Num() && RowValues.IsValidIndex(Index) ? RowValues[Index]->GetText() : FText::GetEmpty();
}

FText UGunCardWidget::GetStatArrowText(int32 Index) const
{
	return Index < ShownStats.Num() && RowArrows.IsValidIndex(Index) ? RowArrows[Index]->GetText() : FText::GetEmpty();
}

FText UGunCardWidget::GetVerdictText() const
{
	const FWeaponInstanceData* Against = bHasCompare ? &Compared : nullptr;
	return FText::FromString(bHasGun && Against ? GunCardRules::VerdictWord(LoadoutRules::Compare(Shown, Against)) : FString());
}

TArray<FString> UGunCardWidget::GetLines() const
{
	return bHasGun ? GunCardRules::Lines(Shown) : TArray<FString>();
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
