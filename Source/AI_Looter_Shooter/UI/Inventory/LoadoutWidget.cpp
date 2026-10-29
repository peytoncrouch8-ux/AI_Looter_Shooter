// ULoadoutWidget: opening and closing, refreshing the page, and what it reads from the inventory. The page's layout is
// LoadoutWidgetLayout.cpp, the rows LoadoutWidgetRows.cpp, the cards and the showcase LoadoutWidgetInspect.cpp, the cursor
// in the two columns LoadoutWidgetColumns.cpp, the actions and keys LoadoutWidgetInput.cpp, dragging LoadoutWidgetDrag.cpp,
// the showcase's ring and the motion LoadoutWidgetPaint.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "AI_Looter_Shooter.h"
#include "UI/Cards/GunCardWidget.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

using namespace LooterUI;
using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// Opening and closing
// ---------------------------------------------------------------------------

void ULoadoutWidget::Open(ALooterHUD* InHUD, UWeaponManagerComponent* InManager)
{
	OwningHUD = InHUD;
	if (Manager.Get() != InManager)
	{
		if (UWeaponManagerComponent* Old = Manager.Get())
		{
			Old->OnInventoryChanged.RemoveDynamic(this, &ULoadoutWidget::Refresh);
			Old->OnAmmoChanged.RemoveDynamic(this, &ULoadoutWidget::HandleAmmoChanged);
		}
		Manager = InManager;
		if (InManager)
		{
			InManager->OnInventoryChanged.AddUniqueDynamic(this, &ULoadoutWidget::Refresh);
			InManager->OnAmmoChanged.AddUniqueDynamic(this, &ULoadoutWidget::HandleAmmoChanged);
		}
	}
	SetIsFocusable(true);

	// The showcase is spawned the first time, then kept, idle while the screen is closed. Only in a played world: a test's
	// editor world builds the screen without it.
	UWorld* World = GetWorld();
	if (!Stage.IsValid() && World && World->IsGameWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Stage = World->SpawnActor<ALoadoutGunStage>(Params);
	}
	if (ALoadoutGunStage* StagePtr = Stage.Get())
	{
		if (!StageMaterial)
		{
			// The inventory's picture material: it cuts the model out of the capture and tone maps it.
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, StageMaterialPath))
			{
				StageMaterial = UMaterialInstanceDynamic::Create(Base, this);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Loadout screen: %s is missing, so the gun can't be shown."), StageMaterialPath);
			}
		}
		if (StageMaterial)
		{
			StageMaterial->SetTextureParameterValue(TEXT("Capture"), StagePtr->GetRenderTarget());
		}
		ApplyStageBrush();
		StagePtr->SetActive(true);
	}

	// The guns carried the first time it opens aren't news; anything found after is NEW until the cursor has been on it.
	if (!bSeenPrimed && InManager)
	{
		for (const AWeaponBase* Weapon : InManager->GetWeapons())
		{
			if (Weapon)
			{
				SeenGuns.Add(LoadoutRules::GunIdentity(Weapon->GetInstance()));
			}
		}
		for (const FWeaponInstanceData& Gun : InManager->GetBackpack())
		{
			SeenGuns.Add(LoadoutRules::GunIdentity(Gun));
		}
		bSeenPrimed = true;
	}

	// Start on the gun in hand (the swap target too), at a glance, nothing picked up or dragged.
	PickedSlot.Reset();
	// A drag still going when it was last closed (Tab mid-drag) would show its ghost card here.
	ClearItemDrag();
	bShowcasePress = bTurning = false;
	bInspecting = false;
	Zone = EZone::Slots;
	TargetSlot = CursorIndex = InManager ? FMath::Max(InManager->GetActiveSlot(), 0) : 0;
	CardIdentity = 0;
	Clock = 0.f;
	ApplyMode();
	Refresh();
}

void ULoadoutWidget::Close()
{
	PickedSlot.Reset();
	if (ALooterHUD* HUD = OwningHUD.Get())
	{
		HUD->CloseInventory();
	}
}

void ULoadoutWidget::NativeDestruct()
{
	// Off screen: the showcase stops rendering, and a gun being dragged is let go (its ghost card is put away with it).
	ClearItemDrag();
	bShowcasePress = bTurning = false;
	TurnInput = 0.f;
	if (ALoadoutGunStage* StagePtr = Stage.Get())
	{
		StagePtr->SetActive(false);
	}
	Super::NativeDestruct();
}

void ULoadoutWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TickMotion(InDeltaTime);
	FitGunCard();

	ALoadoutGunStage* StagePtr = Stage.Get();
	if (!StagePtr)
	{
		return;
	}
	if (!FMath::IsNearlyZero(TurnInput))
	{
		StagePtr->AddTurn(TurnInput * StickTurnRate * InDeltaTime);
	}
	if (StageMaterial)
	{
		StageMaterial->SetScalarParameterValue(TEXT("Exposure"), StagePtr->GetExposure());
	}
}

void ULoadoutWidget::FitGunCard()
{
	// The card's height changes with the gun (stat rows, bullet lines); its layout size is known after a pass, so it is fitted
	// each frame from its desired size, shrunk from its top so its foot never covers the key hints.
	if (!GunCard || GunCard->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}
	const float Height = GunCard->GetDesiredSize().Y;
	if (Height <= 1.f)
	{
		return;
	}
	const float Room = LoadoutLayout::PromptBarPosition.Y - LoadoutLayout::CardFootGap - LoadoutLayout::CardTop;
	const float Scale = FMath::Clamp(Room / Height, LoadoutLayout::CardMinScale, 1.f);
	if (!FMath::IsNearlyEqual(static_cast<float>(GunCard->GetRenderTransform().Scale.Y), Scale, 0.005f))
	{
		GunCard->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
		GunCard->SetRenderScale(FVector2D(Scale, Scale));
	}
}

