#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "Player/FirstPersonHandsRig.h"
#include "Player/FirstPersonHandsTypes.h"
#include "FirstPersonHandsComponent.generated.h"

class AWeaponBase;
class UCameraComponent;
class UEmberComponent;
class UPlayerFlaskComponent;
class UPlayerMeleeComponent;
class UPlayerThrowComponent;
class UPlayerViewComponent;
class USkeletalMesh;
class UWeaponManagerComponent;
class UWeaponModelComponent;

/**
 * The player's first-person arms, the played hero's (UHeroDefinition::FirstPersonArms: Ellis's gloves and duster sleeves
 * SK_FP_Arms, or SK_FP_Arms_<Id>, every one on SK_FP_Arms' skeleton; Ellis's while a hero's aren't in the game), at that
 * hero's size (FirstPersonArmsScale, else ArmsScale), on the gun in hand like Borderlands' (framed by FPHandsFraming: gloves
 * and wrists low in the frame, elbows out of it, a pistol in one hand):
 * a child of the first-person camera drawn as first person (the gun's own field of view and scale, so they never clip a
 * wall), posed entirely by code (FFirstPersonHandsRig) from the gun as it is this frame. ALooterCharacter makes it.
 *
 *  - The right hand is fixed round the gun's grip (its index finger at the trigger), so everything that moves the gun
 *    (bob, sway, sprint, crouch, climb, the draw, the menu's lowering, aiming down the sights, recoil, a strike, a
 *    throw's dip) moves the hand with it.
 *  - The left hand holds the foregrip (a six-gun's support palm) and does the work: the reloads (magazine, shells and
 *    pump, cylinder) and the grenade (FirstPersonHandsMotion), on the same clocks as the parts and the jar they handle,
 *    an ember power's cast (UEmberComponent's cast clock; FirstPersonHandsCast.cpp) and a drink from the soul flask
 *    (UPlayerFlaskComponent's clock; FirstPersonHandsFlask.cpp).
 *  - A melee strike takes the right hand off the grip onto the hero's tool (UPlayerMeleeComponent::GetToolHold), the gun
 *    dropped into the left. With no gun in hand the arms still show for a strike or a cast, the idle hand resting under
 *    the frame (FirstPersonHandsActions.cpp).
 *  - It ticks after everything that places the gun (the locomotion's view model, a strike's and a throw's pose, the
 *    gun's own reload parts: all earlier tick groups), so the hands are never a frame behind.
 *  - Hidden in third person, with no gun in first person, and once a menu has been up long enough to lower the gun
 *    out of view.
 *
 * Until SK_FP_Arms is imported it shows the mannequin (StandInMesh) with every bone but the arms squeezed away; once the
 * package exists at ArmsMesh's path it uses that instead, by itself.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UFirstPersonHandsComponent : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	UFirstPersonHandsComponent();

	/** Places and poses the hands for this frame (TickComponent does, after the gun is placed; tests call it themselves). */
	void UpdateHands(float DeltaTime);

	/** The hands are drawn now. */
	bool IsShowingHands() const { return bShowing; }

	/** The mannequin stands in for the arms (SK_FP_Arms isn't imported yet). */
	bool IsUsingStandIn() const { return bStandIn; }

	/**
	 * Puts on the arms of the hero played now (Looter.Hero switching heroes mid-play: Dev/HeroDevCommands' WearHeroNow).
	 * A session's hero is known before the character begins, so a normal start needs nothing more.
	 */
	void WearHero();

	/** The arms worn now (the hero's, Ellis's or the stand-in), and their size against the guns. */
	const USkeletalMesh* GetArmsMesh() const;
	float GetArmsScale() const { return GetWantedScale(); }

	/** Where a hand's grip point is now, and where it was sent (world): the last frame's pose and its targets. */
	FVector GetHoldPoint(EFirstPersonHand Hand) const;
	FVector GetTargetHoldPoint(EFirstPersonHand Hand) const;
	/** Where it was sent once its palm was seated on its handle (FFirstPersonHandsRig::Seated; world): where it should be. */
	FVector GetSeatedHoldPoint(EFirstPersonHand Hand) const;

	/** What the support hand did last frame. */
	EHandsSupportJob GetSupportJob() const { return SupportJob; }

	const FFirstPersonHandsRig& GetRig() const { return Rig; }

	/**
	 * Ellis's first-person arms (Art/Models/Characters/FPArms.py): every hero's while theirs aren't in the game, and the
	 * player's when no hero is played (a test level's character).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hands")
	TSoftObjectPtr<USkeletalMesh> ArmsMesh;

	/** Until then: a body with the mannequin's bones, all but its arms squeezed away. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hands")
	TSoftObjectPtr<USkeletalMesh> StandInMesh;

	/**
	 * Where the stand-in's shoulders sit off the eye (camera space: X ahead, Y right, Z up; cm). SK_FP_Arms sits as made
	 * (its root is the eye). Far guns bring a shoulder forward anyway (FFirstPersonHandsRig).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hands")
	FVector StandInShoulders = FVector(-2.f, 0.f, -24.f);

	/**
	 * Each hand's usual way out of the wrist for its forearm (camera space), where a placement doesn't say (the guns'
	 * holds do: FGunHands): back, out and down, so the arms come up from under the view. Left, then right.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hands")
	FVector LeftForearm = FVector(-0.5f, -0.5f, -0.7f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hands")
	FVector RightForearm = FVector(-0.7f, 0.45f, -0.55f);

	/**
	 * The arms' size against the guns (1: Ellis's own, as the look sheet showed them). Smaller arms still hold the guns
	 * where they did: every spot is the same, the grip contact exact. A hero's FirstPersonArmsScale replaces it;
	 * Looter.Hands.Scale overrides both while tuning.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hands", meta = (ClampMin = "0.6", ClampMax = "1.3"))
	float ArmsScale = 0.88f;

	/** Seconds a menu is up before the hands go (the gun lowers out of view over 0.2 s first). */
	static constexpr float MenuHideSeconds = 0.25f;
	/** Seconds the support hand takes to ease over when its job is cut short (a strike or a swap stops a reload). */
	static constexpr float JobCatchUpSeconds = 0.12f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** The arms' mesh (the played hero's, else SK_FP_Arms, else the stand-in), the rig bound to it, where it sits on the camera. */
	bool LoadArms();

	/** Sets the arms on the camera at Scale: SK_FP_Arms as made (its root is the eye), the stand-in turned and set back. */
	void PlaceOnCamera(float Scale);

	/** The arms' size now: ArmsScale, or Looter.Hands.Scale while it's set. */
	float GetWantedScale() const;
	/** ...on a gun of GunKind (its framing's ArmsScale): the gun being described this frame, before it's the last one. */
	float GetWantedScaleFor(EWeaponKind GunKind) const;

	/** Places the hands and writes the arms' pose, its bones marked to refresh; false (and hidden) when they don't show. */
	bool PoseHands(float DeltaTime);

	/** The gun in hand, if the hands should be on it now; false hides them. */
	bool FindGun(float DeltaTime, FHandsGun& OutGun);

	/** The gun as the hands see it (camera space). */
	void DescribeGun(const AWeaponBase& Weapon, const FTransform& CameraWorld, FHandsGun& OutGun) const;

	/**
	 * The support hand's cup fitted to the gun's parts (FirstPersonHandsGrips::FitCup), worked out once per gun as assembled
	 * (a part swapped at the bench fits it again) and only at rest (a reload or a rack moves parts: the table meanwhile).
	 */
	FHandsCupFit CupFitFor(const UWeaponModelComponent& Model, EWeaponKind Kind, bool bAtRest) const;
	mutable FHandsCupFit CupFit;
	mutable uint32 CupFitKey = 0;

	/** The support hand's placement this frame (camera space), easing over when its job changed under it. */
	FHandPlacement PlaceSupportHand(const FHandsGun& Gun, float DeltaTime);

	/**
	 * A one-handed gun's support hand (FPHandsFraming::OneHandShare: the revolver's) eased off the gun to its parked spot
	 * under the frame while it only holds; back on the gun for a reload or a throw (camera space).
	 */
	FHandPlacement ParkSupportHand(const FHandsGun& Gun, const FHandPlacement& OnGun, float DeltaTime);

	/** Each hand's forearm turned so it leaves the frame low on a slant, its elbow out of view (FPHandsFraming::FrameForearm). */
	void FrameForearms(float Scale);

	/** How far the support hand is toward its parked spot now (0 on the gun, 1 parked). */
	float OneHandBlend = 0.f;
	/** How far a pump gun's support hand is from its cup toward the pump (FHandsGun::PumpShare). */
	float PumpHandShare = 0.f;
	/** How much the support hand's forearm is its cup's own (fitted to the hand) rather than slanted by the framing. */
	float CupForearmShare = 0.f;

	// --- A strike's tool and a cast (FirstPersonHandsActions.cpp) ---

	/** The trigger hand from OnGrip toward the melee tool by the swing's share (OnGrip itself with no tool out); sets bToolInHand. */
	FHandPlacement RightOnTool(const FHandPlacement& OnGrip);
	/** The support hand in an ember power's cast now, from OnGun; false when no cast is showing. */
	bool CastSupport(const FHandPlacement& OnGun, FHandPlacement& OutPlacement) const;
	/** The support hand in a drink from the soul flask now, from OnGun; false when none is under way. */
	bool DrinkSupport(const FHandPlacement& OnGun, FHandPlacement& OutPlacement) const;
	/** No gun in hand: both hands for a strike, a cast or a drink (Targets), else false (hidden). */
	bool PlaceBareHands();
	/** Swimming (FirstPersonHandsSwim.cpp): both hands stroke once the gun is lowered away; false when not swimming. */
	bool PlaceSwimHands(float DeltaTime);
	/** A melee tool is in the right hand this frame (the gun, if any, is the left hand's alone). */
	bool bToolInHand = false;

	/**
	 * The trigger finger's way to the trigger this frame (FHandsGun::TriggerReady): on it while the trigger is pulled, the
	 * sights are up or for Looter.Hands.TriggerHoldSeconds after a pull, else eased off to lie along the frame.
	 */
	float NextTriggerReady(const AWeaponBase& Weapon, float DeltaTime);
	float TriggerReady = 1.f;
	double LastTriggerTime = -1000.0;

	/** Looter.Hands.Grip.Debug: the handles the hands close on, the trigger spot and the finger joints, drawn in the world. */
	void DrawGripDebug() const;

