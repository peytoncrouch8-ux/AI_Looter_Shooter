#include "UI/Style/WeaponMakerRows.h"
#include "Affixes/WeaponMakers.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

UWidget* WeaponMakerRows::MakeLine(UWidgetTree* Tree, const FWeaponInstanceData& Gun, int32 FontSize)
{
	const FWeaponMaker* Maker = WeaponMakers::Of(Gun);
	if (!Tree || !Maker)
	{
		return nullptr;
	}
	UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// The mark a size up from the text, white as every Inked icon is drawn.
	if (const FInkedIcon* Mark = LooterWeaponText::MakerMark(Gun))
	{
		const float Side = FontSize * 1.6f;
		Line->AddChildToHorizontalBox(MakeImage(Tree, InkedIconBrush(Maker->MarkIcon, *Mark, FVector2D(Side, Side))))
			->SetVerticalAlignment(VAlign_Center);
	}
	const FString Words = FString::Printf(TEXT("%s · %s"), *LooterWeaponText::MakerString(Gun), *Maker->QuirkName.ToString().ToUpper());
	UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(MakeText(Tree, Words, FontSize, LooterWeaponText::MakerColor(), true, 120));
	TextSlot->SetVerticalAlignment(VAlign_Center);
	TextSlot->SetPadding(FMargin(Line->GetChildrenCount() > 1 ? 6.f : 0.f, 0.f, 0.f, 0.f));
	return Line;
}

UWidget* WeaponMakerRows::MakeRows(UWidgetTree* Tree, const FWeaponInstanceData& Gun, int32 FontSize, bool bFiction)
{
	UWidget* Line = MakeLine(Tree, Gun, FontSize);
	if (!Line)
	{
		return nullptr;
	}
	UVerticalBox* Rows = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Rows->AddChildToVerticalBox(Line);
	auto AddDim = [Tree, Rows, FontSize](const FString& Words, float Top)
	{
		if (Words.IsEmpty())
		{
			return;
		}
		UTextBlock* Body = MakeText(Tree, Words, FontSize, Color::TextDim());
		Body->SetAutoWrapText(true);
		Rows->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
	};
	// The trade without its name (the line above carries it): "accuracy +30%, range +20%...".
	FString Trade = LooterWeaponText::MakerQuirkLine(Gun);
	int32 Colon = INDEX_NONE;
	if (Trade.FindChar(TEXT(':'), Colon))
	{
		Trade = Trade.Mid(Colon + 1).TrimStart();
		Trade = Trade.Left(1).ToUpper() + Trade.Mid(1);
	}
	else
	{
		Trade.Reset();
	}
	AddDim(Trade, 2.f);
	if (bFiction)
	{
		AddDim(LooterWeaponText::MakerFullLine(Gun), 5.f);
		AddDim(LooterWeaponText::MakerFiction(Gun), 1.f);
	}
	return Rows;
}
