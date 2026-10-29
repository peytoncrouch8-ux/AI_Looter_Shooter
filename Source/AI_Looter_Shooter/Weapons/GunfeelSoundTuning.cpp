#include "Weapons/GunfeelSoundTuning.h"
#include "Audio/LooterSoundCues.h"
#include "HAL/IConsoleManager.h"
#include "Weapons/GunfeelKickTuning.h"

// The Looter.Gunfeel.Sound.* variables (GunfeelSoundTuning.h) and the plain rules of a shot's layers, an automatic's tails,
// the end of a magazine, and how impacts and creature hits sound by gun. Each accessor reads its variable when asked, never
// caching it, so a change in play is heard on the next shot.

namespace
{
	namespace Def = GunfeelSound::Default;

	TAutoConsoleVariable<float> CVarTail(TEXT("Looter.Gunfeel.Sound.Tail"), Def::Tail,
		TEXT("The shot's tail (the land answering: open, town or inside) at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMech(TEXT("Looter.Gunfeel.Sound.Mech"), Def::Mech,
		TEXT("The shooter's own gun's action (bolt, pump, cylinder) at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLow(TEXT("Looter.Gunfeel.Sound.Low"), Def::Low,
		TEXT("The shooter's own gun's chest thump at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDistantFrom(TEXT("Looter.Gunfeel.Sound.DistantFrom"), Def::DistantFrom,
		TEXT("Someone else's shot farther than this from the listener (cm) is heard as the far shot."), ECVF_Default);
	TAutoConsoleVariable<float> CVarProbeRange(TEXT("Looter.Gunfeel.Sound.ProbeRange"), Def::ProbeRange,
		TEXT("How far a shot's surroundings probe looks for a roof and walls (cm)."), ECVF_Default);

	// An automatic's tail every Nth shot of a burst (a long burst at 900 rpm would stack 60 tails a second and cut itself off).
	TAutoConsoleVariable<int32> CVarTailEveryRifle(TEXT("Looter.Gunfeel.Sound.AutoTailEvery.Rifle"), Def::AutoTailRifle,
		TEXT("The rifle plays its tail on every Nth shot of a burst (1 = every shot)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarTailEverySMG(TEXT("Looter.Gunfeel.Sound.AutoTailEvery.SMG"), Def::AutoTailSMG,
		TEXT("The SMG plays its tail on every Nth shot of a burst (1 = every shot)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarTailEveryLMG(TEXT("Looter.Gunfeel.Sound.AutoTailEvery.LMG"), Def::AutoTailLMG,
		TEXT("The LMG plays its tail on every Nth shot of a burst (1 = every shot)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstEndTail(TEXT("Looter.Gunfeel.Sound.BurstEndTail"), Def::BurstEndTail,
		TEXT("The tail played once as a burst ends, at this volume (0 = none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBurstGap(TEXT("Looter.Gunfeel.Sound.BurstGap"), Def::BurstGap,
		TEXT("Shots further apart than this (s) are not one burst."), ECVF_Default);

	// The end of a magazine: how many rounds click, by kind.
	TAutoConsoleVariable<int32> CVarLowRifle(TEXT("Looter.Gunfeel.Sound.LowAmmo.Rifle"), Def::LowAmmoRifle,
		TEXT("Rounds left that click on the rifle (0 = none; never more than half the magazine)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarLowSMG(TEXT("Looter.Gunfeel.Sound.LowAmmo.SMG"), Def::LowAmmoSMG,
		TEXT("Rounds left that click on the SMG."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarLowLMG(TEXT("Looter.Gunfeel.Sound.LowAmmo.LMG"), Def::LowAmmoLMG,
		TEXT("Rounds left that click on the LMG."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarLowShotgun(TEXT("Looter.Gunfeel.Sound.LowAmmo.Shotgun"), Def::LowAmmoShotgun,
		TEXT("Rounds left that click on the shotgun."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarLowRevolver(TEXT("Looter.Gunfeel.Sound.LowAmmo.Revolver"), Def::LowAmmoRevolver,
		TEXT("Rounds left that click on the revolver."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarLowSniper(TEXT("Looter.Gunfeel.Sound.LowAmmo.Sniper"), Def::LowAmmoSniper,
		TEXT("Rounds left that click on the sniper."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarLowLauncher(TEXT("Looter.Gunfeel.Sound.LowAmmo.Launcher"), Def::LowAmmoLauncher,
		TEXT("Rounds left that click on the launcher."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLastRound(TEXT("Looter.Gunfeel.Sound.LastRound"), Def::LastRound,
		TEXT("The sound of the last round gone, at this volume (0 = none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLastRoundDelay(TEXT("Looter.Gunfeel.Sound.LastRoundDelay"), Def::LastRoundDelay,
		TEXT("The last-round sound comes this long after the shot (s), so the shot does not bury it."), ECVF_Default);

	TAutoConsoleVariable<float> CVarHissFrom(TEXT("Looter.Gunfeel.Sound.HissFrom"), Def::HissFrom,
		TEXT("An LMG burst that ends with the barrel hotter than this (0..1) hisses."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHiss(TEXT("Looter.Gunfeel.Sound.Hiss"), Def::Hiss,
		TEXT("The hot barrel's hiss at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRack(TEXT("Looter.Gunfeel.Sound.Rack"), Def::Rack,
		TEXT("The shotgun's rack between shots at this volume."), ECVF_Default);

	TAutoConsoleVariable<float> CVarImpactBase(TEXT("Looter.Gunfeel.Sound.Impact.Base"), Def::ImpactBase,
		TEXT("The surface cue's loudness is Base + Slope x the gun's mark scale, held between Min and Max."), ECVF_Default);
	TAutoConsoleVariable<float> CVarImpactSlope(TEXT("Looter.Gunfeel.Sound.Impact.Slope"), Def::ImpactSlope,
		TEXT("See Impact.Base."), ECVF_Default);
	TAutoConsoleVariable<float> CVarImpactMin(TEXT("Looter.Gunfeel.Sound.Impact.Min"), Def::ImpactMin,
		TEXT("See Impact.Base."), ECVF_Default);
	TAutoConsoleVariable<float> CVarImpactMax(TEXT("Looter.Gunfeel.Sound.Impact.Max"), Def::ImpactMax,
		TEXT("See Impact.Base."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScatterLouder(TEXT("Looter.Gunfeel.Sound.ScatterLouder"), Def::ScatterLouder,
		TEXT("A shotgun blast's one surface cue (its pellets merge) is louder by this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarImpactHeavyFrom(TEXT("Looter.Gunfeel.Sound.ImpactHeavyFrom"), Def::ImpactHeavyFrom,
		TEXT("Guns whose mark scale reaches this add the heavy thump and debris to an impact (0 = never)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarImpactHeavy(TEXT("Looter.Gunfeel.Sound.ImpactHeavy"), Def::ImpactHeavy,
		TEXT("The impact's heavy layer at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarImpactScatter(TEXT("Looter.Gunfeel.Sound.ImpactScatter"), Def::ImpactScatter,
		TEXT("A shotgun blast's pepper of pellets, once a blast, at this volume."), ECVF_Default);

	TAutoConsoleVariable<float> CVarCreatureHeavyFrom(TEXT("Looter.Gunfeel.Sound.CreatureHeavyFrom"), Def::CreatureHeavyFrom,
		TEXT("Guns whose mark scale reaches this add the heavy thump to a body hit (0 = never)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCreatureHeavy(TEXT("Looter.Gunfeel.Sound.CreatureHeavy"), Def::CreatureHeavy,
		TEXT("A body hit's heavy thump at this volume."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCreatureHeavyShare(TEXT("Looter.Gunfeel.Sound.CreatureHeavyShare"), Def::CreatureHeavyShare,
		TEXT("Hits within the blast window that together take this share of a creature's health thump too (0 = never)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCreatureBlastWindow(TEXT("Looter.Gunfeel.Sound.CreatureBlastWindow"), Def::CreatureBlastWindow,
		TEXT("The window (s) a blast's pellets add up in."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCreatureCrit(TEXT("Looter.Gunfeel.Sound.CreatureCrit"), Def::CreatureCrit,
		TEXT("A critical hit's crack on the body (shell, gel or coal) at this volume."), ECVF_Default);

	// Gun identity (GUNS.md): each kind's layer balance and its body punch. Seven small sets, so a kind is tuned in one line.
	struct FKindMixVars
	{
		FKindMixVars(const TCHAR* MechName, const TCHAR* LowName, const TCHAR* TailName, const GunfeelSound::FKindMix& Start)
			: Mech(MechName, Start.Mech, TEXT("This kind's action (Mech) layer times this, on top of the maker's and the global one."), ECVF_Default)
			, Low(LowName, Start.Low, TEXT("This kind's chest thump (Low) layer times this."), ECVF_Default)
			, Tail(TailName, Start.Tail, TEXT("This kind's tail (the land answering) times this."), ECVF_Default)
		{
		}
		TAutoConsoleVariable<float> Mech;
		TAutoConsoleVariable<float> Low;
		TAutoConsoleVariable<float> Tail;
	};

	struct FPunchVars
	{
		FPunchVars(const TCHAR* EveryName, const TCHAR* GainName, int32 Every, float Gain)
			: PunchEvery(EveryName, Every, TEXT("The body punch plays on every Nth shot of a burst (0 = never, 1 = every shot)."), ECVF_Default)
			, PunchGain(GainName, Gain, TEXT("This kind's body punch at this volume."), ECVF_Default)
		{
		}
		TAutoConsoleVariable<int32> PunchEvery;
		TAutoConsoleVariable<float> PunchGain;
	};

	FKindMixVars GMixRifle(TEXT("Looter.Gunfeel.Sound.Rifle.Mech"), TEXT("Looter.Gunfeel.Sound.Rifle.Low"), TEXT("Looter.Gunfeel.Sound.Rifle.Tail"), Def::KindMix(EWeaponKind::Rifle));
	FKindMixVars GMixShotgun(TEXT("Looter.Gunfeel.Sound.Shotgun.Mech"), TEXT("Looter.Gunfeel.Sound.Shotgun.Low"), TEXT("Looter.Gunfeel.Sound.Shotgun.Tail"), Def::KindMix(EWeaponKind::Shotgun));
	FKindMixVars GMixRevolver(TEXT("Looter.Gunfeel.Sound.Revolver.Mech"), TEXT("Looter.Gunfeel.Sound.Revolver.Low"), TEXT("Looter.Gunfeel.Sound.Revolver.Tail"), Def::KindMix(EWeaponKind::Revolver));
	FKindMixVars GMixSMG(TEXT("Looter.Gunfeel.Sound.SMG.Mech"), TEXT("Looter.Gunfeel.Sound.SMG.Low"), TEXT("Looter.Gunfeel.Sound.SMG.Tail"), Def::KindMix(EWeaponKind::SMG));
	FKindMixVars GMixSniper(TEXT("Looter.Gunfeel.Sound.Sniper.Mech"), TEXT("Looter.Gunfeel.Sound.Sniper.Low"), TEXT("Looter.Gunfeel.Sound.Sniper.Tail"), Def::KindMix(EWeaponKind::Sniper));
	FKindMixVars GMixLMG(TEXT("Looter.Gunfeel.Sound.LMG.Mech"), TEXT("Looter.Gunfeel.Sound.LMG.Low"), TEXT("Looter.Gunfeel.Sound.LMG.Tail"), Def::KindMix(EWeaponKind::LMG));
	FKindMixVars GMixLauncher(TEXT("Looter.Gunfeel.Sound.Launcher.Mech"), TEXT("Looter.Gunfeel.Sound.Launcher.Low"), TEXT("Looter.Gunfeel.Sound.Launcher.Tail"), Def::KindMix(EWeaponKind::Launcher));

	FPunchVars GPunchRifle(TEXT("Looter.Gunfeel.Sound.PunchEvery.Rifle"), TEXT("Looter.Gunfeel.Sound.PunchGain.Rifle"), Def::PunchEvery(EWeaponKind::Rifle), Def::PunchGain(EWeaponKind::Rifle));
	FPunchVars GPunchShotgun(TEXT("Looter.Gunfeel.Sound.PunchEvery.Shotgun"), TEXT("Looter.Gunfeel.Sound.PunchGain.Shotgun"), Def::PunchEvery(EWeaponKind::Shotgun), Def::PunchGain(EWeaponKind::Shotgun));
	FPunchVars GPunchRevolver(TEXT("Looter.Gunfeel.Sound.PunchEvery.Revolver"), TEXT("Looter.Gunfeel.Sound.PunchGain.Revolver"), Def::PunchEvery(EWeaponKind::Revolver), Def::PunchGain(EWeaponKind::Revolver));
	FPunchVars GPunchSMG(TEXT("Looter.Gunfeel.Sound.PunchEvery.SMG"), TEXT("Looter.Gunfeel.Sound.PunchGain.SMG"), Def::PunchEvery(EWeaponKind::SMG), Def::PunchGain(EWeaponKind::SMG));
	FPunchVars GPunchSniper(TEXT("Looter.Gunfeel.Sound.PunchEvery.Sniper"), TEXT("Looter.Gunfeel.Sound.PunchGain.Sniper"), Def::PunchEvery(EWeaponKind::Sniper), Def::PunchGain(EWeaponKind::Sniper));
	FPunchVars GPunchLMG(TEXT("Looter.Gunfeel.Sound.PunchEvery.LMG"), TEXT("Looter.Gunfeel.Sound.PunchGain.LMG"), Def::PunchEvery(EWeaponKind::LMG), Def::PunchGain(EWeaponKind::LMG));
	FPunchVars GPunchLauncher(TEXT("Looter.Gunfeel.Sound.PunchEvery.Launcher"), TEXT("Looter.Gunfeel.Sound.PunchGain.Launcher"), Def::PunchEvery(EWeaponKind::Launcher), Def::PunchGain(EWeaponKind::Launcher));
	TAutoConsoleVariable<float> CVarPunch(TEXT("Looter.Gunfeel.Sound.Punch"), Def::Punch,
		TEXT("The body punch under a shot (the shooter's own gun, 2D) at this volume, all kinds (0 = none)."), ECVF_Default);

	FKindMixVars& MixOf(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return GMixShotgun;
		case EWeaponKind::Revolver: return GMixRevolver;
		case EWeaponKind::SMG: return GMixSMG;
		case EWeaponKind::Sniper: return GMixSniper;
		case EWeaponKind::LMG: return GMixLMG;
		case EWeaponKind::Launcher: return GMixLauncher;
		default: return GMixRifle;
		}
	}

	FPunchVars& PunchOf(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return GPunchShotgun;
		case EWeaponKind::Revolver: return GPunchRevolver;
		case EWeaponKind::SMG: return GPunchSMG;
		case EWeaponKind::Sniper: return GPunchSniper;
		case EWeaponKind::LMG: return GPunchLMG;
		case EWeaponKind::Launcher: return GPunchLauncher;
		default: return GPunchRifle;
		}
	}

	/** A volume or time: never negative. */
	float Positive(const TAutoConsoleVariable<float>& Variable)
	{
		return FMath::Max(0.f, Variable.GetValueOnGameThread());
	}
}

float GunfeelSound::TailVolume()
{
	return Positive(CVarTail);
}

float GunfeelSound::MechVolume()
{
	return Positive(CVarMech);
}

float GunfeelSound::LowVolume()
{
	return Positive(CVarLow);
}

float GunfeelSound::DistantFrom()
{
	return Positive(CVarDistantFrom);
}

float GunfeelSound::ProbeRange()
{
	return Positive(CVarProbeRange);
}

int32 GunfeelSound::AutoTailEvery(EWeaponKind Kind)
{
	int32 Every = 1;
	switch (Kind)
	{
	case EWeaponKind::Rifle: Every = CVarTailEveryRifle.GetValueOnGameThread(); break;
	case EWeaponKind::SMG: Every = CVarTailEverySMG.GetValueOnGameThread(); break;
	case EWeaponKind::LMG: Every = CVarTailEveryLMG.GetValueOnGameThread(); break;
	default: break;
	}
	return FMath::Max(Every, 1);
}

float GunfeelSound::BurstEndTailVolume()
{
	return Positive(CVarBurstEndTail);
}

float GunfeelSound::BurstGap()
{
	return Positive(CVarBurstGap);
}

int32 GunfeelSound::LowAmmoRounds(EWeaponKind Kind)
{
	int32 Rounds = 0;
	switch (Kind)
	{
	case EWeaponKind::Rifle: Rounds = CVarLowRifle.GetValueOnGameThread(); break;
	case EWeaponKind::SMG: Rounds = CVarLowSMG.GetValueOnGameThread(); break;
	case EWeaponKind::LMG: Rounds = CVarLowLMG.GetValueOnGameThread(); break;
	case EWeaponKind::Shotgun: Rounds = CVarLowShotgun.GetValueOnGameThread(); break;
	case EWeaponKind::Revolver: Rounds = CVarLowRevolver.GetValueOnGameThread(); break;
	case EWeaponKind::Sniper: Rounds = CVarLowSniper.GetValueOnGameThread(); break;
	case EWeaponKind::Launcher: Rounds = CVarLowLauncher.GetValueOnGameThread(); break;
	default: break;
	}
	return FMath::Max(Rounds, 0);
}

float GunfeelSound::LowAmmoPitchTop()
{
	// The pitch top stays GUNFEEL-KICK's variable (Looter.Gunfeel.LowAmmo.PitchTop), so the one slider keeps working.
	const IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.LowAmmo.PitchTop"));
	return Variable ? Variable->GetFloat() : Def::LowAmmoPitchTop;
}

float GunfeelSound::LastRoundVolume()
{
	return Positive(CVarLastRound);
}

float GunfeelSound::LastRoundDelay()
{
	return Positive(CVarLastRoundDelay);
}

float GunfeelSound::HissFrom()
{
	return FMath::Clamp(CVarHissFrom.GetValueOnGameThread(), 0.f, 1.f);
}

float GunfeelSound::HissVolume()
{
	return Positive(CVarHiss);
}

float GunfeelSound::RackVolume()
{
	return Positive(CVarRack);
}

GunfeelSound::FKindMix GunfeelSound::KindMix(EWeaponKind Kind)
{
	// Identity 0: every kind balances its layers as before.
	if (!GunfeelKick::IdentityOn())
	{
		return FKindMix();
	}
	FKindMixVars& Vars = MixOf(Kind);
	FKindMix Mix;
	Mix.Mech = Positive(Vars.Mech);
	Mix.Low = Positive(Vars.Low);
	Mix.Tail = Positive(Vars.Tail);
	return Mix;
}

float GunfeelSound::PunchVolume(EWeaponKind Kind)
{
	return GunfeelKick::IdentityOn() ? Positive(CVarPunch) * Positive(PunchOf(Kind).PunchGain) : 0.f;
}

int32 GunfeelSound::PunchEvery(EWeaponKind Kind)
{
	return FMath::Max(PunchOf(Kind).PunchEvery.GetValueOnGameThread(), 0);
}

bool GunfeelSound::PunchPlays(int32 ShotInBurst, int32 Every)
{
	return Every > 0 && ShotInBurst >= 0 && ShotInBurst % Every == 0;
}

bool GunfeelSound::PunchAudible(bool bOwnGun, float Volume)
{
	return bOwnGun && Volume > 0.f;
}

float GunfeelSound::ImpactVolume(float MarkScale, bool bBlast)
{
	const float Loudness = ImpactLoudness(MarkScale, CVarImpactBase.GetValueOnGameThread(), CVarImpactSlope.GetValueOnGameThread(),
		CVarImpactMin.GetValueOnGameThread(), CVarImpactMax.GetValueOnGameThread());
	return Loudness * (bBlast ? Positive(CVarScatterLouder) : 1.f);
}

float GunfeelSound::ImpactHeavyFrom()
{
	return Positive(CVarImpactHeavyFrom);
}

float GunfeelSound::ImpactHeavyVolume()
{
	return Positive(CVarImpactHeavy);
}

float GunfeelSound::ImpactScatterVolume()
{
	return Positive(CVarImpactScatter);
}

float GunfeelSound::CreatureHeavyFrom()
{
	return Positive(CVarCreatureHeavyFrom);
}

float GunfeelSound::CreatureHeavyVolume()
{
	return Positive(CVarCreatureHeavy);
}

float GunfeelSound::CreatureHeavyShare()
{
	return Positive(CVarCreatureHeavyShare);
}

float GunfeelSound::CreatureBlastWindow()
{
	return Positive(CVarCreatureBlastWindow);
}

float GunfeelSound::CreatureCritVolume()
{
	return Positive(CVarCreatureCrit);
}

GunfeelSound::ESurroundings GunfeelSound::Classify(bool bRoof, int32 WallSides)
{
	const bool bWalled = WallSides >= 2;
	if (bRoof && bWalled)
	{
		return ESurroundings::Inside;
	}
	return (bRoof || bWalled) ? ESurroundings::Town : ESurroundings::Open;
}

EGunshotLayer GunfeelSound::TailFor(ESurroundings Surroundings)
{
	switch (Surroundings)
	{
	case ESurroundings::Inside: return EGunshotLayer::TailInside;
	case ESurroundings::Town: return EGunshotLayer::TailTown;
	default: return EGunshotLayer::TailOpen;
	}
}

GunfeelSound::FShotLayers GunfeelSound::Choose(bool bOwnGun, float DistanceToListener, float DistantFromCm,
	ESurroundings Surroundings)
{
	FShotLayers Layers;
	Layers.Tail = TailFor(Surroundings);
	// The shooter's own gun is never far from its listener, whatever the camera does.
	if (!bOwnGun && DistanceToListener > DistantFromCm)
	{
		Layers.bDistant = true;
		return Layers;
	}
	Layers.bCore = true;
	Layers.bTail = true;
	Layers.bMech = bOwnGun;
	Layers.bLow = bOwnGun;
	return Layers;
}

bool GunfeelSound::ProbeStale(double Now, double LastProbe, double MaxAge)
{
	return LastProbe < 0.0 || Now - LastProbe >= MaxAge || Now < LastProbe;
}

bool GunfeelSound::NewBurst(double Now, double LastShot, double Gap)
{
	return LastShot < 0.0 || Now - LastShot > Gap || Now < LastShot;
}

bool GunfeelSound::TailPlays(int32 ShotInBurst, int32 Every)
{
	// The first shot of a burst always has its tail: a single shot, and the start of a burst, sound whole.
	return Every <= 1 || ShotInBurst <= 0 || ShotInBurst % Every == 0;
}

bool GunfeelSound::BurstEndTailPlays(int32 BurstShots, int32 ShotsSinceTail, float Volume)
{
	return Volume > 0.f && BurstShots >= 2 && ShotsSinceTail > 0;
}

int32 GunfeelSound::LowAmmoWindow(int32 Rounds, int32 Magazine)
{
	// Under 4 rounds the whole magazine would click (the launcher's 1-3, a short rack); half is the most any gun clicks.
	if (Rounds <= 0 || Magazine < 4)
	{
		return 0;
	}
	return FMath::Min(Rounds, FMath::Max(Magazine / 2, 1));
}

float GunfeelSound::LowAmmoPitch(int32 RoundsLeft, int32 Window, float Top)
{
	if (Window <= 0 || RoundsLeft <= 0 || RoundsLeft > Window)
	{
		return 0.f;
	}
	// The first click of the run is at pitch 1, the last round's at Top; a run of one is the last round's.
	const float Run = Window > 1 ? static_cast<float>(Window - RoundsLeft) / static_cast<float>(Window - 1) : 1.f;
	return FMath::Lerp(1.f, Top, Run);
}

bool GunfeelSound::LastRoundPlays(EWeaponKind Kind, int32 RoundsLeft, int32 Magazine, float Volume)
{
	// The launcher's one to three rounds are the reload's business (its rocket slides), and it has no cue.
	const bool bHasCue = Kind != EWeaponKind::None && Kind != EWeaponKind::Launcher;
	return bHasCue && Volume > 0.f && RoundsLeft == 0 && Magazine >= 2;
}

bool GunfeelSound::HeatHissPlays(EWeaponKind Kind, float Heat, float From, int32 BurstShots, float Volume)
{
	return Kind == EWeaponKind::LMG && Volume > 0.f && BurstShots >= 2 && Heat > From;
}

float GunfeelSound::HeatHissLoudness(float Heat, float From)
{
	if (From >= 1.f)
	{
		return 1.f;
	}
	return FMath::Lerp(0.5f, 1.f, FMath::Clamp((Heat - From) / (1.f - From), 0.f, 1.f));
}

bool GunfeelSound::RackAudible(bool bOwnGun, float DistanceToListener, float DistantFromCm)
{
	return bOwnGun || DistanceToListener <= DistantFromCm;
}

float GunfeelSound::ImpactLoudness(float MarkScale, float Base, float Slope, float Min, float Max)
{
	return FMath::Clamp(Base + Slope * MarkScale, Min, FMath::Max(Min, Max));
}

bool GunfeelSound::IsHeavy(float MarkScale, float From)
{
	return From > 0.f && MarkScale >= From;
}

bool GunfeelSound::BlastIsHeavy(FBlastTally& Tally, double Now, float Share, float Window, float From)
{
	if (Tally.Start < 0.0 || Now - Tally.Start > Window || Now < Tally.Start)
	{
		Tally = FBlastTally();
		Tally.Start = Now;
	}
	Tally.Share += FMath::Max(Share, 0.f);
	if (From > 0.f && !Tally.bPlayed && Tally.Share >= From)
	{
		Tally.bPlayed = true;
		return true;
	}
	return false;
}

FName GunfeelSound::CritCueFor(ECreatureBlood Blood)
{
	switch (Blood)
	{
	case ECreatureBlood::Ichor:
	case ECreatureBlood::DarkIchor:
		return LooterSoundCue::Gunfeel::CritShell;
	case ECreatureBlood::Gel:
	case ECreatureBlood::Clay:
		return LooterSoundCue::Gunfeel::CritGel;
	case ECreatureBlood::Embers:
	case ECreatureBlood::MinerEmbers:
		return LooterSoundCue::Gunfeel::CritCoal;
	default:
		return NAME_None;
	}
}

FName GunfeelSound::HitConfirmCue(bool bCritical, bool bWardHit)
{
	if (bWardHit)
	{
		return LooterSoundCue::Gunfeel::HitMarkerWard;
	}
	return bCritical ? FName(LooterSoundCue::Gunfeel::HitMarkerCrit) : FName(LooterSoundCue::HitMarker);
}

FName GunfeelSound::KillConfirmCue(bool bCriticalKill, bool bBankHasCritKill)
{
	return bCriticalKill && bBankHasCritKill ? FName(LooterSoundCue::Gunfeel::KillCrit) : FName(LooterSoundCue::Kill);
}
