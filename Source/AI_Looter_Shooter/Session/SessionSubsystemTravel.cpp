// USessionSubsystem: travel between maps, and arriving at a trip's landing.

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaLandings.h"
#include "Session/SessionSave.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

bool USessionSubsystem::TravelToArea(const UAreaDefinition& Area, FName Landing, bool bByTrain)
{
	const FString Destination = Area.GetMapPackage();
	if (!Area.HasMap())
	{
		const FString MissingLevel = Destination.IsEmpty() ? FString(TEXT("none set")) : Destination;
		UE_LOG(LogLooter, Warning, TEXT("Travel: %s's level (%s) isn't in the game yet, so nobody goes."), *Area.DisplayName.ToString(), *MissingLevel);
		return false;
	}
	return TravelToMap(Destination, Landing.IsNone() ? Area.GetDefaultLanding() : Landing, bByTrain);
}

bool USessionSubsystem::TravelToMap(const FString& MapPackage, FName Landing, bool bByTrain)
{
	UWorld* World = PlayWorld.Get();
	if (SaveGate.IsTravelling())
	{
		UE_LOG(LogLooter, Warning, TEXT("Travel: a trip to %s is already under way."), *SaveGate.GetTripDestination());
		return false;
	}
	if (!World || !bWorldRestored)
	{
		UE_LOG(LogLooter, Warning, TEXT("Travel: no level is being played (or it's still starting), so there's nowhere to leave from."));
		return false;
	}
	const FString Destination = FPackageName::ObjectPathToPackageName(MapPackage);
	if (Destination.IsEmpty() || !FPackageName::IsValidLongPackageName(Destination) || !FPackageName::DoesPackageExist(Destination))
	{
		UE_LOG(LogLooter, Warning, TEXT("Travel: there's no level %s in the game, so nobody goes."), *MapPackage);
		return false;
	}

	// The player and the world being left go into the session first, the world under its own map, where it waits for
	// the player to come back. Without a session that's in memory only: nothing is ever written.
	if (!Current)
	{
		Current = NewSave();
	}
	CaptureSession(*World);
	const FString Leaving = Current->Map;
	Current->PrepareTrip(Destination, Landing);
	if (IsPlayingSession() && !WriteSession())
	{
		// No trip without its save: the player stays, and the session goes back to where they stand.
		CaptureSession(*World);
		Current->ArrivalTag = NAME_None;
		UE_LOG(LogLooter, Error, TEXT("Travel: session %d couldn't be saved, so the trip to %s is off."), ActiveIndex + 1, *Destination);
		return false;
	}

	// From here until the destination begins nothing saves: the level being left would file itself as the session's
	// level again (its teardown save most of all), so the autosave and any save-soon stop with it.
	SaveGate.BeginTrip(Destination);
	StopTimers();
	bSaveWanted = false;
	// How it arrives is the trip's, not the save's: a session loaded later never arrives by train.
	bArrivalByTrain = bByTrain;
	const FString Arrival = Landing.IsNone() ? FString(TEXT("the level's start")) : Landing.ToString();
	UE_LOG(LogLooter, Log, TEXT("Travel: from %s to %s, arriving at %s%s%s"), *FPackageName::GetShortName(Leaving), *Destination, *Arrival,
		bByTrain ? TEXT(" by train") : TEXT(""), IsPlayingSession() ? TEXT("") : TEXT(" (no session: nothing saved)"));
	const FString Options = IsPlayingSession() ? FString::Printf(TEXT("Session=%d"), ActiveIndex + 1) : FString();
	UGameplayStatics::OpenLevel(World, FName(*Destination), /*bAbsolute*/ true, Options);
	return true;
}

void USessionSubsystem::PlaceAtLanding(UWorld* World, FName Landing)
{
	AActor* Spot = AreaLandings::Find(World, Landing);
	if (!Spot)
	{
		UE_LOG(LogLooter, Warning, TEXT("Arrival: %s has no landing %s (an actor or player start tagged so), so the player starts at the level's start."),
			*MapOf(World), *Landing.ToString());
		return;
	}
	APlayerController* Controller = World->GetFirstPlayerController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	// A player start marks where the middle of the player goes (the game mode made them there already). Any other
	// landing is a marker on the ground, so the player stands on it: half their height higher. An actor carrying its
	// landing (the jetty's deck, a station's platform) says where on it.
	const FTransform Arrival = AreaLandings::GetSpot(Spot, Landing);
	FVector Location = Arrival.GetLocation();
	if (!Spot->IsA<APlayerStart>())
	{
		Location.Z += Pawn->GetDefaultHalfHeight();
	}
	const FRotator Facing(0.f, Arrival.Rotator().Yaw, 0.f);
	Pawn->TeleportTo(Location, Facing, /*bIsATest*/ false, /*bNoCheck*/ true);
	Controller->SetControlRotation(Facing);
	UE_LOG(LogLooter, Log, TEXT("Arrival: at %s in %s"), *Landing.ToString(), *FPackageName::GetShortName(MapOf(World)));
}
