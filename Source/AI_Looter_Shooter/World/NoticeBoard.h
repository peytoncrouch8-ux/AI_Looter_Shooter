#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "NoticeBoard.generated.h"

class UBoxComponent;
class UMissionDefinition;
class UMissionRunner;
class USceneComponent;

/** Where one of a board's postings stands for this session. */
enum class ENoticePosting : uint8
{
	/** Not up yet: what it waits for isn't done (Skyreach's, until the player has a gun and has read the board). */
	Locked,
	/** Up and being played. */
	Open,
	/** Its objectives done: it waits to be turned in. */
	Ready,
	/** Turned in, or done by itself. */
	Done,
};

/** One posting as the board's screen shows it. */
struct AI_LOOTER_SHOOTER_API FNoticeBoardPosting
{
	FName MissionId;
	/** The mission, held by the level's mission runner for the level's life. */
	const UMissionDefinition* Mission = nullptr;
	ENoticePosting State = ENoticePosting::Locked;
	/** It's turned in here (its giver is this board), rather than finishing by itself. */
	bool bTurnedInHere = false;
	/** The tracked mission: the minimap and the HUD's tracker follow it. */
	bool bTracked = false;
	/** Its current objective's count ("3 / 6") while it's open and counts several; empty otherwise. */
	FString Progress;
};

/**
 * A town's notice board as a mission board (Docs/Polish/TutorialRework.md): Skyreach's, in Crossroads Town's square, at
 * the dressing's NoticeBoard (Tools/Unreal/build_area_board.py places it there; the board's model is the dressing's, so
 * this actor is only the board's use). A tap of Interact reads it: the missions hear NoticeBoard.Read (ReadEvent: the
 * tutorial's "Read the notice board" step, and the read that puts the postings up), then its screen opens
 * (ALooterHUD::OpenNoticeBoard, UNoticeBoardWidget) listing its Postings as they stand, where the player tracks one or
 * turns a finished one in.
 *
 * It's the postings' giver: it carries GiverTag, the tag their FMissionTurnIn names, so the HUD's tracker says "Turn in to
 * the notice board" with its arrow here, the prompt says "Turn in" while one waits, and a Talk event about it (the
 * console's Looter.Mission.Event Talk Speaker_NoticeBoard) turns them in as a speaker point's talk would. It has no
 * speaker point of its own: a board doesn't talk, it turns in through its screen (TurnIn). Rewards (Skyreach's are guns)
 * land at the player's feet, in front of the board.
 *
 * Its Face is a thin box over the board's face that only the Visibility channel sees, so the Interact key's line finds
 * it there; nothing else (shots, walking, the camera) notices it. It never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ANoticeBoard : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag missions find boards by (the tutorial's arrow on it). */
	static const FName BoardTag;

	/** The giver tag its postings are turned in to (FMissionTurnIn::SpeakerTag). */
	static const FName GiverTag;

	/** The mission event a read sends. */
	static const FName ReadEvent;

	ANoticeBoard();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	/**
	 * Reads the board for Player (their pawn or controller), as a tap of Interact does: the missions hear it (Read), then
	 * its screen opens. False without a local player's HUD to show it, or while the pause menu is up.
	 */
	UFUNCTION(BlueprintCallable, Category = "Notice Board")
	bool Use(AActor* Player);

	/**
	 * The missions hear the board read (ReadEvent). The first read puts the postings up; the main one is then what the
	 * player does next, so the minimap guides to it.
	 */
	void Read(UMissionRunner& Runner);

	/**
	 * Its postings as they stand, in Postings' order: each that exists, open, ready, done, or locked; one started from code
	 * (the skiff, which the jetty offers) only once it's up.
	 */
	TArray<FNoticeBoardPosting> GatherPostings(const UMissionRunner& Runner) const;

	/** Turns in a posting given here that's ready: finished, with its rewards at the player's feet. False otherwise. */
	bool TurnIn(UMissionRunner& Runner, FName MissionId) const;

	/** Has the minimap and the tracker follow a running posting. False when it isn't running. */
	bool Track(UMissionRunner& Runner, FName MissionId) const;

	/** Mission is turned in here: it names a giver tag this board carries. */
	bool Gives(const UMissionDefinition& Mission) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Over the board's face: what the Interact key's line meets. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Face;

	/** The missions it posts, by id, in the order its screen lists them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board")
	TArray<FName> Postings;

	/** The one that matters (Skyreach's skiff waits on it): tracked as the postings go up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board")
	FName MainPosting;

	/** Its screen's title, and the line under it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board")
	FText Subtitle;

	/** What its screen says while its postings are still locked (Skyreach's: get a gun first). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board", meta = (MultiLine = true))
	FText LockedNote;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board")
	FText Prompt;

	/** How far from the player's eyes it can be read (cm): a notice is read from a step or two back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 350.f;

	/**
	 * The middle of the board's face in the actor's space (+X the way the face looks, Z up from the board's feet), a hand's
	 * breadth in front of it: where the player looks to read it. The defaults fit SM_NoticeBoard (Boardwalk.py's face: 0.88
	 * to 1.84 m up, 6 cm in front of its posts).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board")
	FVector FaceOffset = FVector(14.0, 0.0, 136.0);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
