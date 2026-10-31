#include "UI/World/ShieldLabelWidget.h"
#include "UI/Cards/WardCardRules.h"
#include "UI/Cards/WardCardWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Interaction/InteractionComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Shields/PlayerShieldComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// The looked-at ward's card over it, as a gun's (WeaponLabelWidgetCard.cpp; the user, 2026-10-31): a UWardCardWidget
// (World) compared with the ward worn, with the take keys, standing on a salt tether down to a hollow diamond in the
// rarity's colour where the name hung.

using namespace LooterUI;

namespace
{
	/** The tether's diamond: a hollow outline, drawn white once and tinted per ward (the gun label's own). */
	FSlateBrush TetherDiamondBrush()
	{
		FVectorIcon Diamond;
		Diamond.ViewBox = FVector2D(12.f, 12.f);
		Diamond.StrokeWidth = 1.8f;
		Diamond.Strokes.Add({ FVector2D(6.f, 1.f), FVector2D(11.f, 6.f), FVector2D(6.f, 11.f), FVector2D(1.f, 6.f), FVector2D(6.f, 1.f) });
		return IconBrush(TEXT("LootTetherDiamond"), Diamond, 4.f,
			FVector2D(UWeaponLabelWidget::DiamondSize, UWeaponLabelWidget::DiamondSize), FLinearColor::White);
	}
}

UWidget* UShieldLabelWidget::BuildFocusStack()
{
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FocusStack"));
	FocusStack = Stack;
	auto AddCentred = [Stack](UWidget* Child)
	{
		UVerticalBoxSlot* ChildSlot = Stack->AddChildToVerticalBox(Child);
		ChildSlot->SetHorizontalAlignment(HAlign_Center);
	};

	// The card; its intro unfolds it from its foot, so it rises out of the tether.
	Card = WidgetTree->ConstructWidget<UWardCardWidget>(UWardCardWidget::StaticClass(), TEXT("WardCard"));
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddCentred(Card);

	// The tether ties the card to its box among other loot lying near (the gun label's lengths).
	UImage* Tether = MakeImage(WidgetTree, RectBrush(Color::SaltLine()));
	AddCentred(MakeSized(WidgetTree, Tether, UWeaponLabelWidget::TetherWidth, UWeaponLabelWidget::TetherLength));
	TetherDiamond = MakeImage(WidgetTree, TetherDiamondBrush());
	AddCentred(MakeSized(WidgetTree, TetherDiamond, UWeaponLabelWidget::DiamondSize, UWeaponLabelWidget::DiamondSize));

	Stack->SetVisibility(ESlateVisibility::Collapsed);
	return Stack;
}

bool UShieldLabelWidget::IsCardShown() const
{
	const ESlateVisibility Shown = FocusStack ? FocusStack->GetVisibility() : ESlateVisibility::Collapsed;
	return Card && Ward.IsSet() && Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden;
}

void UShieldLabelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// The pickup checks the look a few times a second; the label checks it every frame, so the card opens the moment the
	// box is looked at (as a gun's, whose focus the weapon manager sets at once), and follows the worn ward and the hold.
	UpdateFromViewer();
}

void UShieldLabelWidget::UpdateFromViewer()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		const UWorld* World = GetWorld();
		PC = World ? World->GetFirstPlayerController() : nullptr;
	}
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (Source.IsValid() && Interaction)
	{
		SetFocused(Interaction->GetFocusedActor() == Source.Get());
	}
	if (bIsFocused)
	{
		UpdateCard(UPlayerShieldComponent::Find(Pawn), Interaction);
	}
}

void UShieldLabelWidget::UpdateCard(const UPlayerShieldComponent* Shield, const UInteractionComponent* Interaction)
{
	if (!Card || !Ward.IsSet() || !bIsFocused)
	{
		return;
	}
	// Compared with the ward worn (any ward is a ward: the user's like-with-like rule holds by itself). SetWard returns at
	// once when nothing changed.
	const FShieldInstanceData* Worn = Shield && Shield->HasWard() ? &Shield->GetWard() : nullptr;
	if (Card->SetWard(Ward, Worn, EWardCardMode::World))
	{
		bIntroPending = true;
	}

	// The keys: what a tap and a hold do with the ward worn and the backpack as they are (ShieldPickupRules), the HOLD cap
	// filling with the interact hold on this box.
	FWardCardKeys Keys;
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	if (Bindings)
	{
		Keys.Key = Bindings->GetKey(TEXT("Interact")).GetDisplayName().ToString().ToUpper();
	}
	Keys.bWorn = Worn != nullptr;
	Keys.bBackpackFull = Shield && !Shield->HasBackpackRoom();
	const AActor* Looked = Interaction ? Interaction->GetFocusedActor() : nullptr;
	Keys.Hold = Looked && Looked == Source.Get() ? Interaction->GetHoldProgress() : 0.f;
	Card->SetKeys(Keys);

	// A ward newly looked at: its card unfolds with the card's open sound.
	if (bIntroPending)
	{
		bIntroPending = false;
		Card->PlayIntro(/*bWithSound*/ true);
	}
}
