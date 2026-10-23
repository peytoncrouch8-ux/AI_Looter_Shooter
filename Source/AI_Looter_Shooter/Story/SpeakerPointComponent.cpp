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
	ClearPendingOffer();
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
	return bTalkable && bEnabled && Owner && !Owner->IsHidden() && !IsTalking() && (!bMissionsOnly || HasMissionBusiness());
}

bool USpeakerPointComponent::HasMissionBusiness() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const AActor* Owner = GetOwner();
	if (!Runner || !Owner)
	{
		return false;
	}
	if (Runner->FindTurnInAt(Owner) || !Runner->GetOffersAt(Owner).IsEmpty())
	{
		return true;
	}
	int32 Topic = INDEX_NONE;
	GetLinesNow(&Topic);
	return Topic != INDEX_NONE;
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
	// (which may be the step's words again). Its topic's event isn't sent then: that topic wasn't said. With nothing to
	// turn in, a mission offered here with words of its own: those, the same way.
	UMissionRunner* Runner = UMissionRunner::Get(this);
	const UMissionDefinition* TurnedIn = Runner ? Runner->FindTurnInAt(Owner) : nullptr;
	// A turn-in with a choice of two: its words, then the choice card; the talk itself finishes nothing.
	const bool bChoice = TurnedIn && TurnedIn->HasTurnInChoice();
	const TArray<const UMissionDefinition*> OffersNow = !TurnedIn && Runner ? Runner->GetOffersAt(Owner) : TArray<const UMissionDefinition*>();
	const UMissionDefinition* Offered = OffersNow.IsEmpty() ? nullptr : OffersNow[0];
	if (TurnedIn && !TurnedIn->TurnIn.Lines.IsEmpty())
	{
		Said = InOwnName(TurnedIn->TurnIn.Lines);
		TopicIndex = INDEX_NONE;
	}
	else if (Offered && !Offered->Offer.Lines.IsEmpty())
	{
		Said = InOwnName(Offered->Offer.Lines);
		TopicIndex = INDEX_NONE;
	}
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (Captions)
	{
		// Whoever the player turns to talk to has the floor: whatever was being said stops.
		Conversation = Captions->Play(Said, ECaptionPlay::Interrupt);
	}
	// Someone with nothing waiting to be turned in offers the job: the card opens at once, these words said beside it (the
	// user's call, 2026-10-22: Accept never waits for them). A main mission met first gets no card: this talk starts it
	// (below, after the Talk event). A turn-in with a choice asks it once these words are said.
	ClearPendingOffer();
	const bool bMeets = Offered && Offered->IsMetFirst();
	if (Offered && !bMeets)
	{
		QueueOffer(*Offered, Listener, Conversation, /*bWaitForWords*/ false);
	}
	else if (bChoice)
	{
		QueueChoice(*TurnedIn, Listener, Conversation);
	}
	if (Said.IsEmpty() && !Offered && !bChoice)
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
		// Last, so the words already playing are this topic's: a talk given once moves on to the next topic from here.
		if (Topics.IsValidIndex(TopicIndex) && !Topics[TopicIndex].SetsFlag.IsNone())
		{
			Runner->SetFlag(Topics[TopicIndex].SetsFlag);
		}
		// The main mission this talk was for: taken now, after the Talk event, so its first objective doesn't take this talk.
		if (bMeets)
		{
			StartMet(*Offered);
		}
	}
	// The turn-in came first; what they have next (a mission it opened, or one waiting behind it) comes after its words: its
	// own words queued behind them, and the card once the turn-in's are said (beside the job's). A choice still to make comes
	// before anything else they have. (A main mission the turn-in unlocked that starts by itself is under way already, its
	// words queued by the runner.)
	if (TurnedIn && !bChoice && Runner)
	{
		const TArray<const UMissionDefinition*> OffersAfter = Runner->GetOffersAt(Owner);
		if (!OffersAfter.IsEmpty())
		{
			const UMissionDefinition& Next = *OffersAfter[0];
			int32 NextWords = 0;
			if (Captions && !Next.Offer.Lines.IsEmpty())
			{
				// Queued after the turn-in's words, as one conversation: no second talk until both are said.
				NextWords = Captions->Play(InOwnName(Next.Offer.Lines), ECaptionPlay::Queue);
				Conversation = NextWords != 0 ? NextWords : Conversation;
			}
			if (Next.IsMetFirst())
			{
				StartMet(Next);
			}
			else
			{
				// The card comes up with the job's own words (or the silence after everything before them), so its Accept
				// never cuts the turn-in short.
				QueueOffer(Next, Listener, NextWords, /*bWaitForWords*/ true);
			}
		}
	}
	OnTalked.Broadcast(*this, Listener);
	return true;
}

TArray<FStoryLine> USpeakerPointComponent::InOwnName(TArray<FStoryLine> Said) const
{
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
