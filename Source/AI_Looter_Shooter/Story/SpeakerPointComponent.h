#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "SpeakerPointComponent.generated.h"

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

	/** Someone talked here (its lines have begun and the missions have heard). */
	FOnSpeakerTalked OnTalked;

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

	/** The captions' conversation of the last talk here. */
	int32 Conversation = 0;

	/** Mutters said this visit (topic * 1000 + line), when the next may come, and the last line said. */
	TSet<int32> SaidMutters;
	double NextMutter = 0.0;
	FText LastMutter;
};
