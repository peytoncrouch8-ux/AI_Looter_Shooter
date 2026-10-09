#include "World/TownLifeSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Story/SpeakerPointComponent.h"
#include "World/WindowShutter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	/** A source naming no household joins the nearest one that does within this (cm): a shop's shutters, its lights'. */
	constexpr float NeighbourRadius = 1500.f;
}

UTownLifeSubsystem* UTownLifeSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject && GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTownLifeSubsystem>() : nullptr;
}

bool UTownLifeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UTownLifeSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Timer);
	}
	Sources.Reset();
	MutterPoints.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Who joins
// ---------------------------------------------------------------------------

void UTownLifeSubsystem::AddSource(AActor* Source, FName Household)
{
	if (!Source)
	{
		return;
	}
	RemoveSource(Source);
	Sources.Add({ Source, Household });
	UpdateTimer();
}

void UTownLifeSubsystem::RemoveSource(AActor* Source)
{
	Sources.RemoveAll([Source](const FSource& Each) { return !Each.Actor.IsValid() || Each.Actor.Get() == Source; });
	UpdateTimer();
}

void UTownLifeSubsystem::AddMutterPoint(USpeakerPointComponent* Point)
{
	if (Point)
	{
		MutterPoints.RemoveAll([](const TWeakObjectPtr<USpeakerPointComponent>& Each) { return !Each.IsValid(); });
		MutterPoints.AddUnique(Point);
		UpdateTimer();
	}
}

void UTownLifeSubsystem::RemoveMutterPoint(USpeakerPointComponent* Point)
{
	MutterPoints.RemoveAll([Point](const TWeakObjectPtr<USpeakerPointComponent>& Each) { return !Each.IsValid() || Each.Get() == Point; });
	UpdateTimer();
}

int32 UTownLifeSubsystem::NumSources() const
{
	return Sources.FilterByPredicate([](const FSource& Each) { return Each.Actor.IsValid(); }).Num();
}

int32 UTownLifeSubsystem::NumMutterPoints() const
{
	return MutterPoints.FilterByPredicate([](const TWeakObjectPtr<USpeakerPointComponent>& Each) { return Each.IsValid(); }).Num();
}

void UTownLifeSubsystem::UpdateTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (Sources.IsEmpty() && MutterPoints.IsEmpty())
	{
		Timers.ClearTimer(Timer);
	}
	else if (!Timers.IsTimerActive(Timer))
	{
		Timers.SetTimer(Timer, this, &UTownLifeSubsystem::Look, TownLifeRules::CheckSeconds, /*bLoop*/ true,
			Random.FRandRange(0.2f, TownLifeRules::CheckSeconds));
	}
}

// ---------------------------------------------------------------------------
// Households
// ---------------------------------------------------------------------------

bool UTownLifeSubsystem::IsListening(const FSource& Source) const
{
	const AActor* Actor = Source.Actor.Get();
	if (!Actor || Actor->IsHidden())
	{
		return false;
	}
	// Behind shuttered windows: an open one is somebody watching, not hiding, and says nothing.
	const AWindowShutter* Shutter = Cast<AWindowShutter>(Actor);
	return !Shutter || Shutter->IsShut();
}

FName UTownLifeSubsystem::ResolveHousehold(const FSource& Source) const
{
	if (!Source.Household.IsNone())
	{
		return Source.Household;
	}
	const AActor* Actor = Source.Actor.Get();
	if (!Actor)
	{
		return NAME_None;
	}
	FName Found;
	double Best = FMath::Square(static_cast<double>(NeighbourRadius));
	for (const FSource& Other : Sources)
	{
		const AActor* OtherActor = Other.Actor.Get();
		if (OtherActor && OtherActor != Actor && !Other.Household.IsNone())
		{
			const double Squared = FVector::DistSquared(OtherActor->GetActorLocation(), Actor->GetActorLocation());
			if (Squared <= Best)
			{
				Best = Squared;
				Found = Other.Household;
			}
		}
	}
	return Found;
}

FName UTownLifeSubsystem::GetHouseholdOf(const AActor* Actor) const
{
	const FSource* Source = Sources.FindByPredicate([Actor](const FSource& Each) { return Each.Actor.Get() == Actor; });
	return Source ? ResolveHousehold(*Source) : NAME_None;
}

// ---------------------------------------------------------------------------
// The look round
// ---------------------------------------------------------------------------

void UTownLifeSubsystem::Look()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const AActor* Player = TestPlayer.Get();
	if (!Player)
	{
		const APlayerController* Controller = World->GetFirstPlayerController();
		Player = Controller ? Controller->GetPawn() : nullptr;
	}
	if (Player)
	{
		Check(Player->GetActorLocation(), World->GetTimeSeconds());
	}
}

