// USpeakerPointComponent: a mission offered as someone talks. The talk's words come first; once they're said, and the
// player is still there with nothing covering the game, the offer card opens (ALooterHUD::OpenMissionOffer).

#include "Story/SpeakerPointComponent.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

namespace
{
	/** How often a waiting offer looks whether the talk's words are said (s). */
	constexpr float OfferCheckSeconds = 0.1f;
}

void USpeakerPointComponent::QueueChoice(const UMissionDefinition& Mission, AActor* Listener, int32 AfterConversation)
{
	// The same wait as an offer's (the words said, the player still there, nothing over the game), then the choice card.
	QueueOffer(Mission, Listener, AfterConversation);
	bPendingChoice = true;
}

void USpeakerPointComponent::QueueOffer(const UMissionDefinition& Mission, AActor* Listener, int32 AfterConversation)
{
	bPendingChoice = false;
	PendingOffer = Mission.GetMissionId();
	OfferConversation = AfterConversation;
	OfferListener = Listener;
	UWorld* World = GetWorld();
	OfferQueuedAt = World ? World->GetTimeSeconds() : 0.0;
	if (World)
	{
		World->GetTimerManager().SetTimer(OfferTimer, FTimerDelegate::CreateUObject(this, &USpeakerPointComponent::TickPendingOffer),
			OfferCheckSeconds, /*bLoop*/ true);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s will be offered once the words are said."), *GetOwner()->GetActorNameOrLabel(), *PendingOffer.ToString());
}

void USpeakerPointComponent::ClearPendingOffer()
{
	PendingOffer = NAME_None;
	bPendingChoice = false;
	OfferConversation = 0;
	OfferListener.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OfferTimer);
	}
}

void USpeakerPointComponent::TickPendingOffer()
{
	UpdatePendingOffer();
}

bool USpeakerPointComponent::UpdatePendingOffer()
{
	if (PendingOffer.IsNone())
	{
		ClearPendingOffer();
		return false;
	}
	// Taken another way meanwhile (a board, the console), or no longer offered: nothing to open. A choice still waits for it
	// here, ready to turn in.
	UMissionRunner* Runner = UMissionRunner::Get(this);
	const UMissionDefinition* Mission = Runner ? Runner->FindDefinition(PendingOffer) : nullptr;
	const bool bStillDue = Mission && (bPendingChoice ? Runner->FindTurnInAt(GetOwner()) == Mission : Runner->IsAvailable(PendingOffer));
	if (!bStillDue)
	{
		ClearPendingOffer();
		return false;
	}
	// Still saying it.
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (Captions && Captions->IsPlaying(OfferConversation))
	{
		return false;
	}
	// Walked off while they talked: the ! stays, and the next talk offers it.
	AActor* Listener = OfferListener.Get();
	ALooterHUD* HUD = ALooterHUD::FindFor(Listener);
	if (!Listener || !HUD || FVector::Dist(Listener->GetActorLocation(), GetComponentLocation()) > OfferWalkAway)
	{
		ClearPendingOffer();
		return false;
	}
	// A menu or a scene over the game: it waits a while for them to go, then is left for the next talk.
	if (HUD->IsMenuOpen() || USceneSubsystem::HidesGameplayHUD(this))
	{
		const UWorld* World = GetWorld();
		if (World && World->GetTimeSeconds() - OfferQueuedAt > OfferPatience)
		{
			ClearPendingOffer();
		}
		return false;
	}
	const bool bOpened = bPendingChoice ? HUD->OpenTurnInChoice(Mission, GetOwner()) : HUD->OpenMissionOffer(Mission, GetOwner());
	ClearPendingOffer();
	return bOpened;
}
