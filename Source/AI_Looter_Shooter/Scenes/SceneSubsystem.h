#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneTimeline.h"
#include "Subsystems/WorldSubsystem.h"
#include "SceneSubsystem.generated.h"

class AActor;
class ACameraActor;
class APawn;
class APlayerController;
class UEnhancedInputComponent;
class UInputAction;
class USceneComponent;
class USceneSkipPromptWidget;
struct FInputActionValue;

DECLARE_DELEGATE(FOnSceneMoment);

/** A scene to play (USceneSubsystem::Play): its timeline, and what it does with the player. */
struct FScenePlay
{
	/** "SkiffRide": in the log, and the missions hear Scene.<Name> once it has played (a skipped scene has too). */
	FName Name;

	FSceneTimeline Timeline;

	/** The player keeps mouse look (a ride). Otherwise the scene's camera has the view and only the skip keys work. */
	bool bLookOnly = true;

	/**
	 * The player stays held once it ends, because travel follows (the skiff ride): no control comes back in this level. If
	 * no trip comes, ReturnFromScene gives them back.
	 */
	bool bHoldAfterEnd = false;

	/** Runs as it starts, once the player is held: put them aboard, spawn its props. */
	TFunction<void()> OnStart;

	/** Runs once it has ended, played or skipped, after the missions have heard: travel goes here (the ride's OnWhiteout). */
	TFunction<void()> AfterEnd;

	/** For a scene that holds the player for a trip: what to put back when no trip comes (the skiff at its moorings). */
	TFunction<void()> OnReturn;
};

/**
 * The level's scenes (Docs/Areas/RansomsRest.md, Tech needs: Scenes), all in C++ with no logic in Sequencer: the first
 * cast-off's skiff ride now; the cold open, the grave wake-up, Sexton's scene, Abel's ending and the train's two shots
 * later. A scene is a timeline of camera and actor moves plus named moments (FScenePlay), and one plays at a time.
 *
 * While one plays it holds the player (SceneSubsystemPlayer.cpp): look-only control or none (no moving, firing, using or
 * menus; the scene's keys sit over every other key and block them), the gameplay HUD put away, no damage, autosaves held,
 * the first-person view. Every scene can be skipped: Looter.Scene.Skip, holding Interact for a second, or Escape twice. A
 * skip jumps to the scene's end state and still fires its moments, so travel always happens. Each moment goes out as
 * OnSceneEvent, and the missions hear Scene.<Name> once a scene has played.
 *
 * Scenes are off in tour and perf runs (AreScenesOn): nothing plays, and callers go straight on (travel at once).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USceneSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static USceneSubsystem* Get(const UObject* WorldContextObject);

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** The first cast-off: the player on Skiff's deck (its Deck socket) with look-only control, the skiff slipping its ropes
	 *  and drifting out for about 12 s, then into a cloud bank. OnSceneEvent broadcasts "CastOff" as the ropes slip.
	 *  OnWhiteout fires once, when the screen is fully white; the white then holds through travel. False (nothing plays)
	 *  if a scene is already playing, Skiff is null, or scenes are off (tour and perf runs): then travel at once. */
	bool PlaySkiffRide(AActor* Skiff, FOnSceneMoment OnWhiteout);
	bool IsPlaying() const;
	void SkipScene();
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSceneEvent, FName /*Event*/);
	FOnSceneEvent OnSceneEvent;

	// --- Any scene (SceneSubsystem.cpp) ---

	/**
	 * Plays Scene: holds the player, runs its OnStart, then its timeline frame by frame. False (nothing plays) if one is
	 * already playing or scenes are off; with scenes off the missions still hear it as played, so none waits forever.
	 */
	bool Play(FScenePlay&& Scene);

	/** The scene playing's name, or None. */
	FName GetPlayingName() const;

	/** Seconds into the scene playing (0 when none is). */
	float GetSceneTime() const;

	/**
	 * Whether scenes play in World. Looter.Scenes 0 turns them off and 2 forces them on; at 1 (the default) they're off in
	 * tour and perf runs: while Looter.Tour runs, or when the command line says so (CommandLineTurnsScenesOff).
	 */
	static bool AreScenesOn(const UWorld* World);

	/** -NoScenes, a tour run (Looter.Tour named on it, as Tools/tour.ps1 does) or a perf capture (-ExitAfterCsvProfiling). */
	static bool CommandLineTurnsScenesOff(const TCHAR* CommandLine);

	// --- The player during a scene (SceneSubsystemPlayer.cpp) ---

	/** A scene holds the player: while it plays, and after one that holds them for a trip until the trip (or a return). */
	bool IsHoldingPlayer() const { return bHoldingPlayer; }

	/** The player is held by a scene, so the gameplay HUD is put away (ALooterHUD asks every frame); captions stay. */
	static bool HidesGameplayHUD(const UObject* WorldContextObject);

	/** The held player's pawn, or null. */
	APawn* GetHeldPawn() const;

	/** Carries the held player on Carrier: their feet at Feet, facing Yaw, moved with it (not walking) until they're let go. */
	void CarryPlayer(USceneComponent* Carrier, const FVector& Feet, float Yaw);

	/** Turns the held player's view by DeltaYaw degrees: what carries them turned. */
	void TurnHeldView(float DeltaYaw);

	/** The scene's own camera, with the player's view on it (blending in over BlendSeconds) until the scene ends. */
	ACameraActor* ViewFromSceneCamera(float BlendSeconds = 0.f);

	/** An actor the scene spawned (its props, a cloud bank): it goes when the scene ends. */
	void AddSceneActor(AActor* Actor);

	/**
	 * A scene ended holding the player for a trip (FScenePlay::bHoldAfterEnd) and no trip comes: puts back what it moved
	 * (the skiff at its moorings), gives the player back where the scene took them from, and reveals the white (with
	 * Title). It happens by itself when the level is still here TravelGraceSeconds after such a scene.
	 */
	void ReturnFromScene(const FText& Title = FText::GetEmpty());

	/** Seconds Interact is held to skip; how long the prompt shows and a first Escape waits for its second. */
	static constexpr float SkipHoldSeconds = 1.f;
	static constexpr float SkipPromptSeconds = 3.f;

	/** The skip keys work this far into a scene, not before: a key still down from the board that started it does nothing. */
	static constexpr float SkipKeysAfter = 0.5f;

	/** How long a scene that holds the player for a trip waits for it before giving them back. */
	static constexpr float TravelGraceSeconds = 5.f;

