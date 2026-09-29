#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SurfaceMaterials.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialShared.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStylizedMaterialUsageTest, "Looter.Editor.StylizedMaterials",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStylizedMaterialUsageTest::RunTest(const FString& Parameters)
{
	// The stylized materials must allow Nanite (except the additive glow, which Nanite can't draw) and instancing. A
	// missing flag makes a packaged game draw the default material, and in the editor every instance sets it on itself
	// and compiles shaders of its own.
	for (const TCHAR* Name : { TEXT("M_StylizedSurface"), TEXT("M_StylizedFoliage"), TEXT("M_StylizedGlow") })
	{
		const UMaterial* Material = LoadObject<UMaterial>(nullptr, *FString::Printf(TEXT("/Game/Environment/Materials/%s.%s"), Name, Name));
		if (!TestNotNull(Name, Material))
		{
			continue;
		}
		TestEqual(FString(Name) + TEXT(" allows Nanite"), Material->GetUsageByFlag(MATUSAGE_Nanite), IsOpaqueOrMaskedBlendMode(*Material));
		TestTrue(FString(Name) + TEXT(" allows instancing"), Material->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
	}

	// No instance of them sets usage flags of its own (Looter.FixStylizedMaterials clears them).
	TArray<FAssetData> Assets;
	FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByClass(
		UMaterialInstanceConstant::StaticClass()->GetClassPathName(), Assets);
	int32 Checked = 0;
	for (const FAssetData& Asset : Assets)
	{
		if (!Asset.PackageName.ToString().StartsWith(TEXT("/Game/")))
		{
			continue;
		}
		const UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Asset.GetAsset());
		if (SurfaceMaterials::IsStylized(Instance))
		{
			++Checked;
			TestEqual(Asset.AssetName.ToString() + TEXT(" usage overrides"), Instance->BasePropertyOverrides.bOverride_UsageFlags, 0u);
		}
	}
	TestTrue(TEXT("Found the stylized instances"), Checked > 0);
	return true;
}

#endif
