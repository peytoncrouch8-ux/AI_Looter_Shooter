#pragma once

#include "CoreMinimal.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UWorld;

/**
 * The game's Niagara effects (round 5, 2026-10-27: "try Niagara effects", one effect rebuilt to compare). The systems are
 * built by script (Tools/Unreal/build_niagara_fx.py, into /Game/Art/Effects/Niagara), never by hand; the code-drawn look
 * (FWeaponFX) stays beside each one, and Looter.Fx.Niagara 1|0 picks which shows. A system that isn't built yet falls
 * back to the code look on its own, so a build before the script's run plays exactly as before.
 */
enum class ENiagaraFX : uint8
{
	/** The cinder jar's bed (AGraveSaltCinderBed): flames licking up, embers rising, smoke rolling, the ground lit by heat. Loops until deactivated. */
	CinderBed,
	/** The cinder jar's own layer on its burst (GraveSaltBurst::SpawnEffects): a fountain of embers, a fire flare, a plume, a flash of heat on the ground. One shot. */
	CinderBurst,
	Num
};

namespace NiagaraFX
{
	/**
	 * The bed's system is drawn for a bed this wide (cm; Looter.Grenade.Cinder.BedRadius's default): a bigger or smaller bed
	 * scales the system's component by its radius over this (the flames' spread follows; their sizes stay).
	 */
	constexpr float BedDesignRadius = 260.f;
	/** The burst's system is drawn for a cinder burst this wide (cm; the salt jar's 4 m x the cinder's 0.85). */
	constexpr float BurstDesignRadius = 340.f;

	/** Looter.Fx.Niagara (1, the default): the Niagara look where one is built; 0 the code-drawn look everywhere. */
	AI_LOOTER_SHOOTER_API bool IsOn();
	/** Looter.Fx.Niagara.Scale [0.5..2, 1]: every Niagara effect's component scale, times (the spread, not the sprites). */
	AI_LOOTER_SHOOTER_API float ScaleKnob();

	/** The asset path of a system (/Game/Art/Effects/Niagara/NS_...). */
	AI_LOOTER_SHOOTER_API const TCHAR* PathOf(ENiagaraFX Which);
	/** The system, loaded once it's built (looked for again while it isn't, once a call at most); null before the script's run. */
	AI_LOOTER_SHOOTER_API UNiagaraSystem* Find(ENiagaraFX Which);
	/** True when the Niagara look should draw this effect: Looter.Fx.Niagara on and its system built. */
	AI_LOOTER_SHOOTER_API bool UseFor(ENiagaraFX Which);

	/**
	 * The effect at Location, its up along Up (the ground's normal: a bed's disc lies on a slope), Scale times its design
	 * size (and Looter.Fx.Niagara.Scale). Not attached to anything: it outlives whatever spawned it, so its last smoke and
	 * embers finish after a bed is gone; it destroys itself when done. Null when UseFor is false or the world won't have it.
	 */
	AI_LOOTER_SHOOTER_API UNiagaraComponent* SpawnAt(UWorld* World, ENiagaraFX Which, const FVector& Location, const FVector& Up, float Scale);
}