// ---------------------------------------------------------------------------
// Contents
// ---------------------------------------------------------------------------

void ULoadoutWidget::Refresh()
{
	UWeaponManagerComponent* Inventory = Manager.Get();
	if (bApplyingAction || !SlotList || !Inventory)
	{
		return;
	}
	// The inventory may have changed under the cursor and the pick.
	if (PickedSlot.IsSet() && !SlotItem(PickedSlot.GetValue()))
	{
		PickedSlot.Reset();
	}
	TargetSlot = FMath::Clamp(TargetSlot, 0, FMath::Max(NumSlots() - 1, 0));

	RebuildSlots();
	RebuildWards();
	RebuildList();
	if (Zone == EZone::Backpack && ListRows.IsEmpty())
	{
		Zone = EZone::Slots;
		CursorIndex = TargetSlot;
	}
	CursorIndex = FMath::Clamp(CursorIndex, 0, FMath::Max((Zone == EZone::Slots ? SlotRows.Num() : (Zone == EZone::Wards ? WardRows.Num() : ListRows.Num())) - 1, 0));
	MarkSeen();
	Restyle();
	RefreshCounts();
	RefreshCard();
	RefreshAmmo();
	RefreshPrompts();
	RefreshShowcase();
}

void ULoadoutWidget::HandleAmmoChanged(EAmmoType Type, int32 Carried)
{
	RefreshAmmo();
}

ULoadoutWidget::FView ULoadoutWidget::GetView() const
{
	FView View;
	View.bOnSlot = Zone == EZone::Slots;
	View.bOnWard = Zone == EZone::Wards;
	View.Cursor = CursorIndex;
	View.TargetSlot = TargetSlot;
	View.bInspecting = bInspecting;
	View.Sort = Sort;
	View.ListOrder = ListOrder;
	for (const FRow& Row : ListRows)
	{
		View.JunkRows.Add(Row.JunkMark != nullptr);
	}
	for (const FPrompt& Prompt : CurrentPrompts())
	{
		View.Prompts.Add(FString::Printf(TEXT("%s: %s"), *Prompt.Key, *Prompt.Text));
	}
	if (const UCanvasPanelSlot* LeftSlot = LeftColumn ? Cast<UCanvasPanelSlot>(LeftColumn->Slot) : nullptr)
	{
		View.EquippedX = LeftSlot->GetPosition().X;
	}
	if (const UCanvasPanelSlot* RightSlot = RightColumn ? Cast<UCanvasPanelSlot>(RightColumn->Slot) : nullptr)
	{
		View.BackpackX = RightSlot->GetPosition().X;
	}
	View.EmptyBackpackRows = NumEmptyRows;
	View.bGunCard = GunCard && GunCard->GetVisibility() != ESlateVisibility::Collapsed && GunCard->GetVisibility() != ESlateVisibility::Hidden;
	if (View.bGunCard)
	{
		View.CardName = GunCard->GetNameText().ToString();
		View.CardVerdict = GunCard->GetVerdictText().ToString();
	}
	return View;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const FWeaponInstanceData* ULoadoutWidget::SlotItem(int32 SlotIndex) const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory)
	{
		return nullptr;
	}
	const TArray<AWeaponBase*> Weapons = Inventory->GetWeapons();
	return Weapons.IsValidIndex(SlotIndex) && Weapons[SlotIndex] ? &Weapons[SlotIndex]->GetInstance() : nullptr;
}

const FWeaponInstanceData* ULoadoutWidget::ListItem(int32 Row) const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || !ListOrder.IsValidIndex(Row))
	{
		return nullptr;
	}
	const TArray<FWeaponInstanceData>& Backpack = Inventory->GetBackpack();
	return Backpack.IsValidIndex(ListOrder[Row]) ? &Backpack[ListOrder[Row]] : nullptr;
}

const FWeaponInstanceData* ULoadoutWidget::CursorItem() const
{
	return Zone == EZone::Slots ? SlotItem(CursorIndex) : (Zone == EZone::Backpack ? ListItem(CursorIndex) : nullptr);
}

const FWeaponInstanceData* ULoadoutWidget::Baseline() const
{
	// A backpack gun is compared with the gun it would replace, and only when that's the same kind: a shotgun's eight
	// shells against a rifle's thirty rounds would be all red arrows and no news.
	const FWeaponInstanceData* Gun = Zone == EZone::Backpack ? ListItem(CursorIndex) : nullptr;
	const FWeaponInstanceData* Target = SlotItem(TargetSlot);
	return Gun && Target && Gun->Definition && Gun->Definition == Target->Definition ? Target : nullptr;
}

int32 ULoadoutWidget::NumSlots() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? Inventory->MaxWeapons : 0;
}

int32 ULoadoutWidget::NumWeapons() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? Inventory->GetWeapons().Num() : 0;
}

int32 ULoadoutWidget::GetActiveSlot() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? Inventory->GetActiveSlot() : INDEX_NONE;
}

bool ULoadoutWidget::HasBackpackRoom() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory && Inventory->GetBackpack().Num() < Inventory->BackpackCapacity;
}

bool ULoadoutWidget::IsNew(uint32 Identity) const
{
	return Identity != 0 && !SeenGuns.Contains(Identity);
}

FLinearColor ULoadoutWidget::RarityColor(const FWeaponInstanceData* Gun) const
{
	return Gun ? LooterWeaponText::Color(*Gun) : Color::TextDim();
}
