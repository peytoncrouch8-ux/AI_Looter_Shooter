#pragma once

#include "CoreMinimal.h"
#include "Story/DeadFriendPoses.h"
#include "Story/StoryCharacter.h"
#include "DeadFriend.generated.h"

class UMaterialInterface;
class UPoseableMeshComponent;
class USkeletalMesh;
class USpeakerPointComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A friendly dead man of the story on the Unpaid's rig (Hiram Coyle at his boiler, Thaddeus Bright at his bench): a story
 * character, never a creature, as Amos Whitlock is. His body is his model once the art drops it in (SK_Hiram, SK_Thaddeus:
 * the Unpaid's skeleton), else SK_Unpaid in a tint of his own, posed by code from his pose table (DeadFriendPoses: no
 * animation runs) on a poseable mesh, with his things on his bones (a hat on the hat bone, a prop in a fist), found by name
 * when they exist. His coal is banked low under ash (custom primitive data, as M_Ghost reads it).
 *
 * His subclass says which pose he wants each moment (WantedPose: his idle's own cycle) and the figure eases there. Talked
 * to (his speaker point at his mouth: his hat's SOCKET_Speaker, else where the pose puts his mouth), his head turns to the
 * listener and his jaw moves while his lines play. A ghost: nothing meets him but the Interact key's line (a hidden hull
 * about him), and he casts no shadow. He poses only while a player is near (AnimateRadius) and he's in the story.
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API ADeadFriend : public AStoryCharacter
{
	GENERATED_BODY()

public:
	ADeadFriend();

	/** His idle's clock, the pose eased toward what he wants, his head and jaw while he talks. */
	virtual void UpdatePose(float DeltaSeconds) override;

	/** His model (or the Unpaid's) was found: posed by code, rather than the story character's placeholder. */
	bool HasModel() const;

	/** Poses the figure now as it wants to be, at once (the console, the tests). */
	void PoseNow();

	/** The pose he has now (eased toward WantedPose), and the figure's bones as last solved (component space). */
	const FDeadFriendPose& GetPose() const { return Pose; }
	const TArray<FTransform>& GetSolved() const { return Solved; }

	/** Where his head is now (world): where he looks from, and where his words come from. */
	FVector GetHeadLocation() const;

	/** How far his head is turned to whoever he's talking to (degrees), and how open his jaw is (degrees). */
	float GetHeadYaw() const { return HeadYaw; }
	float GetJawOpen() const { return JawOpen; }

	/** His body: posed by code (a poseable mesh: no animation runs). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPoseableMeshComponent> Figure;

	/** His hat on the hat bone (its SOCKET_Speaker at his mouth), and a prop in his fist. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Prop;

	/** His coal's ember: how strongly it glows (M_Ghost's rank color alpha) and its heat (-1 out), as Amos's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Friend", meta = (ClampMin = "0", ClampMax = "2"))
	float EmberStrength = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Friend", meta = (ClampMin = "-1", ClampMax = "0"))
	float EmberHeat = -0.5f;

	/** How far he turns his head to whoever he talks to (degrees), how fast (degrees a second), how far his jaw opens. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Friend", meta = (ClampMin = "0", ClampMax = "80"))
	float HeadTurnLimit = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Friend", meta = (ClampMin = "1"))
	float HeadTurnSpeed = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Friend", meta = (ClampMin = "0", ClampMax = "20"))
	float JawDegrees = 8.f;

	/** How quickly he eases from one pose into the next (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Friend", meta = (ClampMin = "0.5"))
	float PoseEase = 4.f;

	/** He moves only while a player is this near (cm): a cellar or a winch house nobody's in costs nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Friend", meta = (ClampMin = "0", Units = "cm"))
	float AnimateRadius = 2500.f;

	/** The hat's socket at his mouth. */
	static const FName SpeakerSocket;

	/** His banked coal's ember (Amos's, sRGB #96421F) as the rank color M_Ghost reads, linear. */
	static FLinearColor EmberColor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The pose he wants at Time on his own clock (his subclass's idle and moments). */
	virtual FDeadFriendPose WantedPose(float Time) const { return FDeadFriendPose(); }

	/** His subclass's moment-to-moment (its clocks, its sounds), each frame he moves. */
	virtual void TickFriend(float DeltaSeconds) {}

	/** Whether he's talking now (his speaker point's lines playing). */
	bool IsTalking() const;

	/** The player in his level, if any. */
	const APawn* FindPlayer() const;

	// --- For his subclass's constructor: his model and things, found once imported ---

	/** His body: Model, or the Unpaid's without it, in Tint on its body slot (Ghost_A) if given. */
	void WearModel(USkeletalMesh* Model, UMaterialInterface* Tint);
	/** A thing on one of his bones as the model has it at rest (at the bone, its rest turn taken back off). */
	void WearOnBone(UStaticMeshComponent* Part, UStaticMesh* Mesh, FName Bone);

	static USkeletalMesh* FindModel(const TCHAR* Path);
	static UStaticMesh* FindPart(const TCHAR* Path);
	static UMaterialInterface* FindTint(const TCHAR* Path);

	/** His own clock: his idle's cycle runs on it. */
	float FriendClock = 0.f;

private:
	void HandleTalked(USpeakerPointComponent& Point, AActor* InListener);
	/** The banked ember on his body and things. */
	void ApplyLook();
	/** Solves the pose (with his head and jaw) and lays it on the figure. */
	void ApplyPose();
	/** The hull the Interact key's line finds, and his words' place without the hat's socket, from the pose. */
	void PlaceHull();

	FDeadFriendPose Pose;
	bool bPoseStarted = false;
	TArray<FTransform> Solved;
	TWeakObjectPtr<AActor> Listener;
	float HeadYaw = 0.f;
	float JawOpen = 0.f;
	float SpeakClock = 0.f;
	float NearCheck = 0.f;
	bool bPlayerNear = true;
	float HullClock = 0.f;
};
