#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/ShroudChain.h"
#include "Creatures/UnpaidRules.h"
#include "UnpaidCreature.generated.h"

class AShriekRing;
class UPrimitiveComponent;
class UMaterialInterface;
class UStaticMeshComponent;

/** Where AUnpaidCreature keeps its look in its mesh's custom primitive data (M_Ghost reads them; build_creature_materials.py). */
namespace UnpaidLook
{
	/** The rank's color (RGB, linear) and how strongly it glows (A; 0 on a mesh no creature drives: a Basic coal). */
	inline constexpr int32 RankColorIndex = 0;
	/** The phase-step's dissolve: 0 solid, 1 gone. */
	inline constexpr int32 PhaseIndex = 4;
	/** The coal's flare on top of its glow: 0 as it is, up on a crit, -1 out. */
	inline constexpr int32 HeatIndex = 5;
}

/** One arm's bones in SK_Unpaid, from the shoulder out. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FUnpaidArmBones
{
	GENERATED_BODY()

	FUnpaidArmBones() = default;

	/** The arm on one side, named as Unpaid.py names it: Side is "l" or "r". */
	explicit FUnpaidArmBones(const TCHAR* Side);

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName UpperArm;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName LowerArm;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Hand;

	/** A bone per finger, thumb first and little finger last: they curl together and fan out round the middle one. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> Fingers;
};

/**
 * The bones AUnpaidCreature poses, by their names in SK_Unpaid (Art/Models/Creatures/Unpaid.py). If the model names one
 * differently, set the name in Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.UnpaidCreature] (Rig=(...)): no code
 * change. The pelvis, spine, chest, neck, head and the arms down to the hands are needed; a jaw, coal, finger or tail bone
 * the model lacks is simply left at rest.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FUnpaidRigBones
{
	GENERATED_BODY()

	FUnpaidRigBones();

	/** Carries the body: it hovers and leans here. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Pelvis;

	/** The waist and the chest. A shot on the chest's (or the waist's, or the coal's) hit zone may find the coal. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Spine;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Chest;

	/** Where the coal burns through the chest, at about 1.3 m: the crit spot is found round it. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Coal;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Neck;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Head;

	/** Drops for the shriek. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Jaw;

	/** Carries the hat, which is a mesh of its own (SM_UnpaidHat) and follows the head. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Hat;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FUnpaidArmBones LeftArm;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FUnpaidArmBones RightArm;

	/** The shroud's chain, top first, each bone the child of the one before. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> Shroud;

	/** The short chains of the outer strips at its sides, top first. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> LeftStrip;

	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> RightStrip;
};

/**
 * The Unpaid (Docs/Areas/RansomsRest.md, "Enemies by rank"; Docs/Story.md): the restless dead of Ransom's Rest, pale
 * homesteaders in the clothes they died in, fading below the waist into a trailing shroud, with a coal burning through
 * the chest. 160 health and 8 damage at level 1. It floats: no legs, no walk cycle and no navmesh; it steers as every
 * creature does (ACreatureBase), drifting slow to start and slow to stop.
 *
 *  - The coal is its critical spot, found by the shot's line as the slime's core is (UnpaidRules::IsCoalShot): a shot
 *    that lands on its chest from the front (within CoalViewAngle) and whose line passes within CoalCritRadius of the
 *    coal is critical. From behind, or through an arm, a hand or the head in the way, it isn't.
 *  - It attacks with a shriek and a lunge (UnpaidCreatureAttack.cpp): the shriek is the wind-up (jaw dropped, arms
 *    spread), then it flies at its target and the strike lands if it reaches them. Back off and the lunge falls short.
 *  - Stuck, or fallen far behind, it phase-steps (UnpaidCreaturePhase.cpp): fades out and comes back 3 to 5 m nearer its
 *    target, never into a wall, off its level of ground or off its hunting ground.
 *  - Its rank shows in its coal and ember edge (custom primitive data, UnpaidLook): dull red Basic, the rank's tag color
 *    above it. Restless and up lunge faster; Gravebound and up shriek a ring that slows the player (AShriekRing).
 *  - It wears one of three sets of clothes, hat and all: the model's own (Ghost_A) or MI_Ghost_B and C (OtherClothes).
 *
 * The body is SK_Unpaid (Art/Models/Creatures/Unpaid.py), 33 bones under its root, all posed by code (UnpaidCreatureRig.cpp) on
 * top of the model's rest pose, the idle hang: it bobs, leans into its drift, looks at its target, its arms and fingers
 * reach and claw, and the shroud's chains (FShroudChain) trail its motion. Without the model (a test level, or before it
 * is imported) it hunts as well, unseen: its coal is where the model carries it (CoalPointWithoutRig).
 */
