// ALooterHUD: the "POST LIT" banner, shown when the player walks up to one of the Wardens' lantern posts for the first time
// (ARespawnMarker; named for the graves the roosts, then the posts, replaced).

#include "UI/HUD/LooterHUD.h"
#include "UI/HUD/FindBannerShow.h"
#include "UI/HUD/PlayerHUDWidget.h"

void ALooterHUD::ShowFind(const FFindBannerShow& Find)
{
	// The gameplay HUD's "FOUND" banner; it waits for menus, scenes and the other banners by itself.
	if (HUDWidget)
	{
		HUDWidget->ShowFind(Find);
	}
}

void ALooterHUD::ShowGraveMarked(const FText& GraveName)
{
	// The gameplay HUD's banner; it waits for menus, scenes and the other banners by itself.
	if (HUDWidget)
	{
		HUDWidget->ShowGraveMarked(GraveName);
	}
}
