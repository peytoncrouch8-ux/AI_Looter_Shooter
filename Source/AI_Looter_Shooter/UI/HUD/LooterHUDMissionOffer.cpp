// ALooterHUD: the mission offer card (UMissionOfferWidget), opened by a giver: someone after their lines, a notice board's
// posting (over the board's screen, which it goes back to), an artifact picked up, or the console. The same card in its
// choice mode for a turn-in with a choice of two (OpenTurnInChoice), and in its reading mode for something read in the
// world (OpenMissionOffer with no mission, from an AReadableProp).

#include "UI/HUD/LooterHUD.h"
#include "AI_Looter_Shooter.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "UI/HUD/HudCaptionWidget.h"
#include "UI/World/BossShrineWidget.h"
#include "UI/World/BountyBoardWidget.h"
#include "UI/World/MissionOfferWidget.h"
#include "UI/World/NoticeBoardWidget.h"
#include "World/ReadableProp.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Over every other screen of the HUD (25), under the pause menu (40). */
	constexpr int32 OfferZOrder = 30;

	/** The captions while an offer card is open in its offer mode: over its dimming, so the giver's words read beside it. */
	constexpr int32 CaptionsOverCardZOrder = OfferZOrder + 1;
}

bool ALooterHUD::CoversCaptions() const
{
	// The offer card alone (not over a board's screen, the inventory or the pause menu): the giver talks on beside it.
	const bool bOfferTalks = bMissionOfferOpen && MissionOfferWidget && MissionOfferWidget->GetMode() == EMissionCardMode::Offer
		&& !bNoticeBoardOpen && !bInventoryOpen && !bPauseMenuOpen;
	return IsMenuOpen() && !bOfferTalks;
}

void ALooterHUD::LiftCaptions(bool bOver)
{
	if (!CaptionWidget || bCaptionsOverCard == bOver)
	{
		return;
	}
	// A viewport widget keeps the order it was added with: taken off and put back at the new one (its words and clock live
	// in the caption subsystem, so nothing said is lost).
	bCaptionsOverCard = bOver;
	CaptionWidget->RemoveFromParent();
	CaptionWidget->AddToViewport(bOver ? CaptionsOverCardZOrder : UHudCaptionWidget::ViewportZOrder);
}

bool ALooterHUD::OpenMissionOffer(const UMissionDefinition* Mission, AActor* Giver)
{
	APlayerController* PC = GetOwningPlayerController();
	// Only over the game or a notice board's screen: never over the inventory, the pause menu or another screen. With no
	// mission, it's something read (AReadableProp): the card in its reading mode.
	const bool bReading = !Mission && Cast<AReadableProp>(Giver) != nullptr;
	if (!PC || !PC->IsLocalController() || (!Mission && !bReading) || bPauseMenuOpen || bInventoryOpen || bStationBoardOpen || bBenchOpen
		|| bVendorOpen || UBossShrineWidget::GetOpen() != nullptr || UBountyBoardWidget::GetOpen() != nullptr)
	{
		return false;
	}
	// A mission already finished can't be offered again (the console names any mission).
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Mission && Runner && Runner->IsCompleted(Mission->GetMissionId()))
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

	if (bReading)
	{
		MissionOfferWidget->OpenReading(this, Giver, bNoticeBoardOpen);
	}
	else
	{
		MissionOfferWidget->Open(this, Mission, Giver, bNoticeBoardOpen);
	}
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
	// Someone's offer: their words go on beside the card (CoversCaptions), over its dimming.
	LiftCaptions(!bReading && !bNoticeBoardOpen);
	UE_LOG(LogLooter, Log, TEXT("%s: %s%s."), bReading ? TEXT("Reading card") : TEXT("Offer card"),
		bReading ? *Giver->GetActorNameOrLabel() : *Mission->GetMissionId().ToString(),
		Giver && !bReading ? *FString::Printf(TEXT(" from %s"), *Giver->GetActorNameOrLabel()) : TEXT(""));
	return true;
}

bool ALooterHUD::OpenTurnInChoice(const UMissionDefinition* Mission, AActor* Giver)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->IsLocalController() || !Mission || !Mission->HasTurnInChoice() || bPauseMenuOpen || bInventoryOpen || bStationBoardOpen
		|| bBenchOpen || bVendorOpen || UBossShrineWidget::GetOpen() != nullptr || UBountyBoardWidget::GetOpen() != nullptr)
	{
		return false;
	}
	// Only a mission waiting for its turn-in here has a choice to make now.
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !Runner->IsRunning(Mission->GetMissionId()) || !Runner->IsReadyToTurnIn(Mission->GetMissionId()))
	{
		UE_LOG(LogLooter, Log, TEXT("The choice card isn't opened: %s isn't waiting to be turned in here."), *Mission->GetMissionId().ToString());
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

	MissionOfferWidget->OpenChoice(this, Mission, Giver, bNoticeBoardOpen);
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
	LiftCaptions(false);
	UE_LOG(LogLooter, Log, TEXT("Choice card: %s%s."), *Mission->GetMissionId().ToString(),
		Giver ? *FString::Printf(TEXT(" at %s"), *Giver->GetActorNameOrLabel()) : TEXT(""));
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
	LiftCaptions(false);

	// Taken from a notice board's screen (or a choice made over it): back to the board, which shows the posting as it stands.
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
