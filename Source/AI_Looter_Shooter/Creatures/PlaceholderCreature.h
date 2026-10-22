#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/PlaceholderRules.h"
#include "PlaceholderCreature.generated.h"

class UMaterialInstanceDynamic;
class UPlaceholderCreatureDefinition;
class UStaticMeshComponent;

/**
 * A stand-in for a creature the story has a reason for but that has no model yet (the user's ask, 2026-10-21: "a
 * placeholder blob of clay with the name of the enemy"; Docs/Story.md, "Creature roster"). It fights as a real one: it is
 * an ACreatureBase, so its senses, chase, turns and ring places (UFightDirectorSubsystem), ranks, levels, traits, hit
 * reactions and gunfeel, kill experience and loot table are the game's own (the user's call, 2026-10-22: placeholders pay
 * XP and drop loot by their level, rank and role, no special case). Its tag shows "Name (plain type)".
 *
 * The body is SK_Slime's dome in clay (M_StylizedSurface tinted per kind, opaque), stretched to the kind's size, so the
 * tier shell, hit flash, update rate and bestiary stand all work as on the slime; the physics asset's hulls are its hit
 * zones. A clay ball on its "body" bone is its head, the critical spot; two smaller lumps make it lumpy. All motion is code
 * on the body bone (UCreaturePoseAnimInstance): breathing, a walk's bob and sway, a role's tell and strike
 * (PlaceholderRules::PoseAt), a flyer's hover and dive, a slump and sink in death, rising out of the ground as it comes in.
 *
 * Each kind is a data-only BP_Placeholder_<Name> naming its UPlaceholderCreatureDefinition (Definition); its numbers are
 * written into the BP's defaults by Tools/Unreal/create_placeholder_creatures.py (BakeDefinition), so spawners, the
 * bestiary and SpawnAtRuntime read them as any class's; the actor itself takes only its shape, look and shots from the
 * definition. Roles: Melee, Swarm and Big strike in front (Big's blow throws harder: StrikeStrength); Ranged keeps its
 * band (PlaceholderCreatureMoves.cpp) and shoots pellets through UEnemyProjectileSubsystem on a ranged turn, biting when
 * caught; Flyer hovers and dives. Topics: PlaceholderCreature.cpp (body, look, sounds, death), PlaceholderCreatureMoves.cpp
 * (attacks and distance).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API APlaceholderCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	APlaceholderCreature();

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Its head ball is the critical spot. */
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;
	/** Round, as wide as its body. */
	virtual FFootprint GetFootprint() const override;

	// --- Its place in a fight (PlaceholderCreatureMoves.cpp) ---
	virtual EFightRole GetFightRole() const override;
	virtual EFightRole GetAttackTokenRole() const override;
	virtual bool GetFightBand(float& OutNear, float& OutFar) const override;

	/** Its kind: names, size, colour, role, shots. Set in each BP_Placeholder_<Name>. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placeholder")
	TObjectPtr<UPlaceholderCreatureDefinition> Definition;

	/**
	 * Writes Creature's Definition into Creature (its class's defaults when given a BP's CDO: the editor script calls it):
	 * its tag name, health, attack, experience, speeds, senses, pack and a ranged kind's attack range. False without one.
	 */
	UFUNCTION(BlueprintCallable, Category = "Placeholder")
	static bool BakeDefinition(APlaceholderCreature* Creature);

	/** Its numbers are its definition's (each difference into OutDifferences): Looter.Creatures.Placeholder.Pages. */
	bool MatchesDefinition(TArray<FString>& OutDifferences) const;

	EPlaceholderRole GetRole() const;
	UStaticMeshComponent* GetHead() const { return Head; }
	/** The pose it wears now (the tests). */
	const FPlaceholderPose& GetPoseNow() const { return PoseNow; }
	/** Where its shots leave (world): over the front of its head. */
	FVector GetMuzzle() const;
	/** Shots fired in its life so far (the tests). */
	int32 GetShotsFired() const { return ShotsFired; }
	/** Its body's clay look (tinted per kind), once made. */
	UMaterialInstanceDynamic* GetClayLook() const { return ClayLook; }

protected:
	virtual void BeginPlay() override;
	virtual void OnAttackStarted() override;
	virtual void Strike() override;
	virtual void OnAttackCalledOff() override;
	virtual float GetAttackStartRange() const override;
	virtual bool CanReachHeight(float Rise) const override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnNoticed() override;
	virtual void PlayEntrance() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;

private:
	// --- Body and look (PlaceholderCreature.cpp) ---
	/** Its capsule, the blob's stretch and seat, its tag's height, from its definition. */
	void ApplyShape();
	/** Its clay material per slot, in its kind's colour. */
	void ApplyLook();
	/** The head and lumps on the body bone, sized against the blob's stretch so they stay round. */
	void PlaceParts();
	void AnimateBody(float DeltaSeconds);
	/** Its kind's cue for an alert, attack, hurt or death (its Voice). */
	FName CueFor(int32 Which) const;
	void PlayCue(FName Cue, float Volume = 1.f) const;

	// --- Attacks and distance (PlaceholderCreatureMoves.cpp) ---
	/** A ranged kind keeps its band: backing off, walking round to its fight spot, holding. */
	void TickKeepAway(float DeltaSeconds);
	void SetKeepingDistance(bool bKeeping);
	FVector FirstClearWay(TConstArrayView<FVector> Ways) const;
	/** A ranged kind's attack now would be its bite (its target within reach). */
	bool WantsBite() const;
	/** Its shot at its target has a clear line now (looked at a few times a second). */
	bool IsShotClear() const;
	void FireShots();
	/** Its blow in front, at its strength (as ACreatureBase::Strike, harder for a big kind). */
	void StrikeInFront();

	UPROPERTY(VisibleAnywhere, Category = "Placeholder")
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY(VisibleAnywhere, Category = "Placeholder")
	TArray<TObjectPtr<UStaticMeshComponent>> Lumps;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ClayLook;

	/** Its shape and look are on it (PostInitializeComponents, or BeginPlay where a level never initialised its actors: a test level). */
	bool bShapeApplied = false;
	FName BodyBone = TEXT("body");
	FTransform BodyRest;
	bool bRigReady = false;
	/** The blob's stretch (mesh scale) for its kind. */
	FVector Stretch = FVector::OneVector;

	FPlaceholderPose PoseNow;
	float Clock = 0.f;
	float DeadTime = 0.f;
	/** Its entrance: seconds left of it, and how long it is. */
	float EntranceLeft = 0.f;
	float EntranceLength = 0.6f;
	/** A hit's jolt (degrees, pitch and roll), easing out. */
	FVector2D Jolt = FVector2D::ZeroVector;
	/** When its hit sound last played (one a burst). */
	double LastHitSound = -10.0;

	// Attacks
	/** The attack it's in is a shot (a ranged kind's), not a bite or a blow. */
	bool bShotAttack = false;
	int32 ShotsFired = 0;
	mutable double ShotCheckedAt = -10.0;
	mutable bool bShotClear = false;

	// Distance (a ranged kind)
	EPlaceholderKeep Keep = EPlaceholderKeep::None;
	bool bKeepingDistance = false;
	FVector KeepWay = FVector::ZeroVector;
	float KeepLookIn = 0.f;
	/** Seconds at its spot with its shot blocked (it asks for another). */
	float BlockedFor = 0.f;
};
