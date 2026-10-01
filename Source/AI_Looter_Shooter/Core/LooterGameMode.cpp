#include "Core/LooterGameMode.h"
#include "Core/LooterPlayerController.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Engine/GameInstance.h"
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

void ALooterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// Before any actor begins play, so the tutorial and the HUD find the session's progress.
	if (USessionSubsystem* Sessions = GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr)
	{
		Sessions->BeginPlayWorld(GetWorld(), Options);
	}
}

void ALooterGameMode::StartPlay()
{
	// Every actor begins play in here (the player's character too, given its starting weapons).
	Super::StartPlay();
	if (USessionSubsystem* Sessions = GetGameInstance() ? GetGameInstance()->GetSubsystem<USessionSubsystem>() : nullptr)
	{
		Sessions->RestorePlayWorld(GetWorld());
	}
}
