// ALooterHUD: the world screens (a station board, the bench, a notice board, a vendor, an offer card) close once the player
// is out of reach of what they belong to, as the bounty board's, a Vigil Stone's and Lost Property's do.

#include "UI/HUD/LooterHUD.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/Interactable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Where Anchor is used (a board's face, the shop's door), as its Interact reach is measured; else where it stands. */
	FVector UsedAt(const AActor& Anchor)
	{
		const IInteractable* Used = Cast<IInteractable>(&Anchor);
		return Used ? Used->GetInteractionLocation().Get(Anchor.GetActorLocation()) : Anchor.GetActorLocation();
	}
}

bool ALooterHUD::IsWithinScreenReach(const AActor* Anchor) const
{
	const APlayerController* PC = GetOwningPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	return Anchor && Pawn && FVector::Dist(Pawn->GetActorLocation(), UsedAt(*Anchor)) <= ScreenStayRadius;
}

void ALooterHUD::CloseScreensOutOfReach()
{
	const bool bAnyOpen = bStationBoardOpen || bBenchOpen || bNoticeBoardOpen || bVendorOpen || bMissionOfferOpen;
	if (!bAnyOpen)
	{
		// Nothing open: the last screen's anchor is forgotten, so a later one opened without any is never closed by it.
		ScreenAnchor.Reset();
		return;
	}
	const AActor* Anchor = ScreenAnchor.Get();
	const APlayerController* PC = GetOwningPlayerController();
	if (!Anchor || !PC || !PC->GetPawn() || IsWithinScreenReach(Anchor))
	{
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("HUD: the player is %.0f cm from %s: its screen closes."),
		FVector::Dist(PC->GetPawn()->GetActorLocation(), UsedAt(*Anchor)), *Anchor->GetActorNameOrLabel());
	ScreenAnchor.Reset();
	// The card over a board first, then what's under it.
	CloseMissionOffer();
	CloseNoticeBoard();
	CloseVendor();
	CloseBench();
	CloseStationBoard();
}
