#include "Weapons/ReloadTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Gunfeel.Reload.* and .Shotgun.Rack* variables (ReloadTuning.h) and the plain rules that read them. Each
// accessor reads its variable when asked, never caching it, so a change in play shows at the next reload.

namespace
{
	namespace Def = ReloadTuning::Default;

	TAutoConsoleVariable<float> CVarTacticalRifle(TEXT("Looter.Gunfeel.Reload.Tactical.Rifle"), Def::TacticalRifle,
		TEXT("The rifle's reload time scale with rounds still in (no charging handle)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEmptyRifle(TEXT("Looter.Gunfeel.Reload.Empty.Rifle"), Def::EmptyRifle,
		TEXT("...and from empty (the charging handle slapped)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTacticalSMG(TEXT("Looter.Gunfeel.Reload.Tactical.SMG"), Def::TacticalSMG,
		TEXT("The SMG's reload time scale with rounds still in (no charge)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEmptySMG(TEXT("Looter.Gunfeel.Reload.Empty.SMG"), Def::EmptySMG,
		TEXT("...and from empty (the helix charged)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTacticalSniper(TEXT("Looter.Gunfeel.Reload.Tactical.Sniper"), Def::TacticalSniper,
		TEXT("The sniper's reload time scale with a round chambered (the box only, the bolt stays home)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEmptySniper(TEXT("Looter.Gunfeel.Reload.Empty.Sniper"), Def::EmptySniper,
		TEXT("...and from empty (the bolt opened and run home)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTacticalLMG(TEXT("Looter.Gunfeel.Reload.Tactical.LMG"), Def::TacticalLMG,
		TEXT("The LMG's reload time scale with rounds left on the belt (one rhythm: the box always)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEmptyLMG(TEXT("Looter.Gunfeel.Reload.Empty.LMG"), Def::EmptyLMG,
		TEXT("...and from empty."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTacticalRevolver(TEXT("Looter.Gunfeel.Reload.Tactical.Revolver"), Def::TacticalRevolver,
		TEXT("The revolver's reload time scale with live rounds in (all six come out either way)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEmptyRevolver(TEXT("Looter.Gunfeel.Reload.Empty.Revolver"), Def::EmptyRevolver,
		TEXT("...and from empty."), ECVF_Default);
	TAutoConsoleVariable<float> CVarTacticalLauncher(TEXT("Looter.Gunfeel.Reload.Tactical.Launcher"), Def::TacticalLauncher,
		TEXT("The launcher's reload time scale with a rocket still in."), ECVF_Default);
	TAutoConsoleVariable<float> CVarEmptyLauncher(TEXT("Looter.Gunfeel.Reload.Empty.Launcher"), Def::EmptyLauncher,
		TEXT("...and from empty."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarCommitAtSeat(TEXT("Looter.Gunfeel.Reload.CommitAtSeat"), Def::CommitAtSeat ? 1 : 0,
		TEXT("1: the rounds go in as the magazine seats, so a swap after that keeps them. 0: only once the reload is done."), ECVF_Default);
	TAutoConsoleVariable<float> CVarReadyEarly(TEXT("Looter.Gunfeel.Reload.ReadyEarly"), Def::ReadyEarly,
		TEXT("Seconds before a reload's motion ends that the gun can fire again."), ECVF_Default);

	TAutoConsoleVariable<float> CVarShotgunStart(TEXT("Looter.Gunfeel.Reload.Shotgun.Start"), Def::ShotgunStart,
		TEXT("Seconds (base) to bring the shotgun up and take the first shell from the belt."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunShell(TEXT("Looter.Gunfeel.Reload.Shotgun.Shell"), Def::ShotgunShell,
		TEXT("Seconds (base) for each shell: a round in as it seats."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunPump(TEXT("Looter.Gunfeel.Reload.Shotgun.Pump"), Def::ShotgunPump,
		TEXT("Seconds (base) for the pump at the end, from empty only."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarShotgunInterrupt(TEXT("Looter.Gunfeel.Reload.Shotgun.Interrupt"), Def::ShotgunInterrupt ? 1 : 0,
		TEXT("1: a trigger press mid-reload finishes the shell in hand (and the pump from empty), then fires."), ECVF_Default);

	TAutoConsoleVariable<float> CVarRackShare(TEXT("Looter.Gunfeel.Shotgun.RackShare"), Def::RackShare,
		TEXT("The pump's rack between shots takes this share of the time between shots..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRackMin(TEXT("Looter.Gunfeel.Shotgun.RackMin"), Def::RackMin,
		TEXT("...at least this long (s)..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRackMax(TEXT("Looter.Gunfeel.Shotgun.RackMax"), Def::RackMax,
		TEXT("...at most this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRackDelay(TEXT("Looter.Gunfeel.Shotgun.RackDelay"), Def::RackDelay,
		TEXT("Seconds after the shot before the rack starts (the kick first; sooner if the next shot would come first)."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarDebug(TEXT("Looter.Gunfeel.Reload.Debug"), 0,
		TEXT("1: log every reload's plan, times, commit, interrupt and finish (LogLooter)."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarSnap(TEXT("Looter.Gunfeel.Reload.Snap"), Def::Snap ? 1 : 0,
		TEXT("1: the reload's beats spring (a wind-up, an overshoot, a settle) and the seats and slams jolt the gun. 0: the eased curves before."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapAnticipate(TEXT("Looter.Gunfeel.Reload.Snap.Anticipate"), Def::SnapAnticipate,
		TEXT("A snapped move first goes this share of itself the other way (0.2-0.3: the hand cocked before the slap)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapOvershoot(TEXT("Looter.Gunfeel.Reload.Snap.Overshoot"), Def::SnapOvershoot,
		TEXT("...then springs this share past where it goes (0.1-0.15) before it settles."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapLead(TEXT("Looter.Gunfeel.Reload.Snap.Lead"), Def::SnapLead,
		TEXT("The wind-up's share of a snapped move's window."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapSettle(TEXT("Looter.Gunfeel.Reload.Snap.Settle"), Def::SnapSettle,
		TEXT("The settle's share of a snapped move's window (critically damped: no second swing)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapJolt(TEXT("Looter.Gunfeel.Reload.Snap.Jolt"), Def::SnapJolt,
		TEXT("The seat's and the slam's jolt on the first-person gun, times this (0 = none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapJoltSeat(TEXT("Looter.Gunfeel.Reload.Snap.JoltSeat"), Def::SnapJoltSeat,
		TEXT("cm the gun is knocked back and down as a magazine, helix, box, rocket or cylinder seats."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapJoltSlam(TEXT("Looter.Gunfeel.Reload.Snap.JoltSlam"), Def::SnapJoltSlam,
		TEXT("cm the gun is knocked back and down as a charging handle, bolt or pump slams home."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapJoltPitch(TEXT("Looter.Gunfeel.Reload.Snap.JoltPitch"), Def::SnapJoltPitch,
		TEXT("Degrees the muzzle dips per cm of a jolt."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapJoltHz(TEXT("Looter.Gunfeel.Reload.Snap.JoltHz"), Def::SnapJoltHz,
		TEXT("The jolt's spring (Hz, times the gun's recoil Snappiness): the gun recoil's own 9."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSnapJoltDamping(TEXT("Looter.Gunfeel.Reload.Snap.JoltDamping"), Def::SnapJoltDamping,
		TEXT("The jolt spring's damping ratio (under 1: it rebounds a touch; the gun recoil's own 0.55)."), ECVF_Default);

	TAutoConsoleVariable<int32> CVarDropMag(TEXT("Looter.Gunfeel.Reload.DropMag"), Def::DropMag ? 1 : 0,
		TEXT("1: a magazine, helix or box reload throws the old one out to fall, bounce and lie. 0: it goes invisible as before."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDropMagRest(TEXT("Looter.Gunfeel.Reload.DropMag.Rest"), Def::DropMagRest,
		TEXT("Seconds a thrown magazine lies before it sinks away."), ECVF_Default);

	/** The kind's pair of variables (null for the shotgun, whose time is its shells, and for None). */
	void KindVariables(EWeaponKind Kind, TAutoConsoleVariable<float>*& OutTactical, TAutoConsoleVariable<float>*& OutEmpty)
	{
		OutTactical = nullptr;
		OutEmpty = nullptr;
		switch (Kind)
		{
		case EWeaponKind::Rifle:
			OutTactical = &CVarTacticalRifle;
			OutEmpty = &CVarEmptyRifle;
			break;
		case EWeaponKind::SMG:
			OutTactical = &CVarTacticalSMG;
			OutEmpty = &CVarEmptySMG;
			break;
		case EWeaponKind::Sniper:
			OutTactical = &CVarTacticalSniper;
			OutEmpty = &CVarEmptySniper;
			break;
		case EWeaponKind::LMG:
			OutTactical = &CVarTacticalLMG;
			OutEmpty = &CVarEmptyLMG;
			break;
		case EWeaponKind::Revolver:
			OutTactical = &CVarTacticalRevolver;
			OutEmpty = &CVarEmptyRevolver;
			break;
		case EWeaponKind::Launcher:
			OutTactical = &CVarTacticalLauncher;
			OutEmpty = &CVarEmptyLauncher;
			break;
		default:
			break;
		}
	}
}

float ReloadTuning::Default::TimeScale(EWeaponKind Kind, bool bEmpty)
{
	switch (Kind)
	{
	case EWeaponKind::Rifle:
		return bEmpty ? EmptyRifle : TacticalRifle;
	case EWeaponKind::SMG:
		return bEmpty ? EmptySMG : TacticalSMG;
	case EWeaponKind::Sniper:
		return bEmpty ? EmptySniper : TacticalSniper;
	case EWeaponKind::LMG:
		return bEmpty ? EmptyLMG : TacticalLMG;
	case EWeaponKind::Revolver:
		return bEmpty ? EmptyRevolver : TacticalRevolver;
	case EWeaponKind::Launcher:
		return bEmpty ? EmptyLauncher : TacticalLauncher;
	default:
		return 1.f;
	}
}

float ReloadTuning::TimeScale(EWeaponKind Kind, bool bEmpty)
{
	TAutoConsoleVariable<float>* Tactical = nullptr;
	TAutoConsoleVariable<float>* Empty = nullptr;
	KindVariables(Kind, Tactical, Empty);
	TAutoConsoleVariable<float>* Variable = bEmpty ? Empty : Tactical;
	// Never a reload of no time (a typo of 0 would make it instant and skip every step's sound).
	return Variable ? FMath::Max(Variable->GetValueOnGameThread(), 0.05f) : 1.f;
}

bool ReloadTuning::CommitAtSeat()
{
	return CVarCommitAtSeat.GetValueOnGameThread() != 0;
}

float ReloadTuning::ReadyEarly()
{
	return FMath::Max(CVarReadyEarly.GetValueOnGameThread(), 0.f);
}

ReloadTuning::FShellTiming ReloadTuning::ShellTiming()
{
	FShellTiming Timing;
	Timing.Start = FMath::Max(CVarShotgunStart.GetValueOnGameThread(), 0.f);
	Timing.Shell = FMath::Max(CVarShotgunShell.GetValueOnGameThread(), 0.05f);
	Timing.Pump = FMath::Max(CVarShotgunPump.GetValueOnGameThread(), 0.05f);
	return Timing;
}

bool ReloadTuning::ShotgunInterrupt()
{
	return CVarShotgunInterrupt.GetValueOnGameThread() != 0;
}

ReloadTuning::FRackRules ReloadTuning::RackRules()
{
	FRackRules Rules;
	Rules.Share = FMath::Max(CVarRackShare.GetValueOnGameThread(), 0.f);
	Rules.Min = FMath::Max(CVarRackMin.GetValueOnGameThread(), 0.f);
	Rules.Max = FMath::Max(CVarRackMax.GetValueOnGameThread(), Rules.Min);
	Rules.Delay = FMath::Max(CVarRackDelay.GetValueOnGameThread(), 0.f);
	return Rules;
}

bool ReloadTuning::Debug()
{
	return CVarDebug.GetValueOnGameThread() != 0;
}

bool ReloadTuning::SnapOn()
{
	// Read on any thread: the curves are plain math the view model and the hands call each frame.
	return CVarSnap.GetValueOnAnyThread() != 0;
}

ReloadTuning::FSnapShape ReloadTuning::SnapShape()
{
	// Kept to shapes that still start and end at rest with room for the move itself (a typo can't make a beat jump).
	FSnapShape Shape;
	Shape.Anticipate = FMath::Clamp(CVarSnapAnticipate.GetValueOnAnyThread(), 0.f, 0.6f);
	Shape.Overshoot = FMath::Clamp(CVarSnapOvershoot.GetValueOnAnyThread(), 0.f, 0.5f);
	Shape.Lead = FMath::Clamp(CVarSnapLead.GetValueOnAnyThread(), 0.f, 0.4f);
	Shape.Settle = FMath::Clamp(CVarSnapSettle.GetValueOnAnyThread(), 0.05f, 0.5f);
	return Shape;
}

ReloadTuning::FJoltRules ReloadTuning::JoltRules()
{
	FJoltRules Rules;
	Rules.Scale = FMath::Max(CVarSnapJolt.GetValueOnAnyThread(), 0.f);
	Rules.Seat = FMath::Max(CVarSnapJoltSeat.GetValueOnAnyThread(), 0.f);
	Rules.Slam = FMath::Max(CVarSnapJoltSlam.GetValueOnAnyThread(), 0.f);
	Rules.Pitch = CVarSnapJoltPitch.GetValueOnAnyThread();
	Rules.Hz = FMath::Max(CVarSnapJoltHz.GetValueOnAnyThread(), 1.f);
	// Under 1 so it rings (and the closed form holds), above 0 so it dies.
	Rules.Damping = FMath::Clamp(CVarSnapJoltDamping.GetValueOnAnyThread(), 0.05f, 0.95f);
	return Rules;
}

bool ReloadTuning::DropMagOn()
{
	return CVarDropMag.GetValueOnAnyThread() != 0;
}

float ReloadTuning::DropMagRest()
{
	return FMath::Max(CVarDropMagRest.GetValueOnAnyThread(), 0.f);
}

int32 ReloadTuning::ShellsNeeded(int32 Magazine, int32 MagazineSize, int32 Reserve)
{
	return FMath::Max(FMath::Min(MagazineSize - Magazine, Reserve), 0);
}

FReloadPlan ReloadTuning::PlanFor(EWeaponReloadPart Part, int32 Magazine, int32 MagazineSize, int32 Reserve)
{
	FReloadPlan Plan;
	Plan.bEmpty = Magazine <= 0;
	Plan.Shells = Part == EWeaponReloadPart::Pump ? ShellsNeeded(Magazine, MagazineSize, Reserve) : 0;
	return Plan;
}

float ReloadTuning::GunScale(float ReloadTime, float BaseReloadTime)
{
	return ReloadTime > 0.f && BaseReloadTime > 0.f ? ReloadTime / BaseReloadTime : 1.f;
}

float ReloadTuning::ShellReloadSeconds(const FReloadPlan& Plan, const FShellTiming& Timing)
{
	return Timing.Start + FMath::Max(Plan.Shells, 0) * Timing.Shell + (Plan.bEmpty ? Timing.Pump : 0.f);
}

float ReloadTuning::ShellSeatSeconds(int32 Shell, const FShellTiming& Timing)
{
	return Timing.Start + (Shell + 0.5f) * Timing.Shell;
}

float ReloadTuning::MotionSeconds(EWeaponReloadPart Part, const FReloadPlan& Plan, float ReloadTime, float GunScaleValue, float KindScale,
	const FShellTiming& Timing)
{
	const float Seconds = Part == EWeaponReloadPart::Pump ? ShellReloadSeconds(Plan, Timing) * GunScaleValue : ReloadTime * KindScale;
	return FMath::Max(Seconds, 0.05f);
}

float ReloadTuning::ReadySeconds(float Motion, float Early)
{
	// Half at least: a short reload (one shell) is never ready before its shell is even in.
	return FMath::Max(Motion - FMath::Max(Early, 0.f), Motion * 0.5f);
}

int32 ReloadTuning::ShellsKept(const FReloadPlan& Plan, float SecondsIn, const FShellTiming& Timing)
{
	if (Plan.Shells <= 0)
	{
		return 0;
	}
	// The shell whose turn it is now; before the first turn the hand is already on its way for it.
	const int32 InHand = Timing.Shell > 0.f ? FMath::FloorToInt((SecondsIn - Timing.Start) / Timing.Shell) : 0;
	return FMath::Clamp(InHand + 1, 1, Plan.Shells);
}

ReloadTuning::FRackTiming ReloadTuning::RackTiming(float ShotInterval, const FRackRules& Rules)
{
	FRackTiming Rack;
	Rack.Seconds = FMath::Clamp(ShotInterval * Rules.Share, Rules.Min, FMath::Max(Rules.Max, Rules.Min));
	// The kick first, but home by the time the gun can fire again (a fast-firing roll starts it at once).
	Rack.Start = FMath::Clamp(FMath::Min(Rules.Delay, ShotInterval - Rack.Seconds), 0.f, Rules.Delay);
	return Rack;
}

ReloadTuning::FRackTiming ReloadTuning::RackTiming(float ShotInterval)
{
	return RackTiming(ShotInterval, RackRules());
}
