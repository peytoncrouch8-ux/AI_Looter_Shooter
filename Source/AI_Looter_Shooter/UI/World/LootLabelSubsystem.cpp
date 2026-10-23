// ULootLabelSubsystem: the loot labels on screen (guns' and wards') laid out every frame so none draws over another.

#include "UI/World/LootLabelSubsystem.h"
#include "Interaction/InteractionComponent.h"
#include "Shields/ShieldPickup.h"
#include "UI/World/LootLabelLayout.h"
#include "Weapons/WeaponBase.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarLootLabelsSpread(TEXT("Looter.LootLabels.Spread"), 1,
		TEXT("Loot labels near each other on screen step apart (1) or may overlap (0)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLootLabelsGap(TEXT("Looter.LootLabels.Gap"), 4.f,
		TEXT("Room kept between two loot labels stacked on screen (widget units)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLootLabelsEase(TEXT("Looter.LootLabels.Ease"), 14.f,
		TEXT("How fast a loot label slides to its place (per second; 0 jumps)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLootLabelsKeep(TEXT("Looter.LootLabels.Keep"), 0.5f,
		TEXT("How much a loot label prefers the side it is on to a smaller move (0 none)."), ECVF_Default);

	/** A label showing this frame: its widget, its component, and its box for the layout. */
	struct FShownLabel
	{
		UUserWidget* Widget = nullptr;
		FLootLabelBox Box;
	};

	/** Adds Label when it's showing, on screen, with its widget made. */
	void Gather(APlayerController& PC, const FVector& Eyes, const AActor& Owner, const AActor* Focused, UWidgetComponent* Label,
		TArray<FShownLabel>& OutLabels)
	{
		UUserWidget* Widget = Label && Label->IsVisible() ? Label->GetUserWidgetObject() : nullptr;
		if (!Widget)
		{
			return;
		}
		FVector2D Anchor;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(&PC, Label->GetComponentLocation(), Anchor, /*bPlayerViewportRelative*/ true))
		{
			return;
		}
		FShownLabel& Shown = OutLabels.AddDefaulted_GetRef();
		Shown.Widget = Widget;
		Shown.Box.Anchor = Anchor;
		Shown.Box.Size = Widget->GetDesiredSize();
		Shown.Box.bFocused = &Owner == Focused;
		Shown.Box.Distance = static_cast<float>(FVector::Dist(Eyes, Label->GetComponentLocation()));
	}
}

TStatId ULootLabelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULootLabelSubsystem, STATGROUP_Tickables);
}

bool ULootLabelSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void ULootLabelSubsystem::Deinitialize()
{
	Shown.Reset();
	Super::Deinitialize();
}

bool ULootLabelSubsystem::PlateNamesGun(const AWeaponBase& Weapon) const
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = PC && PC->IsLocalController() ? PC->GetPawn() : nullptr;
	const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	const UWidgetComponent* Label = Weapon.GetLabelComponent();
	return LootLabelLayout::PlateNamesGun(Interaction && Interaction->GetFocusedActor() == &Weapon, Label && Label->IsVisible());
}

void ULootLabelSubsystem::Release(UUserWidget* Widget)
{
	if (Widget)
	{
		Widget->SetRenderTranslation(FVector2D::ZeroVector);
	}
}

void ULootLabelSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	TArray<FShownLabel> Labels;
	if (PC && PC->IsLocalController() && CVarLootLabelsSpread.GetValueOnGameThread() != 0)
	{
		const APawn* Pawn = PC->GetPawn();
		const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
		const AActor* Focused = Interaction ? Interaction->GetFocusedActor() : nullptr;
		const FVector Eyes = Pawn ? Pawn->GetPawnViewLocation() : PC->GetFocalLocation();
		// Guns lying as loot and wards on the ground: their labels show only near the player, so this stays a handful.
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			if (It->IsPickup())
			{
				Gather(*PC, Eyes, **It, Focused, It->GetLabelComponent(), Labels);
			}
		}
		for (TActorIterator<AShieldPickup> It(World); It; ++It)
		{
			Gather(*PC, Eyes, **It, Focused, It->GetLabelComponent(), Labels);
		}
	}

	// Each label's shift now (where it was eased to), so the layout keeps it to its side.
	TArray<FLootLabelBox> Boxes;
	for (FShownLabel& Label : Labels)
	{
		const float* Was = Shown.Find(Label.Widget);
		Label.Box.Previous = Was ? *Was : 0.f;
		Boxes.Add(Label.Box);
	}
	const TArray<float> Targets = LootLabelLayout::Arrange(Boxes, CVarLootLabelsGap.GetValueOnGameThread(), CVarLootLabelsKeep.GetValueOnGameThread());

	// Labels no longer showing go back to where their component draws them.
	for (auto It = Shown.CreateIterator(); It; ++It)
	{
		UUserWidget* Widget = It->Key.Get();
		if (!Widget || !Labels.ContainsByPredicate([Widget](const FShownLabel& Label) { return Label.Widget == Widget; }))
		{
			Release(Widget);
			It.RemoveCurrent();
		}
	}
	const float Ease = CVarLootLabelsEase.GetValueOnGameThread();
	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		const float Was = Labels[Index].Box.Previous;
		const float Now = Ease > 0.f ? FMath::FInterpTo(Was, Targets[Index], DeltaTime, Ease) : Targets[Index];
		Shown.Add(Labels[Index].Widget, Now);
		Labels[Index].Widget->SetRenderTranslation(FVector2D(0.0, Now));
	}
}