UCLASS(Config = Game)
class AI_LOOTER_SHOOTER_API AUnpaidCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	AUnpaidCreature();

	virtual void Tick(float DeltaSeconds) override;

	// Crits: a shot onto its chest from the front whose line passes close to the coal.
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	// --- Its coal ---

	/** Where its coal is now (world): the coal bone, or where the model carries it when it has no rig. */
	FVector GetCoalLocation() const;

	/** Which way its coal faces now (world): out of its chest. */
	FVector GetCoalFacing() const;

	/** Its coal's color at its rank: BasicCoalColor, or the rank's tag color (UCreatureRankSettings). */
	FLinearColor GetCoalColor() const;

	/** Its hat (SM_UnpaidHat): on the hat bone once it has its model, hidden without one. */
	UStaticMeshComponent* GetHat() const { return Hat; }

	// --- Its attack ---

	/**
	 * Shrieks now: its lunge's wind-up starts with it. A Gravebound one's shriek sends out a ring at its target that slows
	 * them (at most once every ShriekCooldown seconds): that ring, else null. Public for the tests.
	 */
	AShriekRing* Shriek();

	/** What its rank adds (UnpaidRules::RankTraits). */
	const FUnpaidRankTraits& GetRankTraits() const { return Traits; }

	/** How fast its lunge flies (cm/s) at its rank and size. */
	float GetLungeSpeed() const;

	/** Whether it's in the middle of a lunge. */
	bool IsLunging() const { return bLunging; }

	// --- Phase-steps ---

	/** Whether it's fading out or back in. */
	bool IsPhasing() const;

	/** How far it's dissolved now: 0 solid to 1 gone (its material's Phase). */
	float GetPhase() const { return PhaseAmount; }

	/**
	 * Fades it in where it stands, as the end of a phase-step does (its shroud's ends first, shot through until it's mostly
	 * there): an add rising through the boards (Abel's). Nothing while it's dead.
	 */
	void RiseIn();

	// --- Settings ---

	/** Its bones in SK_Unpaid. */
	UPROPERTY(Config, EditAnywhere, Category = "Unpaid|Rig")
	FUnpaidRigBones Rig;

	/** The body's material slot on SK_Unpaid, where other clothes go (the coal's slot, GhostCoal, keeps its own). */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Look")
	FName BodySlot = TEXT("Ghost_A");

	/** Clothes besides the model's own: MI_Ghost_B and MI_Ghost_C (build_creature_materials.py). */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Look")
	TArray<TObjectPtr<UMaterialInterface>> OtherClothes;

	/** Which clothes it wears: 0 the model's own, 1 and up OtherClothes; -1 any, picked as play begins. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Look", meta = (ClampMin = "-1"))
	int32 Clothes = -1;

	/** A Basic one's coal: dull red, which is no rarity color (Docs/Story.md, "Ranks show in soul-light"). */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Look")
	FLinearColor BasicCoalColor;

	/** A shot's line passing within this of the coal's middle is critical (cm, at size 1; the coal itself is about 7). */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Coal", meta = (ClampMin = "1", Units = "cm"))
	float CoalCritRadius = 12.f;

	/** The coal shows to shots from within this angle of the way it faces (degrees). */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Coal", meta = (ClampMin = "0", ClampMax = "90"))
	float CoalViewAngle = 60.f;

	/** How far past where a shot lands on the chest its line may still find the coal (cm, at size 1). */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Coal", meta = (ClampMin = "1", Units = "cm"))
	float CoalReach = 60.f;

	/** Where the model carries the coal (its own space: X forward, Z up from the ground), for when it has no rig. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Coal")
	FVector CoalPointWithoutRig = FVector(10.9f, -5.9f, 131.3f);

	/** How fast its lunge flies (cm/s at size 1, before its rank's) and how far at most (cm at size 1). */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "100", Units = "cm/s"))
	float LungeSpeed = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "0", Units = "cm"))
	float LungeDistance = 320.f;

	/** The lunge stops this short of its target's middle (cm at size 1), and its strike lands within LungeHitRadius of it. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "0", Units = "cm"))
	float LungeStopShort = 90.f;

	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "0", Units = "cm"))
	float LungeHitRadius = 130.f;

	/** How far the jaw drops at the shriek's height (degrees), from a mouth 10 open: about 46 in all. The model rests at its idle, 16 open. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "0", ClampMax = "60"))
	float JawOpenDegrees = 36.f;

	/** A slowing shriek's ring reaches this far (cm at size 1), slowing to this share of speed for this long. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "100", Units = "cm"))
	float ShriekRadius = 900.f;

	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "0.05", ClampMax = "1"))
	float ShriekSlowMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "0", Units = "s"))
	float ShriekSlowSeconds = 2.f;

	/** Seconds from one slowing shriek to the next at the earliest: not every lunge's shriek slows. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Attack", meta = (ClampMin = "0", Units = "s"))
	float ShriekCooldown = 5.f;

	/** When and how far it phase-steps. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Phase Step")
	FPhaseStepRules PhaseStep;

	/** Seconds to fade out, and to come back. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Phase Step", meta = (ClampMin = "0.05", Units = "s"))
	float PhaseFadeOutSeconds = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Unpaid|Phase Step", meta = (ClampMin = "0.05", Units = "s"))
	float PhaseFadeInSeconds = 0.35f;

	/** How its shroud and side strips move. */
	UPROPERTY(EditAnywhere, Category = "Unpaid|Shroud")
	FShroudChainSettings Shroud;

