#pragma once

#include "CoreMinimal.h"
#include "Story/StoryCharacter.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "HobBird.generated.h"

class UPoseableMeshComponent;
class UStaticMeshComponent;
class UStoryLineSet;

/** One of Hob's perches: when he's there, where his feet grip, which way he faces, and what he says as he lands. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FHobPerch
{
	GENERATED_BODY()

	/** When this is his perch: the first perch whose condition holds is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perch")
	FStoryCondition When;

	/** Where his feet grip (world, cm; a SOCKET_Perch_* socket's place, or any spot) and which way he faces (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perch")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perch")
	float Yaw = 0.f;

	/**
	 * What he says as he lands here, after whatever is being said (his remarks queue). Lines naming no one are his. A
	 * perch in the same spot as the last says its piece without a flight (the story moved on; he stayed).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perch")
	TArray<FStoryLine> Arrival;

	/** The landing's lines from an asset instead, when set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perch")
	TObjectPtr<UStoryLineSet> ArrivalSet;
};

/**
 * Hob, the one-eyed crow (Docs/Story.md: Cast; Docs/Areas/RansomsRest.md, Scope cuts item 4: "a plain bird on fixed
 * perches"): SK_Hob, the Revenant crow the user picked (Art/Models/Creatures/Hob.py), posed by code. He perches near the
 * next objective, and only Ellis hears him. Perched, he breathes, tilts his head in a crow's small sudden steps and now
 * and then ruffles; talked to, he turns to whoever talks. When the story moves on he flies to the perch that suits it,
 * wings open and beating, lands and says his piece (queued after whatever is being said); with no perch for this point in
 * the story he's away. In a checkout without his model he's a stand-in of the engine's plain shapes.
 *
 * Perches are data on the placed actor (Tools/Unreal/build_area_story.py): in Main 1 he sits on Ellis's headboard from
 * the claw-out on, silent until Ellis is out. His feet grip the perch at the actor's origin, his front along +X, as the
 * model's rest pose has it. He's a friend: no hit zones, so shots pass through him.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AHobBird : public AStoryCharacter
{
	GENERATED_BODY()

public:
	AHobBird();

	/** The story's condition and a perch for this point in it: he's there (flying in when he wasn't), or away. */
	virtual void RefreshShown() override;

	/** Flying between perches, wings beating; perched, his idle; turning to whoever talks to him, as the story character does. */
	virtual void UpdatePose(float DeltaSeconds) override;

	/** His perches; the first whose condition holds is his. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hob")
	TArray<FHobPerch> Perches;

	/** How long a flight takes (seconds); flying in from away, he drops in from above and behind his perch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hob", meta = (ClampMin = "0.1"))
	float FlightSeconds = 1.4f;

	/** The perch whose condition holds now in the story, or INDEX_NONE. */
	int32 FindPerch() const;

	/** The perch he's on or flying to, or INDEX_NONE (away). */
	int32 GetPerch() const { return Perch; }

	bool IsFlying() const { return FlightLeft > 0.f; }

	/** Ends a flight now: landed, his piece said (tests; a level that wants him there at once). */
	void FinishFlight();

	/** His model posed by code (SK_Hob's bones found), rather than the stand-in. */
	bool HasRig() const { return bRigReady; }

protected:
	virtual void BeginPlay() override;

private:
	void FlyTo(int32 Index);
	void Land();
	TArray<FStoryLine> GetArrivalLines(const FHobPerch& At) const;

	// --- The rig (HobBirdRig.cpp) ---

	/** One bone he poses: its rest in the model's frame, the turn about its own joint, where it ends up. */
	struct FHobBone
	{
		FName Name;
		/** The nearest posed bone above it (INDEX_NONE: none). */
		int32 Parent = INDEX_NONE;
		FTransform Rest = FTransform::Identity;
		FQuat Own = FQuat::Identity;
		FVector Shift = FVector::ZeroVector;
		FQuat Turned = FQuat::Identity;
		FTransform Posed = FTransform::Identity;
		/** A wing's bone: its side (0 left, 1 right) and which (0 upper, 1 fore, 2 hand, 3 fan). */
		int32 Wing = INDEX_NONE;
		int32 Part = INDEX_NONE;
	};

	/** Reads SK_Hob's skeleton: the bones he poses. False (he stays in his rest pose) without the model or its bones. */
	bool SetupRig();

	/** His pose this frame: breathing, the head's steps, a ruffle now and then, the wings as the flight has them. */
	void PoseRig(float DeltaSeconds);

	/** Each bone where its own turn and its parent's put it; the wings blended open and flapped about the shoulder. */
	void SolveRig();

	/** The pose onto the model. */
	void ApplyRig();

	int32 FindRigBone(FName Name) const;

	/** His model, posed by code (null in a checkout without it: the stand-in shows instead). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPoseableMeshComponent> Bird;

	/** The stand-in's parts, when there's no model. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> StandIn;

	TArray<FHobBone> RigBones;
	int32 BodyBone = INDEX_NONE;
	int32 NeckBone = INDEX_NONE;
	int32 HeadBone = INDEX_NONE;
	int32 TailBone = INDEX_NONE;
	bool bRigReady = false;

	/** The idle: its clock, its dice, the head's step under way, the next step's and ruffle's times. */
	float RigClock = 0.f;
	FRandomStream Moods{ 0x40b };
	int32 HeadStep = 0;
	FQuat HeadFrom = FQuat::Identity;
	FQuat NeckFrom = FQuat::Identity;
	float StepClock = 10.f;
	float NextStepAt = 1.f;
	float RuffleClock = 10.f;
	float NextRuffleAt = 8.f;

	/** How far his wings are open (0 folded, 1 open) and their beat about the shoulder (degrees up). */
	float WingOpen = 0.f;
	float WingFlap = 0.f;

	int32 Perch = INDEX_NONE;
	bool bBegun = false;

	FVector FlightFrom = FVector::ZeroVector;
	FVector FlightTo = FVector::ZeroVector;
	float FlightLeft = 0.f;
	float FlightTotal = 1.f;
};
