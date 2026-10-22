#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/ImpactSurfaceMap.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** A cube 2 m across at Location (its own static material, named Name), static like the world's meshes. */
	UStaticMeshComponent* SpawnSurfaceCube(UWorld& World, const FVector& Location, FName Name, FName ActorTag = NAME_None)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		AActor* Actor = World.SpawnActor<AActor>();
		if (!Cube || !Actor)
		{
			return nullptr;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor);
		Mesh->SetMobility(EComponentMobility::Static);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetWorldScale3D(FVector(2.f));
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetMaterial(0, UMaterialInstanceDynamic::Create(UMaterial::GetDefaultMaterial(MD_Surface), Actor, Name));
		Actor->SetRootComponent(Mesh);
		Mesh->SetWorldLocation(Location);
		Mesh->RegisterComponent();
		if (!ActorTag.IsNone())
		{
			Actor->Tags.Add(ActorTag);
		}
		return Mesh;
	}

	/** A bullet's trace from From to To: complex, with the face and the physical material. */
	bool TraceSurface(UWorld& World, const FVector& From, const FVector& To, FHitResult& OutHit)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ImpactSurfaceTest), /*bTraceComplex*/ true);
		Params.bReturnPhysicalMaterial = true;
		Params.bReturnFaceIndex = true;
		return World.LineTraceSingleByChannel(OutHit, From, To, ECC_Visibility, Params);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImpactSurfaceWordsTest, "Looter.Impact.Surface.Words",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FImpactSurfaceWordsTest::RunTest(const FString& Parameters)
{
	// The game's own material names (/Game/Art/Materials) say what they show.
	const TPair<const TCHAR*, EImpactMaterial> Names[] = {
		{ TEXT("MI_WoodPlanks"), EImpactMaterial::Wood },
		{ TEXT("MI_WoodEndGrain"), EImpactMaterial::Wood },
		{ TEXT("MI_BarkPine"), EImpactMaterial::Wood },
		{ TEXT("MI_HouseTrim"), EImpactMaterial::Wood },
		{ TEXT("MI_PaintCream"), EImpactMaterial::Wood },
		{ TEXT("MI_Charcoal"), EImpactMaterial::Wood },
		{ TEXT("MI_RockCliff_Ransom"), EImpactMaterial::Stone },
		{ TEXT("MI_RockGranite_Sink"), EImpactMaterial::Stone },
		{ TEXT("MI_StoneWall"), EImpactMaterial::Stone },
		{ TEXT("MI_ChestGranite"), EImpactMaterial::Stone },
		{ TEXT("MI_MetalRust"), EImpactMaterial::Metal },
		{ TEXT("MI_IronBlack"), EImpactMaterial::Metal },
		{ TEXT("MI_MetalWorn_Windmill"), EImpactMaterial::Metal },
		{ TEXT("MI_LanternIron"), EImpactMaterial::Metal },
		{ TEXT("MI_ChestSafeSteel"), EImpactMaterial::Metal },
		{ TEXT("MI_MailboxTin"), EImpactMaterial::SheetMetal },
		{ TEXT("MI_ScreenMesh"), EImpactMaterial::SheetMetal },
		{ TEXT("MI_WindowGlow"), EImpactMaterial::Glass },
		{ TEXT("MI_WindowGlow_Dark"), EImpactMaterial::Glass },
		{ TEXT("M_Glass"), EImpactMaterial::Glass },
		{ TEXT("MI_Water"), EImpactMaterial::Water },
		{ TEXT("MI_Waterfall"), EImpactMaterial::Water },
		{ TEXT("MI_Hay"), EImpactMaterial::Hay },
		{ TEXT("MI_Canvas"), EImpactMaterial::Cloth },
		{ TEXT("MI_CanvasDarkPatch"), EImpactMaterial::Cloth },
		{ TEXT("MI_ClothLinen"), EImpactMaterial::Cloth },
		{ TEXT("MI_RopeHemp"), EImpactMaterial::Cloth },
		{ TEXT("MI_MourningCrepe"), EImpactMaterial::Cloth },
		{ TEXT("MI_BoneDry"), EImpactMaterial::Bone },
		{ TEXT("MI_Ballast"), EImpactMaterial::Gravel },
		{ TEXT("MI_GroundDirt"), EImpactMaterial::Dirt },
		{ TEXT("MI_DenFloor"), EImpactMaterial::Dirt },
		{ TEXT("MI_SkyIslandGrass"), EImpactMaterial::Grass },
		{ TEXT("MI_FoliageJuniper"), EImpactMaterial::Grass },
	};
	for (const TPair<const TCHAR*, EImpactMaterial>& Name : Names)
	{
		EImpactMaterial Material = EImpactMaterial::Unknown;
		TestTrue(FString::Printf(TEXT("%s says a surface"), Name.Key), ImpactSurface::FromName(Name.Key, Material));
		TestEqual(FString::Printf(TEXT("%s is %s"), Name.Key, ImpactSurface::Name(Name.Value)), FString(ImpactSurface::Name(Material)),
			FString(ImpactSurface::Name(Name.Value)));
	}
	// Words are whole PascalCase words: "Tin" isn't in "Tinted", nor "Sand" in "Island"; a terrain's says nothing (its map does).
	for (const TCHAR* Silent : { TEXT("MI_Tinted"), TEXT("MI_TutorialIslandMacro"), TEXT("MI_RansomsRestMacroSW_Steep"), TEXT("M_World"),
		TEXT("MI_ChestOlive"), TEXT("DefaultPhysicalMaterial") })
	{
		EImpactMaterial Material = EImpactMaterial::Unknown;
		TestFalse(FString::Printf(TEXT("%s says nothing"), Silent), ImpactSurface::FromName(Silent, Material));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImpactSurfaceTagsTest, "Looter.Impact.Surface.Tags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FImpactSurfaceTagsTest::RunTest(const FString& Parameters)
{
	// A Surface tag says it outright: the surface's own name in any case, or any word that names one.
	EImpactMaterial Material = EImpactMaterial::Unknown;
	TestTrue(TEXT("Surface.Wood"), ImpactSurface::FromTag(TEXT("Surface.Wood"), Material) && Material == EImpactMaterial::Wood);
	TestTrue(TEXT("Surface.gravel, any case"), ImpactSurface::FromTag(TEXT("Surface.gravel"), Material) && Material == EImpactMaterial::Gravel);
	TestTrue(TEXT("Surface.SheetMetal"), ImpactSurface::FromTag(TEXT("Surface.SheetMetal"), Material) && Material == EImpactMaterial::SheetMetal);
	TestTrue(TEXT("Surface.Tin, a word for one"), ImpactSurface::FromTag(TEXT("Surface.Tin"), Material) && Material == EImpactMaterial::SheetMetal);
	TestTrue(TEXT("Surface.Ballast"), ImpactSurface::FromTag(TEXT("Surface.Ballast"), Material) && Material == EImpactMaterial::Gravel);
	TestFalse(TEXT("Another tag says nothing"), ImpactSurface::FromTag(TEXT("Ground"), Material));
	TestFalse(TEXT("A Surface tag naming nothing says nothing"), ImpactSurface::FromTag(TEXT("Surface.Nonsense"), Material));
	TestFalse(TEXT("An empty Surface tag says nothing"), ImpactSurface::FromTag(TEXT("Surface."), Material));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImpactSurfaceTrimTest, "Looter.Impact.Surface.Trim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FImpactSurfaceTrimTest::RunTest(const FString& Parameters)
{
	// The house trim sheet's strips, from its bottom up (looter_textures.py TRIM_STRIPS).
	const TPair<float, EImpactMaterial> Strips[] = {
		{ 0.01f, EImpactMaterial::Glass },       // H4 window glass
		{ 0.05f, EImpactMaterial::Metal },       // H3 iron strap
		{ 0.08f, EImpactMaterial::Wood },        // H2 red trim
		{ 0.11f, EImpactMaterial::Wood },        // H1 teal trim
		{ 0.15f, EImpactMaterial::Wood },        // G cream paint and whitewash (the picket fence's V)
		{ 0.3f, EImpactMaterial::SheetMetal },   // F corrugated tin
		{ 0.45f, EImpactMaterial::Wood },        // E shingles
		{ 0.55f, EImpactMaterial::Stone },       // D fieldstone
		{ 0.7f, EImpactMaterial::Wood },         // C beams
		{ 0.8f, EImpactMaterial::Wood },         // B logs
		{ 0.95f, EImpactMaterial::Wood },        // A siding
		{ 1.f, EImpactMaterial::Wood },
		{ -0.2f, EImpactMaterial::Glass },
	};
	for (const TPair<float, EImpactMaterial>& Strip : Strips)
	{
		TestEqual(FString::Printf(TEXT("The strip at V %.2f"), Strip.Key), FString(ImpactSurface::Name(ImpactSurface::FromHouseTrim(Strip.Key))),
			FString(ImpactSurface::Name(Strip.Value)));
	}
	TestFalse(TEXT("No material, no trim sheet"), ImpactSurface::IsTrimSheet(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImpactSurfaceGroundTest, "Looter.Impact.Surface.Ground",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FImpactSurfaceGroundTest::RunTest(const FString& Parameters)
{
	// The ground's surface from a macro cell, as M_Terrain lays its layers (linear colours, the master's own cues).
	const FLinearColor& Cues = UImpactSurfaceMapData::DefaultCues;
	const FLinearColor& Cues2 = UImpactSurfaceMapData::DefaultCues2;
	auto Ground = [&Cues, &Cues2](const FLinearColor& Macro, float Select, float Pebbles = 0.f)
	{
		FImpactGroundSample Sample;
		Sample.Macro = Macro;
		Sample.Select = Select;
		Sample.Pebbles = Pebbles;
		return FString(ImpactSurface::Name(UImpactSurfaceMapData::ClassifyGround(Sample, Cues, Cues2)));
	};
	const FLinearColor Meadow(0.1f, 0.16f, 0.04f);
	const FLinearColor Thatch(0.25f, 0.2f, 0.08f);
	const FLinearColor Soil(0.3f, 0.18f, 0.08f);
	const FLinearColor ForestFloor(0.04f, 0.045f, 0.03f);
	TestEqual(TEXT("Green meadow"), Ground(Meadow, 0.004f), FString(TEXT("Grass")));
	TestEqual(TEXT("Dry thatch is grass too"), Ground(Thatch, 0.004f), FString(TEXT("Grass")));
	TestEqual(TEXT("Warm bare soil"), Ground(Soil, 0.004f), FString(TEXT("Dirt")));
	TestEqual(TEXT("The dark ground under trees"), Ground(ForestFloor, 0.004f), FString(TEXT("Dirt")));
	TestEqual(TEXT("A dirt road through the meadow"), Ground(Meadow, 0.4f), FString(TEXT("Dirt")));
	TestEqual(TEXT("A footpath"), Ground(Meadow, 0.25f), FString(TEXT("Dirt")));
	TestEqual(TEXT("Scree"), Ground(Soil, 0.65f), FString(TEXT("Gravel")));
	TestEqual(TEXT("Rock"), Ground(Soil, 0.95f), FString(TEXT("Stone")));
	TestEqual(TEXT("The pebbles by a road"), Ground(Meadow, 0.004f, 0.8f), FString(TEXT("Gravel")));
	TestEqual(TEXT("A few pebbles in the grass"), Ground(Meadow, 0.004f, 0.2f), FString(TEXT("Grass")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImpactSurfaceMapTest, "Looter.Impact.Surface.Map",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FImpactSurfaceMapTest::RunTest(const FString& Parameters)
{
	// A 4 x 4 map over 4 m: rows run north (X), columns east (Y).
	UImpactSurfaceMapData* Map = NewObject<UImpactSurfaceMapData>(GetTransientPackage());
	Map->WorldMin = FVector2D(0.0, 0.0);
	Map->WorldMax = FVector2D(400.0, 400.0);
	Map->Size = 4;
	Map->Kinds.Init(static_cast<uint8>(EImpactMaterial::Grass), 16);
	Map->Kinds[0 * 4 + 1] = static_cast<uint8>(EImpactMaterial::Dirt);
	Map->Kinds[3 * 4 + 0] = static_cast<uint8>(EImpactMaterial::Stone);
	Map->Kinds[2 * 4 + 2] = static_cast<uint8>(EImpactMaterial::Unknown);
	EImpactMaterial Material = EImpactMaterial::Unknown;
	TestTrue(TEXT("The first row's second cell"), Map->KindAt(FVector2D(50.0, 150.0), Material) && Material == EImpactMaterial::Dirt);
	TestTrue(TEXT("The north row's first cell"), Map->KindAt(FVector2D(350.0, 50.0), Material) && Material == EImpactMaterial::Stone);
	TestTrue(TEXT("Elsewhere grass"), Map->KindAt(FVector2D(150.0, 350.0), Material) && Material == EImpactMaterial::Grass);
	TestFalse(TEXT("A cell that knows nothing"), Map->KindAt(FVector2D(250.0, 250.0), Material));
	TestFalse(TEXT("South of the map"), Map->KindAt(FVector2D(-1.0, 50.0), Material));
	TestFalse(TEXT("East of the map"), Map->KindAt(FVector2D(50.0, 400.0), Material));
	Map->Kinds.SetNum(15);
	TestFalse(TEXT("A broken map answers nothing"), Map->KindAt(FVector2D(50.0, 50.0), Material));
	Map->Kinds.SetNum(16);

#if WITH_EDITOR
	// A material carries it, and its child instances find their parent's (editor builds only: a constant instance's parent
	// is set through an editor-only call, and the packaged game's build compiles these tests too).
	// Constant instances, as the terrain's window and its steep child are (a dynamic instance made from a dynamic one
	// doesn't keep that one as its parent).
	UMaterialInstanceConstant* Window = NewObject<UMaterialInstanceConstant>(GetTransientPackage(), TEXT("MI_TestMacroSW"));
	Window->SetParentEditorOnly(UMaterial::GetDefaultMaterial(MD_Surface));
	UMaterialInstanceConstant* Steep = NewObject<UMaterialInstanceConstant>(GetTransientPackage(), TEXT("MI_TestMacroSW_Steep"));
	Steep->SetParentEditorOnly(Window);
	TestNull(TEXT("No map before one is added"), UImpactSurfaceMapData::FindOn(Window));
	Window->AddAssetUserData(Map);
	TestTrue(TEXT("The window carries it"), UImpactSurfaceMapData::FindOn(Window) == Map);
	TestTrue(TEXT("Its child finds it"), UImpactSurfaceMapData::FindOn(Steep) == Map);
	TestNull(TEXT("No material, no map"), UImpactSurfaceMapData::FindOn(nullptr));
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImpactSurfaceCuesTest, "Looter.Impact.Surface.Cues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FImpactSurfaceCuesTest::RunTest(const FString& Parameters)
{
	// Every surface has its own cue; until it has sounds, the old four play in its place.
	for (int32 Index = 0; Index < static_cast<int32>(EImpactMaterial::Count); ++Index)
	{
		const EImpactMaterial Material = static_cast<EImpactMaterial>(Index);
		TestFalse(FString::Printf(TEXT("%s has a cue"), ImpactSurface::Name(Material)), ImpactSurface::Cue(Material).IsNone());
		TestFalse(FString::Printf(TEXT("%s has a fallback"), ImpactSurface::Name(Material)), ImpactSurface::FallbackCue(Material).IsNone());
	}
	TestEqual(TEXT("Stone's own"), ImpactSurface::Cue(EImpactMaterial::Stone), FName(LooterSoundCue::Impact::Stone));
	TestEqual(TEXT("Glass's own"), ImpactSurface::Cue(EImpactMaterial::Glass), FName(LooterSoundCue::Impact::Glass));
	TestEqual(TEXT("Nothing known: the old thud"), ImpactSurface::Cue(EImpactMaterial::Unknown), FName(LooterSoundCue::ImpactWorld));
	TestEqual(TEXT("Wood keeps its cue"), ImpactSurface::Cue(EImpactMaterial::Wood), FName(LooterSoundCue::ImpactWood));
	TestEqual(TEXT("Sheet metal falls back to the metal ring"), ImpactSurface::FallbackCue(EImpactMaterial::SheetMetal), FName(LooterSoundCue::ImpactMetal));
	TestEqual(TEXT("Bone falls back to wood"), ImpactSurface::FallbackCue(EImpactMaterial::Bone), FName(LooterSoundCue::ImpactWood));
	TestEqual(TEXT("Dirt falls back to the old thud"), ImpactSurface::FallbackCue(EImpactMaterial::Dirt), FName(LooterSoundCue::ImpactWorld));
	TestEqual(TEXT("Water keeps its splash"), ImpactSurface::FallbackCue(EImpactMaterial::Water), FName(LooterSoundCue::ImpactWater));
	TestEqual(TEXT("With no sound system, a surface's own"), ImpactSurface::PlayableCue(nullptr, EImpactMaterial::Gravel),
		FName(LooterSoundCue::Impact::Gravel));
	TestTrue(TEXT("Stone ricochets"), ImpactSurface::Ricochets(EImpactMaterial::Stone));
	TestTrue(TEXT("Metal ricochets"), ImpactSurface::Ricochets(EImpactMaterial::Metal));
	TestFalse(TEXT("Wood doesn't"), ImpactSurface::Ricochets(EImpactMaterial::Wood));
	TestFalse(TEXT("Grass doesn't"), ImpactSurface::Ricochets(EImpactMaterial::Grass));

	// The bullets' own answer, as before for creatures and dummies.
	const FHitResult Nothing;
	TestTrue(TEXT("A creature's hit: its own sound"), UBulletSubsystem::ImpactCueOf(Nothing, EImpactSurface::Flesh).IsNone());
	TestEqual(TEXT("A dummy rings"), UBulletSubsystem::ImpactCueOf(Nothing, EImpactSurface::Target), FName(LooterSoundCue::ImpactMetal));
	TestEqual(TEXT("Nothing hit: the old thud"), UBulletSubsystem::ImpactCueOf(Nothing, EImpactSurface::World), FName(LooterSoundCue::ImpactWorld));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImpactSurfaceHitsTest, "Looter.Impact.Surface.Hits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FImpactSurfaceHitsTest::RunTest(const FString& Parameters)
{
	// Real traces against meshes in a test level, as a bullet's.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ImpactSurface::ResetCache();

	// A plank wall: its material's name.
	UStaticMeshComponent* Planks = SpawnSurfaceCube(*World, FVector(1000.f, 0.f, 0.f), TEXT("MI_WoodPlanks_Test"));
	// The same wall tagged as gravel: the tag wins.
	UStaticMeshComponent* Tagged = SpawnSurfaceCube(*World, FVector(1000.f, 1000.f, 0.f), TEXT("MI_WoodPlanks_Tagged"));
	// A block of terrain (tagged Ground, a macro material): grass on its flat top, rock on its sheer side.
	UStaticMeshComponent* Terrain = SpawnSurfaceCube(*World, FVector(1000.f, 2000.f, 0.f), TEXT("MI_TestMacro"), TEXT("Ground"));
	if (!TestNotNull(TEXT("Cubes made"), Planks) || !Tagged || !Terrain)
	{
		return false;
	}
	Tagged->ComponentTags.Add(TEXT("Surface.Gravel"));

	FHitResult Hit;
	EImpactSurfaceSource Source = EImpactSurfaceSource::None;
	if (TestTrue(TEXT("The plank wall is hit"), TraceSurface(*World, FVector(0.f, 0.f, 0.f), FVector(2000.f, 0.f, 0.f), Hit)))
	{
		TestEqual(TEXT("Planks are wood"), FString(ImpactSurface::Name(ImpactSurface::Resolve(Hit, Source))), FString(TEXT("Wood")));
		TestTrue(TEXT("...by the material's name"), Source == EImpactSurfaceSource::MaterialName);
		TestEqual(TEXT("...and sound it"), UBulletSubsystem::ImpactCueOf(Hit, EImpactSurface::World), FName(LooterSoundCue::ImpactWood));
	}
	if (TestTrue(TEXT("The tagged wall is hit"), TraceSurface(*World, FVector(0.f, 1000.f, 0.f), FVector(2000.f, 1000.f, 0.f), Hit)))
	{
		TestEqual(TEXT("The tag says gravel"), FString(ImpactSurface::Name(ImpactSurface::Resolve(Hit, Source))), FString(TEXT("Gravel")));
		TestTrue(TEXT("...by the tag"), Source == EImpactSurfaceSource::Tag);
	}
	if (TestTrue(TEXT("The terrain's top is hit"), TraceSurface(*World, FVector(1000.f, 2000.f, 1000.f), FVector(1000.f, 2000.f, -1000.f), Hit)))
	{
		TestEqual(TEXT("Flat terrain with no map is grass"), FString(ImpactSurface::Name(ImpactSurface::Resolve(Hit, Source))), FString(TEXT("Grass")));
		TestTrue(TEXT("...by its slope"), Source == EImpactSurfaceSource::TerrainSlope);
	}
	if (TestTrue(TEXT("The terrain's side is hit"), TraceSurface(*World, FVector(0.f, 2000.f, 0.f), FVector(2000.f, 2000.f, 0.f), Hit)))
	{
		TestEqual(TEXT("A sheer terrain face is rock"), FString(ImpactSurface::Name(ImpactSurface::Of(Hit))), FString(TEXT("Stone")));
	}

	// With a surface map on the terrain's material, its top is what the map says there.
	UImpactSurfaceMapData* Map = NewObject<UImpactSurfaceMapData>(GetTransientPackage());
	Map->WorldMin = FVector2D(0.0, 1000.0);
	Map->WorldMax = FVector2D(2000.0, 3000.0);
	Map->Size = 2;
	Map->Kinds.Init(static_cast<uint8>(EImpactMaterial::Dirt), 4);
	Cast<UMaterialInterface>(Terrain->GetMaterial(0))->AddAssetUserData(Map);
	ImpactSurface::ResetCache();
	if (TestTrue(TEXT("The terrain's top is hit again"), TraceSurface(*World, FVector(1000.f, 2000.f, 1000.f), FVector(1000.f, 2000.f, -1000.f), Hit)))
	{
		TestEqual(TEXT("The map says dirt"), FString(ImpactSurface::Name(ImpactSurface::Resolve(Hit, Source))), FString(TEXT("Dirt")));
		TestTrue(TEXT("...by the map"), Source == EImpactSurfaceSource::SurfaceMap);
	}
	if (TestTrue(TEXT("The terrain's side is hit again"), TraceSurface(*World, FVector(0.f, 2000.f, 0.f), FVector(2000.f, 2000.f, 0.f), Hit)))
	{
		TestEqual(TEXT("A sheer face is still rock"), FString(ImpactSurface::Name(ImpactSurface::Resolve(Hit, Source))), FString(TEXT("Stone")));
		TestTrue(TEXT("...for being steep"), Source == EImpactSurfaceSource::Steep);
	}
	ImpactSurface::ResetCache();
	return true;
}

#endif
