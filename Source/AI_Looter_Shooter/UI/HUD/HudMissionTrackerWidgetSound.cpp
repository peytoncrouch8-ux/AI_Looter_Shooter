// UHudMissionTrackerWidget's sounds: a done objective's tick is heard as it shows.

#include "UI/HUD/HudMissionTrackerWidget.h"
#include "Audio/LooterSound.h"

void UHudMissionTrackerWidget::PlayFinishSound(bool bStepDone, bool bMissionEnded) const
{
	// The mission is over: the fuller fanfare, unless the mission-complete banner sounds it with its rewards (a story
	// mission's turn-in, or its end), which would only double it. Every step is the same chime, softer for an objective
	// within a step; the last objectives done before a turn-in are a step's chime too, as the mission isn't over yet.
	if (bMissionEnded)
	{
		if (!Shown.bAnnouncedEnd)
		{
			LooterSound::Play2D(this, LooterSoundCue::MissionComplete);
		}
		return;
	}
	LooterSound::Play2D(this, LooterSoundCue::MissionStep, bStepDone ? 1.f : 0.7f);
}
