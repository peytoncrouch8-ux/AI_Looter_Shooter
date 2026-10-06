#pragma once

#include "CoreMinimal.h"

/**
 * A scene as a timeline, with nothing in Sequencer: moves, each over a span of the scene (a camera's path, the skiff's
 * drift, the white coming up), and named moments, each happening once (the ropes slipping, full white). Kept apart from
 * the world so tests can play one by hand; USceneSubsystem plays the level's, one at a time.
 *
 * Advancing applies every move whose span has begun, with how far through its span the scene is (and once more at
 * exactly its end, however long the step), then fires the moments passed, in time order. Skipping to the end applies
 * every unfinished move at its end and fires the moments still to come, in order: a skipped scene leaves the world as a
 * played one does, and its moments (travel among them) still happen.
 *
 * Nothing is added to a timeline once it plays.
 */
class AI_LOOTER_SHOOTER_API FSceneTimeline
{
public:
	/** Sets what a move moves for Alpha: how far through the move's span the scene is (0 at its start, 1 at its end). */
	using FMove = TFunction<void(float /*Alpha*/)>;

	/** What a moment does as it happens. None is fine: its name alone is the news. */
	using FAction = TFunction<void()>;

	/** A move over Duration seconds from Start. A Duration of 0 is a cut: applied once, at Alpha 1, when Start comes. */
	void AddMove(float Start, float Duration, FMove Apply);

	/** A moment called Name at At seconds. Moments at the same time happen in the order they were added. */
	void AddMoment(FName Name, float At, FAction Action = FAction());

	/** Plays on for DeltaSeconds. */
	void Advance(float DeltaSeconds);

	/** Jumps to the end: every unfinished move at its end, then every moment still to come, in order. */
	void SkipToEnd();

	/** Hears each moment's name as it happens, after its action (USceneSubsystem passes them on as OnSceneEvent). */
	TFunction<void(FName /*Moment*/)> OnMoment;

	/** Seconds played (the end, once skipped). */
	float GetTime() const { return Time; }

	/** How long it is: its last move's end or its last moment, whichever is later. */
	float GetDuration() const;

	/** Every move is at its end and every moment has happened. */
	bool IsFinished() const;

	bool HasHappened(FName Moment) const;

	/** When the moment called Moment happens; negative when there is none. */
	float GetMomentTime(FName Moment) const;

private:
	struct FMoveEntry
	{
		float Start = 0.f;
		float Duration = 0.f;
		FMove Apply;
		bool bDone = false;
	};

	struct FMomentEntry
	{
		FName Name;
		float At = 0.f;
		FAction Action;
		bool bHappened = false;
	};

	/** Applies the moves that have begun, or every unfinished one at its end (bToEnd). */
	void ApplyMoves(bool bToEnd);

	/** Fires the moments that are due, or every one still to come (bToEnd), in time order. */
	void FireMoments(bool bToEnd);

	TArray<FMoveEntry> Moves;

	/** In time order. */
	TArray<FMomentEntry> Moments;

	float Time = 0.f;
};
