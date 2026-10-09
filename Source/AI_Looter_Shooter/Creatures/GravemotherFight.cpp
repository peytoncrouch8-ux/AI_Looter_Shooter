#include "Creatures/GravemotherFight.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/GravemotherCreature.h"

#define LOCTEXT_NAMESPACE "LooterGravemother"

namespace GravemotherFight
{
	TArray<FBossPhase> MakePhases()
	{
		// Her own code plays each phase (the brood's calls, the roars, the quake): the phases are her bar's cuts and names.
		FBossPhase Meat;
		Meat.Name = LOCTEXT("PhaseMeat", "Fresh Meat");
		Meat.HealthShare = 1.f;

		FBossPhase Brood;
		Brood.Name = LOCTEXT("PhaseBrood", "The Brood Wakes");
		Brood.HealthShare = Gravemother::BroodCallShare(0);

		FBossPhase Fury;
		Fury.Name = LOCTEXT("PhaseFury", "Mother's Fury");
		Fury.HealthShare = Gravemother::BroodCallShare(1);

		return { Meat, Brood, Fury };
	}

	FPace PaceFor(int32 Phase)
	{
		FPace Pace;
		if (Phase >= 1)
		{
			// The brood awake: the quake for whoever hugs her, a charge a little sooner, a spit a little sooner.
			Pace.ChargeCooldownScale = 0.875f;
			Pace.SpitCooldown = 6.f;
			Pace.bQuake = true;
			Pace.QuakeCooldown = 9.f;
		}
		if (Phase >= 2)
		{
			// Her fury: quicker in everything, and her cracks burn. The telegraph stays over a second: still readable.
			Pace.ChargeCooldownScale = 0.69f;
			Pace.ChargeTelegraphScale = 0.87f;
			Pace.ChaseScale = 1.15f;
			Pace.SpitCooldown = 4.5f;
			Pace.SpitPellets = 7;
			Pace.SpitSpread = 22.f;
			Pace.QuakeCooldown = 7.f;
			Pace.bBurningCracks = true;
		}
		return Pace;
	}

	FBossVolley Spit(int32 Pellets, float Spread)
	{
		FBossVolley Volley;
		Volley.Pellets = FMath::Clamp(Pellets, 1, 24);
		Volley.SpreadDegrees = Spread;
		// Slow and bright: seen leaving her fangs and stepped out of.
		Volley.Speed = 1100.f;
		Volley.DamageShare = 0.3f;
		Volley.Radius = 13.f;
		Volley.Range = 3200.f;
		// The tell is hers: she rears with the glow at her fangs through her wind-up, so the pellets leave as she strikes.
		Volley.WindupSeconds = 0.f;
		// Her fangs, a little up, at a spider's size (they scale with her).
		Volley.Muzzle = FVector(95.0, 0.0, 15.0);
		Volley.Color = FLinearColor(0.6f, 1.f, 0.18f);
		return Volley;
	}

	FBossShow MakeShow()
	{
		FBossShow Show;
		Show.Title = LOCTEXT("Title", "Brood of the Sink");
		// Her roars are her own moves (the scream as she rears, the shake as her forelegs slam down): none from the bar.
		Show.IntroShake = 0.f;
		Show.PhaseShake = 0.f;
		Show.StaggerShake = 0.35f;
		Show.DeathShake = 1.f;
		Show.DeathCue = LooterSoundCue::GravemotherDeath;
		Show.DeathSlowMo = 0.3f;
		Show.DeathSlowSeconds = 0.8f;
		Show.DefeatedLine = LOCTEXT("Slain", "Slain");
		return Show;
	}

	FBossStagger MakeStagger()
	{
		FBossStagger Stagger;
		Stagger.CritShare = 0.07f;
		Stagger.DrainSeconds = 4.f;
		Stagger.Seconds = 2.5f;
		Stagger.Cooldown = 9.f;
		return Stagger;
	}

	FBossLootShowerSettings MakeLootShower()
	{
		FBossLootShowerSettings Shower;
		Shower.BonusAmmo = 2;
		// Her body falls first.
		Shower.Delay = 1.1f;
		// Her body is over two metres across: everything lands clear of it.
		Shower.MinReach = 220.f;
		Shower.MaxReach = 480.f;
		return Shower;
	}
}

#undef LOCTEXT_NAMESPACE
