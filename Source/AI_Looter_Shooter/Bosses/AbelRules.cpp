#include "Bosses/AbelRules.h"
#include "Audio/LooterSoundCues.h"
#include "Bosses/AbelPoses.h"
#include "Creatures/UnpaidCreature.h"
#include "Weapons/LegendaryDrops.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "LooterAbel"

namespace
{
	/** The model stands on the capsule's foot: its middle is this far over the ground at size 1 (AUnpaidCreature's capsule). */
	constexpr float ModelOverMiddle = 90.f;

	TAutoConsoleVariable<float> CVarAbelHealthScale(TEXT("Looter.Abel.HealthScale"), AbelRules::DefaultFightHealthScale,
		TEXT("Abel's health against his first tuning (40 Unpaid of his level). Applies when he's next spawned."));

	TAutoConsoleVariable<float> CVarAbelDamageScale(TEXT("Looter.Abel.DamageScale"), AbelRules::DefaultFightDamageScale,
		TEXT("Abel's hits (his swing, his buckshot's pellets) against his first tuning. Applies when he's next spawned."));
}

namespace AbelRules
{
	float FightHealthScale()
	{
		return FMath::Max(CVarAbelHealthScale.GetValueOnGameThread(), 0.05f);
	}

	float FightDamageScale()
	{
		return FMath::Max(CVarAbelDamageScale.GetValueOnGameThread(), 0.05f);
	}

	FName BuckshotEvent()
	{
		return FName(TEXT("Buckshot"));
	}

	FName LungeAttack()
	{
		return FName(TEXT("Lunge"));
	}

	FName BuckshotAttack()
	{
		return FName(TEXT("Buckshot"));
	}

	FName GrieveEvent()
	{
		return FName(TEXT("Grieve"));
	}

	FName CallBackEvent()
	{
		return FName(TEXT("CallBack"));
	}

	FName TendrilEvent()
	{
		return FName(TEXT("Tendril"));
	}

	FName WalkOffEvent()
	{
		return FName(TEXT("WalkOff"));
	}

	FName FogShotEvent()
	{
		return FName(TEXT("FogShot"));
	}

	FBossShow MakeShow()
	{
		FBossShow Show;
		// (Main 7, 2026-11-01: was "Waiting on a Dark Saint". The bar's own label says SMOG; the pips under it say ABEL.)
		Show.Title = LOCTEXT("Title", "A Keeper Under Smog");
		Show.IntroCue = LooterSoundCue::AbelIntro;
		Show.PhaseCue = LooterSoundCue::AbelWail;
		Show.StaggerCue = LooterSoundCue::AbelStagger;
		Show.DeathCue = LooterSoundCue::AbelDeath;
		// His own entrance shakes the view as his lantern flares (AAbelKeeper's intro); the bell and the wind shake it here.
		Show.IntroShake = 0.f;
		Show.PhaseShake = 0.5f;
		Show.StaggerShake = 0.3f;
		Show.DeathShake = 0.8f;
		Show.DeathSlowMo = 0.3f;
		Show.DeathSlowSeconds = 0.9f;
		Show.DefeatedLine = LOCTEXT("SmogBreaks", "The smog breaks");
		return Show;
	}

	FBossStagger MakeStagger()
	{
		FBossStagger Stagger;
		// 5% of his health as first tuned: his health went up 1.6x (2026-10-27), his staggers didn't get harder to earn.
		Stagger.CritShare = 0.05f / DefaultFightHealthScale;
		// His coal is open for seconds at a time: the build-up keeps a little longer than the windows between.
		Stagger.DrainSeconds = 3.5f;
		Stagger.Seconds = 2.5f;
		Stagger.Cooldown = 8.f;
		return Stagger;
	}

	FBossLootShowerSettings MakeLootShower()
	{
		FBossLootShowerSettings Shower;
		Shower.BonusAmmo = 3;
		// He kneels (2 s), the last call at the keeper's post (a 3 s hold), then the scene (FreeAbel); the loot waits for the
		// scene if it has begun by then, and comes once the player has their eyes back. A player who walks off and leaves him
		// knelt gets it on the boards anyway (6 s), never lost.
		Shower.Delay = 6.f;
		Shower.bAfterScene = true;
		// From the deck's middle: he may fall at the open end, over the canyon.
		Shower.bFromSpot = true;
		Shower.MinReach = 160.f;
		Shower.MaxReach = 400.f;
		// His own legendary, farmed at his vigil stone: Lamplighter.
		Shower.DedicatedLegendary = FName(LegendaryDrops::LamplighterId);
		return Shower;
	}

