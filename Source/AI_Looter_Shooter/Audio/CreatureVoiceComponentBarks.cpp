// UCreatureVoiceComponent: the Unpaid's barks (a line from their lives at their moments, murmured syllable by syllable in a
// voice of their own) and the spiders' and slimes' rare idle calls. The rules are CreatureVoiceBarks.h's; the level's
// UCreatureVoiceDirector decides whether a bark may be said now, picks its line and shows it.

#include "Audio/CreatureVoiceComponent.h"
#include "Audio/CreatureVoiceDirector.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/UnpaidCreature.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	/** The murmur's loudness against the cries: a mutter to itself under a voiced bark, a dying whisper between them. */
	constexpr float MutterVolume = 0.8f;
	constexpr float DyingVolume = 0.9f;
	constexpr float VoicedVolume = 1.f;
	/** An angrier soul says its piece a little louder. */
	constexpr float AngryVolume = 1.1f;

	/** An idle call is quiet: heard close by, not across a field. */
	constexpr float IdleCallVolume = 0.7f;
}

bool UCreatureVoiceComponent::CanBark() const
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	return Creature && Creature->IsA<AUnpaidCreature>() && CreatureBarks::CanBarkRank(Creature->GetRank());
}

float UCreatureVoiceComponent::RankPitch(ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Rare: return 0.97f;
	case ECreatureRank::Epic: return 0.94f;
	case ECreatureRank::Legendary: return 0.92f;
	default: return 1.f;
	}
}

bool UCreatureVoiceComponent::TryBark(ECreatureBark Situation)
{
	UWorld* World = GetWorld();
	if (!World || !CanBark())
	{
		return false;
	}
	const double Now = World->GetTimeSeconds();
	// Its last words always get their chance; anything else waits out its rest after its own last bark, and never stacks.
	const bool bLastWords = Situation == ECreatureBark::Death;
	if (!bLastWords && (Now < NextBark || PendingBark.IsSet()))
	{
		return false;
	}
	if (Random.FRand() >= CreatureBarks::ChanceFor(Situation))
	{
		return false;
	}
	PendingBark = Situation;
	const float Delay = CreatureBarks::DelayFor(Situation);
	if (Delay <= 0.f)
	{
		SayPendingBark();
		return true;
	}
	World->GetTimerManager().SetTimer(BarkTimer, this, &UCreatureVoiceComponent::SayPendingBark, Delay, false);
	return true;
}

void UCreatureVoiceComponent::SayPendingBark()
{
	if (!PendingBark.IsSet())
	{
		return;
	}
	const ECreatureBark Situation = PendingBark.GetValue();
	PendingBark.Reset();
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	UWorld* World = GetWorld();
	if (!Creature || !World)
	{
		return;
	}
	// Only the dead speak after dying; a spot needs the one it saw still in its sights.
	if (Situation != ECreatureBark::Death && Creature->IsDead())
	{
		return;
	}
	if (Situation == ECreatureBark::Spot && !Creature->GetTarget())
	{
		return;
	}
	UCreatureVoiceDirector* Director = UCreatureVoiceDirector::Get(this);
	if (Director && Director->RequestBark(*this, Situation))
	{
		NextBark = World->GetTimeSeconds() + CreatureBarks::SpeakerRest;
	}
}

void UCreatureVoiceComponent::SpeakMurmur(const FString& Line, ECreatureBark Situation)
{
	StopMurmur();
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	UWorld* World = GetWorld();
	if (!Creature || !World)
	{
		return;
	}
	Murmur = CreatureBarks::BuildMurmur(Line, GetTypeHash(Line) ^ GetTypeHash(Creature->GetFName().ToString()));
	MurmurNext = 0;
	const bool bWhispered = CreatureBarks::IsWhispered(Situation);
	MurmurCue = bWhispered ? FName(LooterSoundCue::Voice::UnpaidMutter) : FName(LooterSoundCue::Voice::UnpaidMurmur);
	MurmurVolume = Situation == ECreatureBark::Death ? DyingVolume : (bWhispered ? MutterVolume : VoicedVolume);
	if (CreatureBarks::IsAngryRank(Creature->GetRank()))
	{
		MurmurVolume *= AngryVolume;
	}
	PlayNextGrain();
}

void UCreatureVoiceComponent::StopMurmur()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MurmurTimer);
	}
	Murmur.Reset();
	MurmurNext = 0;
}

void UCreatureVoiceComponent::PlayNextGrain()
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	UWorld* World = GetWorld();
	if (!Creature || !World || !Murmur.IsValidIndex(MurmurNext))
	{
		return;
	}
	const FMurmurGrain& Grain = Murmur[MurmurNext];
	// Its kind's syllable in its own voice: the line's contour, times its voice, its size and its rank.
	const float Pitch = Grain.Pitch * VoicePitch * GetPitch() * RankPitch(Creature->GetRank());
	LooterSound::PlayAttached(MurmurCue, Creature->GetRootComponent(), NAME_None, Grain.Volume * MurmurVolume, Pitch);
	++MurmurNext;
	if (Murmur.IsValidIndex(MurmurNext))
	{
		const float Wait = FMath::Max(Murmur[MurmurNext].Time - Grain.Time, 0.03f);
		World->GetTimerManager().SetTimer(MurmurTimer, this, &UCreatureVoiceComponent::PlayNextGrain, Wait, false);
	}
}

bool UCreatureVoiceComponent::PlayIdleCall()
{
	if (Cries.Idle.IsNone())
	{
		return false;
	}
	Play(Cries.Idle, IdleCallVolume);
	return true;
}
