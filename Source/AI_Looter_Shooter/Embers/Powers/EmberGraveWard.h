#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberGraveWard.generated.h"

class AActor;
class ACreatureBase;
class AEmberPropActor;
class UAudioComponent;

/**
 * Grave Ward, Crane's Mercy power: he plants his lantern, and for its duration (8 s) its light keeps a ring of mercy.
 *  - The lantern: the mine's lantern (/Game/Art/Mine/SM_RR_Mine_Lantern, a baked prop: AEmberPropActor) set down a step in
 *    front of him on the ground, lit in the power's pale green (a small light, no shadows), its ring of light Radius (5 m)
 *    laid on the ground round it, breathing, motes rising in it.
 *  - Creatures in its light (flat within Radius, not far over or under it) are slowed: their own clock at Strength (0.5,
 *    half speed: their brain, steps, attacks and pose alike; AActor::CustomTimeDilation), shown by pale motes off them.
 *    Only a clock still its own (1) is taken, and each frame again (a hit-stop's brief stop wins, then the slow comes
 *    back); leaving the light or the ward going gives it back if it's still the slow's. A boss is never slowed.
 *  - The player standing in its light mends HealPerSecond (5) a second (UHealthComponent::Heal), green motes rising off
 *    their feet while it mends.
 *  - Heard: Ember.GraveWard.Plant as it's set down, its hum while it burns (Ember.GraveWard.Hum, a loop on the lantern),
 *    Ember.GraveWard.Out as it goes.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberGraveWard : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override;
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** One frame of the ward: who's slowed, the player mending, the light (its Tick; the tests call it). */
	void StepWard(float DeltaSeconds);

	bool IsBurning() const { return bBurning; }
	const FVector& GetCenter() const { return Center; }
	float GetRadius() const { return Radius; }
	/** Within its light now (flat within the radius, not far over or under it). */
	bool IsInLight(const AActor& Actor) const;
	/** Creatures it's slowing now. */
	int32 NumSlowed() const { return Slowed.Num(); }
	/** Health it has mended this use. */
	float GetMended() const { return Mended; }
	AEmberPropActor* GetLantern() const { return Lantern.Get(); }

	/** Where the lantern goes: on the ground a step in front of the character (false: no ground there or under them). */
	static bool FindPlantSpot(const ACharacter& Character, FVector& OutGround);

	static constexpr float DefaultRadius = 500.f;
	static constexpr float DefaultStrength = 0.5f;
	static constexpr float DefaultHealPerSecond = 5.f;
	/** How far over or under the lantern's ground a body may be and still stand in its light (cm). */
	static constexpr float LightHeight = 300.f;
	static constexpr const TCHAR* LanternPath = TEXT("/Game/Art/Mine/SM_RR_Mine_Lantern.SM_RR_Mine_Lantern");

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Slow(ACreatureBase& Creature);
	void Unslow(ACreatureBase& Creature);
	void ReleaseAll();
	void ShowOut();

	TWeakObjectPtr<AEmberPropActor> Lantern;
	TWeakObjectPtr<UAudioComponent> Hum;
	FVector Center = FVector::ZeroVector;
	TArray<FVector> Ring;
	float Radius = DefaultRadius;
	float Strength = DefaultStrength;
	float HealPerSecond = DefaultHealPerSecond;
	bool bBurning = false;
	float Time = 0.f;
	float Mended = 0.f;
	float MoteTimer = 0.f;
	float HealMoteTimer = 0.f;
	TArray<TWeakObjectPtr<ACreatureBase>> Slowed;
	EmberFX::FEmberSparks Motes;
	FRandomStream Random{ 0x6a7d };
};
