#include "UI/World/WeaponLabelWidget.h"
#include "UI/Cards/GunCardRules.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// The looked-at gun's card over it (the user's call, 2026-10-28: the card over the gun on the ground, the gun in hand on
// the screen's right): a UGunCardWidget (World) compared with the gun in hand, with the take keys, standing on a cyan tether
// down to a hollow diamond in the rarity's colour where the name hung.

using namespace LooterUI;

namespace
{
	/** The tether's diamond: a hollow outline, drawn white once and tinted per gun (ApplyContent). */
	FSlateBrush TetherDiamondBrush()
	{
		FVectorIcon Icon;
		Icon.ViewBox = FVector2D(12.f, 12.f);
		Icon.StrokeWidth = 1.8f;
		Icon.Strokes.Add({ FVector2D(6.f, 1.f), FVector2D(11.f, 6.f), FVector2D(6.f, 11.f), FVector2D(1.f, 6.f), FVector2D(6.f, 1.f) });
		return IconBrush(TEXT("LootTetherDiamond"), Icon, 4.f,
			FVector2D(UWeaponLabelWidget::DiamondSize, UWeaponLabelWidget::DiamondSize), FLinearColor::White);
	}
}

UWidget* UWeaponLabelWidget::BuildFocusStack()
{
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FocusStack"));
	FocusStack = Stack;
	auto AddCentred = [Stack](UWidget* Child, float Below)
	{
		UVerticalBoxSlot* ChildSlot = Stack->AddChildToVerticalBox(Child);
		ChildSlot->SetHorizontalAlignment(HAlign_Center);
		ChildSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Below));
	};

	// A named gun's line, said over its card (ApplyContent styles it and collapses it for any other gun).
	FlavorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Flavor"));
	AddCentred(FlavorText, 6.f);

	// The card; its intro unfolds it from its foot (UGunCardWidget::UnfoldPivotY), so it rises out of the tether.
	Card = WidgetTree->ConstructWidget<UGunCardWidget>(UGunCardWidget::StaticClass(), TEXT("GunCard"));
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddCentred(Card, 0.f);

	// The tether from the card's foot down to the diamond: what ties the card to its gun among others lying near.
	UImage* Tether = MakeImage(WidgetTree, RectBrush(Color::SaltLine()));
	AddCentred(MakeSized(WidgetTree, Tether, TetherWidth, TetherLength), 0.f);
	TetherDiamond = MakeImage(WidgetTree, TetherDiamondBrush());
	AddCentred(MakeSized(WidgetTree, TetherDiamond, DiamondSize, DiamondSize), 0.f);

	Stack->SetVisibility(ESlateVisibility::Collapsed);
	return Stack;
}

bool UWeaponLabelWidget::IsCardShown() const
{
	const ESlateVisibility Shown = FocusStack ? FocusStack->GetVisibility() : ESlateVisibility::Collapsed;
	return Card && bHasGun && Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden;
}

void UWeaponLabelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// The weapon manager sets the label's focus every frame; the card follows the hand, the slots and the hold as they change.
	if (bIsFocused && !bPop)
	{
		UpdateCardFromViewer();
	}
}

void UWeaponLabelWidget::UpdateCardFromViewer()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		const UWorld* World = GetWorld();
		PC = World ? World->GetFirstPlayerController() : nullptr;
	}
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UpdateCard(Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr,
		Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr);
}

void UWeaponLabelWidget::UpdateCard(const UWeaponManagerComponent* Manager, const UInteractionComponent* Interaction)
{
	if (!Card || !bHasGun || !bIsFocused || bPop)
	{
		return;
	}

	// Compared like with like (the user's call, 2026-10-28): the gun in hand if it's this kind, else the first carried gun
	// of this kind (the slots, then the backpack), else nothing. SetGun returns at once when nothing changed.
	const AWeaponBase* InHand = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (InHand == Weapon.Get())
	{
		InHand = nullptr;
	}
	TArray<const FWeaponInstanceData*> Carried;
	if (Manager)
	{
		for (const AWeaponBase* SlotGun : Manager->GetWeapons())
		{
			if (SlotGun && SlotGun != Weapon.Get())
			{
				Carried.Add(&SlotGun->GetInstance());
			}
		}
		for (const FWeaponInstanceData& Stored : Manager->GetBackpack())
		{
			Carried.Add(&Stored);
		}
	}
	const FWeaponInstanceData* Against = GunCardRules::PickCompare(Gun, InHand ? &InHand->GetInstance() : nullptr, Carried);
	const bool bChanged = Card->SetGun(Gun, Against, EGunCardMode::World);
	if (bCardStale || bChanged)
	{
		bCardStale = false;
		ComparedWith = InHand;
		bCompared = Against != nullptr;
		if (bChanged)
		{
			bIntroPending = true;
		}
	}

	// The keys: what a tap and a hold do with the slots and backpack as they are (the weapon manager's take rules), the
	// HOLD cap filling with the interact hold on this gun (looking away cancels a hold, so one under way is on this gun).
	FGunCardKeys Keys;
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	if (Bindings)
	{
		Keys.Key = Bindings->GetKey(TEXT("Interact")).GetDisplayName().ToString().ToUpper();
	}
	if (Manager)
	{
		// The open slots: a shut one takes no gun (WeaponManagerSlotGate.cpp).
		Keys.bSlotsFull = !Manager->HasFreeSlot();
		// The backpack's room is shared with the spare wards.
		Keys.bBackpackFull = !Manager->HasBackpackRoom();
	}
	const AActor* Looked = Interaction ? Interaction->GetFocusedActor() : nullptr;
	Keys.Hold = Looked && Looked == Weapon.Get() ? Interaction->GetHoldProgress() : 0.f;
	Card->SetKeys(Keys);

	// A gun newly looked at: its card unfolds with its open sound and its maker's.
	if (bIntroPending)
	{
		bIntroPending = false;
		Card->PlayIntro(/*bWithSound*/ true);
	}
}
