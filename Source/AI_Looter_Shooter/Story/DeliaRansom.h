#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/Interactable.h"
#include "Story/DeliaRansomRules.h"
#include "Story/StoryCondition.h"
#include "Story/WardenCrewBody.h"
#include "Story/WardenCrewLife.h"
#include "Story/WardenCrewPose.h"
#include "DeliaRansom.generated.h"

class ACreatureBase;
class UBodyFigureComponent;
class UHealthComponent;
class UMissionRunner;
class USpeakerPointComponent;
class UStaticMeshComponent;
class UStoryLineSet;
struct FBulletLanding;
struct FBulletShot;

/** Where Delia is in her fight. */
enum class EDeliaFight : uint8
{
	/** Nothing near: the gun carried low, watching the road. */
	Calm,
	/** Creatures near (or a script's word): the gun up, turning on the nearest, firing. */
	Ready,
	/** Breaking it open for two more shells. */
	Reloading,
};

/**
 * Delia Ransom, Abel's mother, on her porch with her old shotgun (the user's call, 2026-10-31: she gets a model, fights
 * beside the player, then talks face to face for Landfall's turn-in). Tagged Speaker_Delia, her speaker point named "Delia
 * Ransom": the talk and turn-in objectives find her by the tag, as they found her screen door before (WORLD's farm build
 * puts her where the door's speaker point was).
 *
 * Calm, she stands her porch with the gun carried low, glancing up the road and over the orchard, watching the player who
 * comes up (a few seconds at a time). While creatures are within 18 m of her porch she fights (DeliaRansomFight.cpp;
 * DeliaFightRules): the gun to her shoulder, turned on the nearest, a blast every ~1.8 s through the game's own bullets
 * (UBulletSubsystem: pellets, tracers, impacts, hit reactions; about 25% of a Dredger's health if they land; never a
 * critical), two shells then a reload, and she never takes one below a tenth of its health: the finish, the kill and its
 * experience are the player's. She holds fire with the player in her line. Barks from DA_Lines_DeliaFight (STORY's)
 * as queued captions, a few seconds apart at least. The creatures she hurts may turn on her (she's a character with
 * health, as the hunt asks); she never dies (her health takes hits and loses nothing), and a hit makes her flinch and
 * shout. SetFighting overrides it for scripts. Fighting, she can't be talked to; after, she turns to whoever talks to her,
 * the gun lowered, and looks them in the eye.
 *
 * The body is the crew's (WardenCrewBody: the mannequin's idle and the layer, a hero standing in, Odessa's tinted to a dark
 * shawl and grey hair) until the user's model is at /Game/Art/Characters/SK_Delia (rigged to the mannequin's skeleton);
 * the shotgun a stand-in from the Ranchhand's heritage parts until /Game/Art/Props/SM_DeliaShotgun (sockets Grip, Foregrip,
 * Muzzle).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ADeliaRansom : public ACharacter, public IInteractable
{
	GENERATED_BODY()

public:
	ADeliaRansom();

	// --- IInteractable: talking to her is her speaker point's, but not mid-fight ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	virtual void Tick(float DeltaSeconds) override;
	/** She stands her ground: a creature's shove rocks her (a flinch), never moves her off her porch. */
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride) override;

	/** For scripts: true fights (the gun up, firing at anything in reach) whatever is near; false never fights. */
	UFUNCTION(BlueprintCallable, Category = "Delia")
	void SetFighting(bool bFight);

	/** Back to fighting whenever creatures come near. */
	UFUNCTION(BlueprintCallable, Category = "Delia")
	void ClearFightingOverride();

	/** She's in her fight now (ready or reloading). */
	UFUNCTION(BlueprintPure, Category = "Delia")
	bool IsFighting() const { return Fight != EDeliaFight::Calm; }

	/** Moves her on by DeltaSeconds: the fight, the turn, the eyes and the pose (the tick does; tests may). */
	void Step(float DeltaSeconds);

	/** Reads the story again: shown or not (ShownWhen). */
	void RefreshShown();
	bool IsShown() const { return bShown; }

	static const FName SpeakerTag;
	static FText SpeakerNameText();
	/** Her line set of barks (STORY: DA_Lines_DeliaFight). */
	static const TCHAR* const FightBarksPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBodyFigureComponent> Figure;

	/** The gun's grip frame, on the right hand's HandGrip_R socket; the gun's pieces hang from it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Shotgun;

	/** The user's shotgun once it's in the game (SM_DeliaShotgun; empty until then). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ShotgunModel;

	/** The stand-in: the Ranchhand's heritage receiver, then its barrel, muzzle, stock, magazine tube and pump on its sockets. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> ShotgunParts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpeakerPointComponent> SpeakerPoint;

	/** Her health: there so the creatures she hurts can hunt her; it takes their hits and loses nothing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> Health;

	/** When she's on her porch; an empty condition: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	FStoryCondition ShownWhen;

	/** Her barks while she fights (a line set; lines with no speaker are hers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delia")
	TSoftObjectPtr<UStoryLineSet> FightBarks;

	/** At least this long between two barks (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delia", meta = (ClampMin = "0", Units = "s"))
	float BarkGap = 7.f;

	// --- For tests and tools ---
	EDeliaFight GetFight() const { return Fight; }
	ACreatureBase* GetFightTarget() const { return Target.Get(); }
	/** The target is worth a shell (above the floor she leaves to the player). */
	bool IsTargetWorthShooting() const { return bTargetWorth; }
	int32 GetShotsFired() const { return ShotsFired; }
	int32 GetShellsLoaded() const { return Shells; }
	int32 GetBarksSaid() const { return BarksSaid; }
	float GetShoulder() const { return Shoulder; }
	float GetFlinch() const { return Flinch; }
	const FWardenPoseInput& GetPoseInput() const { return PoseInput; }
	const FWardenLife& GetLife() const { return Life; }
	/** Her porch: where she was placed (her fight's middle), and the way she faced. */
	const FVector& GetPorch() const { return Porch; }
	/** Her gun's muzzle now (world). */
	FVector GetMuzzle() const;
	/** The pellets' damage of her next blast at Target (before the game's roll). */
	float PelletDamageFor(const ACreatureBase& Creature) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- DeliaRansomFight.cpp ---
	/** Looks round her porch for creatures in reach, and picks the one she turns on. */
	void ScanForTargets();
	/** The fight's step: ready or calm, the gun up or down, a shot when it's time, the reload. */
	void StepFight(float DeltaSeconds);
	/** One blast at the target: pellets through the game's bullets, the flash, the sound, the kick. */
	void FireBlast();
	void StartReload();
	/** A pellet of hers about to land: never critical, never below the floor (UBulletSubsystem::OnBulletLanding). */
	void HandleBulletLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing);
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);
	/** A bark for Moment (Chance of it; DeliaFightRules::BarkLines), unless she barked lately or someone's speaking. */
	void Bark(EDeliaBark Moment, float Chance);

	// --- DeliaRansomShotgun.cpp ---
	/** In the constructor: the stand-in's pieces on the gun's frame, the grip on the hand. */
	void MakeShotgun();
	/** As play begins: the user's shotgun instead, when it's in the game. */
	void WearShotgun();
	/** The muzzle's flash and smoke, through the bullets' effects. */
	void ShowMuzzle(const FVector& Muzzle, const FVector& Way);

	// --- DeliaRansom.cpp ---
	/** This frame's pose: the gun, the turn, the eyes. */
	void BuildPose(float DeltaSeconds);
	void SetShown(bool bInShown);
	void HandleMissionsChanged();
	void HandleTalked(USpeakerPointComponent& Point, AActor* InListener);
	float BodyScale() const;

	FWardenLife Life;
	FWardenPoseInput PoseInput;
	EWardenDetail Detail = EWardenDetail::Full;
	float DetailClock = 0.f;

	FVector Porch = FVector::ZeroVector;
	float PorchYaw = 0.f;
	/** The gun's grip-to-forend in its own space (from the gun's sockets) and its muzzle from the grip. */
	FVector Forend = FVector(41.5, 0.0, 3.0);
	FVector MuzzleFromGrip = FVector(95.0, 0.0, 6.0);
	FTransform SocketInHand = FTransform::Identity;

	TWeakObjectPtr<AActor> Listener;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
	FDelegateHandle LandingHandle;
	bool bShown = true;
	/** Shown or hidden at least once (play has begun). */
	bool bShownApplied = false;

	EDeliaFight Fight = EDeliaFight::Calm;
	TOptional<bool> FightOverride;
	TWeakObjectPtr<ACreatureBase> Target;
	bool bTargetWorth = false;
	bool bTargetSeen = false;
	float ScanLeft = 0.f;
	float ReadyLeft = 0.f;
	float Cooldown = 0.f;
	float ReloadClock = 0.f;
	float Shoulder = 0.f;
	float Kick = 0.f;
	float Flinch = 0.f;
	float FlinchSide = 1.f;
	int32 Shells = 2;
	int32 ShotsFired = 0;
	/** The creature she shot at last, to hear it fall. */
	TWeakObjectPtr<ACreatureBase> LastShotAt;

	/** The creature she last called on the player to finish, and the hose-man she last warned of (once each). */
	TWeakObjectPtr<ACreatureBase> FinishCalledFor;
	TWeakObjectPtr<ACreatureBase> HoseManCalledFor;

	/** Her barks, loaded as play begins (no hitch in the fight). */
	UPROPERTY(Transient)
	TObjectPtr<UStoryLineSet> Barks;
	int32 BarksSaid = 0;
	double LastBarkAt = -1000.0;
	int32 LastBarkLine = INDEX_NONE;
	FRandomStream Random = FRandomStream(1931);
};
