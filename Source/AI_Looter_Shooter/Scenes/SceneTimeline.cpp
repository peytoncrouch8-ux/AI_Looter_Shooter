#include "Scenes/SceneTimeline.h"

void FSceneTimeline::AddMove(float Start, float Duration, FMove Apply)
{
	FMoveEntry& Move = Moves.AddDefaulted_GetRef();
	Move.Start = FMath::Max(Start, 0.f);
	Move.Duration = FMath::Max(Duration, 0.f);
	Move.Apply = MoveTemp(Apply);
}

void FSceneTimeline::AddMoment(FName Name, float At, FAction Action)
{
	FMomentEntry Moment;
	Moment.Name = Name;
	Moment.At = FMath::Max(At, 0.f);
	Moment.Action = MoveTemp(Action);
	// After every moment at the same time or earlier, so moments at one time keep the order they were added in.
	int32 Index = Moments.Num();
	while (Index > 0 && Moments[Index - 1].At > Moment.At)
	{
		--Index;
	}
	Moments.Insert(MoveTemp(Moment), Index);
}

void FSceneTimeline::AddWait(float At, FCondition Until)
{
	FWaitEntry Wait;
	Wait.At = FMath::Max(At, 0.f);
	Wait.Until = MoveTemp(Until);
	int32 Index = Waits.Num();
	while (Index > 0 && Waits[Index - 1].At > Wait.At)
	{
		--Index;
	}
	Waits.Insert(MoveTemp(Wait), Index);
}

void FSceneTimeline::Advance(float DeltaSeconds)
{
	float Target = Time + FMath::Max(DeltaSeconds, 0.f);
	bWaiting = false;
	for (FWaitEntry& Wait : Waits)
	{
		if (Wait.bPassed)
		{
			continue;
		}
		if (Wait.At > Target)
		{
			// In time order: the rest lie further on.
			break;
		}
		if (Wait.Until && !Wait.Until())
		{
			// Held here, never behind where it already is: the moves stay where this time puts them, and the moments
			// after it wait with it.
			Target = FMath::Max(Time, Wait.At);
			bWaiting = true;
			break;
		}
		Wait.bPassed = true;
	}
	Time = Target;
	ApplyMoves(false);
	FireMoments(false);
}

void FSceneTimeline::SkipToEnd()
{
	bSkipping = true;
	bWaiting = false;
	for (FWaitEntry& Wait : Waits)
	{
		Wait.bPassed = true;
	}
	Time = FMath::Max(Time, GetDuration());
	ApplyMoves(true);
	FireMoments(true);
	bSkipping = false;
}

float FSceneTimeline::GetDuration() const
{
	float Longest = 0.f;
	for (const FMoveEntry& Move : Moves)
	{
		Longest = FMath::Max(Longest, Move.Start + Move.Duration);
	}
	for (const FMomentEntry& Moment : Moments)
	{
		Longest = FMath::Max(Longest, Moment.At);
	}
	for (const FWaitEntry& Wait : Waits)
	{
		Longest = FMath::Max(Longest, Wait.At);
	}
	return Longest;
}

bool FSceneTimeline::IsFinished() const
{
	return !Moves.ContainsByPredicate([](const FMoveEntry& Move) { return !Move.bDone; })
		&& !Moments.ContainsByPredicate([](const FMomentEntry& Moment) { return !Moment.bHappened; })
		&& !Waits.ContainsByPredicate([](const FWaitEntry& Wait) { return !Wait.bPassed; });
}

bool FSceneTimeline::HasHappened(FName Moment) const
{
	return Moments.ContainsByPredicate([Moment](const FMomentEntry& Entry) { return Entry.Name == Moment && Entry.bHappened; });
}

float FSceneTimeline::GetMomentTime(FName Moment) const
{
	const FMomentEntry* Found = Moments.FindByPredicate([Moment](const FMomentEntry& Entry) { return Entry.Name == Moment; });
	return Found ? Found->At : -1.f;
}

void FSceneTimeline::ApplyMoves(bool bToEnd)
{
	for (int32 Index = 0; Index < Moves.Num(); ++Index)
	{
		FMoveEntry& Move = Moves[Index];
		if (Move.bDone || (!bToEnd && Time < Move.Start))
		{
			continue;
		}
		const float Alpha = bToEnd || Move.Duration <= 0.f ? 1.f : FMath::Clamp((Time - Move.Start) / Move.Duration, 0.f, 1.f);
		// Marked before it runs: a move that has reached its end is never applied again, whatever its own code does.
		Move.bDone = Alpha >= 1.f;
		if (Move.Apply)
		{
			Move.Apply(Alpha);
		}
	}
}

void FSceneTimeline::FireMoments(bool bToEnd)
{
	for (int32 Index = 0; Index < Moments.Num(); ++Index)
	{
		if (Moments[Index].bHappened)
		{
			continue;
		}
		if (!bToEnd && Time < Moments[Index].At)
		{
			// In time order: the rest are still to come.
			break;
		}
		// Marked first, so a moment whose action skips the scene (and so fires the rest) never happens twice.
		Moments[Index].bHappened = true;
		const FName Happened = Moments[Index].Name;
		if (Moments[Index].Action)
		{
			Moments[Index].Action();
		}
		if (OnMoment)
		{
			OnMoment(Happened);
		}
	}
}
