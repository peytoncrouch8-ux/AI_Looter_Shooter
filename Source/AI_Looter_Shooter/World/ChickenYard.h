#pragma once

#include "CoreMinimal.h"
#include "World/AnimalPose.h"
#include "World/ChickenYardRules.h"
#include "World/FaunaActor.h"
#include "ChickenYard.generated.h"

class UPoseableMeshComponent;
class USkeletalMesh;

/** What one chicken is doing. */
enum class EChickenDoing : uint8
{
	/** Standing, looking about in quick little turns (alert, when someone's near). */
	Stand,
	/** A bout of two to four pecks at the ground. */
	Peck,
	/** A foot raked back twice, a step back, pecks at the spot. */
	Scratch,
	/** A short walk to another spot of its pen, the head held still and snapped forward each step (the rooster struts). */
	Walk,
	/** Down on its belly in the sun. */
	Sit,
	/** The rooster's crow. */
	Crow,
	/** A flapping run away from what scared it, inside its pen. */
	Flee,
};

/** A chicken's timings (s), which its doings (ChickenYardBirds.cpp) and its poses (ChickenYardPose.cpp) share. */
namespace ChickenTimes
{
	/** A peck, down and up again. */
	inline constexpr float Peck = 0.25f;
	/** A scratch's rake of the foot, and its step back. */
	inline constexpr float Rake = 0.4f;
	inline constexpr float StepBack = 0.35f;
	/** The rooster's crow, all of it. */
	inline constexpr float Crow = 2.4f;
	/** A stride (two steps): a hen's walk, the rooster's strut, the flapping run. */
	inline constexpr float HenStride = 0.56f;
	inline constexpr float RoosterStride = 0.6f;
	inline constexpr float RunStride = 0.22f;
}

/** One chicken (in play only). */
struct FChicken
{
	bool bRooster = false;
	bool bWhite = false;
	/** Its pen (an index into the yard's Pens): it never leaves it. */
	int32 Pen = 0;

	/** Its feet (world XY), the ground's height under them, which way it faces (degrees). */
	FVector2D Position = FVector2D::ZeroVector;
	float Height = 0.f;
	float Yaw = 0.f;
	float Scale = 1.f;

	EChickenDoing Doing = EChickenDoing::Stand;
	/** Seconds into what it's doing, and how long that lasts. */
	float Clock = 0.f;
	float Length = 1.f;
	/** Pecks in the bout. */
	int32 Count = 0;
	/** Scratching with the left foot (the table's scratch rakes the right). */
	bool bMirror = false;

	/** A walk or a run: where to, through Via first (round a block's corner), how fast (cm/s), the stride's phase (0-1). */
	FVector2D Target = FVector2D::ZeroVector;
	FVector2D Via = FVector2D::ZeroVector;
	bool bVia = false;
	float Speed = 0.f;
	float Stride = 0.f;
	/** Its share of the last update spent moving rather than turning on the spot (the head-bob follows moving steps). */
	float Moving = 0.f;
	/** A scared bird's moment before it runs (the nearest go first). */
	float Delay = 0.f;

	/** How alert it stands (0-1); whether it watches someone, and where (world). */
	float Alert = 0.f;
	bool bWatching = false;
	FVector WatchAt = FVector::ZeroVector;

	/** The head's turn and tilt against the body (degrees), where they go next, when it looks somewhere else. */
	float HeadYaw = 0.f;
	float HeadPitch = 0.f;
	float LookYaw = 0.f;
	float LookPitch = 0.f;
	float LookClock = 0.f;

	/** The wattles and the tail's tip, swinging; the joints that throw them (world) last update, and how fast they went. */
	FAnimalSpring Wattles;
	FAnimalSpring TailTip;
	FVector LastHead = FVector::ZeroVector;
	FVector LastHeadVelocity = FVector::ZeroVector;
	FVector LastTail = FVector::ZeroVector;
	FVector LastTailVelocity = FVector::ZeroVector;
	int32 SpringFrames = 0;

	/** The pose it was last given, and the one it eases from after changing what it does. */
	FAnimalPose Pose;
	FAnimalPose From;
	float FromBlend = 1.f;
	EChickenDoing PosedDoing = EChickenDoing::Stand;

