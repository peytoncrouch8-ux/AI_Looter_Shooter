// ULoadoutWidget: the card of the ward under the cursor, in the middle where a gun's card shows (the user, 2026-10-31:
// wards look like the guns in the loadout too): the ward card (UWardCardWidget, Loadout), a backpack ward's compared with
// the ward worn; with none worn, the WARD row gets a plain card saying how wards come. The showcase holds the ward box's
// Inked icon where a gun would turn. The ward rows are LoadoutWidgetWard.cpp, their keys LoadoutWidgetWardKeys.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/Cards/WardCardWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** The ward box's icon in the showcase (it fits the showcase's height). */
	constexpr float ShowcaseIconSize = 240.f;
	/** A plain card's identity for "no ward worn", so it slides in once. */
	constexpr uint32 NoWardCard = 0x3A4D0000u;
}

void ULoadoutWidget::ShowWardCard(bool bShow)
{
	if (WardCard)
	{
		WardCard->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShow)
	{
		return;
	}
	// The ward card takes the gun card's place; the plain card goes too.
	if (GunCard)
	{
		GunCard->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PlainCard)
	{
		PlainCard->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULoadoutWidget::RefreshWardCard()
{
	if (!CardBox)
	{
		return;
	}
	const FShieldInstanceData* Ward = CursorWard();
	if (!Ward)
	{
		// The WARD row with none worn: the plain card says what a ward is and how one comes.
		ShowPlainCard(true);
		CardBox->ClearChildren();
		if (CardIdentity != NoWardCard)
		{
			CardIdentity = NoWardCard;
			CardIntroAge = 0.f;
			if (CardScroll)
			{
				CardScroll->ScrollToStart();
			}
		}
		CardRarity = Color::TextDim();
		bCardBreathes = false;
		const UPlayerShieldComponent* Shield = GetShield();
		const bool bSpares = Shield && !Shield->GetSpares().IsEmpty();
		AddCardEmpty(TEXT("No ward worn"), bSpares ? TEXT("A ward soaks hits before your health does, and fills again after a few seconds unhurt. "
			"E: choose one from the backpack.") : TEXT("A ward soaks hits before your health does, and fills again after a few seconds unhurt. "
			"Wards drop like guns: tap Interact on one to take it, hold to wear it."));
		return;
	}

	// Its rarity colours the showcase ring, as a gun's does; the top rarities' pool breathes.
	CardRarity = ShieldText::Color(*Ward);
	bCardBreathes = Ward->Rarity >= EWeaponRarity::Epic;
	ShowWardCard(true);
	// A backpack ward is compared with the ward worn; the worn one with nothing (its arrows hide).
	const FShieldInstanceData* Against = Zone == EZone::Backpack ? WornWard() : nullptr;
	if (WardCard && WardCard->SetWard(*Ward, Against, EWardCardMode::Loadout))
	{
		WardCard->PlayIntro(false);
		PlayCardOpenCue();
	}
	// The plain card slides in afresh when it shows next.
	CardIdentity = 0;
}

void ULoadoutWidget::RefreshWardShowcase()
{
	if (!WardShowcase)
	{
		return;
	}
	const FShieldInstanceData* Ward = CursorWard();
	const FName IconName = Ward && Ward->Definition ? Ward->Definition->IconName : FName(TEXT("WardBox"));
	const FInkedIcon* Inked = Ward ? FindInkedIcon(IconName) : nullptr;
	if (!Inked || bInspecting)
	{
		WardShowcase->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	WardShowcase->SetBrush(InkedIconBrush(IconName, *Inked, FVector2D(ShowcaseIconSize, ShowcaseIconSize)));
	WardShowcase->SetVisibility(ESlateVisibility::HitTestInvisible);
}
