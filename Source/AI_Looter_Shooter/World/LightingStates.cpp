#include "World/LightingStates.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialParameterCollection.h"

const FName ALightingStates::DayState(TEXT("Day"));
const TCHAR* ALightingStates::DefaultCollectionPath = TEXT("/Game/Art/Materials/MPC_Lighting.MPC_Lighting");

ALightingStates::ALightingStates()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialState = DayState;
	// A soft reference with a default, so a level placed before the collection existed still finds it.
	Collection = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(DefaultCollectionPath));
}

ALightingStates* ALightingStates::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ALightingStates> It(World); It; ++It)
	{
		if (!It->States.IsEmpty())
		{
			return *It;
		}
	}
	return nullptr;
}

const FLightingState* ALightingStates::FindState(FName StateName) const
{
	// FName equality ignores case, so "dusk" from the console finds Dusk.
	return States.FindByPredicate([StateName](const FLightingState& Candidate) { return Candidate.Name == StateName; });
}

const FLightingState* ALightingStates::GetInitialState() const
{
	const FLightingState* Initial = FindState(InitialState);
	return Initial ? Initial : (States.IsEmpty() ? nullptr : &States[0]);
}

TArray<FName> ALightingStates::GetStateNames() const
{
	TArray<FName> Names;
	for (const FLightingState& Each : States)
	{
		Names.Add(Each.Name);
	}
	return Names;
}
