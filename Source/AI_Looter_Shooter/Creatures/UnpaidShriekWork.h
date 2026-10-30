#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"

class UWorld;
class FWeaponFX;

/**
 * What the shriek's two halves share (the flow, UnpaidCreatureShriek.cpp; the look, UnpaidCreatureShriekLook.cpp). Only the
 * Unpaid's shriek files include it.
 */
namespace UnpaidShriekWork
{
	namespace Cue = LooterSoundCue::UnpaidScream;

	/** The world's effects drawer, made ready (UBulletSubsystem's); null without a world or the subsystem. */
	FWeaponFX* ShriekEffects(UWorld* World);
}
