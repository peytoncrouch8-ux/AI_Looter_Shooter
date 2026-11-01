// USecretsWidget: the rows (for sale, then marked), each kind's keyhole, how a row is lit, and a buy's motion (the row's
// soul-gold flash and MARKED stamped in).

#include "UI/Vendor/SecretsWidget.h"
#include "Economy/WalletSubsystem.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/MapIcons.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Vendor/SilverUI.h"
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

#define LOCTEXT_NAMESPACE "LooterSecrets"

using namespace LooterUI;

namespace
{
	const FName SecretsRowAction(TEXT("SecretsRow"));

	/** The flash's soul-gold over a row's glass at its brightest. */
	constexpr float SecretsFlashAlpha = 0.55f;

	/** Eases a 0-1 time out (fast, then settling): the stamp landing. */
	float SecretsEaseOut(float Time)
	{
		const float Left = 1.f - FMath::Clamp(Time, 0.f, 1.f);
		return 1.f - Left * Left * Left;
	}
}

FLinearColor USecretsWidget::KindColor(ESecretKind Kind)
{
	// The kit's palette: the common caches pale, the strongbox and the named guns gold, the Tollman red, the pages salt,
	// the story's place the cursed irons' brass.
	switch (Kind)
	{
	case ESecretKind::Cache:     return Color::IconLight();
	case ESecretKind::Strongbox: return Color::SoulGold();
	case ESecretKind::Named:     return Color::Accent();
	case ESecretKind::Elite:     return Color::Worse();
	case ESecretKind::Lore:      return Color::Title();
	case ESecretKind::Story:     return Color::Curse();
	}
	return Color::Text();
}

void USecretsWidget::BuildRows(UVerticalBox* List)
{
	Parts.Reset();
	Parts.SetNum(Rows.Num());
	const bool bAnyForSale = Rows.ContainsByPredicate([](const FSecretRow& Row) { return Row.State == ESecretState::ForSale; });
	List->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("ForSale", "For sale").ToString()));
	if (!bAnyForSale)
	{
		UTextBlock* Nothing = MakeText(WidgetTree, LOCTEXT("NothingForSale", "Nothing more for sale. He'll know more as the story goes on.").ToString(),
			12, Color::TextDim());
		Nothing->SetAutoWrapText(true);
		List->AddChildToVerticalBox(Nothing)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	bool bMarkedHeader = false;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (Rows[Index].State == ESecretState::Sold && !bMarkedHeader)
		{
			bMarkedHeader = true;
			List->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Marked", "Marked on your map").ToString()))
				->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
		}
		List->AddChildToVerticalBox(MakeRow(Index))->SetPadding(FMargin(0.f, SecretsLayout::RowGap, 0.f, 0.f));
	}
}

UWidget* USecretsWidget::MakeMark(const FSecretRow& Row) const
{
	// A dark badge ringed in the kind's colour (a background: it fades with the UI transparency) and the map's keyhole over it.
	const FLinearColor Tint = KindColor(Row.Kind);
	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Badge = MakeImage(WidgetTree, MapIcons::BadgeBrush(Tint, /*bLit*/ false));
	MarkBackground(Badge);
	FillOverlaySlot(Mark->AddChildToOverlay(Badge));
	UImage* Icon = MakeImage(WidgetTree, MapIcons::SecretBrush(SecretsLayout::MarkSize * MapIcons::IconShare, Tint));
	UOverlaySlot* IconSlot = Mark->AddChildToOverlay(Icon);
	IconSlot->SetHorizontalAlignment(HAlign_Center);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	return MakeSized(WidgetTree, Mark, SecretsLayout::MarkSize, SecretsLayout::MarkSize);
}

