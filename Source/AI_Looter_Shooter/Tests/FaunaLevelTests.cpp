#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Story/HobBird.h"
#include "World/FaunaCloth.h"
#include "World/FaunaDustDevils.h"
#include "World/FaunaFlock.h"
#include "World/FaunaSwarm.h"
#include "World/FaunaTumbleweeds.h"
#include "World/PlayableArea.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "World/MinimapSubsystem.h"

namespace
{
	/** No plain crow nearer one of Hob's perches than this (cm): never seen where the story's one-eyed crow is. */
	constexpr double HobClearance = 1500.0;

	/** At most this many birds, and insects out at once, in a level (the budget's share). */
	constexpr int32 MaxBirds = 120;
	constexpr int32 MaxInsects = 140;
	constexpr int32 MaxTumbleweeds = 3;

	/** A placed solid (a building, a rock) as the test sees it: its box, for "not inside it". */
	struct FSolid
	{
		FString Name;
		FBox Box;
	};

	/** Whether a ground spot is inside a solid's footprint (30 cm in from its sides) and under its top. */
	const FSolid* InsideSolid(const TArray<FSolid>& Solids, const FVector& Spot)
	{
		for (const FSolid& Solid : Solids)
		{
			const FBox& Box = Solid.Box;
			if (Spot.X > Box.Min.X + 30.0 && Spot.X < Box.Max.X - 30.0 && Spot.Y > Box.Min.Y + 30.0 && Spot.Y < Box.Max.Y - 30.0
				&& Spot.Z < Box.Max.Z - 50.0 && Spot.Z > Box.Min.Z)
			{
				return &Solid;
			}
		}
		return nullptr;
	}

