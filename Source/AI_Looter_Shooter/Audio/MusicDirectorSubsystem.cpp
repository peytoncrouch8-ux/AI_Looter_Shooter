#include "Audio/MusicDirectorSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Bosses/BossComponent.h"
#include "Components/AudioComponent.h"
#include "Creatures/CreatureBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Scenes/SceneSubsystem.h"
#include "Sound/SoundWave.h"

namespace
{
	const TCHAR* MoodName(EMusicMood Mood)
	{
		switch (Mood)
		{
		case EMusicMood::Combat:
			return TEXT("combat");
		case EMusicMood::Boss:
			return TEXT("boss");
		default:
			return TEXT("calm");
		}
	}

	constexpr double Never = TNumericLimits<double>::Max();
}

UMusicDirectorSubsystem* UMusicDirectorSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UMusicDirectorSubsystem>() : nullptr;
}

bool UMusicDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Only the game's own worlds: the tests' preview worlds and the editor's viewports play no music.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UMusicDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!ULooterSoundSubsystem::CanPlayIn(&InWorld))
	{
		return;
	}
	Area = AmbienceRules::AreaForMap(InWorld.GetMapName());
	bStarted = true;
	// The air comes up first; the theme a moment after.
	PendingTheme = Now() + MusicRules::FirstThemeDelay;
	UE_LOG(LogLooter, Log, TEXT("Music: %s's theme (%s) in %s."), Area == ELooterAudioArea::RansomsRest ? TEXT("Ransom's Rest") : TEXT("Skyreach"),
		*MusicRules::ExploreFor(Area).Cue.ToString(), *InWorld.GetMapName());
}

void UMusicDirectorSubsystem::Deinitialize()
{
	UnwatchBoss();
	FadeAway(Theme, 0.1f);
	FadeAway(CombatLayer, 0.1f);
	FadeAway(BossTrack, 0.1f);
	Theme = nullptr;
	CombatLayer = nullptr;
	BossTrack = nullptr;
	bStarted = false;
	Super::Deinitialize();
}

TStatId UMusicDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMusicDirectorSubsystem, STATGROUP_Tickables);
}

bool UMusicDirectorSubsystem::IsTickable() const
{
	return bStarted;
}

double UMusicDirectorSubsystem::Now() const
{
	// Real time: it runs on through a pause, as the music does.
	const UWorld* World = GetWorld();
	return World ? World->GetRealTimeSeconds() : 0.0;
}

bool UMusicDirectorSubsystem::HasPlayer() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0) != nullptr;
}

void UMusicDirectorSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Update(Now());
}

void UMusicDirectorSubsystem::Update(double At)
{
	if (At >= NextLook)
	{
		NextLook = At + MusicRules::PollSeconds;
		Senses = Sense();
		Notice(Senses, At);
		Memory.Update(Senses.Hunters > 0, At, MusicRules::CalmAfterSeconds);
		EMusicMood Wanted = MusicRules::Resolve(Senses.bBossFight, Memory.bInCombat);
		EBossTheme WantedTheme = Senses.Theme;
		if (bForced)
		{
			Wanted = ForcedMood;
			WantedTheme = ForcedTheme;
		}
		if (Wanted != Mood)
		{
			BossTheme = WantedTheme;
			Transition(Mood, Wanted, At);
		}
	}
	RunPending(At);
	TickCalm(At);
	ApplyGains(At);
}

