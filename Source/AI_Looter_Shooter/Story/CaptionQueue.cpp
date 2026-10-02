#include "Story/CaptionQueue.h"

int32 FCaptionQueue::Enqueue(const TArray<FStoryLine>& Lines)
{
	return Add(Lines);
}

int32 FCaptionQueue::Interrupt(const TArray<FStoryLine>& Lines)
{
	Clear();
	return Add(Lines);
}

void FCaptionQueue::Clear()
{
	// What waited goes at once; the line on screen fades out quickly rather than vanishing mid-word.
	if (Entries.Num() > 1)
	{
		Entries.RemoveAt(1, Entries.Num() - 1);
	}
	CutCurrent();
	// A line that wasn't visible yet has nothing to fade: it goes now.
	Advance(0.f);
}

int32 FCaptionQueue::Add(const TArray<FStoryLine>& Lines)
{
	int32 Conversation = 0;
	for (const FStoryLine& Line : Lines)
	{
		// A line with no words would only be a pause with a name over it.
		if (Line.Text.IsEmpty())
		{
			continue;
		}
		if (Conversation == 0)
		{
			Conversation = ++LastConversation;
		}
		FCaptionEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Line = Line;
		Entry.Conversation = Conversation;
		Entry.Serial = ++LastSerial;
		Entry.Seconds = Line.GetSeconds();
	}
	return Conversation;
}

void FCaptionQueue::CutCurrent()
{
	if (Entries.IsEmpty() || Entries[0].bCut)
	{
		return;
	}
	// Fading out from however far in it had faded, at the cut's pace: a line barely shown goes almost at once.
	const float Alpha = GetAlpha();
	FCaptionEntry& Current = Entries[0];
	Current.bCut = true;
	Current.CutAt = Elapsed;
	Current.CutAlpha = Alpha;
	Current.Seconds = Elapsed + CutSeconds * Alpha;
}

void FCaptionQueue::Advance(float DeltaSeconds)
{
	Elapsed += FMath::Max(DeltaSeconds, 0.f);
	// A long frame can end several lines; the time past each end belongs to the next line, so the timing holds.
	while (!Entries.IsEmpty() && Elapsed >= Entries[0].Seconds)
	{
		Elapsed -= Entries[0].Seconds;
		Entries.RemoveAt(0);
	}
	if (Entries.IsEmpty())
	{
		Elapsed = 0.f;
	}
}

const FCaptionEntry* FCaptionQueue::GetCurrent() const
{
	return Entries.IsEmpty() ? nullptr : &Entries[0];
}

float FCaptionQueue::GetAlpha() const
{
	const FCaptionEntry* Current = GetCurrent();
	if (!Current)
	{
		return 0.f;
	}
	if (Current->bCut)
	{
		return FMath::Clamp(Current->CutAlpha - (Elapsed - Current->CutAt) / CutSeconds, 0.f, 1.f);
	}
	// In at its start, out at its end; a line shorter than both fades never reaches full.
	const float In = Elapsed / FadeInSeconds;
	const float Out = (Current->Seconds - Elapsed) / FadeOutSeconds;
	return FMath::Clamp(FMath::Min(In, Out), 0.f, 1.f);
}

bool FCaptionQueue::IsPlaying(int32 Conversation) const
{
	return Conversation != 0 && Entries.ContainsByPredicate([Conversation](const FCaptionEntry& Entry)
	{
		return Entry.Conversation == Conversation && !Entry.bCut;
	});
}
