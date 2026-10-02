#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class UMissionDefinition;
class UMissionRunner;
class UTutorialPromptWidget;

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
	FTutorialStep(const TCHAR* InText, ETutorialGoal InGoal, float InAmount) : Text(InText), Goal(InGoal), Amount(InAmount) {}

	/**
	 * The instruction. {Name} stands for the key bound to that action in the settings (Sprint, Jump, Interact,
	 * Reload, Inventory, ...); {Move} for the four movement keys.
	 */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString Text;

	UPROPERTY(EditAnywhere, Category = "Tutorial")
	ETutorialGoal Goal = ETutorialGoal::Move;

	/** Distance in cm, or a count, depending on the goal. */
	UPROPERTY(EditAnywhere, Category = "Tutorial", meta = (ClampMin = "0"))
	float Amount = 1.f;
};

/**
 * Walks a new player through the tutorial island: one instruction at a time at the top of the screen, each finished
 * by doing it (move, reach the village, take the rifle from the rack, shoot the dummies, hunt spiders, open the
 * loadout). Steps already done are passed at once. Placed once in the level (Tools/Unreal/build_tutorial_island.py);
 * once finished or skipped it stays quiet in later games (the session's progress remembers), and a saved session goes
 * on from its step. Behind the main menu it waits. Looter.Tutorial restart|skip for testing.
 *
 * The tutorial is a mission as data: DA_Mission_Tutorial (UMissionDefinition, id MissionId) holds its steps as
 * objectives, and the level's mission runner (UMissionRunner) plays it like any mission: it checks the objectives,
 * moves from step to step, and shows the tutorial as the tracked mission (UMissionSubsystem, the minimap's arrow) with a
 * waypoint per step (the gun rack, the rifle on it, the dummies, the nearest spider). The director starts it, shows
 * each step's instruction as its prompt, keeps the progress's tutorial-done flag and the saved step, and finishes with
 * its closing line. Without the asset, Steps become the same mission (MakeBuiltInMission): they're the tutorial's
 * built-in copy, and the asset is made from them (Tools/Unreal/create_mission_assets.py).
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

	/** Shown when the last step is done. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString DoneText;

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
	UTutorialPromptWidget* GetPrompt();

	UMissionRunner* GetRunner() const;

	/** The tutorial's mission as the runner knows it: the asset, or else the built-in steps, made into one once. */
	const UMissionDefinition* GetMission();

	/** The instruction of step Index: its first objective's words. */
	FString GetStepText(const UMissionDefinition& Mission, int32 Index) const;

	/** Step Index asks for the inventory, so the prompt stays up while it's open. */
	bool IsInventoryStep(const UMissionDefinition& Mission, int32 Index) const;

	void HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded);

	UPROPERTY(Transient)
	TObjectPtr<UTutorialPromptWidget> Prompt;

	int32 Current = INDEX_NONE;
	/** The current step's text is on screen (the prompt is made once the player controller exists). */
	bool bStepShown = false;
	FDelegateHandle FinishedHandle;
};
