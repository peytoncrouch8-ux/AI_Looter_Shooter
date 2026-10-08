#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CriticalSpotTarget.generated.h"

UINTERFACE(MinimalAPI)
class UCriticalSpotTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * Killable targets with a critical spot (a creature's head, a dummy's head bone) implement this. The target
 * only says *where* its critical spots are; how much a critical hit deals is the game-wide rule in
 * LooterCombat::CriticalHitMultiplier, unless the gun's curse changes it (WeaponCurses::CritMultiplier: Unlucky, Cold).
 */
class AI_LOOTER_SHOOTER_API ICriticalSpotTarget
{
	GENERATED_BODY()

public:
	virtual bool IsCriticalSpot(const FHitResult& Hit) const = 0;
};
