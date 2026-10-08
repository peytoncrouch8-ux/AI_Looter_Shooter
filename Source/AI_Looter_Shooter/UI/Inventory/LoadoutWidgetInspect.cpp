#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Inventory/LoadoutStage.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;
using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// The stats card beside the gun under the cursor
// ---------------------------------------------------------------------------

const ULoadoutWidget::FCard* ULoadoutWidget::CursorCard() const
{
	const TArray<FCard>& Cards = Zone == EZone::Slots ? SlotCards : ListCards;
	return Cards.IsValidIndex(CursorIndex) ? &Cards[CursorIndex] : nullptr;
}

void ULoadoutWidget::RefreshInspect()
{
	if (!InspectBox)
	{
		return;
	}
	InspectBox->ClearChildren();
	// A slot's gun on its own; a backpack gun against the gun in the chosen slot (what equipping it would change).
	const FWeaponInstanceData* SlotGun = SlotItem(ChosenSlot);
	const FWeaponInstanceData* Shown = Zone == EZone::Slots ? SlotItem(CursorIndex) : ListItem(CursorIndex);
	const FWeaponInstanceData* Baseline = Zone == EZone::Backpack ? SlotGun : nullptr;
	if (!Shown)
	{
		return;
	}

	const FString Header = Zone == EZone::Slots
		? FString::Printf(TEXT("Slot %d · %s"), CursorIndex + 1, LoadoutCarry::Label(LoadoutCarry::ForSlot(CursorIndex, NumWeapons(), GetActiveSlot())))
		: (Baseline ? FString::Printf(TEXT("Backpack · vs slot %d"), ChosenSlot + 1) : FString(TEXT("Backpack")));
	InspectHeader->SetText(FText::FromString(Header.ToUpper()));

	UTextBlock* NameText = Label(WidgetTree, LooterWeaponText::Name(*Shown), 15, LooterWeaponText::Color(*Shown), 40);
	NameText->SetAutoWrapText(true);
	InspectBox->AddChildToVerticalBox(NameText);
	// A named gun's line under its name, as written.
	const FString Flavor = LooterWeaponText::FlavorLine(*Shown);
	if (!Flavor.IsEmpty())
	{
		UTextBlock* FlavorText = MakeText(WidgetTree, Flavor, 11, LooterWeaponText::FlavorColor());
		FlavorText->SetFont(LooterWeaponText::FlavorFont(11));
		FlavorText->SetAutoWrapText(true);
		InspectBox->AddChildToVerticalBox(FlavorText)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}
	InspectBox->AddChildToVerticalBox(Label(WidgetTree, FString::Printf(TEXT("Lv %d · %s · %s"), Shown->Level,
		*LooterWeaponText::FireModeName(*Shown), *AmmoName(*Shown)), 9, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 4.f, 0.f, 8.f));
	// Its notches and its curse (perk and drawback), when it has any: between the name and the stats.
	if (UWidget* Ideas = MakeGunIdeasRows(WidgetTree, *Shown, 9))
	{
		InspectBox->AddChildToVerticalBox(Ideas)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	const FWeaponStats& S = Shown->Stats;
	const FWeaponStats* B = Baseline ? &Baseline->Stats : nullptr;
	auto AddStat = [this, B](const TCHAR* StatName, float Rating, const FString& Value, float New, float Old, bool bHigherIsBetter, int32 Decimals)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Label(WidgetTree, StatName, 8, Color::TextDim(), 120), 92.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* BarSlot = Line->AddChildToHorizontalBox(MakeSegmentBar(WidgetTree, 8, Rating, Color::SegmentOn(), 7.f));
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* ValueText = MakeText(WidgetTree, Value, 11, Color::Text());
		ValueText->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, ValueText, 60.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		// The change from the chosen slot's gun, green when it's an upgrade.
		UTextBlock* DeltaText = MakeText(WidgetTree, TEXT(""), 9, Color::TextDim());
		DeltaText->SetJustification(ETextJustify::Right);
		if (B && !FMath::IsNearlyEqual(New, Old, 0.01f))
		{
			DeltaText->SetText(FText::FromString(FormatDelta(New - Old, Decimals)));
			DeltaText->SetColorAndOpacity(FSlateColor(((New > Old) == bHigherIsBetter) ? Color::Better() : Color::Worse()));
		}
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, DeltaText, 46.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		InspectBox->AddChildToVerticalBox(MakeSized(WidgetTree, Line, 0.f, 20.f))->SetPadding(FMargin(0.f, 1.f));
	};
	// Damage compares the whole shot, so shotguns and rifles line up fairly.
	AddStat(TEXT("Damage"), LooterWeaponText::DamageRating(S), LooterWeaponText::DamageString(S), S.Damage * S.PelletsPerShot,
		B ? B->Damage * B->PelletsPerShot : 0.f, true, 1);
	AddStat(TEXT("Fire rate"), LooterWeaponText::FireRateRating(S), FString::Printf(TEXT("%.0f"), S.FireRate), S.FireRate, B ? B->FireRate : 0.f, true, 0);
	AddStat(TEXT("Magazine"), LooterWeaponText::MagazineRating(S), FString::FromInt(S.MagazineSize), S.MagazineSize, B ? B->MagazineSize : 0.f, true, 0);
	AddStat(TEXT("Reload"), LooterWeaponText::ReloadRating(S), FString::Printf(TEXT("%.2fs"), S.ReloadTime), S.ReloadTime, B ? B->ReloadTime : 0.f, false, 2);
	AddStat(TEXT("Accuracy"), LooterWeaponText::AccuracyRating(S), FString::Printf(TEXT("%.1f°"), S.Spread), S.Spread, B ? B->Spread : 0.f, false, 1);
	// Range is where the damage starts to fall off; recoil and handling are against a plain gun of its kind.
	AddStat(TEXT("Range"), LooterWeaponText::RangeRating(S), FString::Printf(TEXT("%.0f m"), S.Range / 100.f), S.Range / 100.f, B ? B->Range / 100.f : 0.f, true, 0);
	AddStat(TEXT("Recoil"), LooterWeaponText::RecoilRating(S), FString::Printf(TEXT("%.0f%%"), S.Recoil * 100.f), S.Recoil * 100.f, B ? B->Recoil * 100.f : 0.f, false, 0);
	AddStat(TEXT("Handling"), LooterWeaponText::HandlingRating(S), FString::Printf(TEXT("%.0f%%"), S.Handling * 100.f), S.Handling * 100.f, B ? B->Handling * 100.f : 0.f, true, 0);
	AddStat(TEXT("Zoom"), LooterWeaponText::ZoomRating(S), LooterWeaponText::ZoomString(S), S.Zoom, B ? B->Zoom : 0.f, true, 2);
}


