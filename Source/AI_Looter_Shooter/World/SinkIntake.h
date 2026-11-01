#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "World/SinkIntakeRules.h"
#include "SinkIntake.generated.h"

class AIntakeCharge;
class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionRunner;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UStoryLineSet;

/** Where the intake is in its story. */
UENUM(BlueprintType)
enum class EIntakeState : uint8
{
	/** Drinking the Sink's floor through Landfall's hose (it draws only in its step). */
	Drinking,
	/** Three charges on its collar, the fuse burning: its draw doubled. */
	Fused,
	/** Blown: the hose bucking, then swinging across the floor. */
	Swinging,
	/** Choked: the hose hanging limp from the drum and lying on the floor. */
	Limp,
	/** The barge has fallen: nothing here (after Main 6). */
	Gone,
};

/**
 * The barge's intake on the Sink's floor and the set piece round it (Main 5 "Choke Point", "the Intake":
 * Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 7), the mission's climax. Landfall's baked hose drinks the floor through
 * it; this actor stands at its mouth and drives everything the choke changes:
 *  - The draw (while DrawWhen holds: Main 5's charge step): within DrawRadius it pulls Sully toward the mouth (a share of
 *    their own walk, SinkIntakeRules::DrawInput, rising toward it), the screen's edges frosting, smog sucked in round it;
 *    inside DrainRadius it drains them (a share of their health a second). Every charge is set leaning against it.
 *  - The charges (AIntakeCharge, three round its collar, NoteChargeSet): the first rouses the ramp's spiders
 *    (SpiderSpawnerId) and gets Vane on the barge's horn (ChargeLines from LoudspeakerTag); the second brings the second
 *    drop down the hose (DropSpawnerId); the third lights the fuse: FuseSeconds, the draw doubled (FuseDrawScale), Hob's
 *    warning (FuseLines).
 *  - The choke: the charges blow (a salt-white blast, the shake), the missions hear ChokedEvent; Landfall's hose is hidden
 *    (its props tagged RestHoseTag) and drawn here instead, in the same shape, then it bucks like a snake, coughs a black
 *    cloud and swings across the floor (SwingSeconds: whatever its end passes, creatures and the player, is knocked down
 *    and hurt), and hangs limp (LimpRest). The barge's two dying stacks (DyingStacks) cough their last; Vane chokes on the
 *    horn (ChokedLines). The smog pool at the den's mouth (PoolAt) drains, and something huge growls inside (DenAt, Hob's
 *    GrowlLines).
 *  - The second hose: once SecondHoseWhen holds (Main 5 at the rim), it's let down from the barge (SecondHoseFrom) onto
 *    Larkspur High's air-shaft head (SecondHoseTo) over SecondDropSeconds, and hangs there until SecondHoseGoneWhen.
 * What it shows follows the story: limp from the start while any of ChokedWhen holds; nothing while any of GoneWhen does.
 * A session loaded mid-step finds it drinking, its charges bare (the step's count starts over). It ticks only while it
 * draws, burns, swings, drains or lets the second hose down. Topics: SinkIntake.cpp (story, charges, fuse, choke, talk),
 * SinkIntakeDraw.cpp (the draw, the swing's knocks, the pool and the growl), SinkIntakeHose.cpp (the hoses' shapes and
 * sections); the math is SinkIntakeRules.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASinkIntake : public AActor
{
	GENERATED_BODY()

public:
	ASinkIntake();

	/** Its tag (the charges find it), and what the missions hear as it blows (Main 5's fourth step waits for it). */
	static const FName IntakeTag;
	static const FName ChokedEvent;

	virtual void Tick(float DeltaSeconds) override;

	/** A charge went on (AIntakeCharge::SetCharge): the drops, Vane, the fuse after the third. */
	void NoteChargeSet(AIntakeCharge& Charge);

	/** Blows it now (the fuse's end; the console). False unless it's drinking or fused. */
	bool Choke();

	/** Reads its story again (the missions' changes call it). */
	void RefreshStory();

	/** Moves it on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	EIntakeState GetIntakeState() const { return State; }
	int32 GetChargesSet() const { return ChargesSet; }
	float GetFuseLeft() const { return State == EIntakeState::Fused ? FMath::Max(FuseSeconds - Clock, 0.f) : 0.f; }
	/** Its draw is on now (its story's step, or the fuse). */
	bool IsDrawing() const;
	/** The draw's pull on a player Distance from the mouth now (0: none). */
	float GetDrawOn(float Distance) const;
	/** The hose's shape now (world), and whether this actor draws it (from the choke on). */
	FIntakeHoseShape GetHoseShape() const;
	bool DrawsHose() const { return State == EIntakeState::Swinging || State == EIntakeState::Limp; }
	/** The second hose's let-down share (0 not there, 1 hanging all the way to the shaft). */
	float GetSecondHoseShare() const { return SecondShare; }
	/** Things knocked down by the swing this visit. */
	int32 GetKnocked() const { return Knocked.Num(); }

	// --- Its parts ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The hose from the choke on (Landfall's baked one before), and the second hose. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Sections;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> SecondSections;

	/** The smog pool at the den's mouth (black, glinting), drained after the choke. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Pool;

	// --- The hose (build_area_main5.py: Landfall's own curve and section) ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	FVector HoseTop = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	FVector HoseC1 = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	FVector HoseC2 = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	FVector HoseBottom = FVector::ZeroVector;

	/** Its section's mesh (the Tripo section, its axis Z and its pivot its middle; or the engine's cylinder), how it's scaled across, its length unscaled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	TObjectPtr<UStaticMesh> HoseMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	float HoseGirthScale = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	float HoseNativeLength = 100.f;
	/** Its look for a stand-in section (the Tripo section keeps its own). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	TObjectPtr<UMaterialInterface> HoseMaterial;

	/** Limp, its end lies here on the floor (world). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	FVector LimpRest = FVector::ZeroVector;

	/** Landfall's hose props (by tag): hidden the moment it blows (their story hides them from the next step on). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	FName RestHoseTag = TEXT("Landfall_BargeHose");

	/** The barge's stacks whose smoke dies at the choke (world tops): they cough their last. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Hose")
	TArray<FVector> DyingStacks;

	// --- The story ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	FStoryCondition DrawWhen;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	TArray<FStoryCondition> ChokedWhen;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	TArray<FStoryCondition> GoneWhen;

	/** Vane's horn (a loudspeaker by tag), his words at the first charge and at the choke; Hob's at the fuse and the growl. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	FName LoudspeakerTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	TObjectPtr<UStoryLineSet> ChargeLines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	TObjectPtr<UStoryLineSet> ChokedLines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	TObjectPtr<UStoryLineSet> FuseLines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	TObjectPtr<UStoryLineSet> GrowlLines;

	/** The drops down the hose (the second on the second charge) and the ramp's spiders (on the first), by spawner id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	FName DropSpawnerId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Story")
	FName SpiderSpawnerId;

	/** The smog pool (world, on the floor) and its radius; the den's mouth the growl comes from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Pool")
	FVector PoolAt = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Pool", meta = (ClampMin = "0", Units = "cm"))
	float PoolRadius = 260.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Pool")
	FVector DenAt = FVector::ZeroVector;

	/** The second hose: from the barge (world) to the air shaft's cowl (world), when it's down and when it's gone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Second Hose")
	FVector SecondHoseFrom = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Second Hose")
	FVector SecondHoseTo = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Second Hose")
	TArray<FStoryCondition> SecondHoseWhen;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Second Hose")
	TArray<FStoryCondition> SecondHoseGoneWhen;

	// --- Numbers ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Draw", meta = (ClampMin = "0", Units = "cm"))
	float DrawRadius = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Draw", meta = (ClampMin = "0", Units = "cm"))
	float DrainRadius = 300.f;
	/** Inside DrainRadius the player loses this share of their health a second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Draw", meta = (ClampMin = "0", ClampMax = "1"))
	float DrainShare = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Fuse", meta = (ClampMin = "0.5", Units = "s"))
	float FuseSeconds = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Fuse", meta = (ClampMin = "1"))
	float FuseDrawScale = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Swing", meta = (ClampMin = "0.5", Units = "s"))
	float SwingSeconds = 4.f;
	/** Whatever stands within this of the swinging hose's lower length is knocked down (cm), and hurt by these shares. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Swing", meta = (ClampMin = "0", Units = "cm"))
	float KnockRadius = 240.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Swing", meta = (ClampMin = "0", ClampMax = "1"))
	float KnockCreatureShare = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Swing", meta = (ClampMin = "0", ClampMax = "1"))
	float KnockPlayerShare = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Pool", meta = (ClampMin = "0", Units = "s"))
	float PoolDrainSeconds = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intake|Second Hose", meta = (ClampMin = "0.5", Units = "s"))
	float SecondDropSeconds = 4.f;

	/** The buck before the swing (s). */
	static constexpr float BuckSeconds = 0.8f;
	/** The pool's mesh (the slag splat) is about this radius at scale 1 (cm). */
	static constexpr float PoolMeshRadius = 100.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void SetState(EIntakeState NewState);
	void HandleRunnerChanged();
	bool AnyHolds(const TArray<FStoryCondition>& Conditions) const;
	/** The draw on the player this frame: the pull, the frost, the drain, the smog sucked in. */
	void TickDraw(float DeltaSeconds);
	/** The swing's knocks: whatever the hose's lower length passes. */
	void TickKnocks();
	/** The pool's drain and the growl after it. */
	void TickPool(float DeltaSeconds);
	/** Vane on the barge's horn. */
	void Broadcast(const UStoryLineSet* Lines) const;
	/** Hob (or anyone) as captions, after what's said. */
	void Say(const UStoryLineSet* Lines) const;
	void TriggerSpawner(FName SpawnerId) const;
	/** The blast's look, sound and shake; the stacks' and the mouth's black cough. */
	void Blast();
	/** The charges round its collar (by their intake tag). */
	TArray<AIntakeCharge*> FindCharges() const;
	/** Landfall's hose props shown or hidden. */
	void ShowRestHose(bool bShown) const;
	APawn* GetPlayer() const;

	// --- The hoses (SinkIntakeHose.cpp) ---
	/** Lays Shape's sections into Into (world), as many as its length needs; up to Share of its length (the second's drop). */
	void LayHose(UInstancedStaticMeshComponent* Into, const FIntakeHoseShape& Shape, float Share = 1.f) const;
	void PlaceHoses();
	/** Across the floor, the swing's way: square to the hose's run from the drum to the mouth. */
	FVector SwingSide() const;
	/** The second hose's story: down, letting down, or not there. */
	void RefreshSecondHose();

	EIntakeState State = EIntakeState::Drinking;
	float Clock = 0.f;
	int32 ChargesSet = 0;
	/** The draw's frost and drain clocks, the smog's. */
	float FrostClock = 0.f;
	float DrainClock = 0.f;
	float SmogClock = 0.f;
	/** The pool's drain (1 full, 0 gone), the clock since the choke, the growl played. */
	float PoolLeft = 1.f;
	float SinceChoke = -1.f;
	bool bGrowled = false;
	/** The second hose: its let-down share, and whether it's letting down now. */
	float SecondShare = 0.f;
	bool bSecondDropping = false;
	TSet<TWeakObjectPtr<AActor>> Knocked;
	TWeakObjectPtr<UAudioComponent> DrawSound;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
