#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "CreatureVoiceComponent.generated.h"

class AController;
class ACreatureBase;
enum class ECreatureState : uint8;

/**
 * A creature's voice (every ACreatureBase has one): a cry as it turns from calm to hunting (a pack's cries a moment
 * apart, not as one), its attack's cry at the wind-up, the sound of each bullet going into its body (its kind's own: a
 * spider's shell cracks, a slime squelches; Creature.Hit for a kind without one) under its hurt cry now and then, and its
 * death cry. A kill by a local player plays the player's kill sound (UI.Kill) here, once
 * per death, where the death is certain (the HUD only sees hits, the corpse's too).
 *
 * The cries are its kind's (CriesFor: the spider's, the slime's, the Unpaid's) at a pitch for its size, so the Gravemother
 * is the spider's voice deep and slow, Abel the Unpaid's, a spiderling the spider's high. It never ticks.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureVoiceComponent();

	/** A kind's cries; none plays nothing. Hit is a bullet going into its body. */
	struct FCries
	{
		FName Alert;
		FName Attack;
		FName Hurt;
		FName Death;
		FName Hit = LooterSoundCue::CreatureHit;
	};

	/** The cries of a creature's kind: the spider's (the Gravemother and spiderlings too), the slime's, the Unpaid's (Abel too). */
	static FCries CriesFor(const ACreatureBase& Creature);

	/** Its brain changed state (ACreatureBase::SetState): turning on a player cries out, and an attack's wind-up has its cry. */
	void HandleStateChanged(ECreatureState OldState, ECreatureState NewState);

	/** Plays Cue from its body (it follows the body), pitched for its size: the kind's own sounds beyond its cries. */
	void Play(FName Cue, float VolumeScale = 1.f) const;

	/** Its voice's pitch for its size now (LooterSoundRules::PitchForSize). */
	float GetPitch() const;

	/** Hurt cries come at most this often (s), a little more or less each time; hits' thuds at most HitInterval apart. */
	static constexpr float HurtCryInterval = 0.9f;
	static constexpr float HitInterval = 0.06f;

	/** Alerts wait up to this long (s), so a pack turning together is heard as several cries; then not again for AlertRest. */
	static constexpr float AlertDelayMax = 0.35f;
	static constexpr float AlertRest = 4.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** The alert's cry, a moment after it turned (unless it died or calmed down meanwhile). */
	void CryAlert();

	FCries Cries;
	FTimerHandle AlertTimer;
	/** World times before which no other hurt cry, hit or alert plays. */
	double NextHurtCry = 0.0;
	double NextHit = 0.0;
	double NextAlert = 0.0;
};
