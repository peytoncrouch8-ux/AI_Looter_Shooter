#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ModelImporter.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelImportTest, "Looter.Editor.ModelImport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FModelImportTest::RunTest(const FString& Parameters)
{
	// Tests/ModelImport holds AxisTest.py exported by Tools/Blender/looter_export.py: a 2 x 1 x 0.5 m block with a bar on
	// its front (-Y), a bump on +X, a box hull and two sockets. It's imported under /Temp, so nothing is saved. It's
	// imported twice: the second time re-imports the mesh in place, with the same settings (Interchange once took over
	// re-imports, and the model came back turned 90 degrees with its hull in place of the mesh).
	for (const TCHAR* Pass : { TEXT("Import"), TEXT("Re-import") })
	{
		FModelImporter Importer(TEXT("/Temp/LooterTests"));
		TestEqual(FString::Printf(TEXT("%s: models imported"), Pass), Importer.ImportFolder(FPaths::ProjectDir() / TEXT("Source/LooterEditor/Tests/ModelImport")), 1);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Temp/LooterTests/ModelImport/SM_AxisTest.SM_AxisTest"));
		if (!TestNotNull(TEXT("Imported mesh"), Mesh))
		{
			return false;
		}
		TestNotNull(FString::Printf(TEXT("%s: the classic FBX importer did it"), Pass), Cast<UFbxStaticMeshImportData>(Mesh->GetAssetImportData()));

		// Blender's front (-Y) is the actor's forward (+X), Blender's +X is Unreal's -Y, 1 m is 100 cm, and the Blender
		// origin is the pivot.
		const FBox Bounds = Mesh->GetBoundingBox();
		TestTrue(FString::Printf(TEXT("%s: bounds (are %s)"), Pass, *Bounds.ToString()),
			Bounds.Min.Equals(FVector(-50.0, -120.0, 0.0), 0.1) && Bounds.Max.Equals(FVector(110.0, 100.0, 80.0), 0.1));

		// The hull is the collision (the same box, turned like the mesh), not the render mesh.
		const UBodySetup* Body = Mesh->GetBodySetup();
		if (TestNotNull(TEXT("Body setup"), Body) && TestEqual(TEXT("Hulls"), Body->AggGeom.ConvexElems.Num(), 1))
		{
			const FBox Hull = Body->AggGeom.ConvexElems[0].ElemBox;
			TestTrue(FString::Printf(TEXT("%s: hull box"), Pass), Hull.Min.Equals(FVector(-50.0, -100.0, 0.0), 0.1) && Hull.Max.Equals(FVector(50.0, 100.0, 50.0), 0.1));
			TestTrue(TEXT("Hull used for collision"), Body->CollisionTraceFlag == CTF_UseDefault);
		}

		// An unturned socket lines up with the model; one turned 90 degrees left in Blender has yaw -90.
		const UStaticMeshSocket* PlusX = Mesh->FindSocket(TEXT("PlusX"));
		const UStaticMeshSocket* TurnLeft = Mesh->FindSocket(TEXT("TurnLeft"));
		if (TestNotNull(TEXT("PlusX socket"), PlusX))
		{
			TestTrue(TEXT("PlusX location"), PlusX->RelativeLocation.Equals(FVector(0.0, -120.0, 25.0), 0.01));
			TestTrue(TEXT("PlusX rotation"), PlusX->RelativeRotation.Equals(FRotator::ZeroRotator, 0.01));
		}
		if (TestNotNull(TEXT("TurnLeft socket"), TurnLeft))
		{
			TestTrue(TEXT("TurnLeft location"), TurnLeft->RelativeLocation.Equals(FVector(0.0, 0.0, 50.0), 0.01));
			TestTrue(TEXT("TurnLeft rotation"), TurnLeft->RelativeRotation.Equals(FRotator(0.0, -90.0, 0.0), 0.01));
		}

		// Each slot is painted with the stylized instance named after its Blender material, in that material's color.
		TestTrue(TEXT("Nanite"), Mesh->NaniteSettings.bEnabled);
		TestEqual(FString::Printf(TEXT("%s: slots"), Pass), Mesh->GetStaticMaterials().Num(), 2);
		for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
		{
			const FString Expected = FString::Printf(TEXT("/Temp/LooterTests/Materials/MI_%s"), *Slot.MaterialSlotName.ToString());
			TestEqual(TEXT("Slot material"), Slot.MaterialInterface ? Slot.MaterialInterface->GetPackage()->GetName() : FString(), Expected);
		}
		const UMaterialInstanceConstant* Grey = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Temp/LooterTests/Materials/MI_AxisTestGrey.MI_AxisTestGrey"));
		FLinearColor Color;
		if (TestNotNull(TEXT("Grey material"), Grey) && TestTrue(TEXT("Grey has a color"), Grey->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Color")), Color)))
		{
			// 0x808080 in sRGB.
			TestTrue(TEXT("Grey color"), Color.Equals(FLinearColor(0.2159f, 0.2159f, 0.2159f, 1.f), 0.001f));
		}
	}

	// The test's assets are throwaway: keep them out of the editor's save prompt.
	for (TObjectIterator<UPackage> It; It; ++It)
	{
		if (It->GetName().StartsWith(TEXT("/Temp/LooterTests/")))
		{
			It->SetDirtyFlag(false);
		}
	}
	return true;
}

#endif
