#pragma once

#include "CoreMinimal.h"
#include "World/AnimalPose.h"
#include "World/FaunaActor.h"
#include "BarnCat.generated.h"

class UPoseableMeshComponent;
class USkeletalMesh;

/** The cat's timings (s), which its doings (BarnCat.cpp) and its pose (BarnCatPose.cpp) share. */
namespace CatTimes
{
	/** A stretch: up on its feet, the stretch held, up again. */
	inline constexpr float Stretch = 2.6f;
}

/** What the porch cat is doing. */
enum class ECatDoing : uint8
{
	/** Loafing on the boards (watching whoever is near). */
	Loaf,
	/** Sitting up a while. */
	Sit,
	/** Up on its feet, a long stretch, then off a few steps. */
	Stretch,
	/** A few steps along the porch (or off along it, away from someone lingering). */
	Walk,
	/** Standing, tail up, for a player close by. */
	Greet,
	/** Low and quick (or slow, put out) to its hiding place under the porch. */
	Slink,
	/** Under the porch, out of sight. */
	Hidden,
	/** Out again and back up to its spot. */
	Emerge,
};

/**
 * The porch cat (the "make it alive" polish, look sheet 44): a lean mackerel tabby on a porch, so the town's homes feel
 * lived in while their people stay unseen. Placed by Tools/Unreal/build_area_animals.py from town.json's cat spot, with
 * its porch found by traces (the walk along the deck, the edge it jumps down from, its hiding place under the porch); an
 * ambient fauna actor (World/FaunaActor.h): nothing ticks, UFaunaSubsystem updates it near the player.
 *
 * Its body is SK_BarnCat (Art/Models/Creatures/Cat.py) on a poseable mesh, posed by code from the model's pose table
 * (World/AnimalPoseTables.h), only while on screen. It loafs at its spot breathing slowly, the tail's tip flicking now and
 * then, an ear twitching; now and then it sits up, stretches or walks a few steps (walk_a, walk_b, half a second a step,
 * its paws keeping pace). A player within WatchRadius (FaunaRules::FearOf scales it) gets watched: the head up and
 * following them (neck and head sharing up to 70 degrees), ears forward. Within GreetRadius it stands, tail up; a player
 * who lingers there sends it walking off along the porch. A gunshot within GunfireRadius, a bullet's strike near it, a
 * sprint right up to it (a hiss) or a hostile creature sends it slinking, ears back, down off the porch and under it,
 * gone; it comes back 20-40 s after things are calm.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABarnCat : public AFaunaActor
{
	GENERATED_BODY()

public:
	ABarnCat();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override;

	/** Puts it on its spot (as play begins), for the tests' worlds. */
	void SetupCat();

	ECatDoing GetDoing() const { return Doing; }
	bool IsHiding() const { return Doing == ECatDoing::Hidden; }
	/** Where its feet are (world) and which way it faces (degrees). */
	FVector GetFeet() const { return Position; }
	float GetFacing() const { return Yaw; }

	/** Sends it off to hide as a gunshot would (bRun), or a lingering player (walking, put out). */
	void Spook(bool bRun);

	/** Its body: SK_BarnCat, posed by code. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPoseableMeshComponent> Figure;

	// --- Its porch (world; the build script finds these) ---

	/** Where it loafs (on the deck) and which way it faces there (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Porch")
	FVector LoafSpot = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Porch")
	float LoafYaw = 0.f;

	/** The two ends of the stretch of deck it walks along (its spot between them). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Porch")
	FVector PorchA = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Porch")
	FVector PorchB = FVector::ZeroVector;

	/** The deck's edge it jumps down from (on the deck), and the ground in front of it where it lands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Porch")
	FVector PorchExit = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Porch")
	FVector PorchStep = FVector::ZeroVector;

	/** Its hiding place: on the ground just under the porch's edge, where it slips out of sight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Porch")
	FVector HidePoint = FVector::ZeroVector;

	// --- Behaviour ---

	/** A walking player this near (cm, FaunaRules::FearOf scales it) is watched; this near, it stands to greet them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float WatchRadius = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float GreetRadius = 300.f;

	/** A sprinting player this near (cm) sends it off with a hiss; a hostile creature within CreatureRadius does too. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float SprintRadius = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float CreatureRadius = 800.f;

	/** A gunshot this near (cm), or a bullet's strike this near, sends it under the porch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float GunfireRadius = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float ImpactRadius = 700.f;

	/** How long a player may stay within GreetRadius before it walks off (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float LingerSeconds = 6.f;

	/** How long after things are calm it comes back out (s, Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float ReturnSecondsMin = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float ReturnSecondsMax = 40.f;

	/** Its walk, and its run under the porch (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "1", Units = "cm/s"))
	float WalkSpeed = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour", meta = (ClampMin = "1", Units = "cm/s"))
	float SlinkSpeed = 250.f;

	/** Whether it casts a shadow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Behaviour")
	bool bCastShadows = true;

	// --- Sounds (World/AnimalCues.h; a cue without sounds plays nothing) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Sound")
	FName MeowCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cat|Sound")
	FName HissCue;

protected:
	virtual void BeginPlay() override;

private:
	// --- What it does (BarnCat.cpp; its ways about the porch: BarnCatPath.cpp) ---

	/** Whether a noise, a sprint or a creature sends it off; who it watches; a player lingering. */
	void CheckSenses(const FFaunaTick& Tick, const FFaunaContext& Context, float DeltaSeconds);
	void UpdateDoing(float DeltaSeconds);
	void StartDoing(ECatDoing Next, float InLength);
	/** Sets off along Points (from where it is) at Speed, doing Kind (a walk, the slink under the porch, coming back out). */
	void StartPath(ECatDoing Kind, TConstArrayView<FVector> Points, float Speed);
	/** Moves it along its path; true at its end. */
	bool FollowPath(float DeltaSeconds);
	/** Something idle: sit up, stretch, a few steps. */
	void ChooseIdle();
	/** A few steps along the porch, one way or the other. */
	void StartFewSteps();
	/** The far end of the porch from Where. */
	FVector FarEnd(const FVector& Where) const;
	FVector StepPoint() const;
	bool IsOnDeck(const FVector& Where) const;
	void PlayCue(FName Cue) const;

	// --- Its pose (BarnCatPose.cpp) ---

	void BuildPose(FAnimalPose& Out) const;
	void PoseFigure(float DeltaSeconds);
	void UpdateLook(float DeltaSeconds);

	ECatDoing Doing = ECatDoing::Loaf;
	ECatDoing PosedDoing = ECatDoing::Loaf;
	float Clock = 0.f;
	float Length = 1.f;

	FVector Position = FVector::ZeroVector;
	float Yaw = 0.f;
	/** Its body's pitch on a jump (degrees). */
	float Pitch = 0.f;

	/** Its path: the points still to reach, the leg under way (from, how far along, cm), its speed, a jump's arc. */
	TArray<FVector, TInlineAllocator<6>> Path;
	FVector LegFrom = FVector::ZeroVector;
	float LegDone = 0.f;
	float PathSpeed = 0.f;
	/** The walk's (or slink's) stride phase, 0-1, kept in step with the ground it covers. */
	float Stride = 0.f;
	/** After a walk: off along the porch from someone lingering (it loafs facing them at the end). */
	bool bWalkingOff = false;
	/** A slink in a hurry (gunfire), not a put-out walk. */
	bool bRunning = false;

	/** Watching someone: how much (0-1), where (world). */
	float Watch = 0.f;
	bool bWatching = false;
	FVector WatchAt = FVector::ZeroVector;
	/** A player within GreetRadius, for how long; and how long since one was. */
	bool bClose = false;
	float CloseTime = 0.f;
	float AwayTime = 0.f;
	/** Calm under the porch (s), and how long it waits. */
	float CalmTime = 0.f;
	float ReturnAfter = 30.f;
	float MeowTimer = 90.f;
	float IdleTimer = 30.f;

	/** The head's look (degrees against the body), an ear's twitch, the tail tip's flick, a breath (each a clock). */
	float HeadYaw = 0.f;
	float HeadPitch = 0.f;
	float TwitchClock = 10.f;
	float TwitchTimer = 5.f;
	bool bTwitchLeft = false;
	float FlickClock = 10.f;
	float FlickTimer = 4.f;
	float FlickSide = 1.f;
	float Breath = 0.f;

	FAnimalPose Pose;
	FAnimalPose From;
	float FromBlend = 1.f;
	FAnimalRig Rig;
	/** How far the loaf_watch pose turns its head on its own (degrees): the look adds only the rest. */
	float WatchPoseYaw = 0.f;

	FRandomStream Random;
	bool bCatReady = false;
};
