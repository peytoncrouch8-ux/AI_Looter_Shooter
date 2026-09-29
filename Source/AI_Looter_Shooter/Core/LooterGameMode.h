#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LooterGameMode.generated.h"

/** The game's rules object: which character, controller and HUD a player gets. Set as the project's default game mode. */
UCLASS()
class AI_LOOTER_SHOOTER_API ALooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALooterGameMode();
};
