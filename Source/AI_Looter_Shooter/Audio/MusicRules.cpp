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
	// 4/4, a bar four quarters (Docs/Archive/Work/2026-10-31-Score/CONTRACT.md; the recipes' meters): Abel at 128, the Gravemother
	// at 120, Cyrus Teague at 112 (32 bars each), the Stone Sentinel at 100 (24 bars: its fights are shorter).
	switch (Theme)
	{
	case EBossTheme::Keeper:
		return Piece(LooterSoundCue::MusicBossKeeper, 32, 4, 4.f * 60.f / 128.f);
	case EBossTheme::Foreman:
		return Piece(LooterSoundCue::MusicBossForeman, 32, 4, 4.f * 60.f / 112.f);
	case EBossTheme::Sentinel:
		return Piece(LooterSoundCue::MusicBossSentinel, 24, 4, 4.f * 60.f / 100.f);
	default:
		return Piece(LooterSoundCue::MusicBossGravemother, 32, 4, 4.f * 60.f / 120.f);
	}
}

FMusicPiece MusicRules::RageFor(EBossTheme Theme)
{
	FMusicPiece Rage = BossFor(Theme);
	switch (Theme)
	{
	case EBossTheme::Keeper:
		Rage.Cue = FName(LooterSoundCue::MusicBossKeeperRage);
		break;
	case EBossTheme::Foreman:
		Rage.Cue = FName(LooterSoundCue::MusicBossForemanRage);
		break;
	case EBossTheme::Sentinel:
		Rage.Cue = FName(LooterSoundCue::MusicBossSentinelRage);
		break;
	default:
		Rage.Cue = FName(LooterSoundCue::MusicBossGravemotherRage);
		break;
	}
	return Rage;
}

FMusicPiece MusicRules::Menu()
{
	// 4/4 at 72: a bar of 3.333 s, 32 bars (106.7 s) round.
	return Piece(LooterSoundCue::MusicMenuTheme, 32, 4, 4.f * 60.f / 72.f);
}

FMusicPiece MusicRules::CalmFor(ELooterAudioArea Area, bool bMenu)
{
	return bMenu ? Menu() : ExploreFor(Area);
}

TArray<EBossTheme> MusicRules::AllBossThemes()
{
	return { EBossTheme::Keeper, EBossTheme::Gravemother, EBossTheme::Foreman, EBossTheme::Sentinel };
}

TArray<FMusicPiece> MusicRules::AllPieces()
{
	TArray<FMusicPiece> Pieces = { ExploreFor(ELooterAudioArea::Skyreach), ExploreFor(ELooterAudioArea::RansomsRest), Combat(), Menu() };
	for (const EBossTheme Theme : AllBossThemes())
	{
		Pieces.Add(BossFor(Theme));
		Pieces.Add(RageFor(Theme));
	}
	return Pieces;
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
	// Gravebound and above, short of a boss (whose fight has music of its own).
	return LooterRanks::IsAtLeast(Rank, ECreatureRank::Epic) && Rank != ECreatureRank::Boss;
}

EBossTheme MusicRules::ThemeFor(EBossKind Kind)
{
	switch (Kind)
	{
	case EBossKind::Foreman:
		return EBossTheme::Foreman;
	case EBossKind::Sentinel:
		return EBossTheme::Sentinel;
	case EBossKind::Unpaid:
		return EBossTheme::Keeper;
	default:
		return EBossTheme::Gravemother;
	}
}

bool MusicRules::WantsRage(int32 Phase, int32 NumPhases, float HealthShare)
{
	if (NumPhases >= 2)
	{
		return Phase >= NumPhases - 1;
	}
	return HealthShare <= RageHealthShare;
}

double MusicRules::LoopPlace(double Now, double StartedAt, double LoopSeconds)
{
	if (!(LoopSeconds > 0.0) || Now <= StartedAt)
	{
		return 0.0;
	}
	return FMath::Fmod(Now - StartedAt, LoopSeconds);
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
