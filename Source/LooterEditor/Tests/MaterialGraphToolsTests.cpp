#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "LooterMaterialGraphTools.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialGraphClearTest, "Looter.Editor.MaterialGraph.Clear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMaterialGraphClearTest::RunTest(const FString& Parameters)
{
	// A master built again in place: every node goes and the material's inputs let go of them. A rooted node (one the
	// editor loaded as it started, when a class's defaults reached the material) is moved out instead of destroyed: the
	// engine's delete asserts on it, which once crashed the editor rebuilding M_Ghost.
	UMaterial* Material = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	UMaterialExpressionScalarParameter* Rooted = NewObject<UMaterialExpressionScalarParameter>(Material);
	Rooted->ParameterName = TEXT("Rooted");
	Material->GetExpressionCollection().AddExpression(Rooted);
	UMaterialExpressionConstant* Plain = NewObject<UMaterialExpressionConstant>(Material);
	Material->GetExpressionCollection().AddExpression(Plain);
	Material->GetExpressionInputForProperty(MP_Roughness)->Expression = Plain;
	Material->GetExpressionInputForProperty(MP_Metallic)->Expression = Rooted;
	Rooted->AddToRoot();

	int32 MovedOut = 0;
	const int32 Removed = ULooterMaterialGraphTools::ClearMaterialGraph(Material, MovedOut);
	TestEqual(TEXT("Both nodes removed"), Removed, 2);
	TestEqual(TEXT("The rooted node moved out"), MovedOut, 1);
	TestEqual(TEXT("The material has no nodes left"), Material->GetExpressions().Num(), 0);
	TestTrue(TEXT("Its roughness lets go of its node"), Material->GetExpressionInputForProperty(MP_Roughness)->Expression == nullptr);
	TestTrue(TEXT("Its metallic lets go of its node"), Material->GetExpressionInputForProperty(MP_Metallic)->Expression == nullptr);
	TestTrue(TEXT("The rooted node now lives in the transient package"), Rooted->GetOuter() == GetTransientPackage());
	TestFalse(TEXT("The plain node is destroyed"), IsValid(Plain));

	int32 Nothing = 0;
	TestEqual(TEXT("No material, nothing removed"), ULooterMaterialGraphTools::ClearMaterialGraph(nullptr, Nothing), 0);

	Rooted->RemoveFromRoot();
	Rooted->MarkAsGarbage();
	Material->MarkAsGarbage();
	return true;
}

#endif
