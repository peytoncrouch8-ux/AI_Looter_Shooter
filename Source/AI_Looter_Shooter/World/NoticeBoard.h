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
	/** Up for the taking: a mission this board offers (FMissionOffer), not taken yet. Choosing it opens the offer card. */
	Offered,
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
	/** It's picked up here (the board offers it, or what offers it is pinned on its face), as a bounty board's jobs are. */
	bool bOfferedHere = false;
	/**
	 * What it's picked up from when that is pinned on this board's face rather than the board itself (Ruth Calder's note on
	 * the Rangers' board, an AMissionArtifact): taking it here opens the offer card from that note. Unset otherwise.
	 */
	TWeakObjectPtr<AActor> PinnedGiver;
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
 * Any town's board is a bounty board too (2026-10-13: every mission is picked up from a giver): the missions offered by a
 * tag it carries (FMissionOffer::GiverTag: NoticeBoard_Rangers) are pinned up with its Postings while they're offered,
 * under way, ready or done, and choosing one up for the taking opens the offer card over the board's screen
 * (ALooterHUD::OpenMissionOffer); a ! floats over the board while one is. A job picked up from something pinned on its face
 * (Ruth Calder's note on the Rangers' board: an AMissionArtifact laid over the paper) is up for the taking here too, its
 * card opened from that note (FindPinnedGiver). Placed on any layout by Tools/Unreal/build_area_board.py
 * (gameplay.boards), with its own tags, title and postings.
 *
 * A turn-in here has its moment as a giver's has: the mission's turn-in words (FMissionTurnIn::Lines: the tags pinned on
 * Calder's tally, Hob's remark) play as captions, waiting under the board's screen until it closes.
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
	 * (the skiff, which the jetty offers) only once it's up. After them the missions offered or turned in here by its tags:
	 * up for the taking first, then ready, open and done; what isn't offered yet stays off the board.
	 */
	TArray<FNoticeBoardPosting> GatherPostings(const UMissionRunner& Runner) const;

	/** Mission is picked up here: its offer names a giver tag this board carries. */
	bool Offers(const UMissionDefinition& Mission) const;

	/**
	 * What Mission is picked up from when that is pinned on this board's face (an interactable carrying its offer's giver
	 * tag, shown, lying on the face: Ruth Calder's note on the Rangers' board), else null. A mission this board offers by its
	 * own tag has none: the board is its giver.
	 */
	AActor* FindPinnedGiver(const UMissionDefinition& Mission) const;

	/**
	 * Point lies on the board's face: within its width and height, and no more than a hand's breadth in front of or behind
	 * its surface (a card laid over a note stands a centimetre or two proud; something on the ground before the board isn't
	 * pinned on it).
	 */
	bool IsOnFace(const FVector& Point) const;

	/**
	 * Turns in a posting given here that's ready: finished, with its rewards at the player's feet, and its turn-in words said
	 * (they wait under the board's screen). False otherwise.
	 */
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

	/** How far from the player's eyes it can be read (cm): a notice is read from a step back, never from across the square. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice Board", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 300.f;

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

private:
	/** The level's interactables pinned on its face (shown, other than itself): looked for once per gathering. */
	TArray<AActor*> GatherPinned() const;

	/** Of Pinned, the one Mission is picked up from (it carries the offer's giver tag), unless the board offers it itself. */
	AActor* PinnedGiverAmong(const TArray<AActor*>& Pinned, const UMissionDefinition& Mission) const;
};