void UMusicDirectorSubsystem::Transition(EMusicMood From, EMusicMood To, double At)
{
	Mood = To;
	UE_LOG(LogLooter, Verbose, TEXT("Music: %s -> %s."), MoodName(From), MoodName(To));
	if (To == EMusicMood::Boss)
	{
		// The phase sting's hit opens the fight, and the boss's theme starts on it; everything else gives way.
		PlaySting(LooterSoundCue::StingPhase);
		FadeAway(Theme, 1.f);
		FadeAway(CombatLayer, 1.f);
		FadeAway(BossTrack, 0.5f);
		Theme = nullptr;
		CombatLayer = nullptr;
		BossTrack = nullptr;
		PendingTheme = PendingCombatIn = PendingCombatOut = PendingVictory = -1.0;
		PendingBoss = At + MusicRules::PhaseStingHit;
		bBossWon = false;
		BossFoe = Senses.BossFoe;
		WatchBoss(Senses.Boss.Get());
		return;
	}
	if (From == EMusicMood::Boss)
	{
		PendingBoss = -1.0;
		FadeAway(BossTrack, MusicRules::BossOutSeconds);
		BossTrack = nullptr;
		// Won: the boss died (its fight says so, or its body does). Not when the player died or ran.
		const bool bFoeFell = BossFoe.IsValid() ? BossFoe->IsDead() : !BossFoe.IsExplicitlyNull();
		const bool bWon = bBossWon || (WatchedBoss.IsValid() && WatchedBoss->IsWon()) || bFoeFell;
		if (bWon && !bForced)
		{
			PlaySting(LooterSoundCue::StingVictory);
			NextVictory = At + MusicRules::VictoryCooldown;
		}
		UnwatchBoss();
		BossFoe.Reset();
		bBossWon = false;
		RestUntil = 0.0;
		PendingTheme = At + MusicRules::ThemeAfterBossSeconds;
		if (To == EMusicMood::Combat)
		{
			const FMusicPiece Piece = MusicRules::Combat();
			FightHunters.Reset();
			FightKills = 0;
			PendingCombatIn = At + 1.0;
			GridOrigin = PendingCombatIn;
			GridBar = Piece.BarSeconds;
			GridBeat = Piece.BeatSeconds();
		}
		return;
	}
	if (To == EMusicMood::Combat)
	{
		FightHunters.Reset();
		FightKills = 0;
		// A fight back before the last one's layer left: the layer stays, and no victory is played over it.
		PendingCombatOut = -1.0;
		PendingVictory = -1.0;
		if (CombatLayer)
		{
			return;
		}
		const FMusicPiece Piece = MusicRules::Combat();
		if (Theme)
		{
			// On the theme's next bar line (at most 1.6 s away), so the layer's downbeats are the theme's: the creature's
			// own cry and any elite sting carry the moment meanwhile.
			PendingCombatIn = MusicRules::NextBoundary(At, GridOrigin, GridBar);
		}
		else
		{
			// Out of silence: at once, and the grid starts with it.
			PendingCombatIn = At;
			GridOrigin = At;
			GridBar = Piece.BarSeconds;
			GridBeat = Piece.BeatSeconds();
		}
		return;
	}
	// From a fight to calm: the layer leaves from the next bar line, and a fight worth it ends on the victory sting.
	if (!CombatLayer && PendingCombatIn >= 0.0)
	{
		PendingCombatIn = -1.0;
		return;
	}
	PendingCombatOut = MusicRules::NextBoundary(At, GridOrigin, GridBar);
	if (MusicRules::EarnsVictory(Memory.FightSeconds(), FightKills) && At >= NextVictory && !bForced)
	{
		PendingVictory = PendingCombatOut;
		NextVictory = At + MusicRules::VictoryCooldown;
	}
}

void UMusicDirectorSubsystem::RunPending(double At)
{
	if (PendingBoss >= 0.0 && At >= PendingBoss)
	{
		PendingBoss = -1.0;
		const FMusicPiece Piece = MusicRules::BossFor(BossTheme);
		BossTrack = StartTrack(Piece.Cue, 0.05f, BossGain);
		GridOrigin = At;
		GridBar = Piece.BarSeconds;
		GridBeat = Piece.BeatSeconds();
	}
	if (PendingCombatIn >= 0.0 && At >= PendingCombatIn)
	{
		PendingCombatIn = -1.0;
		if (Mood == EMusicMood::Combat && !CombatLayer)
		{
			CombatLayer = StartTrack(MusicRules::Combat().Cue, MusicRules::CombatFadeInSeconds, CombatGain);
		}
	}
	if (PendingCombatOut >= 0.0 && At >= PendingCombatOut)
	{
		PendingCombatOut = -1.0;
		FadeAway(CombatLayer, static_cast<float>(MusicRules::CombatOutBars * GridBar));
		CombatLayer = nullptr;
		// The theme picks up after the fight, unless it's resting.
		if (!Theme && Mood == EMusicMood::Calm && At >= RestUntil)
		{
			PendingTheme = At + MusicRules::CombatOutBars * GridBar;
		}
	}
	if (PendingVictory >= 0.0 && At >= PendingVictory)
	{
		PendingVictory = -1.0;
		PlaySting(LooterSoundCue::StingVictory);
	}
	if (PendingTheme >= 0.0 && At >= PendingTheme)
	{
		PendingTheme = -1.0;
		if (Mood == EMusicMood::Calm && !Theme)
		{
			StartTheme(At);
		}
	}
}

void UMusicDirectorSubsystem::StartTheme(double At)
{
	const FMusicPiece Piece = MusicRules::ExploreFor(Area);
	Theme = StartTrack(Piece.Cue, MusicRules::ExploreFadeInSeconds, ThemeGain);
	if (!Theme)
	{
		// No sound for it yet: look again in a while rather than every frame.
		RestUntil = At + 30.0;
		return;
	}
	ThemeStarted = At;
	GridOrigin = At;
	GridBar = Piece.BarSeconds;
	GridBeat = Piece.BeatSeconds();
	// With a player it rests after a few plays; in the main menu (no player) it plays on.
	const int32 Loops = HasPlayer() ? MusicRules::CalmLoops(FMath::FRand()) : 0;
	ThemeBowsOut = Loops > 0 ? At + Loops * Piece.LoopSeconds() - MusicRules::ExploreOutBars * Piece.BarSeconds : Never;
}

