#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class ALooterHUD;
class UMissionDefinition;
class UMissionRunner;

/** What finishes a tutorial step. */
UENUM()
enum class ETutorialGoal : uint8
{
	/** Walk Amount cm from where the step began. */
	Move,
	/** Come within Amount cm of the level's weapon rack. */
	ReachRack,
	/** Carry a weapon. */
	HoldWeapon,
	/** Land Amount hits on target dummies. */
	HitDummies,
	/** Kill Amount creatures. */
	KillCreatures,
	/** Open the inventory. */
	OpenInventory,
};

USTRUCT()
struct FTutorialStep
{
	GENERATED_BODY()

	FTutorialStep() = default;
	FTutorialStep(const TCHAR* InText, ETutorialGoal InGoal, float InAmount, const TCHAR* InShortText, FName InHintAction = NAME_None,
		const TCHAR* InHintText = TEXT(""))
		: Text(InText), ShortText(InShortText), HintAction(InHintAction), HintText(InHintText), Goal(InGoal), Amount(InAmount) {}

	/**
	 * The instruction in full, which the Missions page shows. {Name} stands for the key bound to that action in the
	 * settings (Sprint, Jump, Interact, Reload, Inventory, ...); {Move} for the four movement keys.
	 */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString Text;

	/** The HUD's mission tracker's short line for it ("Shoot the target dummies"); the key moves into the hint. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString ShortText;

	/** The key the tracker's hint teaches, by its binding id (Move, Sprint, Interact, Reload, Inventory); None: no hint. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName HintAction;

	/** What the hint's key does ("Hold to run"). */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString HintText;

	UPROPERTY(EditAnywhere, Category = "Tutorial")
	ETutorialGoal Goal = ETutorialGoal::Move;

	/** Distance in cm, or a count, depending on the goal. */
	UPROPERTY(EditAnywhere, Category = "Tutorial", meta = (ClampMin = "0"))
	float Amount = 1.f;
};

/**
 * Walks a new player through the tutorial island: one objective at a time, each finished by doing it (move, reach the
 * village, take the rifle from the rack, shoot the dummies, hunt spiders, open the loadout). Steps already done are
 * passed at once. Placed once in the level (Tools/Unreal/build_tutorial_island.py); once finished or skipped it stays
 * quiet in later games (the session's progress remembers), and a saved session goes on from its step. Behind the main
 * menu it waits. Looter.Tutorial restart|skip for testing.
 *
 * The tutorial is a mission as data: DA_Mission_Tutorial (UMissionDefinition, id MissionId) holds its steps as
 * objectives, and the level's mission runner (UMissionRunner) plays it like any mission: it checks the objectives,
 * moves from step to step, and shows the tutorial as the tracked mission (UMissionSubsystem: the HUD's mission tracker
 * with each step's short line and key hint, the minimap's arrow) with a waypoint per step (the gun rack, the rifle on
 * it, the dummies, the nearest spider). The director starts it, keeps the progress's tutorial-done flag and the saved
 * step, and finishes with its closing line, which it hands to the tracker through ALooterHUD. Without the asset, Steps
 * become the same mission (MakeBuiltInMission): they're the tutorial's built-in copy, and the asset is made from them
 * (Tools/Unreal/create_mission_assets.py).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ATutorialDirector : public AActor
{
	GENERATED_BODY()

public:
	ATutorialDirector();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** The built-in steps: the mission when DA_Mission_Tutorial is missing, and what the asset is made from. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	TArray<FTutorialStep> Steps;

	/** Shown when the last step is done: the mission tracker's last, ticked line. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString DoneText;

	/** How long the closing line shows, counting only while the game is on screen (not under a menu). */
	UPROPERTY(EditAnywhere, Category = "Tutorial", meta = (ClampMin = "1"))
	float DoneSeconds = 8.f;

	/** The tutorial's name as a mission, while it runs (the built-in mission's title; the asset has its own). */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString MissionTitle;

	/** The id of the tutorial's mission asset (UMissionDefinition::GetMissionId). */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName MissionId = TEXT("Tutorial");

	/** From the first step again, even if it was finished before. */
	void Restart();

	/** Ends it now and remembers it as done. */
	void Skip();

	/** Goes on from step Index (a saved session's), the steps before it done. Nothing when it isn't running. */
	void ResumeAtStep(int32 Index);

	/** The step being shown, or INDEX_NONE when the tutorial isn't running. */
	int32 GetCurrentStep() const { return Current; }

	/** The text with each {Action} replaced by the key the player has bound to it, in brackets. */
	FString ResolveKeys(const FString& Text) const;

	/**
	 * The built-in steps as a mission, what DA_Mission_Tutorial holds: each goal becomes the objective that finishes it,
	 * with the waypoint the tutorial has always shown for it (TutorialDirectorMission.cpp).
	 */
	UMissionDefinition* MakeBuiltInMission(UObject* Outer) const;

private:
	void StartStep(int32 Index);
	void Finish(bool bShowDone);

	UMissionRunner* GetRunner() const;

	/** The local player's HUD, whose mission tracker shows the tutorial; null before there is one. */
	ALooterHUD* GetHUD() const;

	/** The tutorial's mission as the runner knows it: the asset, or else the built-in steps, made into one once. */
	const UMissionDefinition* GetMission();

	void HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded);

	int32 Current = INDEX_NONE;
	FDelegateHandle FinishedHandle;
};
