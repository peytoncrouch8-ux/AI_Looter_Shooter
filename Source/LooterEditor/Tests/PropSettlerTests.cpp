#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PropSettler.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** A 4 x 2 m base (corners and the middle of the long sides), 10 cm under the pivot. */
	TArray<FVector> SlabBase()
	{
		TArray<FVector> Base;
		for (const double X : { -200.0, 0.0, 200.0 })
		{
			for (const double Y : { -100.0, 100.0 })
			{
				Base.Add(FVector(X, Y, -10.0));
			}
		}
		return Base;
	}

	double TiltDegrees(const FTransform& Transform)
	{
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Transform.GetRotation().GetUpVector().Z, -1.0, 1.0)));
	}

	/** How far the base point highest above the ground sits above it (negative: under it). */
	double HighestAboveGround(const FTransform& Transform, TConstArrayView<FVector> Base, TFunctionRef<TOptional<double>(const FVector&)> Ground)
	{
		double Highest = -UE_BIG_NUMBER;
		for (const FVector& Local : Base)
		{
			const FVector Point = Transform.TransformPosition(Local);
			Highest = FMath::Max(Highest, Point.Z - Ground(Point).Get(0.0));
		}
		return Highest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPropSettlerSettleTest, "Looter.Editor.PropSettler.Settle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPropSettlerSettleTest::RunTest(const FString& Parameters)
{
	using namespace PropSettler;
	const TArray<FVector> Base = SlabBase();
	const FRules Leans{ 15.f, 3.f };
	const FRules StaysUpright{ 0.f, 3.f };

	// Flat ground: the prop keeps its yaw and its base ends up exactly Embed under the ground, from above or below.
	auto Flat = [](const FVector&) -> TOptional<double> { return 100.0; };
	for (const double StartZ : { 500.0, -300.0 })
	{
		const TOptional<FTransform> Seated = Settle(FTransform(FRotator(0.0, 30.0, 0.0), FVector(50.0, 60.0, StartZ)), Base, Leans, Flat);
		if (TestTrue(TEXT("Stands on flat ground"), Seated.IsSet()))
		{
			TestEqual(TEXT("Pivot height on flat ground"), Seated->GetLocation().Z, 100.0 + 10.0 - 3.0, 0.01);
			TestTrue(TEXT("Keeps its yaw"), Seated->GetRotation().Equals(FQuat(FRotator(0.0, 30.0, 0.0)), 1e-6));
		}
	}

	// A 10 degree slope: a low, wide prop leans with it, a tall one stays upright; either way nothing hovers.
	const double Slope = FMath::Tan(FMath::DegreesToRadians(10.0));
	auto Sloped = [Slope](const FVector& Point) -> TOptional<double> { return Point.X * Slope; };
	const TOptional<FTransform> Leaning = Settle(FTransform(FRotator(0.0, 25.0, 0.0), FVector(0.0, 0.0, 300.0)), Base, Leans, Sloped);
	const TOptional<FTransform> Upright = Settle(FTransform(FRotator(0.0, 25.0, 0.0), FVector(0.0, 0.0, 300.0)), Base, StaysUpright, Sloped);
	if (TestTrue(TEXT("Stands on the slope"), Leaning.IsSet() && Upright.IsSet()))
	{
		TestEqual(TEXT("Leans with the slope"), TiltDegrees(*Leaning), 10.0, 0.05);
		TestEqual(TEXT("Its base follows the slope, Embed under it"), HighestAboveGround(*Leaning, Base, Sloped), -3.0, 0.01);
		TestEqual(TEXT("A tall prop stays upright"), TiltDegrees(*Upright), 0.0, 1e-4);
		TestEqual(TEXT("...with its downhill edge Embed under the ground"), HighestAboveGround(*Upright, Base, Sloped), -3.0, 0.01);

		// Settling a settled prop changes nothing.
		const TOptional<FTransform> Again = Settle(*Leaning, Base, Leans, Sloped);
		TestTrue(TEXT("Settling again changes nothing"), Again.IsSet() && Again->Equals(*Leaning, 0.01));
	}

	// Steeper than a prop may lean: it leans as far as it may, and the rest of the slope buries its uphill side.
	const double Steep = FMath::Tan(FMath::DegreesToRadians(30.0));
	auto Cliffside = [Steep](const FVector& Point) -> TOptional<double> { return Point.Y * Steep; };
	const TOptional<FTransform> Capped = Settle(FTransform(FVector(0.0, 0.0, 300.0)), Base, Leans, Cliffside);
	if (TestTrue(TEXT("Stands on the steep slope"), Capped.IsSet()))
	{
		TestEqual(TEXT("Leans no further than its limit"), TiltDegrees(*Capped), 15.0, 0.05);
		TestEqual(TEXT("Nothing hovers on the steep slope"), HighestAboveGround(*Capped, Base, Cliffside), -3.0, 0.01);
	}

	// Half of it over nothing (a sky island, an edge): it isn't standing on the ground, so it's left alone.
	auto Edge = [](const FVector& Point) -> TOptional<double> { return Point.X > 0.0 ? TOptional<double>() : TOptional<double>(0.0); };
	TestFalse(TEXT("Not standing where the ground ends"), Settle(FTransform(FVector(0.0, 0.0, 50.0)), Base, Leans, Edge).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPropSettlerRulesTest, "Looter.Editor.PropSettler.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPropSettlerRulesTest::RunTest(const FString& Parameters)
{
	using namespace PropSettler;
	TestEqual(TEXT("A tree stays upright"), RulesFor(600.0, 350.0).MaxTiltDegrees, 0.f);
	TestEqual(TEXT("A pillar stays upright"), RulesFor(440.0, 200.0).MaxTiltDegrees, 0.f);
	TestTrue(TEXT("A wall leans with the slope"), RulesFor(220.0, 320.0).MaxTiltDegrees > 0.f);
	TestTrue(TEXT("Stepping stones lean with the slope"), RulesFor(25.0, 150.0).MaxTiltDegrees > 0.f);
	TestTrue(TEXT("Small things go a little under the ground"), RulesFor(25.0, 150.0).Embed >= 2.f);
	TestTrue(TEXT("...big things deeper, within reason"), RulesFor(600.0, 350.0).Embed > RulesFor(25.0, 150.0).Embed && RulesFor(1500.0, 500.0).Embed <= 12.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundMeshFallbackTest, "Looter.Editor.GroundMeshes.FullFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGroundMeshFallbackTest::RunTest(const FString& Parameters)
{
	// Medium and Low draw the Nanite fallback, and the collision everything is placed with is cooked from it: the
	// ground's fallback has to be the whole mesh, or props and grass float over (or sink into) what High and Epic draw.
	const IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	int32 Checked = 0;
	for (const TCHAR* Kind : { TEXT("IslandTerrain"), TEXT("Terrain"), TEXT("Hill"), TEXT("Cliff") })
	{
		TArray<FAssetData> Assets;
		Registry.GetAssetsByPath(FName(FString(TEXT("/Game/Environment/Props/")) + Kind), Assets);
		for (const FAssetData& Asset : Assets)
		{
			const UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset());
			if (!Mesh)
			{
				continue;
			}
			const FMeshNaniteSettings& Nanite = Mesh->GetNaniteSettings();
			TestTrue(FString::Printf(TEXT("%s keeps every triangle in its fallback"), *Mesh->GetName()),
				!Nanite.bEnabled || (Nanite.FallbackTarget == ENaniteFallbackTarget::PercentTriangles && Nanite.FallbackPercentTriangles >= 1.f));
			++Checked;
		}
	}
	TestTrue(TEXT("Found the ground meshes"), Checked >= 6);
	return true;
}

#endif
