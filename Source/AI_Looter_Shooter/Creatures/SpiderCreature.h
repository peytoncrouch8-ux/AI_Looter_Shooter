#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "SpiderCreature.generated.h"

/**
 * Human-sized brown hunting spider (wolf-spider look, not a black widow). 300 health; the head is the critical spot
 * (x1.5 per the game-wide rule), and every other part takes base damage.
 *
 * The body is SK_Spider, made in Blender (Art/Models/Creatures/Spider.py), and its physics asset holds a hit zone
 * around every part: shots report the bone they hit. The motion is all code: the eight legs are two-bone IK chains
 * driven by a stepping gait (two tetrapod groups taking turns, feet planted on the ground, steps triggered by distance
 * from each foot's rest spot and led by velocity). Attacks rear up and lunge, hits make it flinch, and death folds the legs
 * in, knees up and out, tips under the body (SpiderCreatureLegs.cpp). The legs' layout and lengths are read from the
 * skeleton (SpiderCreatureRig.cpp).
 *
 * At any size (GetSizeScale: a 0.45x spiderling, a 1.8x giant) the pose is the full-size spider's, scaled: the body frame
 * and leg segments carry the size, so the bones' pose against the model is the same, and the gait's distances and step
 * times grow with it (a big spider takes long, slow strides at the same speed).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASpiderCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	ASpiderCreature();

	virtual void Tick(float DeltaSeconds) override;

	/** Its body from the chelicerae to the abdomen's tip (Spider.py), its legs left free to interleave with a packmate's. */
	virtual FFootprint GetFootprint() const override;

	/** Where knees bend: up from the body and a little outward. Spider.py bent the model's resting legs the same way. */
	static FVector KneePole(const FVector& Up, const FVector& Outward) { return Up + Outward * 0.4f; }

	/**
	 * How high above a point a foot looks for footing (cm), at size 1 and with no slope to follow: a little, so feet find
	 * bumps and steps but never climb walls. Up a slope it looks from where the slope puts the ground there (GroundUnder).
	 */
	static constexpr float FootSearchAbove = 60.f;

	/** Climbing out of the dirt as it comes into the world in view (PlayEntrance) takes this long (s). */
	static constexpr float EmergeSeconds = 0.8f;

	/** Its idle life now (SpiderCreatureReact.cpp): a front leg tapped, the front legs groomed at its fangs. For the tests and tools. */
	enum class EIdleAct : uint8 { None, Tap, Groom };
	EIdleAct GetIdleAct() const { return IdleAct; }

protected:
	virtual void BeginPlay() override;
	// --- Hits, death, coming back, the notice beat, its entrance (SpiderCreatureReact.cpp) ---
	virtual void OnAttackStarted() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnPoseThawed() override;
	virtual void OnSizeChanged() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual void OnNoticed() override;
	virtual void PlayEntrance() override;

private:
	struct FLeg
	{
		float Side = 1.f;          // +1 right, -1 left
		int32 Pair = 0;            // 0 = front ... 3 = back
		int32 Group = 0;           // gait group (alternating tetrapod)
		FVector Hip;               // body space (at size 1; the body's frame carries the size)
		FVector Rest;              // resting foot, actor space at size 1 (Z ignored; feet sit on the ground)
		float FemurLength = 90.f;  // at size 1
		float TibiaLength = 115.f;
		FName Femur;
		FName Tibia;
		/** Each bone relative to its segment's frame (from its root joint, X toward the next, Z toward the pole). */
		FTransform FemurInSegment;
		FTransform TibiaInSegment;

		/** The hip-to-foot distance standing on its resting spot (cm at size 1): how far in a foot has come, for the knee's spread. */
		float RestStretch = 150.f;

		FVector Foot = FVector::ZeroVector;  // world
		FVector StepFrom = FVector::ZeroVector;
		FVector StepTo = FVector::ZeroVector;
		float StepAlpha = 1.f;
		bool bStepping = false;
		bool bNeedsReset = false;
		float LastStepTime = 0.f;
		/** A front leg the attack holds follows its target on a spring (cm/s, world), so it rises from rest rather than leaping. */
		FVector HeldVelocity = FVector::ZeroVector;
		/** How far the knee spreads out past KneePole now (eased toward WantedKneeSpread). */
		float KneeSpread = 0.f;
		/** The foot as last posed (world: where the IK put it), and where it was as the spider died (its curl starts there). */
		FVector PosedFoot = FVector::ZeroVector;
		FVector DeathFoot = FVector::ZeroVector;
	};

