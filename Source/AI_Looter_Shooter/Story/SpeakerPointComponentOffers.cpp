// USpeakerPointComponent: a mission offered as someone talks. The offer card opens at once, the talk's words said beside it
// (the user's call, 2026-10-22: Accept never waits for them), once the player is still there with nothing covering the game
// (ALooterHUD::OpenMissionOffer); Accept cuts the words short for the job's accept lines (SayInstead), Not now stops them
// (StopTalking). A turn-in's choice card, and a job offered after a turn-in, wait for the turn-in's words first. A main
// mission met first (FMissionOffer::bMeetFirst) gets no card: the talk starts it (StartMet).

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
	/** How often a waiting offer looks whether it can open (s). */
	constexpr float OfferCheckSeconds = 0.1f;
}

void USpeakerPointComponent::QueueChoice(const UMissionDefinition& Mission, AActor* Listener, int32 AfterConversation)
{
	// The same wait as an offer after a turn-in (the words said, the player still there, nothing over the game), then the
	// choice card.
	QueueOffer(Mission, Listener, AfterConversation, /*bWaitForWords*/ true);
	bPendingChoice = true;
}

void USpeakerPointComponent::QueueOffer(const UMissionDefinition& Mission, AActor* Listener, int32 AfterConversation, bool bWaitForWords)
{
	bPendingChoice = false;
	bOfferWaitsForWords = bWaitForWords;
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
	UE_LOG(LogLooter, Log, TEXT("%s: %s will be offered %s."), *GetOwner()->GetActorNameOrLabel(), *PendingOffer.ToString(),
		bWaitForWords ? TEXT("once the words before it are said") : TEXT("at once, beside the words"));
}

void USpeakerPointComponent::ClearPendingOffer()
{
	PendingOffer = NAME_None;
	bPendingChoice = false;
	bOfferWaitsForWords = true;
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
	// A choice waits for the turn-in's words to be said. A job offered after a turn-in waits for whatever comes before its
	// own words (the turn-in, a story mission's words on the move), so Accept never cuts them short: it comes up with its
	// own words on screen, or the silence after. A job offered as they talk opens at once, beside its words.
	if (const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		if (bPendingChoice && Captions->IsPlaying(OfferConversation))
		{
			return false;
		}
		const FCaptionEntry* OnScreen = Captions->GetCurrent();
		if (!bPendingChoice && bOfferWaitsForWords && OnScreen && OnScreen->Conversation != OfferConversation)
		{
			return false;
		}
	}
	// Walked off meanwhile: the ? stays, and the next talk offers it.
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

// ---------------------------------------------------------------------------
// Around the taking: the card's Accept and Not now, a main mission met first
// ---------------------------------------------------------------------------

void USpeakerPointComponent::StopTalking()
{
	// Only while their words are still being said (their talk interrupted whatever came before it; a remark queued behind
	// it goes with it).
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (Captions && IsTalking())
	{
		Captions->Clear();
	}
}

int32 USpeakerPointComponent::SayInstead(const TArray<FStoryLine>& Said)
{
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (!Captions || Said.IsEmpty())
	{
		StopTalking();
		return 0;
	}
	// They still have the floor: what they were saying stops, these come at once, and they can't be talked over meanwhile.
	Conversation = Captions->Play(InOwnName(Said), ECaptionPlay::Interrupt);
	return Conversation;
}

void USpeakerPointComponent::StartMet(const UMissionDefinition& Mission)
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	const FName MissionId = Mission.GetMissionId();
	if (!Runner || !Runner->AcceptMission(MissionId))
	{
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s started by this talk (a main mission is never offered on a card)."), *GetOwner()->GetActorNameOrLabel(),
		*MissionId.ToString());
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (Captions && !Mission.Offer.AcceptLines.IsEmpty())
	{
		// After the words that started it, as one talk.
		const int32 Queued = Captions->Play(InOwnName(Mission.Offer.AcceptLines), ECaptionPlay::Queue);
		Conversation = Queued != 0 ? Queued : Conversation;
	}
}
