#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * What a press of the trigger does while the gun is still cycling from its last shot (AWeaponBase::StartFire). Spamming
 * the button on a pump shotgun used to queue a shot from any click in the pump, so the gun kept firing after the player
 * had stopped clicking. Now a semi-auto or burst gun remembers one press, and only a press that comes within
 * BufferSeconds of the gun being ready; anything earlier is lost, like a click on a gun that isn't up. Full auto keeps
 * the trigger held (it fires when ready, and letting go clears it: AWeaponBase::StopFire), so it never fires late.
 */
namespace WeaponTrigger
{
	/** How long before the gun is ready again a press is still remembered, in seconds. */
	inline constexpr double BufferSeconds = 0.12;

	enum class EPress : uint8
	{
		/** The gun is ready: the shot goes now. */
		FireNow,
		/** Remembered: the shot goes the moment the gun is ready. */
		Wait,
		/** Lost: too early for the gun to remember, or a shot is already waiting (at most one is buffered). */
		Ignore
	};

	/**
	 * What a press does. SecondsUntilReady is how long until the gun can fire again (zero or less: now); bShotWaiting is
	 * whether a shot is already on its way (a buffered press, auto fire or the rest of a burst).
	 */
	EPress Decide(EWeaponFireMode FireMode, double SecondsUntilReady, bool bShotWaiting);
}
