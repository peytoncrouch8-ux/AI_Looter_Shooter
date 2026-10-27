#pragma once

#include "CoreMinimal.h"

class APawn;
class UHealthComponent;

/**
 * Keeps the player untouchable through a boss intro (Scenes/BossIntro.h) and BossIntro::GraceSeconds after it: their
 * health's own guard (UHealthComponent::bInvulnerable) through the scene, whatever reaches them (a second boss nearby, a
 * shot in flight; DEMO 2026-10-27), and, once the scene has let them go, CanBeDamaged off too until the grace ends (Abel's
 * buckshot landed 0.5 s after the hand back). Their own settings come back exactly as they were.
 */
class AI_LOOTER_SHOOTER_API FBossIntroGuard
{
public:
	/** Guards Player from now (nothing without a health component, or if already guarding). */
	void Begin(APawn* Player);

	/** The scene has let them go: still guarded for Seconds of play, then their own settings back (at once without a world). */
	void EndAfter(float Seconds);

	bool IsGuarding() const { return bGuarding; }

private:
	TWeakObjectPtr<APawn> Player;
	TWeakObjectPtr<UHealthComponent> Health;
	bool bWasInvulnerable = false;
	bool bGuarding = false;
};
