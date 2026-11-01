#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "CompanyHoseCart.generated.h"

class AController;
class UAudioComponent;
class UCapsuleComponent;
class UDamageType;
class UMissionRunner;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStoryLineSet;

/** Where a hose-cart stands in its story. */
UENUM(BlueprintType)
enum class EHoseCartState : uint8
{
	/** Not in the level yet (before its story). Hidden, nothing to hit or use. */
	Away,
	/** Pumping: its hose in a body, its tank filling; the valve can be shut and the tank burst. */
	Pumping,
	/** Stopped: the valve shut or the tank burst, its spill on the ground (and so for good once its story is past it). */
	Stopped,
};

/**
 * A Vane Consolidated hose-cart (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md, Main 2 step 1 and Main 3's Main Street): a
 * two-wheeled iron pump cart with a riveted tank, a valve wheel, a hose reel and a black hose run into what it drinks (the
 * week's shrouded dead on the burial boards; the Gilt Spur's door). The reason for a fight: while it pumps, the dead on the
 * boards are being drunk into it.
 *
 * Stopped two ways, each a feeling of its own: shut its valve (hold Interact, ValveHoldSeconds, under fire: the wheel
 * clanks shut), or burst its tank (TankHealth of shots on the tank: a crit spot like the hose-man's, a soul-cyan blast that
 * hurts whoever stands at it). Either way it spills what it drank: a black spill on the ground and a cough of smog, the
 * missions hear StoppedEvent about it (Main 2's "Stop the hose-cart"), Vane complains from a loudspeaker (LoudspeakerTag,
 * StopBroadcast: "That cart is Company property."), and the crew's spawner (CrewSpawnerId) brings its next wave at the
 * spill (SpillSpots): the half-dredged dead the hose leaves, rising out of the black (Hob: "That's what the hose leaves,
 * sunshine. Half a soul and a temper.").
 *
 * What it shows follows the story, not the save: away until ShownWhen holds, pumping while it does (stoppable only while
 * PumpingWhen holds, the step that listens for it), stopped from the start while any of StoppedWhen holds (Main 2 past
 * the cart). A session loaded mid-fight finds it pumping again, as the step's count starts over with it. It ticks only near the player, while it pumps (the hose's pulse) or its spill spreads.
 *
 * Stand-in look: engine shapes in the game's iron, wood, brass and hose-rubber materials (paths in the .cpp); ModelPath (the
 * Tripo hose-cart, TRIPO_MODELS.md "Mains 2-3") replaces the cart, tank, reel and valve once it's imported. Tagged
 * HoseCartTag and Obstacle (the minimap; cover for the fight).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACompanyHoseCart : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ACompanyHoseCart();

	/** Every hose-cart carries it; the build adds its own (HoseCart_Boards, HoseCart_GiltSpur). */
	static const FName HoseCartTag;

	/** What the missions hear as one is stopped, about it. */
	static const FName StoppedEvent;

	/** The Tripo model (one path): used in place of the stand-in's cart, tank, reel and valve once it's in the project. */
	static const TCHAR* const ModelPath;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Stops it as the valve's hold or the tank's last shot does (CompanyHoseCartSpill.cpp): By is who did it; bBurst, the
	 * tank burst (the blast) rather than the valve shut. The spill, the missions told, Vane's complaint, the crew's next
	 * wave. False unless it's pumping.
	 */
	bool StopPumping(AController* By, bool bBurst);

	EHoseCartState GetCartState() const { return State; }

	/** It's pumping and its story lets it be stopped now (PumpingWhen): the valve turns, the tank can burst. */
	bool IsLive() const { return State == EHoseCartState::Pumping && bLive; }

	/** It was stopped by bursting its tank (this visit). */
	bool WasBurst() const { return bWasBurst; }

	/** Damage its tank has taken this visit, and what bursts it. */
	float GetTankDamage() const { return TankDamage; }

	/** Reads its story again: away, pumping or stopped (the missions' changes call it; so can tests). */
	void RefreshStory();

	/** The valve wheel's middle and the tank's, in the world. */
	FVector GetValveLocation() const;
	FVector GetTankCenter() const;

	// --- Its parts ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The cart's bed and wheels (what the player and the fight's traces bump into). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WheelLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> WheelRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Tank;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Valve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Reel;

	/** The hose from the reel to HoseEnd, in two lengths sagging to the ground between. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HoseNear;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HoseFar;

	/** The Tripo cart, once it's in (hidden otherwise). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Model;

	/** What shots on the tank hit (only the Weapon channel). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> TankHit;

	/** The black spill, shown once it's stopped (it spreads over a second). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Spill;

	// --- Its story ---

	/**
	 * It stands there pumping while this holds (Main 2: from its first step, so the player sees the crew at work as they
	 * come up the cairns). Empty: while PumpingWhen holds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	FStoryCondition ShownWhen;

	/**
	 * It can be stopped while this holds (Main 2: its cart step only, so the step that listens for the stop is the one being
	 * played; before it the valve won't turn and the tank holds whatever it takes). An empty condition: always.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	FStoryCondition PumpingWhen;

	/** It stands stopped, spilled, from the start while any of these holds (its story past it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	TArray<FStoryCondition> StoppedWhen;

	/** Where its hose's nozzle lies, relative to the cart (a body on a bier, a saloon's door). Zero: on the ground in front. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart", meta = (MakeEditWidget = true))
	FVector HoseEnd = FVector::ZeroVector;

	/** The spawner (by id) whose next wave its spill brings: the crew's (BoardsCrew). None: nothing rises. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	FName CrewSpawnerId;

	/** Where the spilled dead rise, relative to the cart (the crew spawner's spots for that wave). Empty: the spawner's own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart", meta = (MakeEditWidget = true))
	TArray<FVector> SpillSpots;

	/** The loudspeaker (by tag) Vane complains from as it's stopped, and his words. None: he says nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	FName LoudspeakerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	TObjectPtr<UStoryLineSet> StopBroadcast;

	/** Said the moment it's stopped (Hob naming what rises out of the spill); Vane's complaint waits until they're done. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	TObjectPtr<UStoryLineSet> SpillLines;

	/** Vane's complaint waits at most this long (s) for the spill's words to end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart", meta = (ClampMin = "0", Units = "s"))
	float ComplainWaitSeconds = 4.f;

	/** Interact is held this long (s) to shut the valve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart", meta = (ClampMin = "0.1", Units = "s"))
	float ValveHoldSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart")
	FText ValvePrompt;

	/** The damage its tank takes before it bursts (about a magazine from a gun of the area's band). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart", meta = (ClampMin = "1"))
	float TankHealth = 150.f;

	/** A burst tank hurts creatures within this (cm) by this share of their health (the player is spared: it's their shot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart", meta = (ClampMin = "0", Units = "cm"))
	float BurstRadius = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose Cart", meta = (ClampMin = "0", ClampMax = "1"))
	float BurstShare = 0.35f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Stretches the hose from the reel to its end, sagging between (Pulse: its swell while pumping, 1 at rest). */
	void PlaceHose(float Pulse);

	/** Shows what State asks: hidden when away, the spill and the slack hose when stopped. */
	void ShowState();

	/** The player near enough for its pulse and its pump's sound (cm). */
	bool IsPlayerNear(double Within) const;

	void HandleRunnerChanged();

	UFUNCTION()
	void HandlePointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
		UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType,
		AActor* DamageCauser);

	// --- The stop (CompanyHoseCartSpill.cpp) ---
	/** The tank's blast: the soul let go, the shake, the hurt round it. */
	void BurstTank(AController* By);
	/** The crew's spawner brings its next wave at the spill. */
	void RaiseSpill();
	/** Vane's complaint from the loudspeaker. */
	void Complain();
	/** Smog coughing up off the spill. */
	void CoughSmog();

	EHoseCartState State = EHoseCartState::Away;
	bool bLive = false;
	bool bWasBurst = false;
	/** Stopped this visit by the player (so the story's later look keeps it stopped, spilled where it was). */
	bool bStoppedHere = false;
	float TankDamage = 0.f;
	float PulseClock = 0.f;
	/** How far its spill has spread (0-1). */
	float SpillSpread = 1.f;
	/** Vane's complaint is still to come (after the spill's words), and how long it has waited (s). */
	bool bComplainPending = false;
	float ComplainWaited = 0.f;
	TWeakObjectPtr<UAudioComponent> PumpSound;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
