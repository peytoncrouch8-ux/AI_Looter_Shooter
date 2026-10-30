#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTrait.h"
#include "Creatures/StoneSentinelRules.h"
#include "StoneSentinelCreature.generated.h"

class AStaticMeshActor;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Where a Stone Sentinel is in its life beyond the brain's states (AStoneSentinelCreature::GetPhase). */
enum class EStoneSentinelPhase : uint8
{
	/** A statue on its plinth: held back (passive), no tag, no traits showing; a player near or a shot wakes it. */
	Dormant,
	/** Cracking loose and grinding round to face them, its brain held (WakeSeconds). */
	Waking,
	/** A creature like any other: it hunts, walks, slams. */
	Awake,
	/** Calm by its plinth again: it climbs back on and stiffens into a statue (SettleSeconds). */
	Settling,
	/** Dead: it sags, cracks through and bursts into stone (CrumbleSeconds), then nothing is left. */
	Crumbling,
};

/**
 * The Stone Sentinel (the Elements matter job, Docs/Work/2026-10-29-ElementsMatter/CONTRACT.md): an old grave statue, a
 * hooded mourner, that has stood over the rows so long something got into the stone. It stands on its plinth as a statue
 * (held passive: no tag, no traits, nothing on it to give it away) until a player comes within WakeDistance or shoots it;
 * then it cracks loose (dust, stone chips, its Wake cue), grinds round to face them and steps off its plinth. Slow
 * (FeltChaseSpeed), knock-proof (nothing launches it: LaunchCharacter does nothing, a knock is undone, a shove is clamped
 * out), stone (Defense 0.25 on top of its tier's; body Stone: weak to Wardbreak and Thunder, resists Cinder, immune to
 * Rot; never Touched), three dead miners' health. A Thunder jolt staggers it as any creature (the elements' rules).
 *
 *  - The slam (StoneSentinelCreatureSlam.cpp): it heaves both fists overhead through a 1.1 s wind-up (its WindUp cue), its
 *    aim set for the last 0.45 s of it, and brings them down on whoever is within 2.8 m in front: a crack runs out across
 *    the ground (AGroundCrack), grit and stone fly, the view shakes (Slam). Bent over its fists a moment after: the window.
 *  - Steps: each stride a heavy footfall (Step), grit and a faint jolt for a player near.
 *  - Hits chip it (Chip): stone pieces off where the shot landed, more for a crit; the head (SOCKET_Head, the head bone)
 *    is the crit.
 *  - Death: it sags and cracks through, then bursts into stone (Crumble): its body gone, pieces and dust where it stood.
 *  - Calm again by its plinth (RestAfter), it climbs back on and is a statue once more, whole.
 *
 * Body: SK_StoneSentinel (lane E's model: root, pelvis, spine_01, spine_02, head, the arms and legs; sockets SOCKET_Head,
 * SOCKET_FistL, SOCKET_FistR; its physics asset the hit zones), posed by code (StoneSentinelCreaturePose.cpp: its rest pose
 * is the statue). Until it's imported, a stand-in of the engine's cylinder and sphere in granite takes the shots, so it can be
 * fought and tested; the missing model is said once in the log. Its plinth (SM_StoneSentinelPlinth, a granite block until
 * that's imported) is an actor of its own it raises under itself as play begins, so it stays when the sentinel walks off,
 * dies or is hidden. Stone pieces are the Gravemother's headstone shards (SM_GravemotherStoneShard_1-5) thrown in code
 * (StoneSentinelCreatureChips.cpp), with UGravemotherFX's stone dust and chips.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AStoneSentinelCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	AStoneSentinelCreature();

	virtual void Tick(float DeltaSeconds) override;

	/** The head is the crit: the head bone, or a hit within HeadCritRadius of SOCKET_Head (the stand-in's head ball). */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	/** Knock-proof: nothing launches a statue (a knock, a shotgun's throw, a blast, a melee shove). */
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride) override;

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

	// --- Settings ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone Sentinel")
	FStoneSentinelTuning Tuning;

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
	/** Its stone's defense with its tier's, kept so whatever resets the tier's (a level change) is put right. */
	void ApplyStoneDefense();
	/** The ground under Where (its own level), or Where itself where there's none (a test level). */
	FVector GroundUnder(const FVector& Where) const;
	/** Its feet: the ground under its middle, by its capsule. */
	FVector GetFeet() const;
	/** Undoes a knock begun on it (knock-proof) and clamps out a shove (anything faster than it walks). */
	void StayPut();

	// --- Its life (StoneSentinelCreatureWake.cpp) ---
	void SetPhase(EStoneSentinelPhase NewPhase);
	/** A statue: held back, no tag, no traits, its pose the model's rest. */
	void BecomeStatue();
	/** A few looks a second: a player near enough wakes it. */
	void TickDormant(float DeltaSeconds);
	void TickWaking(float DeltaSeconds);
	/** Calm by its plinth: back to sleep (Settling). */
	void TickAwake(float DeltaSeconds);
	void TickSettling(float DeltaSeconds);
	void TickCrumbling(float DeltaSeconds);
	/** Its traits on (awake) or off (a statue), as its tier rolled them. */
	void ShowTraits(bool bShown);

	// --- The slam and its tread (StoneSentinelCreatureSlam.cpp) ---
	/** Nothing solid between it and Victim (a wall, a crypt's side): the blow never lands through one. */
	bool IsSlamClear(const APawn& Victim) const;
	/** Its gait: the stride's phase on, a footfall at each half (Step). */
	void TickGait(float DeltaSeconds);
	void Footfall(bool bLeft);

	// --- Its pose (StoneSentinelCreaturePose.cpp) ---
	/** Reads its rig (bone indices, rest pose, limb directions) from the model; false without one. */
	bool SetupRig();
	/** This frame's pose: the statue's rest blended to its awake pose by the waking, the slam, the gait, a flinch, the crumble. */
	void AnimateBody(float DeltaSeconds);
	/** The pose it wants now (arms and trunk) before the gait and flinches. */
	FStoneSentinelPose WantedPose() const;

	// --- Stone pieces (StoneSentinelCreatureChips.cpp) ---
	/** The shard meshes (or the stand-in's block) as instanced sets, made once. */
	void SetupChips();
	/** Count pieces thrown from At along Up and outward at about Speed (cm/s), each SizeMin to SizeMax cm across. */
	void ThrowChips(const FVector& At, const FVector& Up, int32 Count, float Speed, float SizeMin, float SizeMax);
	void TickChips(float DeltaSeconds);
	void DrawChips();
	/** A puff of dust round At (UGravemotherFX): the stone's, or the ground's own (bFloor: kicked up), Count puffs Size cm across. */
	void StoneDust(const FVector& At, int32 Count, float Size, float Spread, bool bFloor = false) const;
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
	TArray<ECreatureTrait> OwnTraits;
	/** No model: the stand-in stands in (said once). */
	bool bStandIn = false;

	// The gait
	float GaitPhase = 0.f;
	/** How much it's walking now (0-1, eased). */
	float Walking = 0.f;
	/** A hit's jolt (degrees, pitch and roll), easing out. */
	FVector2D Jolt = FVector2D::ZeroVector;
	double LastChipSound = -10.0;

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
