#include "Creatures/GravemotherFight.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherTuning.h"

#define LOCTEXT_NAMESPACE "LooterGravemother"

namespace GravemotherFight
{
	TArray<FBossPhase> MakePhases()
	{
		// Her own code plays each phase (the mounds, the roars, the quake, the stone): the phases are her bar's cuts and names.
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
			// The brood awake: the quake for whoever hugs her, a furrow and a clod a little sooner.
			Pace.FurrowCooldownScale = 0.875f;
			Pace.ClodCooldown = 6.f;
			Pace.bQuake = true;
			Pace.QuakeCooldown = 9.f;
		}
		if (Phase >= 2)
		{
			// Her fury: a quarter faster on her feet (the user's +25% in the last third), every move sooner. The tells keep
			// the user's timings.
			Pace.FurrowCooldownScale = 0.69f;
			Pace.ClodCooldown = 4.5f;
			Pace.QuakeCooldown = 7.f;
			Pace.SpeedScale = 1.25f;
		}
		return Pace;
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
		// 7% of her health as first tuned: her health went up 1.6x (2026-10-27), her staggers didn't get harder to earn.
		Stagger.CritShare = 0.07f / GravemotherTuning::DefaultHealthScale;
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

	FText StunCallout()
	{
		return LOCTEXT("Stunned", "Stunned");
	}

	FText ShrugCallout(EGravemotherShrug Why)
	{
		switch (Why)
		{
		case EGravemotherShrug::Stone:
			return LOCTEXT("ShrugStone", "Break the headstone");
		case EGravemotherShrug::OffGround:
			return LOCTEXT("ShrugRim", "Out of reach");
		default:
			return FText::GetEmpty();
		}
	}

	FStoryLine ShrugLine(EGravemotherShrug Why)
	{
		// Hob, as he talks on the deck ("sunshine"): short, a plain instruction inside his voice.
		const FText Hob = LOCTEXT("Hob", "Hob");
		switch (Why)
		{
		case EGravemotherShrug::Stone:
			return FStoryLine::Make(Hob, LOCTEXT("HintStone", "That headstone's rooted to her back. Crack it off, sunshine, or you're only tickling her."), 4.2f);
		case EGravemotherShrug::OffGround:
			return FStoryLine::Make(Hob, LOCTEXT("HintRim", "Waste of lead from up here. She only feels it down in the Sink with her."), 3.8f);
		default:
			return FStoryLine();
		}
	}

	float ClodFlightSeconds(float Distance, float AtTwentyMetres)
	{
		return FMath::Clamp(AtTwentyMetres * FMath::Sqrt(FMath::Max(Distance, 0.f) / 2000.f), 0.8f, 1.7f);
	}

	float HeaveFront(float TellTime, float AimAt, float CrawlEnd, float Length)
	{
		const float Span = FMath::Max(CrawlEnd - AimAt, 0.01f);
		return FMath::Max(Length, 0.f) * FMath::Clamp((TellTime - AimAt) / Span, 0.f, 1.f);
	}

	EGravemotherStone StoneStateFor(float Share, float CrackedAt, float ShatteredAt)
	{
		if (Share <= 0.f)
		{
			return EGravemotherStone::Broken;
		}
		if (Share <= ShatteredAt)
		{
			return EGravemotherStone::Shattered;
		}
		return Share <= CrackedAt ? EGravemotherStone::Cracked : EGravemotherStone::Intact;
	}

	float BermRise(float Age, float RiseSeconds)
	{
		return Age <= 0.f ? 0.f : FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Age / FMath::Max(RiseSeconds, 0.01f), 0.f, 1.f), 2.f);
	}
}

#undef LOCTEXT_NAMESPACE
