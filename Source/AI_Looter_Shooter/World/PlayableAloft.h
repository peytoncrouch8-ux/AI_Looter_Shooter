#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayableAloft.generated.h"

UINTERFACE(MinimalAPI)
class UPlayableAloft : public UInterface
{
	GENERATED_BODY()
};

/**
 * A vessel whose deck is playable wherever she hangs. The dredge barge over the chapel knoll is one: 47 m up, her bow
 * reaches 10 m past the boundary's closed edge east of it. While she's added (APlayableArea::AddAloft), the level's
 * playable area counts a point aboard her as inside the boundary. Then encounter spots, creatures' free spots and
 * knockbacks, the player's climbs and the ember powers' paths all work on the whole of her deck. The ground under her
 * stays as the boundary has it. She adds herself as she begins play and goes as she ends.
 */
class AI_LOOTER_SHOOTER_API IPlayableAloft
{
	GENERATED_BODY()

public:
	/** A world point is aboard her now: in her bounds, while she's up there to stand on. */
	virtual bool IsAboardAt(const FVector& WorldPoint) const = 0;
};
