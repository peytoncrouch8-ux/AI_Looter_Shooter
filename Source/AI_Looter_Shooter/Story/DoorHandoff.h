#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorHandoff.generated.h"

class AWeaponBase;
class UMissionRunner;
class UNamedWeaponDefinition;
struct FMissionEvent;

/** Where a hand-off through a door stands. */
UENUM(BlueprintType)
enum class EDoorHandoffState : uint8
{
	/** Its step isn't done yet. */
	Waiting,
	/** Words are being said: the door opens a crack, and the gun comes out at GunOutAfter. Nothing is in the world yet. */
	Handing,
	/** The gun came out through the crack onto the porch, and lies there as loot waiting to be taken. */
	Out,
	/** Taken (or never came out: the session had it already): the door eases shut. */
	Closing,
	/** Handed over, or the story was past it as the level began. */
	Done,
};

/**
 * Something handed out through a door opened a crack (Docs/Areas/RansomsRest.md, Main 7 "The Lantern Leans": Grandma
 * Delia hands Heirloom, Abel's shotgun, out through her screen door: "A keeper's buried with his lantern, not his iron. His
 * lantern wasn't on him, so I kept this back. He'd want you to have it. Hold the door."). It stands where a held thing
 * comes out (the farmhouse's SOCKET_Handoff: 1 m over the floor, just outside the door on its latch side, facing out) and
 * swings Door (her screen door, SM_ScreenDoor on its hinge) open a crack, as Farmhouse.py's Handoff preview shows it.
 *
 * The gun is the mission's: its named gun given by hand (FMissionRewards::NamedGun with bNamedGunByHand), at the player's
 * level. The beat plays once, as the step asking for the talk is done (Mission's Step: Main 7's first, Delia's door), in
 * this level: while she speaks the door opens a crack, and as she says he'd want you to have it the gun comes out through
 * it and drops onto the porch in front of the door, a world drop like any loot (its beam, its rarity's landing:
 * ULootFanfareSubsystem); a tap of Interact takes it as any gun, and the door then eases shut.
 *
 * Nothing is in the world before it comes out. (It used to wait at the crack unseen from her first word, as loot: with
 * her talk under way her door can't be talked to, so the next tap of Interact took the unseen gun, straight into a full
 * player's backpack, and Heirloom never appeared at all.) A session saved before it came out, or a gun never taken from
 * the porch, is made good as Main 7 is turned in (MissionRewards::EnsureNamedGun), and the gun never comes out when the
 * session has it already (the turn-in came first): one Heirloom, ever. A level that begins with the story past that step
 * hands nothing out. It ticks only while the door moves; it hears the gun taken from the missions' Interact event
 * (UMissionRunner::OnEvent).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ADoorHandoff : public AActor
{
	GENERATED_BODY()

public:
	ADoorHandoff();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Hands the gun out now, as its step's end does: the beat starts (the gun comes out at GunOutAfter). False when it's
	 * under way or done already, or there's no gun to hand (logged). bForce (Looter.Story.Handoff force): again once done,
	 * and the gun comes out even when the session has one already.
	 */
	bool HandOut(bool bForce = false);

	/** Moves the beat on by DeltaSeconds (the tick calls it while the door moves; tests call it). */
	void Advance(float DeltaSeconds);

	EDoorHandoffState GetState() const { return State; }

	/** The gun that came out, while it's in the world. */
	AWeaponBase* GetGun() const;

	/** The gun is out, lying on the porch as loot, waiting to be taken. */
	bool IsGunOut() const;

	/** How far the door stands open now (degrees). */
	float GetDoorAngle() const { return DoorAngle; }

	/** The named gun's id it hands out: NamedGun, else Mission's named gun given by hand; None when neither names one. */
	FName FindGunId() const;

	/** The door it opens: its root turns about its own up axis (the hinge line). Null: no door moves, the gun still comes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	TObjectPtr<AActor> Door;

	/** The mission and its step (counted from 0) whose end hands the gun out: Main 7's talk at Delia's door. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	FName Mission = TEXT("Main7");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff", meta = (ClampMin = "0"))
	int32 Step = 0;

	/** A named gun by id to hand out instead of the mission's (empty: the mission's, FMissionRewards::NamedGun). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	FName NamedGun;

	/** How far the door opens (degrees of its yaw; positive swings it out toward the porch, Farmhouse.py's +25). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	float CrackDegrees = 25.f;

	/**
	 * Seconds from the talk: the door starts to open (as she says she kept this back), and the gun comes out (as she says
	 * he'd want you to have it). Her line's own reading times, FStoryLine::GetSeconds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff", meta = (ClampMin = "0"))
	float OpenAfter = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff", meta = (ClampMin = "0"))
	float GunOutAfter = 7.8f;

	/** How long the door takes to swing open or shut (seconds), and how long after the gun is taken it starts to shut. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff", meta = (ClampMin = "0.05"))
	float SwingSeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff", meta = (ClampMin = "0"))
	float CloseAfterTaken = 1.f;

	/** Where the gun comes out, in the actor's frame (its +X out of the door, +Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	FVector OutOffset = FVector::ZeroVector;

	/**
	 * How it comes out, in the actor's frame (cm/s): out of the crack and a little up, so it drops onto the porch boards
	 * about a metre in front of the door, at the feet of whoever she's talking to.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	FVector OutVelocity = FVector(160.f, 0.f, 140.f);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The mission's step while it runs here (INDEX_NONE when it doesn't), and whether it's finished. */
	int32 ReadStep(bool& bOutFinished) const;

	void HandleMissionsChanged();
	void HandleMissionEvent(const FMissionEvent& Event);

	/** The named gun it hands out (FindGunId's asset), or null (logged). */
	UNamedWeaponDefinition* FindGun() const;

	/**
	 * The named gun at the player's level, out through the crack onto the porch as loot. False when it didn't come out:
	 * no gun, or (unless forced) the session has it already.
	 */
	bool BringGunOut();

	/** The door turned Degrees open from shut. */
	void SetDoorAngle(float Degrees);

	void StartClosing();

	EDoorHandoffState State = EDoorHandoffState::Waiting;
	float Clock = 0.f;
	float DoorAngle = 0.f;
	/** Where the door stood when it started to shut. */
	float ClosingFrom = 0.f;

	/** The door's turn when shut, kept as the beat begins. */
	FQuat DoorShut = FQuat::Identity;
	bool bDoorShutKnown = false;

	TWeakObjectPtr<AWeaponBase> Gun;
	/** This beat was forced (the console): its gun comes out whatever the session has. */
	bool bForced = false;

	/** The mission's place in the story at the last look: its step (INDEX_NONE: not running), and finished. */
	int32 LastStep = INDEX_NONE;
	bool bLastFinished = false;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
	FDelegateHandle EventHandle;
};
