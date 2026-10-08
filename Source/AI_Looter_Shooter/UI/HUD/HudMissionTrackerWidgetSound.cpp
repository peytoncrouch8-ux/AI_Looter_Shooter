// UHudMissionTrackerWidget's sounds: a done objective's tick is heard as it shows.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "Audio/LooterSound.h"

void UHudMissionTrackerWidget::PlayFinishSound(bool bStepDone) const
{
	// The mission is done when its last step is (the shown mission's runner has moved past it, or the tutorial's closing
	// line follows its last objective): the fuller fanfare. An objective within a step is the same chime, softer.
	const bool bMissionDone = bStepDone && Shown.StepCount > 0 && Shown.Step + 1 >= Shown.StepCount;
	if (bMissionDone)
	{
		LooterSound::Play2D(this, LooterSoundCue::MissionComplete);
	}
	else
	{
		LooterSound::Play2D(this, LooterSoundCue::MissionStep, bStepDone ? 1.f : 0.7f);
	}
}