	FBossVolley Buckshot()
	{
		FBossVolley Volley;
		Volley.Pellets = 7;
		Volley.SpreadDegrees = 18.f;
		// Slow and pale: seen leaving the pump and stepped out of.
		Volley.Speed = 950.f;
		Volley.DamageShare = 0.35f;
		Volley.Radius = 11.f;
		Volley.Range = 3500.f;
		// The tell is his: the lantern's flare (AAbelKeeper), so the pellets leave as the shot comes.
		Volley.WindupSeconds = 0.f;
		// Where the pump's muzzle is in the shot, from his middle (he aims from the pump's own socket when it has one).
		Volley.Muzzle = AbelPoses::FireMuzzle() - FVector(0.0, 0.0, ModelOverMiddle);
		Volley.Color = FLinearColor(0.72f, 0.86f, 1.f);
		return Volley;
	}

	FBossAddWave RisingUnpaid(int32 Count, int32 MaxAlive)
	{
		FBossAddWave Wave;
		Wave.CreatureClass = AUnpaidCreature::StaticClass();
		Wave.Rank = ECreatureRank::Basic;
		Wave.Count = Count;
		Wave.MaxAlive = MaxAlive;
		// Round the deck's middle, between the biers, on its boards: wherever he is (out over the canyon in the second phase).
		Wave.bAroundSpot = true;
		Wave.Radius = 650.f;
		return Wave;
	}

	FBossUntargetable FogSpell()
	{
		// Out over the canyon in the fog, drinking the dead: he can't be hurt until the three lanterns drag him back (his own
		// code ends the spell), and he leaves the fight (withdrawn) while he's out there.
		FBossUntargetable InTheFog;
		InTheFog.Seconds = 0.f;
		InTheFog.bUntilAddsDie = false;
		InTheFog.bWithdraw = true;
		InTheFog.Hint = LOCTEXT("RelightHint", "Relight the lanterns. Shoot the smog lines");
		return InTheFog;
	}

	TArray<FBossPhase> MakePhases()
	{
		// "Stoker" (100-66%), the furnace in him: the buckshot after the black lantern's flare, the smog tendril, two Unpaid
		// every 25 s (at most 4), and every 12 s he turns to the six dead on the boards for 3 s, his coal open (the keeper in
		// him trying to walk them; the event keeps its old name, Grieve).
		FBossPhase Stoker;
		Stoker.Name = LOCTEXT("PhaseStoker", "Stoker");
		Stoker.HealthShare = 1.f;
		Stoker.Events.Add(FBossPhaseEvent::MakeCustom(BuckshotEvent(), 4.f, BuckshotEvery));
		Stoker.Events.Add(FBossPhaseEvent::MakeCustom(GrieveEvent(), GrieveEvery, GrieveEvery));
		Stoker.Events.Add(FBossPhaseEvent::MakeCustom(TendrilEvent(), TendrilFirst, TendrilEvery));
		Stoker.Events.Add(FBossPhaseEvent::MakeWave(RisingUnpaid(RiseCount, RiseMaxAlive), RiseEvery, RiseEvery));

		// "The Hose" (66-33%): the call-back first (his bar locks, he kneels: the keeper's post), then out over the canyon
		// into the fog (the call-back's end starts it: UAbelCorruptionComponent), the lanterns dark, a smog line to one of the
		// six dead at a time, and 8 Unpaid in two waves. Relit, the lanterns drag him back.
		FBossPhase Hose;
		Hose.Name = LOCTEXT("PhaseHose", "The Hose");
		Hose.HealthShare = HoseShare;
		Hose.Events.Add(FBossPhaseEvent::MakeCustom(CallBackEvent()));
		Hose.Events.Add(FBossPhaseEvent::MakeWave(RisingUnpaid(BellWave, 0), FirstWaveAt));
		Hose.Events.Add(FBossPhaseEvent::MakeWave(RisingUnpaid(BellWave, 0), FirstWaveAt + SecondWaveAfter));
		// Then the dead keep rising as in phase one (his health 1.6x, 2026-10-27: the back half on the deck is long).
		Hose.Events.Add(FBossPhaseEvent::MakeWave(RisingUnpaid(RiseCount, RiseMaxAlive), HoseRiseFrom, RiseEvery));
		// Out in the fog his black lantern flares, a beacon over the canyon, and he fires from it: the lanterns are relit and
		// the smog lines shot under fire, from behind the biers.
		Hose.Events.Add(FBossPhaseEvent::MakeCustom(FogShotEvent(), FogShotFirst, FogShotEvery));
		// Back on the deck he fights on as before (out in the fog these wait for him).
		Hose.Events.Add(FBossPhaseEvent::MakeCustom(BuckshotEvent(), 16.f, BuckshotEvery));
		Hose.Events.Add(FBossPhaseEvent::MakeCustom(GrieveEvent(), 20.f, GrieveEvery));
		Hose.Events.Add(FBossPhaseEvent::MakeCustom(TendrilEvent(), 18.f, TendrilEvery));

		// "Keeper" (33-0%): the call-back again, then the smog from the wreck pours up over the deck (the fog surge) and the
		// Gravewind gusts (both started by the call-back's end); he fights faster (two tendrils, his buckshot a barrage), and
		// between attacks the keeper in him walks to a lantern post to hang his black lantern there and the furnace pulls him
		// back (a stun, his coal open).
		FBossPhase Keeper;
		Keeper.Name = LOCTEXT("PhaseKeeper", "Keeper");
		Keeper.HealthShare = KeeperShare;
		Keeper.Events.Add(FBossPhaseEvent::MakeCustom(CallBackEvent()));
		Keeper.Events.Add(FBossPhaseEvent::MakeCustom(WalkOffEvent(), 10.f, WalkOffEvery));
		Keeper.Events.Add(FBossPhaseEvent::MakeCustom(BuckshotEvent(), 8.f, WindBuckshotEvery));
		Keeper.Events.Add(FBossPhaseEvent::MakeCustom(TendrilEvent(), 12.f, KeeperTendrilEvery));

		return { Stoker, Hose, Keeper };
	}