public:
	/**
	 * How long the death curl takes, and the body's sinking onto the folded legs with it (seconds; a bigger spider's takes as
	 * long). A foot travels about 1.7 m from where it stood to under the body: in 0.55 s the knees moved 14 cm a frame.
	 */
	static constexpr float CurlSeconds = 0.8f;
	/** Dead, the thorax lies this high over the ground (cm at size 1), propped on its folded legs. */
	static constexpr float DeadRide = 32.f;

private:
	/** A bone the code turns about its resting position, in the body's frame. */
	struct FPivotBone
	{
		FName Bone;
		FVector Pivot = FVector::ZeroVector;  // body space
		FTransform BoneInPivot;
	};

	/** Reads the rig: the bones it moves and the legs' layout. False when the mesh isn't the spider's (SpiderCreatureRig.cpp). */
	bool SetupRig();

	/**
	 * A leg segment's frame: at its root joint, X along the segment, Z toward the pole, scaled by the spider's size (Scale)
	 * like the mesh, so the bone keeps its own scale against the model. The rig measures each leg bone in it at rest and the
	 * gait poses the bone in it every frame, so both build it here, the same way.
	 */
	static FTransform SegmentFrame(const FVector& From, const FVector& To, const FVector& Pole, float Scale);

	void PlantLegs();
	void AnimateBody(float DeltaSeconds);
	void AnimateLegs(float DeltaSeconds);
	// --- The legs' held and dying poses, and the knee's spread (SpiderCreatureLegs.cpp) ---
	/**
	 * Two-bone IK: places the knee so both segments keep their length, bending toward Pole. Unreachable targets are clamped
	 * along the hip-to-target line.
	 */
	static void SolveTwoBone(const FVector& Hip, const FVector& Target, float Upper, float Lower, const FVector& Pole, FVector& OutKnee,
		FVector& OutFoot);
	/**
	 * Dead (Time: seconds since), one leg folds from where its foot was as it died to lying crumpled at its side, knee up and
	 * out, tip tucked under the body: its foot eases along in the world and its knee's bend turns with it, so nothing snaps.
	 * Kick moves its foot's target (a last twitch, world). Returns the knee, the foot and the pole the bones are laid by (world).
	 */
	void CurlDeadLeg(const FLeg& Leg, float Time, const FVector& Kick, FVector& OutKnee, FVector& OutFoot, FVector& OutPole) const;
	/** Records where each foot is as the spider dies, for its curl to start from. */
	void RecordDeathPose();
	/**
	 * A front leg the attack holds moves toward Target on a critically damped spring (Omega: 1/s), in short even steps: it
	 * starts from rest and eases in, where a plain chase leapt off the ground at full speed.
	 */
	static void StepHeldFoot(FLeg& Leg, const FVector& Target, float Omega, float DeltaSeconds);
	/**
	 * How far past KneePole a walking leg's knee spreads out (a share of the outward way) with its foot Stretch from its hip
	 * (cm at size 1): farther as the foot comes in, so a folding leg's femur swings out to the side instead of up into the
	 * body (a back leg's into the abdomen it sits under).
	 */
	static float WantedKneeSpread(const FLeg& Leg, float Stretch);
	/** Puts pose slot Index's bone where it sits (InSegment) in a frame that is now at SegmentToWorld. */
	void SetBone(int32 Index, const FTransform& InSegment, const FTransform& SegmentToWorld);
	/** Turns (and scales) a pivot bone about its resting position, in the body's frame. */
	void PosePivot(int32 Index, const FPivotBone& Pivot, const FRotator& Rotation, const FVector& Scale);
	/**
	 * The ground under a foot's spot (SpiderCreatureRig.cpp): looked for from a little above where the slope the spider
	 * stands on puts the ground there, so feet up a slope land on it rather than inside it.
	 */
	FVector GroundUnder(const FVector& Point) const;

	/** How far (cm, full size) a foot may get from its spot before its group steps it back: farther at speed. */
	static float StepThresholdAt(float Speed);

	// --- Reactions and idle life (SpiderCreatureReact.cpp) ---
	/** Its idle acts, its entrance's climb and its death's twitches move on. */
	void TickReactions(float DeltaSeconds);
	/** The notice beat's rear (higher, nose up, fangs flared) and a groom's nod and chewing, on the body's targets. */
	void ReactBody(float& TargetZ, float& TargetPitch, float& TargetFang) const;
	/** Where a leg's foot is posed off its planted spot now (world): a front leg raised for the notice, tapped or groomed with. */
	FVector ReactFootOffset(const FLeg& Leg, const FTransform& Body) const;
	/** A dead leg's last twitch now (world offset of its foot), or none. */
	FVector DeathTwitch(int32 LegIndex, float Time, const FVector& Outward, const FVector& Up) const;
	/** A foot landed from a step at Speed: running, it kicks up a puff (every other landing). */
	void NoteFootLanded(const FLeg& Leg, float Speed);
	/** How far under its stance the body is as it climbs out of the dirt (cm, negative), 0 once out. */
	float EmergeDrop() const;
	/** Dead, its twitches over and its sink not yet begun: its pose is still, and isn't worked out again. */
	bool IsCorpseSettled() const;

	EIdleAct IdleAct = EIdleAct::None;
	float IdleActTime = 0.f;
	/** Its idle act's hold on the legs (eased off at once if it's disturbed), the next act's wait, and the side tapping. */
	float IdleWeight = 0.f;
	float NextIdleAct = 4.f;
	bool bIdleClockSet = false;
	float IdleSide = 1.f;
	/** How much of the notice beat's pose it shows (eased after the beat's own curve). */
	float NoticePose = 0.f;
	/** Seconds into its climb out of the dirt (below 0: not climbing). */
	float EmergeTime = -1.f;
	/** Its death's two last twitches: when (s after death) and which leg. */
	float TwitchAt[2] = { 1.6f, 2.6f };
	int32 TwitchLeg[2] = { 0, 5 };
	int32 FootfallsLanded = 0;

	TArray<FLeg> Legs;
	/** The gait group whose turn it is to step (INDEX_NONE while all feet are down), and the group that stepped last. */
	int32 SteppingGroup = INDEX_NONE;
	int32 LastGroup = 1;
	FPivotBone FangLeft;
	FPivotBone FangRight;
	FPivotBone Abdomen;
	/** The thorax bone relative to the body's frame (level, at the thorax's resting position). */
	FTransform BodyInFrame;
	/** Thorax height above the ground when standing: the model's (at size 1). */
	float RideHeight = 62.f;
	bool bRigReady = false;

	/** The body's frame this frame, in the world: the old spider's BodyRoot. Scaled by the spider's size. */
	FTransform BodyFrame;
	FTransform ComponentToWorld;

	float AnimTime = 0.f;
	float BodyZ = 0.f;
	float BodyPitch = 0.f;
	float BodyRoll = 0.f;
	/** The tilt its planted feet last gave the body, alive and not attacking: a dead body keeps the ground's slope. */
	float GroundPitch = 0.f;
	float GroundRoll = 0.f;
	bool bBodyInitialized = false;
	/**
	 * A hit's jolt: the hit sets a kick that holds a moment (HurtHold, seconds) and fades fast, and the body's offset follows
	 * it on a stiff spring, so it peaks a few frames on instead of jumping there in one.
	 */
	FVector HurtKick = FVector::ZeroVector;
	float HurtHold = 0.f;
	FVector HurtOffset = FVector::ZeroVector;
	FVector HurtVelocity = FVector::ZeroVector;
	/** A hit's tilt away from the shot (X pitch, Y roll, degrees), on the jolt's spring. */
	FVector HurtTiltKick = FVector::ZeroVector;
	FVector HurtTilt = FVector::ZeroVector;
	FVector HurtTiltVelocity = FVector::ZeroVector;
	float AbdomenKick = 0.f;
	float FangOpen = 0.f;
};
