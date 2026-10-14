#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/NearMiss.h"
#include "Weapons/WeaponFX.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Tests/AutomationCommon.h"

// The hits of the guns' polish (Saved/Concepts/Polish/E_guns.png, and the creatures' board's own blood): enemy shots heard
// going by the player's head, once each, never the player's own; each creature's own splash where a bullet hits it.

namespace
{
	/** A player as the near-miss needs one: a living character, held (or not) by this machine's player controller. */
	ACharacter* HitsTestPlayer(UWorld* World, const FVector& Where, bool bPlayerHeld)
	{
		ACharacter* Player = World->SpawnActor<ACharacter>(Where, FRotator::ZeroRotator);
		if (!Player)
		{
			return nullptr;
		}
		UHealthComponent* Health = NewObject<UHealthComponent>(Player, TEXT("Health"));
		Health->bShowDamageNumbers = false;
		Player->AddInstanceComponent(Health);
		Health->RegisterComponent();
		Player->DispatchBeginPlay();
		if (bPlayerHeld)
		{
			// A test world never readies its actors: the controller joins the world's list, as play would add it, holds the
			// pawn, and counts as this machine's player (with no net driver, UE asks for a local player otherwise).
			APlayerController* Controller = World->SpawnActor<APlayerController>();
			if (!Controller)
			{
				return nullptr;
			}
			World->AddController(Controller);
			Controller->SetAsLocalPlayerController();
			Controller->SetPawn(Player);
		}
		return Player;
	}

	FEnemyShot HitsTestPellet(const FVector& Start, AActor* Shooter)
	{
		FEnemyShot Pellet;
		Pellet.Start = Start;
		Pellet.Direction = FVector::ForwardVector;
		Pellet.Speed = 1000.f;
		Pellet.Range = 4000.f;
		Pellet.Radius = 12.f;
		Pellet.Damage = 5.f;
		Pellet.Shooter = Shooter;
		return Pellet;
	}