protected:
	virtual void BeginPlay() override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnPoseThawed() override;
	virtual void OnRankChanged() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual bool CanStartAttack() const override;

	// --- For a body built on the Unpaid's rig (Abel, the Keeper: Bosses/AbelKeeper.h) ---

	/** Bones such a body poses besides the Unpaid's (Abel's lantern, gun and coat skirts), by name; one the model lacks is left out. */
	virtual void GetExtraRigBones(TArray<FName>& OutBones) const {}

	/**
	 * Once this frame's pose is worked out channel by channel and the shroud's chains have swung, before it's solved: such a
	 * body lays a pose of its own over the bones' turns and shifts (Bones' Own and Shift). The Unpaid lays nothing.
	 */
	virtual void LayerPose(float DeltaSeconds) {}

	/** Its look as it dies, DeathTime into its death: by default it slumps, then dissolves while its coal goes out. */
	virtual void UpdateDeathLook(float DeltaSeconds);

	/** One bone the code poses: its rest pose (component space), this frame's turn and shift, and the pose they make. */
	struct FPosedBone
	{
		FName Name;
		int32 SkeletonIndex = INDEX_NONE;
		/** The nearest posed bone above it (an index into Bones), or none. */
		int32 Parent = INDEX_NONE;
		FTransform Rest;
		/** Its own turn about its joint this frame, in the rest pose's frame (it takes its parent's turn on top). */
		FQuat Own = FQuat::Identity;
		/** Its own shift this frame (component space) and size against its rest. */
		FVector Shift = FVector::ZeroVector;
		float Scale = 1.f;
		/** A shroud link: its chain and link (its turn comes from the chain, under what the chain hangs from). */
		int32 Chain = INDEX_NONE;
		int32 Link = INDEX_NONE;
		/** Its turn from rest with everything above it, and its pose: worked out each frame. */
		FQuat Turned = FQuat::Identity;
		FTransform Posed;
	};

	/** Finds a bone of the rig by name (INDEX_NONE when the model lacks it). */
	int32 FindPosedBone(FName Name) const;

	/** The bones it poses, parents first (none without its model), and whether it has its rig. */
	TArray<FPosedBone> Bones;
	bool bRigReady = false;

	/**
	 * How far the shroud's chains swing its links: 1, they do (the Unpaid, always); at 0 the links take their own turns
	 * (Abel knelt or sat, his shroud lying on the boards as his pose table lays it).
	 */
	float ChainSwing = 1.f;

	/** How far it's dissolved (its material's Phase: 0 solid, 1 gone), its coal's flare (0 as it is, up on a crit, -1 out). */
	float PhaseAmount = 0.f;
	float Heat = 0.f;

	/** Seconds since it died. */
	float DeathTime = 0.f;

	/** Meshes that wear the body's look besides the hat (Abel's lantern and pump): its rank's color, its dissolve, its flare. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> LookParts;

private:
	// --- Look (UnpaidCreature.cpp) ---
	/** Puts on its clothes: the model's own, or one of OtherClothes. */
	void PutOnClothes();
	/** Puts the hat on the hat bone as it was modelled, or hides it without the bone or the hat. */
	void PutOnHat();
	/** Writes its rank's color into the mesh's custom primitive data. */
	void ApplyCoalColor();
	/** Writes its dissolve and the coal's flare when they change, and turns its hit zones off while it's faded away. */
	void ApplyLook(bool bForce);
	bool IsCoalShotBone(FName Bone) const;

	// --- The lunge (UnpaidCreatureAttack.cpp) ---
	void TickLunge(float DeltaSeconds);
	/** Strikes whoever the lunge has reached (once); whether it did. */
	bool TryLungeHit();
	void EndLunge();

	// --- Phase-steps (UnpaidCreaturePhase.cpp) ---
	enum class EPhaseState : uint8
	{
		None,
		/** Fading away where it stands. */
		Out,
		/** Gone, moved, and fading back in nearer its target. */
		In
	};
	void TickPhaseStep(float DeltaSeconds);
	/** Keeps up how long it has chased without getting nearer (stuck, or being outrun). */
	void WatchProgress(float DeltaSeconds);
	void ResetProgress();
	bool WantsToStep() const;
	void StartPhaseStep();
	/** Where a step lands now: the rules' first spot the level allows (ground, room, sight of its target, safe zones). */
	bool FindPhaseSpot(FVector& OutFeet) const;
	/** Unseen at last: moves to the spot it chose and starts coming back. */
	void Arrive();

	// --- The body (UnpaidCreatureRig.cpp) ---
	struct FArm
	{
		float Side = 1.f;
		int32 UpperArm = INDEX_NONE;
		int32 LowerArm = INDEX_NONE;
		int32 Hand = INDEX_NONE;
		TArray<int32> Fingers;
		/** The axis each finger curls about, in the rest pose's frame, and how far it fans from the middle one (in fingers). */
		TArray<FVector> CurlAxes;
		TArray<float> FanOffsets;
		/** The idle curl and fan each finger rests in (the model bakes them in), taken back off as it's posed. */
		TArray<FQuat> IdleTurns;
	};

	/** The pose's smoothed channels (degrees, or 0 to 1). */
	struct FPoseChannels
	{
		float Lean = 0.f;
		float Rear = 0.f;
		float Reach = 0.f;
		float Spread = 0.f;
		float Jaw = 0.f;
		float Curl = 0.f;
		float Splay = 0.f;
		float Snap = 0.f;
		float LookYaw = 0.f;
		float LookPitch = 0.f;
		float Death = 0.f;
	};

	/** Reads its rig from the skeleton; false without the model or the bones it needs (it then stays at rest). */
	bool SetupRig();
	/** Works out this frame's pose and hands it to the animation (GetBonePose). */
	void AnimateBody(float DeltaSeconds);
	/** Where each channel heads now, from what the brain is doing (Speed: its drift against its chase speed; Time: its clock). */
	FPoseChannels PoseTargets(float Speed, float Time) const;
	/** Works the bones' poses out from their turns, parents first. */
	void SolvePose();

	// The rig
	int32 PelvisBone = INDEX_NONE;
	int32 SpineBone = INDEX_NONE;
	int32 ChestBone = INDEX_NONE;
	int32 CoalBone = INDEX_NONE;
	int32 NeckBone = INDEX_NONE;
	int32 HeadBone = INDEX_NONE;
	int32 JawBone = INDEX_NONE;
	FArm Arms[2];
	/** The shroud, its left strip and its right strip; what each hangs from (an index into Bones), and each link's direction at rest. */
	FShroudChain Chains[3];
	int32 ChainAnchors[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
	TArray<FVector> ChainRestDirections[3];

	// The pose
	FPoseChannels Pose;
	bool bPoseStarted = false;
	float PoseTime = 0.f;
	float LastYaw = 0.f;
	/** Each one moves in its own time, so a crowd doesn't bob in step. */
	float PoseSeed = 0.f;
	/** A hit's flinch (0 to 1, easing off) and which way it pushed (the body's frame). */
	float Flinch = 0.f;
	FVector FlinchAway = FVector::ZeroVector;

	// Rank
	FUnpaidRankTraits Traits;
	bool bTimingCaptured = false;
	float BaseWindup = 0.6f;

	// Attack
	bool bLunging = false;
	bool bLungeLanded = false;
	FVector LungeDirection = FVector::ForwardVector;
	float LungeLeft = 0.f;
	float LungeTime = 0.f;
	float SinceSlowingShriek = 1000.f;

	// Phase-steps
	EPhaseState PhaseState = EPhaseState::None;
	float PhaseTime = 0.f;
	FVector PhaseDestination = FVector::ZeroVector;
	float SinceLastStep = 1000.f;
	float StallTime = 0.f;
	float BestDistance = TNumericLimits<float>::Max();
	float RetryIn = 0.f;

	/** The hat is a mesh of its own: it wears the body's clothes and look (custom primitive data), and casts no shadow either. */
	UPROPERTY(VisibleAnywhere, Category = "Unpaid|Look")
	TObjectPtr<UStaticMeshComponent> Hat;

	// Look
	float ShownPhase = -1.f;
	float ShownHeat = -10.f;
	bool bHitVolumesWanted = true;
	bool bHitVolumesOn = true;
};
