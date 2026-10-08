// ALooterHUD: the gunsmith's bench's screen, and the sound every page makes as it opens and closes.

#include "UI/HUD/LooterHUD.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Bench/BenchWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Templates/UnrealTemplate.h"

void ALooterHUD::PlayPageSound(bool bOpening) const
{
	if (bOpening || !bQuietClose)
	{
		LooterSound::Play2D(this, bOpening ? LooterSoundCue::Open : LooterSoundCue::Close);
	}
}

bool ALooterHUD::OpenBench(AActor* Bench)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || bPauseMenuOpen)
	{
		return false;
	}
	const APawn* Pawn = PC->GetPawn();
	UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (!Manager)
	{
		return false;
	}
	{
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseInventory();
		CloseStationBoard();
	}
	if (!BenchWidget)
	{
		BenchWidget = CreateWidget<UBenchWidget>(PC, UBenchWidget::StaticClass());
	}
	if (!BenchWidget)
	{
		return false;
	}
	Manager->StopFire();

	// The missions hear of it from the player's interaction component (a tap on the bench), not from here.
	BenchWidget->Open(this, Manager, Bench);
	if (!bBenchOpen)
	{
		BenchWidget->AddToViewport(25);
		PlayPageSound(true);
	}
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(BenchWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	bBenchOpen = true;
	return true;
}

void ALooterHUD::CloseBench()
{
	if (!bBenchOpen)
	{
		return;
	}
	if (BenchWidget)
	{
		BenchWidget->RemoveFromParent();
	}
	bBenchOpen = false;
	RestoreGameInput();
	PlayPageSound(false);
}
