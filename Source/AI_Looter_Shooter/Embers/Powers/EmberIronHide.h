#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberIronHide.generated.h"

class AActor;
class AController;
class UHealthComponent;
class UMaterialInstanceDynamic;

/**
 * Iron Hide, Gauge's second Forge power: its plates lock, and for its duration (6 s) it's an engine in armour.
 *  - It takes Strength (60%) less damage: the player's health scales what it takes (UHealthComponent::SetDamageTakenScale,
 *    "IronHide" at 1 - Strength), so the number, the hurt and every listener see the blow as the plates let it through.
 *  - It moves at Pace (0.75) of its speed, walking, sprinting and aiming alike (UMovementSlowComponent, renewed every frame,
 *    so the slow ends within a moment of the plates opening).
 *  - Whatever bites it (a creature's own blow: the hurt's causer is the creature) burns its teeth on the hot plates:
 *    BiteBurn x the level's scale (12 at level 1; UEmberDamageType, no gun's notch), sparks and a sizzle at the biter
 *    (Ember.IronHide.Burn), at most once per BiteEvery from one biter.
 *  - Every blow taken rings off the plates (Ember.IronHide.Clang, sparks where it landed).
 *  - On use: plates slammed shut and bolts home (Ember.IronHide.Lock), steam venting off its shoulders, a heavy dip of the
 *    view; steam breathes off it while it lasts, its body glowing a dull forge orange (the hit flash's overlay, FEmberTint)
 *    that flares with each blow; at the end, steam let out, the glow gone and the plates open (Ember.IronHide.Unlock).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberIronHide : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	bool IsLocked() const { return bLocked; }
	/** The share of a blow that gets through now (1 - Strength while locked). */
	float GetTakenShare() const { return bLocked ? TakenShare : 1.f; }
	/** Biters burned this use. */
	int32 NumBurned() const { return Burned; }

	/** The source name it scales the player's damage under (UHealthComponent::SetDamageTakenScale). */
	static const FName ScaleSource;

	static constexpr float DefaultStrength = 0.6f;
	static constexpr float DefaultPace = 0.75f;
	static constexpr float DefaultBiteBurn = 12.f;
	/** One biter is burned at most this often (seconds). */
	static constexpr float BiteEvery = 0.3f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	void Bind(bool bOn);
	void Vent(int32 Puffs, float Strength);

	TWeakObjectPtr<UHealthComponent> Health;
	float TakenShare = 1.f - DefaultStrength;
	float Pace = DefaultPace;
	float BiteBurn = DefaultBiteBurn;
	bool bLocked = false;
	int32 Burned = 0;
	float Time = 0.f;
	float VentClock = 0.f;
	TMap<TWeakObjectPtr<AActor>, float> LastBurned;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x1404 };
	/** The plates glowing hot on its own body while locked (the hit flash's overlay), flaring as a blow lands. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeatLook;
	EmberFX::FEmberTint Heat;
	float HeatFlare = 0.f;
};
