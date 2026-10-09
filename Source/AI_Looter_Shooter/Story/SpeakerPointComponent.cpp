#include "Story/SpeakerPointComponent.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLineSet.h"
#include "World/TownLifeSubsystem.h"
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
	// Somebody who mutters as the player passes: the town's life listens for the player near it.
	if (HasMutters())
	{
		if (UTownLifeSubsystem* TownLife = UTownLifeSubsystem::Get(this))
		{
			TownLife->AddMutterPoint(this);
		}
	}
	// Townsfolk who only mutter can't be talked to: no prompt, nothing for the Interact key.
	if (!bTalkable)
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
	if (UInteractionSubsystem* Registry = bTalkable ? UInteractionSubsystem::Get(this) : nullptr)
	{
		Registry->Unregister(GetOwner());
	}
	if (UTownLifeSubsystem* TownLife = UTownLifeSubsystem::Get(this))
	{
		TownLife->RemoveMutterPoint(this);
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
	// Someone a mission waits to be turned in to says so on the key, as Borderlands' givers do.
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bTurnIn = Runner && Runner->FindTurnInAt(GetOwner());
	Options.TapPrompt = bTurnIn ? NSLOCTEXT("LooterSpeaker", "TurnInPrompt", "Turn in") : Prompt;
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
	return bTalkable && bEnabled && Owner && !Owner->IsHidden() && !IsTalking();
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
	TArray<FStoryLine> Said = GetLinesNow(&TopicIndex);
	// A mission turned in here with words of its own for it: those, instead of what's said here at this point in the story
	// (which may be the step's words again). Its topic's event isn't sent then: that topic wasn't said.
	UMissionRunner* Runner = UMissionRunner::Get(this);
	const UMissionDefinition* TurnedIn = Runner ? Runner->FindTurnInAt(Owner) : nullptr;
	if (TurnedIn && !TurnedIn->TurnIn.Lines.IsEmpty())
	{
		Said = TurnedIn->TurnIn.Lines;
		TopicIndex = INDEX_NONE;
		for (FStoryLine& Line : Said)
		{
			if (Line.Speaker.IsEmpty())
			{
				Line.Speaker = SpeakerName;
			}
		}
	}
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

	// The talk objective waits for this: a Talk event about the actor the point is on, which carries the speaker's tag;
	// so does a mission ready to turn in to them, which it finishes. A topic's own event comes after it, so a mission that
	// event starts doesn't take this talk as its first objective.
	if (Runner)
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

// ---------------------------------------------------------------------------
// Mutters
// ---------------------------------------------------------------------------

TArray<FStoryLine> USpeakerPointComponent::GetMutterLinesNow(int32* OutTopic) const
{
	int32 Chosen = INDEX_NONE;
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	// Without the missions (a test level), only what's always said applies.
	FCampaignRecord Empty;
	const FCampaignRecord& Campaign = Runner ? Runner->GetCampaign() : Empty;
	for (int32 Index = 0; Index < Mutters.Num(); ++Index)
	{
		if (Mutters[Index].When.IsMet(Campaign, Runner))
		{
			Chosen = Index;
			break;
		}
	}
	if (OutTopic)
	{
		*OutTopic = Chosen;
	}
	TArray<FStoryLine> Said;
	if (Mutters.IsValidIndex(Chosen))
	{
		const FSpeakerTopic& Topic = Mutters[Chosen];
		Said = Topic.LineSet ? Topic.LineSet->Lines : Topic.Lines;
	}
	for (FStoryLine& Line : Said)
	{
		if (Line.Speaker.IsEmpty())
		{
			Line.Speaker = SpeakerName;
		}
	}
	return Said;
}

int32 USpeakerPointComponent::NextMutterLine(const TArray<FStoryLine>& MutterLines, int32 Topic) const
{
	for (int32 Index = 0; Index < MutterLines.Num(); ++Index)
	{
		if (!MutterLines[Index].Text.IsEmpty() && !SaidMutters.Contains(Topic * 1000 + Index))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool USpeakerPointComponent::CanMutter(double Now) const
{
	const AActor* Owner = GetOwner();
	if (!HasMutters() || !Owner || Owner->IsHidden() || Now < NextMutter)
	{
		return false;
	}
	int32 Topic = INDEX_NONE;
	const TArray<FStoryLine> MutterLines = GetMutterLinesNow(&Topic);
	return NextMutterLine(MutterLines, Topic) != INDEX_NONE;
}

bool USpeakerPointComponent::Mutter(double Now)
{
	if (!CanMutter(Now))
	{
		return false;
	}
	// A remark under the breath never talks over anyone: only into silence.
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (!Captions || !Captions->GetQueue().IsEmpty())
	{
		return false;
	}
	int32 Topic = INDEX_NONE;
	const TArray<FStoryLine> MutterLines = GetMutterLinesNow(&Topic);
	const int32 Index = NextMutterLine(MutterLines, Topic);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	Captions->Play(TArray<FStoryLine>{ MutterLines[Index] }, ECaptionPlay::Queue);
	SaidMutters.Add(Topic * 1000 + Index);
	NextMutter = Now + MutterRest;
	LastMutter = MutterLines[Index].Text;
	UE_LOG(LogLooter, Log, TEXT("%s: %s mutters (%s)."), *GetOwner()->GetActorNameOrLabel(), *MutterLines[Index].Speaker.ToString(),
		*LastMutter.ToString());
	return true;
}
