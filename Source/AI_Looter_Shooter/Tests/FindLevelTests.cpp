#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/EncounterSpawner.h"
#include "Loot/Chest.h"
#include "Story/SpeakerPoint.h"
#include "Story/StoryCharacter.h"
#include "World/FindRules.h"
#include "World/FindSpot.h"
#include "World/PlayableArea.h"
#include "World/ReadableProp.h"
#include "World/SwimWater.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Rendering/PositionVertexBuffer.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "StaticMeshCompiler.h"
#endif

// Ransom's Rest's finds as Tools/Unreal/build_area_finds.py placed them (Docs/Work/2026-10-28-Core/FINDS.md, lane C): the
// fourteen finds and the six Ledger pages, each on a source that exists, on the ground, inside the playable area, off the
// story's people; the tarn's lockbox under deep water, the bolt-hole's crawl gap; Ranger Post Three gone (the user,
// 2026-10-31: its chest and stone steps stood by the grave the player wakes in). The level is
// loaded to look at only (no physics), so heights come from the terrain's triangles and the hidden cubes' boxes. How the
// finds play is FindWorldTests.cpp's.

namespace
{
	constexpr int32 FindCount = 14;
	constexpr int32 LoreCount = 6;
	/** The bolt-hole's gap (cm): a crouched player (116 cm) passes, a standing one (163) doesn't. */
	constexpr double GapLeast = 120.0;
	constexpr double GapMost = 145.0;
	/** The tarn's lockbox top at least this far under the surface (cm). */
	constexpr double TarnCover = 40.0;
	/** No find this near a story character or speaker point (cm). */
	constexpr double StoryClear = 300.0;
	/** A spot stands at most this far over the terrain under it (cm): Coffin Rock's top, the Rimrock's, a ruin's floor. */
	constexpr double HighestOverGround = 1200.0;

	const FName TarnId(TEXT("Tarn"));
	const FName CeilingTag(TEXT("FindBoltHoleCeiling"));
	const FName FloorTag(TEXT("FindBoltHoleFloor"));
	const FName StepTag(TEXT("FindRangerStep"));
	const FName DeckTag(TEXT("FindRangerDeck"));
	const FName TarnWater(TEXT("larkspurTarn"));

	/** Where a loaded (not playing) level's actor stands: its root's saved place. */
	FVector PlacedAt(const AActor* Actor)
	{
		const USceneComponent* Root = Actor ? Actor->GetRootComponent() : nullptr;
		return Root ? Root->GetRelativeLocation() : FVector::ZeroVector;
	}

	/** A hidden engine cube's (100 cm, its pivot its middle) bottom and top (cm). */
	TPair<double, double> CubeSpan(const AActor* Cube)
	{
		const USceneComponent* Root = Cube->GetRootComponent();
		const double Half = 50.0 * (Root ? Root->GetRelativeScale3D().Z : 1.0);
		const double Middle = PlacedAt(Cube).Z;
		return TPair<double, double>(Middle - Half, Middle + Half);
	}

