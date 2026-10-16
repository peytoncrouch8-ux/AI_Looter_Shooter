#pragma once

#include "CoreMinimal.h"
#include "Embers/EmberPower.h"
#include "EmberSundown.generated.h"

class AActor;
class ACharacter;

/**
 * Sundown, Ellis's Dusk power (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Five heroes"): for its duration (6 s)
 * the world slows to Strength (0.25) of its speed while the player moves and shoots at full speed.
 *  - The world: global time dilation (WorldSettings), so creatures, their shots, the dust, the water and every timer
 *    crawl.
 *  - The player kept at full speed: their CustomTimeDilation is the dilation's inverse, and so is that of everything that
 *    acts for them: what they carry (the guns, re-checked every frame for a gun swapped in), their controller (the look),
 *    its camera manager (the view's kicks) and the HUD actor. Their bullets fly on their clock (UBulletSubsystem). The
 *    component ticking it is the player's, so the 6 s are the player's seconds.
 *  - Heard: the world's audio pitched down (global pitch modulation, Extras "Pitch", 0.75), cues Ember.Sundown.Start and
 *    .End. Seen: a warm burst of motes round the feet and a slow narrowing of the view as it falls, a kick as it lifts.
 *  - End (run out or cut short) puts everything back: only what is still as it left it (a boss's slow-motion death that
 *    took the clock since keeps it; an actor someone else re-timed keeps theirs).
 * Known gap: the guns' fire-rate and reload timers run on the world's timer manager, which no actor's dilation reaches,
 * so under Sundown they run at the world's speed until AWeaponBase scales them by its owner's clock (Docs/Handoff.md).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberSundown : public UEmberPower
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override;
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** Slowing the world now. */
	bool IsSlowing() const { return bSlowing; }
	/** The world's speed it set (Strength, as the world settings took it); 1 when not slowing. */
	float GetWorldSpeed() const { return bSlowing ? AppliedDilation : 1.f; }
	/** The CustomTimeDilation it gives the player's actors (1 / the world's speed). */
	float GetPlayerClock() const { return bSlowing ? Compensation : 1.f; }

	/** The world's speed when the definition has no Strength, and the audio's pitch under it. */
	static constexpr float DefaultStrength = 0.25f;
	static constexpr float DefaultPitch = 0.75f;
	/** The pitch glides down and back up over this long (seconds). */
	static constexpr float PitchGlide = 0.35f;

private:
	/** The player's own: the pawn, what it carries, its controller, camera manager and HUD, at the player's clock. */
	void CompensatePlayer();
	void Retime(AActor* Actor);
	void ShowStart();

	TWeakObjectPtr<ACharacter> Body;
	bool bSlowing = false;
	/** The world's dilation before (put back if the world still runs at ours) and the one it took. */
	float PreviousDilation = 1.f;
	float AppliedDilation = 1.f;
	float Compensation = 1.f;
	float Pitch = DefaultPitch;
	/** Every actor re-timed, with the dilation it had. */
	TMap<TWeakObjectPtr<AActor>, float> Retimed;
};
