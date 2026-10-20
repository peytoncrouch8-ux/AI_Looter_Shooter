#include "Story/CaptionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Story/HeroLines.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UCaptionSubsystem* UCaptionSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UCaptionSubsystem>() : nullptr;
}

bool UCaptionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UCaptionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Update(DeltaTime);
}

bool UCaptionSubsystem::IsTickable() const
{
	// Nothing to time while nothing is said.
	return !Queue.IsEmpty();
}

TStatId UCaptionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCaptionSubsystem, STATGROUP_Tickables);
}

int32 UCaptionSubsystem::Play(const TArray<FStoryLine>& Lines, ECaptionPlay How)
{
	// Every line is written as Ellis's: the played hero's words (Story/HeroLines), speakers too. A line left with no
	// words (Ellis's alone) is skipped by the queue.
	const TArray<FStoryLine> Said = HeroLines::ResolveLines(Lines);
	const int32 Conversation = How == ECaptionPlay::Interrupt ? Queue.Interrupt(Said) : Queue.Enqueue(Said);
	LogNewLine();
	return Conversation;
}

void UCaptionSubsystem::Clear()
{
	Queue.Clear();
}

bool UCaptionSubsystem::IsPlaying(int32 Conversation) const
{
	return Queue.IsPlaying(Conversation);
}

const FCaptionEntry* UCaptionSubsystem::GetCurrent() const
{
	return Queue.GetCurrent();
}

float UCaptionSubsystem::GetAlpha() const
{
	return Queue.GetAlpha();
}

void UCaptionSubsystem::SetHeld(bool bInHeld)
{
	bHeld = bInHeld;
}

void UCaptionSubsystem::SetSubtitlesShown(bool bShown)
{
	bSubtitlesShown = bShown;
}

const FCaptionEntry* UCaptionSubsystem::GetCurrentShown() const
{
	const FCaptionEntry* Current = Queue.GetCurrent();
	return Current && (bSubtitlesShown || !IsSubtitle(Current->Line)) ? Current : nullptr;
}

void UCaptionSubsystem::Update(float DeltaSeconds)
{
	if (bHeld)
	{
		return;
	}
	Queue.Advance(DeltaSeconds);
	LogNewLine();
}

void UCaptionSubsystem::LogNewLine()
{
	const FCaptionEntry* Current = Queue.GetCurrent();
	if (!Current || Current->Serial == LoggedSerial)
	{
		return;
	}
	LoggedSerial = Current->Serial;
	const FString Speaker = Current->Line.Speaker.ToString();
	UE_LOG(LogLooter, Log, TEXT("Caption: %s%s"), Speaker.IsEmpty() ? TEXT("") : *(Speaker + TEXT(": ")), *Current->Line.Text.ToString());
}
