#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Scenes/ColdOpenCourse.h"
#include "Story/StoryCondition.h"
#include "GangSkiffFlyBy.generated.h"

class UAudioComponent;
class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;
class UStoryLineSet;

/**
 * Lucky Ned leaving the Rest (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md, Main 3 step 0: "the Dunne Gang's dark skiff casts
 * off from the barge and flies south over the Hogback", the user's call: Ned isn't fought on the Rest, he's seen
 * leaving): the gang's black skiff, seen from the whole farm road, slipping off Vane's barge and climbing away south over
 * the Hogback and out of the valley, about Seconds long, with Vane and Ned on the radio (RadioLines, UVaneBroadcast) and
 * Hob's and Sully's words after (AfterLines, queued once the radio's done): the player sees the man who put the hose to
 * Abel get away, and leaves the Rest wanting him.
 *
 * It flies once a campaign: the first time PlayWhen holds (Main 3's first step), into a quiet moment (Delia's words said
 * first), and once it's flown its story flag (DoneFlag) keeps it from flying again on a later load. The course is the cold
 * open's skiff flight (FColdOpenCourse: a smooth curve through Course, casting off slow and picking up, its bow along the
 * curve, leaning into its turns), so it moves as the gang's skiff always has. Hidden and still when it isn't flying; it
 * ticks only while it flies. Its hull is the built gang skiff (SM_Skiff_A_Gang: no Tripo model needed).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AGangSkiffFlyBy : public AActor
{
	GENERATED_BODY()

public:
	AGangSkiffFlyBy();

	/** The gang skiff's model (one path). */
	static const TCHAR* const HullPath;

	/** Casts off now, whatever its story (its own look calls it once due; the console and tests too). False without a course. */
	bool Begin();

	bool IsFlying() const { return bFlying; }

	/** It has flown (this visit, or before: its flag was set). */
	bool HasFlown() const { return bDone; }

	/** Moves its flight on by DeltaSeconds (the tick's work while it flies; tests call it). */
	void Advance(float DeltaSeconds);

	/** Looks whether it's due (the timer's work, twice a second; tests call it). */
	void CheckDue();

	/** Its course as it flies it (built from Course as play begins). */
	const FColdOpenCourse& GetCourse() const { return Path; }

	/** The world points it flies through: off the barge, south over the town, over the Hogback, out of the valley. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by")
	TArray<FVector> Course;

	/** How long the flight takes (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by", meta = (ClampMin = "5", Units = "s"))
	float Seconds = 22.f;

	/** When it flies (once a campaign: DoneFlag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by")
	FStoryCondition PlayWhen;

	/** The story flag it sets once it has flown ("Main3.NedLeft"). None: it flies again each time it's due. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by")
	FName DoneFlag;

	/** Said over the radio this far in (Vane and Ned), and the words after once the radio is done (Hob, Sully). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by")
	TObjectPtr<UStoryLineSet> RadioLines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by", meta = (ClampMin = "0", Units = "s"))
	float RadioAt = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by")
	TObjectPtr<UStoryLineSet> AfterLines;

	/** Due, it waits this long (s) at most for the captions to fall quiet before casting off anyway. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fly-by", meta = (ClampMin = "0", Units = "s"))
	float QuietWaitSeconds = 8.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hull;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Out of the valley: hidden, the flag set, the engine's drone gone. */
	void Land();

	FColdOpenCourse Path;
	FTimerHandle DueTimer;
	TWeakObjectPtr<UAudioComponent> Engine;
	float Clock = 0.f;
	double DueSince = -1.0;
	bool bFlying = false;
	bool bDone = false;
	bool bRadioSaid = false;
	bool bAfterSaid = false;
};
