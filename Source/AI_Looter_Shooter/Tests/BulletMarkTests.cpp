#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BulletSubsystem.h"
#include "Weapons/BulletMarks.h"
#include "Weapons/ImpactSurface.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** A wall 4 m across facing -X at Location, its material named for what it is. */
	UStaticMeshComponent* SpawnMarkWall(UWorld& World, const FVector& Location, FName MaterialName, EComponentMobility::Type Mobility)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		AActor* Actor = World.SpawnActor<AActor>();
		if (!Cube || !Actor)
		{
			return nullptr;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor);
		Mesh->SetMobility(Mobility);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetWorldScale3D(FVector(0.4f, 4.f, 4.f));
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetMaterial(0, UMaterialInstanceDynamic::Create(UMaterial::GetDefaultMaterial(MD_Surface), Actor, MaterialName));
		Actor->SetRootComponent(Mesh);
		Mesh->SetWorldLocation(Location);
		Mesh->RegisterComponent();
		return Mesh;
	}

	/** One instant shot from Start along +X, fired by Shooter (each volley its own shooter: one frame, one gun, one volley). */
	void FireAt(UBulletSubsystem& Bullets, const FVector& Start, AActor* Shooter)
	{
		FBulletShot Shot;
		Shot.Start = Start;
		Shot.VisualStart = Start;
		Shot.Direction = FVector::ForwardVector;
		Shot.Speed = 0.f;
		Shot.Range = 5000.f;
		Shot.Channel = ECC_Visibility;
		Shot.Shooter = Shooter;
		Bullets.Fire(Shot);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBulletMarkRulesTest, "Looter.BulletMarks.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBulletMarkRulesTest::RunTest(const FString& Parameters)
{
	// Every surface but water takes a mark of its own look; soft ground throws a wider one than a hole in timber.
	TestTrue(TEXT("Water takes no mark"), FBulletMarks::LookOf(EImpactMaterial::Water).Style < 0);
	TArray<int32> Styles;
	for (int32 Index = 0; Index < static_cast<int32>(EImpactMaterial::Count); ++Index)
	{
		const EImpactMaterial Material = static_cast<EImpactMaterial>(Index);
		if (Material != EImpactMaterial::Water)
		{
			const FBulletMarkLook Look = FBulletMarks::LookOf(Material);
			TestTrue(FString::Printf(TEXT("%s takes a mark"), ImpactSurface::Name(Material)), Look.Style >= 0 && Look.Style < 16 && Look.Radius > 2.f);
			TestFalse(FString::Printf(TEXT("%s's look is its own"), ImpactSurface::Name(Material)), Styles.Contains(Look.Style));
			Styles.Add(Look.Style);
		}
	}
	TestTrue(TEXT("Dirt's puff is wider than wood's hole"), FBulletMarks::LookOf(EImpactMaterial::Dirt).Radius > FBulletMarks::LookOf(EImpactMaterial::Wood).Radius);

	// Sized to the gun: a pellet's smaller than a rifle round's, the six-gun's heavy slug bigger.
	TestTrue(TEXT("Pellets are smaller"), FBulletMarks::GunScale(EWeaponKind::Shotgun) < FBulletMarks::GunScale(EWeaponKind::Rifle));
	TestTrue(TEXT("The revolver's are bigger"), FBulletMarks::GunScale(EWeaponKind::Revolver) > FBulletMarks::GunScale(EWeaponKind::Rifle));

	// The decal's colour carries the style for M_BulletMark (floor(R * StyleSlots): 32 since the element marks) and the seed.
	for (int32 Style = 0; Style < FBulletMarks::FirstElementStyle + 5; ++Style)
	{
		const FLinearColor Code = FBulletMarks::Code(Style, 0.42f);
		TestEqual(FString::Printf(TEXT("Style %d reads back"), Style), FMath::FloorToInt32(Code.R * FBulletMarks::StyleSlots), Style);
		TestTrue(TEXT("The seed rides along"), FMath::IsNearlyEqual(Code.G, 0.42f));
	}

	// An element round's mark: its element's own style (15-19) on any surface that takes a mark, never on water, none for None.
	TArray<int32> ElementStyles;
	for (const ELooterElement Element : { ELooterElement::Ember, ELooterElement::Thunder, ELooterElement::Rot, ELooterElement::GraveSalt,
		ELooterElement::SoulLight })
	{
		const FBulletMarkLook Look = FBulletMarks::ElementLookOf(Element, EImpactMaterial::Stone);
		TestTrue(FString::Printf(TEXT("Element %d has its mark"), static_cast<int32>(Element)),
			Look.Style >= FBulletMarks::FirstElementStyle && Look.Style < FBulletMarks::StyleSlots);
		TestFalse(TEXT("...its own"), ElementStyles.Contains(Look.Style));
		ElementStyles.Add(Look.Style);
		TestEqual(TEXT("...the same on dirt"), FBulletMarks::ElementLookOf(Element, EImpactMaterial::Dirt).Style, Look.Style);
		TestTrue(TEXT("...never smaller than the surface's"),
			FBulletMarks::ElementLookOf(Element, EImpactMaterial::Glass).Radius >= FBulletMarks::LookOf(EImpactMaterial::Glass).Radius);
		TestTrue(TEXT("...no mark on water"), FBulletMarks::ElementLookOf(Element, EImpactMaterial::Water).Style < 0);
	}
	TestTrue(TEXT("A plain round has no element mark"), FBulletMarks::ElementLookOf(ELooterElement::None, EImpactMaterial::Stone).Style < 0);

	// Nothing to mark without a component.
	const FHitResult Nothing;
	TestFalse(TEXT("No component, no mark"), FBulletMarks::CanMark(Nothing, EImpactMaterial::Stone));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBulletMarkPlacementTest, "Looter.BulletMarks.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBulletMarkPlacementTest::RunTest(const FString& Parameters)
{
	// A shot straight down into the ground: the decal projects down into it, round, where it struck.
	const FVector Point(100.f, 200.f, 300.f);
	const FBulletMarkPlacement Square = FBulletMarks::Place(Point, FVector::UpVector, FVector::DownVector, 6.f, 0.3f);
	const FRotationMatrix SquareAxes(Square.Rotation);
	TestTrue(TEXT("It projects into the surface"), SquareAxes.GetScaledAxis(EAxis::X).Equals(FVector::DownVector, 0.01));
	TestTrue(TEXT("Square on: no streak"), FMath::IsNearlyZero(Square.Grazing) && FMath::IsNearlyEqual(Square.Size.Y, Square.Size.Z));
	TestTrue(TEXT("Its size is the radius"), FMath::IsNearlyEqual(Square.Size.Y, 6.0));
	TestTrue(TEXT("Deep enough for uneven ground"), Square.Size.X >= 3.0);
	TestTrue(TEXT("Where it struck"), Square.Location.Equals(Point, 0.01));

	// A round that only grazed a wall gouges a streak along its way, a little down range.
	const FVector Wall(-1.f, 0.f, 0.f);
	const FVector Glancing = FVector(0.15f, 1.f, 0.f).GetSafeNormal();
	const FBulletMarkPlacement Graze = FBulletMarks::Place(Point, Wall, Glancing, 6.f, 0.3f);
	const FRotationMatrix GrazeAxes(Graze.Rotation);
	TestTrue(TEXT("Grazing"), Graze.Grazing > 0.9f);
	TestTrue(TEXT("It projects into the wall"), GrazeAxes.GetScaledAxis(EAxis::X).Equals(FVector(1.f, 0.f, 0.f), 0.01));
	TestTrue(TEXT("Its length runs with the shot"), GrazeAxes.GetScaledAxis(EAxis::Z).Equals(FVector(0.f, 1.f, 0.f), 0.05));
	TestTrue(TEXT("Longer than it is wide"), Graze.Size.Z > Graze.Size.Y * 1.8);
	TestTrue(TEXT("Shifted down range"), Graze.Location.Y > Point.Y);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBulletMarkSlotsTest, "Looter.BulletMarks.Slots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBulletMarkSlotsTest::RunTest(const FString& Parameters)
{
	// A fresh mark takes an ended one, else a new one under the cap, else the oldest.
	TestEqual(TEXT("The first is new"), FBulletMarks::PickSlot({}, {}, 0.0, 4), 0);
	TestEqual(TEXT("Under the cap: a new one"), FBulletMarks::PickSlot({ 30.0, 31.0 }, { 0.0, 1.0 }, 5.0, 4), 2);
	TestEqual(TEXT("An ended one first"), FBulletMarks::PickSlot({ 30.0, 4.0, 32.0 }, { 0.0, 1.0, 2.0 }, 5.0, 3), 1);
	TestEqual(TEXT("At the cap: the oldest"), FBulletMarks::PickSlot({ 32.0, 30.0, 31.0 }, { 2.0, 0.0, 1.0 }, 5.0, 3), 1);
	TestEqual(TEXT("Nothing to give with a cap of none"), FBulletMarks::PickSlot({}, {}, 0.0, 0), INDEX_NONE);

	// Past the soft cap the oldest still holding fades out early; fading or hurried ones are left to go.
	const TArray<double> Born = { 0.0, 1.0, 2.0, 3.0, 4.0 };
	const TArray<double> FadeAt = { 0.5, 21.0, 22.0, 23.0, 24.0 };
	const TArray<double> Ends = { 15.5, 36.0, 37.0, 38.0, 39.0 };
	TArray<bool> Hurried = { false, false, false, false, false };
	TestEqual(TEXT("Over the soft cap: the oldest holding"), FBulletMarks::PickHurry(Born, FadeAt, Ends, Hurried, 5.0, 3), 1);
	Hurried[1] = true;
	TestEqual(TEXT("Not one hurried already"), FBulletMarks::PickHurry(Born, FadeAt, Ends, Hurried, 5.0, 3), 2);
	TestEqual(TEXT("Within the soft cap: none"), FBulletMarks::PickHurry(Born, FadeAt, Ends, Hurried, 5.0, 5), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBulletMarkDecalsTest, "Looter.BulletMarks.Decals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBulletMarkDecalsTest::RunTest(const FString& Parameters)
{
	// Shots on walls in a test level leave decals from the pool: never on a thing that moves, a blast capped, the pool capped.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("Bullet system exists"), Bullets))
	{
		return false;
	}
	if (!LoadObject<UObject>(nullptr, FBulletMarks::MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		AddWarning(TEXT("M_BulletMark isn't made yet (Tools/Unreal/build_world_materials.py M_BulletMark): the decals aren't checked."));
		return true;
	}
	IConsoleVariable* MaxMarks = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.BulletMarks.Max"));
	const int32 OldMax = MaxMarks ? MaxMarks->GetInt() : 64;

	UStaticMeshComponent* Stone = SpawnMarkWall(*World, FVector(1000.f, 0.f, 0.f), TEXT("MI_StoneWall_Test"), EComponentMobility::Static);
	UStaticMeshComponent* Door = SpawnMarkWall(*World, FVector(1000.f, 1000.f, 0.f), TEXT("MI_WoodPlanks_Door"), EComponentMobility::Movable);
	UStaticMeshComponent* Pond = SpawnMarkWall(*World, FVector(1000.f, 2000.f, 0.f), TEXT("MI_Water_Test"), EComponentMobility::Static);
	if (!TestNotNull(TEXT("Walls made"), Stone) || !Door || !Pond)
	{
		return false;
	}
	FBulletMarks& Marks = Bullets->GetMarks();
	auto Shooter = [World]() { return World->SpawnActor<AActor>(); };

	FireAt(*Bullets, FVector(0.f, 0.f, 0.f), Shooter());
	TestEqual(TEXT("A shot on the stone wall leaves a mark"), Marks.NumLive(World->GetTimeSeconds()), 1);
	FireAt(*Bullets, FVector(0.f, 1000.f, 0.f), Shooter());
	TestEqual(TEXT("...not one on a door that moves"), Marks.NumLive(World->GetTimeSeconds()), 1);
	FireAt(*Bullets, FVector(0.f, 2000.f, 0.f), Shooter());
	TestEqual(TEXT("...nor on water"), Marks.NumLive(World->GetTimeSeconds()), 1);

	// A shotgun's blast: nine pellets, at most MarksPerVolley marks.
	AActor* Blast = Shooter();
	for (int32 Pellet = 0; Pellet < 9; ++Pellet)
	{
		FireAt(*Bullets, FVector(0.f, -60.f + Pellet * 15.f, -40.f + Pellet * 10.f), Blast);
	}
	TestEqual(TEXT("A blast is capped"), Marks.NumLive(World->GetTimeSeconds()), 1 + FBulletMarks::MarksPerVolley);

	// The pool never holds more than its cap: the oldest makes room.
	if (MaxMarks)
	{
		MaxMarks->Set(8);
		for (int32 Shot = 0; Shot < 12; ++Shot)
		{
			FireAt(*Bullets, FVector(0.f, -150.f + Shot * 25.f, 60.f), Shooter());
		}
		TestTrue(FString::Printf(TEXT("At most the cap showing (%d)"), Marks.NumLive(World->GetTimeSeconds())), Marks.NumLive(World->GetTimeSeconds()) <= 8);
		TestTrue(FString::Printf(TEXT("Decals reused, not piled up (%d)"), Marks.NumDecals()), Marks.NumDecals() <= 8);
		MaxMarks->Set(OldMax);
	}
	Marks.Clear();
	TestEqual(TEXT("Cleared"), Marks.NumLive(World->GetTimeSeconds()), 0);
	return true;
}

#endif
