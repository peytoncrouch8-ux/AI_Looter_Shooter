#pragma once

#include "CoreMinimal.h"
#include "StoryLine.generated.h"

/**
 * One line of the story said aloud: who says it, the words, and how long they stay on screen. Lines are data (a speaker
 * point's, a line set's) and play in order as captions (UCaptionSubsystem).
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FStoryLine
{
	GENERATED_BODY()

	/** A line with no seconds set stays at least this long... */
	static constexpr float MinSeconds = 2.f;
	/** ...or a moment to look down to it, then this long for each character: an unhurried reader's pace. */
	static constexpr float LeadSeconds = 1.f;
	static constexpr float SecondsPerCharacter = 0.06f;

	/**
	 * Who says it, as the caption names them ("Grandma Delia"). Empty: a speaker point fills in its own speaker; played
	 * any other way, the words show alone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line")
	FText Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line", meta = (MultiLine = true))
	FText Text;

	/** Seconds on screen, its fades included. 0: long enough to read (GetSeconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Line", meta = (ClampMin = "0"))
	float Seconds = 0.f;

	/** How long it stays on screen: Seconds when set, else a reading time for its words. */
	float GetSeconds() const;

	static FStoryLine Make(const FText& InSpeaker, const FText& InText, float InSeconds = 0.f);
};
