// ULootLabelSubsystem: the loot labels on screen (guns' and wards') laid out every frame so none draws over another, and
// a pile's guns past the first few folded to their rarity's dot.

#include "UI/World/LootLabelSubsystem.h"
#include "Interaction/InteractionComponent.h"
#include "Shields/ShieldPickup.h"
#include "UI/World/LootLabelLayout.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponTypes.h"
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
	// A pile of guns (a boss's drop, a strongbox's) once stacked a tower of names (playtest 2026-10-27): four names read at a
	// glance, the looked-at one among them, and the rest fold to their rarity's dot until looked at.
	TAutoConsoleVariable<int32> CVarLootLabelsPileShown(TEXT("Looter.LootLabels.PileShown"), 4,
		TEXT("Gun names shown in full within one pile, the looked-at one included; the rest fold to a rarity dot (0: every name shows)."), ECVF_Default);
	// About a strongbox's spill or a boss's drop: guns this close count as one pile.
	TAutoConsoleVariable<float> CVarLootLabelsPileRadius(TEXT("Looter.LootLabels.PileRadius"), 300.f,
		TEXT("Guns lying within this distance of each other (cm) count as one pile for Looter.LootLabels.PileShown."), ECVF_Default);

	/** A legendary's name always shows in full, however many lie with it: it's the find of the day. */
	constexpr int32 AlwaysShownRank = static_cast<int32>(EWeaponRarity::Legendary);

	/** A label showing this frame: its widget, its box for the layout, and for a gun's, its place in a pile. */
	struct FShownLabel
	{
		UUserWidget* Widget = nullptr;
		FLootLabelBox Box;
		/** A gun's label (wards' never fold). */
		UWeaponLabelWidget* GunLabel = nullptr;
		FLootPileLabel Pile;
	};

	/** Adds Label when it's showing, on screen, with its widget made. */
	FShownLabel* Gather(APlayerController& PC, const FVector& Eyes, const AActor& Owner, const AActor* Focused, UWidgetComponent* Label,
		TArray<FShownLabel>& OutLabels)
	{
		UUserWidget* Widget = Label && Label->IsVisible() ? Label->GetUserWidgetObject() : nullptr;
		if (!Widget)
		{
			return nullptr;
		}
		FVector2D Anchor;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(&PC, Label->GetComponentLocation(), Anchor, /*bPlayerViewportRelative*/ true))
		{
			return nullptr;
		}
		FShownLabel& Shown = OutLabels.AddDefaulted_GetRef();
		Shown.Widget = Widget;
		Shown.Box.Anchor = Anchor;
		Shown.Box.Size = Widget->GetDesiredSize();
		Shown.Box.bFocused = &Owner == Focused;
		Shown.Box.Distance = static_cast<float>(FVector::Dist(Eyes, Label->GetComponentLocation()));
		return &Shown;
	}

	/**
	 * Which gun names show in full in each pile (LootLabelLayout::PickShown); the rest fold to their dot and leave Labels,
	 * so the layout never moves them (a dot sits on its gun, and a scatter of dots may touch). Folded: the labels folded
	 * last frame, kept up to date.
	 */
	void FoldPiles(TArray<FShownLabel>& Labels, TSet<TWeakObjectPtr<UWeaponLabelWidget>>& Folded)
	{
		TArray<FLootPileLabel> Piled;
		TArray<int32> GunIndices;
		for (int32 Index = 0; Index < Labels.Num(); ++Index)
		{
			if (Labels[Index].GunLabel)
			{
				Labels[Index].Pile.bWasShown = !Labels[Index].GunLabel->IsFolded();
				Piled.Add(Labels[Index].Pile);
				GunIndices.Add(Index);
			}
		}
		const TArray<bool> InFull = LootLabelLayout::PickShown(Piled, CVarLootLabelsPileRadius.GetValueOnGameThread(),
			CVarLootLabelsPileShown.GetValueOnGameThread(), AlwaysShownRank);

		TSet<TWeakObjectPtr<UWeaponLabelWidget>> NowFolded;
		for (int32 Pick = 0; Pick < GunIndices.Num(); ++Pick)
		{
			UWeaponLabelWidget* GunLabel = Labels[GunIndices[Pick]].GunLabel;
			GunLabel->SetFolded(!InFull[Pick]);
			if (!InFull[Pick])
			{
				NowFolded.Add(GunLabel);
			}
		}
		// Folded before and not now (opened, or no longer showing: picked up, out of range): opened, so it comes back whole.
		for (const TWeakObjectPtr<UWeaponLabelWidget>& Was : Folded)
		{
			UWeaponLabelWidget* GunLabel = Was.Get();
			if (GunLabel && !NowFolded.Contains(Was))
			{
				GunLabel->SetFolded(false);
			}
		}
		Folded = MoveTemp(NowFolded);
		Labels.RemoveAll([](const FShownLabel& Label) { return Label.GunLabel && Label.GunLabel->IsFolded(); });
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
	Folded.Reset();
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
	if (PC && PC->IsLocalController())
	{
		const APawn* Pawn = PC->GetPawn();
		const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
		const AActor* Focused = Interaction ? Interaction->GetFocusedActor() : nullptr;
		const FVector Eyes = Pawn ? Pawn->GetPawnViewLocation() : PC->GetFocalLocation();
		// Guns lying as loot and wards on the ground: their labels show only near the player, so this stays a handful.
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			if (!It->IsPickup())
			{
				continue;
			}
			if (FShownLabel* Label = Gather(*PC, Eyes, **It, Focused, It->GetLabelComponent(), Labels))
			{
				Label->GunLabel = Cast<UWeaponLabelWidget>(Label->Widget);
				Label->Pile.Location = It->GetActorLocation();
				Label->Pile.Distance = Label->Box.Distance;
				Label->Pile.Rank = static_cast<int32>(It->GetInstance().Rarity);
				Label->Pile.bFocused = Label->Box.bFocused;
			}
		}
		for (TActorIterator<AShieldPickup> It(World); It; ++It)
		{
			Gather(*PC, Eyes, **It, Focused, It->GetLabelComponent(), Labels);
		}
	}
	FoldPiles(Labels, Folded);
	// Spread off: every label where its component draws it (the folding above still holds).
	if (CVarLootLabelsSpread.GetValueOnGameThread() == 0)
	{
		Labels.Reset();
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
	// The looked-at card stays wholly on screen (it may cover its gun then, never run off the top).
	const FVector2D View = PC ? UWidgetLayoutLibrary::GetViewportSize(PC) / FMath::Max(UWidgetLayoutLibrary::GetViewportScale(PC), 0.01f)
		: FVector2D::ZeroVector;
	for (int32 Index = 0; Index < Labels.Num(); ++Index)
	{
		const float Was = Labels[Index].Box.Previous;
		const float Now = Ease > 0.f ? FMath::FInterpTo(Was, Targets[Index], DeltaTime, Ease) : Targets[Index];
		Shown.Add(Labels[Index].Widget, Now);
		const FVector2D Keep = Labels[Index].Box.bFocused ? LootLabelLayout::KeepOnScreen(Labels[Index].Box, Now, View, 16.f) : FVector2D::ZeroVector;
		Labels[Index].Widget->SetRenderTranslation(FVector2D(Keep.X, Now + Keep.Y));
	}
}
