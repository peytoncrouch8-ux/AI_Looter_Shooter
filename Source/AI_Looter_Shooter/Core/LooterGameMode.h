#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LooterGameMode.generated.h"

/**
 * The game's rules object: which character, controller and HUD a player gets. Set as the project's default game mode.
 * A level opened with ?Session=N plays that saved session (USessionSubsystem): it's loaded as the level starts and the
 * player and the world are put back once every actor has begun play. After a trip the player starts at its landing.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALooterGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;

	/**
	 * After a trip, the player start tagged with its landing (AreaLandings). Otherwise the level's own start, never a
	 * landing: the engine picks at random among the player starts it finds, so a level with a station or a jetty would
	 * sometimes start a new game there.
	 */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
