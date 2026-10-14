#include "Weapons/NearMiss.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Weapons/CreatureBlood.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// Until Audio/LooterSoundCues.h lists them (a cue in the header without its sounds fails a test): the names Main's
	// sound pass makes.
	const TCHAR* const NearMissWhizCue = TEXT("Weapon.NearMiss.Whiz");
	const TCHAR* const NearMissHissCue = TEXT("Weapon.NearMiss.Hiss");
}

float NearMiss::ClosestApproach(const FVector& From, const FVector& To, const FVector& Head, FVector& OutNearest)
{
	OutNearest = FMath::ClosestPointOnSegment(Head, From, To);
	return static_cast<float>(FVector::Dist(OutNearest, Head));
}

ENearMissSound NearMiss::SoundOf(const AActor* Shooter)
{
	return CreatureBlood::IsWet(CreatureBlood::Of(Shooter)) ? ENearMissSound::Hiss : ENearMissSound::Whiz;
}

FName NearMiss::CueOf(ENearMissSound Sound)
{
	switch (Sound)
	{
	case ENearMissSound::None:
		return NAME_None;
	case ENearMissSound::Hiss:
		return NearMissHissCue;
	case ENearMissSound::Whiz:
	case ENearMissSound::Auto:
	default:
		return NearMissWhizCue;
	}
}

FVector NearMiss::HeadOf(const APawn& Listener)
{
	return Listener.GetPawnViewLocation();
}

APawn* NearMiss::FindPassed(const UWorld& World, const FVector& From, const FVector& To, float Flown, const AActor* Shooter,
	FVector& OutNearest)
{
	APawn* Passed = nullptr;
	float Nearest = Radius;
	// The local players' pawns (one in this game): a whiz is for the ears of whoever plays, never an NPC's.
	for (FConstPlayerControllerIterator It = World.GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Player = It->Get();
		APawn* Pawn = Player && Player->IsLocalController() ? Player->GetPawn() : nullptr;
		if (!Pawn || Pawn == Shooter)
		{
			continue;
		}
		const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
		if (Health && Health->IsDead())
		{
			continue;
		}
		FVector Point;
		const float Distance = ClosestApproach(From, To, HeadOf(*Pawn), Point);
		// Point blank isn't a fly-by: the shot itself is what's heard.
		if (Distance > Nearest || Flown + static_cast<float>(FVector::Dist(From, Point)) < MinFlight)
		{
			continue;
		}
		Nearest = Distance;
		Passed = Pawn;
		OutNearest = Point;
	}
	return Passed;
}

void NearMiss::Play(const UObject* WorldContext, ENearMissSound Sound, const AActor* Shooter, const FVector& Nearest)
{
	const ENearMissSound Heard = Sound == ENearMissSound::Auto ? SoundOf(Shooter) : Sound;
	const FName Cue = CueOf(Heard);
	if (!Cue.IsNone())
	{
		LooterSound::PlayAt(WorldContext, Cue, Nearest);
	}
}

bool NearMiss::HearFlyBy(const UWorld& World, const FVector& From, const FVector& To, float Flown, const AActor* Shooter,
	ENearMissSound Sound, bool& bInOutHeard)
{
	FVector Nearest;
	if (bInOutHeard || Sound == ENearMissSound::None || !FindPassed(World, From, To, Flown, Shooter, Nearest))
	{
		return false;
	}
	bInOutHeard = true;
	Play(&World, Sound, Shooter, Nearest);
	return true;
}
