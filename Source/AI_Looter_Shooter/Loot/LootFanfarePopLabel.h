#pragma once

#include "CoreMinimal.h"

class AWeaponBase;
class UWeaponLabelWidget;
class UWidgetComponent;
class UWorld;

/**
 * What a Legendary's name pop (ULootFanfareSubsystem, LootFanfareShows.cpp) does to the labels around it: the gun's own label
 * gives way, and the pop slides off the LEVEL UP banner while that shows. Both from the playtest check of 2026-10-27.
 */
namespace LootFanfarePop
{
	/**
	 * The gun's own label gives way while its name pops (it carries the same name, small, in the pile beside it: two names
	 * for one gun) and comes back after (bQuiet false). Looked at too: the legendary lands in the crosshair's line as often as
	 * not, and its small name beside the pop's was the second name.
	 */
	void QuietOwnLabel(const AWeaponBase& Weapon, bool bQuiet);

	/**
	 * Slides a pop off the LEVEL UP banner while that shows (the two drew over each other): under it if there's room on screen,
	 * else over it, eased; back where it hangs once the banner is gone. Call each frame the pop shows.
	 */
	void KeepClearOfLevelUp(UWorld& World, UWidgetComponent& Widget, UWeaponLabelWidget& Label, float DeltaTime);
}