void ULoadoutWidget::PlaceInspect()
{
	if (!InspectCard || !InspectSlot)
	{
		return;
	}
	// It shows for the cursor's gun when the cursor was moved with keys, or the mouse is on that gun's card; never while
	// something is being dragged.
	const FCard* Card = CursorCard();
	const bool bShow = Card && InspectBox->HasAnyChildren() && !bItemDrag && !bPressPending && !bDragging
		&& (bKeyboardCursor || Card->Button->IsHovered());
	if (!bShow)
	{
		if (InspectCard->GetVisibility() != ESlateVisibility::Hidden)
		{
			InspectCard->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}
	// Beside its card, toward the stand-in (right of the slots, left of the backpack), level with it and on the page.
	const FVector2D CardTop = ToPage(Card->Button->GetCachedGeometry().GetAbsolutePosition());
	const float Height = InspectCard->GetDesiredSize().Y;
	const float X = Zone == EZone::Slots ? LeftX + LeftWidth + InspectGap : RightX - InspectGap - InspectWidth;
	const float Y = FMath::Clamp(static_cast<float>(CardTop.Y), 70.f, FMath::Max(70.f, static_cast<float>(PageSize.Y) - 70.f - Height));
	InspectSlot->SetPosition(FVector2D(X, Y));
	if (InspectCard->GetVisibility() != ESlateVisibility::HitTestInvisible)
	{
		InspectCard->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

FVector2D ULoadoutWidget::ToPage(const FVector2D& ScreenPosition) const
{
	return Page ? Page->GetCachedGeometry().AbsoluteToLocal(ScreenPosition) : ScreenPosition;
}
