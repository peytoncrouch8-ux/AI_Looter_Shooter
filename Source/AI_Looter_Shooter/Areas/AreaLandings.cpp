#include "Areas/AreaLandings.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerStart.h"

namespace AreaLandings
{
	bool IsLandingTag(FName Tag)
	{
		return !Tag.IsNone() && Tag.ToString().StartsWith(NamePrefix);
	}

	bool IsLanding(const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}
		const APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
		if (PlayerStart && IsLandingTag(PlayerStart->PlayerStartTag))
		{
			return true;
		}
		return Actor->Tags.ContainsByPredicate([](FName Tag) { return IsLandingTag(Tag); });
	}

	AActor* Find(const UWorld* World, FName Landing)
	{
		if (!World || Landing.IsNone())
		{
			return nullptr;
		}
		// A player start named for the landing wins: the player is made right there as the level starts.
		AActor* Tagged = nullptr;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const APlayerStart* PlayerStart = Cast<APlayerStart>(*It);
			if (PlayerStart && PlayerStart->PlayerStartTag == Landing)
			{
				return *It;
			}
			if (!Tagged && It->ActorHasTag(Landing))
			{
				Tagged = *It;
			}
		}
		return Tagged;
	}
}
