#include "Core/LooterMenuGameMode.h"
#include "Core/LooterMenuPlayerController.h"
#include "Session/SessionSubsystem.h"
#include "UI/Menus/MainMenuHUD.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

ALooterMenuGameMode::ALooterMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = ALooterMenuPlayerController::StaticClass();
	HUDClass = AMainMenuHUD::StaticClass();
}

void ALooterMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	if (USessionSubsystem* Sessions = GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr)
	{
		Sessions->BeginMenuWorld(GetWorld());
	}
}

void ALooterMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// The default would spawn a pawn, or a spectator that flies around: the menu has neither.
}

bool ALooterMenuGameMode::IsMenuWorld(const UWorld* World)
{
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	return GameMode && GameMode->IsA<ALooterMenuGameMode>();
}
