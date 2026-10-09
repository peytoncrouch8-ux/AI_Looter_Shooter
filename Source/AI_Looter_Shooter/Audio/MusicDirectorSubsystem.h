#pragma once

#include "CoreMinimal.h"
#include "Audio/MusicRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "MusicDirectorSubsystem.generated.h"

class ACreatureBase;
class UAudioComponent;
class UBossComponent;

/**
 * The score's director (Docs/Polish/BorderlandsComparison.md, item 1). It looks at the world four times a second and
 * plays what fits, crossfading on the pieces' bar lines (MusicRules: their bars and tempos, music.py's):
 *  - Calm: the area's exploration theme, two or three times through, then a rest of 40-90 s for the world's own sound,
 *    then again (in the main menu, with no player, it plays on).
 *  - Combat, while any creature hunts the local player and for a few seconds after the last lets go: the combat layer
 *    comes in on the theme's next bar line (at once, alone, if the theme was resting) and the theme dips under it; it
 *    leaves over two bars from a bar line. A fight that ran a while and killed something ends on the victory sting.
 *  - Boss, while a boss's fight runs (UBossComponent::IsFighting) or a Soulfed monster hunts the player: the phase sting
 *    hits, and Abel's theme (an Unpaid boss) or the Gravemother's starts on its hit; each later phase hits the sting
 *    again over the theme; a win plays the victory sting and the area's theme returns after it.
 *  - A creature of high rank (Gravebound, Soulfed) coming for the player the first time plays the elite sting.
 * Under a scene (USceneSubsystem) the music dips; over a paused game it goes on, quieter (its tracks are UI sounds, so
 * the pause doesn't stop them). The Music slider sets the Music class, which every piece is in.
 *
 * Time is the world's real time: the music runs on while the game is paused, and so does this, so it ticks then too.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMusicDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UMusicDirectorSubsystem* Get(const UObject* WorldContext);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override;

	EMusicMood GetMood() const { return Mood; }
	ELooterAudioArea GetArea() const { return Area; }

	/** Holds the mood whatever the world says (Looter.Music.Force), for listening; unset with Release. */
	void Force(EMusicMood InMood, EBossTheme InTheme = EBossTheme::Keeper);
	void Release();

	/** Plays a stinger now (Looter.Music.Sting). */
	void PlaySting(FName Cue);

	/** A line describing what it's doing, for the console (Looter.Music). */
	FString Describe() const;

private:
	/** What it saw on its last look round (MusicDirectorSenses.cpp). */
	struct FSenses
	{
		int32 Hunters = 0;
		bool bBossFight = false;
		EBossTheme Theme = EBossTheme::Keeper;
		TWeakObjectPtr<UBossComponent> Boss;
		/** The creature the boss's music is for (a boss, or a Soulfed monster): its death is the win. */
		TWeakObjectPtr<ACreatureBase> BossFoe;
		TArray<TWeakObjectPtr<ACreatureBase>> HunterList;
	};

	double Now() const;

	// --- Looking (MusicDirectorSenses.cpp) ---
	FSenses Sense() const;
	/** The fight's kills and the elites' stings from what it saw. */
	void Notice(const FSenses& Seen, double At);
	void WatchBoss(UBossComponent* Boss);
	void UnwatchBoss();
	void HandleBossPhase(int32 NewPhase, int32 OldPhase);
	void HandleBossWon();

	// --- Deciding (MusicDirectorSubsystem.cpp) ---
	void Update(double At);
	void Transition(EMusicMood From, EMusicMood To, double At);
	void RunPending(double At);
	void TickCalm(double At);
	void ApplyGains(double At);
	void StartTheme(double At);
	bool HasPlayer() const;

	// --- Playing ---
	UAudioComponent* StartTrack(FName Cue, float FadeSeconds, float Gain);
	static void FadeAway(UAudioComponent* Track, float Seconds);

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Theme;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CombatLayer;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BossTrack;

	ELooterAudioArea Area = ELooterAudioArea::Unknown;
	EMusicMood Mood = EMusicMood::Calm;
	EBossTheme BossTheme = EBossTheme::Keeper;
	FSenses Senses;
	FCombatMemory Memory;
	bool bStarted = false;
	double NextLook = 0.0;

	/** The bar grid the layers keep to: where bar 1 fell and how long a bar and a beat are. */
	double GridOrigin = 0.0;
	double GridBar = 1.6;
	double GridBeat = 0.8;

	/** The theme's plan in calm: when it began, when it bows out (its last two bars), when the rest ends. */
	double ThemeStarted = 0.0;
	double ThemeBowsOut = 0.0;
	double RestUntil = 0.0;

	/** Things due at a time (negative: none). */
	double PendingTheme = -1.0;
	double PendingCombatIn = -1.0;
	double PendingCombatOut = -1.0;
	double PendingBoss = -1.0;
	double PendingVictory = -1.0;

	/** The gains last given each track (so they're only moved when they change), and the duck in force. */
	float ThemeGain = 1.f;
	float CombatGain = 1.f;
	float BossGain = 1.f;
	double PhaseDuckUntil = 0.0;

	/** The fight's bookkeeping: who hunted the player in it, how many of them died, stings' cooldowns. */
	TSet<TWeakObjectPtr<ACreatureBase>> FightHunters;
	int32 FightKills = 0;
	TSet<TWeakObjectPtr<ACreatureBase>> Announced;
	double NextElite = 0.0;
	double NextVictory = 0.0;
	bool bBossWon = false;
	TWeakObjectPtr<ACreatureBase> BossFoe;
	TWeakObjectPtr<UBossComponent> WatchedBoss;
	FDelegateHandle PhaseHandle;
	FDelegateHandle WonHandle;

	bool bForced = false;
	EMusicMood ForcedMood = EMusicMood::Calm;
	EBossTheme ForcedTheme = EBossTheme::Keeper;
};
