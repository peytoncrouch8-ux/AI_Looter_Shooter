#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Embers/EmberTypes.h"
#include "Player/PawnInputBinding.h"
#include "EmberComponent.generated.h"

class AController;
class APawn;
class UEmberDefinition;
class UEmberPower;
class UHeroDefinition;
struct FEmberContext;

/**
 * The player's ember powers (Docs/Story.md, "The heroes"; the user's calls, 2026-10-15): the hero owns three from the
 * start and equips one at a time; the Ember key (Q by default, the left bumper on a gamepad; rebindable:
 * UKeyBindingSubsystem::GetEmberAction) fires the equipped one, which runs for its duration and then cools down. The
 * powers page (the inventory's second tab) swaps the equipped one. Made in C++ on ALooterCharacter like the grenade.
 *
 *  - The hero: on the player's character the hero played (the session's pick, USessionSubsystem::FindPlayedHero);
 *    otherwise HeroDefinition, or the default (DA_Hero_Ellis) when the Blueprint sets none. Each owned power with code
 *    gets its UEmberPower made once (Powers, by the same index as the hero's list); one still without code can't fire.
 *  - The gate (TryUse): nothing dead, in a scene, behind a menu, mid-mantle, mid-swap, while a power runs or cools down;
 *    then the power's own CanActivate. A denied press tells the HUD why (OnDenied: the socket shakes for Cooldown and
 *    Active, clicks for the rest).
 *  - The clock: ActiveLeft counts the duration down (Tick runs the power), CooldownLeft counts from the use, so a long
 *    power can be ready again soon after it ends. Both tick in the component's own tick, in game time.
 *  - Saving: the equipped power's id and its cooldown left (FEmberSave; SaveEmbers, RestoreEmbers; the session's
 *    Embers field). A running power isn't saved: a load finds it ended.
 *  - Reports: OnEquipped, OnStateChanged, OnUsed, OnDenied, for the HUD's socket and the page.
 *  - Console: Looter.Embers.Use [id] fires the equipped (or the named) power past the cooldown; Looter.Embers.Equip <id>.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UEmberComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEmberComponent();

	/** Pawn's ember component, if it has one. */
	static UEmberComponent* Find(const AActor* Pawn);

	/** The default hero when the Blueprint sets none: /Game/Data/Embers/DA_Hero_Ellis. */
	static const TCHAR* DefaultHeroPath();

	// --- The hero and the powers owned ---

	const UHeroDefinition* GetHero() const { return Hero; }

	/**
	 * Binds the hero again (Looter.Hero switched it mid-play): a running power is cut short, the new hero's powers are
	 * made and their first equipped, ready.
	 */
	void RebindHero();

	const TArray<TObjectPtr<UEmberDefinition>>& GetOwned() const;
	int32 NumOwned() const { return GetOwned().Num(); }

	/** The equipped power's data, or null with none. */
	const UEmberDefinition* GetEquipped() const;
	int32 GetEquippedIndex() const { return EquippedIndex; }
	FName GetEquippedId() const;

	/** Equips an owned power by id or index (nothing while one runs: it finishes first). True if it changed. */
	bool Equip(FName Id);
	bool EquipIndex(int32 Index);

	// --- Using it ---

	/**
	 * The Ember key: fires the equipped power unless something blocks it (returns why; None: it fired, or will at its
	 * cast's release, CastLeadSeconds later, while the first-person hands show the cast).
	 */
	EEmberDenied TryUse();

	/** Why a press right now would do nothing (None: it would fire). */
	EEmberDenied GetBlock() const;

	EEmberState GetState() const { return State; }
	bool IsActive() const { return State == EEmberState::Active; }
	bool IsReady() const { return State == EEmberState::Ready; }

	/** Seconds left of the cooldown (0 when ready) and its share gone (0 ready, 1 just used), for the socket's pie. */
	float GetCooldownLeft() const { return CooldownLeft; }
	float GetCooldownFraction() const;

	/** Seconds left of the running power (0 when none) and its share left (1 just started), for the socket's arc. */
	float GetActiveLeft() const { return ActiveLeft; }
	float GetActiveFraction() const;

	/** The running power's object (null when none), for the HUD's specific readouts (Borrowed Time's debt). */
	UEmberPower* GetActivePower() const;
	UEmberPower* GetPowerFor(FName Id) const;

	/** FLevelRules::EnemyScale at the player's level (1 at level 1 or with no player): what a power multiplies its damage by. */
	float GetLevelScale() const;

	/** Ends the running power now (death, a scene, the level ending): End(bInterrupted). A cast waiting to fire is dropped. */
	void Interrupt();

	// --- The cast (EmberComponentCast.cpp; the user's note 27, 2026-10-27: "action skill activation animation") ---

	/**
	 * The hands' cast is under way (a press waiting out its lead, or the first-person hands still in the cast's pose):
	 * the left hand is off the gun, so a melee strike waits (UPlayerMeleeComponent's gate).
	 */
	bool IsCasting() const;

	/** Seconds since the cast began (the press; for a console use, its fire less no lead), or -1 when none is showing. */
	float GetCastTime() const;

	/** The lead that cast waited before its power fired (0: it fired at the press). */
	float GetCastLead() const { return CastLead; }

	/** The power cast (its id): the hands' pose by its kind (FirstPersonHandsMotion::CastKindOf). */
	FName GetCastId() const { return CastId; }

	/**
	 * How long a press waits before its power fires while the first-person hands show the cast (raise, flare, release:
	 * the power goes off at the release): Looter.Embers.CastLead, never over MaxCastLead. None in third person or for a body
	 * no local player controls (the tests' bare bodies fire at once).
	 */
	static float CastLeadSeconds();
	static constexpr float MaxCastLead = 0.15f;

	/**
	 * Ends the running power now as if its time had run out (End, not interrupted): a power whose work is done early (Last
	 * Rites' mark settled by a death) calls it from its Tick, so the HUD's arc stops with it. Its cooldown runs on as ever.
	 */
	void Finish();

	// --- Saving ---

	void SaveEmbers(FEmberSave& OutSave) const;
	void RestoreEmbers(const FEmberSave& Save);

	// --- Reports ---

	FOnEmberEquipped OnEquipped;
	FOnEmberStateChanged OnStateChanged;
	FOnEmberUsed OnUsed;
	FOnEmberDenied OnDenied;

	/** The hero whose powers these are when the owner isn't the player's character (the Blueprint may set it; DefaultHeroPath when none). */
	UPROPERTY(EditDefaultsOnly, Category = "Embers")
	TObjectPtr<UHeroDefinition> HeroDefinition;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void SetupInput(AController* Controller);
	void HandleEmberPressed();

	/** Loads the hero (FindHeroToBind) and makes a power object per owned power (EmberComponentHero.cpp). */
	void BindHero();
	/** The player's character: the hero played (USessionSubsystem::FindPlayedHero); else the Blueprint's, else Ellis's. */
	const UHeroDefinition* FindHeroToBind() const;
	FEmberContext MakeContext() const;
	void SetState(EEmberState NewState);
	/** Fires power Index now (the gate already passed, or a console command): starts its clocks. */
	void Fire(int32 Index);
	void EndActive(bool bInterrupted);
	bool IsOwnerDead() const;

	/** The gate passed for power Index: the cast begins, and the power fires now or after the lead (EmberComponentCast.cpp). */
	void BeginCast(int32 Index);
	/** The lead counting down: fires the waiting power at its end, or drops it (death, a scene, the power's own say). */
	void TickCast(float DeltaTime);
	/** A waiting cast dropped (it never fired). */
	void CancelCast();
	/** A power fired with no cast before it (a console use): the hands' clock starts at its release. */
	void NoteFired(FName Id);
	/** The press should wait out the lead: a local player in first person. */
	bool ShouldLead() const;

	/** The power waiting out the cast's lead (Powers' index), and the lead left. */
	int32 PendingIndex = INDEX_NONE;
	float PendingLeft = 0.f;
	/** When the cast began (world seconds), its lead, and the power cast. */
	double CastStart = -1000.0;
	float CastLead = 0.f;
	FName CastId;

	FPawnInputBinding InputBinding;

	UPROPERTY(Transient)
	TObjectPtr<const UHeroDefinition> Hero;

	/** One per owned power, by the hero's index. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UEmberPower>> Powers;

	int32 EquippedIndex = INDEX_NONE;
	/** The power running (Powers' index), or INDEX_NONE. */
	int32 ActiveIndex = INDEX_NONE;
	EEmberState State = EEmberState::Ready;
	float CooldownLeft = 0.f;
	float CooldownTotal = 0.f;
	float ActiveLeft = 0.f;
	float ActiveTotal = 0.f;
};