private:
	/** Moves the scene playing on by DeltaSeconds; a skip asked for from inside one of its moments happens after it. */
	void AdvanceScene(float DeltaSeconds);

	/** The scene playing has ended (played, or skipped to its end): the missions hear, the player comes back or stays held. */
	void FinishScene(bool bSkipped);

	/** One of the scene's moments happened. */
	void HandleMoment(FName Moment);

	/** Destroys the actors the scene spawned. */
	void DestroySceneActors();

	// --- The player (SceneSubsystemPlayer.cpp) ---

	/** Takes hold of the local player for a scene. Without one (a test level) the scene plays on its own. */
	void HoldPlayer(bool bLookOnly);

	/** Gives the held player back: off whatever carried them, at PutBack and looking along View when given, keys and HUD back. */
	void ReleasePlayer(const FTransform* PutBack, const FRotator* View);

	/** The scene's keys, over every other key in the game and blocking them: look (bLookOnly), and the skip keys. */
	void BindSceneInput(APlayerController& Controller, bool bLookOnly);
	void UnbindSceneInput();

	void HandleLook(const FInputActionValue& Value);
	void HandleSkipKeyDown();
	void HandleSkipKeyUp();
	void HandleEscape();

	/** The skip keys over time: the held Interact's second, the first Escape's wait, and the prompt that shows them. */
	void UpdateSkipKeys(float DeltaSeconds);

	bool CanSkipByKey() const;

	TUniquePtr<FScenePlay> Current;
	bool bAdvancing = false;
	bool bSkipWanted = false;

	// The player held, and what they get back.
	TWeakObjectPtr<APlayerController> HeldController;
	TWeakObjectPtr<APawn> HeldPawn;
	FTransform HeldFrom = FTransform::Identity;
	FRotator HeldFromView = FRotator::ZeroRotator;
	uint8 HeldViewMode = 0;
	bool bHeldCanBeDamaged = true;
	bool bHoldingPlayer = false;
	bool bCarried = false;
	bool bSavesHeld = false;
	bool bSceneCameraView = false;

	/** After a scene that holds the player for a trip: what to put back if none comes, and seconds waited (negative: none). */
	TFunction<void()> PendingReturn;
	float HeldAfterEnd = -1.f;

	// The skip keys.
	TWeakObjectPtr<const UInputAction> InteractAction;
	bool bSkipKeyDown = false;
	bool bPromptForEscape = false;
	float SkipKeyHeld = 0.f;
	float EscapeWait = 0.f;
	float PromptLeft = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> SceneInput;

	UPROPERTY(Transient)
	TObjectPtr<USceneSkipPromptWidget> SkipPrompt;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> SceneCamera;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SceneActors;
};