void UTownLifeSubsystem::Check(const FVector& Player, double Now)
{
	// The houses near the player, each by its nearest source, nearest first; and whether the player is in town at all.
	struct FNear
	{
		FName Household;
		FVector Where;
		double DistSquared = 0.0;
	};
	TArray<FNear, TInlineAllocator<8>> Near;
	TArray<FNear, TInlineAllocator<16>> Heard;
	bool bInTown = false;
	const double HearSquared = FMath::Square(static_cast<double>(TownLifeRules::HearRadius));
	const double TownSquared = FMath::Square(static_cast<double>(TownLifeRules::TownRadius));
	for (const FSource& Source : Sources)
	{
		if (!IsListening(Source))
		{
			continue;
		}
		const FName Household = ResolveHousehold(Source);
		if (Household.IsNone() || !TownLifeRules::FindHousehold(Household))
		{
			continue;
		}
		const FVector Where = Source.Actor->GetActorLocation();
		const double Squared = FVector::DistSquared(Where, Player);
		bInTown |= Squared <= TownSquared;
		Heard.Add({ Household, Where, Squared });
		if (Squared > HearSquared)
		{
			continue;
		}
		FNear* Known = Near.FindByPredicate([Household](const FNear& Each) { return Each.Household == Household; });
		if (!Known)
		{
			Near.Add({ Household, Where, Squared });
		}
		else if (Squared < Known->DistSquared)
		{
			Known->Where = Where;
			Known->DistSquared = Squared;
		}
	}
	Near.Sort([](const FNear& A, const FNear& B) { return A.DistSquared < B.DistSquared; });

	// A house heard through its walls: likelier on the first pass in a while, rare while the player lingers.
	bool bPlayed = false;
	for (const FNear& House : Near)
	{
		const bool bFresh = Clock.IsFreshVisit(House.Household, Now);
		Clock.MarkNear(House.Household, Now);
		if (!bPlayed && Clock.WantsSound(House.Household, Now, bFresh, Random))
		{
			const FName Cue = Clock.PickSound(*TownLifeRules::FindHousehold(House.Household), Random);
			PlayFrom(Cue, House.Where, House.Household);
			Clock.Played(House.Household, Now, Random);
			bPlayed = true;
		}
	}

	// A dog far off, from a lived-in house well away from the player, now and then while they're in town.
	if (bInTown && !bPlayed && Clock.WantsDog(Now, Random))
	{
		const FNear* Farthest = nullptr;
		for (const FNear& House : Heard)
		{
			const TownLifeRules::FHousehold* Home = TownLifeRules::FindHousehold(House.Household);
			if (Home && Home->bKeepsDog && House.DistSquared >= FMath::Square(static_cast<double>(TownLifeRules::DogNear))
				&& House.DistSquared <= FMath::Square(static_cast<double>(TownLifeRules::DogFar))
				&& (!Farthest || House.DistSquared > Farthest->DistSquared))
			{
				Farthest = &House;
			}
		}
		if (Farthest)
		{
			PlayFrom(LooterSoundCue::TownLife::DogFar, Farthest->Where, Farthest->Household);
			Clock.DogBarked(Now, Random);
			bPlayed = true;
		}
	}

	// A door the player passes may mutter a line (one try a look, the nearest door first).
	if (!Clock.MayMutter(Now))
	{
		return;
	}
	USpeakerPointComponent* Door = nullptr;
	double DoorSquared = TNumericLimits<double>::Max();
	for (const TWeakObjectPtr<USpeakerPointComponent>& Each : MutterPoints)
	{
		USpeakerPointComponent* Point = Each.Get();
		if (!Point || !Point->CanMutter(Now))
		{
			continue;
		}
		const double Squared = FVector::DistSquared(Point->GetComponentLocation(), Player);
		if (Squared <= FMath::Square(static_cast<double>(Point->MutterRadius)) && Squared < DoorSquared)
		{
			Door = Point;
			DoorSquared = Squared;
		}
	}
	if (Door && Random.FRand() < TownLifeRules::MutterChance && Door->Mutter(Now))
	{
		Clock.Muttered(Now, Random);
		LastMutterer = Door;
		// Their voice through the door, under the caption.
		const FName Household = GetHouseholdOf(Door->GetOwner());
		PlayFrom(LooterSoundCue::TownLife::Voices, Door->GetComponentLocation(), Household, TownLifeRules::MutterVoiceVolume);
	}
}

void UTownLifeSubsystem::PlayFrom(FName Cue, const FVector& Where, FName Household, float Volume)
{
	const TownLifeRules::FHousehold* Home = TownLifeRules::FindHousehold(Household);
	const float Pitch = Home && TownLifeRules::IsVoice(Cue) ? Home->VoicePitch : 1.f;
	LooterSound::PlayAt(this, Cue, Where, Volume, Pitch);
	LastSound = Cue;
	LastSoundAt = Where;
	LastHousehold = Household;
	UE_LOG(LogLooter, Verbose, TEXT("Town life: %s from the %s household."), *Cue.ToString(), *Household.ToString());
}
