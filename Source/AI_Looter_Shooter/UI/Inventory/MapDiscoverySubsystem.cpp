#include "UI/Inventory/MapDiscoverySubsystem.h"
#include "Loot/Chest.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

bool UMapDiscoverySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UMapDiscoverySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SinceLook += DeltaTime;
	if (SinceLook >= LookInterval)
	{
		SinceLook = 0.f;
		LookAround();
	}
}

void UMapDiscoverySubsystem::LookAround()
{
	UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	if (!Player)
	{
		return;
	}
	const FVector Where = Player->GetActorLocation();
	for (TActorIterator<AChest> It(World); It; ++It)
	{
		const AChest* Chest = *It;
		// Across the ground: a cache on a ledge overhead counts as near as one beside the player. Only chests the map shows
		// are kept (a grave or a mailbox never gets a pin).
		if (IsValid(Chest) && Chest->ShowsOnMap() && FVector::DistSquared2D(Chest->GetActorLocation(), Where) <= FMath::Square(DiscoverRadius))
		{
			FoundChests.Add(Chest->GetSaveKey());
		}
	}
}
