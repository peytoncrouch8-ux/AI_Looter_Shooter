#pragma once

#include "CoreMinimal.h"
#include "Combat/RecoverySettings.h"
#include "Combat/WoundsClose.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerVitalsSubsystem.generated.h"

class APlayerController;

/**
 * Player-side consequences of taking damage: a red flash when hurt, and on death a respawn with full health at the Hob's
 * roost the player last found on this map, wherever they fell (the user's rule, 2026-10-14; ARespawnMarker::ChooseWakeSpot),
 * or, with none found yet, at the level's own start, never at a trip's landing. On a map with roosts, Hob's crows gather
 * round the dying at the screen's edges and drag them back (URoostCarrySubsystem), bursting away at the wake-up with Hob's
 * word; elsewhere (Skyreach, outside the story), the plain fade. It also gives health back: the revenant's wounds close on
 * their own after a while without being hurt (FWoundsClose), by the numbers in Recovery. Works for any player character
 * with a HealthComponent.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPlayerVitalsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Seconds from death to respawn. */
	UPROPERTY(EditAnywhere, Category = "Player Vitals")
	float RespawnDelay = 2.5f;

	/** Every number of how health comes back: the wounds that close and the soul-motes. */
	UPROPERTY(EditAnywhere, Category = "Player Vitals")
	FRecoverySettings Recovery;

	/**
	 * The recovery numbers in force in WorldContext's world: the subsystem's, or the defaults where there is none (a test
	 * level, the editor). Soul-motes and the drop roll read them here.
	 */
	static const FRecoverySettings& SettingsFor(const UObject* WorldContext);

	/** Whether PC's wounds are closing right now (past the wait, below full health, not held): the HUD shows it as it rises. */
	bool IsRegenerating(const APlayerController* PC) const;

private:
	/** A death carried by the crows goes black by this share of RespawnDelay, under them (the rest held black). */
	static constexpr float CrowsCover = 0.85f;

	/** How often the fight is looked for (a walk over the level's creatures): four times a second, as the music does. */
	static constexpr float CombatLookSeconds = 0.25f;

	struct FPlayerVitals
	{
		float LastHealth = -1.f;
		bool bDying = false;
		/** Hob's crows are dragging them back (to a roost, or the level's start on a map with roosts; else the plain fade). */
		bool bByCrows = false;
		float DeathTime = 0.f;
		/** The wait since the last hurt and the run of healing after it. */
		FWoundsClose Wounds;
		/** A creature is fighting them (FightState::IsInCombat), looked at a few times a second (CombatLookSeconds). */
		bool bInCombat = false;
		float CombatLookIn = 0.f;
	};

	void Respawn(APlayerController* PC, FPlayerVitals& Vitals);

	TMap<TWeakObjectPtr<APlayerController>, FPlayerVitals> Players;
};
