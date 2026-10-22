#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"

/**
 * The ward's sounds, by cue (Art/Sounds/recipes/shields.py; all script-made). Their names live with every other cue in
 * Audio/LooterSoundCues.h (LooterSoundCue::Shield), where the sound bank's build checks them against cues.json; ShieldCue
 * is that namespace, so the ward's code reads as before.
 */
namespace ShieldCue = LooterSoundCue::Shield;
