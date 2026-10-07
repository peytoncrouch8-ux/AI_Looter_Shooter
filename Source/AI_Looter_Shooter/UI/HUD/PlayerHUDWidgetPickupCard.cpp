#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudInteractPromptWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

// What the interact key would do to the thing looked at: the loot comparison card for loot (the weapon, stat by stat
// against the one in hand, and how to take it), the interaction prompt under the crosshair for anything else.

using namespace LooterUI;

namespace
{
	FString FormatNumber(float Value, int32 Decimals)
	{
		FNumberFormattingOptions Options;
		Options.UseGrouping = false;
		Options.MinimumFractionalDigits = Decimals;
		Options.MaximumFractionalDigits = Decimals;
		return FText::AsNumber(Value, &Options).ToString();
	}

	/** "LABEL  value (+delta)", colored better/worse against the weapon in hand. ShownValue replaces the formatted number when set. */
	void SetCompareLine(UTextBlock* Text, const TCHAR* Label, float NewValue, float OldValue, bool bHigherIsBetter,
		int32 Decimals, const TCHAR* Prefix, const TCHAR* Suffix, bool bHasCurrent, const FString& ShownValue = FString())
	{
		FString Line = FString::Printf(TEXT("%-10s "), Label) + Prefix + (ShownValue.IsEmpty() ? FormatNumber(NewValue, Decimals) : ShownValue) + Suffix;
		FLinearColor LineColor = Color::Text();
		if (bHasCurrent && !FMath::IsNearlyEqual(NewValue, OldValue, 0.01f))
		{
			const float Delta = NewValue - OldValue;
			Line += FString::Printf(TEXT("   (%s%s)"), Delta > 0.f ? TEXT("+") : TEXT(""), *FormatNumber(Delta, FMath::Max(Decimals, 1)));
			LineColor = ((Delta > 0.f) == bHigherIsBetter) ? Color::Better() : Color::Worse();
		}
		Text->SetText(FText::FromString(Line));
		Text->SetColorAndOpacity(FSlateColor(LineColor));
	}
}

