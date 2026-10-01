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
 * once finished or skipped it stays quiet in later games (the player's progress save remembers). Looter.Tutorial
 * restart|skip for testing.
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

	/** From the first step again, even if it was finished before. */
	void Restart();

	/** Ends it now and remembers it as done. */
	void Skip();

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
};
