#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DoorHandoff.generated.h"

class AWeaponBase;
class UMissionRunner;
struct FMissionEvent;

/** Where a hand-off through a door stands. */
UENUM(BlueprintType)
enum class EDoorHandoffState : uint8
{
	/** Its step isn't done yet. */
	Waiting,
	/** Words are being said: the door opens a crack, and the gun comes out at GunOutAfter. */
	Handing,
	/** The gun is held out through the crack, waiting to be taken. */
	Holding,
	/** Taken: the door eases shut. */
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
 * level, so the mission's own end never drops another. It's handed out once, as the step asking for the talk is done
 * (Mission's Step: Main 7's first, Delia's door), in this level: from that moment the gun is in the world, unseen at the
 * crack, so a save then keeps it (as loot, lying on the porch when the session comes back). While she speaks the door
 * opens and the gun comes out, held there as loot: a tap of Interact takes it as any gun, and the door then eases shut. A
 * level that begins with the story past that step hands nothing out: the gun is the session's already. It ticks only
 * while the door moves; it hears the gun taken from the missions' Interact event (UMissionRunner::OnEvent).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ADoorHandoff : public AActor
{
	GENERATED_BODY()

public:
	ADoorHandoff();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Hands the gun out now, as its step's end does: the beat starts, the gun spawned unseen at the crack. False when it's
	 * under way or done already (bForce: again once done, Looter.Story.Handoff force), or there's no gun to hand (logged).
	 */
	bool HandOut(bool bForce = false);

	/** Moves the beat on by DeltaSeconds (the tick calls it while the door moves; tests call it). */
	void Advance(float DeltaSeconds);

	EDoorHandoffState GetState() const { return State; }

	/** The gun handed out (spawned as the beat began), while it's in the world. */
	AWeaponBase* GetGun() const;

	/** The gun is out at the crack, waiting to be taken. */
	bool IsHeldOut() const;

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

	/** The gun as it's held out, in the actor's frame (its +X out of the door): muzzle up, tipped out toward the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	FRotator HeldRotation = FRotator(70.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handoff")
	FVector HeldOffset = FVector::ZeroVector;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The mission's step while it runs here (INDEX_NONE when it doesn't), and whether it's finished. */
	int32 ReadStep(bool& bOutFinished) const;

	void HandleMissionsChanged();
	void HandleMissionEvent(const FMissionEvent& Event);

	/** The named gun at the player's level, held at the crack, unseen. */
	void SpawnGun();

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
	bool bGunShown = false;

	/** The mission's place in the story at the last look: its step (INDEX_NONE: not running), and finished. */
	int32 LastStep = INDEX_NONE;
	bool bLastFinished = false;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
	FDelegateHandle EventHandle;
};
