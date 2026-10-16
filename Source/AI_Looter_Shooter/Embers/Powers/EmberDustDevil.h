#pragma once

#include "CoreMinimal.h"
#include "Embers/EmberPower.h"
#include "EmberDustDevil.generated.h"

class AActor;
class ACharacter;
class ACreatureBase;
class AController;
class UHealthComponent;

/** One dash laid out before it's run: the body's middle at every step along the ground (the first is where it starts). */
struct AI_LOOTER_SHOOTER_API FDustDevilPath
{
	TArray<FVector> Points;
	/** Flat, unit: the way it goes. */
	FVector Direction = FVector::ForwardVector;
	/** Along the points (cm). */
	float Length = 0.f;
	/** It follows the ground (false: started high in the air, it keeps its height and ends falling). */
	bool bOnGround = true;

	/** The body's middle Distance cm along it (held at the ends). */
	FVector At(float Distance) const;
};

/**
 * Dust Devil, Ellis's Wind power (the design: Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Ember powers"). A dash of
 * Range (900 cm) over the definition's duration (0.3 s) the way the player is moving, or the way they look when still:
 *  - Laid out first (PlanDash) along the ground in 30 cm steps: it climbs and drops what a walk would (walkable slopes,
 *    step-sized rises), stops short of walls (a capsule sweep that ignores creatures) and never runs off a drop or out of
 *    the playable area. Shorter than a metre and the press is refused (the socket shakes).
 *  - Run on the power's Tick (the component's, on the player's own clock): the movement stands aside (MOVE_None) while the
 *    body rides the path, fastest at the start; nothing hurts the player meanwhile (bInvulnerable). It ends walking on with
 *    a little of its speed.
 *  - Creatures within Radius (260 cm) of the path as it passes, in sight of it, take Damage x the level's scale (15 at
 *    level 1; UEmberDamageType, the player's controller as instigator, no causer: no gun's notch) and are flung FlingMin to
 *    FlingMax (400-500 cm) to the side, tumbling, and left dizzy Dizzy (1 s) once they land (UCreatureKnockComponent).
 *  - Felt: the view widens FovKick (14 degrees) and settles, speed lines rush past, a ring of dust where it starts and a
 *    smaller one where it ends, a dust trail along the ground, a puff and a thud at each creature struck; cue
 *    Ember.DustDevil.Use. What lingers after the dash (the settle, the lines) runs on a next-frame timer of its own.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberDustDevil : public UEmberPower
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override;
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** The way a dash goes now: the way the player is moving (their keys, else their speed), else the way they look. Flat, unit. */
	static FVector DashDirection(const ACharacter& Character);

	/** Lays out a dash of up to Range cm along Direction from where the character stands (see the class comment). */
	static FDustDevilPath PlanDash(const ACharacter& Character, const FVector& Direction, float Range);

	/** Moves the running dash on by DeltaSeconds (the power's Tick; the tests call it directly, the test level never ticks). */
	void StepDash(float DeltaSeconds);

	bool IsDashing() const { return bDashing; }
	const FDustDevilPath& GetPath() const { return Path; }
	/** The creatures it has struck this dash. */
	const TArray<TWeakObjectPtr<ACreatureBase>>& GetStruck() const { return Struck; }
	/** How much it widens the view right now (degrees). */
	float GetFieldOfViewNow() const { return Widening; }

	/** Shorter than this and there's no room to dash (cm). */
	static constexpr float MinDash = 100.f;
	/** The path is laid out in steps this long (cm). */
	static constexpr float StepLength = 30.f;
	/** The dash's length when the definition has no duration (an instant power: it runs on its own timer). */
	static constexpr float DefaultSeconds = 0.3f;
	/** How high a fling's arc tops out (cm). */
	static constexpr float FlingHeight = 110.f;
	/** The view widens over this long and settles back over SettleSeconds after the dash. */
	static constexpr float WidenSeconds = 0.07f;
	static constexpr float SettleSeconds = 0.45f;
	/** The speed it walks on at once the dash is over (cm/s). */
	static constexpr float ExitSpeed = 700.f;

private:
	/** The dash's own frame-by-frame timer: the dash itself for an instant definition, then the view's settle and the lines. */
	void SchedulePump();
	void Pump();
	void FinishDash(bool bAborted);
	/** Creatures within reach of the stretch of path from From to To (cm along it). */
	void StrikeAlong(float From, float To);
	void Strike(ACreatureBase& Creature, const FVector& Nearest);
	void TickSpeedLines(float DeltaSeconds);
	void SpawnSpeedLine();
	void TrailDust(float From, float To);
	void SpawnDustRing(const FVector& Feet, float Scale, int32 Puffs);
	void SetViewWidening(float Degrees);
	FVector EyeLocation() const;
	FVector FeetAt(const FVector& Middle) const;

	TWeakObjectPtr<ACharacter> Body;
	TWeakObjectPtr<AController> Instigator;
	TWeakObjectPtr<UHealthComponent> Health;
	FDustDevilPath Path;

	// The definition's numbers, kept at the use.
	float Damage = 15.f;
	float Radius = 260.f;
	float FlingMin = 400.f;
	float FlingMax = 500.f;
	float Dizzy = 1.f;
	float FovKick = 14.f;
	float DashSeconds = DefaultSeconds;
	FLinearColor Color = FLinearColor(0.80f, 0.92f, 1.f);

	bool bDashing = false;
	/** The component ticks it (a definition with a duration); else the pump runs the dash. */
	bool bComponentTicks = false;
	/** It turned the player's invulnerability on (and so turns it off). */
	bool bMadeInvulnerable = false;
	float DashTime = 0.f;
	float Travelled = 0.f;
	float TrailLaid = 0.f;
	/** Seconds since the dash ended (the view's settle), negative while it runs. */
	float SettleTime = -1.f;
	/** The view's widening as last set (degrees). */
	float Widening = 0.f;
	bool bPumping = false;

	TArray<TWeakObjectPtr<ACreatureBase>> Candidates;
	TArray<TWeakObjectPtr<ACreatureBase>> Struck;

	struct FSpeedLine
	{
		FVector Tail = FVector::ZeroVector;
		FVector Head = FVector::ZeroVector;
		float Age = 0.f;
		float Life = 0.2f;
		float Width = 1.4f;
	};
	TArray<FSpeedLine> SpeedLines;
	float LinesOwed = 0.f;
	FRandomStream Random{ 0xd057de };
};