	FRandomStream Random;
};

/**
 * A chicken yard (the "make it alive" polish, look sheet 44: chickens in a yard or two make the town feel lived in while
 * its people stay unseen): three to five hens, brown and white, and a rooster, in the yard's pens (the picket yard round
 * the coop, the coop's own run). Placed by Tools/Unreal/build_area_animals.py from town.json's chicken spots; an ambient
 * fauna actor (World/FaunaActor.h): nothing ticks, UFaunaSubsystem updates it near the player, and past CullDistance it's
 * hidden and only keeps time.
 *
 * Each bird is SK_HenBrown, SK_HenWhite or SK_Rooster (Art/Models/Creatures/Chicken.py) on a poseable mesh, posed by code
 * from the model's pose table (World/AnimalPoseTables.h), only while on screen. It pecks in bouts (rest -> peck -> rest,
 * a quarter second each), scratches (a foot raked back twice, a step back, pecks at the spot), walks a little (about 30
 * cm/s, the head held still and snapped forward each step), looks about in quick turns, sits in the sun a while; the
 * wattles and the tail's tip swing on springs. The rooster struts and crows every minute or so (World/AnimalCues.h).
 *
 * A player within AlertRadius (scaled by how boldly they move: FaunaRules::FearOf) makes them stand tall and watch; nearer
 * than FleeRadius (a walk at 3.3 m, a sprint at 4.5 m), a gunshot within GunfireRadius or a bullet striking within
 * ImpactRadius scatters them: a flapping run (wings at 8 beats a second) to the far side of their pen, a squawk; they calm
 * 5-8 s after the last scare. Their feet keep to the ground (a height grid traced once as play begins) and inside their
 * pens (ChickenPens), never through the fence or the coop.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AChickenYard : public AFaunaActor
{
	GENERATED_BODY()

public:
	AChickenYard();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override;

	/** Builds the birds, their figures and the ground now (as play begins it), for the tests' worlds. */
	void SetupBirds();

	/** The yard's birds (in play). */
	const TArray<FChicken>& GetChickens() const { return Birds; }

	/** Whether they're scattered (running or not yet calm again). */
	bool IsScared() const { return bScared; }

	/** Scatters them as a noise or a threat at Source would. */
	void Scare(const FVector& Source);

	/** Where a bird's feet are (world). */
	FVector GetFeet(int32 Index) const;

	/** The ground's height under a point of the yard (world Z): the traced grid, else the actor's height. */
	float GroundAt(const FVector2D& Point) const;

	/** A bird's figure (null without its model). */
	UPoseableMeshComponent* GetFigure(int32 Index) const;

	/** The pens (world XY): the birds' feet stay inside each one's outline and out of its blocks by BodyRoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens")
	TArray<FChickenPen> Pens;

	// --- The models (Art/Models/Creatures/Chicken.py; found in /Game/Art/Creatures when left empty) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Model")
	TObjectPtr<USkeletalMesh> HenBrownMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Model")
	TObjectPtr<USkeletalMesh> HenWhiteMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Model")
	TObjectPtr<USkeletalMesh> RoosterMesh;

	/** Whether the birds cast shadows (small, but they ground them). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Model")
	bool bCastShadows = true;

	// --- Behaviour ---

	/** A walking player this near (cm, FaunaRules::FearOf scales it) makes them stand tall and watch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float AlertRadius = 600.f;

	/** A walking player this near scatters them (a sprinting one from 1.35 times as far). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float FleeRadius = 333.f;

	/** A gunshot this near (cm) scatters them, and a bullet's strike this near. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float GunfireRadius = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float ImpactRadius = 800.f;

	/** A hen's walk and the rooster's strut (cm/s), and the flapping run's speed (from Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "1", Units = "cm/s"))
	float WalkSpeed = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "1", Units = "cm/s"))
	float StrutSpeed = 34.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "1", Units = "cm/s"))
	float FleeSpeedMin = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "1", Units = "cm/s"))
	float FleeSpeedMax = 350.f;

	/** How long after the last scare they calm down (s, Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float CalmSecondsMin = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float CalmSecondsMax = 8.f;

	/** How long one sits in the sun (s, Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float SitSecondsMin = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float SitSecondsMax = 30.f;

	/** A body's room (cm): how far a bird's feet keep from its pen's outline and its blocks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float BodyRoom = 12.f;

	// --- Sounds (World/AnimalCues.h; a cue without sounds plays nothing) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound")
	FName CluckCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound")
	FName SquawkCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound")
	FName CrowCue;

	/** A cluck now and then (s, Min to Max) while a player is within CluckRange (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound", meta = (ClampMin = "0.5", Units = "s"))
	float CluckSecondsMin = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound", meta = (ClampMin = "0.5", Units = "s"))
	float CluckSecondsMax = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound", meta = (ClampMin = "0", Units = "cm"))
	float CluckRange = 1500.f;

	/** The rooster's crow now and then (s, Min to Max) while a player is within CrowRange (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound", meta = (ClampMin = "1", Units = "s"))
	float CrowSecondsMin = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound", meta = (ClampMin = "1", Units = "s"))
	float CrowSecondsMax = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens|Sound", meta = (ClampMin = "0", Units = "cm"))
	float CrowRange = 6000.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnFaunaShownChanged(bool bShown) override;

private:
	// --- The yard (ChickenYard.cpp) ---

	void AddBird(int32 PenIndex, bool bRooster);
	/** The ground under the pens, traced once on a grid. */
	void TraceGround();
	/** Whether a threat or a noise scatters them now; who watches whom. */
	void CheckAlarm(const FFaunaTick& Tick, const FFaunaContext& Context);
	void UpdateCalm(float DeltaSeconds, const FFaunaContext& Context);
	/** Clucks, the rooster's crow, the squawks of a scatter. */
	void UpdateVoices(float DeltaSeconds, const FFaunaContext& Context);
	void PlayCue(FName Cue, const FVector& Where) const;
	/** A bird's head (world), near enough for where it looks from and its voice. */
	FVector HeadOf(const FChicken& Bird) const;

	// --- The birds (ChickenYardBirds.cpp) ---

	void UpdateBird(int32 Index, float DeltaSeconds);
	/** What a bird does next, now it has finished what it did. */
	void ChooseNext(int32 Index);
	void StartStand(FChicken& Bird, float MinSeconds, float MaxSeconds);
	bool StartWalk(int32 Index);
	void StartFlee(int32 Index, const FVector2D& Source);
	void StartCrow(int32 Index);
	/** Moves a walking or running bird on; true once it's there (or can't go on). */
	bool MoveOn(FChicken& Bird, float DeltaSeconds, float TurnRate, bool bTurnFirst);
	/** A step back while scratching, if there's room. */
	void StepBack(FChicken& Bird, float Distance);
	void UpdateHead(FChicken& Bird, float DeltaSeconds);

	// --- Posing (ChickenYardPose.cpp) ---

	void MakeFigures();
	/** Every bird's pose and figure, while on screen. */
	void PoseBirds(float DeltaSeconds);
	void BuildPose(const FChicken& Bird, FAnimalPose& Out) const;
	void UpdateSprings(int32 Index, float DeltaSeconds);

	TArray<FChicken> Birds;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPoseableMeshComponent>> Figures;

	/** Each bird's rig (its figure's skeleton bound to its table). */
	TArray<FAnimalRig> Rigs;

	/** Each pen's spots to walk and run to, and the corners round its blocks. */
	TArray<TArray<FVector2D>> PenSpots;
	TArray<TArray<FVector2D>> PenCorners;

	/** The ground: heights every GroundStep cm over GroundBox (world). */
	FBox2D GroundBox = FBox2D(ForceInit);
	int32 GroundColumns = 0;
	int32 GroundRows = 0;
	TArray<float> GroundHeights;

	FVector Center = FVector::ZeroVector;
	float Reach = 0.f;

	bool bScared = false;
	FVector ScaredBy = FVector::ZeroVector;
	float CalmTime = 0.f;
	float CalmNeeded = 6.f;
	float CluckTimer = 0.f;
	float CrowTimer = 0.f;
	/** The scatter's squawks still to come: seconds to wait, and whose. */
	TArray<TPair<float, int32>> Squawks;
	FRandomStream YardRandom;
	bool bBirdsReady = false;
};
