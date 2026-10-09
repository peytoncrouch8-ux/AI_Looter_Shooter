// ALooterHUD: a notice board's screen (Skyreach's postings: UNoticeBoardWidget), opened by reading a board.

#include "UI/HUD/LooterHUD.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/World/NoticeBoardWidget.h"
#include "World/NoticeBoard.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Templates/UnrealTemplate.h"

bool ALooterHUD::OpenNoticeBoard(ANoticeBoard* Board)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || bPauseMenuOpen || !Board)
	{
		return false;
	}
	{
		TGuardValue<bool> Quiet(bQuietClose, true);
		CloseInventory();
		CloseStationBoard();
		CloseBench();
	}
	if (!NoticeBoardWidget)
	{
		NoticeBoardWidget = CreateWidget<UNoticeBoardWidget>(PC, UNoticeBoardWidget::StaticClass());
	}
	if (!NoticeBoardWidget)
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

	// The missions heard the read from the board itself, before this: the screen lists the postings it put up.
	NoticeBoardWidget->Open(this, Board);
	if (!bNoticeBoardOpen)
	{
		NoticeBoardWidget->AddToViewport(25);
		PlayPageSound(true);
	}
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(NoticeBoardWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	bNoticeBoardOpen = true;
	return true;
}

void ALooterHUD::CloseNoticeBoard()
{
	if (!bNoticeBoardOpen)
	{
		return;
	}
	if (NoticeBoardWidget)
	{
		NoticeBoardWidget->RemoveFromParent();
	}
	bNoticeBoardOpen = false;
	RestoreGameInput();
	PlayPageSound(false);
}