UWidget* USecretsWidget::MakeRow(int32 RowIndex)
{
	const FSecretRow& Row = Rows[RowIndex];
	const bool bSold = Row.State == ESecretState::Sold;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeMark(Row));
	MarkSlot->SetVerticalAlignment(VAlign_Center);
	MarkSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));

	// Its name; under it the teaser and where (for sale), or his line (bought).
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Info->AddChildToVerticalBox(LoadoutParts::FittedLabel(WidgetTree, Row.Title.ToString(), 14, bSold ? Color::TextDim() : Color::Text(), 60));
	UTextBlock* Detail = MakeText(WidgetTree, bSold ? FString::Printf(TEXT("“%s”"), *Row.Line.ToString()) : Row.Teaser.ToString(), 12,
		bSold ? Color::SoulGold() : Color::TextDim());
	Detail->SetAutoWrapText(true);
	Info->AddChildToVerticalBox(Detail)->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	const FText Where = FText::Format(LOCTEXT("Where", "{0} · {1}"), Row.Region, SecretRules::KindWord(Row.Kind));
	Info->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, Where.ToString(), 9, Color::TextDim(), 150))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	UHorizontalBoxSlot* InfoSlot = Line->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);

	// The price (red when it's more than the purse), or MARKED (its places when there are several).
	UWidget* Right = nullptr;
	if (bSold)
	{
		const FText Word = Row.Places > 1 ? FText::Format(LOCTEXT("PlacesMarked", "{0} marked"), FText::AsNumber(Row.Places))
			: LOCTEXT("MarkedWord", "Marked");
		UWidget* Stamp = LoadoutParts::Label(WidgetTree, Word.ToString(), 12, Color::SoulGold(), 200);
		Stamp->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Parts[RowIndex].Stamp = Stamp;
		Right = Stamp;
	}
	else
	{
		const bool bAffordable = UWalletSubsystem::SilverOf(this) >= Row.Price;
		Right = SilverUI::MakeAmount(WidgetTree, Row.Price, 16, bAffordable ? Color::Text() : Color::Worse(), false);
	}
	UHorizontalBoxSlot* RightSlot = Line->AddChildToHorizontalBox(Right);
	RightSlot->SetVerticalAlignment(VAlign_Center);
	RightSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));

	FRowParts& RowParts = Parts[RowIndex];
	UOverlay* Card = LoadoutParts::MakeCard(WidgetTree, Line, FMargin(12.f, 8.f, 16.f, 8.f), 1.5f, RowParts.Fill, RowParts.Line);
	USizeBox* Sized = MakeSized(WidgetTree, Card, 0.f);
	Sized->SetMinDesiredHeight(SecretsLayout::RowHeight);
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	// The screen sounds its own choosing and buying.
	Button->bPlaysSounds = false;
	Button->SetupContent(Sized, SecretsRowAction, RowIndex, EButtonKind::Bare);
	Button->OnButtonClicked.BindUObject(this, &USecretsWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &USecretsWidget::HandleHovered);
	RowParts.Button = Button;
	return Button;
}

void USecretsWidget::Restyle()
{
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		const FRowParts& Row = Parts[Index];
		const bool bChosen = Index == Selected;
		const bool bSold = Rows.IsValidIndex(Index) && Rows[Index].State == ESecretState::Sold;
		// Gold is the chosen row (the kit's Slate & Salt); a bought one's glass is fainter, as it's paid for.
		if (Row.Fill)
		{
			Row.Fill->SetColorAndOpacity(bChosen ? LoadoutParts::Colors::PickedFill() : bSold ? LoadoutParts::Colors::EmptyFill() : Color::Row());
		}
		if (Row.Line)
		{
			Row.Line->SetColorAndOpacity(bChosen ? Color::Accent() : Color::RowLine());
		}
		if (Row.Stamp && (FlashAge < 0.f || !Rows.IsValidIndex(Index) || Rows[Index].Id != FlashId))
		{
			Row.Stamp->SetRenderScale(FVector2D(1.f));
			Row.Stamp->SetRenderOpacity(1.f);
		}
	}
}

void USecretsWidget::UpdateMotion(float DeltaTime)
{
	if (FlashAge < 0.f)
	{
		return;
	}
	FlashAge += DeltaTime;
	const int32 Index = Rows.IndexOfByPredicate([this](const FSecretRow& Row) { return Row.Id == FlashId; });
	const float Done = FMath::Max(SecretsLayout::FlashSeconds, SecretsLayout::StampSeconds);
	if (!Parts.IsValidIndex(Index) || FlashAge >= Done)
	{
		FlashAge = -1.f;
		FlashId = NAME_None;
		Restyle();
		return;
	}
	const FRowParts& Row = Parts[Index];
	// The row lights soul-gold and settles back to its glass.
	if (Row.Fill)
	{
		const float Glow = FMath::Square(1.f - FMath::Clamp(FlashAge / SecretsLayout::FlashSeconds, 0.f, 1.f));
		const FLinearColor Base = Index == Selected ? LoadoutParts::Colors::PickedFill() : LoadoutParts::Colors::EmptyFill();
		FLinearColor Gold = Color::SoulGold();
		Gold.A = SecretsFlashAlpha;
		Row.Fill->SetColorAndOpacity(FMath::Lerp(Base, Gold, Glow));
	}
	// MARKED lands: in from a size and a half, fading in.
	if (Row.Stamp)
	{
		const float Land = SecretsEaseOut(FlashAge / SecretsLayout::StampSeconds);
		Row.Stamp->SetRenderScale(FVector2D(FMath::Lerp(SecretsLayout::StampFrom, 1.f, Land)));
		Row.Stamp->SetRenderOpacity(FMath::Clamp(FlashAge / SecretsLayout::StampSeconds, 0.f, 1.f));
	}
}

#undef LOCTEXT_NAMESPACE
