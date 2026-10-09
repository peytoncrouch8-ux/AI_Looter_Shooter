#include "Audio/MusicRules.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureRank.h"

namespace
{
	/** 6/8 with the dotted quarter at 75: a bar of two beats is 1.6 s (music.py's SIX_EIGHT). */
	constexpr float SixEightBar = 1.6f;

	FMusicPiece Piece(const TCHAR* Cue, int32 Bars, int32 BeatsPerBar, float BarSeconds)
	{
		FMusicPiece P;
		P.Cue = FName(Cue);
		P.Bars = Bars;
		P.BeatsPerBar = BeatsPerBar;
		P.BarSeconds = BarSeconds;
		return P;
	}
}

bool FCombatMemory::Update(bool bHunted, double Now, float CalmAfter)
{
	if (bHunted)
	{
		LastHunted = Now;
		if (!bInCombat)
		{
			bInCombat = true;
			StartedAt = Now;
			return true;
		}
		return false;
	}
	if (bInCombat && Now - LastHunted >= CalmAfter)
	{
		bInCombat = false;
		return true;
	}
	return false;
}

FMusicPiece MusicRules::ExploreFor(ELooterAudioArea Area)
{
	if (Area == ELooterAudioArea::RansomsRest)
	{
		return Piece(LooterSoundCue::MusicRansomsRestExplore, 32, 2, SixEightBar);
	}
	return Piece(LooterSoundCue::MusicSkyreachExplore, 32, 2, SixEightBar);
}

FMusicPiece MusicRules::Combat()
{
	return Piece(LooterSoundCue::MusicCombat, 16, 2, SixEightBar);
}

FMusicPiece MusicRules::BossFor(EBossTheme Theme)
{
	// 4/4: a bar is four quarters at 128 (1.875 s) for Abel, at 120 (2 s) for the Gravemother.
	if (Theme == EBossTheme::Keeper)
	{
		return Piece(LooterSoundCue::MusicBossKeeper, 16, 4, 1.875f);
	}
	return Piece(LooterSoundCue::MusicBossGravemother, 16, 4, 2.f);
}

TArray<FMusicPiece> MusicRules::AllPieces()
{
	return { ExploreFor(ELooterAudioArea::Skyreach), ExploreFor(ELooterAudioArea::RansomsRest), Combat(),
		BossFor(EBossTheme::Keeper), BossFor(EBossTheme::Gravemother) };
}

double MusicRules::NextBoundary(double Now, double Origin, double Step)
{
	if (!(Step > 0.0) || Now <= Origin)
	{
		return FMath::Max(Now, Origin);
	}
	const double Passed = FMath::Fmod(Now - Origin, Step);
	if (Passed <= BoundaryGrace)
	{
		return Now;
	}
	return Now + (Step - Passed);
}

EMusicMood MusicRules::Resolve(bool bBossFight, bool bInCombat)
{
	if (bBossFight)
	{
		return EMusicMood::Boss;
	}
	return bInCombat ? EMusicMood::Combat : EMusicMood::Calm;
}

float MusicRules::DuckFor(bool bScenePlaying, bool bPaused)
{
	float Gain = 1.f;
	if (bScenePlaying)
	{
		Gain = FMath::Min(Gain, SceneDuck);
	}
	if (bPaused)
	{
		Gain = FMath::Min(Gain, PauseDuck);
	}
	return Gain;
}

bool MusicRules::EarnsVictory(double FightSeconds, int32 Kills)
{
	return Kills > 0 && FightSeconds >= VictoryMinFightSeconds;
}

bool MusicRules::IsElite(ECreatureRank Rank)
{
	return Rank == ECreatureRank::Epic || Rank == ECreatureRank::Legendary;
}

EBossTheme MusicRules::ThemeFor(bool bUnpaidBoss)
{
	return bUnpaidBoss ? EBossTheme::Keeper : EBossTheme::Gravemother;
}

int32 MusicRules::CalmLoops(float Roll)
{
	return FMath::Clamp(Roll, 0.f, 1.f) < 0.5f ? 2 : 3;
}

float MusicRules::CalmRestSeconds(float Roll)
{
	return FMath::Lerp(MinRestSeconds, MaxRestSeconds, FMath::Clamp(Roll, 0.f, 1.f));
}
