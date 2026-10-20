#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's Barrow fight's pieces that aren't a move: her effects' pools and their stand-ins (UGravemotherFX), the
// art's paths (GravemotherAssets: each found once imported), and her poses over the spider's (the clod lifted in her grip).

#include "Combat/BulletSubsystem.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherClod.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFX.h"
#include "Tests/BossTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Object.h"

using namespace GravemotherTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherFXPoolsTest, "Looter.Creatures.Gravemother.FX.Pools",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherFXPoolsTest::RunTest(const FString& Parameters)
{
	// Her effects stay within their caps however much is thrown, all go in a few seconds, and until the art is imported
	// (or forced) the weapons' own effects stand in for them.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UGravemotherFX* Effects = UGravemotherFX::Get(World);
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("Her effects"), Effects) || !TestNotNull(TEXT("The weapons' effects"), Bullets))
	{
		return false;
	}
	Effects->bIgnoreViewers = true;
	FGraveDustBurst Burst;
	Burst.Count = UGravemotherFX::MaxCards * 2;
	const int32 WeaponsBefore = Bullets->GetEffects().NumParticles();
	Effects->Puffs(FVector::ZeroVector, Burst);
	if (GravemotherAssets::IsImported(GravemotherAssets::CardGraveDust))
	{
		TestEqual(TEXT("Dust past the cap: held at it"), Effects->NumCards(), UGravemotherFX::MaxCards);
	}
	else
	{
		TestTrue(TEXT("MI_FX_GraveDust not imported: the weapons' dust stands in"), Effects->NumCards() == 0
			&& Bullets->GetEffects().NumParticles() > WeaponsBefore);
		AddInfo(TEXT("MI_FX_GraveDust isn't imported yet: its stand-in was checked."));
	}
	Effects->Chunks(EGraveChunk::Debris, FVector::ZeroVector, FVector::UpVector, UGravemotherFX::MaxChunks * 2, 400.f);
	if (GravemotherAssets::IsImported(GravemotherAssets::DebrisPath(1)))
	{
		TestEqual(TEXT("Debris past the cap: held at it"), Effects->NumChunks(), UGravemotherFX::MaxChunks);
	}
	for (int32 Frame = 0; Frame < 100; ++Frame)
	{
		Effects->Advance(0.05f);
	}
	TestTrue(TEXT("Five seconds on, all gone"), Effects->NumCards() == 0 && Effects->NumChunks() == 0);

	// Forced to its stand-ins, it throws the weapons' effects instead.
	Effects->bForceFallback = true;
	const int32 Before = Bullets->GetEffects().NumParticles();
	FGraveDustBurst Small;
	Small.Count = 4;
	Effects->Puffs(FVector(0.0, 0.0, 100.0), Small);
	Effects->Sparks(FVector(0.0, 0.0, 100.0), FVector::UpVector, FVector(1.0, 0.0, -1.0), 4);
	TestTrue(TEXT("Its stand-ins: the weapons' puffs and sparks"), Effects->NumCards() == 0 && Bullets->GetEffects().NumParticles() > Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherAssetPathsTest, "Looter.Creatures.Gravemother.Assets.Paths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherAssetPathsTest::RunTest(const FString& Parameters)
{
	// Every asset the fight loads by path is a real one once imported (a wrong path would leave its stand-in for good).
	const TArray<FString> Paths = GravemotherAssets::AllPaths();
	TSet<FString> Unique(Paths);
	TestEqual(TEXT("No path twice"), Unique.Num(), Paths.Num());
	int32 Imported = 0;
	for (const FString& Path : Paths)
	{
		TestTrue(*FString::Printf(TEXT("%s is an object path"), *Path), Path.StartsWith(TEXT("/Game/")) && Path.Contains(TEXT(".")));
		if (!GravemotherAssets::IsImported(*Path))
		{
			continue;
		}
		++Imported;
		TestNotNull(*FString::Printf(TEXT("%s loads"), *Path), GravemotherAssets::LoadAny(*Path, UObject::StaticClass()));
	}
	AddInfo(FString::Printf(TEXT("%d of %d of the fight's assets are imported; the rest show their stand-ins."), Imported, Paths.Num()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherPoseGripTest, "Looter.Creatures.Gravemother.Pose.Grip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherPoseGripTest::RunTest(const FString& Parameters)
{
	// Her poses over the spider's: digging in, her front feet on the ground ahead of her; lifted, over her body with the clod
	// between them.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World, FVector(900.0, 0.0, 0.0), 3000.0);
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(1800.0, 0.0, 90.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	Mother->GetBoss()->StartFight(Player);
	Mother->AlertTo(Player);
	Mother->ReadyClod();
	const float TellAt = TickUntil(Mother, 4.f, [Mother]()
	{
		return Mother->GetMove() == EGravemotherMove::Clod && Mother->GetCreatureState() == ECreatureState::Attack;
	});
	if (!TestTrue(TEXT("Her clod's tell"), TellAt >= 0.f))
	{
		return false;
	}
	TickFor(Mother, Mother->Clods.TearAt - 0.05f);
	const float Feet = static_cast<float>(FeetOf(*Mother).Z);
	const float Scale = Mother->GetSizeScale();
	const FVector LeftDug = Mother->GetPosedFoot(0, -1.f);
	const FVector RightDug = Mother->GetPosedFoot(0, 1.f);
	TestTrue(*FString::Printf(TEXT("Digging in: her front feet down on the ground (%.0f, %.0f cm over her feet)"), LeftDug.Z - Feet, RightDug.Z - Feet),
		LeftDug.Z - Feet < 30.f * Scale && RightDug.Z - Feet < 30.f * Scale);
	TestTrue(TEXT("...ahead of her"), FVector::DotProduct(LeftDug - Mother->GetActorLocation(), Mother->GetActorForwardVector()) > 0.0);
	TickFor(Mother, Mother->Clods.LiftedAt - Mother->Clods.TearAt + 0.25f);
	const FVector Left = Mother->GetPosedFoot(0, -1.f);
	const FVector Right = Mother->GetPosedFoot(0, 1.f);
	const double Middle = Mother->GetActorLocation().Z;
	TestTrue(*FString::Printf(TEXT("Lifted: her front feet over her middle (%.0f, %.0f cm)"), Left.Z - Middle, Right.Z - Middle),
		Left.Z > Middle && Right.Z > Middle);
	const AGravemotherClod* Clod = Mother->GetHeldClod();
	if (TestNotNull(TEXT("The clod in her grip"), Clod))
	{
		const FVector Between = (Left + Right) * 0.5;
		TestTrue(*FString::Printf(TEXT("...between them (%.0f cm off, flat)"), FVector::Dist2D(Clod->GetActorLocation(), Between)),
			FVector::Dist2D(Clod->GetActorLocation(), Between) < 30.0 && Clod->GetActorLocation().Z > Between.Z);
	}
	return true;
}

#endif
