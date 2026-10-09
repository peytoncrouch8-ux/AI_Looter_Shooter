#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "Player/PawnInputBinding.h"
#include "Player/PlayerThrowRules.h"
#include "PlayerThrowComponent.generated.h"

class AController;
class AGraveSaltGrenade;
class APawn;
class APlayerController;
class AWeaponBase;
class UStaticMeshComponent;
class UWeaponManagerComponent;
struct FGraveSaltBurstResult;
struct FThrowablesSave;

/** Why the grenade count changed (UPlayerThrowComponent::OnGrenadesChanged), so the HUD can say so. */
enum class EGrenadeChange : uint8
{
	/** One left the hand. */
	Thrown,
	/** Found: a pickup run over. */
	PickedUp,
	/** Given: the starting two with the first gun, or anything else that hands them out. */
	Given,
	/** Put back from a saved session (no fanfare). */
	Restored,
	/** The key was pressed with none left (the count didn't change; the HUD flashes it). */
	Denied
};

/** A burst's hit on a body: as a gun's OnHit tells of a bullet (the hit, its damage, never critical), for the hit marker. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGrenadeHit, const FHitResult&, Hit, float, Damage, bool, bCritical);
/** A thrown grenade burst: how many bodies it hurt and how many it killed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrenadeBurst, int32, Hits, int32, Kills);
/** The count changed: the new count, by how much (0 for Denied), and why. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGrenadesChanged, int32 /*Count*/, int32 /*Delta*/, EGrenadeChange /*Why*/);

