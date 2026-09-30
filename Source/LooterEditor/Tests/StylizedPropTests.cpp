#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "StylizedProp.h"
#include "World/MinimapSubsystem.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UDynamicMesh.h"

namespace
{
	struct FMeshSummary
	{
		int32 Triangles = 0;
		int32 HighestSlot = -1;
	};

	FMeshSummary Summarize(const UDynamicMesh* Mesh)
	{
		FMeshSummary Summary;
		Mesh->ProcessMesh([&Summary](const UE::Geometry::FDynamicMesh3& Geometry)
		{
			Summary.Triangles = Geometry.TriangleCount();
			const UE::Geometry::FDynamicMeshMaterialAttribute* Slots = Geometry.HasAttributes() ? Geometry.Attributes()->GetMaterialID() : nullptr;
			for (const int32 Triangle : Geometry.TriangleIndicesItr())
			{
				Summary.HighestSlot = FMath::Max(Summary.HighestSlot, Slots ? Slots->GetValue(Triangle) : 0);
			}
		});
		return Summary;
	}

	TArray<EStylizedPropShape> AllShapes()
	{
		TArray<EStylizedPropShape> Shapes;
		const UEnum* Enum = StaticEnum<EStylizedPropShape>();
		for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index) // the last entry is the generated _MAX
		{
			Shapes.Add(static_cast<EStylizedPropShape>(Enum->GetValueByIndex(Index)));
		}
		return Shapes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStylizedPropGenerateTest, "Looter.Editor.StylizedProp.Generate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStylizedPropGenerateTest::RunTest(const FString& Parameters)
{
	// Baking relies on this: every shape makes a mesh, every material slot it uses gets painted, and the same seed
	// always makes the same mesh (the baker names meshes by their settings and reuses them).
	for (const EStylizedPropShape Shape : AllShapes())
	{
		const FString Name = UEnum::GetValueAsString(Shape);
		UDynamicMesh* First = NewObject<UDynamicMesh>();
		UDynamicMesh* Again = NewObject<UDynamicMesh>();
		const FStylizedPropLook Look = AStylizedProp::Generate(Shape, 7, FLinearColor::Gray, FLinearColor::Green, FTransform::Identity, nullptr, First);
		AStylizedProp::Generate(Shape, 7, FLinearColor::Gray, FLinearColor::Green, FTransform::Identity, nullptr, Again);
		const FMeshSummary Summary = Summarize(First);
		TestTrue(Name + TEXT(" has triangles"), Summary.Triangles > 0);
		TestTrue(Name + TEXT(" paints every slot it uses"), Summary.HighestSlot < Look.Surfaces.Num());
		TestEqual(Name + TEXT(" is the same for the same seed"), Summarize(Again).Triangles, Summary.Triangles);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStylizedPropGroundCoverTest, "Looter.Editor.StylizedProp.GroundCoverSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStylizedPropGroundCoverTest::RunTest(const FString& Parameters)
{
	// A ground cover patch is one rigid mesh laid on the slope under its middle: on ground that curves, its edges float
	// by more the wider it is. About 4 m across keeps them within a few centimeters on the island's rolling ground.
	for (const EStylizedPropShape Shape : { EStylizedPropShape::GrassPatch, EStylizedPropShape::TallGrass, EStylizedPropShape::FlowerPatch })
	{
		for (const int32 Seed : { 7919, 15838, 23757 })
		{
			UDynamicMesh* Mesh = NewObject<UDynamicMesh>();
			AStylizedProp::Generate(Shape, Seed, FLinearColor::Gray, FLinearColor::Green, FTransform::Identity, nullptr, Mesh);
			FBox Bounds(ForceInit);
			Mesh->ProcessMesh([&Bounds](const UE::Geometry::FDynamicMesh3& Geometry)
			{
				for (const int32 Vertex : Geometry.VertexIndicesItr())
				{
					Bounds += Geometry.GetVertex(Vertex);
				}
			});
			const FVector Size = Bounds.GetSize();
			TestTrue(FString::Printf(TEXT("%s (seed %d) is at most 4.2 m across (%.0f x %.0f cm)"), *UEnum::GetValueAsString(Shape), Seed, Size.X, Size.Y),
				Size.X <= 420.0 && Size.Y <= 420.0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStylizedPropMinimapTagsTest, "Looter.Editor.StylizedProp.MinimapTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStylizedPropMinimapTagsTest::RunTest(const FString& Parameters)
{
	// The minimap reads only tags, so a prop carries the one its baked actor will: ground, an obstacle standing on it, or
	// none for soft cover. Changing the shape swaps the tag.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AStylizedProp* Prop = WorldWrapper.GetTestWorld()->SpawnActor<AStylizedProp>();
	if (!TestNotNull(TEXT("Prop spawned"), Prop))
	{
		return false;
	}
	for (const EStylizedPropShape Shape : AllShapes())
	{
		const FString Name = UEnum::GetValueAsString(Shape);
		Prop->Configure(Shape, 1, FLinearColor::Gray, FLinearColor::Green);
		const bool bGround = AStylizedProp::IsGroundShape(Shape);
		TestEqual(Name + TEXT(" is ground"), Prop->ActorHasTag(MinimapTags::Ground), bGround);
		TestEqual(Name + TEXT(" is an obstacle"), Prop->ActorHasTag(MinimapTags::Obstacle), !bGround && !AStylizedProp::IsSoftShape(Shape));
	}
	return true;
}

#endif
