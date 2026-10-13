// ALooterHUD: the mission offer card (UMissionOfferWidget), opened by a giver: someone after their lines, a notice board's
// posting (over the board's screen, which it goes back to), an artifact picked up, or the console.

#include "UI/HUD/LooterHUD.h"
#include "AI_Looter_Shooter.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "UI/World/BossShrineWidget.h"
#include "UI/World/MissionOfferWidget.h"
#include "UI/World/NoticeBoardWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Over every other screen of the HUD (25), under the pause menu (40). */
	constexpr int32 OfferZOrder = 30;
}

bool ALooterHUD::OpenMissionOffer(const UMissionDefinition* Mission, AActor* Giver)
{
	APlayerController* PC = GetOwningPlayerController();
	// Only over the game or a notice board's screen: never over the inventory, the pause menu or another screen.
	if (!PC || !PC->IsLocalController() || !Mission || bPauseMenuOpen || bInventoryOpen || bStationBoardOpen || bBenchOpen
		|| bVendorOpen || UBossShrineWidget::GetOpen() != nullptr)
	{
		return false;
	}
	// A mission already finished can't be offered again (the console names any mission).
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && Runner->IsCompleted(Mission->GetMissionId()))
	{
		UE_LOG(LogLooter, Log, TEXT("The offer card isn't opened: %s is finished."), *Mission->GetMissionId().ToString());
		return false;
	}
	if (!MissionOfferWidget)
	{
		MissionOfferWidget = CreateWidget<UMissionOfferWidget>(PC, UMissionOfferWidget::StaticClass());
	}
	if (!MissionOfferWidget)
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

	MissionOfferWidget->Open(this, Mission, Giver, bNoticeBoardOpen);
	if (!bMissionOfferOpen)
	{
		MissionOfferWidget->AddToViewport(OfferZOrder);
	}
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MissionOfferWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
	bMissionOfferOpen = true;
	UE_LOG(LogLooter, Log, TEXT("Offer card: %s%s."), *Mission->GetMissionId().ToString(),
		Giver ? *FString::Printf(TEXT(" from %s"), *Giver->GetActorNameOrLabel()) : TEXT(""));
	return true;
}

void ALooterHUD::CloseMissionOffer()
{
	if (!bMissionOfferOpen)
	{
		return;
	}
	const UMissionDefinition* Mission = MissionOfferWidget ? MissionOfferWidget->GetMission() : nullptr;
	const bool bAccepted = MissionOfferWidget && MissionOfferWidget->WasAccepted();
	if (MissionOfferWidget)
	{
		MissionOfferWidget->RemoveFromParent();
	}
	bMissionOfferOpen = false;

	// Taken from a notice board's screen: back to the board, which shows the posting taken.
	APlayerController* PC = GetOwningPlayerController();
	if (bNoticeBoardOpen && NoticeBoardWidget && PC)
	{
		NoticeBoardWidget->HandleOfferClosed(Mission ? Mission->GetMissionId() : NAME_None, bAccepted);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(NoticeBoardWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
		return;
	}
	RestoreGameInput();
}
