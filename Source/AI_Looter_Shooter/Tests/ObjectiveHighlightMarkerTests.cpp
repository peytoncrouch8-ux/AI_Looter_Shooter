#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/ObjectiveHighlightSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "UI/World/ObjectiveMarkerActor.h"
#include "World/WantedPoster.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The objective highlight's marker over a paper (2026-10-28): a decal (the gate's wanted poster) or a sheet-thin mesh has
// nothing that shows the orange overlay past a few metres, so it gets a small marker over it (AObjectiveMarkerActor) for
// the highlight's whole radius; a thing with a solid mesh of its own wears the overlay and gets none.

using namespace MissionTestWorld;

namespace
{
	const FName MarkerCrateTag(TEXT("HLM_Crate"));

	/** An engine shape (Path) as a static mesh actor, Scale times its size: a crate, a board, a sheet. */
	AStaticMeshActor* SpawnShape(UWorld* World, const TCHAR* Path, const FVector& Center, const FVector& Scale, FName Tag = NAME_None)
	{
		UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, Path);
		AStaticMeshActor* Made = Shape ? World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FQuat::Identity, Center, Scale)) : nullptr;
		if (Made)
		{
			UStaticMeshComponent* Mesh = Made->GetStaticMeshComponent();
			Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Mesh->SetStaticMesh(Shape);
			if (!Tag.IsNone())
			{
				Made->Tags.Add(Tag);
			}
		}
		return Made;
	}

	/** A decal put on Owner (a paper pasted on it). */
	UDecalComponent* AddDecal(AActor& Owner)
	{
		UDecalComponent* Decal = NewObject<UDecalComponent>(&Owner, TEXT("Paper"));
		Decal->SetupAttachment(Owner.GetRootComponent());
		Decal->DecalSize = FVector(5.0, 20.0, 25.0);
		Decal->RegisterComponent();
		return Decal;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FObjectiveHighlightMarkerTest, "Looter.Missions.Highlight.Marker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FObjectiveHighlightMarkerTest::RunTest(const FString& Parameters)
{
	using Highlighter = UObjectiveHighlightSubsystem;
	// Close up it gives way to the paper; at reading distance and beyond it's whole.
	TestEqual(TEXT("Gone close up"), AObjectiveMarkerActor::NearFade(AObjectiveMarkerActor::GoneWithin - 1.f), 0.f);
	TestEqual(TEXT("Whole from a few metres"), AObjectiveMarkerActor::NearFade(AObjectiveMarkerActor::FullFrom + 1.f), 1.f);
	TestEqual(TEXT("Whole at the highlight's edge"), AObjectiveMarkerActor::NearFade(Highlighter::DefaultRadius), 1.f);
	TestTrue(TEXT("It pops in from nothing and settles"), AObjectiveMarkerActor::PopAt(0.f) == 0.f && AObjectiveMarkerActor::PopAt(10.f) == 1.f);

	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	Highlighter* Highlights = World->GetSubsystem<Highlighter>();
	// A wanted poster facing the player 8 m off (a decal and an Interact box, nothing else), a crate, a sheet (the engine's
	// plane, 30 cm and no thickness: a letter), and a board with a paper pasted on it.
	AWantedPoster* Poster = World->SpawnActor<AWantedPoster>(FVector(800.0, 0.0, 150.0), FRotator(0.0, 180.0, 0.0));
	AStaticMeshActor* Crate = SpawnShape(World, TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(600.0, 300.0, 50.0), FVector(0.5), MarkerCrateTag);
	AStaticMeshActor* Sheet = SpawnShape(World, TEXT("/Engine/BasicShapes/Plane.Plane"), FVector(0.0, 3000.0, 80.0), FVector(0.3));
	AStaticMeshActor* Board = SpawnShape(World, TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0.0, -3000.0, 100.0), FVector(0.1, 1.0, 1.0));
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Place = SpawnMarker(World, FVector(0.0, 0.0, -3000.0));
	if (!Runner || !Highlights || !Poster || !Poster->Decal || !Crate || !Sheet || !Board || !Player || !Place)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Poster->DispatchBeginPlay();
	AddDecal(*Board);

	TestTrue(TEXT("A wanted poster (a decal only) is marked"), Highlighter::NeedsMarker(*Poster));
	TestTrue(TEXT("A sheet-thin mesh (a letter) is marked"), Highlighter::NeedsMarker(*Sheet));
	TestFalse(TEXT("A crate (its own mesh wears the overlay) isn't"), Highlighter::NeedsMarker(*Crate));
	TestFalse(TEXT("A paper on a solid board isn't (the board wears it)"), Highlighter::NeedsMarker(*Board));
	TestFalse(TEXT("A bare place (nothing shown) isn't"), Highlighter::NeedsMarker(*Place));

	// Through the highlight: one step asks to use the poster and the crate.
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), TEXT("TestMarker"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionInteractObjective>(Mission, 0)->Target.ActorTag = AWantedPoster::WantedTag;
	AddObjective<UMissionInteractObjective>(Mission, 0)->Target.ActorTag = MarkerCrateTag;
	Runner->BeginForTesting({ Mission }, Campaign, Player);
	Runner->Update(0.f);
	Highlights->Refresh();
	Highlights->Advance(Highlighter::FadeInSeconds);

	const FObjectiveHighlight* PosterLit = Highlights->FindHighlight(Poster);
	const FObjectiveHighlight* CrateLit = Highlights->FindHighlight(Crate);
	AObjectiveMarkerActor* Marker = PosterLit ? PosterLit->Marker.Get() : nullptr;
	TestTrue(TEXT("The poster is lit"), PosterLit && PosterLit->bWanted);
	TestTrue(TEXT("...with a marker over it"), Marker && Marker->GetTarget() == Poster);
	TestTrue(TEXT("...whole at 8 m"), Marker && Marker->GetPresence() > 0.99f);
	TestTrue(TEXT("...standing over the paper's top"), Marker && Marker->GetActorLocation().Z > Poster->Decal->Bounds.GetBox().Max.Z);
	TestTrue(TEXT("The crate is lit"), CrateLit && CrateLit->bWanted);
	TestTrue(TEXT("...with no marker (its mesh wears the overlay)"), CrateLit && !CrateLit->Marker.IsValid());
	if (!Marker)
	{
		return false;
	}

	// Up to the poster to tear it: the mark gives way to the paper.
	Player->SetActorLocation(FVector(650.0, 0.0, 150.0));
	Highlights->Advance(Highlighter::RefreshInterval);
	Highlights->Advance(AObjectiveMarkerActor::NearFadeSeconds + 0.1f);
	TestTrue(TEXT("Close up it fades out"), Marker->GetPresence() < 0.01f);
	TestTrue(TEXT("...while the poster stays lit"), Highlights->FindHighlight(Poster) && Highlights->FindHighlight(Poster)->bWanted);

	// Off out of range: the poster goes dark and its marker back to the pool.
	Player->SetActorLocation(FVector(-2000.0, 0.0, 0.0));
	Highlights->Advance(Highlighter::RefreshInterval);
	Highlights->Advance(Highlighter::FadeOutSeconds);
	TestNull(TEXT("Out of range: dark"), Highlights->FindHighlight(Poster));
	TestTrue(TEXT("...its marker free again"), Marker->IsFree());

	// Back: the same marker is taken again.
	Player->SetActorLocation(FVector::ZeroVector);
	Highlights->Advance(Highlighter::RefreshInterval);
	PosterLit = Highlights->FindHighlight(Poster);
	TestTrue(TEXT("Lit again, with the pool's marker"), PosterLit && PosterLit->Marker.Get() == Marker);
	Highlights->ClearAll();
	TestTrue(TEXT("Cleared: the marker free"), Marker->IsFree());
	return true;
}

#endif
