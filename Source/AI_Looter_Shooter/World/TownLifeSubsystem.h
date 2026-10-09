#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Math/RandomStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/TownLifeRules.h"
#include "TownLifeSubsystem.generated.h"

class USpeakerPointComponent;

/**
 * Signs of the living in town (TownLifeRules has the households and the timing): the lived-in houses' lights
 * (AHouseLights), their shut shutters (AWindowShutter) and the townsfolk's doors (ATownLifeDoor) join as sound sources of
 * their household, and every speaker point with mutters joins as one that may speak up as the player passes. Once a
 * second, while any has joined, it looks round the player with plain distances (no traces): a house near them may be
 * heard through its walls, a dog barks far off now and then, and a door they pass may mutter a line as a caption. Played
 * worlds, and the editor preview worlds the automated tests build.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UTownLifeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTownLifeSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	/** Source is heard as Household (TownLifeRules::FindHousehold; None: it joins the household of the nearest other source). */
	void AddSource(AActor* Source, FName Household);
	void RemoveSource(AActor* Source);

	/** A speaker point with mutters (USpeakerPointComponent::Mutters) joins, or leaves. */
	void AddMutterPoint(USpeakerPointComponent* Point);
	void RemoveMutterPoint(USpeakerPointComponent* Point);

	/** One look round a player standing at Player, at world time Now (the timer's, from the player's pawn; tests call it). */
	void Check(const FVector& Player, double Now);

	/** A test's stand-in for the player: a test level has no player controller. */
	void SetTestPlayer(AActor* StandIn) { TestPlayer = StandIn; }

	/** The household a source is heard as (its own, or its nearest neighbour's), or None. */
	FName GetHouseholdOf(const AActor* Source) const;

	/** What was heard last: the cue, where, and from which household (None: nothing yet). */
	FName GetLastSound() const { return LastSound; }
	FVector GetLastSoundAt() const { return LastSoundAt; }
	FName GetLastHousehold() const { return LastHousehold; }

	/** The door that muttered last, or null. */
	const USpeakerPointComponent* GetLastMutterer() const { return LastMutterer.Get(); }

	const FTownLifeClock& GetClock() const { return Clock; }
	int32 NumSources() const;
	int32 NumMutterPoints() const;

	/** The dice, fixed for a test. */
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

private:
	struct FSource
	{
		TWeakObjectPtr<AActor> Actor;
		FName Household;
	};

	void UpdateTimer();
	void Look();
	/** A source can be heard now: still there, and a shutter shut (an open window has nobody hiding behind it). */
	bool IsListening(const FSource& Source) const;
	FName ResolveHousehold(const FSource& Source) const;
	void PlayFrom(FName Cue, const FVector& Where, FName Household, float Volume = 1.f);

	TArray<FSource> Sources;
	TArray<TWeakObjectPtr<USpeakerPointComponent>> MutterPoints;
	FTownLifeClock Clock;
	FRandomStream Random = FRandomStream(20261009);
	FTimerHandle Timer;
	TWeakObjectPtr<AActor> TestPlayer;
	FName LastSound;
	FVector LastSoundAt = FVector::ZeroVector;
	FName LastHousehold;
	TWeakObjectPtr<USpeakerPointComponent> LastMutterer;
};
