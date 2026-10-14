#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/LumenScatterSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** An instanced part of the mesh on a new actor, with one instance. */
	UInstancedStaticMeshComponent* LumenScatterPart(UWorld& World, UStaticMesh* Mesh)
	{
		AActor* Holder = World.SpawnActor<AActor>();
		if (!Holder)
		{
			return nullptr;
		}
		UInstancedStaticMeshComponent* Part = NewObject<UInstancedStaticMeshComponent>(Holder);
		Part->SetStaticMesh(Mesh);
		Holder->SetRootComponent(Part);
		Part->RegisterComponent();
		Holder->AddInstanceComponent(Part);
		Part->AddInstance(FTransform::Identity);
		return Part;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLumenScatterSmallLeftOutTest, "Looter.World.LumenScatter.SmallLeftOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLumenScatterSmallLeftOutTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("World and the engine's cube"), World ? Cube : nullptr))
	{
		return false;
	}

	// A metre cube (radius 87 cm) is small scatter: left out of Lumen and the distance fields, once.
	UInstancedStaticMeshComponent* Pebble = LumenScatterPart(*World, Cube);
	if (!TestNotNull(TEXT("Small part made"), Pebble))
	{
		return false;
	}
	TestTrue(TEXT("Starts in the distance fields"), Pebble->bAffectDistanceFieldLighting != 0);
	TestTrue(TEXT("Starts in Lumen"), Pebble->bAffectDynamicIndirectLighting != 0);
	TestTrue(TEXT("A metre cube is small scatter"), ULumenScatterSubsystem::IsSmallScatter(*Pebble));
	TestEqual(TEXT("One part changed"), ULumenScatterSubsystem::LeaveOutOf(*Pebble->GetOwner()), 1);
	TestFalse(TEXT("Out of the distance fields"), Pebble->bAffectDistanceFieldLighting != 0);
	TestFalse(TEXT("Out of Lumen"), Pebble->bAffectDynamicIndirectLighting != 0);
	TestEqual(TEXT("Asking again changes nothing"), ULumenScatterSubsystem::LeaveOutOf(*Pebble->GetOwner()), 0);

	// Its instances' scale doesn't make it big: the mesh's own size decides.
	Pebble->UpdateInstanceTransform(0, FTransform(FQuat::Identity, FVector::ZeroVector, FVector(10.f)), false, true);
	TestTrue(TEXT("A stretched pebble is still a pebble"), ULumenScatterSubsystem::IsSmallScatter(*Pebble));

	// A big mesh (the engine's sky sphere) stays in.
	if (UStaticMesh* Big = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineSky/SM_SkySphere.SM_SkySphere")))
	{
		UInstancedStaticMeshComponent* Boulder = LumenScatterPart(*World, Big);
		if (TestNotNull(TEXT("Big part made"), Boulder))
		{
			TestFalse(TEXT("A big mesh isn't small scatter"), ULumenScatterSubsystem::IsSmallScatter(*Boulder));
			TestEqual(TEXT("Nothing changed on it"), ULumenScatterSubsystem::LeaveOutOf(*Boulder->GetOwner()), 0);
			TestTrue(TEXT("Still in Lumen"), Boulder->bAffectDynamicIndirectLighting != 0);
		}
	}
	else
	{
		AddInfo(TEXT("The engine's sky sphere isn't there: the big-mesh half was skipped."));
	}

	// A part with no mesh isn't scatter at all.
	UInstancedStaticMeshComponent* Empty = LumenScatterPart(*World, nullptr);
	if (TestNotNull(TEXT("Empty part made"), Empty))
	{
		TestFalse(TEXT("No mesh, not scatter"), ULumenScatterSubsystem::IsSmallScatter(*Empty));
	}
	return true;
}

#endif