void UMusicDirectorSubsystem::TickCalm(double At)
{
	if (Mood != EMusicMood::Calm)
	{
		return;
	}
	if (Theme)
	{
		if (At >= ThemeBowsOut)
		{
			// It bows out over its last two bars (the guitar alone), and the world has the floor for a while.
			FadeAway(Theme, static_cast<float>(MusicRules::ExploreOutBars * GridBar));
			Theme = nullptr;
			RestUntil = At + MusicRules::CalmRestSeconds(FMath::FRand());
		}
		return;
	}
	const bool bBusy = CombatLayer || BossTrack || PendingTheme >= 0.0 || PendingCombatOut >= 0.0 || PendingBoss >= 0.0;
	if (!bBusy && At >= RestUntil)
	{
		StartTheme(At);
	}
}

void UMusicDirectorSubsystem::ApplyGains(double At)
{
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	const UWorld* World = GetWorld();
	const float Duck = MusicRules::DuckFor(Scenes && Scenes->IsPlaying(), World && World->IsPaused());
	auto Move = [](UAudioComponent* Track, float& Gain, float Wanted)
	{
		if (!FMath::IsNearlyEqual(Gain, Wanted, 0.01f))
		{
			Gain = Wanted;
			if (Track)
			{
				Track->AdjustVolume(MusicRules::DuckSeconds, Wanted);
			}
		}
	};
	Move(Theme, ThemeGain, Duck * (CombatLayer ? MusicRules::UnderCombat : 1.f));
	Move(CombatLayer, CombatGain, Duck);
	Move(BossTrack, BossGain, Duck * (At < PhaseDuckUntil ? MusicRules::PhaseDuck : 1.f));
}

UAudioComponent* UMusicDirectorSubsystem::StartTrack(FName Cue, float FadeSeconds, float Gain)
{
	ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(this);
	if (!Sounds)
	{
		return nullptr;
	}
	const ULooterSoundSubsystem::FPlay Play = Sounds->Prepare(Cue);
	if (!Play.IsValid())
	{
		return nullptr;
	}
	// Pitch 1 always: the pieces layer on one grid.
	UAudioComponent* Track = UGameplayStatics::CreateSound2D(this, Play.Sound, Play.Volume, 1.f, 0.f, Play.Concurrency,
		/*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ false);
	if (!Track)
	{
		return nullptr;
	}
	// The music goes on over a paused game (quieter: ApplyGains); a UI sound isn't paused with the world.
	Track->bIsUISound = true;
	Track->FadeIn(FMath::Max(FadeSeconds, 0.01f), Gain);
	return Track;
}

void UMusicDirectorSubsystem::FadeAway(UAudioComponent* Track, float Seconds)
{
	if (IsValid(Track))
	{
		// It goes once quiet.
		Track->bAutoDestroy = true;
		Track->FadeOut(FMath::Max(Seconds, 0.05f), 0.f);
	}
}

void UMusicDirectorSubsystem::PlaySting(FName Cue)
{
	LooterSound::Play2D(this, Cue);
}

void UMusicDirectorSubsystem::Force(EMusicMood InMood, EBossTheme InTheme)
{
	bForced = true;
	ForcedMood = InMood;
	ForcedTheme = InTheme;
	NextLook = 0.0;
}

void UMusicDirectorSubsystem::Release()
{
	bForced = false;
	NextLook = 0.0;
}

FString UMusicDirectorSubsystem::Describe() const
{
	const double At = Now();
	FString ThemeState;
	if (Theme)
	{
		const int32 Bar = GridBar > 0.0 ? FMath::FloorToInt32((At - ThemeStarted) / GridBar) % MusicRules::ExploreFor(Area).Bars + 1 : 0;
		ThemeState = FString::Printf(TEXT("theme playing (bar %d)"), Bar);
	}
	else if (PendingTheme >= 0.0)
	{
		ThemeState = FString::Printf(TEXT("theme in %.1f s"), PendingTheme - At);
	}
	else
	{
		ThemeState = FString::Printf(TEXT("theme resting %.0f s more"), FMath::Max(0.0, RestUntil - At));
	}
	return FString::Printf(TEXT("Music: %s%s; %s; combat layer %s; boss track %s; %d hunting, fight %.0f s with %d kill(s)."),
		MoodName(Mood), bForced ? TEXT(" (forced)") : TEXT(""), *ThemeState, CombatLayer ? TEXT("on") : TEXT("off"),
		BossTrack ? (BossTheme == EBossTheme::Keeper ? TEXT("Abel's") : TEXT("the Gravemother's")) : TEXT("off"),
		Senses.Hunters, Memory.bInCombat ? At - Memory.StartedAt : 0.0, FightKills);
}
