#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LegendaryLantern.generated.h"

class APawn;

/**
 * Lamplighter's lantern (Abel's legendary; LegendaryRules, ULegendarySubsystem::LightLantern): left on the ground by a
 * scoped kill, it burns a few seconds and heals the player who made the kill while they stand within its reach
 * (Looter.Legendary.Lamplighter.*), never on top of another lantern's heal. No mesh, no light: a flickering warm flame,
 * its halo, a ring on the ground showing its reach and motes rising, all through the bullets' pooled effects (FWeaponFX),
 * so it costs a few shapes a frame and goes by itself.
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API ALegendaryLantern : public AActor
{
	GENERATED_BODY()

public:
	ALegendaryLantern();

	/** Lights one at Ground for InHealer, burning Seconds and healing within InRadius cm. Null without a world. */
	static ALegendaryLantern* Light(UWorld* World, const FVector& Ground, APawn* InHealer, float Seconds, float InRadius);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on by DeltaSeconds: it heals, shows itself, and goes out at its end. Its tick calls it; tests call it. */
	void Advance(float DeltaSeconds);

	/** Health it has given so far. */
	float GetHealed() const { return Healed; }
	float GetRadius() const { return Radius; }
	float GetLife() const { return Life; }

private:
	/** Its flame, halo, ring and motes this frame, Glow bright (0-1), and motes rising off the player while it heals them. */
	void Draw(float Glow, const APawn* Healing, float DeltaSeconds);

	TWeakObjectPtr<APawn> Healer;
	float Life = 5.f;
	float Radius = 400.f;
	float Age = 0.f;
	float Healed = 0.f;
	float MotesOwed = 0.f;
	float HealMotesOwed = 0.f;
};
