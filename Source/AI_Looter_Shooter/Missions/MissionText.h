#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Words the missions show: instructions with the player's own keys in them. */
namespace MissionText
{
	/**
	 * Text with each {Action} replaced by the key the player has bound to it, in brackets ({Interact} is "[E]"), and {Move}
	 * by the four movement keys. Without a player to ask (outside a game) each action shows its own name: "[Interact]".
	 * Shared by the tutorial's prompt and the missions' objectives, so both name the same keys.
	 */
	FString ResolveKeys(const UWorld* World, const FString& Text);
}
