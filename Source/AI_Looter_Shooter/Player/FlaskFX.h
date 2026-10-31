#pragma once

#include "CoreMinimal.h"

class APawn;

/**
 * The soul flask's sounds and light (Docs/Work/2026-10-30-SoulFlask/CONTRACT.md; UPlayerFlaskComponent calls these). Sounds by
 * cue (FlaskCue, Player/PlayerFlaskCues.h): 2D for the local player, at the body for anyone else. The light is code-drawn on
 * the bullets' effects (FWeaponFX's glows and streaks, M_FX_Glow): soul-motes' green (LooterUI::Color::Heal), no particle
 * assets, no lights, a few dozen quads for under a second, so it costs next to nothing. Each look that lasts more than a
 * frame is a short run stepped once a frame on the world's timer (FlaskFX.cpp), following the flask in the hand as it moves.
 *
 * Where the flask is drawn: its model is found on the drinker (a visible static mesh whose mesh is SM_SoulFlask* or whose
 * component is named *Flask*); a first-person one is drawn at the view model's field of view, so every point taken from it
 * is moved to where it shows (FMinimalViewInfo::TransformWorldToFirstPerson, as AWeaponBase::GetVisibleLocation does for a
 * muzzle). Without a model (a test world, the stand-in before the import) the light starts at the Mouth given.
 */
namespace FlaskFX
{
	/** The press: the cork's pop and squeak; the flask's light brightens (sound at the hands, 2D for the player). */
	AI_LOOTER_SHOOTER_API void Uncork(APawn& Drinker);
	/** SipAt: the gulp and the soul's rush: green light streaming from Mouth (the flask's SOCKET_Mouth in the world)
	 *  into the view / the chest, and a warm swell (FlaskCue::Drink, FlaskCue::Heal). Mouth is the socket's own world
	 *  location (GetSocketLocation), not moved for the first-person view: FlaskFX does that itself. */
	AI_LOOTER_SHOOTER_API void Drink(APawn& Drinker, const FVector& Mouth);
	/** A charge filled by soul: a clear little chime (FlaskCue::ChargeFilled), and a small flare at the flask (or the hip). */
	AI_LOOTER_SHOOTER_API void ChargeFilled(APawn& Drinker);
	/** Refused: empty (a dry glass clink, FlaskCue::Empty) or full health (a soft muted tap, FlaskCue::Full). */
	AI_LOOTER_SHOOTER_API void Denied(APawn& Drinker, bool bFullHealth);
	/** Respawn's top-up: a soft three-note fill (FlaskCue::Refilled). */
	AI_LOOTER_SHOOTER_API void RefilledAll(APawn& Drinker);

	/** The flask's model (Art/Models/Items/SoulFlask.py), and the same without its cork (to swap in at the uncork, if wanted). */
	inline constexpr const TCHAR* MeshPath = TEXT("/Game/Art/Items/SM_SoulFlask.SM_SoulFlask");
	inline constexpr const TCHAR* UncorkedMeshPath = TEXT("/Game/Art/Items/SM_SoulFlaskUncorked.SM_SoulFlaskUncorked");
	/** Its mouth: at the neck's opening on the neck's axis, +Z out. */
	inline constexpr const TCHAR* MouthSocket = TEXT("Mouth");
	/** The liquid's material (MI_SoulFlaskLiquid on M_StylizedSurface): its emissive strength and colour parameters, and the
	 *  strength it's authored with (a full flask). */
	inline constexpr const TCHAR* LiquidSlot = TEXT("SoulFlaskLiquid");
	inline constexpr const TCHAR* GlowParameter = TEXT("Glow");
	inline constexpr const TCHAR* ColorParameter = TEXT("Color");
	inline constexpr float AuthoredGlow = 2.f;
}
