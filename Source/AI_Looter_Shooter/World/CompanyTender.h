#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CompanyTender.generated.h"

class ACreatureBase;
class ALamplighterShip;
class UAudioComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Vane Consolidated's boarding tender (Main 1 steps 1-2): a small iron skiff that runs the Lamplighter down from astern as
 * Vane's voice breaks in on her horn, comes alongside, fires its two grapples into her rail, drops a gangplank across for
 * the boarders, and keeps two riflemen on its own deck firing across while the player cuts the lines (they never count for
 * the deck: they're the tender's). When both lines are cut it falls away into the canyon, its bag burning and trailing
 * black smoke, its riflemen with it. The Lamplighter spawns it (ALamplighterShip::CallTender) and tells it what to do; it
 * keeps station on her calm pose (her course with no bob), so while she's grappled it's dead still too and its riflemen
 * stand on a still deck.
 *
 * Stand-in look: SM_Skiff_A_Gang (the gang's black skiff) scaled to ~12 m, the gangplank SM_Skiff_Gangplank, the launchers
 * iron cylinders (one path each in the .cpp; the Tripo company tender replaces the hull).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACompanyTender : public AActor
{
	GENERATED_BODY()

public:
	ACompanyTender();

	/** Spawns the tender astern of ForShip, on her TenderSide, and starts its run in. (Not named Ship: the member's name.) */
	static ACompanyTender* SpawnFor(ALamplighterShip& ForShip);

	/** Moves its run in, its station alongside or its fall on: the Lamplighter calls it right after moving herself. */
	void Advance(float DeltaSeconds);

	/** Alongside: its run in is done and it keeps station by her rail. */
	bool IsAlongside() const { return bAlongside; }

	/** Puts it alongside at once (the dev jumps, a skipped approach). */
	void SetAlongsideNow();

	/** The world point launcher Index (0 the bow's, 1 the stern's) fires from. */
	FVector GetLauncher(int32 Index) const;

	/** Lowers its gangplank across to her rail, with a thud. */
	void LowerGangplank();

	/** Puts its two riflemen on its deck (DREDGER's rifleman), alerted to the player. */
	void SpawnRiflemen();

	/** Its riflemen still standing. */
	int32 GetRiflemenAlive() const;

	/** The lines are cut: it falls away into the canyon, burning, and is gone FallSeconds later with its riflemen. */
	void FallAway();
	bool IsFalling() const { return bFalling; }

	/** Seconds its run in takes, and its fall before it's gone. */
	static constexpr float ApproachSeconds = 9.f;
	static constexpr float FallSeconds = 8.f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hull;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Gangplank;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Launchers;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its place beside her in her calm frame, Alpha (0-1) through its run in (1: alongside). */
	FTransform StationAt(float Alpha) const;

	/** +1: on her starboard (+Y) side; -1: port. Hers, read when it's spawned. */
	float Side = 1.f;

	TWeakObjectPtr<ALamplighterShip> Ship;
	float Clock = 0.f;
	bool bAlongside = false;

	/** The gangplank swinging down (0 raised, 1 across), and whether it has been asked to. */
	float PlankAlpha = 0.f;
	bool bPlankLowering = false;

	bool bFalling = false;
	float FallClock = 0.f;
	FVector FallVelocity = FVector::ZeroVector;
	FTransform FallFrom = FTransform::Identity;
	float SmokeWait = 0.f;
	bool bBurst = false;

	/** Its riflemen and where each stands on its deck (tender-local), so they go down with it. */
	TArray<TWeakObjectPtr<ACreatureBase>> Riflemen;
	TArray<FVector> RiflemanSpots;

	TWeakObjectPtr<UAudioComponent> EngineSound;
};
