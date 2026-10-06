#include "Story/StoryLine.h"

float FStoryLine::GetSeconds() const
{
	if (Seconds > 0.f)
	{
		return Seconds;
	}
	// Written lines vary a lot in length, so the time follows the words rather than one fixed time for every line.
	const float Characters = static_cast<float>(Text.ToString().Len());
	return FMath::Max(MinSeconds, LeadSeconds + SecondsPerCharacter * Characters);
}

FStoryLine FStoryLine::Make(const FText& InSpeaker, const FText& InText, float InSeconds)
{
	FStoryLine Line;
	Line.Speaker = InSpeaker;
	Line.Text = InText;
	Line.Seconds = InSeconds;
	return Line;
}
