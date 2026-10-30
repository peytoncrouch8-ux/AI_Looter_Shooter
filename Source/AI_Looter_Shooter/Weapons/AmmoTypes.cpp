#include "Weapons/AmmoTypes.h"

namespace
{
	// KillBox: a kill's pickup holds half of it to all of it, by class (the user, 2026-10-29 night), so a shotgun's kill
	// gives 4-8 shells rather than 18-36 (which filled its 48 in two kills) and a sniper 3-6.
	//                                       Name              Max  Box         KillBox
	const LooterAmmo::FInfo Infos[] = {
		/* AssaultRifle */ { TEXT("AR Ammo"),        280, 36, TEXT("AR"),  36 },
		/* Shotgun      */ { TEXT("Shotgun Shells"),  48,  8, TEXT("SG"),   8 },
		/* Pistol       */ { TEXT("Pistol Ammo"),    200, 24, TEXT("PS"),  24 },
		/* SMG          */ { TEXT("SMG Ammo"),       360, 48, TEXT("SMG"), 48 },
		/* Sniper       */ { TEXT("Sniper Rounds"),   36,  6, TEXT("SR"),   6 },
		// Each rocket is a fight's answer: the pool stays small (kills drop one to three: LooterLoot::RocketsFor).
		/* Rocket       */ { TEXT("Rockets"),         12,  2, TEXT("RKT"),  3 },
	};
	static_assert(UE_ARRAY_COUNT(Infos) == LooterAmmo::NumTypes, "One entry per ammo type");

	const EAmmoType Types[] = { EAmmoType::AssaultRifle, EAmmoType::Shotgun, EAmmoType::Pistol, EAmmoType::SMG, EAmmoType::Sniper,
		EAmmoType::Rocket };
	static_assert(UE_ARRAY_COUNT(Types) == LooterAmmo::NumTypes, "One entry per ammo type");
}

const LooterAmmo::FInfo& LooterAmmo::GetInfo(EAmmoType Type)
{
	return Infos[FMath::Clamp(static_cast<int32>(Type), 0, NumTypes - 1)];
}

TConstArrayView<EAmmoType> LooterAmmo::AllTypes()
{
	return MakeArrayView(Types);
}
