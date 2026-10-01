#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class AController;
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
 * While it runs, the tutorial is also the player's mission (UMissionSubsystem, named MissionTitle): its objective is
 * the current step's text, and its waypoint, which the minimap's compass arrow points to, is where that step happens
 * (the gun rack, the rifle on it, the dummies, the nearest spider). The mission goes when the tutorial ends.
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

	UPROPERTY(EditAnywhere, Category = "Tutorial")
	TArray<FTutorialStep> Steps;

	/** Shown when the last step is done. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString DoneText;

	UPROPERTY(EditAnywhere, Category = "Tutorial", meta = (ClampMin = "1"))
	float DoneSeconds = 8.f;

	/** The tutorial's name as a mission, while it runs. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString MissionTitle;

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

private:
	void StartStep(int32 Index);
	bool IsStepDone(const FTutorialStep& Step) const;
	void Finish(bool bShowDone);
	void BindTargets();
	UTutorialPromptWidget* GetPrompt();

	/** Adds the tutorial's mission if it has none yet, and sets its objective and waypoint to the current step's. */
	void SyncMission();
	/** Removes the tutorial's mission, if it has one. */
	void EndMission();
	/** Where the step happens, for the minimap's arrow; unset when it isn't anywhere in particular. */
	TOptional<FVector> FindWaypoint(const FTutorialStep& Step) const;

	UFUNCTION()
	void HandleDummyDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleCreatureDeath(AController* Killer);

	UPROPERTY(Transient)
	TObjectPtr<UTutorialPromptWidget> Prompt;

	int32 Current = INDEX_NONE;
	/** The current step's text is on screen (the prompt is made once the player controller exists). */
	bool bStepShown = false;
	/** Where the player stood when the step began, once there was a pawn to ask. */
	TOptional<FVector> StepStart;
	int32 Hits = 0;
	int32 Kills = 0;
	/** The tutorial's mission in UMissionSubsystem, or INDEX_NONE while it has none. */
	int32 MissionId = INDEX_NONE;
};
