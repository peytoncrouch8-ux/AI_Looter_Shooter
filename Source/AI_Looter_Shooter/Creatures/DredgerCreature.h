#pragma once

#include "CoreMinimal.h"
#include "Combat/HitPose.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DredgerBody.h"
#include "Creatures/DredgerTypes.h"
#include "DredgerCreature.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Vane's Dredgers (the user's pick, 2026-10-31; contract Docs/Work/2026-10-31-Wardens/CONTRACT.md, DREDGER): Vane
 * Consolidated's company men in black rubber coats and gas masks, the game's first human enemy, recurring through the
 * campaign. A real creature (ACreatureBase): senses, chase, the fight director's turns and places, ranks and tiers, levels by
 * area, traits, hit reactions (flinch, stagger, kill styles), kill XP and the loot table, its tag and its bestiary page.
 *
 * The body is a human on the mannequin's skeleton, animated by the mannequin's own clips the way the player's third-person
 * body is (UDredgerBodyAnimInstance: walk and jog by speed and direction, the rifle hold, the reload, the player's stance
 * layer for crouch, aim and grip, and a code layer for hits). Until the user's Tripo Dredger lands (TRIPO_MODELS.md 5-6,
 * rigged to the mannequin skeleton) it wears the mannequin darkened to black rubber, with a stand-in kit on its bones: a gas
 * mask (face plate, filter snout, two round glass lenses that flare before it acts) and a flat company cap. One path swaps
 * the body (DredgerLook::BodyMeshPath); the kit then goes (Looter.Dredger.Body.Kit 0).
 *
 * Its jobs are subclasses with a fixed Role (ClassFor): ADredgerRifleman (cover, bursts, reloads, flanks) and ADredgerHoseMan
 * (the soul-hose, the tank). The head (its mask's lenses) is the critical spot, x1.5 as every crit. Killed, it goes limp
 * (a ragdoll thrown by the killing shot, harder by the gun and an overkill) and its soul-smog leaks out of the mask.
 * Topics: DredgerCreature.cpp (body, kit, hold, the body's input), DredgerCreatureReact.cpp (hits, barks, notice, death).
 */
UCLASS(Abstract)
class AI_LOOTER_SHOOTER_API ADredgerCreature : public ACreatureBase
{
	GENERATED_BODY()

public:
	ADredgerCreature();

	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsCriticalSpot(const FHitResult& Hit) const override;
	virtual FFootprint GetFootprint() const override;

	/** Its job, fixed by its class (a spawner or a bestiary page names the class: ClassFor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dredger")
	EDredgerRole DredgerRole = EDredgerRole::Rifleman;

	/** The class that does Role (ADredgerRifleman, ADredgerHoseMan): what spawners and the dev command spawn. */
	static TSubclassOf<ADredgerCreature> ClassFor(EDredgerRole InRole);

	/**
	 * Holds its ground (the tender's riflemen, who stay on its deck and fire across): it looks for no cover and keeps no
	 * distance, only turns and fires from where it stands, and its brain never walks it off toward its target.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dredger")
	void SetHoldsGround(bool bHold);
	bool HoldsGround() const { return bHoldsGround; }

	/** A post (ACreatureBase::PostRadius): a job that fights from it (FightsFromPost) is held from walking in, as holding. */
	virtual void SetPost(float Radius) override;

	/** Its body's input this frame (UDredgerBodyAnimInstance reads it as its update begins). */
	const FDredgerBodyInput& GetBodyInput() const { return BodyInput; }

	/** What it holds (the rifle's model, the hose-man's nozzle) and where its muzzle is now (world). */
	USceneComponent* GetHeld() const { return Held; }
	FVector GetMuzzle() const;

	/** Its mask's lenses (0 left, 1 right) and whether a crit has broken one. */
	UStaticMeshComponent* GetLens(int32 Side) const { return Side == 0 ? LensLeft : LensRight; }
	int32 GetBrokenLenses() const { return BrokenLenses; }

	/** A Dredger's bark (muffled through the mask, LooterSoundCue::DredgerBark or another cue), at most one near the player a Looter.Dredger.BarkGap. */
	bool Bark(FName Cue, float Chance = 1.f);

	/** The world's last Dredger bark (world seconds), shared by them all; tests reset it. */
	static void ResetBarks();

protected:
	virtual void BeginPlay() override;
	virtual void OnHurt(bool bCritical, const FVector& HitLocation) override;
	virtual void OnDied() override;
	virtual void OnRespawned() override;
	virtual void OnNoticed() override;
	virtual void SetHitVolumesEnabled(bool bEnabled) override;
	virtual void OnPoseThawed() override;

	// --- For the jobs ---

	/** The job's own frame before the brain (the hose-man holds his brain while latched). */
	virtual void TickBeforeBrain(float DeltaSeconds) {}
	/** The job's own frame after the brain (tactics, bursts, the hose), alive only. */
	virtual void TickJob(float DeltaSeconds) {}
	/** Puts the job's parts on the body once the mesh is there (BeginPlay; the tank). */
	virtual void PlaceJobKit() {}
	/** The thing in its hands, in its own space (cm): where the right hand grips it, the left hand holds it, and it fires from. */
	virtual FVector GetHeldGrip() const { return FVector::ZeroVector; }
	virtual FVector GetHeldForegrip() const { return FVector(30.f, 0.f, -3.f); }
	virtual FVector GetHeldMuzzle() const { return FVector(60.f, 0.f, 0.f); }
	virtual float GetHeldScale() const { return 1.f; }
	/**
	 * With a post, its brain never walks it in after a target (its job's tactics keep it in its post and fire from there: the
	 * rifleman). Off (the hose-man): its brain walks it in, only as far as the post's edge.
	 */
	virtual bool FightsFromPost() const { return false; }

	/** Where it aims now (world): its target's chest, else straight ahead at chest height. */
	FVector GetAimPoint() const;
	/** The middle of its chest now (world), where its aim turns from. */
	FVector GetChestPoint() const;

	/** What the job wants of the body this frame; eased toward each frame (TickBody). */
	struct FPoseWants
	{
		/** Crouched behind cover (0-1). */
		float Crouch = 0.f;
		/** Leaning out of cover (-1 left .. 1 right). */
		float Lean = 0.f;
		/** Braced against the line (0-1) and its tug now (degrees). */
		float Brace = 0.f;
		float Tug = 0.f;
		/** The reload's weight and how far into it (0-1). */
		float ReloadWeight = 0.f;
		float ReloadShare = 0.f;
		/** The gun at the shoulder on its target (fighting), else carried at the low-ready. */
		bool bShouldered = false;
	};
	FPoseWants Wants;

	/** A shot's kick on the body and the gun (the stance layer's recoil), Strength 1 a rifle round. */
	void KickRecoil(float Strength);
	/** The lenses flare for Seconds (a tell: it is about to act), strongest at its end. */
	void FlareLenses(float Seconds, float Strength = 1.f);

	/** Makes a look on M_StylizedSurface (the project's master: Color, Glow, Variation). */
	UMaterialInstanceDynamic* MakeLook(const FLinearColor& Color, float Glow, float Variation);
	/** A kit part on the body: its mesh, a look, no collision, placed as InMesh says in the mesh's frame at rest and riding Bone. */
	UStaticMeshComponent* MakePart(const TCHAR* Name, FName Bone);
	void PlacePart(UStaticMeshComponent* Part, FName Bone, const FTransform& InMesh) const;
	/** InMesh as a part whose mesh is PartMesh fills a box Size (cm) centred at Centre (the mesh's frame), whatever its pivot. */
	static FTransform FitInBox(const UStaticMesh* PartMesh, const FVector& Centre, const FVector& Size, const FQuat& Turn = FQuat::Identity);
	/** Where Bone sits at rest in the mesh's frame (identity without a mesh or the bone). */
	FTransform BoneAtRest(FName Bone) const;

	/** The component its held thing hangs from (the mannequin's right-hand grip socket). */
	static const FName HoldSocket;

	UPROPERTY(VisibleAnywhere, Category = "Dredger")
	TObjectPtr<USceneComponent> Held;

	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> MaskFace;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> MaskSnout;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> MaskFilter;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> LensLeft;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> LensRight;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> CapCrown;
	UPROPERTY(VisibleAnywhere, Category = "Dredger|Kit")
	TObjectPtr<UStaticMeshComponent> CapBrim;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RubberLook;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LensLook;

	/** The engine's basic shapes, loaded once by the constructor for the kit. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMesh;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceMaster;

	/** Seconds since its death (its sink and its code fall). */
	float DeadTime = 0.f;
	/** Fighting: the gun shouldered on its target (eased; the low-ready otherwise). */
	float Shouldered = 0.f;

private:
	// --- Body (DredgerCreature.cpp) ---
	void ApplyBodyLook();
	void PlaceKit();
	/** Springs, eases, the gun in its hands and the body's input for this frame. */
	void TickBody(float DeltaSeconds);
	/** The held thing turned to the aim with its grip in the hand (as the player's third-person gun); the gun's turn back. */
	FQuat PlaceHeld(const FQuat& Aiming, float Yaw);
	void FillStance(const FQuat& Aiming, const FQuat& GunWorld, float AimPitch);

	// --- Hits and death (DredgerCreatureReact.cpp) ---
	void BreakLens(const FVector& HitLocation);
	void GoLimp();
	void TickCorpse(float DeltaSeconds);
	void LeakSmog(float Strength) const;

	/** Widens its attack range while it holds its ground or fights from a post; puts it back once neither holds. */
	void UpdateHoldRange();

	FDredgerBodyInput BodyInput;
	bool bHoldsGround = false;
	/** Its attack range is widened (holding its ground or at a post it fights from), and what it was before (put back when let go). */
	bool bRangeWidened = false;
	float RangeBeforeHold = 0.f;
	bool bKitPlaced = false;

	// The body's springs and eases.
	FHitSpring FlinchSpring;
	FHitSpring HeadSpring;
	float CrouchNow = 0.f;
	float LeanNow = 0.f;
	float BraceNow = 0.f;
	float ReloadWeightNow = 0.f;
	/** The kick: how far back (cm) and up (degrees) right now, easing out. */
	float KickBack = 0.f;
	float KickPitch = 0.f;
	float BreathTime = 0.f;
	/** The lenses' flare: seconds left, its length, its strength. */
	float FlareLeft = 0.f;
	float FlareLength = 0.f;
	float FlareStrength = 0.f;
	int32 BrokenLenses = 0;
	/** When its hurt sound last played (one a burst). */
	double LastHurtSound = -10.0;

	// Death
	/** The body's place on its capsule as play began (put back when it comes back from a ragdoll or a sink). */
	FTransform MeshRest = FTransform::Identity;
	bool bLimp = false;
	bool bSinking = false;
	float FallWay = 1.f;
};
