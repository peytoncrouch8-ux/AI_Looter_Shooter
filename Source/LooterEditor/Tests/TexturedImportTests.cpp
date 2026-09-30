#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ModelImporter.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTexturedImportTest, "Looter.Editor.TexturedImport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTexturedImportTest::RunTest(const FString& Parameters)
{
	// Tests/TexturedImport holds TexturedTest.py exported by looter_export.py: three boxes sharing a material of the
	// textured style (master World, the TexturedTest set beside it, a tint and a UV scale); LodBlock has Nanite off and a
	// 50% LOD; GhostBlock has no collision. Imported under /Temp twice (the second time finds the textures unchanged).
	for (const TCHAR* Pass : { TEXT("Import"), TEXT("Re-import") })
	{
		FModelImporter Importer(TEXT("/Temp/LooterTests"));
		TestEqual(FString::Printf(TEXT("%s: models imported"), Pass), Importer.ImportFolder(FPaths::ProjectDir() / TEXT("Source/LooterEditor/Tests/TexturedImport")), 3);

		// The instance: the World master, its maps, tint and UV scale.
		const UMaterialInstanceConstant* Look = LoadObject<UMaterialInstanceConstant>(nullptr, TEXT("/Temp/LooterTests/Materials/MI_TexturedTestLook.MI_TexturedTestLook"));
		if (!TestNotNull(TEXT("Textured material"), Look))
		{
			return false;
		}
		TestEqual(TEXT("Master"), Look->Parent ? Look->Parent->GetPathName() : FString(), FString(TEXT("/Game/Art/Materials/Masters/M_World.M_World")));
		auto Map = [Look](const TCHAR* Param) -> UTexture2D*
		{
			UTexture* Texture = nullptr;
			Look->GetTextureParameterValue(FHashedMaterialParameterInfo(Param), Texture, true);
			return Cast<UTexture2D>(Texture);
		};
		const UTexture2D* Color = Map(TEXT("BaseColorMap"));
		const UTexture2D* Normal = Map(TEXT("NormalMap"));
		const UTexture2D* Masks = Map(TEXT("ORMMap"));
		if (TestNotNull(TEXT("Base color map"), Color) && TestNotNull(TEXT("Normal map"), Normal) && TestNotNull(TEXT("ORM map"), Masks))
		{
			TestEqual(TEXT("Textures land in Textures/<set folder>"), Color->GetPathName(), FString(TEXT("/Temp/LooterTests/Textures/TexturedImport/T_TexturedTest_BC.T_TexturedTest_BC")));
			TestTrue(TEXT("Color map is sRGB"), Color->SRGB);
			TestEqual(TEXT("Normal map compression"), static_cast<int32>(Normal->CompressionSettings.GetValue()), static_cast<int32>(TC_Normalmap));
			TestFalse(TEXT("Normal map isn't sRGB"), Normal->SRGB);
			TestEqual(TEXT("ORM compression"), static_cast<int32>(Masks->CompressionSettings.GetValue()), static_cast<int32>(TC_Masks));
			TestFalse(TEXT("ORM isn't sRGB"), Masks->SRGB);
		}
		FLinearColor Tint;
		if (TestTrue(TEXT("Has a tint"), Look->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Tint")), Tint)))
		{
			// #FF8040 in sRGB.
			TestTrue(TEXT("Tint"), Tint.Equals(FLinearColor(1.f, 0.2159f, 0.0513f, 1.f), 0.001f));
		}
		float UVScale = 0.f;
		TestTrue(TEXT("UV scale"), Look->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("UVScale")), UVScale) && FMath::IsNearlyEqual(UVScale, 2.f));
		float Moss = 0.f;
		TestTrue(TEXT("Other scalar parameters by name"), Look->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("MossAmount")), Moss) && FMath::IsNearlyEqual(Moss, 0.5f));
		FLinearColor MossColor;
		TestTrue(TEXT("Other color parameters by name"), Look->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("MossColor")), MossColor) && MossColor.Equals(FLinearColor(1.f, 0.2159f, 0.0513f, 1.f), 0.001f));

		// LODs on a mesh without Nanite; no collision on the ghost.
		const UStaticMesh* Lod = LoadObject<UStaticMesh>(nullptr, TEXT("/Temp/LooterTests/TexturedImport/SM_LodBlock.SM_LodBlock"));
		if (TestNotNull(TEXT("LOD mesh"), Lod))
		{
			TestFalse(TEXT("LOD mesh has no Nanite"), Lod->GetNaniteSettings().bEnabled);
			TestEqual(TEXT("LOD count"), Lod->GetNumSourceModels(), 2);
			if (Lod->GetNumSourceModels() == 2)
			{
				TestEqual(TEXT("LOD1 share"), Lod->GetSourceModel(1).ReductionSettings.PercentTriangles, 0.5f, 0.001f);
				TestEqual(TEXT("LOD1 screen size"), Lod->GetSourceModel(1).ScreenSize.Default, 0.4f, 0.001f);
			}
		}
		const UStaticMesh* Ghost = LoadObject<UStaticMesh>(nullptr, TEXT("/Temp/LooterTests/TexturedImport/SM_GhostBlock.SM_GhostBlock"));
		const UBodySetup* Body = Ghost ? Ghost->GetBodySetup() : nullptr;
		if (TestNotNull(TEXT("Ghost body"), Body))
		{
			TestEqual(TEXT("Ghost has no shapes"), Body->AggGeom.GetElementCount(), 0);
			TestEqual(TEXT("Ghost doesn't collide"), static_cast<int32>(Body->CollisionTraceFlag.GetValue()), static_cast<int32>(CTF_UseSimpleAsComplex));
			TestEqual(TEXT("Ghost's profile"), Body->DefaultInstance.GetCollisionProfileName().ToString(), UCollisionProfile::NoCollision_ProfileName.ToString());
		}
	}

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
