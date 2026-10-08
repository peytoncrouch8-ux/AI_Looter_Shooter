#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Words the missions show: instructions with the player's own keys in them. */
namespace MissionText
{
	/**
	 * Text with each {Action} replaced by the key the player has bound to it, in brackets ({Interact} is "[E]"), and {Move}
	 * by the four movement keys. Without a player to ask (outside a game) each action shows its own name: "[Interact]".
	 * Shared by the missions' objectives and the tracker's lines, so both name the same keys.
	 */
	FString ResolveKeys(const UWorld* World, const FString& Text);

	/**
	 * The key bound to one action as a keycap shows it, without brackets ("E"); Move gives the four movement keys
	 * ("W A S D"). Without a player to ask, the action's own name ("Reload"), as ResolveKeys does.
	 */
	FString KeyName(const UWorld* World, FName Action);
}
