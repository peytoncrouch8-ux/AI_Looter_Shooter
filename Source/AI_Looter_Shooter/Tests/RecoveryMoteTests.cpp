#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Combat/RecoverySettings.h"
#include "Creatures/CreatureBase.h"
#include "Loot/LootDropComponent.h"
#include "Loot/SoulMotePickup.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RecoveryTestWorld.h"

// The recovery loop's second half (Docs/Polish/BorderlandsComparison.md, item 3): the soul-motes a kill leaves through its
// loot drop component, what one does about a player (the pull, the heal, when it is not taken), and how it looks and fades.
// The wounds that close and the drop odds are RecoveryTests.cpp's.

using namespace RecoveryTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryMoteHookTest, "Looter.Recovery.MoteHook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecoveryMoteHookTest::RunTest(const FString& Parameters)
{
	// A kill's motes come out of its loot drop component: by the creature's rank, in a practice area too (the drop doesn't
	// ask the area), from the same seeded roll, and a creature whose loot is off leaves none, a boss excepted.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const FRecoverySettings Settings;

	// The component's roll is the settings' roll: the same seed, the same number of motes, in the world.
	{
		ASpiderCreature* Spider = SpawnSpider(World, FVector::ZeroVector, ECreatureRank::Basic);
		ULootDropComponent* Loot = Spider ? Spider->FindComponentByClass<ULootDropComponent>() : nullptr;
		if (!TestNotNull(TEXT("A spider with a loot drop"), Loot))
		{
			return false;
		}
		int32 Dropped = 0;
		bool bMatches = true;
		for (int32 Seed = 0; Seed < 150; ++Seed)
		{
			FRandomStream Expected(Seed);
			const int32 Want = Settings.RollMotes(ECreatureRank::Basic, Expected);
			FRandomStream Rolled(Seed);
			const TArray<ASoulMotePickup*> Motes = Loot->DropSoulMotes(Rolled);
			bMatches &= Motes.Num() == Want;
			Dropped += Motes.Num();
		}
		TestTrue(TEXT("The component drops as many as the settings roll for the same seed"), bMatches);
		TestEqual(TEXT("...and they are in the world"), CountMotes(World), Dropped);
		TestTrue(TEXT("A Basic spider's 150 kills left some, but not most (12%)"), Dropped > 4 && Dropped < 50);

		Loot->bDropSoulMotes = false;
		FRandomStream Rolled(1);
		TestEqual(TEXT("With motes turned off: none"), Loot->DropSoulMotes(Rolled).Num(), 0);
	}

	// A boss leaves two or three, spread apart, even with its loot drop off (its own shower throws the loot).
	{
		const int32 Before = CountMotes(World);
		ASpiderCreature* Boss = SpawnSpider(World, FVector(3000.0, 0.0, 0.0), ECreatureRank::Boss);
		if (!TestNotNull(TEXT("A boss-ranked spider"), Boss))
		{
			return false;
		}
		TestTrue(TEXT("It is a boss"), Boss->GetRank() == ECreatureRank::Boss);
		const UHealthComponent* BossHealth = Boss->FindComponentByClass<UHealthComponent>();
		if (TestNotNull(TEXT("Its health"), BossHealth))
		{
			Hurt(Boss, BossHealth->GetMaxHealth() + 10.f);
			TestTrue(TEXT("It died"), Boss->IsDead());
		}
		TArray<ASoulMotePickup*> Motes;
		for (TActorIterator<ASoulMotePickup> It(World); It; ++It)
		{
			if (FVector::Dist(It->GetActorLocation(), FVector(3000.0, 0.0, 0.0)) < 400.0)
			{
				Motes.Add(*It);
			}
		}
		TestTrue(TEXT("A boss's death left two or three motes at its body"), Motes.Num() >= 2 && Motes.Num() <= 3);
		TestEqual(TEXT("...and no more in the world than that"), CountMotes(World) - Before, Motes.Num());
		for (ASoulMotePickup* Mote : Motes)
		{
			Mote->Advance(0.5f, nullptr);
		}
		bool bSpread = true;
		for (int32 A = 0; A < Motes.Num(); ++A)
		{
			for (int32 B = A + 1; B < Motes.Num(); ++B)
			{
				bSpread &= FVector::Dist(Motes[A]->GetActorLocation(), Motes[B]->GetActorLocation()) > 20.0;
			}
		}
		TestTrue(TEXT("Half a second on they have fanned out"), bSpread);
	}

	// A Basic creature with its loot off leaves none (the 12% is not rolled), and so does a thing that isn't a creature.
	{
		const int32 Before = CountMotes(World);
		ASpiderCreature* Spider = SpawnSpider(World, FVector(0.0, 3000.0, 0.0), ECreatureRank::Basic, /*bDropsLoot*/ false);
		const UHealthComponent* Health = Spider ? Spider->FindComponentByClass<UHealthComponent>() : nullptr;
		if (TestNotNull(TEXT("A spider with its loot off"), Health))
		{
			Hurt(Spider, Health->GetMaxHealth() + 10.f);
			TestTrue(TEXT("It died"), Spider->IsDead());
		}
		TestEqual(TEXT("A Basic creature with its loot off leaves no mote"), CountMotes(World), Before);

		AActor* Crate = World->SpawnActor<AActor>(FVector(0.0, -3000.0, 0.0), FRotator::ZeroRotator);
		ULootDropComponent* CrateLoot = Crate ? NewObject<ULootDropComponent>(Crate, TEXT("Loot")) : nullptr;
		if (TestNotNull(TEXT("A plain actor with a loot drop"), CrateLoot))
		{
			Crate->AddInstanceComponent(CrateLoot);
			CrateLoot->RegisterComponent();
			FRandomStream Rolled(3);
			TestEqual(TEXT("It is no creature: no motes"), CrateLoot->DropSoulMotes(Rolled).Num(), 0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryMotePickupTest, "Looter.Recovery.MotePickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecoveryMotePickupTest::RunTest(const FString& Parameters)
{
	// What a mote does about a player: nothing at full health; for a hurt one, a pull from a few meters, a heal of 15% of the
	// most (up to full), and gone. It waits a moment after dropping, and a dead player gets nothing.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	ACharacter* Player = SpawnPlayer(World, FVector::ZeroVector, Health);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Its health"), Health))
	{
		return false;
	}
	const FRecoverySettings Settings;
	const FVector Near(300.0, 0.0, 60.0);

	// A player at full health is not pulled and does not take it, however long it floats by.
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, Near, FVector::ZeroVector);
		if (!TestNotNull(TEXT("A mote"), Mote))
		{
			return false;
		}
		TestFalse(TEXT("Full health needs no mote"), ASoulMotePickup::NeedsHealing(*Health));
		const float Start = ASoulMotePickup::DistanceToBody(Mote->GetActorLocation(), *Player);
		TestTrue(TEXT("It starts within the magnet's reach, outside the pickup's"),
			Start < Settings.MoteMagnetRadius && Start > Settings.MoteCollectRadius);
		bool bTaken = false;
		for (int32 Step = 0; Step < 100; ++Step)
		{
			bTaken |= Mote->Advance(0.05f, Player);
		}
		TestFalse(TEXT("Not taken at full health"), bTaken);
		TestTrue(TEXT("...still in the world"), IsHere(Mote));
		TestEqual(TEXT("...health unchanged"), Health->GetHealth(), 100.f, 0.001f);
		const float After = ASoulMotePickup::DistanceToBody(Mote->GetActorLocation(), *Player);
		TestTrue(TEXT("...and it did not drift to them"), After > Start - 40.f);
		Mote->Destroy();
	}

	// A hurt player: it waits out its first moments, is pulled in, and heals 15 points of a hundred.
	Hurt(Player, 50.f);
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, Near, FVector::ZeroVector);
		if (!TestNotNull(TEXT("A mote for a hurt player"), Mote))
		{
			return false;
		}
		TestTrue(TEXT("A half-health player needs one"), ASoulMotePickup::NeedsHealing(*Health));
		TestFalse(TEXT("Just dropped (0.1 s): not yet"), Mote->Advance(0.1f, Player));
		TestEqual(TEXT("...health still 50"), Health->GetHealth(), 50.f, 0.001f);

		// Its first moments (to 0.65 s, with the pull only just starting) it floats up to its hover height: not yet tracked.
		bool bTaken = false;
		for (int32 Step = 0; Step < 11; ++Step)
		{
			bTaken |= Mote->Advance(0.05f, Player);
		}
		TestFalse(TEXT("Not taken in its first moments"), bTaken);
		float Distance = ASoulMotePickup::DistanceToBody(Mote->GetActorLocation(), *Player);
		bool bAlwaysCloser = true;
		for (int32 Step = 0; Step < 80 && !bTaken; ++Step)
		{
			bTaken = Mote->Advance(0.05f, Player);
			if (!bTaken)
			{
				const float Now = ASoulMotePickup::DistanceToBody(Mote->GetActorLocation(), *Player);
				bAlwaysCloser &= Now <= Distance + 1.f;
				Distance = Now;
			}
		}
		TestTrue(TEXT("The mote is pulled in and taken within four seconds"), bTaken);
		TestTrue(TEXT("...closing in all the way"), bAlwaysCloser);
		TestEqual(TEXT("A mote heals 15 of 100"), Health->GetHealth(), 65.f, 0.001f);
		TestFalse(TEXT("...and is gone"), IsHere(Mote));
	}

	// It is a share of the most health, so a bigger bar gets a bigger heal.
	Health->SetMaxHealth(200.f);
	Health->SetHealth(100.f);
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, FVector(100.0, 0.0, 60.0), FVector::ZeroVector);
		Mote->Advance(1.f, nullptr);
		TestTrue(TEXT("Right beside them, it is taken"), Mote->Advance(0.05f, Player));
		TestEqual(TEXT("15% of 200: 30"), Health->GetHealth(), 130.f, 0.001f);
	}

	// Only up to full, and the whole mote is spent.
	Health->SetHealth(195.f);
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, FVector(100.0, 0.0, 60.0), FVector::ZeroVector);
		Mote->Advance(1.f, nullptr);
		TestTrue(TEXT("Taken a few points short of full"), Mote->Advance(0.05f, Player));
		TestEqual(TEXT("...heals to the most, not past it"), Health->GetHealth(), 200.f, 0.001f);
		TestFalse(TEXT("...and is spent"), IsHere(Mote));
	}

	// A scratch (half a point) is no use to them: it stays.
	Health->SetHealth(199.7f);
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, FVector(100.0, 0.0, 60.0), FVector::ZeroVector);
		Mote->Advance(1.f, nullptr);
		TestFalse(TEXT("Not spent on a scratch"), Mote->Advance(0.05f, Player));
		TestTrue(TEXT("...it waits"), IsHere(Mote));
		Mote->Destroy();
	}

	// Out of the magnet's reach, a hurt player is not pulled.
	Health->SetHealth(100.f);
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, FVector(1200.0, 0.0, 60.0), FVector::ZeroVector);
		Mote->Advance(1.f, nullptr);
		const float Start = ASoulMotePickup::DistanceToBody(Mote->GetActorLocation(), *Player);
		for (int32 Step = 0; Step < 80; ++Step)
		{
			Mote->Advance(0.05f, Player);
		}
		TestTrue(TEXT("Twelve meters away: not pulled"), ASoulMotePickup::DistanceToBody(Mote->GetActorLocation(), *Player) > Start - 40.f);
		TestEqual(TEXT("...nothing healed"), Health->GetHealth(), 100.f, 0.001f);
		Mote->Destroy();
	}

	// The dead are not healed.
	Hurt(Player, 1.0e6f);
	{
		ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, FVector(100.0, 0.0, 60.0), FVector::ZeroVector);
		Mote->Advance(1.f, nullptr);
		TestFalse(TEXT("A dead player takes nothing"), Mote->Advance(0.05f, Player));
		TestTrue(TEXT("...the mote waits"), IsHere(Mote));
		TestEqual(TEXT("...and they stay dead"), Health->GetHealth(), 0.f);
		Mote->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryMoteLookTest, "Looter.Recovery.MoteLook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRecoveryMoteLookTest::RunTest(const FString& Parameters)
{
	// A mote waits about 30 seconds, then fades: full size to the last five, then flickering and shrinking to nothing,
	// then gone. It is the HUD's heal green, with a trail.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const FRecoverySettings Settings;
	TestEqual(TEXT("It waits 30 s"), Settings.MoteLifeSeconds, 30.f);

	// The fade's curve, on its own: full, then down to nothing, never up.
	TestEqual(TEXT("New: all there"), Settings.MoteVisible(0.f), 1.f);
	TestEqual(TEXT("24 s: all there"), Settings.MoteVisible(24.f), 1.f);
	TestEqual(TEXT("25 s: all there, about to go"), Settings.MoteVisible(25.f), 1.f);
	TestEqual(TEXT("30 s: nothing"), Settings.MoteVisible(30.f), 0.f);
	TestEqual(TEXT("After: nothing"), Settings.MoteVisible(99.f), 0.f);
	float Previous = 1.f;
	bool bNeverUp = true;
	for (float Age = 25.f; Age <= 30.f; Age += 0.05f)
	{
		const float Visible = Settings.MoteVisible(Age);
		bNeverUp &= Visible <= Previous + 0.0001f;
		Previous = Visible;
	}
	TestTrue(TEXT("The fade only goes down"), bNeverUp);
	TestTrue(TEXT("27.5 s: halfway out"), Settings.MoteVisible(27.5f) > 0.1f && Settings.MoteVisible(27.5f) < 0.9f);

	ASoulMotePickup* Mote = ASoulMotePickup::SpawnMote(World, FVector(0.0, 0.0, 60.0), FVector::ZeroVector);
	if (!TestNotNull(TEXT("A mote"), Mote))
	{
		return false;
	}

	// Its parts: a glowing core and a trail of three, no collision, no shadow, no light.
	const UStaticMeshComponent* Core = Mote->GetCore();
	if (TestNotNull(TEXT("A core"), Core))
	{
		TestNotNull(TEXT("...a sphere"), Core->GetStaticMesh().Get());
		TestTrue(TEXT("...with no collision"), Core->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(TEXT("...and no shadow"), Core->CastShadow != 0);
		TestEqual(TEXT("...17 cm across"), static_cast<float>(Core->GetRelativeScale3D().X) * 100.f, 17.f, 1.5f);
		UMaterialInstanceDynamic* Look = Cast<UMaterialInstanceDynamic>(Core->GetMaterial(0));
		if (TestNotNull(TEXT("...in an emissive material instance"), Look))
		{
			const FLinearColor Color = Look->K2_GetVectorParameterValue(TEXT("Color"));
			const FLinearColor Green = LooterUI::Color::Heal();
			TestTrue(TEXT("...the HUD's heal green (not a rarity's color)"),
				FMath::Abs(Color.R - Green.R) < 0.01f && FMath::Abs(Color.G - Green.G) < 0.01f && FMath::Abs(Color.B - Green.B) < 0.01f);
			TestTrue(TEXT("...and a glow"), Look->K2_GetScalarParameterValue(TEXT("Glow")) > 1.f);
		}
	}
	TestEqual(TEXT("A trail of three"), Mote->GetTrail().Num(), 3);
	for (const UStaticMeshComponent* Follower : Mote->GetTrail())
	{
		TestNotNull(TEXT("...each a sphere"), Follower ? Follower->GetStaticMesh().Get() : nullptr);
	}

	// It floats: after a second of drift and settling it has risen and is nearly still.
	const double StartZ = Mote->GetActorLocation().Z;
	for (int32 Step = 0; Step < 20; ++Step)
	{
		Mote->Advance(0.05f, nullptr);
	}
	TestTrue(TEXT("It floats (still in the world, near its hover height)"), IsHere(Mote) && FMath::Abs(Mote->GetActorLocation().Z - (StartZ + 40.0)) < 25.0);

	// The trail follows it when it moves.
	const FVector Before = Mote->GetActorLocation();
	Mote->SetActorLocation(Before + FVector(200.0, 0.0, 0.0));
	Mote->Advance(0.016f, nullptr);
	const UStaticMeshComponent* FirstFollower = Mote->GetTrail().IsValidIndex(0) ? Mote->GetTrail()[0].Get() : nullptr;
	if (TestNotNull(TEXT("A trailing sphere"), FirstFollower))
	{
		TestTrue(TEXT("Moved 2 m: the trail lags behind it"), FirstFollower->GetComponentLocation().X < Mote->GetActorLocation().X - 20.0);
	}
	const float FullScale = Mote->GetCore()->GetRelativeScale3D().X;

	// Growing old: full size to 25 s, then it shrinks and goes.
	Mote->Advance(25.f - Mote->GetAge(), nullptr);
	TestEqual(TEXT("25 s old: all there"), Mote->GetVisible(), 1.f);
	Mote->Advance(2.5f, nullptr);
	const float Half = Mote->GetVisible();
	TestTrue(TEXT("27.5 s old: partly gone"), Half > 0.05f && Half < 0.95f);
	TestTrue(TEXT("...and smaller to see"), Mote->GetCore()->GetRelativeScale3D().X < FullScale * 0.95f);
	Mote->Advance(2.4f, nullptr);
	TestTrue(TEXT("29.9 s old: all but gone"), Mote->GetVisible() < 0.2f && Mote->GetVisible() < Half);
	TestTrue(TEXT("...and tiny"), Mote->GetCore()->GetRelativeScale3D().X < FullScale * 0.25f);
	TestTrue(TEXT("...still there"), IsHere(Mote));
	Mote->Advance(0.2f, nullptr);
	TestFalse(TEXT("30 s: gone"), IsHere(Mote));
	return true;
}

#endif
