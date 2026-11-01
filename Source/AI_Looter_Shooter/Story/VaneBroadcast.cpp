// UVaneBroadcast: Cornelius Vane on the radio: the crackle, his lines as captions under a hiss, the click.

#include "Story/VaneBroadcast.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Story/CaptionSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "VaneBroadcast"

UVaneBroadcast* UVaneBroadcast::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UVaneBroadcast>() : nullptr;
}

bool UVaneBroadcast::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

FText UVaneBroadcast::SpeakerName()
{
	return LOCTEXT("Vane", "Cornelius Vane");
}

void UVaneBroadcast::Say(UObject* Context, const TArray<FStoryLine>& Lines)
{
	if (Lines.IsEmpty())
	{
		return;
	}
	if (UVaneBroadcast* Radio = Get(Context))
	{
		Radio->Begin(Lines);
	}
}

void UVaneBroadcast::Stop(UObject* Context)
{
	if (UVaneBroadcast* Radio = Get(Context))
	{
		if (Radio->bOnAir)
		{
			Radio->End(/*bClearCaptions*/ true);
		}
	}
}

bool UVaneBroadcast::IsSpeaking(const UObject* Context)
{
	const UVaneBroadcast* Radio = Get(Context);
	return Radio && Radio->bOnAir;
}

void UVaneBroadcast::Begin(const TArray<FStoryLine>& Lines)
{
	if (bOnAir)
	{
		// He cuts himself off: one voice on the air.
		End(/*bClearCaptions*/ true);
	}
	Pending = Lines;
	for (FStoryLine& Line : Pending)
	{
		if (Line.Speaker.IsEmpty())
		{
			Line.Speaker = SpeakerName();
		}
	}
	bOnAir = true;
	bLinesStarted = false;
	Conversation = INDEX_NONE;
	// Someone is talking (Hob's remark, a talk): he waits for quiet, silent; otherwise the crackle now.
	bWaiting = true;
	WaitLeft = QuietWaitSeconds;
	if (AreCaptionsQuiet())
	{
		StartRadio();
	}
	UE_LOG(LogLooter, Log, TEXT("Vane on the air: %d lines%s."), Pending.Num(), bWaiting ? TEXT(", once the captions are quiet") : TEXT(""));
}

void UVaneBroadcast::StartRadio()
{
	bWaiting = false;
	LeadLeft = RadioLeadSeconds;
	LooterSound::Play2D(this, LooterSoundCue::VaneRadioIn);
	Hiss = LooterSound::Start(this, LooterSoundCue::VaneRadioHiss);
}

bool UVaneBroadcast::AreCaptionsQuiet() const
{
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	return !Captions || Captions->GetQueue().IsEmpty();
}

void UVaneBroadcast::End(bool bClearCaptions)
{
	if (bClearCaptions && bLinesStarted)
	{
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			Captions->Clear();
		}
	}
	// Still waiting for quiet, he was never heard: no click.
	if (!bWaiting)
	{
		LooterSound::Stop(Hiss.Get(), 0.2f);
		LooterSound::Play2D(this, LooterSoundCue::VaneRadioOut);
	}
	Hiss.Reset();
	Pending.Reset();
	Conversation = INDEX_NONE;
	bOnAir = false;
	bWaiting = false;
	bLinesStarted = false;
}

void UVaneBroadcast::Advance(float DeltaSeconds)
{
	if (!bOnAir)
	{
		return;
	}
	if (bWaiting)
	{
		WaitLeft -= DeltaSeconds;
		if (AreCaptionsQuiet() || WaitLeft <= 0.f)
		{
			StartRadio();
		}
		return;
	}
	if (!bLinesStarted)
	{
		LeadLeft -= DeltaSeconds;
		if (LeadLeft > 0.f)
		{
			return;
		}
		bLinesStarted = true;
		// After whatever is being said (a line that came during the crackle, or one still going when his wait ran out): story
		// lines never overlap or cut each other off.
		UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
		Conversation = Captions ? Captions->Play(Pending, ECaptionPlay::Queue) : INDEX_NONE;
		return;
	}
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (!Captions || Conversation == INDEX_NONE || !Captions->IsPlaying(Conversation))
	{
		End(/*bClearCaptions*/ false);
	}
}

void UVaneBroadcast::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UVaneBroadcast::IsTickable() const
{
	return bOnAir;
}

TStatId UVaneBroadcast::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVaneBroadcast, STATGROUP_Tickables);
}

void UVaneBroadcast::Deinitialize()
{
	LooterSound::Stop(Hiss.Get(), 0.f);
	Hiss.Reset();
	Pending.Reset();
	bOnAir = false;
	bWaiting = false;
	Super::Deinitialize();
}

#undef LOCTEXT_NAMESPACE
