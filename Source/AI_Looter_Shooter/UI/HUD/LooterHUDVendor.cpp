// ALooterHUD: a vendor's screen (UVendorWidget), opened by trading at a vendor (AVendor).

#include "UI/HUD/LooterHUD.h"
#include "Economy/Vendor.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Vendor/VendorWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Templates/UnrealTemplate.h"

bool ALooterHUD::OpenVendor(AVendor* Vendor)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || bPauseMenuOpen || !Vendor)
	{
		return false;
	}
	{
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseInventory();
		CloseStationBoard();
		CloseBench();
		CloseNoticeBoard();
	}
	if (!VendorWidget)
	{
		VendorWidget = CreateWidget<UVendorWidget>(PC, UVendorWidget::StaticClass());
	}
	if (!VendorWidget)
	{
		return false;
	}
	if (const APawn* Pawn = PC->GetPawn())
	{
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Manager->StopFire();
		}
	}

	VendorWidget->Open(this, Vendor);
	if (!bVendorOpen)
	{
		VendorWidget->AddToViewport(25);
		PlayPageSound(true);
	}
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(VendorWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	bVendorOpen = true;
	return true;
}

void ALooterHUD::CloseVendor()
{
	if (!bVendorOpen)
	{
		return;
	}
	if (VendorWidget)
	{
		VendorWidget->RemoveFromParent();
	}
	bVendorOpen = false;
	RestoreGameInput();
	PlayPageSound(false);
}
