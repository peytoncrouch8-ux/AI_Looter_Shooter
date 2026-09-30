#include "Weapons/AmmoTypes.h"

namespace
{
	//                                       Name              Max  Box
	const LooterAmmo::FInfo Infos[] = {
		/* AssaultRifle */ { TEXT("AR Ammo"),        280, 36 },
		/* Shotgun      */ { TEXT("Shotgun Shells"),  48,  8 },
		/* Pistol       */ { TEXT("Pistol Ammo"),    200, 24 },
		/* SMG          */ { TEXT("SMG Ammo"),       360, 48 },
		/* Sniper       */ { TEXT("Sniper Rounds"),   36,  6 },
	};
	static_assert(UE_ARRAY_COUNT(Infos) == LooterAmmo::NumTypes, "One entry per ammo type");

	const EAmmoType Types[] = { EAmmoType::AssaultRifle, EAmmoType::Shotgun, EAmmoType::Pistol, EAmmoType::SMG, EAmmoType::Sniper };
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
