#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForemanFuseStick.generated.h"

class AController;
class AGroundCrack;
class UAudioComponent;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * The Foreman's fuse stick (Docs/Areas/RansomsRest_Expansion.md, "Fire in the hole"): a stick of blasting powder lit in his
 * raised fist (sparks spitting off its fuse, its hiss, a little orange light flickering on his face and his open coal),
 * lobbed in a high arc at where the player stands, then a moment's fuse on the floor over a ring of cracks spreading to its
 * reach, and the burst: about 6 m across (Radius 3 m), full in its heart and a fifth at its edge, through no wall, knocking
 * whoever it catches away. Dropped instead (the Foreman staggered or killed with it lit), it bursts at his feet and hurts
 * him and his shift too (him capped, as the grave salt caps a boss).
 *
 * Its model is SM_ForemanFuseStick (its pivot at its middle, the fuse end along +Z) once made; a thin cylinder stands in.
 * Ticks while it's out; it goes after its burst, or put out (Defuse) when his fight starts over.
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API AForemanFuseStick : public AActor
{
	GENERATED_BODY()

public:
	static const TCHAR* const ModelPath;

	AForemanFuseStick();

	/** Lit in a fist: on Hand's Bone at OnHand (the bone's space), hissing and sparking. By is its thrower (credited). */
	static AForemanFuseStick* SpawnLit(UWorld* World, AActor* By, USceneComponent* Hand, FName Bone, const FVector& OnHand);

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Thrown from From (the fist's release, world) on a high arc that comes down on Target (the floor) after FlightSeconds. */
	void Lob(const FVector& From, const FVector& Target, float FlightSeconds);

	/** Let fall where it is (its thrower staggered or killed with it lit): it bursts on the floor, hurting creatures too. */
	void Drop(AController* CreditTo);

	/** Put out with no burst (the fight started over). */
	void Defuse();

	/** Moves it on by DeltaSeconds: flight, the fuse on the floor, the burst (the tick does; tests call it). */
	void Advance(float DeltaSeconds);

	enum class EStage : uint8
	{
		Held,
		Flying,
		Landed,
		Burst,
		Out,
	};
	EStage GetStage() const { return Stage; }
	bool HasBurst() const { return Stage == EStage::Burst; }

	/** Where it came down (the floor) once landed, and whether its burst caught the player. */
	FVector GetLanding() const { return Landing; }
	bool HitPlayer() const { return bHitPlayer; }

	/** Its fuse's end (world): the sparks' spot. */
	FVector GetFuseEnd() const;

	// --- Its burst (the thrower sets them as it's lit) ---

	/** Damage at its heart; its reach and full-strength core (cm); its share at the edge. */
	float Damage = 0.f;
	float Radius = 300.f;
	float CoreRadius = 100.f;
	float EdgeShare = 0.2f;

	/** On the floor it burns this long before it bursts (s), over a ring of cracks to its reach. */
	float GroundSeconds = 0.8f;

	/** Bursts in the air if it hasn't come down by this long (s). */
	float MaxFlightSeconds = 3.f;

	/** A dropped stick hurts creatures too, the boss among them at most this share of his health. */
	float BossShareCap = 0.04f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Stick;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Glow;

private:
	void Land(const FVector& Floor);
	void Burst();
	/** Sparks spitting off the fuse this frame (code-drawn streaks), and the light's flicker. */
	void Spark(float DeltaSeconds);
	/** Hurts the players in reach (and creatures, dropped); whether it caught a player. */
	bool HurtAround(const FVector& Center);
	void StopHiss();

	EStage Stage = EStage::Held;
	FVector Velocity = FVector::ZeroVector;
	FVector Landing = FVector::ZeroVector;
	float Flight = 0.f;
	float FuseLeft = 0.f;
	float Clock = 0.f;
	float BurstAge = 0.f;
	bool bDropped = false;
	bool bHitPlayer = false;
	TWeakObjectPtr<AActor> Thrower;
	TWeakObjectPtr<AController> Credit;
	TWeakObjectPtr<UAudioComponent> Hiss;
	TWeakObjectPtr<AGroundCrack> Mark;
};
