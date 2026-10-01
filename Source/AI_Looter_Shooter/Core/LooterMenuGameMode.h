#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LooterMenuGameMode.generated.h"

/**
 * The main menu's rules: no character, a camera circling the island behind the menu (ALooterMenuPlayerController) and
 * the menu itself (AMainMenuHUD). The game starts with it: DefaultEngine.ini's LocalMapOptions add ?game=Menu (its
 * alias in GameModeClassAliases) to the default map, so the level itself is the menu's backdrop. Its creatures roam;
 * the tutorial waits for a player.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALooterMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALooterMenuGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** No character: the menu's camera is the player's view. */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** The world is the main menu's backdrop: no one plays in it. */
	static bool IsMenuWorld(const UWorld* World);
};