/**
 * The player's grave-salt grenade (Docs/Polish/BorderlandsComparison.md, item 16): the Grenade key (G, or the right
 * bumper) lobs a stoppered tin of grave salt that bounces, fizzes and bursts in white salt and embers, a crowd answer and
 * a panic button. FThrowRules has the throw's numbers, ThrowMotion its motion, AGraveSaltGrenade the jar in flight and
 * GraveSaltBurst the burst.
 *
 *  - The throw: 0.5 s, the jar leaving the off hand 0.16 s in, 0.8 s from one press to the next. Never mid-mantle or
 *    vault, in a scene, behind a menu, mid-strike or dead; nothing with none left (a soft click and the HUD's counter
 *    flashing instead).
 *  - With a gun in hand: it cuts a reload short (it starts over after, as after a strike), holds the trigger meanwhile,
 *    and dips the first-person gun out of the throw's way (AWeaponBase's melee swing state and pose, reused as they are).
 *    In first person the jar itself is seen coming up from under the view and slung (the arms are hidden).
 *  - The count: three at most, saved with the session (SaveThrowables, RestoreThrowables). The first gun that comes into
 *    the player's hands gives the starting two (the tutorial's rifle, the skip's bullpup, an old save's guns); a pickup
 *    found before that gives its own and unlocks the counter.
 *  - What it reports: OnGrenadesChanged (the HUD's counter), OnGrenadeHit (the hit marker) and OnGrenadeBurst; HasThrown
 *    and CountTargetsAhead serve the tutorial's hint.
 *
 * PlayerThrowComponent.cpp: life, input, the gate and the count; PlayerThrowRelease.cpp: the throw's clock, its motion and
 * the jar's release.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerThrowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerThrowComponent();

	/** Pawn's throw component, if it has one. */
	static UPlayerThrowComponent* Find(const AActor* Pawn);

	/** The Grenade key: starts a throw unless something blocks it (GetBlock). True if one started. */
	bool TryThrow();

	/** Why a press right now would start no throw (None: it would). */
	EThrowBlock GetBlock() const;

	bool IsThrowing() const { return bThrowing; }

	/** Seconds into the throw; 0 when not throwing. */
	float GetThrowTime() const { return bThrowing ? ThrowTime : 0.f; }

	// --- The count ---

	UFUNCTION(BlueprintPure, Category = "Grenade")
	int32 GetGrenades() const { return Grenades; }

	static int32 GetMaxGrenades() { return FThrowRules::MaxGrenades; }

	/** The player has been given grenades: the HUD shows the counter. */
	UFUNCTION(BlueprintPure, Category = "Grenade")
	bool IsUnlocked() const { return bUnlocked; }

	/**
	 * Up to Amount more (held to the most carried), unlocking the counter. Returns how many went in (0 when full).
	 * Why tells the HUD how to show it.
	 */
	int32 AddGrenades(int32 Amount, EGrenadeChange Why = EGrenadeChange::PickedUp);

	/** The starting two, once: when the first gun comes into the player's hands (or a test asks). False if already given. */
	bool GiveStartingGrenades();

	/** What the session keeps of the grenades, and putting it back (no fanfare). */
	void SaveThrowables(FThrowablesSave& OutSave) const;
	void RestoreThrowables(const FThrowablesSave& Save);

	// --- For the tutorial's hint ---

	/** The player has thrown at least once with this body (the hint stops then). */
	UFUNCTION(BlueprintPure, Category = "Grenade")
	bool HasThrown() const { return ThrowCount > 0; }

	/** Throws started with this body. */
	int32 GetThrowCount() const { return ThrowCount; }

	/**
	 * Live creatures within Range (cm) in front of the player, within HalfConeDegrees of the look across the ground and
	 * in sight (no wall between): a crowd worth a grenade when it's two or more.
	 */
	int32 CountTargetsAhead(float Range = 1500.f, float HalfConeDegrees = 35.f) const;

	// --- The burst's report (AGraveSaltGrenade calls it) ---

	/** A grenade this body threw burst: the hit markers and OnGrenadeBurst. */
	void NotifyBurst(const FGraveSaltBurstResult& Result);

	/** Where a throw from this body comes from now: the eye, and the way the player looks. */
	void GetAim(FVector& OutEye, FVector& OutLook) const;

	/** Each burst's hits on bodies, for the HUD's hit marker. */
	UPROPERTY(BlueprintAssignable, Category = "Grenade")
	FOnGrenadeHit OnGrenadeHit;

	/** Each burst of a grenade this body threw. */
	UPROPERTY(BlueprintAssignable, Category = "Grenade")
	FOnGrenadeBurst OnGrenadeBurst;

	/** Every change of the count (and a press with none left). */
	FOnGrenadesChanged OnGrenadesChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	/** The guns carried changed: the first one in the player's hands gives the starting grenades. */
	UFUNCTION()
	void HandleInventoryChanged();

	void SetupInput(AController* Controller);
	void HandleGrenadePressed();
	void BindWeaponManager();

	/** What the gate weighs right now. */
	FThrowGateInput GatherGate() const;

	/** Sets the count and tells whoever listens. */
	void SetGrenades(int32 Count, EGrenadeChange Why);

	// --- The throw (PlayerThrowRelease.cpp) ---

	void StartThrow();
	/** The jar leaves the hand: the real grenade is thrown and the count goes down. */
	void Release();
	/** The throw is over (bResume: the gun takes up what it held back; not when it ends early, by death or play ending). */
	void EndThrow(bool bResume);
	/** The gun the throw took leaves it (back in its hold); bResume as EndThrow. */
	void ReleaseWeapon(bool bResume);
	/** The first-person gun and the jar where the throw has them now. */
	void UpdatePoses();
	/** The jar seen in the off hand in first person, made the first time it's needed (null without a first-person camera). */
	UStaticMeshComponent* GetOrMakeJarModel();
	void HideJarModel();

	/** The local player controlling the pawn, or null. */
	APlayerController* GetPlayer() const;
	/** The gun in the pawn's hands, or null. */
	AWeaponBase* GetWeaponInHand() const;
	bool IsOwnerDead() const;
	/** The burst's damage growth at the player's level (FLevelRules::EnemyScale; 1 at level 1 or with no player). */
	float GetLevelScale() const;

	FPawnInputBinding InputBinding;

	/** The gun the throw took up (its pose, its held-back reload), while it stays in hand. */
	TWeakObjectPtr<AWeaponBase> ThrowWeapon;

	/** The weapon manager it listens to for the first gun. */
	TWeakObjectPtr<UWeaponManagerComponent> BoundManager;

	/** The jar in the off hand, first person only, shown during a throw until it leaves the hand. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> JarModel;

	double LastThrowTime = -1000.0;
	float ThrowTime = 0.f;
	bool bThrowing = false;
	/** The jar has left the hand this throw. */
	bool bReleased = false;
	int32 Grenades = 0;
	bool bUnlocked = false;
	int32 ThrowCount = 0;
};
