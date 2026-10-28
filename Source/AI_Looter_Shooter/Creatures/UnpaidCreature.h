#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureBodyHulls.h"
#include "Creatures/ShroudChain.h"
#include "Creatures/UnpaidBodyParts.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidRigBones.h"
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
 *  - Its fighting kit (UnpaidCreatureMoves.cpp, Looter.Unpaid.*, round 2): the lunge leads its target, close in it rakes
 *    twice, on the ring it throws cinders and screams its pack on together, given its turn in front of the player's eyes it
 *    fades round to their side. Kinds on its rig with attacks of their own (the miners, the Bell-Ringer, Abel) and a boss's
 *    adds keep the old way (HasFightKit).
 *  - Stuck, or fallen far behind, it phase-steps (UnpaidCreaturePhase.cpp): fades out and comes back 3 to 5 m nearer its
 *    target, never into a wall, off its level of ground or off its hunting ground.
 *  - Its rank shows in its coal and ember edge (custom primitive data, UnpaidLook): dull red Basic, the rank's tag color
 *    above it. Restless and up lunge faster; Gravebound and up shriek a ring that slows the player (AShriekRing).
 *  - It wears one of three sets of clothes, hat and all: the model's own (Ghost_A) or MI_Ghost_B and C (OtherClothes).
 *
 * The body is SK_Unpaid (Art/Models/Creatures/Unpaid.py), 33 bones under its root, all posed by code (UnpaidCreatureRig.cpp) on
 * top of the model's rest pose, the idle hang: it bobs, leans into its drift, looks at its target, and the shroud's chains
 * (FShroudChain) trail its motion, lifting off ground that rises behind it. Its arms (UnpaidCreatureArms.cpp) drift loose
 * at rest, hang against its lean, reach and claw while it hunts, fling wide for the shriek, lag behind its moves and its
 * hits on springs, and never pass into its own torso: the model's hit hulls keep them out. Without the model (a test level,
 * or before it is imported) it hunts as well, unseen: its coal is where the model carries it (CoalPointWithoutRig).
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

	/** Its body and the top of its shroud trailing behind it (the rest of the shroud is thin and fades, and may pass through). */
	virtual FFootprint GetFootprint() const override;

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
	/** In the committed last stretch of a lunge (Looter.Unpaid.LungeCommit): the hit slow is held off it. */
	bool IsLungeCommitted() const { return bLunging && bLungeCommitted; }
	/** Its hit slows come at most once every Looter.Unpaid.HitSlowGap seconds (with its kit). */
	virtual float GetHitSlowGap() const override;
	/** With its kit, shot at from just past its ground's edge it fights there (Looter.Unpaid.PressReach, .PressSeconds). */
	virtual float GetPressReach() const override;
	virtual float GetPressSeconds() const override;
	/** Whether it owes a phase-step in (pressed by a hit from past its edge), taken once its cooldown allows. */
	bool OwesPressStep() const { return bPressStepOwed; }
	/** Waiting its turn, whether the wait can't end where it stands: its target past its own edge, or out of its sight (one trace). */
	bool WaitGoesNowhere(const APawn& Victim) const;
	/**
	 * Why a phase-step toward its target finds nowhere now: each candidate spot's first refusal counted (off its ground, no
	 * ground, steep or an obstacle's top, no room, no sight of the target, a safe zone or past the playable area, another level),
	 * as one line. Looter.Unpaid.TryStep prints it; a failed step logs it (Verbose).
	 */
	FString DescribePhaseRefusals() const;

	// --- Its fighting kit (UnpaidCreatureMoves.cpp, UnpaidCreatureRing.cpp; FUnpaidFightTuning) ---

	/** A plain Unpaid (its class or a data-only child), not a boss nor a boss's add, with Looter.Unpaid.Kit on. */
	bool HasFightKit() const;
	/** The attack it chose as its wind-up began (None outside one), and what it does on the ring between turns. */
	EUnpaidMove GetKitMove() const { return Kit.Move; }
	EUnpaidAction GetKitAction() const { return Kit.Action; }
	/** Starts a cinder's tell or a rally scream now at its target, as its ring would (false when it can't). For the tests and dev tools. */
	bool StartKitAction(EUnpaidAction Action);
	/** Throws a cinder at its target now, led at where they'll be: the end of a cinder's tell. True when one left its hand. */
	bool ThrowCinder();

	// --- Its arms (UnpaidCreatureArms.cpp) ---

	/**
	 * How far its arms reach into its own torso past where they sit in the rest pose, as it's posed now (cm, the deepest
	 * point of either arm's hit hull in the pelvis's, spine's, chest's or shroud top's): 0 when they keep out, as they
	 * should. Measured on the last pose solved, so a body built on its rig (Abel) is measured in its own pose. For the
	 * tests and Looter.CastShots; 0 without the model.
	 */
	float GetArmIntrusion() const;

	// --- Phase-steps ---

	/** Whether it's fading out or back in. */
	bool IsPhasing() const;

	/** How far it's dissolved now: 0 solid to 1 gone (its material's Phase). */
	float GetPhase() const { return PhaseAmount; }

	/**
	 * Fades it in where it stands, as the end of a phase-step does (its shroud's ends first, shot through until it's mostly
	 * there), coming up out of the ground (RiseSeconds) clawing, dirt thrown up: an add rising through the boards (Abel's), an
	 * ambush's dead, one a toll raises. Nothing while it's dead.
	 */
	void RiseIn();
	static constexpr float RiseSeconds = 0.9f;
	bool IsRising() const { return RiseTime >= 0.f; }

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
	/** With its kit, a lunge starts from farther out (Looter.Unpaid.LungeStart). */
	virtual float GetAttackStartRange() const override;
	// --- The notice beat, its entrance, its ash (UnpaidCreatureMoods.cpp) ---
	virtual void OnNoticed() override;
	virtual void PlayEntrance() override;
	/** Ash motes off its body while it dissolves dying (PhaseAmount between 0 and 1). */
	void ShedDeathAsh(float DeltaSeconds);
	/** Its notice beat's sound: the Unpaid's coal flaring; its kinds' own (a miner's pick lowered, the Bell-Ringer's bell). */
	FName NoticeCue;

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
	/** Commits the lunge (its slow suspended) or lets it go again (the slow holds once more, if any is left). */
	void CommitLunge(bool bOn);

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
	virtual void OnPressedByHit(const APawn& By) override;
	/** Where a step lands now: the rules' first spot the level allows (ground, room, sight of its target, safe zones). */
	bool FindPhaseSpot(FVector& OutFeet) const;
	/** Whether it may come back at Candidate: walkable ground near it, room, its target in sight, no safe zone (the ground: OutGround). */
	bool CanStandAt(const FVector& Candidate, FVector& OutGround, int32* OutRefusal = nullptr) const;
	/** Starts a step to Feet now: it fades out here and comes back there. */
	void StartStepTo(const FVector& Feet);
	/** Unseen at last: moves to the spot it chose and starts coming back. */
	void Arrive();

	// --- Its fighting kit (UnpaidCreatureMoves.cpp; on the ring, the cinder and the rally: UnpaidCreatureRing.cpp) ---
	void TickFightKit(float DeltaSeconds);
	/** As its wind-up begins: the rake or the lunge, and their timings. */
	void ChooseKitMove();
	/** The rake after its wind-up: the drift in, the second swipe. */
	void TickRake();
	/** One swipe of Arm (0 left, 1 right) across its front: whether it caught its target. */
	bool Swipe(int32 Arm);
	void TickKitAction(float DeltaSeconds, const APawn& Victim);
	/** Its cinder or rally ended, or called off (bCalledOff: a cinder's tell put out). */
	void EndKitAction(bool bCalledOff);
	/** Waiting on the ring: a rally, or a cinder, if the moment's right. */
	void ThinkOnRing(const APawn& Victim);
	/** Its turn, in front of the player's eyes: fades round to their side (true when it went). */
	bool TryFlank(const APawn& Victim);
	/** Tells the fight director how much faster than its chase it comes now (closing in, its pack's frenzy). */
	void UpdateChaseBoost(const APawn* Victim, bool bFrenzy);
	/** Whether it rose as a boss's add (looked up once a life). */
	bool IsBossAdd();

	// --- The body (UnpaidCreatureRig.cpp) ---
	// Its parts' structs live in UnpaidBodyParts.h (the ~500-line rule, 2026-10-28).
	using FArm = UnpaidBody::FArm;
	using FPoseChannels = UnpaidBody::FPoseChannels;
	using FArmSway = UnpaidBody::FArmSway;
	using FArmGuardPoint = UnpaidBody::FArmGuardPoint;
	using FTorsoGuard = UnpaidBody::FTorsoGuard;

	/** Reads its rig from the skeleton; false without the model or the bones it needs (it then stays at rest). */
	bool SetupRig();
	/** Works out this frame's pose and hands it to the animation (GetBonePose). */
	void AnimateBody(float DeltaSeconds);
	/** Where each channel heads now, from what the brain is doing (Speed: its drift against its chase speed; Time: its clock). UnpaidCreatureMoods.cpp. */
	FPoseChannels PoseTargets(float Speed, float Time) const;
	/** Its rise, glances, notice flare and the dust its shroud drags (UnpaidCreatureMoods.cpp). */
	void TickReactions(float DeltaSeconds);
	/** How far under its hang it is as it rises out of the ground (cm, negative), 0 once up. */
	float RiseDrop() const;
	/** Works the bones' poses out from their turns, parents first. */
	void SolvePose();

	// --- The arms (UnpaidCreatureArms.cpp) ---
	/** Reads the points of the arms' hit hulls that keep out of the torso, and the torso's hulls, from the model's physics asset. */
	void SetupArmGuard();
	/**
	 * Turns the arms' bones for this frame (after the trunk's): their channels, the drift, the sway and the keep-out. Speed is
	 * its drift against its chase speed, LocalVelocity its velocity in its own frame, YawRate its turn (degrees a second).
	 */
	void PoseArms(float DeltaSeconds, float Time, float Speed, const FVector& LocalVelocity, float YawRate);
	/** Steps the arms' sway springs on by DeltaSeconds under the body's acceleration (cm/s^2, its own frame) and turn. */
	void StepArmSway(float DeltaSeconds, const FVector& LocalAcceleration, float YawRate);
	/** Turns one arm's bones for these angles (degrees: swung back, out from the body, the elbow's bend, the wrist) and finger curl. */
	void TurnArm(const FArm& Arm, float Swing, float Out, float Bend, float Wrist, float Curl);
	/** How far one arm (0 left, 1 right) reaches into the torso past its rest, on the pose as solved now (cm), and how far from the shoulder that deepest point is (cm, across the body). */
	float ArmIntrusion(int32 Side, float* OutLever = nullptr) const;
	/** Sets the arms' sway swinging (degrees a second, per side: back, and out from the body), for a hit or a twitch. */
	void KickArm(int32 Side, float SwingSpeed, float OutSpeed);
	/** A hit (its jolt along FlinchAway): the flinch kicked, both arms flung the other way, a crit harder, all Strength times (Gunfeel). */
	void ReactToHit(bool bCritical, float Strength = 1.f);
	/** The flinch follows its fading kick on a stiff spring, so a hit jolts the body without a one-frame jump. */
	void StepFlinch(float DeltaSeconds);

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
	/** How far it banks into its turn now (degrees, eased toward what its turn rate asks). */
	float Bank = 0.f;
	/** Each one moves in its own time, so a crowd doesn't bob in step. */
	float PoseSeed = 0.f;
	/** A hit's flinch (about 0 to 1, its speed, and the fading kick it follows: StepFlinch) and which way it pushed (the body's frame). */
	float Flinch = 0.f;
	float FlinchSpeed = 0.f;
	float FlinchKick = 0.f;
	FVector FlinchAway = FVector::ZeroVector;

	// The arms
	FArmSway ArmSway[2];
	/** The torso's hit hulls the arms keep out of, and the arms' guard points (UnpaidCreatureArms.cpp). */
	FCreatureBodyHulls TorsoHulls;
	TArray<FTorsoGuard> TorsoGuards;
	TArray<FArmGuardPoint> ArmGuards[2];
	/** The last frame's velocity in its own frame, for its acceleration (the sway). */
	FVector LastLocalVelocity = FVector::ZeroVector;
	/** Seconds until an arm twitches next, while it hangs about. */
	float NextTwitch = 4.f;

	/** How far the shroud lifts (degrees) off ground that rises behind it, eased. */
	float ShroudLift = 0.f;

	// Reactions: seconds into its rise (below 0: not rising), a glance's turn and time left, the next glance, the shroud's
	// next puff of dust, ash owed from earlier frames.
	float RiseTime = -1.f;
	float GlanceYaw = 0.f;
	float GlanceLeft = 0.f;
	float NextGlance = -1.f;
	float ShroudDustClock = 0.f;
	float AshOwed = -1.f;

	// Rank
	FUnpaidRankTraits Traits;
	bool bTimingCaptured = false;
	float BaseWindup = 0.6f;

	// Its fighting kit: its move, its actions on the ring, its clocks.
	FUnpaidKitState Kit;

	// Attack
	bool bLunging = false;
	bool bLungeLanded = false;
	bool bLungeCommitted = false;
	FVector LungeDirection = FVector::ForwardVector;
	float LungeLeft = 0.f;
	float LungeTime = 0.f;
	float SinceSlowingShriek = 1000.f;

	// Phase-steps
	EPhaseState PhaseState = EPhaseState::None;
	float PhaseTime = 0.f;
	FVector PhaseDestination = FVector::ZeroVector;
	float SinceLastStep = 1000.f;
	/** Pressed by a hit from past its edge: its next step comes as soon as its cooldown allows, stuck or not. */
	bool bPressStepOwed = false;
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
