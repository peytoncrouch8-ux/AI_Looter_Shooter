// ULoadoutWidget: a shut equip slot (the user, 2026-10-31: two slots from the start, the third at level 3, the fourth after
// Ransom's Rest's story; GunSlotRules). Its EQUIPPED row in the same "Wide Sweep" row look as an empty slot's, dimmed, with
// the slot's padlock where a gun's picture would be and how it opens; its plain card says the same. Nothing goes into it:
// E, a put-down and a drop there are refused (LoadoutWidgetInput.cpp, LoadoutWidgetDrag.cpp).

#include "UI/Inventory/LoadoutWidget.h"
#include "Inventory/GunSlotRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/HUD/HudWeaponSlotsWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** The row's parts as an open slot's lays them out (LoadoutWidgetRows.cpp): the number badge, then the picture's box. */
	constexpr float BadgeSize = 24.f;
	constexpr float BadgeGap = 10.f;
	constexpr float PictureWidth = 104.f;
	constexpr float PictureHeight = 34.f;
	/** The padlock in the picture's box: the HUD's, a touch smaller to sit inside the row's line. */
	constexpr float LockScale = 1.15f;
	/** The whole row's content stands dimmed: there, but not the player's yet. */
	constexpr float LockedContentOpacity = 0.72f;
}

EGunSlotLock ULoadoutWidget::SlotLock(int32 SlotIndex) const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? Inventory->GetSlotLock(SlotIndex) : EGunSlotLock::Open;
}

bool ULoadoutWidget::IsSlotLocked(int32 SlotIndex) const
{
	return SlotLock(SlotIndex) != EGunSlotLock::Open;
}

ULoadoutWidget::FRow ULoadoutWidget::MakeLockedSlotRow(int32 SlotIndex, EGunSlotLock Lock)
{
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// The slot's number, as its key would take it in hand once it opens (dim: Restyle keeps an empty slot's number so).
	UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Badge->SetHorizontalAlignment(HAlign_Center);
	Badge->SetVerticalAlignment(VAlign_Center);
	UTextBlock* BadgeText = MakeText(WidgetTree, FString::FromInt(SlotIndex + 1), 12, Color::TextDim());
	BadgeText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Badge->SetContent(BadgeText);
	UHorizontalBoxSlot* BadgeSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Badge, BadgeSize, BadgeSize));
	BadgeSlot->SetVerticalAlignment(VAlign_Center);
	BadgeSlot->SetPadding(FMargin(0.f, 0.f, BadgeGap, 0.f));

	// The padlock the HUD's slot shows, centred where a gun's picture would be.
	FSlateBrush LockBrush = UHudWeaponSlotsWidget::PictureBrush(EHudSlotPicture::Lock);
	LockBrush.ImageSize = FVector2D(UHudWeaponSlotsWidget::LockWidth, UHudWeaponSlotsWidget::LockHeight) * LockScale;
	USizeBox* PictureBox = MakeSized(WidgetTree, MakeImage(WidgetTree, LockBrush), PictureWidth, PictureHeight);
	// At its own size in the box's middle (a size box's content fills it otherwise).
	if (USizeBoxSlot* LockSlot = Cast<USizeBoxSlot>(PictureBox->GetContentSlot()))
	{
		LockSlot->SetHorizontalAlignment(HAlign_Center);
		LockSlot->SetVerticalAlignment(VAlign_Center);
	}
	Line->AddChildToHorizontalBox(PictureBox)->SetVerticalAlignment(VAlign_Center);

	// "Locked", and how it opens: an empty slot's two lines.
	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Words->AddChildToVerticalBox(Label(WidgetTree, TEXT("Locked"), 11, Color::TextDim(), 200));
	Words->AddChildToVerticalBox(FittedLabel(WidgetTree, GunSlotRules::LockText(Lock).ToString(), 8,
		Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.85f), 120))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(14.f, 0.f, 10.f, 0.f));
	Line->SetRenderOpacity(LockedContentOpacity);

	FRow Row = WrapRow(Line, Color::TextDim(), false, LoadoutLayout::SlotRowHeight, ActionSlot, SlotIndex);
	Row.Badge = Badge;
	Row.BadgeText = BadgeText;
	return Row;
}

void ULoadoutWidget::AddCardLocked(int32 SlotIndex, EGunSlotLock Lock)
{
	const FString Title = FString::Printf(TEXT("Slot %d is locked"), SlotIndex + 1);
	const FString Words = Lock == EGunSlotLock::Level
		? FString::Printf(TEXT("%s. Until then the guns past the open slots wait in the backpack."), *GunSlotRules::LockText(Lock).ToString())
		: FString::Printf(TEXT("%s: finish every main mission there and a fourth gun rides with you."), *GunSlotRules::LockText(Lock).ToString());
	AddCardEmpty(Title, Words);
}
