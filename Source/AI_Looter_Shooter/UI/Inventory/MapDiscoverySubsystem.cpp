#include "UI/Inventory/MapDiscoverySubsystem.h"
#include "Loot/Chest.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

bool UMapDiscoverySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (nobody walks there, so nothing is found by itself).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
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

TArray<FName> UMapDiscoverySubsystem::GetUnopenedFound() const
{
	TArray<FName> Unopened;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Unopened;
	}
	for (TActorIterator<AChest> It(World); It; ++It)
	{
		// HasGivenLoot is what the session's OpenedChests go by, so a chest is in exactly one of the two lists.
		const AChest* Chest = *It;
		if (IsValid(Chest) && Chest->ShowsOnMap() && !Chest->HasGivenLoot() && FoundChests.Contains(Chest->GetSaveKey()))
		{
			Unopened.AddUnique(Chest->GetSaveKey());
		}
	}
	return Unopened;
}

void UMapDiscoverySubsystem::RestoreFound(const TArray<FName>& ChestKeys)
{
	for (const FName Key : ChestKeys)
	{
		if (!Key.IsNone())
		{
			FoundChests.Add(Key);
		}
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
