#include "Story/SpeakerPointComponent.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLineSet.h"
#include "GameFramework/Actor.h"

USpeakerPointComponent::USpeakerPointComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Prompt = FText::FromString(TEXT("Talk"));
}

void USpeakerPointComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	// The player's interaction component looks among the level's interactables: the actor this is on joins them, and
	// hands the Interact key on to it.
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(Owner);
	}
	if (!Owner->Implements<UInteractable>())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: its speaker point is on an actor the Interact key doesn't use, so nobody can talk there. ")
			TEXT("Put it on a speaker point (ASpeakerPoint) or a story character (AStoryCharacter)."), *Owner->GetActorNameOrLabel());
	}
}

void USpeakerPointComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(GetOwner());
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// The Interact key
// ---------------------------------------------------------------------------

FInteractionOptions USpeakerPointComponent::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.bUsable = CanTalk();
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool USpeakerPointComponent::Interact(UInteractionComponent& User, bool bHeld)
{
	return Talk(User.GetOwner());
}

TOptional<FVector> USpeakerPointComponent::GetInteractionLocation() const
{
	return GetComponentLocation();
}

// ---------------------------------------------------------------------------
// Talking
// ---------------------------------------------------------------------------

bool USpeakerPointComponent::CanTalk() const
{
	const AActor* Owner = GetOwner();
	return bEnabled && Owner && !Owner->IsHidden() && !IsTalking();
}

bool USpeakerPointComponent::IsTalking() const
{
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	return Captions && Captions->IsPlaying(Conversation);
}

TArray<FStoryLine> USpeakerPointComponent::GetLinesNow(int32* OutTopic) const
{
	int32 Chosen = INDEX_NONE;
	if (const UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		const FCampaignRecord& Campaign = Runner->GetCampaign();
		for (int32 Index = 0; Index < Topics.Num(); ++Index)
		{
			if (Topics[Index].When.IsMet(Campaign, Runner))
			{
				Chosen = Index;
				break;
			}
		}
	}
	if (OutTopic)
	{
		*OutTopic = Chosen;
	}

	TArray<FStoryLine> Said;
	if (Topics.IsValidIndex(Chosen))
	{
		const FSpeakerTopic& Topic = Topics[Chosen];
		Said = Topic.LineSet ? Topic.LineSet->Lines : Topic.Lines;
	}
	else
	{
		Said = LineSet ? LineSet->Lines : Lines;
	}
	// Written once on the point, not on every line.
	for (FStoryLine& Line : Said)
	{
		if (Line.Speaker.IsEmpty())
		{
			Line.Speaker = SpeakerName;
		}
	}
	return Said;
}

bool USpeakerPointComponent::Talk(AActor* Listener)
{
	if (!CanTalk())
	{
		return false;
	}
	AActor* Owner = GetOwner();
	int32 TopicIndex = INDEX_NONE;
	const TArray<FStoryLine> Said = GetLinesNow(&TopicIndex);
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		// Whoever the player turns to talk to has the floor: whatever was being said stops.
		Conversation = Captions->Play(Said, ECaptionPlay::Interrupt);
	}
	if (Said.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: %s has nothing to say here (no lines for this point in the story)."), *Owner->GetActorNameOrLabel(),
			*SpeakerName.ToString());
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s talks (%d lines%s)."), *Owner->GetActorNameOrLabel(), *SpeakerName.ToString(), Said.Num(),
		TopicIndex == INDEX_NONE ? TEXT("") : *FString::Printf(TEXT(", topic %d"), TopicIndex + 1));

	// The talk objective waits for this: a Talk event about the actor the point is on, which carries the speaker's tag.
	// A topic's own event comes after it, so a mission that event starts doesn't take this talk as its first objective.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Talked(Owner));
		if (Topics.IsValidIndex(TopicIndex) && !Topics[TopicIndex].Event.IsNone())
		{
			Runner->NotifyEvent(FMissionEvent::Named(Topics[TopicIndex].Event, Owner));
		}
	}
	OnTalked.Broadcast(*this, Listener);
	return true;
}