	/** Checks one built level's fauna; returns false when the level isn't there to check. */
	bool CheckLevel(FAutomationTestBase& Test, const TCHAR* Package, const TCHAR* Object)
	{
		if (!FPackageName::DoesPackageExist(Package))
		{
			Test.AddInfo(FString::Printf(TEXT("%s isn't built: skipped."), Package));
			return false;
		}
		const UWorld* Map = LoadObject<UWorld>(nullptr, Object);
		const ULevel* Level = Map ? Map->PersistentLevel.Get() : nullptr;
		if (!Test.TestNotNull(FString::Printf(TEXT("%s loads"), Package), Level))
		{
			return false;
		}
		TArray<const AFaunaActor*> Fauna;
		TArray<FVector> HobPerches;
		TArray<FSolid> Solids;
		const APlayableArea* Area = nullptr;
		for (const AActor* Actor : Level->Actors)
		{
			if (const AFaunaActor* Each = Cast<AFaunaActor>(Actor))
			{
				Fauna.Add(Each);
			}
			else if (const AHobBird* Hob = Cast<AHobBird>(Actor))
			{
				for (const FHobPerch& Perch : Hob->Perches)
				{
					HobPerches.Add(Perch.Location);
				}
			}
			else if (const APlayableArea* Playable = Cast<APlayableArea>(Actor))
			{
				Area = Playable;
			}
			else if (const AStaticMeshActor* Placed = Cast<AStaticMeshActor>(Actor))
			{
				// The level's buildings (tagged Obstacle as built; a rock's box is mostly air, so rocks aren't judged by it). A
				// level loaded for a test isn't registered, so its boxes come from each mesh's bounds and the root's saved
				// transform.
				const USceneComponent* Root = Placed->GetRootComponent();
				const UStaticMesh* Mesh = Placed->GetStaticMeshComponent() ? Placed->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
				static const TCHAR* const Buildings[] = { TEXT("Farmhouse"), TEXT("Barn"), TEXT("Cottage"), TEXT("LogCabin"),
					TEXT("FalseFront"), TEXT("Chapel"), TEXT("Depot"), TEXT("Outhouse"), TEXT("CoffinShed"), TEXT("WaterTower"),
					TEXT("Woodshed"), TEXT("Lookout") };
				bool bBuilding = false;
				for (const TCHAR* Kind : Buildings)
				{
					bBuilding |= Mesh && Mesh->GetName().Contains(Kind);
				}
				if (bBuilding && Placed->Tags.Contains(MinimapTags::Obstacle) && Root)
				{
					const FTransform Placement(Root->GetRelativeRotation(), Root->GetRelativeLocation(), Root->GetRelativeScale3D());
					const FBox Box = Mesh->GetBoundingBox().TransformBy(Placement);
					if (Box.IsValid && Box.GetSize().Z > 180.0)
					{
						Solids.Add({ Placed->GetActorNameOrLabel(), Box });
					}
				}
			}
		}
		if (Fauna.Num() == 0)
		{
			Test.AddInfo(FString::Printf(TEXT("%s has no fauna yet (Tools/Unreal/build_area_fauna.py): nothing to check."), Package));
			return true;
		}

		int32 Birds = 0;
		int32 Insects = 0;
		int32 Tumbleweeds = 0;
		int32 Checked = 0;
		auto CheckPlace = [&](const FString& What, const FVector& Where)
		{
			++Checked;
			if (Area)
			{
				Test.TestTrue(FString::Printf(TEXT("%s at (%.0f, %.0f) is inside the playable boundary"), *What, Where.X, Where.Y), Area->Contains(Where));
			}
		};
		for (const AFaunaActor* Actor : Fauna)
		{
			const FString Name = Actor->GetActorNameOrLabel();
			if (const AFaunaFlock* Flock = Cast<AFaunaFlock>(Actor))
			{
				Birds += Flock->BirdCount;
				if (Flock->Mode == EFaunaFlockMode::Aerial)
				{
					Test.TestTrue(FString::Printf(TEXT("%s has a sky to fly in"), *Name), Flock->AerialArea.Radius > 0.f
						&& Flock->AerialArea.MaxHeight >= Flock->AerialArea.MinHeight);
					continue;
				}
				Test.TestTrue(FString::Printf(TEXT("%s has a perch for each bird (%d birds, %d perches)"), *Name, Flock->BirdCount,
					Flock->Perches.Num()), Flock->BirdCount <= Flock->Perches.Num() && Flock->BirdCount > 0);
				for (int32 Index = 0; Index < Flock->Perches.Num(); ++Index)
				{
					const FFaunaPerch& Perch = Flock->Perches[Index];
					const FString What = FString::Printf(TEXT("%s's perch %d"), *Name, Index);
					CheckPlace(What, Perch.Location);
					for (const FVector& Hob : HobPerches)
					{
						Test.TestTrue(FString::Printf(TEXT("%s is %.0f m from one of Hob's perches (at least %.0f)"), *What,
							FVector::Dist(Perch.Location, Hob) / 100.0, HobClearance / 100.0), FVector::Dist(Perch.Location, Hob) >= HobClearance);
					}
					if (Perch.Kind == EFaunaPerchKind::Ground)
					{
						const FSolid* Inside = InsideSolid(Solids, Perch.Location);
						Test.TestNull(FString::Printf(TEXT("%s (on the ground) is inside %s"), *What, Inside ? *Inside->Name : TEXT("nothing")), Inside);
					}
				}
			}
			else if (const AFaunaSwarm* Swarm = Cast<AFaunaSwarm>(Actor))
			{
				Insects += Swarm->MaxActive;
				for (int32 Index = 0; Index < Swarm->Zones.Num(); ++Index)
				{
					CheckPlace(FString::Printf(TEXT("%s's zone %d"), *Name, Index), Swarm->Zones[Index].Center);
				}
			}
			else if (const AFaunaTumbleweeds* Rolling = Cast<AFaunaTumbleweeds>(Actor))
			{
				Tumbleweeds += Rolling->MaxActive;
				for (int32 Index = 0; Index < Rolling->Lanes.Num(); ++Index)
				{
					CheckPlace(FString::Printf(TEXT("%s's lane %d's start"), *Name, Index), Rolling->Lanes[Index].Start);
					const FSolid* Inside = InsideSolid(Solids, Rolling->Lanes[Index].Start);
					Test.TestNull(FString::Printf(TEXT("%s's lane %d starts inside %s"), *Name, Index, Inside ? *Inside->Name : TEXT("nothing")), Inside);
				}
			}
			else if (const AFaunaDustDevils* Devils = Cast<AFaunaDustDevils>(Actor))
			{
				for (int32 Index = 0; Index < Devils->Spots.Num(); ++Index)
				{
					CheckPlace(FString::Printf(TEXT("%s's spot %d"), *Name, Index), Devils->Spots[Index].Center);
				}
			}
			else if (const AFaunaCloth* Cloth = Cast<AFaunaCloth>(Actor))
			{
				for (const FFaunaClothPiece& Piece : Cloth->Pieces)
				{
					Test.TestTrue(FString::Printf(TEXT("%s's pieces name its meshes"), *Name), Cloth->Meshes.IsValidIndex(Piece.Mesh));
				}
			}
		}
		Test.TestTrue(FString::Printf(TEXT("%s has at most %d birds (%d)"), Package, MaxBirds, Birds), Birds <= MaxBirds);
		Test.TestTrue(FString::Printf(TEXT("%s has at most %d insects out at once (%d)"), Package, MaxInsects, Insects), Insects <= MaxInsects);
		Test.TestTrue(FString::Printf(TEXT("%s rolls at most %d tumbleweeds at once (%d)"), Package, MaxTumbleweeds, Tumbleweeds),
			Tumbleweeds <= MaxTumbleweeds);
		Test.AddInfo(FString::Printf(TEXT("%s: %d fauna actors, %d birds, %d places checked, %d of Hob's perches kept clear."), Package,
			Fauna.Num(), Birds, Checked, HobPerches.Num()));
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaLevelsTest, "Looter.World.Fauna.Levels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaLevelsTest::RunTest(const FString& Parameters)
{
	// Each built level's ambient life, as Tools/Unreal/build_area_fauna.py placed it: inside the playable boundary, no bird
	// near Hob's perches, nothing on the ground inside a building or rock, and the level's counts within the budget.
	CheckLevel(*this, TEXT("/Game/Maps/Lvl_RansomsRest"), TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
	CheckLevel(*this, TEXT("/Game/Maps/Lvl_TutorialIsland"), TEXT("/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland"));
	return true;
}

#endif
