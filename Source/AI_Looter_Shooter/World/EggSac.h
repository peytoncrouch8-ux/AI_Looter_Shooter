#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/HuntingGround.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "EggSac.generated.h"

class ACreatureBase;
class AController;
class APawn;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;

/** Where an egg sac stands. */
UENUM(BlueprintType)
enum class EEggSacState : uint8
{
	/** Up where it was spun: in its sling against the wall, or hanging from its silk. */
	Hanging,
	/** Shot down, dropping to the ground under it. */
	Falling,
	/** On the ground, hatched: the burst sac, its two spiders out (or long gone, when the story is past it). */
	Burst,
};

/**
 * An egg sac in the Sink (Docs/Areas/RansomsRest.md, Main 5 "The Keeper's Lantern": "Shoot down the three egg sacs (each
 * lets out 2 spiders on the floor)"; "The egg sacs are shootable props modeled on the existing ATargetDummy, not
 * enemies"). Its model is one of Art/Models/Props/Sink.py's intact sacs (SM_EggSac_A or _C hanging from SOCKET_Silk on
 * web lines, SM_EggSac_B in its Web_Sling against a wall), its pivot at the bottom of the sac; the build script
 * (Tools/Unreal/build_area_sink.py) sets which and hangs it.
 *
 * Shots hit its hulls (the Weapon channel) and hurt it while ShootableWhen holds (Main 5's second step); before and after,
 * it takes no damage at all. Shot down, it drops to the ground under it (a fall worked out in code, no physics), lands,
 * becomes SM_EggSac_Burst lying with its torn mouth along the actor's front (+X: the build turns it away from the wall),
 * and lets out its spiders at the burst sac's SOCKET_Spawn_1 and _2, on the ground in front of the mouth: Basic
 * ASpiderCreatures spawned in play, gone for good once killed (ACreatureBase::SpawnAtRuntime), fighting on SpiderGround
 * (the Sink's floor, so they give up at the foot of the ramp) and coming for whoever shot it. As it lands the missions
 * hear BurstEvent about it (Main 5's step counts three).
 *
 * What it shows follows the story, not the save: while any of DownWhen holds (Main 5 past its sacs, and after Main 5) it
 * lies burst from the start, empty, with no spiders and nothing told; before it hangs intact. A session loaded in the
 * middle of the step finds all three up again, as the step's count starts over with it (the runner keeps a mission's step,
 * not its objectives' counts). The sling or the lines it hung from are their own actors and stay. It ticks only while it
 * falls.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AEggSac : public AActor
{
	GENERATED_BODY()

public:
	/** The tag missions find the sacs by (Main 5's arrow points at the nearest one still up). */
	static const FName EggSacTag;

	/** What a sac landing sends the missions: Main 5's second step counts three. */
	static const FName BurstEvent;

	AEggSac();

	/** Shots hurt it only while it can be shot (CanBeShot): before and after its step, nothing at all. */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** It hangs, and its story lets it be shot now (an empty ShootableWhen: any time), or a shot is being forced. */
	UFUNCTION(BlueprintPure, Category = "Egg Sac")
	bool CanBeShot() const;

	/** One of DownWhen holds: the story is past it, and it lies burst from the start. */
	UFUNCTION(BlueprintPure, Category = "Egg Sac")
	bool IsStoryDown() const;

	/**
	 * Shoots it down as a killing shot from By would (the console): the fall, the burst, the spiders, the missions told.
	 * bForce does it whatever ShootableWhen says. False when it isn't hanging, or can't be shot now.
	 */
	bool ShootDown(AController* By, bool bForce = false);

	/** Hangs it up again, intact and at full health (the console, after a step started over); its spiders stay out. */
	void Hang();

	/** Reads its story again: down from the start once the story is past it, up again when the story went back before it. */
	void RefreshStory();

	/** Moves its fall on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	EEggSacState GetState() const { return State; }

	/** It's down: falling or burst on the ground. */
	bool IsDown() const { return State != EEggSacState::Hanging; }

	/** The spiders it let out (dead ones and ones taken away drop to null). */
	TArray<ACreatureBase*> GetSpiders() const;

	/** Where the burst sac lies (or will): the ground under it, out along its front by BurstOut. */
	FVector GetLandingSpot() const;

	/** Where its spiders come out, on the ground in front of the burst sac's mouth: its Spawn sockets, else its model's spots. */
	TArray<FTransform> GetSpawnSpots() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The intact sac (SM_EggSac_A, _B or _C, set by the build script), its pivot at its bottom; shots hit its hulls. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Sac;

	/** The hatched sac it becomes on the ground (SM_EggSac_Burst), its torn mouth along the actor's front. Hidden until then. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Burst;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> Health;

	// --- When ---

	/** It can be shot only while this holds (Main 5's second step). Empty: any time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Story")
	FStoryCondition ShootableWhen;

	/** While any of these holds it lies burst from the start, empty (Main 5 past its sacs; after Main 5). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Story")
	TArray<FStoryCondition> DownWhen;

	// --- Its spiders ---

	/** What comes out of it (Docs/Areas/RansomsRest.md: "each lets out 2 spiders on the floor"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Spiders")
	TSubclassOf<ACreatureBase> SpiderClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Spiders")
	ECreatureRank SpiderRank = ECreatureRank::Basic;

	/** Their size against their class's; 0 keeps the class's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Spiders", meta = (ClampMin = "0", ClampMax = "5"))
	float SpiderScale = 0.f;

	/** The burst sac's sockets they come out at, one spider each. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Spiders")
	TArray<FName> SpawnSockets;

	/** Where they fight (the Sink's floor, its height band keeping them off the ramp and the rim). Unset: anywhere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Spiders")
	FHuntingGround SpiderGround;

	/** Tags they carry, for missions that count them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Spiders")
	TArray<FName> SpiderTags;

	// --- The fall ---

	/**
	 * How far it drops to the ground (cm) when a trace finds none under it (the build script measures it; a test level's
	 * traces find nothing). In play the ground under it decides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Fall", meta = (ClampMin = "0", Units = "cm"))
	float DropHeight = 300.f;

	/** The burst sac lies this far out along its front from under the sac (cm): off a wall the sling held it against. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Fall", meta = (Units = "cm"))
	float BurstOut = 0.f;

	/** How fast it falls (cm/s each second): a little more than the world's, so a short drop still reads as a drop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Fall", meta = (ClampMin = "100"))
	float FallGravity = 1600.f;

	/** It tips forward this far (degrees) as it falls, out of its sling or off its silk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Egg Sac|Fall", meta = (ClampMin = "0", ClampMax = "90"))
	float FallTip = 25.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	void HandleMissionsChanged();

	/** A sac's body solid to shots, the Interact key's line and pawns, or not there for anything. */
	static void SetSolid(UStaticMeshComponent& Mesh, bool bSolid);

	/** Remembers where the sac hangs, the first time it's needed. */
	void CaptureRest();

	/** Shows it hanging intact (and solid), the burst sac put away. */
	void ShowHanging();

	/** Starts the drop from where it hangs. */
	void StartFall();

	/** It's on the ground: the burst sac, its spiders (bHatch), the missions told (bHatch). */
	void Land(bool bHatch);

	/** Lets its spiders out at the burst sac's spawn spots, coming for Target. */
	void Hatch(APawn* Target);

	/** The ground's height under Point (world-static only, past itself), or unset. */
	TOptional<double> FindGroundUnder(const FVector& Point, double Above, double Below) const;

	/** Whom its spiders come for: whoever shot it, else the level's player. */
	APawn* FindTarget() const;

	/** A quick glow over the sac (an overlay material), fading out, as the dummy flashes. */
	void FlashHit(bool bCritical);
	void UpdateHitFlash();

	EEggSacState State = EEggSacState::Hanging;

	/** Where the sac hangs, relative to the actor. */
	FTransform SacRest = FTransform::Identity;
	bool bRestCaptured = false;

	/** The fall: where it started (world), the ground it lands on (Z), how far it has dropped and how fast it drops now. */
	FVector FallStart = FVector::ZeroVector;
	double LandZ = 0.0;
	double Fallen = 0.0;
	double FallSpeed = 0.0;

	/** A shot is being forced through (ShootDown with bForce). */
	bool bForceShot = false;

	TWeakObjectPtr<AController> ShotBy;
	TArray<TWeakObjectPtr<ACreatureBase>> Spiders;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HitFlashMaterial;

	FTimerHandle HitFlashTimer;
	double HitFlashStart = 0.0;
	float HitFlashStrength = 0.f;
};
