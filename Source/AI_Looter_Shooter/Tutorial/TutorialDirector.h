#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialDirector.generated.h"

class ALooterHUD;
class UMissionDefinition;
class UMissionRunner;

/** What finishes one of the first goal's steps. */
UENUM()
enum class ETutorialGoal : uint8
{
	/** Carry Amount guns (the rifle on the gun rack in the square). */
	HoldWeapon,
	/** Read the notice board (ANoticeBoard::ReadEvent). */
	ReadBoard,
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

	/** The HUD's mission tracker's short line for it ("Find a gun in town"). */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString ShortText;

	/**
	 * A key the tracker teaches under the line, by its binding id; None: no hint. The first goal has none: the keys are
	 * taught by the contextual hints (UControlHintSubsystem) when they're needed, not on the tracker.
	 */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName HintAction;

	/** What the hint's key does. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString HintText;

	UPROPERTY(EditAnywhere, Category = "Tutorial")
	ETutorialGoal Goal = ETutorialGoal::HoldWeapon;

	/** A count, for the goals that count. */
	UPROPERTY(EditAnywhere, Category = "Tutorial", meta = (ClampMin = "0"))
	float Amount = 1.f;
};

/** What the director does as a level begins, from what the session knows. */
enum class ETutorialStart : uint8
{
	/** A new player: the first goal starts (find a gun in town, then read the notice board). */
	Teach,
	/** The first goal is behind them and the island's main posting isn't: nothing to start, the postings start by themselves. */
	Postings,
	/**
	 * The tutorial is done (Web Hollow turned in, the island skipped from the main menu, or the old tutorial finished): the
	 * first goal is recorded finished, so the board's postings are offered on a practice visit.
	 */
	Done,
};

/**
 * Skyreach's tutorial, reworked (Docs/Polish/TutorialRework.md; the user: "the tutorial feels rushed and unnecessary").
 * No forced checklist: the player starts at the farm free to roam, the contextual control hints teach the keys as they're
 * needed (UControlHintSubsystem), and the director plays one short first goal, "Welcome to Skyreach": find a gun in town
 * (the arrow on the gun rack's rifle), then read the notice board in the square (the arrow on the board). Reading it puts
 * the town's postings up (DA_Mission_WebHollow, RangePractice, Wallow, Lookout: automatic missions waiting on this one,
 * turned in at the board, ANoticeBoard), and the board tracks the main one, Clear Web Hollow.
 *
 * The tutorial counts as done (the progress's flag, which the skiff's jetty waits for: its gangplank comes down and "Board
 * the Skiff" is offered) once Clear Web Hollow is turned in, or when the island is skipped (the main menu's "Skip to
 * Ransom's Rest", Looter.Tutorial skip). Skyreach stays a practice island after: on a later visit the first goal is
 * recorded finished, so the board's postings are there to do.
 *
 * The first goal is a mission as data, DA_Mission_Tutorial (id MissionId), played by the level's mission runner; without
 * the asset, Steps become the same mission (MakeBuiltInMission, TutorialDirectorMission.cpp), and the asset is made from
 * them (Tools/Unreal/create_mission_assets.py). The director starts it and keeps its step for the session's save
 * (GetCurrentStep, ResumeAtStep). Placed once in the level with the PlayerStart (build_area.py); behind the main menu it
 * waits. Looter.Tutorial restart|skip for testing.
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

	/** The first goal's built-in steps: the mission when DA_Mission_Tutorial is missing, and what the asset is made from. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	TArray<FTutorialStep> Steps;

	/** The first goal's name as a mission, while it runs (the built-in mission's title; the asset has its own). */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FString MissionTitle;

	/**
	 * The first goal's last step, "Read the notice board" (from 0): the point the HUD and menu photo scenes (Dev/HudShotScene,
	 * Dev/MenuShotScene) rest the tutorial at, with a gun already in hand and one goal still on the tracker. A test pins it
	 * to the steps, so reworking them again fails there rather than silently emptying the photos.
	 */
	static constexpr int32 BoardStep = 1;

	/** The id of the first goal's mission asset (UMissionDefinition::GetMissionId). */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName MissionId = TEXT("Tutorial");

	/** The island's main posting: turned in, the tutorial is done (ASkiffJetty::TutorialMissionId names it too). */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName MainPostingId = TEXT("WebHollow");

	/** What to do as a level begins: teach, leave it to the postings, or count the tutorial done. */
	static ETutorialStart DecideStart(bool bTutorialDone, bool bFirstGoalDone, bool bMainPostingDone);

	/** The first goal from its first step again, the tutorial no longer done (the postings already up stay up). */
	void Restart();

	/** Ends the first goal now and counts the tutorial done: the skiff is offered. */
	void Skip();

	/** Goes on from step Index (a saved session's), the steps before it done. Nothing when the first goal isn't running. */
	void ResumeAtStep(int32 Index);

	/** The first goal's step being shown, or INDEX_NONE when it isn't running. */
	int32 GetCurrentStep() const { return Current; }

	/** The text with each {Action} replaced by the key the player has bound to it, in brackets. */
	FString ResolveKeys(const FString& Text) const;

	/**
	 * The built-in steps as a mission, what DA_Mission_Tutorial holds: each goal becomes the objective that finishes it,
	 * with its waypoint (the gun rack, the notice board) (TutorialDirectorMission.cpp).
	 */
	UMissionDefinition* MakeBuiltInMission(UObject* Outer) const;

private:
	void StartStep(int32 Index);

	/** The first goal is over: done, or skipped (bComplete finishes its mission too). */
	void FinishFirstGoal(bool bComplete);

	/** The tutorial counts as done from now on (the progress's flag): the jetty offers the skiff. */
	void MarkTutorialDone();

	UMissionRunner* GetRunner() const;

	/** The local player's HUD; null before there is one. */
	ALooterHUD* GetHUD() const;

	/** The first goal's mission as the runner knows it: the asset, or else the built-in steps, made into one once. */
	const UMissionDefinition* GetMission();

	void HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded);

	int32 Current = INDEX_NONE;
	/** The step the level was last looked at for a notice board (a level without one can't hold the first goal up). */
	int32 BoardCheckedStep = INDEX_NONE;
	FDelegateHandle FinishedHandle;
};
