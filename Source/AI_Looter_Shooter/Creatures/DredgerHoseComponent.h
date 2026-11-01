#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/DredgerTypes.h"
#include "DredgerHoseComponent.generated.h"

class ADredgerHoseMan;
class APawn;
class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * The hose-man's soul-hose (ADredgerHoseMan): the feed hose from his tank to his nozzle, always hanging there, and the line
 * he throws. Thrown (Fire), its tip flies at the victim's chest; if it reaches them with a clear line it latches, else it
 * whips back. Latched: the line hangs from the nozzle to their chest with a ripple running back along it and pulses of
 * soul-light drawn in toward him; it pulls them toward him (a share of their own walking input against them, a slow) and
 * drains them (damage ticks, his level's rate: DredgerRules::DrainPerSecond), a hiss on them, the screen's edges frosted
 * (the rattle's frost while Looter.Dredger.Hose.FrostEdge is 1; GetDrainOn for the HUD's own edge). It breaks
 * (DredgerRules::HoseBreak) at 14 m, when the world cuts it a moment, when he is staggered, his tank bursts or he falls, or
 * after a while: a snap, a whip back, a sound. Both hoses are engine cylinders on one instanced mesh (a draw call),
 * updated only while seen near. Topics: DredgerHoseComponent.cpp (the line's life), DredgerHoseComponentDraw.cpp (its look).
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UDredgerHoseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDredgerHoseComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Throws the line at Victim. False when it can't (already out, no victim, its hose-man down or his tank burst). */
	bool Fire(APawn* InVictim);
	/** Lets go now for Why: the snap's look and sound unless bQuiet (a reset). */
	void Break(EDredgerHoseBreak Why, bool bQuiet = false);
	/** One step of its life (TickComponent's; the tests call it in a level that never ticks). */
	void Advance(float DeltaSeconds);

	EDredgerHoseState GetState() const { return State; }
	bool IsLatched() const { return State == EDredgerHoseState::Latched; }
	bool IsOut() const { return State == EDredgerHoseState::Flying || State == EDredgerHoseState::Latched; }
	APawn* GetVictim() const { return Victim.Get(); }
	/** Health drained from its victims so far (counted whether the damage reached them or not: a test level deals none). */
	float GetDrainDealt() const { return DrainDealt; }
	float GetLatchedFor() const { return LatchedFor; }
	EDredgerHoseBreak GetLastBreak() const { return LastBreak; }
	/** The thrown line's tip now (world). */
	FVector GetTip() const { return Tip; }
	/** The line's tug on the hose-man's body (degrees), for his brace. */
	float GetTug() const { return Tug; }

	/** How strongly Pawn is being drained right now (0-1, eased in and out): for the HUD's cyan-black edge. */
	static float GetDrainOn(const APawn* Pawn);

	/** Hides both hoses (his tank burst: the feed hose is torn away) or shows them again. */
	void SetTorn(bool bInTorn);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ADredgerHoseMan* GetHoseMan() const;
	void StepFlying(float DeltaSeconds);
	void StepLatched(float DeltaSeconds);
	void StepSnapping(float DeltaSeconds);
	void Latch();
	/** The victim's chest now (world), where the line holds. */
	FVector VictimChest() const;
	/** Whether the world cuts the line between the nozzle and Point. */
	bool IsCut(const FVector& Point) const;
	/** Its feedback on the victim: the frost at the screen's edges, a little shake, the arc to him; a slow. */
	void TouchVictim(bool bFirst);
	void StopDrainSound();

	// --- The look (DredgerHoseComponentDraw.cpp) ---
	void EnsureLine();
	void Draw(float DeltaSeconds);
	/** Lays Segments cylinders along the hose from A to B, hanging with Slack and rippling by Wave, into Transforms. */
	void LayHose(const FVector& A, const FVector& B, int32 Segments, float Radius, float Slack, float Wave, TArray<FTransform>& Transforms) const;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Line;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LineLook;

	EDredgerHoseState State = EDredgerHoseState::Stowed;
	EDredgerHoseBreak LastBreak = EDredgerHoseBreak::None;
	TWeakObjectPtr<APawn> Victim;
	TWeakObjectPtr<UAudioComponent> DrainSound;
	/** The tip (world), where it was thrown at, how far it has flown and may fly. */
	FVector Tip = FVector::ZeroVector;
	FVector Aim = FVector::ZeroVector;
	float Flown = 0.f;
	float Reach = 0.f;
	/** Latched: for how long, cut by the world for how long, damage owed until the next tick, the next tick's clock, the feedback's clock. */
	float LatchedFor = 0.f;
	float BlockedFor = 0.f;
	float DrainOwed = 0.f;
	float DrainClock = 0.f;
	float TouchClock = 0.f;
	float DrainDealt = 0.f;
	/** Snapping: how far back (0-1) and where it let go. */
	float SnapShare = 0.f;
	FVector SnapFrom = FVector::ZeroVector;
	/** The line's look: its clock (the ripple, the pulses), its tug, the drain's shown strength (eased). */
	float Clock = 0.f;
	float Tug = 0.f;
	float DrainShown = 0.f;
	bool bTorn = false;
	/** The instanced hose had instances last frame (so a hidden one is cleared once). */
	bool bDrawn = false;
};
