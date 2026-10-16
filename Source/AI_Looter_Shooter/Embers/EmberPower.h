#pragma once

#include "CoreMinimal.h"
#include "Embers/EmberTypes.h"
#include "UObject/Object.h"
#include "EmberPower.generated.h"

class ALooterCharacter;
class UEmberComponent;
class UEmberDefinition;

/** What a power is given to work with: the body, its component, its data, and how far the player's level grows its damage. */
struct AI_LOOTER_SHOOTER_API FEmberContext
{
	ALooterCharacter* Character = nullptr;
	UEmberComponent* Component = nullptr;
	const UEmberDefinition* Definition = nullptr;
	/** FLevelRules::EnemyScale at the player's level (1 at level 1): the definition's Damage times this. */
	float LevelScale = 1.f;
};

/**
 * What one ember power does (Embers/Powers/*: one subclass per power, named by UEmberDefinition::PowerClass). The
 * component makes one per owned power and drives it: CanActivate asks whether a press right now may fire it (on top of
 * the component's own gate), Activate fires it, Tick runs each frame while it lasts (its definition's DurationSeconds;
 * never for an instant power), End closes it (bInterrupted: death, a scene or the level ending cut it short). A power
 * keeps its own state between Activate and End and leaves the world as it found it.
 */
UCLASS(Abstract, Within = EmberComponent)
class AI_LOOTER_SHOOTER_API UEmberPower : public UObject
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const { return EEmberDenied::None; }
	virtual void Activate(const FEmberContext& Context) {}
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) {}
	virtual void End(const FEmberContext& Context, bool bInterrupted) {}

	/** For the HUD and the page while it runs: 0 to 1 of its duration gone (the component keeps the clock). */
	bool IsRunning() const { return bRunning; }

	/** The component's bookkeeping; powers don't call these. */
	void MarkRunning(bool bInRunning) { bRunning = bInRunning; }

private:
	bool bRunning = false;
};