	/** A creature's class and what it should spill. */
	struct FHitsTestBloodKind
	{
		UClass* Class;
		ECreatureBlood Blood;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponFXNearMissTest, "Looter.Weapons.FX.NearMiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponFXNearMissTest::RunTest(const FString& Parameters)
{
	// The rules: how near a path comes, and what each shooter's shots sound like.
	FVector Nearest;
	TestNearlyEqual(TEXT("A path a meter beside the head"), NearMiss::ClosestApproach(FVector(-500.0, 100.0, 0.0), FVector(500.0, 100.0, 0.0),
		FVector::ZeroVector, Nearest), 100.f, 0.01f);
	TestTrue(TEXT("...nearest abreast of it"), Nearest.Equals(FVector(0.0, 100.0, 0.0), 0.01));
	TestEqual(TEXT("Abel's buckshot whizzes"), NearMiss::SoundOf(GetDefault<AAbelKeeper>()), ENearMissSound::Whiz);
	TestEqual(TEXT("The Gravemother's spit hisses"), NearMiss::SoundOf(GetDefault<AGravemotherCreature>()), ENearMissSound::Hiss);
	TestEqual(TEXT("The Spitter's glob hisses"), NearMiss::SoundOf(GetDefault<ASpitterSpider>()), ENearMissSound::Hiss);
	TestEqual(TEXT("A shooter gone: a whiz"), NearMiss::SoundOf(nullptr), ENearMissSound::Whiz);
	TestEqual(TEXT("The whiz's cue"), NearMiss::CueOf(ENearMissSound::Whiz), FName(TEXT("Weapon.NearMiss.Whiz")));
	TestEqual(TEXT("The hiss's cue"), NearMiss::CueOf(ENearMissSound::Hiss), FName(TEXT("Weapon.NearMiss.Hiss")));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UEnemyProjectileSubsystem* Shots = World->GetSubsystem<UEnemyProjectileSubsystem>();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	ACharacter* Player = HitsTestPlayer(World, FVector::ZeroVector, true);
	AActor* Shooter = World->SpawnActor<AActor>(FVector(-2500.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Enemy pellets"), Shots) || !TestNotNull(TEXT("Bullets"), Bullets) || !TestNotNull(TEXT("A player"), Player)
		|| !TestNotNull(TEXT("A shooter"), Shooter))
	{
		return false;
	}
	const FVector Head = NearMiss::HeadOf(*Player);
	TArray<APawn*> Heard;
	int32 Hits = 0;
	Shots->OnNearMiss.AddLambda([&Heard](APawn* Listener, AActor*, const FVector&) { Heard.Add(Listener); });
	Shots->OnShotHit.AddLambda([&Hits](AActor*, AActor*, float) { ++Hits; });
	auto Fly = [Shots](float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += 0.05f)
		{
			Shots->Tick(0.05f);
		}
	};

	// A pellet a meter wide of the head: heard once as it goes by, and it hurts nobody.
	Shots->Fire(HitsTestPellet(Head + FVector(-2000.0, 100.0, 0.0), Shooter));
	Fly(4.f);
	TestEqual(TEXT("A meter wide: heard going by, once"), Heard.Num(), 1);
	TestTrue(TEXT("...by the player"), Heard.Num() == 1 && Heard[0] == Player);
	TestEqual(TEXT("...and it hit nobody"), Hits, 0);

	// Three meters wide: not heard.
	Heard.Reset();
	Shots->Fire(HitsTestPellet(Head + FVector(-2000.0, 300.0, 0.0), Shooter));
	Fly(4.f);
	TestEqual(TEXT("Three meters wide: unheard"), Heard.Num(), 0);

	// A volley going by together: each pellet heard once.
	Heard.Reset();
	Shots->FireVolley(HitsTestPellet(Head + FVector(-2000.0, 110.0, 0.0), Shooter), FVector::ForwardVector, 3, 1.f);
	Fly(4.f);
	TestEqual(TEXT("A volley of three going by: three, once each"), Heard.Num(), 3);

	// The player's own: never.
	Heard.Reset();
	Shots->Fire(HitsTestPellet(Head + FVector(-2000.0, 100.0, 0.0), Player));
	Fly(4.f);
	TestEqual(TEXT("A pellet of the player's own: unheard"), Heard.Num(), 0);
	FBulletShot Own;
	Own.Start = Head + FVector(-1500.0, 80.0, 0.0);
	Own.VisualStart = Own.Start;
	Own.Direction = FVector::ForwardVector;
	Own.Speed = 10000.f;
	Own.Range = 3000.f;
	Own.Channel = ECC_Visibility;
	Own.Shooter = Player;
	Bullets->Fire(Own);
	for (float Time = 0.f; Time < 0.5f; Time += 1.f / 60.f)
	{
		Bullets->Tick(1.f / 60.f);
		Shots->Tick(1.f / 60.f);
	}
	TestEqual(TEXT("The player's own bullets past their head: unheard"), Heard.Num(), 0);

	// Someone no player holds hears nothing (a whiz is for the player's ears).
	Heard.Reset();
	ACharacter* Stranger = HitsTestPlayer(World, FVector(0.0, 3000.0, 0.0), false);
	if (TestNotNull(TEXT("A stranger"), Stranger))
	{
		Shots->Fire(HitsTestPellet(NearMiss::HeadOf(*Stranger) + FVector(-2000.0, 100.0, 0.0), Shooter));
		Fly(4.f);
		TestEqual(TEXT("Past someone no player holds: unheard"), Heard.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponFXBloodTest, "Looter.Weapons.FX.Blood",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponFXBloodTest::RunTest(const FString& Parameters)
{
	// Each creature spills its own, by its class (the most specific wins).
	const FHitsTestBloodKind Kinds[] = {
		{ ASpiderCreature::StaticClass(), ECreatureBlood::Ichor },
		{ ASpitterSpider::StaticClass(), ECreatureBlood::Ichor },
		{ AGravemotherCreature::StaticClass(), ECreatureBlood::DarkIchor },
		{ ASlimeCreature::StaticClass(), ECreatureBlood::Gel },
		{ AUnpaidCreature::StaticClass(), ECreatureBlood::Embers },
		{ ABellRingerCreature::StaticClass(), ECreatureBlood::Embers },
		{ AAbelKeeper::StaticClass(), ECreatureBlood::Embers },
		{ ADeadMinerCreature::StaticClass(), ECreatureBlood::MinerEmbers },
		{ AForemanCreature::StaticClass(), ECreatureBlood::MinerEmbers },
		{ ATargetDummy::StaticClass(), ECreatureBlood::None },
		{ AActor::StaticClass(), ECreatureBlood::None },
	};
	for (const FHitsTestBloodKind& Kind : Kinds)
	{
		TestEqual(*FString::Printf(TEXT("%s spills %s"), *Kind.Class->GetName(), CreatureBlood::Name(Kind.Blood)), CreatureBlood::OfClass(Kind.Class), Kind.Blood);
	}
	TestEqual(TEXT("Nothing spills nothing"), CreatureBlood::Of(nullptr), ECreatureBlood::None);
	TestTrue(TEXT("An Unpaid's embers are its coal's color"), CreatureBlood::TintOf(GetDefault<AUnpaidCreature>()).Equals(GetDefault<AUnpaidCreature>()->GetCoalColor()));
	TestTrue(TEXT("Venom is wet, embers aren't"), CreatureBlood::IsWet(ECreatureBlood::Gel) && CreatureBlood::IsWet(ECreatureBlood::DarkIchor)
		&& !CreatureBlood::IsWet(ECreatureBlood::Embers) && !CreatureBlood::IsWet(ECreatureBlood::MinerEmbers));
	TestEqual(TEXT("A bullet on a creature splashes"), UBulletSubsystem::SurfaceOf(GetDefault<ASlimeCreature>()), EImpactSurface::Flesh);

	// Every kind throws something where it's hit (on top of the hit's own reaction).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	FWeaponFX Effects;
	Effects.Initialize(World);
	for (const ECreatureBlood Blood : { ECreatureBlood::Ichor, ECreatureBlood::DarkIchor, ECreatureBlood::Gel, ECreatureBlood::Embers, ECreatureBlood::MinerEmbers })
	{
		const int32 Before = Effects.NumParticles();
		Effects.SpawnCreatureHit(FVector::ZeroVector, FVector::UpVector, FVector(1.0, 0.0, -0.3), Blood, CreatureBlood::DefaultTint(Blood), 1.f, false);
		TestTrue(FString::Printf(TEXT("%s: a splash"), CreatureBlood::Name(Blood)), Effects.NumParticles() > Before + 3);
	}
	Effects.Shutdown();
	return true;
}

#endif
