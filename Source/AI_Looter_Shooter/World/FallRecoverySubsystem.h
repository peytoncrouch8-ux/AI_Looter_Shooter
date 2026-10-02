#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/FallRecoveryTracker.h"
#include "FallRecoverySubsystem.generated.h"

class ACharacter;
class APawn;
class APlayerController;

/** What OnRecovered tells: who was brought back, why, and from where to where. */
struct FFallRecoveryEvent
{
	APawn* Pawn = nullptr;
	EFallRecoveryReason Reason = EFallRecoveryReason::LongFall;
	/** Where the player was when they were brought back. */
	FVector From = FVector::ZeroVector;
	/** The safe spot they were put back on. */
	FVector To = FVector::ZeroVector;
	/** The playable area's edge they left it over (APlayableArea), or INDEX_NONE. */
	int32 Edge = INDEX_NONE;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFallRecovered, const FFallRecoveryEvent& /*Event*/);

/**
 * Walking off an edge is easy (a floating island's rim, the Rim over a canyon), so instead of killing the player this
 * remembers where each player last stood on solid ground and, after a fall, fades them back there. Without a playable
 * area in the level (the tutorial island) a fall of RecoverDropHeight brings them back. With one (APlayableArea), safe
 * spots are taken only inside it, a player who leaves it over an open edge comes back once they've dropped
 * OpenEdgeDropHeight, and the long fall stays as the backstop. FFallRecoveryTracker holds the rules.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UFallRecoverySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** How far below the last safe spot a player may fall before being returned, anywhere (cm). */
	UPROPERTY(EditAnywhere, Category = "Fall Recovery")
	float RecoverDropHeight = 3000.f;

	/** With a playable area: how far below the last safe spot a player who left it over an open edge may drop (cm). */
	UPROPERTY(EditAnywhere, Category = "Fall Recovery")
	float OpenEdgeDropHeight = 500.f;

	/** Fires each time a player is brought back (Hob's lines about falls, the boss's Gravewind phase). */
	FOnFallRecovered OnRecovered;

private:
	/** Puts the player back on their safe spot, behind a pale flash, and tells OnRecovered. */
	void Recover(APlayerController& PC, ACharacter& Character, FFallRecoveryTracker& Tracker, EFallRecoveryReason Reason);

	TMap<TWeakObjectPtr<APawn>, FFallRecoveryTracker> Trackers;
};
