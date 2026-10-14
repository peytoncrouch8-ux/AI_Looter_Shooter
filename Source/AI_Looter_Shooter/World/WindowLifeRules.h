#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"
#include "Math/RandomStream.h"
#include "UObject/ObjectKey.h"

/** What a lived-in house's window shows for a moment. */
enum class EWindowLifeKind : uint8
{
	None,
	/** A shadow crosses behind the lamplit glass (someone walking through the room), or stops there a moment. Dusk only. */
	Shadow,
	/** A curtain twitches: pulled in from one side, let go, settling back. */
	Curtain,
	/** A shutter left open swings shut (AWindowShutter::Slam). */
	Shutter,
};

/** One window event as it plays (WindowLifeRules::MakeEvent rolls it). */
struct AI_LOOTER_SHOOTER_API FWindowLifeEvent
{
	EWindowLifeKind Kind = EWindowLifeKind::None;
	/** How long it runs (s). */
	float Seconds = 0.f;
	/** The side it comes from across the window (+1 or -1). */
	float Side = 1.f;
	/** A shadow that stops at the window a moment and goes back the way it came, instead of walking through. */
	bool bLingers = false;
	/** A curtain: how far it's pulled in (a share of its travel). */
	float Reach = 1.f;
	/** The shadow's or the curtain's colour (unlit, linear) and how solid it is at most. */
	FLinearColor Color = FLinearColor::Black;
	float Density = 1.f;
};

/** How the window's card shows an event at a moment: what the material (M_WindowLife) is given. */
struct AI_LOOTER_SHOOTER_API FWindowLifePose
{
	/** How solid it is (0: nothing shows). */
	float Strength = 0.f;
	/** A shadow's middle across the window, or a curtain's edge (cm from the pane's middle). */
	float Slide = 0.f;
	/** A shadow's rise (a walker's bob, cm). */
	float Rise = 0.f;
	/** A curtain's swing (0 at rest, 1 pulled fully in): its edge bows toward the middle. */
	float Swing = 0.f;
};

/**
 * Somebody's home, never seen (Docs/Areas/RansomsRest.md: the townsfolk hide from the walking corpse): at a lamplit
 * window of a lived-in house, now and then as the player looks over at it from across the street, a shadow crosses
 * behind the glass, a curtain twitches, or a shutter left open swings shut. Plain rules (UWindowLifeSubsystem plays them;
 * the tests run them without a world).
 *
 *  - Seen: the player within NearRadius-FarRadius of the window, looking within ViewCosine of it, the window facing them.
 *  - Rare: a house at most every HouseRestMin-HouseRestMax seconds, any window GapMin-GapMax after the last event ends,
 *    one event at a time, and even then only a Chance on each look.
 *  - By day only curtains move (a shadow can't show on glass the day lights from outside) and shutters left open swing
 *    shut (bShuttersByDay: they're only ever open by day, before Main 3); at dusk, shadows too.
 */
namespace WindowLifeRules
{
	/** Seconds between looks. */
	inline constexpr float LookSeconds = 0.5f;

	/** Nearer than this (cm) it's too close to be someone glimpsed inside; farther, too far to see. */
	inline constexpr float NearRadius = 600.f;
	inline constexpr float FarRadius = 2500.f;

	/** The window within this of where the player looks (the cosine of about 40 degrees). */
	inline constexpr float ViewCosine = 0.77f;

	/** The window faces the player at least this much (not seen edge-on, nor from behind the house). */
	inline constexpr float FacingCosine = 0.25f;

	/** A house rests this long after an event (s); any window this long after the last event ends. */
	inline constexpr float HouseRestMin = 30.f;
	inline constexpr float HouseRestMax = 90.f;
	inline constexpr float GapMin = 10.f;
	inline constexpr float GapMax = 22.f;

	/** The chance on each look while a window is seen and may play. */
	inline constexpr float Chance = 0.12f;

	/** Shutters left open swing shut by day too (they're open only by day, before Main 3; see the namespace's note). */
	inline constexpr bool bShuttersByDay = true;

	/** How likely each kind is, among those that may play. */
	inline constexpr float ShadowWeight = 0.45f;
	inline constexpr float CurtainWeight = 0.35f;
	inline constexpr float ShutterWeight = 0.2f;

	/** A shadow: how long a walk past lasts, and a stop at the window (s); how often it stops rather than walks through. */
	inline constexpr float ShadowSecondsMin = 2.6f;
	inline constexpr float ShadowSecondsMax = 3.8f;
	inline constexpr float LingerSecondsMin = 4.f;
	inline constexpr float LingerSecondsMax = 6.f;
	inline constexpr float LingerChance = 0.35f;

