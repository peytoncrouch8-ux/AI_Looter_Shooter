#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/TimerHandle.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "SpeakerPointComponent.generated.h"

class UMissionDefinition;
class USpeakerPointComponent;
class UStoryLineSet;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSpeakerTalked, USpeakerPointComponent& /*Point*/, AActor* /*Listener*/);

/** What someone says at a point in the story. A talk plays the first topic whose condition holds. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSpeakerTopic
{
	GENERATED_BODY()

	/** When this is what they say. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Topic")
	FStoryCondition When;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Topic")
	TArray<FStoryLine> Lines;

	/** Lines from an asset instead, when set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Topic")
	TObjectPtr<UStoryLineSet> LineSet;

	/**
	 * A mission event sent when this topic is said, after the Talk event: a mission waiting for it starts (its StartEvent),
	 * as Main 6 does at Delia's door. None: only the Talk event.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Topic")
	FName Event;
};

/**
 * Where someone talks to the player: on a door or a window (Grandma Delia's screen door, Tilly's shop window, which never
 * open for it) or on a story character. It holds who speaks there and what they say: its Lines (or a line set), or what
 * suits the point in the story (Topics). A tap of the Interact key ("Talk") plays them as captions, cutting off whatever
 * was being said, and tells the missions: a Talk event about the actor it's on, which the talk objective waits for by
 * the actor's tag (Speaker_Delia), and which turns in a mission ready to turn in to them (its key then says "Turn in",
 * and the mission's own turn-in lines are said, when it has any). While its lines play it can't be talked to again.
 *
 * Someone with a mission to offer (FMissionOffer::GiverTag on the actor) offers it as they talk, Borderlands' way: they say
 * the offer's lines (else their own), and once those are said the offer card opens (ALooterHUD::OpenMissionOffer). A
 * mission waiting to be turned in to them comes first: that talk turns it in, and what they have next is offered after the
 * turn-in's words. Walking off before the words end leaves the offer for the next talk (its ! stays). A turn-in with a
 * choice of two (FMissionTurnIn::Choices) isn't finished by the talk: once its words are said the card opens in its choice
 * mode (ALooterHUD::OpenTurnInChoice), and the mission finishes when the player chooses.
 *
 * Anything can speak: a door, a window, a story character, a talking coffin, a speaking tube, a tag board, a store's hatch.
 * Place the point where the voice comes from (its owner's speaker socket); bMissionsOnly keeps a thing that speaks only for
 * the story quiet the rest of the time.
 *
 * It's an IInteractable, but the interaction component asks actors, not components: the actor it's on hands the Interact
 * key on to it (ASpeakerPoint for doors and windows, AStoryCharacter). It enters that actor among the level's interactables.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API USpeakerPointComponent : public USceneComponent, public IInteractable
{
	GENERATED_BODY()

public:
	USpeakerPointComponent();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	/**
	 * Someone talks here, as a tap of the Interact key does (a script, a scene, a test): the lines for this point in the
	 * story play as captions and the missions hear of it. False when it can't be talked to now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Speaker")
	bool Talk(AActor* Listener);

	/** It can be talked to now: on, its actor shown, and not still saying its last lines. */
	UFUNCTION(BlueprintPure, Category = "Speaker")
	bool CanTalk() const;

	/** Its last lines are on screen or still to come. */
	UFUNCTION(BlueprintPure, Category = "Speaker")
	bool IsTalking() const;

	/**
	 * What a talk would say now, each line naming its speaker: the first topic that applies, else Lines (LineSet's when
	 * set). OutTopic gets the topic's index, or INDEX_NONE for the plain lines.
	 */
	TArray<FStoryLine> GetLinesNow(int32* OutTopic = nullptr) const;

	/** Who speaks here ("Grandma Delia"): the name on every line that doesn't name its own speaker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	FText SpeakerName;

	/** The prompt's words: "Talk". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	FText Prompt;

	/** What they say when no topic applies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	TArray<FStoryLine> Lines;

	/** Lines from an asset instead of Lines, when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	TObjectPtr<UStoryLineSet> LineSet;

	/** What they say at points in the story, the first that applies; none applying: Lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	TArray<FSpeakerTopic> Topics;

	/**
	 * How far from the player's eyes it can be talked to (cm). A conversation carries farther than a hand reaches (the
	 * player's 2.5 m), so someone a few steps away can be talked to; 0: the player's reach.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 400.f;

	/** It can be talked to at all (a mission may close a door for a while). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	bool bEnabled = true;

	/**
	 * Whether the Interact key talks here at all. Off for townsfolk who only mutter behind their doors (ATownLifeDoor): the
	 * point then never joins the level's interactables and shows no prompt.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	bool bTalkable = true;

	/**
	 * It talks only while it has something for the player: a mission to turn in or to offer here, or a topic that applies
	 * now (its plain Lines don't count). For things that speak only for the story (a tag board's voice out of the dark, a
	 * speaking tube, a talking coffin, a store's hatch beside its vendor): no prompt the rest of the time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker")
	bool bMissionsOnly = false;

	/** A mission waits to be turned in here, one is offered here, or a topic applies now: what bMissionsOnly asks for. */
	bool HasMissionBusiness() const;

	/** Someone talked here (its lines have begun and the missions have heard). */
	FOnSpeakerTalked OnTalked;

	// --- Offers: a mission offered once the talk's words are said ---

	/** The player farther than this from the point (cm) once the words are said has walked off: nothing is offered. */
	static constexpr float OfferWalkAway = 900.f;

	/** How long an offer waits for a menu or a scene to get out of the way (s) before it's left for the next talk. */
	static constexpr float OfferPatience = 20.f;

	/** The mission the last talk will offer once its words are said, or None. */
	FName GetPendingOffer() const { return PendingOffer; }

	/** The pending mission is a turn-in waiting for the player's choice (the choice card opens after the words), not an offer. */
	bool IsChoicePending() const { return bPendingChoice && !PendingOffer.IsNone(); }

	/**
	 * Opens the offer card on the pending offer if its words are said, the listener is still near and nothing covers the
	 * game: true when it opened. A timer asks a few times a second while one waits; tests may ask.
	 */
	bool UpdatePendingOffer();

	// --- Mutters: said unasked as the player passes (UTownLifeSubsystem decides when) ---

	/**
	 * What they mutter as the player passes, at points in the story: the first topic that applies, one line at a time in
	 * order, each once a visit (a level's play). None: they never mutter.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker|Mutters")
	TArray<FSpeakerTopic> Mutters;

	/** How near the player passes for a mutter (cm, from the point). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker|Mutters", meta = (ClampMin = "0", Units = "cm"))
	float MutterRadius = 700.f;

	/** After a mutter here, the next waits at least this long (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speaker|Mutters", meta = (ClampMin = "0", Units = "s"))
	float MutterRest = 75.f;

	/** It has mutters at all. */
	bool HasMutters() const { return !Mutters.IsEmpty(); }

	/** The mutters for this point in the story (the first topic that applies), each naming its speaker; OutTopic its index. */
	TArray<FStoryLine> GetMutterLinesNow(int32* OutTopic = nullptr) const;

	/** A mutter could come at world time Now: its actor shown, its rest over, and a line of this topic not said yet. */
	bool CanMutter(double Now) const;

	/**
	 * Mutters its next line now, as a caption (after nothing: never over a line already on screen or waiting). True when
	 * it said one. The town's life calls it as the player passes; so can tests.
	 */
	bool Mutter(double Now);

	/** The line it muttered last, or empty. */
	const FText& GetLastMutter() const { return LastMutter; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The next unsaid line of the topic that applies now (its index in Lines), or INDEX_NONE. */
	int32 NextMutterLine(const TArray<FStoryLine>& MutterLines, int32 Topic) const;

	/** Lines in this point's speaker's name where they name none. */
	TArray<FStoryLine> InOwnName(TArray<FStoryLine> Said) const;

	/** Mission is offered once the conversation's words are said, to Listener. */
	void QueueOffer(const UMissionDefinition& Mission, AActor* Listener, int32 AfterConversation);
	/** Mission's turn-in choice card opens once the conversation's words are said, for Listener (SpeakerPointComponentOffers.cpp). */
	void QueueChoice(const UMissionDefinition& Mission, AActor* Listener, int32 AfterConversation);
	void ClearPendingOffer();
	void TickPendingOffer();

	/**
	 * The offer (or the turn-in choice, bPendingChoice) waiting for the talk's words, the conversation it waits for, who
	 * it's for and since when (world seconds).
	 */
	FName PendingOffer;
	bool bPendingChoice = false;
	int32 OfferConversation = 0;
	TWeakObjectPtr<AActor> OfferListener;
	double OfferQueuedAt = 0.0;
	FTimerHandle OfferTimer;

	/** The captions' conversation of the last talk here. */
	int32 Conversation = 0;

	/** Mutters said this visit (topic * 1000 + line), when the next may come, and the last line said. */
	TSet<int32> SaidMutters;
	double NextMutter = 0.0;
	FText LastMutter;
};
