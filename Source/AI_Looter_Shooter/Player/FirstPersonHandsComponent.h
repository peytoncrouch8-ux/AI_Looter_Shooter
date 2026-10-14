#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "Player/FirstPersonHandsRig.h"
#include "Player/FirstPersonHandsTypes.h"
#include "FirstPersonHandsComponent.generated.h"

class AWeaponBase;
class UCameraComponent;
class UPlayerThrowComponent;
class UPlayerViewComponent;
class USkeletalMesh;
class UWeaponManagerComponent;

/**
 * The player's first-person arms, Ellis's gloves and duster sleeves (SK_FP_Arms), on the gun in hand like Borderlands':
 * a child of the first-person camera drawn as first person (the gun's own field of view and scale, so they never clip a
 * wall), posed entirely by code (FFirstPersonHandsRig) from the gun as it is this frame. ALooterCharacter makes it.
 *
 *  - The right hand is fixed round the gun's grip (its index finger at the trigger), so everything that moves the gun
 *    (bob, sway, sprint, crouch, climb, the draw, the menu's lowering, aiming down the sights, recoil, a strike, a
 *    throw's dip) moves the hand with it.
 *  - The left hand holds the foregrip (a six-gun's support palm) and does the work: the reloads (magazine, shells and
 *    pump, cylinder) and the grenade (FirstPersonHandsMotion), on the same clocks as the parts and the jar they handle.
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

	/** Where a hand's grip point is now, and where it was sent (world): the last frame's pose and its targets. */
	FVector GetHoldPoint(EFirstPersonHand Hand) const;
	FVector GetTargetHoldPoint(EFirstPersonHand Hand) const;

	/** What the support hand did last frame. */
	EHandsSupportJob GetSupportJob() const { return SupportJob; }

	const FFirstPersonHandsRig& GetRig() const { return Rig; }

	/** Ellis's first-person arms (Art/Models/Characters/FPArms.py): used once their package exists. */
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
	 * where they did: every spot is the same, the grip contact exact. Looter.Hands.Scale overrides it while tuning.
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
	/** The arms' mesh (SK_FP_Arms, else the stand-in), the rig bound to it, and where it sits on the camera. */
	bool LoadArms();

	/** Sets the arms on the camera at Scale: SK_FP_Arms as made (its root is the eye), the stand-in turned and set back. */
	void PlaceOnCamera(float Scale);

	/** The arms' size now: ArmsScale, or Looter.Hands.Scale while it's set. */
	float GetWantedScale() const;

	/** Places the hands and writes the arms' pose, its bones marked to refresh; false (and hidden) when they don't show. */
	bool PoseHands(float DeltaTime);

	/** The gun in hand, if the hands should be on it now; false hides them. */
	bool FindGun(float DeltaTime, FHandsGun& OutGun);

	/** The gun as the hands see it (camera space). */
	void DescribeGun(const AWeaponBase& Weapon, const FTransform& CameraWorld, FHandsGun& OutGun) const;

	/** The support hand's placement this frame (camera space), easing over when its job changed under it. */
	FHandPlacement PlaceSupportHand(const FHandsGun& Gun, float DeltaTime);

	void Show(bool bShow);

	FFirstPersonHandsRig Rig;
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<UPlayerViewComponent> View;
	TWeakObjectPtr<UWeaponManagerComponent> Weapons;
	TWeakObjectPtr<UPlayerThrowComponent> Throw;

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
	bool bStandIn = false;
	bool bShowing = false;
};
