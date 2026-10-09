#include "Tutorial/TutorialChoice.h"

#define LOCTEXT_NAMESPACE "TutorialChoice"

namespace TutorialChoice
{
	FText StartLabel(const FText& PracticeName)
	{
		return FText::Format(LOCTEXT("Start", "Start on {0} (learn the basics)"), PracticeName);
	}

	FText SkipLabel(const FText& FirstName)
	{
		return FText::Format(LOCTEXT("Skip", "Skip to {0}"), FirstName);
	}

	FText Explain(const FText& PracticeName, const FText& FirstName)
	{
		// No promise of a lesson: Skyreach is a place to get your bearings at your own pace, with its board's odd jobs.
		return FText::Format(LOCTEXT("Explain",
			"{0} is a practice island: find a gun, take a few jobs off the town's notice board, and pick up the controls as you go. "
			"Skip to {1} to start the story at once, with a Common Bullpup. You can sail back to {0} to practice any time."),
			PracticeName, FirstName);
	}

	FText SkipClosed(const FText& FirstName)
	{
		return FText::Format(LOCTEXT("SkipClosed", "{0} isn't in the game yet, so you start on the practice island."), FirstName);
	}
}

#undef LOCTEXT_NAMESPACE
