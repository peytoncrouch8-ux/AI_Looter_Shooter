#pragma once

#include "CoreMinimal.h"
#include "Audio/CreatureVoiceBarks.h"
#include "Audio/LooterSoundCues.h"
#include "Components/ActorComponent.h"
#include "Engine/TimerHandle.h"
#include "Math/RandomStream.h"
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
 *
 * The Unpaid also bark (CreatureVoiceComponentBarks.cpp): a line from their lives now and then as they mutter to
 * themselves, spot the player, get hurt, see a packmate fall or die, shown over them in words (UCreatureVoiceDirector
 * keeps it to one at a time near the player) and murmured syllable by syllable in a voice of their own, never speech.
 * Spiders and slimes have no words, only a rare idle call (a chitter, a gurgle).
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureVoiceComponent();

	/** A kind's cries; none plays nothing. Hit is a bullet going into its body; Idle its rare call at rest. */
	struct FCries
	{
		FName Alert;
		FName Attack;
		FName Hurt;
		FName Death;
		FName Hit = LooterSoundCue::CreatureHit;
		FName Idle;
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

	// --- Barks (CreatureVoiceComponentBarks.cpp; the rules in CreatureVoiceBarks.h) ---

	/** Whether it has words at all: an Unpaid below a boss (Abel has his own). Spiders and slimes never speak. */
	bool CanBark() const;

	/**
	 * Its moment to say something of Situation: on its chance (CreatureBarks::ChanceFor), and not within SpeakerRest of its
	 * own last bark (its last words excepted), the bark comes after the situation's delay (after the cry it comes with), when
	 * the level's board lets it then. True when a bark is on its way.
	 */
	bool TryBark(ECreatureBark Situation);

	/** UCreatureVoiceDirector let it say Line: its murmur, a syllable at a time (the director shows the words over it). */
	void SpeakMurmur(const FString& Line, ECreatureBark Situation);

	/** Its murmur stops where it is (it died mid-line, or was interrupted by its own last words). */
	void StopMurmur();

	/** Its idle call (a spider's chitter, a slime's gurgle), quietly. False for a kind without one. */
	bool PlayIdleCall();
	bool HasIdleCall() const { return !Cries.Idle.IsNone(); }

	/** Its own voice against its kind's, from its name (CreatureBarks::VoicePitchFor): each Unpaid sounds like somebody. */
	float GetVoicePitch() const { return VoicePitch; }

	/** The bark waiting for its delay, if any; the murmur's syllables and the next to say (tests). */
	TOptional<ECreatureBark> GetPendingBark() const { return PendingBark; }
	const TArray<FMurmurGrain>& GetMurmur() const { return Murmur; }
	int32 GetMurmurNext() const { return MurmurNext; }
	FName GetMurmurCue() const { return MurmurCue; }

	/** No bark of its own before this, and no idle call (world seconds). The director sets the latter. */
	double GetNextBark() const { return NextBark; }
	double GetNextIdleCall() const { return NextIdleCall; }
	void SetNextIdleCall(double When) { NextIdleCall = When; }

	/** A rank's voice: the angrier souls a little deeper and louder. */
	static float RankPitch(ECreatureRank Rank);

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

	/** The bark that waited its delay: said if it still makes sense (alive, still after the player for a spot). */
	void SayPendingBark();

	/** The murmur's next syllable, and the timer for the one after. */
	void PlayNextGrain();

	FCries Cries;
	FTimerHandle AlertTimer;
	/** World times before which no other hurt cry, hit or alert plays. */
	double NextHurtCry = 0.0;
	double NextHit = 0.0;
	double NextAlert = 0.0;

	// Barks
	FTimerHandle BarkTimer;
	FTimerHandle MurmurTimer;
	TOptional<ECreatureBark> PendingBark;
	TArray<FMurmurGrain> Murmur;
	int32 MurmurNext = 0;
	FName MurmurCue;
	float MurmurVolume = 1.f;
	float VoicePitch = 1.f;
	double NextBark = 0.0;
	double NextIdleCall = 0.0;
	FRandomStream Random;
};
