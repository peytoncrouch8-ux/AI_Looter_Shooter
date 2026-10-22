#pragma once

#include "CoreMinimal.h"
#include "Weapons/DeathSplats.h"
#include "Weapons/VfxTypes.h"

class ACreatureBase;
struct FCreatureHit;

/**
 * A creature's kill as it looks (the VFX pass, Saved/Vfx/PLAN.md), on top of the death burst its hit reaction plays
 * (UCreatureHitReactionComponent::SpawnDeathBurst): the kill's own layers (FWeaponFX::SpawnKillLayers: gore thrown by the gun,
 * mist, a dust skirt, a soul-light's ring and ash, a crit's flare, its tier's rings, star and motes) and the splat it leaves
 * on the ground (UVfxSubsystem). Each kind's death calls Play (SpiderCreatureReact.cpp, SlimeCreatureReact.cpp,
 * UnpaidCreatureMoods.cpp) with its last hit; a boss's death is its own show and plays nothing here.
 */
namespace CreatureKillLook
{
	/** The splat a creature of this blood leaves (an Unpaid's ash; a miner's too). */
	AI_LOOTER_SHOOTER_API EVfxSplat SplatFor(ECreatureBlood Blood);

	/** The kill as the effects take it: its body, the ground under it, what it spills, its tier, the killing hit. */
	AI_LOOTER_SHOOTER_API FVfxKill MakeKill(const ACreatureBase& Creature, const FCreatureHit& Hit);

	/** Plays the kill's layers and lays its splat (bSplat), unless it's a boss's or nobody is near enough to see. True if played. */
	AI_LOOTER_SHOOTER_API bool Play(ACreatureBase& Creature, const FCreatureHit& Hit, bool bSplat = true);

	/** Lays only its splat (a body that settles before its pool spreads: the spider's lands a beat after it dies). */
	AI_LOOTER_SHOOTER_API bool LaySplat(ACreatureBase& Creature);

	/** Kills farther than this from every view play nothing (cm): nobody sees them, and they'd cost a trace and a splat. */
	inline constexpr float SeenWithin = 9000.f;
}
