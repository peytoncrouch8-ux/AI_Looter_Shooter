#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/EncounterSpawner.h"
#include "World/PlayableArea.h"
#include "World/RespawnMarker.h"
#include "Dom/JsonObject.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRansomsRestInsideBoundaryTest, "Looter.World.RansomsRest.InsideBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRansomsRestInsideBoundaryTest::RunTest(const FString& Parameters)
{
	// Step 27's level check (Docs/Areas/RansomsRest.md): every respawn grave, every place something spawns (the player's
	// start, each encounter's spawner and its own spots) and every tour viewpoint lies inside the playable boundary, so
	// no one wakes, appears or is measured where the player can't be.
	if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
	{
		AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
		return true;
	}
	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
	const ULevel* Level = Map ? Map->PersistentLevel.Get() : nullptr;
	if (!TestNotNull(TEXT("Lvl_RansomsRest loads"), Level))
	{
		return false;
	}
	const APlayableArea* Area = nullptr;
	for (const AActor* Actor : Level->Actors)
	{
		Area = Area ? Area : Cast<APlayableArea>(Actor);
	}
	if (!TestNotNull(TEXT("It has its playable area (Tools/Unreal/build_area_bounds.py)"), Area))
	{
		return false;
	}

	int32 Checked = 0;
	auto CheckInside = [&](const FString& What, const FVector& Where)
	{
		++Checked;
		TestTrue(FString::Printf(TEXT("%s at (%.0f, %.0f) is inside the boundary"), *What, Where.X, Where.Y), Area->Contains(Where));
	};
	int32 Graves = 0;
	int32 Spawners = 0;
	for (const AActor* Actor : Level->Actors)
	{
		if (const ARespawnMarker* Grave = Cast<ARespawnMarker>(Actor))
		{
			++Graves;
			CheckInside(FString::Printf(TEXT("The respawn grave %s"), *Grave->GetActorNameOrLabel()), Grave->GetActorLocation());
		}
		else if (const APlayerStart* Start = Cast<APlayerStart>(Actor))
		{
			CheckInside(FString::Printf(TEXT("The player start %s"), *Start->GetActorNameOrLabel()), Start->GetActorLocation());
		}
		else if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor))
		{
			++Spawners;
			const FString Name = Spawner->GetActorNameOrLabel();
			CheckInside(FString::Printf(TEXT("The encounter %s"), *Name), Spawner->GetActorLocation());
			for (int32 Spot = 0; Spot < Spawner->SpawnPoints.Num(); ++Spot)
			{
				CheckInside(FString::Printf(TEXT("%s's spot %d"), *Name, Spot + 1),
					Spawner->GetActorTransform().TransformPosition(Spawner->SpawnPoints[Spot]));
			}
		}
	}
	TestTrue(TEXT("Its respawn graves are placed (the family plot's at least)"), Graves > 0);
	TestTrue(TEXT("Its encounters are placed"), Spawners > 0);

	// The tour's viewpoints (Art/Levels/RansomsRest/views.json: x and y in cm).
	FString Text;
	const FString ViewsFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Levels/RansomsRest/views.json"));
	TSharedPtr<FJsonObject> Views;
	if (TestTrue(TEXT("views.json reads"), FFileHelper::LoadFileToString(Text, *ViewsFile))
		&& TestTrue(TEXT("...as JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Views) && Views.IsValid()))
	{
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (TestTrue(TEXT("...with its views"), Views->TryGetArrayField(TEXT("views"), List) && List && List->Num() > 0))
		{
			for (const TSharedPtr<FJsonValue>& Each : *List)
			{
				const TSharedPtr<FJsonObject> View = Each->AsObject();
				if (View.IsValid())
				{
					CheckInside(FString::Printf(TEXT("The tour view %s"), *View->GetStringField(TEXT("name"))),
						FVector(View->GetNumberField(TEXT("x")), View->GetNumberField(TEXT("y")), 0.0));
				}
			}
		}
	}
	AddInfo(FString::Printf(TEXT("%d places checked against the boundary."), Checked));
	return true;
}

#endif
