#include "Creatures/ForemanFight.h"
#include "Audio/LooterSoundCues.h"
#include "Math/RandomStream.h"
#include "Weapons/LegendaryDrops.h"

#define LOCTEXT_NAMESPACE "LooterForeman"

namespace ForemanFight
{
	TArray<FBossPhase> MakePhases()
	{
		// His own code plays each phase (the shift, the dark, the roof): the phases are his bar's cuts and names.
		FBossPhase Fire;
		Fire.Name = LOCTEXT("PhaseFire", "Fire in the Hole");
		Fire.HealthShare = 1.f;

		FBossPhase Shift;
		Shift.Name = LOCTEXT("PhaseShift", "The Shift");
		Shift.HealthShare = Foreman::ShiftShare;

		FBossPhase Dark;
		Dark.Name = LOCTEXT("PhaseDark", "Lights Out");
		Dark.HealthShare = Foreman::DarkShare;

		FBossPhase Last;
		Last.Name = LOCTEXT("PhaseLast", "Last Shift");
		Last.HealthShare = Foreman::LastShiftShare;

		return { Fire, Shift, Dark, Last };
	}

	FPace PaceFor(int32 Phase)
	{
		FPace Pace;
		if (Phase >= Foreman::ShiftPhase)
		{
			// The shift up and the roof coming down: he starts on the props.
			Pace.bRoof = true;
		}
		if (Phase >= Foreman::DarkPhase)
		{
			Pace.FuseCooldown = 6.5f;
		}
		if (Phase >= Foreman::LastShiftPhase)
		{
			// The last shift: angrier in everything, his swing's wind-up still readable (over half a second).
			Pace.FuseCooldown = 5.5f;
			Pace.RoofEvery = 15.f;
			Pace.ChaseScale = 1.12f;
			Pace.SwingWindupScale = 0.85f;
		}
		return Pace;
	}

	FBossShow MakeShow()
	{
		FBossShow Show;
		Show.Title = LOCTEXT("Title", "Foreman of the Providence");
		// His mine whistle and bellow as the bar sweeps in; his slam is his own move, shaken by his own code.
		Show.IntroCue = LooterSoundCue::Foreman::Intro;
		Show.PhaseCue = LooterSoundCue::Foreman::Phase;
		Show.StaggerCue = LooterSoundCue::Foreman::Stagger;
		Show.DeathCue = LooterSoundCue::Foreman::Death;
		Show.IntroShake = 0.f;
		Show.PhaseShake = 0.5f;
		Show.StaggerShake = 0.35f;
		Show.DeathShake = 1.f;
		Show.DeathSlowMo = 0.3f;
		Show.DeathSlowSeconds = 0.9f;
		Show.DefeatedLine = LOCTEXT("Defeated", "Shift's Over");
		return Show;
	}

	FBossStagger MakeStagger()
	{
		// His coal is open only in its windows, so it takes less to stagger him than the Gravemother (7%).
		FBossStagger Stagger;
		Stagger.CritShare = 0.06f;
		Stagger.DrainSeconds = 4.f;
		Stagger.Seconds = 2.5f;
		Stagger.Cooldown = 10.f;
		return Stagger;
	}

	FBossLootShowerSettings MakeLootShower()
	{
		FBossLootShowerSettings Shower;
		Shower.BonusAmmo = 2;
		// He kneels and says his last word first.
		Shower.Delay = 3.f;
		// His body is a big man's at 1.5: everything lands clear of it.
		Shower.MinReach = 200.f;
		Shower.MaxReach = 460.f;
		// His own legendary, farmed at his vigil stone: Company Scrip.
		Shower.DedicatedLegendary = FName(LegendaryDrops::CompanyScripId);
		return Shower;
	}

	bool IsCoalOpen(bool bLightingFuse, float SinceRoofStrike, float RoofOpenSeconds, bool bStaggered)
	{
		return bLightingFuse || bStaggered || (SinceRoofStrike >= 0.f && SinceRoofStrike < RoofOpenSeconds);
	}

	float BurstFalloff(float Distance, float Radius, float CoreRadius, float EdgeShare)
	{
		const float Out = FMath::Max(Distance, 0.f);
		if (Out > Radius)
		{
			return 0.f;
		}
		if (Out <= CoreRadius || Radius <= CoreRadius)
		{
			return 1.f;
		}
		return FMath::Lerp(1.f, EdgeShare, (Out - CoreRadius) / (Radius - CoreRadius));
	}

	TArray<FVector> RoofSpots(const FVector& Player, const TOptional<FVector>& StruckProp, const TOptional<FVector>& CoverProp,
		const FVector& ArenaCenter, float ArenaRadius, int32 Count, float Spacing, float FillDistance, FRandomStream& Random)
	{
		TArray<FVector> Spots;
		auto Inside = [&](const FVector& Spot)
		{
			return FVector::Dist2D(Spot, ArenaCenter) <= ArenaRadius;
		};
		auto Clear = [&](const FVector& Spot)
		{
			for (const FVector& Taken : Spots)
			{
				if (FVector::Dist2D(Taken, Spot) < Spacing)
				{
					return false;
				}
			}
			return true;
		};
		auto Take = [&](const FVector& Spot)
		{
			if (Spots.Num() < Count && Inside(Spot) && Clear(Spot))
			{
				Spots.Add(FVector(Spot.X, Spot.Y, Player.Z));
			}
		};
		// The prop he struck, the player's spot, the cover they're using: the roof comes down on all three.
		if (StruckProp.IsSet())
		{
			Take(StruckProp.GetValue());
		}
		Take(Player);
		if (CoverProp.IsSet())
		{
			Take(CoverProp.GetValue());
		}
		// The rest round the player, a few metres off, so the way out of the dust isn't every way at once.
		const float Start = Random.FRandRange(0.f, 360.f);
		for (int32 Try = 0; Try < 12 && Spots.Num() < Count; ++Try)
		{
			const float Angle = Start + 360.f * static_cast<float>(Try) / 12.f * 5.f;
			const float Reach = FillDistance * (Try < 6 ? 1.f : 1.35f);
			Take(Player + FVector::ForwardVector.RotateAngleAxis(Angle, FVector::UpVector) * Reach);
		}
		return Spots;
	}

	FText SpeakerName()
	{
		return LOCTEXT("Speaker", "Cyrus Teague");
	}

	FStoryLine IntroLine()
	{
		return FStoryLine::Make(SpeakerName(), LOCTEXT("LineIntro", "Nobody leaves till the drift's cleared!"), 2.8f);
	}

	FStoryLine ShiftLine()
	{
		return FStoryLine::Make(SpeakerName(), LOCTEXT("LineShift", "Back to work!"), 1.8f);
	}

	FStoryLine FuseLine()
	{
		return FStoryLine::Make(SpeakerName(), LOCTEXT("LineFuse", "Fire in the hole!"), 1.6f);
	}

	FStoryLine DarkLine()
	{
		return FStoryLine::Make(SpeakerName(), LOCTEXT("LineDark", "I can dig them out. I can still dig them out."), 3.8f);
	}

	FStoryLine LastWord()
	{
		return FStoryLine::Make(SpeakerName(), LOCTEXT("LineLast", "...Tell the company it was the roof."), 4.2f);
	}
}

#undef LOCTEXT_NAMESPACE
