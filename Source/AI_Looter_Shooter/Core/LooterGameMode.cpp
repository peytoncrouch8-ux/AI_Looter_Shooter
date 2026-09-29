#include "Core/LooterGameMode.h"
#include "Core/LooterPlayerController.h"
#include "UI/HUD/LooterHUD.h"
#include "UObject/ConstructorHelpers.h"

ALooterGameMode::ALooterGameMode()
{
	// The character's meshes, animation, camera and component settings are data in its Blueprint.
	static ConstructorHelpers::FClassFinder<APawn> PlayerCharacter(TEXT("/Game/Player/BP_LooterCharacter"));
	if (PlayerCharacter.Succeeded())
	{
		DefaultPawnClass = PlayerCharacter.Class;
	}
	PlayerControllerClass = ALooterPlayerController::StaticClass();
	HUDClass = ALooterHUD::StaticClass();
}