	/**
	 * Where a shadow starts and ends across the window (out of sight behind the wall past a pane's edge, about 42 cm from
	 * its middle for the homes' sashes), and where one that stops stands (cm from the middle).
	 */
	inline constexpr float SlideFar = 78.f;
	inline constexpr float SlideNear = 10.f;

	/** A curtain twitch's length (s); its edge at rest (hidden past the pane) and how far a full pull brings it in (cm). */
	inline constexpr float CurtainSecondsMin = 1.8f;
	inline constexpr float CurtainSecondsMax = 2.6f;
	inline constexpr float CurtainRest = 46.f;
	inline constexpr float CurtainTravel = 34.f;

	/** A shutter's slam as a window event (s, its own swing and bang included), after a moment's wait of up to ShutterDelay. */
	inline constexpr float ShutterSeconds = 0.8f;
	inline constexpr float ShutterDelay = 0.2f;

	/** The shadow's colour (near black, a little warm) and how solid it is at most. */
	AI_LOOTER_SHOOTER_API FLinearColor ShadowColor();
	inline constexpr float ShadowDensity = 0.85f;

	/** The curtain's linen: by day as it shows in the dim room behind the glass, at dusk lit from behind by the lamp. */
	AI_LOOTER_SHOOTER_API FLinearColor CurtainColor(bool bDusk);
	inline constexpr float CurtainDensity = 0.92f;

	/** Its quiet sounds at the window (LooterSoundCue::Window), and their level. */
	inline constexpr const TCHAR* StepsCue = LooterSoundCue::Window::Steps;
	inline constexpr const TCHAR* CurtainCue = LooterSoundCue::Window::Curtain;
	inline constexpr float CueVolume = 0.7f;

	/** Whether Kind may play by day or at dusk. */
	AI_LOOTER_SHOOTER_API bool Allows(EWindowLifeKind Kind, bool bDusk);

	/** How likely Kind is to be picked among those that may play. */
	AI_LOOTER_SHOOTER_API float WeightOf(EWindowLifeKind Kind);

	/** Picks one of Kinds (each that may play now), by weight; None when none may. */
	AI_LOOTER_SHOOTER_API EWindowLifeKind PickKind(TConstArrayView<EWindowLifeKind> Kinds, bool bDusk, FRandomStream& Random);

	/**
	 * Whether the player sees a window from Eyes, looking along ViewDirection: Window (its middle) within range, near
	 * where they look, and WindowOut (the way the window faces) toward them.
	 */
	AI_LOOTER_SHOOTER_API bool Sees(const FVector& Eyes, const FVector& ViewDirection, const FVector& Window, const FVector& WindowOut);

	/** An event of Kind, rolled: its length, side, whether a shadow stops, a curtain's pull, its colour. */
	AI_LOOTER_SHOOTER_API FWindowLifeEvent MakeEvent(EWindowLifeKind Kind, bool bDusk, FRandomStream& Random);

	/** What Event shows Since seconds into it. */
	AI_LOOTER_SHOOTER_API FWindowLifePose PoseAt(const FWindowLifeEvent& Event, float Since);

	/** The sound Kind makes at the window as it starts (None: none of its own; a shutter bangs by itself). */
	AI_LOOTER_SHOOTER_API FName CueFor(EWindowLifeKind Kind);

	AI_LOOTER_SHOOTER_API const TCHAR* KindName(EWindowLifeKind Kind);
}

/** The town's windows' clock: when the next event may come, at all and at each house, and whether one is under way. */
struct AI_LOOTER_SHOOTER_API FWindowLifeClock
{
	/** Whether an event may start at House now: none under way, the gap after the last over, the house rested. */
	bool MayPlay(FObjectKey House, double Now) const;

	/** Whether an event is still under way. */
	bool IsBusy(double Now) const { return Now < BusyUntil; }

	/** Whether any event may start now: none under way and the gap after the last over (each house's rest aside). */
	bool IsQuiet(double Now) const { return Now >= BusyUntil && Now >= NextAny; }

	/** An event of Seconds started at House at Now: busy till it ends, then the gap; the house rests. */
	void Played(FObjectKey House, double Now, float Seconds, FRandomStream& Random);

	double BusyUntil = 0.0;
	double NextAny = 0.0;
	TMap<FObjectKey, double> HouseNext;
};