public:
	/**
	 * Looter.Hands.Grip.Report: each hand on the gun in numbers (the gun's own cm): where it was asked to hold and where it
	 * got to, its palm's gap to its handle, which fingers touched, their curls and gaps, the trigger finger's miss.
	 */
	void ReportGrip() const;

private:
	/** The gun as the hands last saw it (for the report). */
	FHandsGun LastGun;

	void Show(bool bShow);

	FFirstPersonHandsRig Rig;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<UPlayerViewComponent> View;
	TWeakObjectPtr<UWeaponManagerComponent> Weapons;
	TWeakObjectPtr<UPlayerThrowComponent> Throw;
	TWeakObjectPtr<UPlayerMeleeComponent> Melee;
	TWeakObjectPtr<UEmberComponent> Embers;
	TWeakObjectPtr<UPlayerFlaskComponent> Flask;

	/** The last pose in the component's space, and the targets it was posed for (camera space). */
	TArray<FTransform> PosedSpace;
	FHandPlacement Targets[2];

	EHandsSupportJob SupportJob = EHandsSupportJob::Foregrip;
	/** Where the support hand was on the gun (the gun's own space) last frame, and how much of the ease over is left. */
	FHandPlacement SupportOnGun;
	FHandPlacement CatchUpFrom;
	float CatchUp = 0.f;
	bool bHaveSupport = false;

	float MenuOpenSeconds = 0.f;
	/** The played hero's FirstPersonArmsScale while their own arms are worn (0: ArmsScale). */
	float HeroScale = 0.f;
	bool bStandIn = false;
	bool bShowing = false;
};
