#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "BodyFigureComponent.generated.h"

class UHeroDefinition;
class UIKRetargeter;
class UWeaponManagerComponent;

/**
 * The player's own body, the hero played (the session's pick: UHeroDefinition's FigureMesh and Retargeter; Ellis's,
 * SK_Ellis and RTG_MannyToEllis, the user's call of 2026-10-15, when the hero's aren't in the game yet), worn over the
 * mannequin, which stays the animated body: attached to it, the hero copies its pose every frame through the retargeter
 * (UBodyRetargetAnimInstance), ticking after it. Once both assets are there the mannequin is hidden but keeps animating
 * (the guns, the hands' IK and every socket still use it), and the hero is drawn the way the mannequin would be: hidden
 * to its owner in first person but casting the player's shadow, seen in third (UPlayerViewComponent switches the
 * mannequin; this follows it). Without them it stays hidden and the mannequin shows as before. A holstered gun (any
 * material slot ending in HolsterGun) shows unless a revolver is the gun in hand.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UBodyFigureComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UBodyFigureComponent();

	/** The retargeter from the mannequin to this body (UBodyRetargetAnimInstance reads it as it starts). */
	UIKRetargeter* GetRetargeter() const { return Retargeter; }

	/** A hero's body is worn: its mesh and retargeter are there and the mannequin is hidden under it. */
	bool IsWorn() const;

	/** The skeletal mesh this is attached to and copies (the mannequin), or null. */
	USkeletalMeshComponent* GetBody() const;

	/** Puts on the body of the hero played now (Looter.Hero switched it mid-play); the same body is left as it is. */
	void WearHero();

	/**
	 * Wears Hero's body from now on instead of the hero played (a stand: the hero select's), at once if play has begun.
	 * Null goes back to the hero played.
	 */
	void SetShownHero(const UHeroDefinition* Hero);

	/** Ellis's mesh and the retargeter, found by path once imported (null before): the class's defaults. */
	static USkeletalMesh* FindFigureMesh();
	static UIKRetargeter* FindRetargeter();

	/** A material slot of a holstered gun: its name ends in HolsterGun ("EllisHolsterGun", "<Hero>HolsterGun"). */
	static bool IsHolsterGunSlot(FName SlotName);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The shown hero's (else the hero played's) mesh and retargeter when both are in the game, else Ellis's. */
	void PickBody(USkeletalMesh*& OutMesh, UIKRetargeter*& OutRetargeter) const;
	/** Puts on PickBody's body, hides the mannequin under it, ticks after it, and draws as it is drawn (FollowBody). */
	void Wear();
	/** Drawn as the mannequin would be: hidden to its owner in first person (a world-space stand-in for the shadow). */
	void FollowBody();
	/** The holstered revolver shown unless a revolver is the gun in hand (the user: "empty only while drawn"). */
	void FollowHolster();
	/** Shows or hides every holstered gun's slot (IsHolsterGunSlot) on every LOD. */
	void ShowHolsterGun(bool bShow);

	UPROPERTY(Transient)
	TObjectPtr<UIKRetargeter> Retargeter;

	/** The hero a stand shows (SetShownHero); null: the hero played. */
	UPROPERTY(Transient)
	TObjectPtr<const UHeroDefinition> ShownHero;

	/** The mannequin this hid, shown again if Ellis goes. */
	TWeakObjectPtr<USkeletalMeshComponent> HiddenBody;

	/** The owner's guns (the player's), whose gun in hand empties the holster; looked up once. */
	TWeakObjectPtr<const UWeaponManagerComponent> Weapons;
	bool bLookedForWeapons = false;
	/** The holstered revolver as last shown (it starts shown). */
	bool bHolsterGunShown = true;

public:
	/** SK_Ellis's slot of the holstered revolver (Art/Models/Characters/Ellis.py's GAME_MATERIALS). */
	static const FName HolsterGunSlot;
	/** What every hero's holstered-gun slot ends in (the contract of 2026-10-19: `<Hero>HolsterGun`). */
	static const TCHAR* const HolsterGunSuffix;

	bool IsHolsterGunShown() const { return bHolsterGunShown; }
};