	bool InsidePolygon(const TArray<FVector2D>& Polygon, const FVector2D& Point)
	{
		bool bInside = false;
		for (int32 I = 0, J = Polygon.Num() - 1; I < Polygon.Num(); J = I++)
		{
			const FVector2D& A = Polygon[I];
			const FVector2D& B = Polygon[J];
			if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X)
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	/**
	 * The terrain's highest height under Point (cm), from the triangles its Ground tiles draw on Medium, which are their
	 * collision too (a terrain mesh's fallback keeps every triangle): unset when no tile lies under it.
	 */
	TOptional<double> TerrainUnder(const ULevel& Level, const FVector2D& Point)
	{
		const FName GroundTag(TEXT("Ground"));
		TOptional<double> Highest;
		for (const AActor* Actor : Level.Actors)
		{
			const AStaticMeshActor* Tile = Cast<AStaticMeshActor>(Actor);
			const UStaticMeshComponent* Mesh = Tile && Tile->ActorHasTag(GroundTag) ? Tile->GetStaticMeshComponent() : nullptr;
			UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
			if (!Asset)
			{
				continue;
			}
			const FTransform ToWorld = Mesh->GetRelativeTransform();
			const FBox Box = Asset->GetBoundingBox().TransformBy(ToWorld);
			if (Point.X < Box.Min.X || Point.X > Box.Max.X || Point.Y < Box.Min.Y || Point.Y > Box.Max.Y)
			{
				continue;
			}
#if WITH_EDITOR
			UStaticMesh* const Compiling[] = { Asset };
			FStaticMeshCompilingManager::Get().FinishCompilation(Compiling);
#endif
			const FStaticMeshRenderData* Render = Asset->GetRenderData();
			if (!Render || Render->LODResources.Num() == 0)
			{
				continue;
			}
			const FStaticMeshLODResources& Drawn = Render->LODResources[0];
			const FIndexArrayView Indices = Drawn.IndexBuffer.GetArrayView();
			const FPositionVertexBuffer& Positions = Drawn.VertexBuffers.PositionVertexBuffer;
			const FVector Start = ToWorld.InverseTransformPosition(FVector(Point, Box.Max.Z + 100.0));
			const FVector End = ToWorld.InverseTransformPosition(FVector(Point, Box.Min.Z - 100.0));
			for (int32 Index = 0; Index + 2 < Indices.Num(); Index += 3)
			{
				const FVector A(Positions.VertexPosition(Indices[Index]));
				const FVector B(Positions.VertexPosition(Indices[Index + 1]));
				const FVector C(Positions.VertexPosition(Indices[Index + 2]));
				if (Start.X < FMath::Min3(A.X, B.X, C.X) || Start.X > FMath::Max3(A.X, B.X, C.X)
					|| Start.Y < FMath::Min3(A.Y, B.Y, C.Y) || Start.Y > FMath::Max3(A.Y, B.Y, C.Y))
				{
					continue;
				}
				FVector Hit;
				FVector Normal;
				if (FMath::SegmentTriangleIntersection(Start, End, A, B, C, Hit, Normal))
				{
					const double Height = ToWorld.TransformPosition(Hit).Z;
					Highest = Highest.IsSet() ? FMath::Max(Highest.GetValue(), Height) : Height;
				}
			}
		}
		return Highest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFindsLevelTest, "Looter.Finds.Level",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFindsLevelTest::RunTest(const FString& Parameters)
{
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

	TArray<const AFindSpot*> Spots;
	TArray<FVector> People;
	TArray<const AEncounterSpawner*> Spawners;
	TArray<const AActor*> Ceilings, Floors, Steps, Decks;
	const APlayableArea* Area = nullptr;
	const ASwimWater* Tarn = nullptr;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const AFindSpot* Spot = Cast<AFindSpot>(Actor))
		{
			Spots.Add(Spot);
		}
		else if (Actor->IsA<AStoryCharacter>() || Actor->IsA<ASpeakerPoint>())
		{
			People.Add(PlacedAt(Actor));
		}
		else if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor))
		{
			Spawners.Add(Spawner);
		}
		else if (const APlayableArea* Playable = Cast<APlayableArea>(Actor))
		{
			Area = Area ? Area : Playable;
		}
		else if (const ASwimWater* Water = Cast<ASwimWater>(Actor))
		{
			Tarn = Water->WaterId == TarnWater ? Water : Tarn;
		}
		else if (Actor->IsA<AStaticMeshActor>())
		{
			if (Actor->ActorHasTag(CeilingTag)) { Ceilings.Add(Actor); }
			if (Actor->ActorHasTag(FloorTag)) { Floors.Add(Actor); }
			if (Actor->ActorHasTag(StepTag)) { Steps.Add(Actor); }
			if (Actor->ActorHasTag(DeckTag)) { Decks.Add(Actor); }
		}
	}
	TestNotNull(TEXT("The level has its playable area"), Area);

	// Fifteen finds and six Ledger pages, every id once, every find in one of the map's regions.
	TSet<FName> Ids;
	int32 Finds = 0;
	int32 Lore = 0;
	for (const AFindSpot* Spot : Spots)
	{
		const FString Name = Spot->FindId.ToString();
		TestFalse(FString::Printf(TEXT("%s has an id"), *Spot->GetName()), Spot->FindId.IsNone());
		bool bAgain = false;
		Ids.Add(Spot->FindId, &bAgain);
		TestFalse(FString::Printf(TEXT("%s is the only find with its id"), *Name), bAgain);
		const bool bCounts = FindRules::CountsInFinds(Spot->Kind);
		(bCounts ? Finds : Lore) += 1;
		if (bCounts)
		{
			TestTrue(FString::Printf(TEXT("%s's region (%s) is one of the map's rows"), *Name, *Spot->Region.ToString()),
				FindRules::RegionOrder().Contains(Spot->Region.ToString()));
		}

		// Its source: a chest for a cache, a hidden find or a grave; a readable sending its event for a page; a creature tag
		// an encounter in the level carries for an Elite.
		const AActor* Source = Spot->GetSourceActor();
		switch (Spot->Kind)
		{
		case EFindKind::Lore:
		{
			const AReadableProp* Readable = Cast<AReadableProp>(Source);
			if (TestNotNull(FString::Printf(TEXT("%s's source is a readable"), *Name), Readable))
			{
				TestEqual(FString::Printf(TEXT("%s hears its readable's event"), *Name), Spot->SourceEvent, Readable->GetReadEvent());
			}
			break;
		}
		case EFindKind::Elite:
		{
			TestFalse(FString::Printf(TEXT("%s has its creature tag"), *Name), Spot->SourceTag.IsNone());
			// On every creature of the spawner, or on one group's alone: the Tollman's Find_Tollman is his group's (FEncounterGroup::
			// Tags), so his two escorts' kills don't credit the find.
			const bool bCarried = Spawners.ContainsByPredicate([Spot](const AEncounterSpawner* Each)
			{
				return Each->CreatureTags.Contains(Spot->SourceTag) || Each->Groups.ContainsByPredicate([Spot](const FEncounterGroup& Group)
				{
					return Group.Count > 0 && Group.Tags.Contains(Spot->SourceTag);
				});
			});
			TestTrue(FString::Printf(TEXT("An encounter spawns creatures tagged %s (the FIGHTS lane's patrol)"), *Spot->SourceTag.ToString()), bCarried);
			break;
		}
		default:
		{
			const AChest* Chest = Cast<AChest>(Source);
			if (TestNotNull(FString::Printf(TEXT("%s's source is a chest"), *Name), Chest))
			{
				TestFalse(FString::Printf(TEXT("%s's chest has its id"), *Name), Chest->ChestId.IsNone());
				TestTrue(FString::Printf(TEXT("%s stands at its chest"), *Name), FVector::Dist(PlacedAt(Chest), PlacedAt(Spot)) < 50.0);
			}
			break;
		}
		}

		// On the ground (a find: the Ledger pages lie where the story put them, Providence's underground), inside the
		// playable area.
		const FVector At = PlacedAt(Spot);
		const TOptional<double> Ground = bCounts ? TerrainUnder(*Level, FVector2D(At)) : TOptional<double>();
		if (bCounts && TestTrue(FString::Printf(TEXT("%s at (%.0f, %.0f) has the terrain under it"), *Name, At.X, At.Y), Ground.IsSet()))
		{
			// The tarn's box lies under the water, under the terrain's surface there; the rest stand on or over it.
			const double Over = At.Z - Ground.GetValue();
			TestTrue(FString::Printf(TEXT("%s stands on its ground, not under it nor floating (%.0f cm over the terrain)"), *Name, Over),
				Over > -60.0 && Over < HighestOverGround);
		}
		if (Area)
		{
			TestTrue(FString::Printf(TEXT("%s at (%.0f, %.0f) is inside the playable area"), *Name, At.X, At.Y), Area->Contains(At));
		}
		if (bCounts)
		{
			for (const FVector& Person : People)
			{
				TestTrue(FString::Printf(TEXT("%s is %.0f cm or more from the story's people (one at %.0f cm)"), *Name, StoryClear,
					FVector2D::Distance(FVector2D(At), FVector2D(Person))), FVector2D::Distance(FVector2D(At), FVector2D(Person)) >= StoryClear);
			}
		}
	}
	TestEqual(TEXT("Fourteen finds"), Finds, FindCount);
	TestEqual(TEXT("Six Ledger pages, counted apart"), Lore, LoreCount);

	// The tarn's lockbox: on the bed, its top TarnCover under the surface, inside the deep zone (no creature follows there).
	const AFindSpot* const* TarnSpot = Spots.FindByPredicate([](const AFindSpot* Each) { return Each->FindId == TarnId; });
	const AChest* Lockbox = TarnSpot ? Cast<AChest>((*TarnSpot)->GetSourceActor()) : nullptr;
	if (TestNotNull(TEXT("The tarn's lockbox is placed"), Lockbox) && TestNotNull(TEXT("Larkspur Tarn's swim water is placed"), Tarn))
	{
		const UStaticMesh* Body = Lockbox->Body ? Lockbox->Body->GetStaticMesh() : nullptr;
		const double Top = PlacedAt(Lockbox).Z + (Body ? Body->GetBoundingBox().Max.Z : 70.0);
		const double Surface = PlacedAt(Tarn).Z;
		TestTrue(FString::Printf(TEXT("The lockbox's top is %.0f cm under the surface (at least %.0f)"), Surface - Top, TarnCover), Surface - Top >= TarnCover);
		TestTrue(TEXT("...inside the tarn's deep zone"), InsidePolygon(Tarn->DeepZone, FVector2D(PlacedAt(Lockbox))));
	}

	// The bolt-hole's crawl: the hidden ceiling GapLeast-GapMost over the hidden lane floor.
	if (TestEqual(TEXT("The bolt-hole has one hidden ceiling"), Ceilings.Num(), 1) && TestEqual(TEXT("...and one lane floor"), Floors.Num(), 1))
	{
		const double Gap = CubeSpan(Ceilings[0]).Key - CubeSpan(Floors[0]).Value;
		TestTrue(FString::Printf(TEXT("The bolt-hole's gap is %.0f cm (%.0f-%.0f)"), Gap, GapLeast, GapMost), Gap >= GapLeast && Gap <= GapMost);
	}

	// Ranger Post Three is gone, and none of its pieces stands (the user, 2026-10-31).
	TestEqual(TEXT("No Ranger Post steps"), Steps.Num(), 0);
	TestEqual(TEXT("...nor its deck"), Decks.Num(), 0);
	AddInfo(FString::Printf(TEXT("%d finds and %d Ledger pages checked."), Finds, Lore));
	return true;
}

#endif
