#include "Weapons/AmmoTypes.h"

namespace
{
	//                                       Name              Max  Box  Cartridge r / h / count
	const LooterAmmo::FInfo Infos[] = {
		/* AssaultRifle */ { TEXT("AR Ammo"),        280, 36, 1.1f, 7.f, 4 },
		/* Shotgun      */ { TEXT("Shotgun Shells"),  48,  8, 2.0f, 6.f, 3 },
		/* Pistol       */ { TEXT("Pistol Ammo"),    200, 24, 1.0f, 4.f, 5 },
		/* SMG          */ { TEXT("SMG Ammo"),       360, 48, 0.9f, 5.f, 6 },
		/* Sniper       */ { TEXT("Sniper Rounds"),   36,  6, 1.3f, 11.f, 3 },
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
