#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossComponent.h"
#include "Bosses/BossSeal.h"
#include "Bosses/BossTestSpider.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/BossTestWorld.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace BossTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossPelletsTest, "Looter.Bosses.Pellets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossPelletsTest::RunTest(const FString& Parameters)
{
	// A pellet's path against an upright capsule (34 cm round, 88 cm half height, at the origin), as the player's.
	float Time = 0.f;
	TestTrue(TEXT("Straight through: a hit"), UEnemyProjectileSubsystem::SweepHitsCapsule(FVector(-200.0, 0.0, 0.0), FVector(200.0, 0.0, 0.0), 12.f,
		FVector::ZeroVector, 34.f, 88.f, Time));
	TestNearlyEqual(TEXT("...closest halfway"), Time, 0.5f, 0.01f);
	TestTrue(TEXT("Grazing within both radii: a hit"), UEnemyProjectileSubsystem::SweepHitsCapsule(FVector(-200.0, 40.0, 0.0), FVector(200.0, 40.0, 0.0), 12.f,
		FVector::ZeroVector, 34.f, 88.f, Time));
	TestFalse(TEXT("Wide of it: a miss"), UEnemyProjectileSubsystem::SweepHitsCapsule(FVector(-200.0, 50.0, 0.0), FVector(200.0, 50.0, 0.0), 12.f,
		FVector::ZeroVector, 34.f, 88.f, Time));
	TestTrue(TEXT("Just over the head: a hit"), UEnemyProjectileSubsystem::SweepHitsCapsule(FVector(-200.0, 0.0, 95.0), FVector(200.0, 0.0, 95.0), 12.f,
		FVector::ZeroVector, 34.f, 88.f, Time));
	TestFalse(TEXT("Well over it: a miss"), UEnemyProjectileSubsystem::SweepHitsCapsule(FVector(-200.0, 0.0, 105.0), FVector(200.0, 0.0, 105.0), 12.f,
		FVector::ZeroVector, 34.f, 88.f, Time));
	TestFalse(TEXT("Short of it: a miss"), UEnemyProjectileSubsystem::SweepHitsCapsule(FVector(-300.0, 0.0, 0.0), FVector(-100.0, 0.0, 0.0), 12.f,
		FVector::ZeroVector, 34.f, 88.f, Time));

	// A volley's shape: one down the middle, the rest round the cone's edge.
	const TArray<FVector> Cone = UEnemyProjectileSubsystem::VolleyDirections(FVector(1.0, 0.0, 0.0), 5, 14.f);
	if (TestEqual(TEXT("Five pellets"), Cone.Num(), 5))
	{
		TestTrue(TEXT("The first straight down the middle"), Cone[0].Equals(FVector(1.0, 0.0, 0.0), 0.0001));
		for (int32 Index = 1; Index < Cone.Num(); ++Index)
		{
			TestNearlyEqual(TEXT("The rest on the cone's edge (7 degrees out)"), FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Cone[Index], Cone[0]))), 7.0, 0.01);
			TestFalse(TEXT("...each its own way"), Cone[Index].Equals(Cone[Index == 1 ? 2 : 1], 0.001));
		}
	}

	// Who a pellet hurts: the player, never its shooter, never a creature (so never the shooter's adds).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = SpawnPlayer(World, FVector(1000.0, 0.0, 0.0));
	UBossComponent* Boss = BossTestSpider::Spawn(World, FVector(0.0, 0.0, -100.0), 0.f);
	ACreatureBase* Add = ACreatureBase::SpawnAtRuntime(World, ASpiderCreature::StaticClass(), FVector(500.0, 0.0, -60.0), 180.f,
		ACreatureBase::FRuntimeSpawn());
	UEnemyProjectileSubsystem* Shots = World->GetSubsystem<UEnemyProjectileSubsystem>();
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Boss"), Boss) || !TestNotNull(TEXT("Add"), Add)
		|| !TestNotNull(TEXT("The level has enemy pellets"), Shots))
	{
		return false;
	}
	if (!Add->HasActorBegunPlay())
	{
		Add->DispatchBeginPlay();
	}
	AActor* Shooter = Boss->GetOwner();
	TestTrue(TEXT("The player can be hurt"), UEnemyProjectileSubsystem::CanHurt(Shooter, Player));
	TestFalse(TEXT("The shooter can't"), UEnemyProjectileSubsystem::CanHurt(Shooter, Shooter));
	TestFalse(TEXT("Its add can't"), UEnemyProjectileSubsystem::CanHurt(Shooter, Add));
	TestFalse(TEXT("No creature can"), UEnemyProjectileSubsystem::CanHurt(Add, Shooter));
	TestTrue(TEXT("A pellet whose shooter is gone still can hurt the player"), UEnemyProjectileSubsystem::CanHurt(nullptr, Player));

	// A pellet from the boss at the player, flying through the add on the way: it hits the player, once, and nothing else.
	TArray<AActor*> Hit;
	Shots->OnShotHit.AddLambda([&Hit](AActor* Victim, AActor* From, float Damage) { Hit.Add(Victim); });
	FEnemyShot Pellet;
	Pellet.Start = FVector(100.0, 0.0, 0.0);
	Pellet.Direction = FVector(1.0, 0.0, 0.0);
	Pellet.Speed = 1000.f;
	Pellet.Range = 3000.f;
	Pellet.Damage = 10.f;
	Pellet.Shooter = Shooter;
	Shots->Fire(Pellet);
	TestEqual(TEXT("One pellet in flight"), Shots->NumShotsInFlight(), 1);
	for (int32 Frame = 0; Frame < 40; ++Frame)
	{
		Shots->Tick(0.05f);
	}
	TestTrue(TEXT("It hit the player, once"), Hit.Num() == 1 && Hit[0] == Player);
	TestEqual(TEXT("...and is gone"), Shots->NumShotsInFlight(), 0);

	// Fired by the add at the boss, it touches nobody and flies out its range.
	Hit.Reset();
	FEnemyShot Stray = Pellet;
	Stray.Start = FVector(400.0, 0.0, 0.0);
	Stray.Direction = FVector(-1.0, 0.0, 0.0);
	Stray.Range = 600.f;
	Stray.Shooter = Add;
	Shots->Fire(Stray);
	for (int32 Frame = 0; Frame < 20; ++Frame)
	{
		Shots->Tick(0.05f);
	}
	TestEqual(TEXT("A creature's pellet through a creature hurts nobody"), Hit.Num(), 0);
	TestEqual(TEXT("...and is gone at its range"), Shots->NumShotsInFlight(), 0);

	// A volley, and a fight's reset putting a shooter's pellets out.
	Shots->FireVolley(Pellet, FVector(1.0, 0.0, 0.0), 5, 14.f);
	TestEqual(TEXT("A volley of five"), Shots->NumShotsFrom(Shooter), 5);
	Shots->ClearShotsFrom(Shooter);
	TestEqual(TEXT("Put out at once"), Shots->NumShotsInFlight(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossSealTest, "Looter.Bosses.Seal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossSealTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ABossSeal* Ring = ABossSeal::SpawnRing(World, FVector::ZeroVector, 1500.f);
	if (!TestNotNull(TEXT("A ring seal"), Ring))
	{
		return false;
	}
	TestFalse(TEXT("It starts down"), Ring->IsRaised());
	TestTrue(TEXT("6 m from its middle is inside"), Ring->IsInside(FVector(600.0, 0.0, 0.0), 150.f));
	TestFalse(TEXT("14 m out is too near the wall to close on"), Ring->IsInside(FVector(1400.0, 0.0, 0.0), 150.f));
	TestFalse(TEXT("16 m out is outside"), Ring->IsInside(FVector(1600.0, 0.0, 0.0)));

	Ring->Raise();
	TestTrue(TEXT("Raised"), Ring->IsRaised());
	const TArray<TObjectPtr<UBoxComponent>>& Walls = Ring->GetWalls();
	TestEqual(TEXT("A wall per span all round"), Walls.Num(), Ring->GetPath().Num());
	const ECollisionChannel Passing[] = { ECC_WorldStatic, ECC_WorldDynamic, ECC_Visibility, ECC_Camera, ECC_PhysicsBody, ECC_Vehicle,
		ECC_Destructible, ECC_GameTraceChannel1 /*Projectile*/, ECC_GameTraceChannel2 /*Weapon: bullets*/ };
	for (const UBoxComponent* Wall : Walls)
	{
		if (!TestNotNull(TEXT("Wall"), Wall))
		{
			return false;
		}
		TestTrue(TEXT("It stops walking pawns"), Wall->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
			&& Wall->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
		TestTrue(TEXT("It's world-dynamic: ground traces never land on it"), Wall->GetCollisionObjectType() == ECC_WorldDynamic);
		for (const ECollisionChannel Channel : Passing)
		{
			TestTrue(FString::Printf(TEXT("It lets channel %d through"), static_cast<int32>(Channel)),
				Wall->GetCollisionResponseToChannel(Channel) == ECR_Ignore);
		}
		TestTrue(TEXT("Nobody steps up onto it"), Wall->CanCharacterStepUpOn == ECB_No);
		TestTrue(TEXT("Only its light is seen"), Wall->bHiddenInGame != 0);
	}

	// A player-sized capsule walking out from its middle is stopped inside; once it drops, nothing stops it.
	auto Blocked = [&Walls](const FVector& From, const FVector& To, FHitResult& OutHit)
	{
		bool bStopped = false;
		OutHit = FHitResult();
		OutHit.Time = 1.f;
		for (UBoxComponent* Wall : Walls)
		{
			FHitResult Hit;
			if (Wall && Wall->SweepComponent(Hit, From, To, FQuat::Identity, FCollisionShape::MakeCapsule(34.f, 88.f)) && Hit.Time <= OutHit.Time)
			{
				OutHit = Hit;
				bStopped = true;
			}
		}
		return bStopped;
	};
	FHitResult Stop;
	for (const FVector& Out : { FVector(2500.0, 0.0, 100.0), FVector(0.0, -2500.0, 100.0), FVector(1800.0, 1800.0, 100.0) })
	{
		if (TestTrue(TEXT("Walking out: stopped"), Blocked(FVector(0.0, 0.0, 100.0), Out, Stop)))
		{
			TestTrue(TEXT("...still inside"), Ring->IsInside(Stop.Location));
		}
	}
	Ring->Drop();
	TestFalse(TEXT("Dropped"), Ring->IsRaised());
	TestFalse(TEXT("Dropped, it stops nobody"), Blocked(FVector(0.0, 0.0, 100.0), FVector(2500.0, 0.0, 100.0), Stop));

	// A gate: one wall across a gap, the arena ahead of it.
	ABossSeal* Gate = World->SpawnActor<ABossSeal>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A gate seal"), Gate))
	{
		Gate->Shape = EBossSealShape::Gate;
		Gate->GateEnd = FVector(0.0, 800.0, 0.0);
		Gate->Raise();
		TestEqual(TEXT("One wall across the gap"), Gate->GetWalls().Num(), 1);
		TestTrue(TEXT("Ahead of the gate is inside"), Gate->IsInside(FVector(300.0, 400.0, 0.0), 150.f));
		TestFalse(TEXT("Behind it is outside"), Gate->IsInside(FVector(-300.0, 400.0, 0.0)));

		// Where a player stands against the gate's line: the nearest point on it, flat, at its own height.
		FVector Nearest;
		const float Away = BossSealFog::NearestOnPath(Gate->GetPath(), Gate->IsClosed(), FVector(300.0, 400.0, 100.0), Nearest);
		TestNearlyEqual(TEXT("3 m ahead of the gate's middle: 3 m from the line"), Away, 300.f, 0.5f);
		TestTrue(TEXT("...straight across from it"), Nearest.Equals(FVector(0.0, 400.0, 0.0), 0.5));
		BossSealFog::NearestOnPath(Gate->GetPath(), Gate->IsClosed(), FVector(300.0, 1200.0, 0.0), Nearest);
		TestTrue(TEXT("Past the gate's end: the end itself"), Nearest.Equals(FVector(0.0, 800.0, 0.0), 0.5));
	}

	// The ring's: a point 5 m inside it is about 5 m from its wall (the wall is chords, a few cm in from the circle).
	FVector Nearest;
	const float Away = BossSealFog::NearestOnPath(Ring->GetPath(), Ring->IsClosed(), FVector(1000.0, 0.0, 100.0), Nearest);
	TestNearlyEqual(TEXT("5 m inside the ring: 5 m from its wall"), Away, 500.f, 10.f);
	TestTrue(TEXT("...at the wall"), Nearest.X > 1480.0 && FMath::Abs(Nearest.Y) < 100.0);

	// The fog's look as rules: thickest at the foot, thinning to a quarter at the top, and never thickening on the way up.
	TestNearlyEqual(TEXT("Fog: full strength at the foot"), BossSealFog::Density(0.f, 700.f), 1.f, 0.001f);
	TestNearlyEqual(TEXT("...a quarter at the top"), BossSealFog::Density(700.f, 700.f), 0.25f, 0.001f);
	bool bOnlyThins = true;
	float Before = 2.f;
	for (float Z = 0.f; Z <= 700.f; Z += 35.f)
	{
		const float Now = BossSealFog::Density(Z, 700.f);
		bOnlyThins &= Now <= Before + UE_KINDA_SMALL_NUMBER;
		Before = Now;
	}
	TestTrue(TEXT("...and only thins on the way up"), bOnlyThins);
	// The flare where the player touches the wall: full against it, none from four metres out, weaker the farther between.
	TestNearlyEqual(TEXT("Touching the wall: full flare"), BossSealFog::Flare(40.f), 1.f, 0.001f);
	TestTrue(TEXT("2 m off: some, less than 1 m"), BossSealFog::Flare(200.f) > 0.f && BossSealFog::Flare(200.f) < BossSealFog::Flare(100.f));
	TestNearlyEqual(TEXT("4 m off: none"), BossSealFog::Flare(400.f), 0.f, 0.001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossBarTest, "Looter.Bosses.Bar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossBarTest::RunTest(const FString& Parameters)
{
	// Ticks where each later phase starts, inside the bar, highest first; none at its ends or twice on one line.
	TestTrue(TEXT("Three phases: ticks at a half and a quarter"), UHudBossBarWidget::MakeTickShares({ 1.f, 0.5f, 0.25f }) == TArray<float>({ 0.5f, 0.25f }));
	TestTrue(TEXT("Written out of order: the same"), UHudBossBarWidget::MakeTickShares({ 1.f, 0.25f, 0.5f }) == TArray<float>({ 0.5f, 0.25f }));
	TestEqual(TEXT("One phase: no ticks"), UHudBossBarWidget::MakeTickShares({ 1.f }).Num(), 0);
	TestTrue(TEXT("Two on one line, and one at the empty end: one tick"), UHudBossBarWidget::MakeTickShares({ 1.f, 0.6f, 0.6f, 0.f }) == TArray<float>({ 0.6f }));

	// Its words: the level, the phase's name, or what to do while the boss can't be hurt.
	TestEqual(TEXT("The level"), UHudBossBarWidget::MakeLevelText(9).ToString(), FString(TEXT("LV 9")));
	const FText Silk = FText::FromString(TEXT("Under Silk"));
	const FText Brood = FText::FromString(TEXT("Kill her brood"));
	TestEqual(TEXT("The phase's name"), UHudBossBarWidget::MakePhaseLine(Silk, false, Brood).ToString(), FString(TEXT("UNDER SILK")));
	TestEqual(TEXT("Untargetable: its hint"), UHudBossBarWidget::MakePhaseLine(Silk, true, Brood).ToString(), FString(TEXT("KILL HER BROOD")));
	TestEqual(TEXT("Untargetable with no hint: the phase's name"), UHudBossBarWidget::MakePhaseLine(Silk, true, FText::GetEmpty()).ToString(),
		FString(TEXT("UNDER SILK")));

	// The bar itself, built.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHudBossBarWidget* Bar = CreateWidget<UHudBossBarWidget>(WorldWrapper.GetTestWorld(), UHudBossBarWidget::StaticClass());
	if (!TestNotNull(TEXT("The boss bar"), Bar))
	{
		return false;
	}
	TArray<float> Shares;
	for (const FBossPhase& Each : BossTestSpider::MakePhases(3))
	{
		Shares.Add(Each.HealthShare);
	}
	Bar->SetBoss(FText::FromString(TEXT("Brood Mother (test)")), 2, UCreatureRankSettings::Get(ECreatureRank::Boss).Color, Shares);
	Bar->TakeWidget();
	TestEqual(TEXT("Its name, in capitals"), Bar->GetNameText().ToString(), FString(TEXT("BROOD MOTHER (TEST)")));
	TestEqual(TEXT("Its level"), Bar->GetLevelText().ToString(), FString(TEXT("LV 2")));
	TestEqual(TEXT("A tick per later phase"), Bar->GetTickCount(), 2);
	TestTrue(TEXT("...at a half and a quarter"), Bar->GetTickShares() == TArray<float>({ 0.5f, 0.25f }));

	Bar->SetPhase(0, FText::FromString(TEXT("The Brood Stirs")));
	TestEqual(TEXT("The phase's name under it"), Bar->GetPhaseText().ToString(), FString(TEXT("THE BROOD STIRS")));
	Bar->SetHealth(60.f, 100.f);
	TestNearlyEqual(TEXT("A hit drops the bar at once"), Bar->GetFraction(), 0.6f, 0.001f);
	TestNearlyEqual(TEXT("...and its chip lingers"), Bar->GetChipFraction(), 1.f, 0.001f);
	Bar->SetPhase(1, Silk);
	Bar->SetUntargetable(true, Brood);
	TestTrue(TEXT("Untargetable: greyed"), Bar->IsGreyed());
	TestEqual(TEXT("...saying what to do"), Bar->GetPhaseText().ToString(), FString(TEXT("KILL HER BROOD")));
	Bar->SetUntargetable(false, FText::GetEmpty());
	TestFalse(TEXT("Hurtable again: not greyed"), Bar->IsGreyed());
	TestEqual(TEXT("...its phase's name back"), Bar->GetPhaseText().ToString(), FString(TEXT("UNDER SILK")));
	Bar->SetHealth(100.f, 100.f);
	TestNearlyEqual(TEXT("Healed (a reset): full, no chip"), Bar->GetChipFraction(), 1.f, 0.001f);
	return true;
}

#endif
