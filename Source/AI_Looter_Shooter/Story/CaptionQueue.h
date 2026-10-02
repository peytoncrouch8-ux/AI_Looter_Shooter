#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

/** A line in the caption queue: the line, the conversation it came with, and how long it stays on screen. */
struct AI_LOOTER_SHOOTER_API FCaptionEntry
{
	FStoryLine Line;

	/** Lines played together (a speaker point's talk) share one; never 0. */
	int32 Conversation = 0;

	/** Every line queued takes the next number, so the caption widget can tell a new line from the same words said again. */
	int32 Serial = 0;

	/** Seconds on screen: the line's own, or once it's cut short, until its quick fade ends. */
	float Seconds = 0.f;

	/** Cut short by a conversation that interrupted: it fades out from CutAlpha, starting CutAt seconds in. */
	bool bCut = false;
	float CutAt = 0.f;
	float CutAlpha = 1.f;
};

/**
 * The captions' rules apart from the world, so the tests drive its clock. Lines show one at a time in the order they came,
 * each for its seconds, fading in at its start and out at its end, the next one right after. Lines played together are a
 * conversation: one that queues waits its turn; one that interrupts cuts the line on screen short (it fades out quickly)
 * and drops whatever was waiting.
 */
struct AI_LOOTER_SHOOTER_API FCaptionQueue
{
	static constexpr float FadeInSeconds = 0.25f;
	static constexpr float FadeOutSeconds = 0.4f;
	/** A line cut short fades out at this pace: this many seconds from fully shown. */
	static constexpr float CutSeconds = 0.2f;

	/** Lines after everything queued. Returns their conversation, or 0 when none had words. */
	int32 Enqueue(const TArray<FStoryLine>& Lines);

	/** Lines now: the line on screen fades out quickly and the lines waiting are dropped. Returns their conversation. */
	int32 Interrupt(const TArray<FStoryLine>& Lines);

	/** Everything stops: the line on screen fades out quickly, and nothing waits. */
	void Clear();

	/** Moves the clock on: lines end and the next ones start in order, carrying the rest of the step, however long it is. */
	void Advance(float DeltaSeconds);

	/** The line on screen (fading in or out included), or null. */
	const FCaptionEntry* GetCurrent() const;

	/** How visible the line on screen is, 0 to 1. */
	float GetAlpha() const;

	/** Seconds into the line on screen. */
	float GetElapsed() const { return Elapsed; }

	/** Some of the conversation's lines are showing or still to come (a line cut short is over already). */
	bool IsPlaying(int32 Conversation) const;

	bool IsEmpty() const { return Entries.IsEmpty(); }

	/** Lines on screen and waiting. */
	int32 Num() const { return Entries.Num(); }

private:
	/** Queues the lines with words as one conversation. */
	int32 Add(const TArray<FStoryLine>& Lines);

	/** The line on screen starts its quick fade from where it is. */
	void CutCurrent();

	/** The line on screen first, then the ones waiting. */
	TArray<FCaptionEntry> Entries;
	float Elapsed = 0.f;
	int32 LastConversation = 0;
	int32 LastSerial = 0;
};
