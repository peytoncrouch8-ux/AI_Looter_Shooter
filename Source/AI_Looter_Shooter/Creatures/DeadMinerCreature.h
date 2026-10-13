#pragma once

#include "CoreMinimal.h"
#include "Creatures/DeadMinerRules.h"
#include "Creatures/UnpaidCreature.h"
#include "DeadMinerCreature.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * The dead miners (Docs/Areas/RansomsRest_Expansion.md, section 6 "The Providence"): forty years of the Providence's
 * buried dead, home to the mine when the saint went dark, and the eleven who never came up. A kind of the Unpaid on its
 * rig, as the Bell-Ringer is: canvas coats, braces, a felt hat or leather cap with a head lamp, a pick; three tints. 180
 * health and 9 damage at level 1 (the Unpaid's are 160 and 8). They read apart from the Unpaid in a fight: no shriek and
 * no flying lunge, but a heavy, planted pick.
 *
 *  - The swing (DeadMinerCreatureSwing.cpp): it closes in, plants itself and heaves the pick overhead through a 0.8 s
 *    wind-up while its lamp flares (the tell), then brings it down on whoever stands within 2.5 m in front, the pick
 *    biting the floor (sparks, grit; a Gravebound one's blow sends out the Unpaid's slowing ring). The pick stays down a
 *    moment after: the window to punish it. Back off in the wind-up and it hits rock.
 *  - The lamp (DeadMinerCreatureLamp.cpp) is a weak spot of its own besides the coal: a small ball round its lens, always
 *    critical. The first shot on it puts it out for this life (a pop of glass and sparks), and a shot on it in the wind-up
 *    breaks the swing: it reels, the pick flung wide (like the bell and the venom sac). Lit, it lights the miner and what's
 *    round it: an emissive glow always, and a small shadowless light for the two nearest the player only (the area's light
 *    budget), so a gallery of them reads in the dark.
 *  - Unaware, it works (DeadMinerCreatureWork.cpp): it finds the nearest wall at chest height and swings its pick at it,
 *    a steady blow every couple of seconds with sparks off the rock and its lamp on the face; with no wall near it, it
 *    stands over its lunch pail. It never wanders: it works where it was put.
 *  - It barks in its own words (its voice's BarkKind: CreatureVoiceBarkLines.cpp's DeadMiner lines), shown over it.
 *
 * Its body is SK_DeadMiner once the art drops it in (the Unpaid's skeleton), else SK_Unpaid; its parts are found by name
 * when they exist and left off otherwise, so the class works and its tests run without them: SM_DeadMinerHat on the hat
 * bone (in place of the Unpaid's hat; its socket SOCKET_Lamp is where the lamp goes), SM_DeadMinerLamp (its pivot at its
 * mount, its lens along +X, SOCKET_Light at the lens), SM_DeadMinerPick in the right fist (its pivot where the fist grips
 * the haft, the haft along +Z to the head, the point along +X) and SM_DeadMinerPail in the left (the lunch pail, its pivot
 * at the bail). Tints: MI_DeadMiner_A, B and C on the body's slot. Posed by code (DeadMinerCreaturePose.cpp): its arms
 * laid over the Unpaid's pose, the pick held to its right forearm's line.
 */
UCLASS(Config = Game)
class AI_LOOTER_SHOOTER_API ADeadMinerCreature : public AUnpaidCreature
{
	GENERATED_BODY()

public:
	ADeadMinerCreature();

	virtual void Tick(float DeltaSeconds) override;

	/** The lamp is a weak spot of its own, besides the Unpaid's coal. */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;

	// --- Its models (found once imported) and names ---

	static const TCHAR* const ModelPath;
	static const TCHAR* const HatPath;
	static const TCHAR* const LampPath;
	static const TCHAR* const PickPath;
	static const TCHAR* const PailPath;
	static const TCHAR* const ClothesPaths[3];

	/** The hat's socket for the lamp, and the lamp's at its lens. */
	static const FName LampSocket;
	static const FName LightSocket;

	/** Its voice's lines (CreatureVoiceBarkLines.cpp's own for it). */
	static const FName BarkKind;

	// --- What it's doing besides the brain's chase and swing ---

	enum class EAct : uint8
	{
		None,
		/** Unaware: swinging its pick at the wall (GetWorkPoint). */
		Work,
		/** Unaware, no wall near: standing over its lunch pail. */
		Pail,
		/** Its lamp shot in the wind-up: the swing lost, reeling. */
		Reel,
	};
	EAct GetAct() const { return Act; }
	float GetActTime() const { return ActTime; }
	bool IsWorking() const { return Act == EAct::Work; }
	bool IsReeling() const { return Act == EAct::Reel; }

	/** In its swing's wind-up now (the pick going up, the lamp flaring): a shot on the lamp breaks it. */
	bool IsWindingUp() const;

	/** This swing was broken by a shot on the lamp: its blow lands on nothing. */
	bool WasSwingBroken() const { return bSwingBroken; }

	// --- The lamp (DeadMinerCreatureLamp.cpp) ---

	UStaticMeshComponent* GetLamp() const { return Lamp; }
	USphereComponent* GetLampHitZone() const { return LampHitZone; }
	UPointLightComponent* GetLampLight() const { return LampLight; }
	UStaticMeshComponent* GetPick() const { return Pick; }

	/** The middle of the lamp's lens (world): its hit zone's, its light's. */
	FVector GetLampCenter() const;

	/** Whether a hit at Point (world) landed on the lamp. */
	bool IsLampHit(const FVector& Point) const;

	/** Its lamp still burns (until a shot puts it out, for this life). */
	bool IsLampLit() const { return bLampLit; }

	/** Its lamp lights up round it now (lit, and inside the area's light budget). */
	bool IsLampLightOn() const;

	/** How far its lamp is flared now (0 at rest, 1 at the wind-up's height). */
	float GetLampFlare() const { return LampFlare; }

	/** Puts the lamp out for this life (a shot from From: the glass pops toward it). Nothing if it's out already. */
	void PutOutLamp(const FVector& From);

	/** Looks again now whether its lamp lights up round it (the budget), rather than at its next look. */
	void RefreshLampLight();

	// --- Work (DeadMinerCreatureWork.cpp) ---

	/** The nearest wall at chest height within reach (world: the point on it and its face's normal); false: none. */
	bool FindWorkWall(FVector& OutPoint, FVector& OutNormal) const;

	/** Where its pick meets the wall while it works. */
	FVector GetWorkPoint() const { return WorkPoint; }

	/** Blows landed on the wall this life (the tests). */
	int32 GetWorkBlows() const { return WorkBlows; }

	// --- Settings ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Miner")
	FDeadMinerTuning Tuning;

	/** Where the lamp sits on the hat bone without the hat's SOCKET_Lamp (cm, the model's axes: X forward, Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Miner")
	FVector LampOnHat = FVector(11.f, 0.f, 3.f);

	/** The lens's middle from the lamp's pivot without the lamp's SOCKET_Light (cm, the lamp's axes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Miner")
	FVector LensOnLamp = FVector(5.f, 0.f, 0.f);

	/** Where the lamp is without the model (its own space, the ground under it): the brow of a man's height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dead Miner")
	FVector LampPointWithoutRig = FVector(14.f, 0.f, 172.f);

	/**
	 * Where its arms point and what its trunk does (WantedArms), laid over the Unpaid's pose (component space: X forward, Y
	 * right, Z up; the chest's lean and look stay on top). LeftWeight: how much of the Unpaid's own left arm gives way to
	 * LeftUpper and LeftFore. Bow bends it forward at the waist, Kneel lowers it (cm), HeadPitch nods its head (degrees,
	 * + down).
	 */
	struct FMinerArms
	{
		FVector RightUpper = FVector::DownVector;
		FVector RightFore = FVector::ForwardVector;
		FVector LeftUpper = FVector::DownVector;
		FVector LeftFore = FVector::DownVector;
		float Wrist = 0.f;
		float LeftWeight = 1.f;
		/** How quickly the arms ease there (1/s). */
		float Rate = 6.f;
		float Bow = 0.f;
		float Kneel = 0.f;
		float HeadPitch = 0.f;
	};

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual bool CanStartAttack() const override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnRankChanged() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual void LayerPose(float DeltaSeconds) override;

	// --- For a body built on the dead miner (the Foreman: Creatures/ForemanCreature.h) ---

	/** What the arms and trunk want now, for what it's doing (DeadMinerCreaturePose.cpp). */
	virtual void WantedArms(FMinerArms& Out) const;

	/** Whether it works now: calm, alive, nobody to hunt (the Foreman: waiting for his fight, digging at the fall). */
	virtual bool WantsToWork() const;

	/** Whether its lamp's light keeps to the miners' budget (the Foreman's own lamp is always lit round him). */
	virtual bool UsesLampBudget() const { return true; }

	/** How flared its lamp wants to be now (0-1): the swing's wind-up. */
	virtual float WantedLampFlare() const;

	/** Whether it keeps its pose as it dies (the Foreman kneels at the fall), rather than slumping as the Unpaid do. */
	virtual bool HoldsPoseInDeath() const { return false; }

	/** The swing's blow (the brain's strike): the pick comes down on whoever is in reach in front, Strength times its attack. */
	void SwingBlow(float Strength);

	/** Grave dirt and grit thrown up at Where (FWeaponFX), Strength 0 to 1. */
	void KickDirt(const FVector& Where, float Strength) const;

	/** The ground under Where (its own level), or Where itself where there's none (a test level). */
	FVector GroundUnder(const FVector& Where) const;

	/** Its feet: the ground under its middle, by its capsule. */
	FVector GetFeet() const;

	void SetAct(EAct NewAct);

	/** Its swing's own timing, as its class and rank set it (the Foreman's other moves borrow the attack's and give it back). */
	float SwingWindup = 0.8f;
	float SwingRecovery = 0.9f;

	/** Its lamp is a weak spot (the dead miners'); the Foreman's carbide lamp isn't. */
	bool bLampIsWeakSpot = true;

	/** The lamp's glow and light scale on top of the flare (the Foreman's lamp burns brighter). */
	float LampBrightness = 1.f;

	UPROPERTY(VisibleAnywhere, Category = "Dead Miner")
	TObjectPtr<UStaticMeshComponent> Pick;

	UPROPERTY(VisibleAnywhere, Category = "Dead Miner")
	TObjectPtr<UStaticMeshComponent> Lamp;

	/** The lamp's light: small, warm, shadowless, on only inside the budget. */
	UPROPERTY(VisibleAnywhere, Category = "Dead Miner")
	TObjectPtr<UPointLightComponent> LampLight;

	/** The lamp's glow: crossed cards of the gunfire's additive glow (M_FX_Glow), one instanced draw. */
	UPROPERTY(VisibleAnywhere, Category = "Dead Miner")
	TObjectPtr<UInstancedStaticMeshComponent> LampGlow;

	UPROPERTY(VisibleAnywhere, Category = "Dead Miner")
	TObjectPtr<USphereComponent> LampHitZone;

	UPROPERTY(VisibleAnywhere, Category = "Dead Miner")
	TObjectPtr<UStaticMeshComponent> Pail;

private:
	// --- The lamp (DeadMinerCreatureLamp.cpp) ---
	/** Puts the lamp on the hat (its socket), or on the hat bone, or where the model carries it without one. */
	void PlaceLamp();
	void BuildLampGlow();
	/** The lamp's glow and light this frame: its flare, its fade with the body, lit or out. */
	void ApplyLamp(float DeltaSeconds);
	/** Looks a few times a second whether its light is inside the budget. */
	void TickLampBudget(float DeltaSeconds);
	void ApplyLampHitZone();
	/** A bullet on the lamp: out if it was lit, and a broken swing if it was winding up. */
	void LampStruck(const FVector& HitLocation);

	// --- The swing (DeadMinerCreatureSwing.cpp) ---
	void BeginReel();
	void TickReel(float DeltaSeconds);

	// --- Work (DeadMinerCreatureWork.cpp) ---
	void TickWork(float DeltaSeconds);
	void StartWorking();
	void WorkBlow();
	void FaceYaw(float Yaw, float DeltaSeconds);

	// --- The arms and the pick (DeadMinerCreaturePose.cpp) ---
	void SetupArmLayer();
	void LayArm(int32 UpperBone, int32 LowerBone, int32 HandBone, const FVector& RestUpper, const FVector& RestFore, const FVector& Upper,
		const FVector& Fore, float Wrist, float Weight);
	/** The pick held to the right forearm's line, as this frame posed it. */
	void UpdatePick();

	EAct Act = EAct::None;
	float ActTime = 0.f;
	bool bSwingBroken = false;
	bool bSwingTimingCaptured = false;
	/** It wears the miners' tints on the Unpaid's body (no SK_DeadMiner yet): never the Unpaid's own clothes. */
	bool bMinerTints = false;

	// The lamp
	bool bLampLit = true;
	bool bLampLightWanted = false;
	bool bLampZoneWanted = true;
	bool bLampZoneOn = true;
	float LampFlare = 0.f;
	float LampBudgetClock = 0.f;
	float ShownGlow = -1.f;
	float ShownLight = -1.f;

	// Work
	FVector WorkPoint = FVector::ZeroVector;
	FVector WorkNormal = FVector::ZeroVector;
	float WorkYaw = 0.f;
	float WorkTime = 0.f;
	float NextWorkLook = 0.f;
	int32 WorkBlows = 0;
	float PailClock = 0.f;

	// The arm layer
	bool bArmLayerReady = false;
	bool bArmsStarted = false;
	int32 RightUpper = INDEX_NONE;
	int32 RightLower = INDEX_NONE;
	int32 RightHand = INDEX_NONE;
	int32 RightKnuckle = INDEX_NONE;
	int32 LeftUpper = INDEX_NONE;
	int32 LeftLower = INDEX_NONE;
	int32 LeftHand = INDEX_NONE;
	int32 PelvisIndex = INDEX_NONE;
	int32 SpineIndex = INDEX_NONE;
	int32 HeadIndex = INDEX_NONE;
	FVector RestRightUpper = FVector::DownVector;
	FVector RestRightFore = FVector::DownVector;
	FVector RestLeftUpper = FVector::DownVector;
	FVector RestLeftFore = FVector::DownVector;
	FVector NowRightUpper = FVector::DownVector;
	FVector NowRightFore = FVector::ForwardVector;
	FVector NowLeftUpper = FVector::DownVector;
	FVector NowLeftFore = FVector::DownVector;
	float NowBow = 0.f;
	float NowKneel = 0.f;
	float NowHeadPitch = 0.f;
	float NowLeftWeight = 0.f;
	float LayerWeight = 1.f;
	/** Its own clock for the arms' small drift, so a crowd doesn't move in step. */
	float ArmClock = 0.f;
};
