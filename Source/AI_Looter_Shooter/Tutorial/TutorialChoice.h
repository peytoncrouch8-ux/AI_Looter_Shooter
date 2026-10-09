#pragma once

#include "CoreMinimal.h"

/**
 * The main menu's choice for a new session (Docs/Polish/TutorialRework.md, "The main menu"): start on Skyreach and learn
 * the basics there, or skip straight to the story's first area (USessionSubsystem::PlaySessionSkippingTutorial). Its words
 * live with the tutorial, so the menu (UI/Menus/MainMenuSessions.cpp) and the tests say the same.
 */
namespace TutorialChoice
{
	/** "Start on Skyreach (learn the basics)": the call to action. PracticeName is the practice island's area name. */
	AI_LOOTER_SHOOTER_API FText StartLabel(const FText& PracticeName);

	/** "Skip to Ransom's Rest". FirstName is the story's first area's name. */
	AI_LOOTER_SHOOTER_API FText SkipLabel(const FText& FirstName);

	/** What each choice means, above the buttons. */
	AI_LOOTER_SHOOTER_API FText Explain(const FText& PracticeName, const FText& FirstName);

	/** Said when the story's first level isn't in the game yet, so the skip can't be taken. */
	AI_LOOTER_SHOOTER_API FText SkipClosed(const FText& FirstName);
}
