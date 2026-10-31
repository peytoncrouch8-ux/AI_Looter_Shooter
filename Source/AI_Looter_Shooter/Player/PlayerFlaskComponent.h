#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/PawnInputBinding.h"
#include "Player/PlayerFlaskRules.h"
#include "PlayerFlaskComponent.generated.h"

class AController;
class APawn;
class APlayerController;
class AWeaponBase;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UPlayerProgressionSubsystem;
class UStaticMesh;
class UStaticMeshComponent;
struct FFlaskSave;

/** Count now, change (+1 filled, -1 drunk, 0 denied/soul), why. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnFlaskChanged, int32 /*Charges*/, int32 /*Delta*/, EFlaskChange /*Why*/);

/**
 * The player's soul flask (Docs/Archive/Work/2026-10-30-SoulFlask/CONTRACT.md; the user's playtest note 2: a way to heal in a
 * fight, boss fights too): the Flask key (H, or left on the D-pad) drinks a charge of soul-light that heals 35% of the most
 * health over a gulp. FFlaskRules has every number, FlaskMotion the motion, FlaskFX the look and sound.
 *
 *  - The drink: 1.1 s from the press, the cork pulled at once (FlaskFX::Uncork), the heal from 0.35 s (FlaskFX::Drink at the
 *    flask's mouth) over 0.8 s through UHealthComponent::Heal; a hit doesn't stop it. The player walks at 0.75 meanwhile
 *    (UMovementSlowComponent). Refused (no charge used) when dead, in a scene, behind a menu, mid-mantle or vault, while
 *    the hands are busy (a throw, a strike, a cast, a swap, a swim), already drinking or within 0.4 s of the last; empty
 *    (FlaskFX::Denied(false)) and full health (FlaskFX::Denied(true)) are said, the rest are quiet.
 *  - With a gun in hand: it holds its fire and cuts a reload short (it starts over after), and dips out of the way, all
 *    as the grenade throw does it (AWeaponBase's melee swing state and pose). Aiming, throwing, striking and ember powers
 *    wait for the drink (their gates ask IsDrinkingActor).
 *  - Seen: the flask in the first-person off hand (on the camera, FlaskMotion's poses; the left hand round it,
 *    FirstPersonHandsFlask.cpp) and in the body's left hand in third person (LooterStancePose::DescribeDrink lifts it).
 *    SM_SoulFlask; a plain stand-in until it's imported. Its liquid's "Glow" is bright with charges in it and dim at none,
 *    its "Fill" the level, when the material offers them.
 *  - Charges: three, full at a new game and at each respawn (RefillAll, the vitals' hook). Soul fills them: each kill the
 *    player makes (UPlayerProgressionSubsystem::OnKill; FFlaskRules::KillSoul) and soul-motes touched at full health
 *    (ASoulMotePickup, FFlaskRules::MoteSoul). Saved with the session (SaveFlask, RestoreFlask; an older save reads full).
 *  - What it reports: OnFlaskChanged (the HUD's pips), IsHealing (the screen's edge glow).
 *
 * PlayerFlaskComponent.cpp: life, input, the gate, the charges and soul, the save, the kills; PlayerFlaskDrink.cpp: the
 * drink's clock, the heal, the gun's dip; PlayerFlaskModels.cpp: the flask seen in either hand.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UPlayerFlaskComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerFlaskComponent();

	/** Pawn's flask, if it has one. */
	static UPlayerFlaskComponent* Find(const AActor* Pawn);

	/** Pawn has a flask and is drinking from it now: the hands are taken (the other actions' gates ask). */
	static bool IsDrinkingActor(const AActor* Pawn);

	/** The Flask key: starts a drink unless something blocks it (GetBlock). True if one started. */
	bool TryDrink();

	/** Why a press right now would start no drink (None: it would). */
	EFlaskBlock GetBlock() const;

	bool IsDrinking() const { return bDrinking; }

	/** Seconds into the drink (0 when not drinking), and that over DrinkSeconds. */
	float GetDrinkTime() const { return bDrinking ? Clock : 0.f; }
	float GetDrinkAlpha() const;

	/** The heal is flowing now (between SipAt and SipAt + HealSeconds; it runs a touch past the drink's own end). */
	bool IsHealing() const;

	int32 GetCharges() const { return Charges; }
	static int32 GetMaxCharges() { return FFlaskRules::MaxCharges; }
	bool IsFull() const { return Charges >= FFlaskRules::MaxCharges; }

	/** Soul toward the next charge (0 to SoulPerCharge; 0 when full), and that as a share. */
	float GetSoul() const { return IsFull() ? 0.f : Soul; }
	/** 0-1 toward the next charge (0 when full). */
	float GetSoulShare() const;

	/** Soul poured in: overflow carries into the next charge, a full flask keeps none. Filled charges play FlaskFX::ChargeFilled. */
	void AddSoul(float Added);

	/** Every charge back (a respawn; FlaskFX::RefilledAll for Refilled), or Why's way (Restored: no fanfare). */
	void RefillAll(EFlaskChange Why = EFlaskChange::Refilled);

	/** What the session keeps of the flask, and putting it back (no fanfare; a save from before the flask reads full). */
	void SaveFlask(FFlaskSave& OutSave) const;
	void RestoreFlask(const FFlaskSave& Save);

	/** The soul a kill of Victim gives now (FFlaskRules::KillSoul: its rank, a boss, a boss fight running), poured in. */
	float GiveKillSoul(const AActor* Victim);

	/** Moves a drink on by DeltaSeconds: its tick calls it; a test level, which doesn't tick, calls it directly. */
	void Advance(float DeltaSeconds);

	/** Drinks started with this body. */
	int32 GetDrinkCount() const { return DrinkCount; }

	/** Where the flask's mouth is in the world now: the model in hand (its SOCKET_Mouth), else just under the eye. */
	FVector GetMouthLocation() const;

	/** The flask's model and what the code reads on it (FX's SM_SoulFlask, Art/Models/Items/SoulFlask.py). */
	static const TCHAR* FlaskMeshPath() { return TEXT("/Game/Art/Items/SM_SoulFlask.SM_SoulFlask"); }
	static FName MouthSocket() { return TEXT("SOCKET_Mouth"); }
	static FName GlowParameter() { return TEXT("Glow"); }
	static FName FillParameter() { return TEXT("Fill"); }

	/** The flask in the first-person off hand, and in the body's left hand (null until the first drink makes them). */
	UStaticMeshComponent* GetHandModel() const { return HandModel; }
	UStaticMeshComponent* GetBodyModel() const { return BodyModel; }

	/** Every change of the count (and a press refused for empty or full health). */
	FOnFlaskChanged OnFlaskChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void SetupInput(AController* Controller);
	void HandleFlaskPressed();

	/** Listens for the player's kills (their local player's progression), once there is one. */
	void BindProgression();
	void UnbindProgression();
	void HandleKill(const AActor* Victim);

	/** What the gate weighs right now. */
	FFlaskGateInput GatherGate() const;

	// --- The drink (PlayerFlaskDrink.cpp) ---

	void StartDrink();
	/** The first swallow (SipAt): the soul's rush at the flask's mouth, the view tipping back. */
	void Sip();
	/** The heal up to Share (0-1) of the drink's whole heal. */
	void HealTo(float Share);
	/** The hands' part is over (bResume: the gun takes up what it held back; not when it ends early, by death or play ending). */
	void EndDrink(bool bResume);
	/** The gun the drink took up: lock it (its swing state), and let it go (bResume as EndDrink). */
	void TakeWeapon(AWeaponBase* Gun);
	void ReleaseWeapon(bool bResume);
	/** The gun's dip and the flask's poses where the drink has them now. */
	void UpdatePoses();

	// --- The flask seen (PlayerFlaskModels.cpp) ---

	/** The flask mesh (SM_SoulFlask, or the engine's cylinder standing in), loaded once. */
	UStaticMesh* LoadFlaskMesh();
	/** A flask model: no collision, no shadow of its own, the glow's dynamic materials made. */
	UStaticMeshComponent* MakeModel(USceneComponent* Parent, FName Socket, const TCHAR* Name);
	/** The first-person flask on the camera, made the first time it's needed (null without a first-person camera). */
	UStaticMeshComponent* GetOrMakeHandModel();
	/** The third-person flask in the body's left hand, made the first time it's needed (null without a body). */
	UStaticMeshComponent* GetOrMakeBodyModel();
	/** Places the first-person flask on its pose and shows or hides both as the view and the drink say. */
	void UpdateModels();
	void HideModels();
	/** The liquid's Glow and Fill on both models, from the charges and the drink. */
	void UpdateGlow();
	/** Where a model's mouth is in the world. */
	FVector MouthOfModel(const UStaticMeshComponent& Model) const;

	/** The local player controlling the pawn, or null. */
	APlayerController* GetPlayer() const;
	/** The gun in the pawn's hands, or null. */
	AWeaponBase* GetWeaponInHand() const;
	UHealthComponent* GetHealth() const;
	/** Sets the count and tells whoever listens (always for Restored and the denials, else only on a change). */
	void SetCharges(int32 Count, EFlaskChange Why);

	FPawnInputBinding InputBinding;

	/** The local player's progression whose kills it listens to. */
	TWeakObjectPtr<UPlayerProgressionSubsystem> Progression;
	FDelegateHandle KillHandle;

	/** The gun the drink took up (its pose, its held-back reload), while it stays in hand. */
	TWeakObjectPtr<AWeaponBase> DrinkWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HandModel;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> BodyModel;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FlaskMesh;

	/** Each model's liquid materials (those with a Glow), and their own Glow, which the code scales. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GlowMaterials;
	TArray<float> BaseGlows;

	/** The mesh is the cylinder standing in (its pivot is its middle, not its foot). */
	bool bStandIn = false;
	bool bMeshLooked = false;

	/** The drink's clock: from the press until the heal has all gone in (a touch past the hands' part). */
	float Clock = 0.f;
	bool bDrinking = false;
	bool bPouring = false;
	bool bSipped = false;
	/** The whole heal of this drink (HealShare of the most health at the press), and the share of it given so far. */
	float HealAmount = 0.f;
	float HealedShare = 0.f;
	double LastDrinkEnd = -1000.0;

	int32 Charges = FFlaskRules::MaxCharges;
	float Soul = 0.f;
	int32 DrinkCount = 0;
};