	float DragShare(int32 LitLanterns, int32 TotalLanterns)
	{
		return TotalLanterns > 0 ? FMath::Clamp(static_cast<float>(LitLanterns) / static_cast<float>(TotalLanterns), 0.f, 1.f) : 1.f;
	}

	FVector GlideAt(const FVector& From, const FVector& To, float Alpha, float Lift)
	{
		const float Eased = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
		return FMath::Lerp(From, To, Eased) + FVector(0.0, 0.0, Lift * FMath::Sin(UE_PI * Eased));
	}

	float GustStrength(const FAbelGustRules& Rules, float WindTime, int32* OutGust)
	{
		const float Since = WindTime - Rules.FirstAfter;
		if (OutGust)
		{
			*OutGust = Since < 0.f ? 0 : 1 + FMath::FloorToInt32(Since / FMath::Max(Rules.Every, 0.1f));
		}
		if (Since < 0.f)
		{
			return 0.f;
		}
		const float Into = FMath::Fmod(Since, FMath::Max(Rules.Every, 0.1f));
		if (Into > Rules.Seconds)
		{
			return 0.f;
		}
		// Up over its ramp, held, down over its ramp.
		const float Ramp = FMath::Max(Rules.Ramp, KINDA_SMALL_NUMBER);
		const float Rise = FMath::Min3(Into / Ramp, (Rules.Seconds - Into) / Ramp, 1.f);
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Rise, 0.f, 1.f));
	}

	double FallSpeedPastEnd(const FAbelGustRules& Rules, double VelocityZ)
	{
		return FMath::Min(VelocityZ, -static_cast<double>(Rules.Downdraft));
	}

	FStoryLine PhaseLine(int32 Phase)
	{
		// The furnace talking with his mouth (drafts; no family words: he doesn't know the Warden). His own words come back a
		// third at a time with the call-backs (DA_Lines_AbelCallBack1/2), and whole in FreeAbel.
		const FText Abel = LOCTEXT("Abel", "Abel");
		switch (Phase)
		{
		case 0:
			return FStoryLine::Make(Abel, LOCTEXT("LineStoker", "Off my boards. These six are mine to walk."), 3.2f);
		case 1:
			return FStoryLine::Make(Abel, LOCTEXT("LineHose", "So hungry. They smell like home."), 3.f);
		case 2:
			return FStoryLine::Make(Abel, LOCTEXT("LineKeeper", "Let it burn. Let the whole Rest burn down."), 3.2f);
		default:
			return FStoryLine();
		}
	}

	FStoryLine HobOnFall()
	{
		return FStoryLine::Make(LOCTEXT("Hob", "Hob"), LOCTEXT("HobFall", "Caught you. Keep off the end, sunshine. The wind's his, not yours."), 3.6f);
	}

	void ShineOnGhostChannel(ULightComponent& Light)
	{
		static_assert(GhostLightChannel == 2, "The channels below are written out for channel 2");
		// Written straight in (as the stands' lights are), so it holds in a constructor too; drawn again if it's showing.
		Light.LightingChannels.bChannel0 = false;
		Light.LightingChannels.bChannel1 = false;
		Light.LightingChannels.bChannel2 = true;
		Light.MarkRenderStateDirty();
	}

	bool IsOnGhostChannel(const ULightComponent& Light)
	{
		return !Light.LightingChannels.bChannel0 && !Light.LightingChannels.bChannel1 && Light.LightingChannels.bChannel2;
	}

	void LetGhostLightReach(UPrimitiveComponent& Part)
	{
		Part.SetLightingChannels(Part.LightingChannels.bChannel0, Part.LightingChannels.bChannel1, /*bChannel2*/ true);
	}

	void LetGhostLightReach(AActor& Actor)
	{
		TInlineComponentArray<UPrimitiveComponent*> Parts(&Actor);
		for (UPrimitiveComponent* Part : Parts)
		{
			if (Part)
			{
				LetGhostLightReach(*Part);
			}
		}
	}

	bool DoesGhostLightReach(const UPrimitiveComponent& Part)
	{
		return Part.LightingChannels.bChannel2;
	}
}

#undef LOCTEXT_NAMESPACE
