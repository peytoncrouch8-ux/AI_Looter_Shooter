#include "Weapons/WeaponTrigger.h"

WeaponTrigger::EPress WeaponTrigger::Decide(EWeaponFireMode FireMode, double SecondsUntilReady, bool bShotWaiting)
{
	// Already cycling: one shot is on its way, and a second press adds nothing (this is what keeps clicks from stacking).
	if (bShotWaiting)
	{
		return EPress::Ignore;
	}
	if (SecondsUntilReady <= 0.0)
	{
		return EPress::FireNow;
	}
	// A held trigger on a full-auto gun fires when ready; letting go cancels it, so nothing arrives late.
	if (FireMode == EWeaponFireMode::FullAuto)
	{
		return EPress::Wait;
	}
	return SecondsUntilReady <= BufferSeconds ? EPress::Wait : EPress::Ignore;
}
