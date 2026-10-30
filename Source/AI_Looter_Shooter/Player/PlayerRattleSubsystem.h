#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PlayerRattleSubsystem.generated.h"

class APawn;
class UCameraComponent;

/** How hard one rattle hits the player (UPlayerRattleSubsystem::Rattle). Strengths 0-1; seconds; a share of speed. */
struct AI_LOOTER_SHOOTER_API FPlayerRattle
{
	/** How hard it is felt (the distance falloff is the caller's): the frost, the wobble and the shake scale with it. */
	float Strength = 1.f;
	/** The aim's sway: degrees at full strength, over this long; none when bSway is false (felt from farther off). */
	bool bSway = true;
	float SwayDegrees = 1.1f;
	float SwaySeconds = 0.9f;
	/** A slight slow: the share of speed kept, for this long; none when bSlow is false. Never a stun. */
	bool bSlow = true;
	float SlowShare = 0.85f;
	float SlowSeconds = 1.f;
	/** The camera's shake at full strength (UCameraShakeModifier), and how long it rings. */
	float Shake = 0.5f;
	float ShakeSeconds = 0.6f;
	/** The screen-edge frost and the chromatic wobble last this long (s). */
	float FrostSeconds = 1.1f;
	/** The sway and slow come at most this often (s), whoever rattles the player (the frost, shake and arc still show). */
	float Gap = 3.f;
};

/**
 * The player rattled by a scream (the Unpaid's failed-lunge shriek, 2026-10-29; any enemy may use it): a short aim sway (the
 * control rotation swung and brought back to where it was), a slight slow (UMovementSlowComponent), a camera shake, the
 * screen's edges frosting over (UHudScreenEdgeWidget reads Frost) with a chromatic wobble on the player's cameras (scene
 * fringe, put back after), and an arc on the HUD pointing at where it came from (UHudDamageIndicatorWidget). The aim's sway
 * and the wobble follow the player's camera-shake setting. Ticks only while a rattle lasts. Local players only.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPlayerRattleSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UPlayerRattleSubsystem* Get(const UObject* WorldContextObject);

	/** Rattles Pawn (a local player's) from Source (world). */
	void Rattle(APawn* Pawn, const FVector& Source, const FPlayerRattle& How);

	/** For the HUD: how frosted Pawn's view is now (0-1), where the last rattle came from, how long ago (s). False: no rattle now. */
	bool GetRattle(const APawn* Pawn, float& OutFrost, FVector& OutSource, float& OutAge) const;

	/** A rattle's frost and wobble: 0 to 1 over Age of Seconds (a quick rise, a hold, an ease out). */
	static float Envelope(float Age, float Seconds);
	/** The aim's sway Age into Seconds (degrees: pitch, yaw), Degrees at its widest, 0 at both ends; Seed varies its shape. */
	static FRotator SwayAt(float Age, float Seconds, float Degrees, float Seed);

	/** The HUD's arc shows this long (s). */
	static constexpr float ArcSeconds = 1.4f;

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FFringeCamera
	{
		TWeakObjectPtr<UCameraComponent> Camera;
		bool bHadOverride = false;
		float Fringe = 0.f;
	};

	struct FRattled
	{
		TWeakObjectPtr<APawn> Pawn;
		FVector Source = FVector::ZeroVector;
		FPlayerRattle How;
		/** Seconds since it began; since the sway began (below 0: no sway); the sway applied so far (put back as it goes). */
		float Age = 0.f;
		float SwayAge = -1.f;
		FRotator SwayShown = FRotator::ZeroRotator;
		float Seed = 0.f;
		/** When its last sway and slow came (world seconds): the gap. */
		double LastSway = -1.0e9;
		/** Its cameras' fringe as they had it, put back at the end. */
		TArray<FFringeCamera> Cameras;
	};

	/** Lays the wobble on Rattled's cameras (Amount 0 puts them back). */
	void ShowFringe(FRattled& Rattled, float Amount);
	/** The player's camera-shake setting for Pawn's controller (1 without one). */
	static float ShakeSetting(const APawn* Pawn);

	TArray<FRattled> Rattles;
};
