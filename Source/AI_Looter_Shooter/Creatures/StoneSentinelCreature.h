#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTrait.h"
#include "Creatures/StoneSentinelMoveRules.h"
#include "Creatures/StoneSentinelRules.h"
#include "StoneSentinelCreature.generated.h"

class AController;
class AGroundCrack;
class AStaticMeshActor;
class AStoneSentinelBoulder;
class AStoneSentinelShockwave;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Where a Stone Sentinel is in its life beyond the brain's states (AStoneSentinelCreature::GetPhase). */
enum class EStoneSentinelPhase : uint8
{
	/** A statue on its plinth: held back (passive), no tag, no traits showing; a player near or a shot wakes it. */
	Dormant,
	/** Cracking loose, stretching free and grinding round to face them, its brain held (WakeSeconds). */
	Waking,
	/** A creature like any other: it hunts, walks, slams, throws, sends its shockwave. */
	Awake,
	/** Calm by its plinth again: it climbs back on and stiffens into a statue (SettleSeconds). */
	Settling,
	/** Dead: it sags, cracks through and bursts into stone (CrumbleSeconds), then nothing is left. */
	Crumbling,
};

/**
 * The Stone Sentinel (the Elements matter job, Docs/Work/2026-10-29-ElementsMatter/CONTRACT.md; made a threat 2026-10-31,
 * Docs/Archive/Work/2026-10-31-Creatures/CONTRACT.md): an old grave statue, a hooded mourner, that has stood over the rows so long
 * something got into the stone. It stands on its plinth as a statue (held passive: no tag, no traits, nothing on it to give
 * it away) until a player comes within WakeDistance or shoots it; then it cracks loose (dust, stone chips, its Wake cue),
 * stretches free of the years, drops into its stance with a thud and steps off its plinth. Slow (FeltChaseSpeed),
 * knock-proof (nothing launches it: LaunchCharacter does nothing, a knock is undone, a shove is clamped out), stone (Defense
 * 0.25 on top of its tier's; body Stone: weak to Wardbreak and Thunder, resists Cinder, immune to Rot; never Touched), six
 * dead miners' health. A Thunder jolt staggers it as any creature (the elements' rules).
 *
 * It presses at every distance (StoneSentinelMoves::Choose, StoneSentinelCreatureMoves.cpp):
 *  - The slam, up close (StoneSentinelCreatureSlam.cpp): both fists heaved overhead through a 1.1 s wind-up, down on whoever
 *    is within 2.8 m in front: a crack runs out, grit and stone fly, the view shakes.
 *  - The shockwave, near (StoneSentinelCreatureShock.cpp): it rears with its arms spread wide while the ground cracks round
 *    its feet, then drives both fists into the ground: a ring of rock spikes runs out to 9 m (AStoneSentinelShockwave), with
 *    gaps never where its target stood. Jump it, step round into a gap, or be out past it. Hugged, or after two slams, it
 *    throws you off with one.
 *  - The boulder, out in the open (StoneSentinelCreatureBoulder.cpp): bent to the ground it tears a rock out of it (the
 *    ground cracks, earth pours off), lifts it overhead, winds back and hurls it on an arc at where its target will be
 *    (AStoneSentinelBoulder): a shadow and a ring mark where it lands; it bursts there, hurting all round. The rock in its
 *    grip can be shot: a few hits break it over its own head, hurting it, and it reels, open.
 *  - Open (the rhythm to learn): after a slam, a shockwave or a rock burst on it, it's stuck a moment bent over its fists or
 *    reeling, the soul-light pouring out of the cracks in its back (its Exposed cue): its stone's defense is gone and its
 *    back and shoulders are critical as its head always is.
 *  - Steps: each stride a heavy footfall (Step), grit, pebbles off its knees, a jolt on a near player's view. Pebbles and a
 *    breath of dust trickle off its joints as it moves, more as it heaves.
 *  - Hits chip it (Chip): stone pieces off where the shot landed, more for a crit; the head (SOCKET_Head, the head bone)
 *    is the crit.
 *  - Death: it sags and cracks through, then bursts into stone (Crumble): its body gone, pieces and dust where it stood.
 *  - Calm again by its plinth (RestAfter), it climbs back on and is a statue once more, whole.
 *
 * Body: SK_StoneSentinel (root, pelvis, spine_01, spine_02, head, the arms and legs; sockets SOCKET_Head, SOCKET_FistL,
 * SOCKET_FistR; its physics asset the hit zones), posed by code (StoneSentinelCreaturePose.cpp, the curves
 * StoneSentinelPoses.cpp: its rest pose is the statue). Until it's imported, a stand-in of the engine's cylinder and sphere
 * in granite takes the shots, so it can be fought and tested; the missing model is said once in the log. Its plinth
 * (SM_StoneSentinelPlinth, a granite block until that's imported) is an actor of its own it raises under itself as play
 * begins, so it stays when the sentinel walks off, dies or is hidden. Stone pieces are the Gravemother's headstone shards
 * (SM_GravemotherStoneShard_1-5) thrown in code (StoneSentinelCreatureChips.cpp), with UGravemotherFX's stone dust and chips.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AStoneSentinelCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	AStoneSentinelCreature();

	virtual void Tick(float DeltaSeconds) override;

	/** The head is the crit: the head bone, or a hit within HeadCritRadius of SOCKET_Head; while it's open, its back too. */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	/** Knock-proof: nothing launches a statue (a knock, a shotgun's throw, a blast, a melee shove). */
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride) override;

	/** Its boulder takes a ranged turn (spread from other throwers'), its slam and shockwave melee ones. */
	virtual EFightRole GetAttackTokenRole() const override;

	// --- Its models (where the import puts them: /Game/Art/Creatures) ---

	static const TCHAR* const ModelPath;
	static const TCHAR* const PlinthPath;
	/** Its attach points: small bones named SOCKET_Head, SOCKET_FistL, SOCKET_FistR (the mesh's socket lookups find bones). */
	static const FName HeadSocket;
	static const FName FistLSocket;
	static const FName FistRSocket;
	/** What its plinth carries (besides Obstacle), so a sentinel placed or spawned on one stands on it rather than raising another. */
	static const FName PlinthTag;

	// --- Its life (StoneSentinelCreatureWake.cpp) ---

	EStoneSentinelPhase GetPhase() const { return Phase; }
	float GetPhaseTime() const { return PhaseTime; }
	/** A statue still: anything that shows hostiles (the minimap, a power's aim) may pass it by. */
	bool IsDormant() const { return Phase == EStoneSentinelPhase::Dormant; }

	/** Wakes it now (By: whom it turns on, or nobody: a shot's hunt follows from the hit). Nothing unless it's a statue. */
	void Wake(APawn* By);

	/** Its plinth (raised as play begins, or one it found under it; null with bBringsPlinth off or before play). */
	AStaticMeshActor* GetPlinth() const;

	/** Its traits as rolled for its tier, Fleet taken out: what it shows once awake (none while a statue). */
	const TArray<ECreatureTrait>& GetOwnTraits() const { return OwnTraits; }

	/** Where its fists come down now (world): between its fist sockets, or SlamImpactAhead in front. */
	FVector GetFistsPoint() const;

	/** The pose its arms and trunk take now (StoneSentinelRules; the tests). */
	const FStoneSentinelPose& GetPoseNow() const { return PoseNow; }

	/** Stone pieces in the air or lying now (StoneSentinelCreatureChips.cpp; the tests and the caps). */
	int32 NumChips() const { return Chips.Num(); }

	// --- Its attacks (StoneSentinelCreatureMoves.cpp) ---

	/** The attack it's making (or made last), and the one it would make at its target this moment (None: it closes in). */
	EStoneSentinelMove GetMove() const { return Move; }
	EStoneSentinelMove PlanMove() const;

	/** Seconds until its shockwave and its boulder are ready again (0: ready). */
	float GetShockReadyIn() const { return FMath::Max(ShockReadyIn, 0.f); }
	float GetBoulderReadyIn() const { return FMath::Max(BoulderReadyIn, 0.f); }
	/** Readies both at once (the console, the tests). */
	void ReadyMoves() { ShockReadyIn = 0.f; BoulderReadyIn = 0.f; }

	/** Open now (after a heavy blow or a burst rock): its stone's guard down, its back critical. Seconds left of it. */
	bool IsExposed() const { return ExposedLeft > 0.f; }
	float GetExposedLeft() const { return FMath::Max(ExposedLeft, 0.f); }

	/** The rock in its grip or in flight from its last throw (null when there's none). */
	AStoneSentinelBoulder* GetBoulder() const;

	/** Where the rock sits in its grip now (world): on its fists, or where its stand-in's hands would be. */
	FVector GetBoulderHoldPoint() const;

	/**
	 * The rock in its grip was shot to pieces (AStoneSentinelBoulder, By who broke it), bursting over its head at Where:
	 * stone rains on it, it takes CounterShare of its health (By credited), its throw is cut off and it reels, open.
	 */
	void CounterBoulder(AController* By, const FVector& Where);

	/**
	 * Count stone pieces thrown from At (its own instanced chips, reused: nothing spawned), short-lived: its boulder's burst
	 * (StoneSentinelBoulderLanding.cpp) while it stands.
	 */
	void ThrowStones(const FVector& At, const FVector& Up, int32 Count, float Speed, float SizeMin, float SizeMax);

	// --- Settings ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone Sentinel")
	FStoneSentinelTuning Tuning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone Sentinel")
	FSentinelBoulderTuning BoulderTuning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone Sentinel")
	FSentinelShockTuning ShockTuning;

	/** It raises its own plinth under itself as play begins (off: none; one tagged PlinthTag already under it is used either way). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone Sentinel")
	bool bBringsPlinth = true;

	/** A hit this near SOCKET_Head (cm, at size 1) is on its head: the crit, whatever hull it met. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone Sentinel", meta = (ClampMin = "0", Units = "cm"))
	float HeadCritRadius = 22.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnAttackCalledOff() override;
	virtual bool CanStartAttack() const override;
	virtual float GetAttackStartRange() const override;
	virtual bool CanReachHeight(float Rise) const override;
	virtual bool TracksTargetInWindup() const override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnRankChanged() override;
	virtual void PlayEntrance() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;

private:
	// --- Body (StoneSentinelCreature.cpp) ---
	/** The stand-in's shapes in granite and its shot zones, when SK_StoneSentinel isn't imported. */
	void SetupStandIn();
	/** Raises its plinth on the ground under it and stands it on top (before its base's start settles it). */
	void RaisePlinth();
	/** Its stone's defense with its tier's, kept so whatever resets the tier's (a level change) is put right; less while open. */
	void ApplyStoneDefense();
	/** The ground under Where (its own level), or Where itself where there's none (a test level). */
	FVector GroundUnder(const FVector& Where) const;
	/** Its feet: the ground under its middle, by its capsule. */
	FVector GetFeet() const;
	/** Undoes a knock begun on it (knock-proof) and clamps out a shove (anything faster than it walks). */
	void StayPut();
	/** A bone's place now (world), or Fallback without the rig. */
	FVector BoneSpot(int32 BoneIndex, const FVector& Fallback) const;

	// --- Its life (StoneSentinelCreatureWake.cpp) ---
	void SetPhase(EStoneSentinelPhase NewPhase);
	/** A statue: held back, no tag, no traits, its pose the model's rest. */
	void BecomeStatue();
	/** A few looks a second: a player near enough wakes it. */
	void TickDormant(float DeltaSeconds);
	void TickWaking(float DeltaSeconds);
	/** Its drop into its stance as its waking ends: the thud, dust round its feet, a jolt for those near. */
	void SettleIntoStance();
	/** Calm by its plinth: back to sleep (Settling). */
	void TickAwake(float DeltaSeconds);
	void TickSettling(float DeltaSeconds);
	void TickCrumbling(float DeltaSeconds);
	/** Its traits on (awake) or off (a statue), as its tier rolled them. */
	void ShowTraits(bool bShown);

	// --- Its attacks (StoneSentinelCreatureMoves.cpp) ---
	/** Its moves' clocks, and the move under way's own beats (the tear, the tell). */
	void TickMoves(float DeltaSeconds);
	/** Whether its lob's way to Victim is clear (looked at a few times a second). */
	bool IsLobClear(const APawn& Victim) const;
	/** It's open for Seconds (at least what's left): its Exposed cue the first moment. */
	void SetExposed(float Seconds);
	/** While open: soul-light embers pouring out of its back, grit off it (StoneSentinelCreatureHurt.cpp). */
	void TickExposed(float DeltaSeconds);

	// --- The slam and its tread (StoneSentinelCreatureSlam.cpp) ---
	void BeginSlam();
	void SlamStrike();
	/** Nothing solid between it and Victim (a wall, a crypt's side): the blow never lands through one. */
	bool IsSlamClear(const APawn& Victim) const;
	/** Its gait: the stride's phase on, a footfall at each half (Step). */
	void TickGait(float DeltaSeconds);
	void Footfall(bool bLeft);

	// --- The shockwave (StoneSentinelCreatureShock.cpp) ---
	void BeginShock();
	/** The tell: the ground groaning round its feet, grit off it, a tremor for those near. */
	void TickShockTell(float DeltaSeconds);
	void ShockStrike();
	/** The tell's ring of cracks closed (struck, called off, dead). */
	void EndShockTell();

	// --- The boulder (StoneSentinelCreatureBoulder.cpp) ---
	void BeginThrow();
	/** The throw's beats in its wind-up: the rock torn up at TearAt, carried up in its grip after. */
	void TickThrow(float DeltaSeconds);
	void TearBoulder();
	void ReleaseBoulder();
	/** The rock let go of without a throw (called off, dead): it falls and bursts harmlessly. */
	void DropBoulder();

	// --- Its pose (StoneSentinelCreaturePose.cpp) ---
	/** Reads its rig (bone indices, rest pose, limb directions) from the model; false without one. */
	bool SetupRig();
	/** This frame's pose: the statue's rest blended to its awake pose by the waking, the moves, the gait, a flinch, the crumble. */
	void AnimateBody(float DeltaSeconds);
	/** The pose it wants now (arms and trunk) before the gait and flinches. */
	FStoneSentinelPose WantedPose() const;

	// --- Stone pieces (StoneSentinelCreatureChips.cpp) ---
	/** The shard meshes (or the stand-in's block) as instanced sets, made once. */
	void SetupChips();
	/** Count pieces thrown from At along Up and outward at about Speed (cm/s), each SizeMin to SizeMax cm across, lying LifeMin to
	 *  LifeMax seconds in all (short: opaque and instanced, but a burst near the lens should clear fast). */
	void ThrowChips(const FVector& At, const FVector& Up, int32 Count, float Speed, float SizeMin, float SizeMax, float LifeMin = 1.2f,
		float LifeMax = 2.f);
	void TickChips(float DeltaSeconds);
	void DrawChips();
	/** A puff of dust round At (UGravemotherFX): the stone's, or the ground's own (bFloor: kicked up), Count puffs Size cm across. */
	void StoneDust(const FVector& At, int32 Count, float Size, float Spread, bool bFloor = false) const;
	/** Pebbles and grit trickling off its joints at the rate its doings call for (Tuning's Grit*). */
	void TickGrit(float DeltaSeconds);
	/** Count pebbles off a joint (a shoulder, an elbow, its neck, a knee: Joint, or one picked by chance), a breath of dust. */
	void ShedGrit(int32 Count, int32 Joint = INDEX_NONE);
	/** Its body breaks up: hidden, pieces and dust where it stood. */
	void Crumble();

	// --- Components and assets ---

	/** Stand-in body and head until SK_StoneSentinel is imported (no mesh, no collision once it is). */
	UPROPERTY(VisibleAnywhere, Category = "Stone Sentinel")
	TObjectPtr<UStaticMeshComponent> StandInBody;

	UPROPERTY(VisibleAnywhere, Category = "Stone Sentinel")
	TObjectPtr<UStaticMeshComponent> StandInHead;

	UPROPERTY(EditDefaultsOnly, Category = "Stone Sentinel")
	TObjectPtr<UStaticMesh> PlinthMesh;

	/** Its plinth: one it raised (removed with it, bOwnsPlinth) or one it found standing under it (left where it is). */
	TWeakObjectPtr<AStaticMeshActor> Plinth;
	bool bOwnsPlinth = false;

	/** The stone pieces' sets (one a shard mesh; the stand-in's block where none is imported). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> ChipSets;

	// --- State ---

	EStoneSentinelPhase Phase = EStoneSentinelPhase::Dormant;
	float PhaseTime = 0.f;
	float LookClock = 0.f;
	/** Seconds calm (awake, no target) so far. */
	float CalmTime = 0.f;
	/** Settling: where it climbs from, and its turn then. */
	FVector SettleFrom = FVector::ZeroVector;
	float SettleFromYaw = 0.f;
	bool bCrumbled = false;
	/** Its waking's drop into its stance has come (once a waking). */
	bool bStanceTaken = false;
	TArray<ECreatureTrait> OwnTraits;
	/** No model: the stand-in stands in (said once). */
	bool bStandIn = false;

	// Its attacks
	EStoneSentinelMove Move = EStoneSentinelMove::None;
	/** Its last attack and how many of it in a row (StoneSentinelMoves::Choose breaks a run of slams). */
	EStoneSentinelMove LastMove = EStoneSentinelMove::None;
	int32 MoveStreak = 0;
	float ShockReadyIn = 0.f;
	float BoulderReadyIn = 0.f;
	/** It had a target last frame (losing one sets the first delays again). */
	bool bHadTarget = false;
	/** Its lob's way, as last looked at (IsLobClear), and when (Clock). */
	mutable float LobCheckedAt = -1.f;
	mutable bool bLobClear = false;
	/** The throw: the rock torn up, thrown, burst in its grip; where it was torn from, sunk. */
	bool bBoulderTorn = false;
	bool bBoulderThrown = false;
	bool bBoulderCountered = false;
	FVector TearSpot = FVector::ZeroVector;
	float BoulderYaw = 0.f;
	TWeakObjectPtr<AStoneSentinelBoulder> HeldBoulder;
	/** Its rock and its shockwave, kept to use again (their meshes, instances and decals reused: nothing spawned per attack). */
	TWeakObjectPtr<AStoneSentinelBoulder> KeptBoulder;
	TWeakObjectPtr<AStoneSentinelShockwave> KeptWave;
	/** The shockwave's tell: its ring of cracks, and the tremor's clock. */
	TWeakObjectPtr<AGroundCrack> ShockTellCrack;
	float TremorClock = 0.f;
	/** Open: seconds left, and its embers' clock. */
	float ExposedLeft = 0.f;
	float EmberClock = 0.f;

	// The gait
	float GaitPhase = 0.f;
	/** How much it's walking now (0-1, eased). */
	float Walking = 0.f;
	/** A hit's jolt (degrees, pitch and roll), easing out. */
	FVector2D Jolt = FVector2D::ZeroVector;
	double LastChipSound = -10.0;
	/** Pebbles owed (TickGrit's rate builds it up, one shed per whole one), and the last pebbles' sound. */
	float GritOwed = 0.f;
	double LastPebbleSound = -10.0;

	// The pose
	FStoneSentinelPose PoseNow;
	/** How much of its awake pose shows over the statue's (0-1). */
	float AwakeWeight = 0.f;
	float HeadYaw = 0.f;
	float Clock = 0.f;

	/** Its rig, read from the model once (StoneSentinelCreaturePose.cpp). */
	struct FRig
	{
		bool bReady = false;
		TArray<FTransform> RefLocal;
		TArray<int32> Parents;
		/** Scratch: every bone's pose this frame (component space). */
		TArray<FTransform> Posed;
		int32 Root = INDEX_NONE;
		int32 Pelvis = INDEX_NONE;
		int32 Spine1 = INDEX_NONE;
		int32 Spine2 = INDEX_NONE;
		int32 Head = INDEX_NONE;
		/** Left 0, right 1. */
		int32 Upper[2] = { INDEX_NONE, INDEX_NONE };
		int32 Lower[2] = { INDEX_NONE, INDEX_NONE };
		int32 Hand[2] = { INDEX_NONE, INDEX_NONE };
		int32 Thigh[2] = { INDEX_NONE, INDEX_NONE };
		int32 Calf[2] = { INDEX_NONE, INDEX_NONE };
		int32 Foot[2] = { INDEX_NONE, INDEX_NONE };
		/** Each limb bone's way to the next one down at rest, in the bone's own axes. */
		FVector UpperWay[2];
		FVector LowerWay[2];
		FVector ThighWay[2];
		FVector CalfWay[2];
		/** The bones it hands the anim instance, by name with their index. */
		TArray<TPair<FName, int32>> Emitted;
	};
	FRig Rig;

	/** A stone piece in flight or lying (StoneSentinelCreatureChips.cpp). */
	struct FChip
	{
		int32 Set = 0;
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector Spin = FVector::ZeroVector;
		/** Its scale on its mesh. */
		float Scale = 1.f;
		float Age = 0.f;
		float Life = 2.5f;
		float FloorZ = -1.0e9f;
		uint8 Bounces = 0;
		bool bResting = false;
	};
	TArray<FChip> Chips;
	/** Each set's mesh middle (the shards sit round the headstone's pivot) and size across (cm). */
	TArray<FVector> ChipCenters;
	TArray<float> ChipSizes;
	bool bChipsReady = false;
	bool bChipsDrawn = false;
};