void UPlayerHUDWidget::UpdatePickupCard(UWeaponManagerComponent* Manager, const UInteractionComponent* Interaction, float DeltaTime)
{
	// The interaction component knows what's looked at; loot (offered by the weapon manager) gets the card.
	const AWeaponBase* Pickup = Interaction ? Cast<AWeaponBase>(Interaction->GetFocusedActor()) : nullptr;
	if (Pickup && (!Pickup->IsPickup() || !Manager))
	{
		Pickup = nullptr;
	}
	// The key's name is only needed while something is looked at (a fading prompt keeps its words).
	const FString Key = Interaction && Interaction->GetFocusedActor() ? BoundKeyName(TEXT("Interact"), TEXT("E")) : FString();
	if (InteractPrompt)
	{
		InteractPrompt->Update(Pickup ? nullptr : Interaction, Key, DeltaTime);
	}
	if (!Pickup)
	{
		PickupCard->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	PickupCard->SetVisibility(ESlateVisibility::HitTestInvisible);

	const FWeaponInstanceData& New = Pickup->GetInstance();
	const AWeaponBase* Current = Manager->GetActiveWeapon();
	const FWeaponStats Old = Current ? Current->GetStats() : FWeaponStats();
	const bool bHasCurrent = Current != nullptr;
	const FWeaponStats& S = New.Stats;

	PickupName->SetText(FText::FromString(LooterWeaponText::Name(New).ToUpper()));
	PickupName->SetColorAndOpacity(FSlateColor(LooterWeaponText::Color(New)));
	// A named gun's line under its name, as written.
	const FString Flavor = LooterWeaponText::FlavorLine(New);
	PickupFlavor->SetText(FText::FromString(Flavor));
	const ESlateVisibility FlavorShown = Flavor.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible;
	if (PickupFlavor->GetVisibility() != FlavorShown)
	{
		PickupFlavor->SetVisibility(FlavorShown);
	}
	PickupLevel->SetText(FText::FromString(FString::Printf(TEXT("LV %d  |  %s  |  VS WEAPON IN HAND"), New.Level, *LooterWeaponText::FireModeName(New).ToUpper())));

	// Damage shows the weapon's damage, compared on total per-shot damage so shotguns and rifles line up fairly.
	SetCompareLine(PickupStatTexts[0], TEXT("DAMAGE"), S.Damage * S.PelletsPerShot, Old.Damage * Old.PelletsPerShot, true, 1, TEXT(""), TEXT(""), bHasCurrent,
		LooterWeaponText::DamageString(S));
	SetCompareLine(PickupStatTexts[1], TEXT("FIRE RATE"), S.FireRate, Old.FireRate, true, 0, TEXT(""), TEXT(" RPM"), bHasCurrent);
	SetCompareLine(PickupStatTexts[2], TEXT("MAGAZINE"), S.MagazineSize, Old.MagazineSize, true, 0, TEXT(""), TEXT(""), bHasCurrent);
	SetCompareLine(PickupStatTexts[3], TEXT("RELOAD"), S.ReloadTime, Old.ReloadTime, false, 2, TEXT(""), TEXT("S"), bHasCurrent);
	SetCompareLine(PickupStatTexts[4], TEXT("SPREAD"), S.Spread, Old.Spread, false, 2, TEXT(""), TEXT(" DEG"), bHasCurrent);
	SetCompareLine(PickupStatTexts[5], TEXT("RANGE"), S.Range / 100.f, Old.Range / 100.f, true, 0, TEXT(""), TEXT(" M"), bHasCurrent);
	SetCompareLine(PickupStatTexts[6], TEXT("RECOIL"), S.Recoil * 100.f, Old.Recoil * 100.f, false, 0, TEXT(""), TEXT("%"), bHasCurrent);
	SetCompareLine(PickupStatTexts[7], TEXT("HANDLING"), S.Handling * 100.f, Old.Handling * 100.f, true, 0, TEXT(""), TEXT("%"), bHasCurrent);
	SetCompareLine(PickupStatTexts[8], TEXT("ZOOM"), S.Zoom, Old.Zoom, true, 2, TEXT(""), TEXT(""), bHasCurrent, LooterWeaponText::ZoomString(S).ToUpper());

	// A tap and a hold only differ when every slot is full: the tap stashes the loot, the hold takes it in hand.
	const bool bSlotsFull = Manager->GetWeapons().Num() >= Manager->MaxWeapons;
	const bool bBackpackFull = Manager->GetBackpack().Num() >= Manager->BackpackCapacity;
	FString Hint;
	if (!bSlotsFull)
	{
		Hint = FString::Printf(TEXT("[%s] PICK UP"), *Key);
	}
	else if (!bBackpackFull)
	{
		Hint = FString::Printf(TEXT("[%s] SEND TO BACKPACK\nHOLD [%s] EQUIP, WEAPON IN HAND TO BACKPACK"), *Key, *Key);
	}
	else
	{
		Hint = FString::Printf(TEXT("[%s] SWAP WITH WEAPON IN HAND\nBACKPACK FULL: IT DROPS"), *Key);
	}
	if (!PickupHint->GetText().ToString().Equals(Hint))
	{
		PickupHint->SetText(FText::FromString(Hint));
	}

	// The hold that equips it (looking away cancels a hold, so a hold under way is always on the loot looked at).
	const float Hold = Interaction->GetHoldProgress();
	PickupHoldBar->SetVisibility(Hold > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (Hold > 0.f)
	{
		const int32 Segments = PickupHoldSegments.Num();
		const int32 Lit = FMath::Clamp(FMath::CeilToInt32(Hold * Segments), 0, Segments);
		for (int32 Index = 0; Index < PickupHoldSegments.Num(); ++Index)
		{
			PickupHoldSegments[Index]->SetColorAndOpacity(Index < Lit ? Color::Accent() : Color::SegmentOff());
		}
	}
}
