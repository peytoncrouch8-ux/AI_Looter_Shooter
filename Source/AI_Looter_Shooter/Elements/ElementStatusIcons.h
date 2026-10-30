#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"

/**
 * One status a creature carries, as its tag's icon shows it (UElementStatusComponent::GetStatusIcons; the health bar's
 * draining ring under the bar, lane B's SetStatuses).
 */
struct FCreatureStatusIcon
{
	ELooterElement Element = ELooterElement::None;
	/** Its time left as a share of its whole (1 just set, 0 about to end); a smite's or jolt's: its wait left. */
	float TimeShare = 0.f;
	/** How many of it at once: burns stacked, smites or jolts waiting; 1 for salt and rot. */
	int32 Stacks = 0;
};
