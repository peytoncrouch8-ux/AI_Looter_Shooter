#pragma once

#include "CoreMinimal.h"
#include "Story/StoryCharacter.h"
#include "Story/WardenCrewBody.h"
#include "Story/WardenCrewLife.h"
#include "Story/WardenCrewPose.h"
#include "WardenCrewMember.generated.h"

class UBodyFigureComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/** Who of the Lamplighter's crew (the Lantern Wardens' airship, Docs/Work/2026-10-31-Wardens). */
UENUM(BlueprintType)
enum class EWardenCrew : uint8
{
	/** Captain Mercy Holloway, at the wheel. */
	Captain,
	/** Brother Tobiah Finch, the lantern-keeper, at the Great Lantern. */
	Keeper,
	/** Junie Calloway, the engineer, at the engine. */
	Engineer,
};

/**
 * Where a crew member's hands are at their station, and what keeps them busy there (actor space, the full-size body's cm:
 * ahead, right, up from their feet; scaled with the body). WardenCrewStations.cpp holds the three.
 */
struct FWardenStation
{
	/** Each wrist's spot (0 left, 1 right) and where its elbow points. */
	FVector Hands[2];
	FVector Elbows[2];
	/** What they watch while working, and how firmly. */
	FVector Focus = FVector(2000.0, 0.0, 165.0);
	float FocusWeight = 0.6f;
	/** Leaning in (degrees) and down (cm) to the work. */
	float Lean = 0.f;
	float Crouch = 0.f;
	/** The work: each hand moves this far and back over WorkSeconds, in bursts of BurstSeconds on, as long off (0: steady). */
	FVector Work[2];
	float WorkSeconds = 4.f;
	float BurstSeconds = 0.f;
	/** Talking, the right hand stays at the station (the captain keeps her wheel); the body turns at most this far to them. */
	bool bKeepsRightHand = false;
	float MaxTalkTurn = 60.f;
};

/**
 * One of the Lamplighter's crew on deck (the user's call, 2026-10-31: "the crew are on deck as models"): Captain Mercy
 * Holloway at the wheel, hands on it, looking ahead; Brother Tobiah Finch at the Great Lantern, hands up to it; Junie
 * Calloway at the engine, leaning in with a wrench. A story character (AStoryCharacter: shown by the story, a speaker
 * point the missions find by tag, Speaker_Holloway / Speaker_Finch / Speaker_Junie).
 *
 * The body: the mannequin playing its own unarmed idle (real breath and weight shifts) under a layer made in code
 * (WardenCrewPose: hands at the station, the work's motion, a lean, a brace, a turn to someone, the head's looks), worn as
 * a hero's body standing in (UBodyFigureComponent, as the hero select's live stand and the player's body wear theirs),
 * tinted toward who they are, until the user's model lands at the crew member's path (WardenCrewStations.cpp's table:
 * /Game/Art/Characters/SK_Holloway, SK_Finch, SK_Junie; rigged to the mannequin's skeleton, worn as the body itself).
 * Chosen over AStoryCharacter's code-only posing because a captured idle breathes and settles as no hand-made sine does,
 * and the hero bodies are the most finished people in the game.
 *
 * Alive: they glance about, turn their head to the player coming near (a few seconds at a time), look the one they talk
 * to in the eye (Finch and Junie let go and turn to them; the captain keeps a hand on the wheel), and brace and sway when
 * the ship lurches (Brace; ALamplighterShip calls it, or BraceAll). FLIGHT places them at their stations on the ship
 * (attached to it, facing the wheel, the lantern, the engine) and may shift the hands with HandsShift to fit the props.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AWardenCrewMember : public AStoryCharacter
{
	GENERATED_BODY()

public:
	AWardenCrewMember();

	/** Who this is: their name, tag, look and station follow (SetCrew changes it in play). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	EWardenCrew Crew = EWardenCrew::Captain;

	/** At their station: hands on the wheel, the lantern, the engine. Off: they stand at ease where they are. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crew")
	bool bAtStation = true;

	/** Moves both hands' station spots (actor space, cm) to fit the ship's props as they are. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew")
	FVector HandsShift = FVector::ZeroVector;

	/** The hidden mannequin: the animated body (its own model is worn on it once it's in the game). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> Mannequin;

	/** A hero's body standing in until their own model lands (gone once it has). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBodyFigureComponent> Figure;

	/** Junie's wrench (a stand-in: a handle and a jaw), in her right hand; hidden on the others. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Wrench;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WrenchJaw;

	/** Becomes Crew (name, tag, look, station, wrench), in play too. */
	UFUNCTION(BlueprintCallable, Category = "Crew")
	void SetCrew(EWardenCrew InCrew);

	UFUNCTION(BlueprintCallable, Category = "Crew")
	void SetAtStation(bool bInAtStation);

	/** The ship lurched: they brace for Seconds (gripping their station, knees bent) and sway as it rings out. */
	UFUNCTION(BlueprintCallable, Category = "Crew")
	void Brace(float Seconds);

	/** Every crew member in Context's world braces (the ship's lurch, the grapples biting, the hard landing). */
	static void BraceAll(const UObject* Context, float Seconds);

	/** The speaker name, actor tag, look and station of each. */
	static FText NameFor(EWardenCrew InCrew);
	static FName SpeakerTagFor(EWardenCrew InCrew);
	static FWardenLook LookFor(EWardenCrew InCrew);
	static const FWardenStation& StationFor(EWardenCrew InCrew);

	/** Their own model is worn (not a stand-in). */
	bool HasOwnModel() const { return bOwnModel; }
	const FWardenLife& GetLife() const { return Life; }
	const FWardenPoseInput& GetPoseInput() const { return PoseInput; }
	/** How far into a talk's turn (0-1, eased): hands let go, the body turned to them. */
	float GetTalkAlpha() const { return TalkAlpha; }
	EWardenDetail GetDetail() const { return Detail; }

	virtual void UpdatePose(float DeltaSeconds) override;
	virtual void RefreshShown() override;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	/** Name, tag, hull, speaker height and wrench for Crew. */
	void ApplyCrew();
	/** This frame's pose from the station, the talk, the brace and the eyes. */
	void BuildPose(float DeltaSeconds);
	void HandleTalkedTo(class USpeakerPointComponent& Point, AActor* InListener);
	/** The body's size (its own model's or the stand-in's). */
	float BodyScale() const;

	FWardenLife Life;
	FWardenPoseInput PoseInput;
	TWeakObjectPtr<AActor> Listener;
	float TalkAlpha = 0.f;
	/** The chest's turn toward the player watched (degrees, eased). */
	float ChestTurn = 0.f;
	float WorkClock = 0.f;
	float DetailClock = 0.f;
	EWardenDetail Detail = EWardenDetail::Full;
	bool bOwnModel = false;
};
