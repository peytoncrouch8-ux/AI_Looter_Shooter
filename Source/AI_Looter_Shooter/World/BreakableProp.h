#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/BreakableKinds.h"
#include "BreakableProp.generated.h"

class AController;
class UHealthComponent;
class ULootTable;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * A wooden crate or barrel that breaks (Docs/Polish/BorderlandsComparison.md, item 10: "every corner should offer
 * something"): a few shots or one melee strike, and it bursts into its pieces (modeled broken in advance,
 * Art/Models/Props/Lootables.py; UBreakableDebrisSubsystem throws them), with the crack of its wood (its kind's cue), dust
 * and splinters, leaving its stump standing. Sometimes something was in it: an ammo pickup (the default loot table's
 * classes at a kill's 18-36 rounds, leaning toward the gun that broke it, through ULootLibrary::SpawnKillLoot) or a
 * soul-mote. A hit that doesn't break it rocks it away from the blow.
 *
 * Its body is the dressing's own model (Crate_A, Crate_B, Barrel_A): Tools/Unreal/build_area_loot.py turns some of the
 * dressing's instances into breakables where they stood, and stands more of them about the areas. Solid like a chest
 * (BlockAllDynamic, world dynamic: loot and ground traces pass through it), so shots and the melee strike find it through
 * its health; tagged Breakable and Obstacle, no damage numbers.
 *
 * Broken stays broken: the session keeps the broken ones with the map's world by BreakableId
 * (FSavedMapWorld::BrokenProps) and puts them back as stumps (RestoreBroken), giving nothing again. It never ticks but for
 * the moment a hit rocks it; its pieces are the debris subsystem's.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABreakableProp : public AActor
{
	GENERATED_BODY()

public:
	ABreakableProp();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Breaks it as the blow that empties its health does: the pieces thrown (along Blow, any length), the sound, the dust,
	 * the stump, the loot (as broken by By). False when it's broken already.
	 */
	UFUNCTION(BlueprintCallable, Category = "Breakable")
	bool Break(AController* By, FVector Blow);

	/** Broken as the session keeps it (USessionSubsystem::RestoreWorld): the stump at once, no pieces, no sound, no loot. */
	void RestoreBroken();

	/** Whole again and at full health (the console): what it gave stays where it lies. */
	void Mend();

	UFUNCTION(BlueprintPure, Category = "Breakable")
	bool IsBroken() const { return bBroken; }

	/** Moves its rocking on by DeltaSeconds (the tick does while it rocks; tests call it). */
	void Advance(float DeltaSeconds);

	/** How far a hit has it rocked now (degrees). */
	float GetShudder() const { return ShudderAngle; }

	/** What the session keeps it by: BreakableId, else its name in the level. */
	FName GetSaveKey() const;

	FBreakableKindInfo GetKindInfo() const { return FBreakableKindInfo::Get(Kind); }

	/**
	 * What a break gives, as a loot table: the default table's ammo classes, one pickup at its kind's chance at a kill's
	 * rounds, leaning toward the gun that broke it, and never a gun. A new transient table each call.
	 */
	ULootTable* MakeLootTable() const;

	/** The loot its break dropped, while it's still in the world. */
	TArray<AActor*> GetDroppedLoot() const;

	/** The whole prop: the dressing's model, its pivot on the ground at the middle of its footprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	/** What's left standing once it's broken (its kind's stump), shown only then; nothing collides with it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Stump;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable")
	EBreakableKind Kind = EBreakableKind::SlattedCrate;

	/** Its stable id (the build script sets it: "Breakable_RansomsRest_012"), which the session keeps it broken by. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable")
	FName BreakableId;

	/** Whether its break can leave ammo or a soul-mote at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breakable")
	bool bDropsLoot = true;

	/**
	 * Its pieces and stump, its kind's, held here as it's built so the level refers to them: they load with the level
	 * (no hitch at the first break) and a packaged game cooks them.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Breakable")
	TArray<TObjectPtr<UStaticMesh>> PieceMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Breakable")
	TObjectPtr<UStaticMesh> StumpMesh;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** Its kind's models: the body (unless the build gave it one), the pieces and the stump. */
	void ApplyKind();

	/** Hides the body and turns its collision off, shows the stump: broken, as a break or the session leaves it. */
	void ShowBroken();

	/** The pieces out of where they stood, the dust and the splinters, the sound. */
	void Burst(const FVector& Blow);

	/** Rolls and throws what it held (ammo, a soul-mote), once. */
	void DropLoot(AController* By);

	/** The break's middle: halfway up it. */
	FVector GetMiddle() const;

	/** Its transform as it stands at rest (a hit rocks it away from this and back). */
	FTransform RestTransform = FTransform::Identity;
	bool bRestCaptured = false;

	/** The latest hit's way (for the rock and the break's throw). */
	FVector LastBlow = FVector::ZeroVector;

	float ShudderAngle = 0.f;
	float ShudderClock = 0.f;
	FVector ShudderAxis = FVector::RightVector;

	bool bBroken = false;
	bool bLootGiven = false;

	TArray<TWeakObjectPtr<AActor>> DroppedLoot;
};
