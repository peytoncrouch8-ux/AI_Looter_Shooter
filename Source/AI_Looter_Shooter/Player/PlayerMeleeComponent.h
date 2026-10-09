#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "Player/PawnInputBinding.h"
#include "Player/PlayerMeleeRules.h"
#include "PlayerMeleeComponent.generated.h"

class AActor;
class AController;
class APawn;
class APlayerController;
class AWeaponBase;
class UWorld;
struct FViewKick;

/** A strike's blow came (hit or miss) on what it hit (null for a miss). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerMelee, bool, bHit, AActor*, Target);
/** A strike landed: as a gun's OnHit tells of a bullet (the hit, its damage, never critical). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerMeleeHit, const FHitResult&, Hit, float, Damage, bool, bCritical);

/**
 * The player's quick melee strike (Docs/Polish/BorderlandsComparison.md, item 16): the Melee key (V, or the right stick's
 * click) swings the gun's stock, or a fist with no gun in hand, at the creature in front. The panic button against a
 * lunging Unpaid or a biting spider: solid damage, a stagger and a hop back that buy room to shoot. FMeleeRules has the
 * numbers and rules, MeleeMotion the swing's motion.
 *
 *  - The swing: 0.45 s, the blow at 0.12 s on the best body within reach and cone (FindStrikeTarget), then 0.6 s from
 *    the press to the next. Never mid-mantle or vault, in a scene, behind a menu, or dead.
 *  - The blow: the damage (UMeleeDamageType, from the player, so the gun in hand counts the kill as guns do for their
 *    holder's blows), the creature's stagger and hit-stop (FHitReaction's melee rules, through an FMeleeScope), the knock
 *    back (never over a drop; none for a boss), impact effects, the thud by what it hit, the hit marker (through the
 *    gun's OnHit with a gun in hand; OnMeleeHit for anyone), the view's jolt and the swing's own hit-stop.
 *  - With a gun in hand: it cuts a reload short (it starts over after), holds the trigger back meanwhile, and swings the
 *    first-person gun (AWeaponBase::SetMeleePose). A sprint keeps its speed: nothing here touches the movement.
 *  - For the tutorial's hint: HasMeleed, OnMelee, and FindTargetInReach (a creature close in front).
 *
 * PlayerMeleeComponent.cpp: life, input, the swing's clock; PlayerMeleeStrike.cpp: finding the body and the blow.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerMeleeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerMeleeComponent();

	/** Pawn's melee component, if it has one. */
	static UPlayerMeleeComponent* Find(const AActor* Pawn);

	/** The Melee key: starts a strike unless something blocks it (GetBlock). True if one started. */
	bool TryMelee();

	/** Why a press right now would start no strike (None: it would). */
	EMeleeBlock GetBlock() const;

	bool IsSwinging() const { return bSwinging; }

	/** Seconds into the swing on its own clock (it holds through the blow's hit-stop); 0 when not swinging. */
	float GetSwingTime() const { return bSwinging ? SwingTime : 0.f; }

	/** The player has struck at least once with this body (the tutorial's hint stops then). */
	UFUNCTION(BlueprintPure, Category = "Melee")
	bool HasMeleed() const { return MeleeCount > 0; }

	/** Strikes started with this body. */
	int32 GetMeleeCount() const { return MeleeCount; }

	/**
	 * What a strike now would land on, with the reach times ReachScale (2: a creature within twice the reach in front, for
	 * a hint to strike); null for nothing. OutContact, if given, says where.
	 */
	AActor* FindTargetInReach(float ReachScale = 1.f, FMeleeContact* OutContact = nullptr) const;

	/** Where a strike from this body comes from now: the eye, the way the player looks, the feet. */
	FMeleeAim GetAim() const;

	/**
	 * The best body a strike from Aim reaches within Reach in World (FMeleeRules::FindContact), seen past no wall: a
	 * living thing with health (a creature, a dummy, an egg sac), never Attacker nor another player. Null for none.
	 */
	static AActor* FindStrikeTarget(const UWorld* World, const FMeleeAim& Aim, float Reach, const AActor* Attacker, FMeleeContact& OutContact);

	/**
	 * Knocks a struck creature back along Away (FMeleeRules::KnockVelocity): a hop that lands on ground, at half the
	 * distance if the full one wouldn't, else none at all (never off an island's edge). Nothing for a boss, a Soulfed
	 * monster, a corpse, a creature held back by a spell, or anything that isn't a creature. Returns the launch given.
	 */
	static FVector KnockBack(AActor& Target, const FVector& Away);

	/** The thud's layer for what a blow lands on: shell for a spider, gel for a slime, flesh for any other creature. */
	static FName HitCueOf(const AActor* Target);

	/** Each strike's blow, hit or miss (the tutorial's hint listens). */
	UPROPERTY(BlueprintAssignable, Category = "Melee")
	FOnPlayerMelee OnMelee;

	/** Each blow that landed on a body (a fist's has no gun to show the hit marker: the HUD can bind here). */
	UPROPERTY(BlueprintAssignable, Category = "Melee")
	FOnPlayerMeleeHit OnMeleeHit;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void SetupInput(AController* Controller);
	void HandleMeleePressed();

	/** What the gate weighs right now. */
	FMeleeGateInput GatherGate() const;

	void StartSwing();
	/** The swing is over (bResume: the gun takes up what it held back; not when it ends early, by death or play ending). */
	void EndSwing(bool bResume);
	/** The gun the swing took leaves it (back in its hold); bResume as EndSwing. */
	void ReleaseWeapon(bool bResume);
	/** The first-person gun where the swing has it now. */
	void UpdateGunPose();

	/** The local player controlling the pawn, or null. */
	APlayerController* GetPlayer() const;
	/** The gun in the pawn's hands, or null. */
	AWeaponBase* GetWeaponInHand() const;
	bool IsOwnerDead() const;
	/** The strike's damage growth at the player's level (FLevelRules::EnemyScale; 1 at level 1 or with no player). */
	float GetLevelScale() const;

	// --- The blow (PlayerMeleeStrike.cpp) ---

	/** The blow, at ContactSeconds into the swing: on the best body in reach, else on a wall, else the air. */
	void Strike();
	/** Lands the blow on Target at Contact. False if it was already dead. */
	bool LandStrike(AActor& Target, const FMeleeContact& Contact, const FMeleeAim& Aim);
	/** Nothing to hit: the stock (or knuckles) on a wall just ahead, if there is one. */
	void StrikeWorld(const FMeleeAim& Aim);
	/** The blow's jolt on the player's view and the swing's hit-stop (Seconds of it). */
	void FeelBlow(const FViewKick& Kick, float Shake, float Seconds);

	FPawnInputBinding InputBinding;

	/** The gun the swing took up (its pose, its held-back reload), while it stays in hand. */
	TWeakObjectPtr<AWeaponBase> SwingWeapon;

	double LastStrikeTime = -1000.0;
	/** The swing's own clock, and how much of the striker's hit-stop is still to hold it. */
	float SwingTime = 0.f;
	float HoldTime = 0.f;
	bool bSwinging = false;
	/** The blow has come. */
	bool bStruck = false;
	/** It met something (a body or a wall): the gun stops there instead of following through. */
	bool bLanded = false;
	/** There was a gun in hand as the swing began (the fist's swing feels different). */
	bool bArmedSwing = false;
	int32 MeleeCount = 0;

	/** Which way each landed blow's jolt rolls. */
	FRandomStream KickRandom{ 0x3e1ee };
};
