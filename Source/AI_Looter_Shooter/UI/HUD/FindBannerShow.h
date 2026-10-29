#pragma once

#include "CoreMinimal.h"

/**
 * What the find banner shows for one find (UFindSubsystem fills it with the words FindRules makes; ALooterHUD::ShowFind hands it
 * to the banner): a plain struct so the subsystem and the banner share it without including each other. The banner shows the
 * words as given (in capitals) and tells the subsystem when each show is done (UFindSubsystem::BannerDone), which is when the
 * experience is paid.
 */
struct FFindBannerShow
{
	/** The title between the rules; empty: "FOUND". The area's last find: FindRules::CompleteTitle ("EVERY FIND IN RANSOM'S REST"). */
	FText Title;
	/** The find's name ("The Widow's Strongbox"); empty: no name line (the completion). */
	FText Name;
	/** The count line: FindRules::CountLine ("4 / 15 · RANSOM'S REST") or LoreLine ("LEDGER PAGE · 2 / 6"); empty: none. */
	FText Count;
	/** The experience this find pays, counted up on the banner (0: no line). The subsystem pays it when the show is done. */
	int32 Xp = 0;
	/** How many finds this show stands for: above one, the queue was collapsed into it (the title becomes "3 FOUND"). */
	int32 Merged = 1;
};
