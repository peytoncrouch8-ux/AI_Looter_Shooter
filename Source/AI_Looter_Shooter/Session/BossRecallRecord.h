#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "BossRecallRecord.generated.h"

/**
 * A boss's last defeat as its vigil stone keeps it (World/BossShrine; ULooterSessionSave::BossRecalls, by the stone's boss key:
 * "Gravemother", "Abel"): when it was beaten, in the session's time played and on the clock, which of the session's loads
 * that was in, how many times, and whether its stone has risen. New within version 2: a save from before it has none, which
 * reads as never kept (a boss beaten then is ready to call back at once, and its stone rises the first time the player
 * comes near).
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FBossRecallRecord
{
	GENERATED_BODY()

	/** When it was last beaten, in the session's time played (ULooterSessionSave::PlayedSeconds); negative: never kept. */
	UPROPERTY()
	double DefeatedAt = -1.0;

	/** The same moment on the clock (local time), for people ("Today 14:32"). */
	UPROPERTY()
	FDateTime DefeatedOn;

	/** The session's load it was beaten in (ULooterSessionSave::SessionLoads then): kept for diagnostics, read by no rule since 2026-10-27. */
	UPROPERTY()
	int32 LoadsAtDefeat = 0;

	/** Defeats the stone has kept (its first before the stones existed isn't counted). */
	UPROPERTY()
	int32 Defeats = 0;

	/** Its level as it fell (0: not known). */
	UPROPERTY()
	int32 Level = 0;

	/** Its stone has risen beside the arena: from then on it stands there from the start of every visit. */
	UPROPERTY()
	bool bRisen = false;

	bool HasDefeat() const { return DefeatedAt >= 0.0; }
};
