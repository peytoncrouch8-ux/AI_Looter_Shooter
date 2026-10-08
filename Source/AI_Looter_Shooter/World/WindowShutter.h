#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "WindowShutter.generated.h"

class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;

/** Where a window shutter stands. */
enum class EWindowShutterState : uint8
{
	/** Swung open flat against the wall, looking out for the player a few times a second. */
	Open,
	/** The player came near: it slams after its moment's delay. */
	Noticed,
	/** Swinging shut, then bouncing off the casing. */
	Slamming,
	/** Shut, for this visit (or from the start, once the story says). */
	Shut,
};

/**
 * One of Main Street's window shutters (Docs/Areas/RansomsRest.md, Main 3 "Cold Welcome": "The living shutter their
 * windows"; Zones: "Shutters slam as Ellis passes"): a plank shutter, SM_Shutter_Left or _Right
 * (Art/Models/Buildings/FalseFronts.py), hung on a false front's SOCKET_Shutter_<L|R><n> with its hinge at the actor's
 * origin, closed as the socket hangs it. Tools/Unreal/build_area_story.py places one actor per socket (Pruitt's store has
 * eight). It hangs open, swung out through 178 degrees flat against the wall, until the player first comes within
 * SlamRadius; then, after a moment of its own (so a row of them slams one after another), it slams: a swing that speeds
 * up all the way, a bang, a little bounce off the casing. It stays shut for the visit. Once ShutWhen holds (after Main 3)
 * it's shut as the level begins, and one still open when the story gets there slams.
 *
 * No collision, as the model has none: nobody snags on one. It never ticks while still: a timer looks for the player four
 * times a second while it's open, and the tick runs only for the slam.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AWindowShutter : public AActor
{
	GENERATED_BODY()

public:
	AWindowShutter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** The hinge: the actor's spot, on the socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Hinge;

	/** The shutter (SM_Shutter_Left or _Right), its pivot on the hinge; it turns about the hinge's up axis. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Leaf;

	/** It's shut as the level begins once this holds (after Main 3). Empty: never; it hangs open whenever a level begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shutter")
	FStoryCondition ShutWhen;

	/** How near the player comes before it slams (cm, from its hinge). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shutter", meta = (ClampMin = "0", Units = "cm"))
	float SlamRadius = 1800.f;

	/**
	 * How far it hangs open (degrees from shut about the hinge's up axis, its free edge out toward the street). 0: 178,
	 * whichever way its leaf reaches (GetOpenSwing).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shutter")
	float OpenYaw = 0.f;

	/** Up to this long (s) between noticing the player and slamming, rolled for each shutter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shutter", meta = (ClampMin = "0", Units = "s"))
	float MaxDelay = 0.5f;

	/** How long the swing shut takes (s), speeding up all the way to the casing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shutter", meta = (ClampMin = "0.05", Units = "s"))
	float SlamSeconds = 0.24f;

	EWindowShutterState GetState() const { return State; }
	bool IsShut() const { return State == EWindowShutterState::Shut; }

	/** How far it's swung open now (degrees from shut, signed as GetOpenSwing). */
	float GetSwing() const { return Swing; }

	/**
	 * The swing it hangs open at: OpenYaw, or 178 degrees the other way round from where its leaf reaches. A left shutter
	 * (hung at a window's left, reaching right over it, toward the hinge's -Y as it faces out) swings open positive.
	 */
	float GetOpenSwing() const;

	/** The player stands at PlayerLocation: open, it slams (after its delay) when they're within SlamRadius. True when it noticed them. */
	bool NoticePlayer(const FVector& PlayerLocation);

	/** Slams after InDelay seconds from wherever it hangs (the story, the console); nothing when it's shut or slamming. */
	void Slam(float InDelay = 0.f);

	/** Shut at once, with no swing (the story is past it as the level begins). */
	void ShutNow();

	/** Back open flat against the wall, looking out for the player again (the console, tests). */
	void OpenNow();

	/** Moves the slam on by DeltaSeconds (the tick does; a test level never ticks, so tests call it). */
	void AdvanceSlam(float DeltaSeconds);

	/** Reads ShutWhen again: one still open slams once it holds (the missions' changes call it; so can tests). */
	void RefreshStory();

	/** Seconds between its looks for the player while it's open. */
	static constexpr float LookSeconds = 0.25f;

	/** How far open it hangs by default (degrees): flat against the wall, a little off it. */
	static constexpr float OpenDegrees = 178.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void LookForPlayer();
	void ApplySwing();
	void SetLooking(bool bLooking);
	bool IsStoryShut() const;
	void HandleMissionsChanged();

	EWindowShutterState State = EWindowShutterState::Open;
	float Swing = 0.f;
	/** Seconds into the slam, and the delay still to wait before it. */
	float Clock = 0.f;
	float Delay = 0.f;
	/** Where the slam started from (degrees). */
	float SwingFrom = 0.f;
	bool bBanged = false;

	FTimerHandle LookTimer;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
