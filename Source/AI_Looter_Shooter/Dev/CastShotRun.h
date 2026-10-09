#pragma once

#include "CoreMinimal.h"
#include "Dev/CastShotProbe.h"
#include "Dev/CastShotScene.h"

class AActor;
class ACameraActor;
class APawn;
class APlayerController;
class USkinnedMeshComponent;
class UWorld;

/**
 * A run of Looter.CastShots (CastShotDevCommands.cpp starts it): a state machine the core ticker moves on every frame, on
 * real seconds. For each subject, place and state (CastShotScene) it sets the scene up, lets it run while watching the
 * body frame by frame, stops time, takes the numbers (CastShotRunNumbers.cpp) and the three pictures, and moves on; at the
 * end it puts everything back and saves numbers.csv. Developer builds only (the .cpps are empty in shipping).
 */
class FCastShotRun
{
public:
	FCastShotRun(bool bInQuit, TArray<FString> InOnly, TOptional<FVector> InFlat, TOptional<FVector> InSlope, FString InTag);

	/** One frame of the run (real seconds); false once it's over. */
	bool Tick(float DeltaTime);

	/** Where the pictures and numbers go: Saved/Screenshots/CastShots, or a folder in it named Tag (a "before" run kept apart). */
	static FString Directory(const FString& Tag);

private:
	enum class EPhase : uint8
	{
		Waiting,
		Warmup,
		/** A fresh creature standing before its state starts. */
		Settle,
		/** Its state under way, watched frame by frame. */
		Run,
		/** Time stopped: the pictures. */
		Shoot,
	};

	/** One picture set: a subject, a place, a state. */
	struct FStep
	{
		int32 Subject = 0;
		CastShotScene::EPlace Place = CastShotScene::EPlace::Flat;
		CastShotScene::EState State = CastShotScene::EState::Idle;
	};

	// --- The steps (CastShotRun.cpp) ---
	bool TickWaiting();
	void Prepare(UWorld& World, APlayerController& Controller);
	bool BeginStep(UWorld& World, int32 Index);
	/** Puts back what the last step changed: its creatures gone, its character as the story had it, the player hidden. */
	void EndStep();
	/** The level's own creatures near a spot go, so nothing walks into the pictures. */
	void ClearAround(UWorld& World, const FVector& Spot);
	bool StartRun(UWorld& World, float Seconds);
	bool TickSettle(UWorld& World, APawn& Pawn);
	bool TickRun(UWorld& World, APawn& Pawn);
	bool TickShoot(UWorld& World);
	/** The camera on the step's bodies from angle Which, after a cut. */
	void FrameNow(UWorld& World, int32 Which);
	bool Finish(bool bCompleted);
	static void ShowPlayer(APawn& Pawn, bool bShow);

	// --- The numbers (CastShotRunNumbers.cpp) ---
	/** The first body's feet, trailing parts and pops, frame by frame while it runs. */
	void SampleBody(UWorld& World, float DeltaSeconds);
	/** The pack's bodies in each other, the deepest so far. */
	void SamplePack();
	/** Everything at the moment time stops, into the csv and the step's log line. */
	void MeasureNow(UWorld& World);
	/** The slime's gel foot, a ring at its edge on its body bone, against the ground under it. */
	void MeasureGel(UWorld& World, const USkinnedMeshComponent& Mesh, TArray<FString>& Line);
	/** A number for the csv and the log line, marked "!" past what a player would notice. */
	void Note(TArray<FString>& Line, const FString& Metric, float Value, const FString& Detail);
	/** Each number's mark. */
	static float MarkFor(const FString& Metric);

	bool bQuit = false;
	TArray<FString> Only;
	TOptional<FVector> GivenFlat;
	TOptional<FVector> GivenSlope;
	FString Tag;

	TWeakObjectPtr<UWorld> WorldPtr;
	TWeakObjectPtr<APlayerController> ControllerPtr;
	TWeakObjectPtr<APawn> PawnPtr;
	TWeakObjectPtr<ACameraActor> CameraPtr;
	int32 OldMotionBlur = -1;
	CastShotScene::FSpots Spots;
	TArray<CastShotScene::FSubject> Subjects;
	TArray<FStep> Steps;
	TArray<FString> Rows;

	EPhase Phase = EPhase::Waiting;
	int32 StepIndex = INDEX_NONE;
	/** Seconds in the phase (in Shoot: since the camera moved), how long the state runs, which angle, when its picture was asked for. */
	float Clock = 0.f;
	float RunSeconds = 0.f;
	int32 Angle = 0;
	bool bShotRequested = false;
	float ShotAt = 0.f;
	int32 Pictures = 0;
	int32 Marked = 0;

	// The step under way: its bodies (the first is watched frame by frame), what was spawned for it, the way it heads, and
	// the character to put back.
	TArray<TWeakObjectPtr<AActor>> Actors;
	TArray<TWeakObjectPtr<AActor>> Spawned;
	FVector Ahead = FVector::ForwardVector;
	TWeakObjectPtr<AActor> Restore;
	bool bRestoreHidden = false;
	bool bRestoreTicking = true;
	CastShotProbe::FFootSlide Slide;
	CastShotProbe::FPopWatch Pop;
	float WorstUnder = 0.f;
	FString WorstUnderPart;
	float WorstPack = 0.f;
	FString WorstPackDetail;
	float PackClock = 0.f;
};
