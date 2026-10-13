// ALooterHUD: the "GRAVE MARKED" banner, shown when the player walks up to a respawn grave for the first time (ARespawnMarker).

#include "UI/HUD/LooterHUD.h"
#include "UI/HUD/PlayerHUDWidget.h"

void ALooterHUD::ShowGraveMarked(const FText& GraveName)
{
	// The gameplay HUD's banner; it waits for menus, scenes and the other banners by itself.
	if (HUDWidget)
	{
		HUDWidget->ShowGraveMarked(GraveName);
	}
}
