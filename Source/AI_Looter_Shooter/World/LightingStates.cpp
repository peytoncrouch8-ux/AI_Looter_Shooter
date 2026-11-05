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

// ---------------------------------------------------------------------------
// How lit the town is in a state
// ---------------------------------------------------------------------------

namespace
{
	struct FStateKind
	{
		FName State;
		ELightKind Kind;
	};

	/** Every lighting state the town lights itself for, by name (layout.json level.environment.states). Any other is Day. */
	const TArray<FStateKind>& StateKinds()
	{
		static const TArray<FStateKind> Table = {
			{ FName(TEXT("Day")), ELightKind::Day },
			// The choked golden hour: the lamps lit dim against the smog (the user's call, 2026-11-04: dim lit).
			{ FName(TEXT("Smog")), ELightKind::Gloom },
			{ FName(TEXT("SmogThin")), ELightKind::Gloom },
			{ FName(TEXT("Dusk")), ELightKind::Dusk },
			// Mains 6-7 run it; the town's lamps are lit for the dusk it is, smog or not.
			{ FName(TEXT("DuskSmog")), ELightKind::Dusk },
		};
		return Table;
	}
}

ELightKind LightingStates::Kind(FName State)
{
	if (State.IsNone())
	{
		return ELightKind::Day;
	}
	// FName equality ignores case, as the console's state names do ("dusksmog").
	const FStateKind* Found = StateKinds().FindByPredicate([State](const FStateKind& Each) { return Each.State == State; });
	return Found ? Found->Kind : ELightKind::Day;
}

const TCHAR* LightingStates::KindName(ELightKind Kind)
{
	switch (Kind)
	{
	case ELightKind::Gloom: return TEXT("Gloom");
	case ELightKind::Dusk: return TEXT("Dusk");
	default: return TEXT("Day");
	}
}
