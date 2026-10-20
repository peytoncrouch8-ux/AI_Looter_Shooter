#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberRailSpike.generated.h"

class ACreatureBase;
class AGroundCrack;

/** One spike's line, laid out before it flies: from the aim's start to where the world stops it (or its reach). */
struct AI_LOOTER_SHOOTER_API FRailSpikeLine
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float Length = 0.f;
	/** It stops in the world at End (else it flies its whole reach and drops). */
	bool bEmbeds = false;
	FVector EmbedNormal = FVector::UpVector;
	/** Every living creature on the line (within its half width of the body), nearest first, and how far along each is. */
	TArray<TWeakObjectPtr<ACreatureBase>> Creatures;
	TArray<float> Along;
};

/**
 * Rail Spike, Gauge's Steam power (instant): a spike driven from its forearm on a burst of steam, through every creature in
 * a line.
 *  - The line (PlanLine): the aim, out to Range (25 m) or the first of the world's solid shapes; every living creature whose
 *    body comes within Radius (60 cm) of it, however many, is on it.
 *  - The spike flies the line at SpikeSpeed (a white-hot streak, a steam trail laid behind it, a glowing line along its way
 *    fading over a moment and a smoke thread left hanging, since the flight itself is gone in a few frames); as it passes each creature
 *    it deals Damage x the level's scale (60 at level 1; UEmberDamageType, no gun's notch) and shoves it along the line
 *    (Extras "Shove" 150 cm, "Dizzy" 0.3 s; never a boss): Ember.RailSpike.Hit, sparks and steam. In the world at the end
 *    it embeds, glowing and cooling for a while (Ember.RailSpike.Embed, sparks and dust); with nothing there it falls away.
 *  - On use: a steam piston's bang (Ember.RailSpike.Fire), steam from the forearm, a hard kick of the view.
 *  - Seen from behind (the player always is): its line runs straight away from the eye and shows end-on, so what reads is
 *    what lies across it (EmberRailSpikeLook.cpp, 2026-10-20 polish): a glowing head that keeps its size into the distance,
 *    a shock ring and a doughnut of steam as it leaves the arm, a helix of hot air wound round its way, rings of steam
 *    every few metres, sparks burst out sideways and a ring at each body it goes through, a ring on the surface where it
 *    embeds; and on the ground under it (where it skims within 3.2 m) dust thrown out both sides as it passes and, where
 *    within 1.5 m, a glowing scar torn open behind it (AGroundCrack) that closes again.
 * Instant: the component ends it the moment it's used; the spike flies on its linger (the player's clock).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberRailSpike : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** The line a spike from Character would fly now (see the class comment). */
	static FRailSpikeLine PlanLine(const ACharacter& Character, float Range, float HalfWidth);

	/** Moves the spike on by DeltaSeconds (its linger; the tests call it). */
	void StepSpike(float DeltaSeconds);

	bool IsFlying() const { return bFlying; }
	const FRailSpikeLine& GetLine() const { return Line; }
	/** The creatures it has gone through this use. */
	int32 NumPierced() const { return Pierced; }

	static constexpr float DefaultDamage = 60.f;
	static constexpr float DefaultRange = 2500.f;
	static constexpr float DefaultRadius = 60.f;
	static constexpr float DefaultShove = 150.f;
	static constexpr float DefaultDizzy = 0.3f;
	/** How fast the spike flies (cm/s): the whole 25 m in under a tenth of a second, still seen going. */
	static constexpr float SpikeSpeed = 28000.f;
	/** It glows where it's embedded this long, cooling. */
	static constexpr float EmbedSeconds = 2.5f;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	/** A shock ring spreading square to its Axis (as it leaves the arm, through a body, on the surface it embeds in). */
	struct FShockRing
	{
		FVector Center = FVector::ZeroVector;
		FVector Axis = FVector::ForwardVector;
		float Age = 0.f;
		float Life = 0.2f;
		float From = 10.f;
		float To = 80.f;
		float Width = 3.f;
		float Glow = 10.f;
		FLinearColor Color = FLinearColor::White;
	};

	void Pierce(ACreatureBase& Creature, float Along);
	void Embed();
	/** Where the spike is seen at Distance along its line: from the forearm, joining the aim line over ConvergeDistance. */
	FVector SeenAt(float Distance) const;

	// --- What reads from behind (EmberRailSpikeLook.cpp) ---
	/** On use: the rings and steam at the arm; the ground under the line looked up for the dust and the scar. */
	void BeginLook();
	/** As it flies on from Was to Flown: its head's glow, steam rings and ground dust as it passes, the scar opening. */
	void LookInFlight(float Was);
	/** Through a body at At: sparks burst sideways, a ring, a flash, what it spills thrown out the far side. */
	void LookAtPierce(ACreatureBase& Creature, const FVector& At);
	/** Stuck at the line's end: a ring on the surface. */
	void LookAtEmbed();
	/** One frame of the rings and the hot-air helix; true while any shows. */
	bool LingerLook(float DeltaSeconds);
	void AddShock(const FVector& Center, const FVector& Axis, float Life, float From, float To, float Width, float Glow,
		const FLinearColor& Hue);

	FRailSpikeLine Line;
	FVector VisualStart = FVector::ZeroVector;
	float Damage = DefaultDamage;
	float Shove = DefaultShove;
	float Dizzy = DefaultDizzy;
	bool bFlying = false;
	float Flown = 0.f;
	float SteamLaid = 0.f;
	int32 NextCreature = 0;
	int32 Pierced = 0;
	/** Embedded (seconds since), or falling away (negative: not). */
	float EmbedAge = -1.f;
	float FallAge = -1.f;
	/** The glowing line it leaves along its way, fading (seconds since it was driven; negative: gone). */
	float TrailAge = -1.f;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x5a1e };

	TArray<FShockRing> Shocks;
	/** The hot-air helix round its way (seconds since it was driven; negative: gone), and its turn's start. */
	float WakeAge = -1.f;
	float WakeTwist = 0.f;
	/** How far along it the next steam ring and the next breath of ground dust go (cm). */
	float RingLaid = 0.f;
	int32 GroundLaid = 0;
	/** The ground under the line every GroundStep cm from its start (its height under the line; negative: none). */
	TArray<float> GroundHeight;
	TArray<FVector> GroundPoint;
	/** The scar torn under it where it skims low, from ScarFrom to ScarTo along the line (cm). */
	TWeakObjectPtr<AGroundCrack> Scar;
	float ScarFrom = 0.f;
	float ScarTo = 0.f;
};
