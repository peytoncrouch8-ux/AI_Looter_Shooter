#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponBrass.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponModelComponent.h"
#include "Components/BoxComponent.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Tests/AutomationCommon.h"

// The guns' physical touches (Saved/Concepts/Polish/E_guns.png): spent brass from the right port per gun kind, capped and
// landing; the revolver's empties on a reload; what an impact throws landing on its floor; a ricochet, a muzzle's wisp and
// a tracer's thread drawn and gone. The hits' side (fly-bys, creatures' splashes) is WeaponFXHitsTests.cpp.

namespace
{
	const TCHAR* const FXTestRiflePath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	const TCHAR* const FXTestShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");
	const TCHAR* const FXTestRevolverPath = TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver");

	/** A solid floor, its top at Z (cm), wide enough for anything thrown in the tests. */
	AActor* FXTestFloor(UWorld* World, double Z)
	{
		AActor* Floor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
		Box->SetBoxExtent(FVector(3000.f, 3000.f, 50.f));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Floor->SetRootComponent(Box);
		Box->RegisterComponent();
		Floor->SetActorLocation(FVector(0.0, 0.0, Z - 50.0));
		return Floor;
	}

	/** A loose gun of Definition (no holder: nothing stands between the trigger and the shot). */
	AWeaponBase* FXTestLooseGun(UWorld* World, UWeaponDefinition* Definition, const FTransform& Where)
	{
		const FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1);
		return UWeaponRollLibrary::SpawnWeapon(World, Gun, Where);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponFXBrassTest, "Looter.Weapons.FX.Brass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponFXBrassTest::RunTest(const FString& Parameters)
{
	// The rules: which case each gun throws, and what it sounds like where it lands.
	TestEqual(TEXT("A rifle throws brass"), FWeaponBrass::KindFor(EWeaponKind::Rifle), ECasingKind::Rifle);
	TestEqual(TEXT("A shotgun throws hulls"), FWeaponBrass::KindFor(EWeaponKind::Shotgun), ECasingKind::Hull);
	TestEqual(TEXT("A revolver's are pistol cases"), FWeaponBrass::KindFor(EWeaponKind::Revolver), ECasingKind::Pistol);
	TestEqual(TEXT("Brass on stone clinks"), FWeaponBrass::CueFor(ECasingKind::Rifle, EImpactMaterial::Stone), FName(TEXT("Weapon.Casing.Stone")));
	TestEqual(TEXT("...on wood knocks"), FWeaponBrass::CueFor(ECasingKind::Rifle, EImpactMaterial::Wood), FName(TEXT("Weapon.Casing.Wood")));
	TestEqual(TEXT("...on steel rings"), FWeaponBrass::CueFor(ECasingKind::Pistol, EImpactMaterial::Metal), FName(TEXT("Weapon.Casing.Metal")));
	TestEqual(TEXT("...in the dirt, a dull tick"), FWeaponBrass::CueFor(ECasingKind::Rifle, EImpactMaterial::Grass), FName(TEXT("Weapon.Casing.Dirt")));
	TestEqual(TEXT("A hull on stone knocks hollow"), FWeaponBrass::CueFor(ECasingKind::Hull, EImpactMaterial::Stone), FName(TEXT("Weapon.Casing.Hull")));
	TestEqual(TEXT("Anything into water plips"), FWeaponBrass::CueFor(ECasingKind::Hull, EImpactMaterial::Water), FName(TEXT("Weapon.Casing.Water")));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("Bullet system"), Bullets))
	{
		return false;
	}
	FXTestFloor(World, 0.0);
	FWeaponBrass& Brass = Bullets->GetBrass();

	// A long burst's worth: never more than the cap, the oldest going first.
	for (int32 Shot = 0; Shot < 100; ++Shot)
	{
		FCasingLaunch Launch;
		Launch.Kind = Shot % 3 == 0 ? ECasingKind::Hull : ECasingKind::Rifle;
		Launch.Location = FVector(Shot * 2.0, 0.0, 120.0);
		Launch.Velocity = FVector(-40.0, 300.0, 150.0);
		Launch.Spin = FVector(0.0, 0.0, 30.0);
		Launch.Channel = ECC_GameTraceChannel2;
		Bullets->EjectCasing(Launch);
		if (Brass.Num() > FWeaponBrass::MaxCasings)
		{
			AddError(FString::Printf(TEXT("%d cases after %d shots, over the cap of %d"), Brass.Num(), Shot + 1, FWeaponBrass::MaxCasings));
			break;
		}
	}
	TestEqual(TEXT("The last 60 kept"), Brass.Num(), FWeaponBrass::MaxCasings);
	TestEqual(TEXT("...all flying"), Brass.NumFlying(), FWeaponBrass::MaxCasings);

	// They fall, bounce and come to lie on the floor.
	for (float Time = 0.f; Time < 3.f; Time += 1.f / 60.f)
	{
		Bullets->Tick(1.f / 60.f);
	}
	TestEqual(TEXT("Three seconds on, none flying"), Brass.NumFlying(), 0);
	TestEqual(TEXT("...all still there"), Brass.Num(), FWeaponBrass::MaxCasings);
	bool bAllOnFloor = true;
	for (const FVector& Location : Brass.GetLocations())
	{
		bAllOnFloor &= Location.Z > -0.1 && Location.Z < 2.;
	}
	TestTrue(TEXT("...lying on the floor, not in it or over it"), bAllOnFloor);
	TestFalse(TEXT("Brass lying about keeps the system awake"), Brass.IsIdle());

	// A while later they sink away and the system goes quiet.
	for (float Time = 0.f; Time < FWeaponBrass::RestSeconds + FWeaponBrass::FadeSeconds + 0.5f; Time += 0.1f)
	{
		Bullets->Tick(0.1f);
	}
	TestEqual(TEXT("Gone after lying a while"), Brass.Num(), 0);
	Bullets->Tick(0.1f);
	TestTrue(TEXT("...and nothing left to tick"), Brass.IsIdle());

	// Thrown over nothing (off a cliff), a case is given up once it has fallen far.
	FCasingLaunch Over;
	Over.Location = FVector(50000.0, 0.0, 100.0);
	Over.Velocity = FVector(0.0, 200.0, 100.0);
	Bullets->EjectCasing(Over);
	for (float Time = 0.f; Time < 3.f; Time += 0.05f)
	{
		Bullets->Tick(0.05f);
	}
	TestEqual(TEXT("A case over nothing is given up"), Brass.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponFXBrassPortsTest, "Looter.Weapons.FX.BrassPorts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponFXBrassPortsTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, FXTestRiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, FXTestShotgunPath);
	UWeaponDefinition* Revolver = LoadObject<UWeaponDefinition>(nullptr, FXTestRevolverPath);
	FTestWorldWrapper WorldWrapper;
	if (!TestNotNull(TEXT("The rifle"), Rifle) || !TestNotNull(TEXT("The shotgun"), Shotgun) || !TestNotNull(TEXT("The revolver"), Revolver)
		|| !TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("Bullet system"), Bullets))
	{
		return false;
	}
	FWeaponBrass& Brass = Bullets->GetBrass();
	// Turned and moved, so the port is found through the gun's own frame.
	const FTransform Where(FRotator(0.f, 90.f, 0.f), FVector(500.0, 300.0, 200.0));

	// Where a case leaves, in the gun's own frame: out of its right side (+Y), along its body, flung out to the right.
	auto CheckPort = [this](const TCHAR* Gun, const AWeaponBase& Weapon, const FCasingLaunch& Launch, double MaxX)
	{
		const UWeaponModelComponent* Model = Weapon.FindComponentByClass<UWeaponModelComponent>();
		if (!TestNotNull(*FString::Printf(TEXT("%s: built from parts"), Gun), Model))
		{
			return;
		}
		const FTransform& Frame = Model->GetComponentTransform();
		const FVector Port = Frame.InverseTransformPosition(Launch.Location);
		const FVector Out = Frame.InverseTransformVectorNoScale(Launch.Velocity);
		FVector Expected;
		FVector Direction;
		TestTrue(*FString::Printf(TEXT("%s: has a port"), Gun), Weapon.GetEjectPort(Expected, Direction));
		TestTrue(*FString::Printf(TEXT("%s: the case leaves from it"), Gun), Launch.Location.Equals(Expected, 0.01));
		TestTrue(*FString::Printf(TEXT("%s: the port on the gun's right (%s)"), Gun, *Port.ToCompactString()), Port.Y > 0.8 && Port.Y < 6.0);
		TestTrue(*FString::Printf(TEXT("%s: ...on its body (%.1f cm along)"), Gun, Port.X), Port.X >= 0.0 && Port.X <= MaxX && FMath::Abs(Port.Z) < 8.0);
		TestTrue(*FString::Printf(TEXT("%s: flung out to the right (%s)"), Gun, *Out.ToCompactString()), Out.Y > 100.0 && Out.Y > FMath::Abs(Out.X));
	};

	// The rifle: a case the moment it fires.
	AWeaponBase* RifleGun = FXTestLooseGun(World, Rifle, Where);
	if (TestNotNull(TEXT("A rifle"), RifleGun))
	{
		const int32 Before = Brass.GetLaunches();
		RifleGun->StartFire();
		RifleGun->StopFire();
		TestEqual(TEXT("Rifle: one case a shot"), Brass.GetLaunches(), Before + 1);
		TestEqual(TEXT("Rifle: a rifle case"), Brass.GetLastLaunch().Kind, ECasingKind::Rifle);
		CheckPort(TEXT("Rifle"), *RifleGun, Brass.GetLastLaunch(), 30.0);
	}

	// The shotgun: its hull once it's pumped, a moment after the shot.
	AWeaponBase* ShotgunGun = FXTestLooseGun(World, Shotgun, Where);
	if (TestNotNull(TEXT("A shotgun"), ShotgunGun))
	{
		const int32 Before = Brass.GetLaunches();
		ShotgunGun->StartFire();
		ShotgunGun->StopFire();
		TestEqual(TEXT("Shotgun: no hull at the shot itself"), Brass.GetLaunches(), Before);
		for (float Time = 0.f; Time < ShotgunGun->GetHullDelay() + 0.1f; Time += 0.05f)
		{
			ShotgunGun->Tick(0.05f);
		}
		TestEqual(TEXT("Shotgun: the hull once pumped"), Brass.GetLaunches(), Before + 1);
		TestEqual(TEXT("Shotgun: a hull"), Brass.GetLastLaunch().Kind, ECasingKind::Hull);
		CheckPort(TEXT("Shotgun"), *ShotgunGun, Brass.GetLastLaunch(), 25.0);
	}

	// The revolver keeps its empties.
	AWeaponBase* RevolverGun = FXTestLooseGun(World, Revolver, Where);
	if (TestNotNull(TEXT("A revolver"), RevolverGun))
	{
		const int32 Before = Brass.GetLaunches();
		RevolverGun->StartFire();
		RevolverGun->StopFire();
		for (int32 Frame = 0; Frame < 10; ++Frame)
		{
			RevolverGun->Tick(0.05f);
		}
		TestEqual(TEXT("Revolver: nothing thrown on firing"), Brass.GetLaunches(), Before);
		FVector Port;
		FVector Out;
		TestFalse(TEXT("Revolver: no ejection port"), RevolverGun->GetEjectPort(Port, Out));
		TestEqual(TEXT("Revolver: one empty in the cylinder"), RevolverGun->GetSpentInCylinder(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponFXRevolverEmptiesTest, "Looter.Weapons.FX.RevolverEmpties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponFXRevolverEmptiesTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Revolver = LoadObject<UWeaponDefinition>(nullptr, FXTestRevolverPath);
	FTestWorldWrapper WorldWrapper;
	if (!TestNotNull(TEXT("The revolver"), Revolver) || !TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();

	// A holder as the game has one, the revolver in hand with two rounds left and more to load.
	APawn* Holder = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("Bullet system"), Bullets) || !TestNotNull(TEXT("A holder"), Holder))
	{
		return false;
	}
	UHealthComponent* Health = NewObject<UHealthComponent>(Holder, TEXT("Health"));
	Health->bShowDamageNumbers = false;
	Holder->AddInstanceComponent(Health);
	Health->RegisterComponent();
	UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
	Holder->AddInstanceComponent(Weapons);
	Weapons->RegisterComponent();
	Weapons->MaxWeapons = 3;
	Holder->DispatchBeginPlay();
	FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Revolver, EWeaponRarity::Rare, 1);
	Instance.SavedMagazine = 2;
	AWeaponBase* Gun = Weapons->GiveWeapon(Instance);
	const UWeaponModelComponent* Model = Gun ? Gun->FindComponentByClass<UWeaponModelComponent>() : nullptr;
	if (!TestNotNull(TEXT("A revolver in hand"), Gun) || !TestNotNull(TEXT("...built from parts"), Model))
	{
		return false;
	}
	Weapons->AddAmmo(Gun->GetAmmoType(), 30);
	const int32 Empties = Gun->GetStats().MagazineSize - 2;
	TestEqual(TEXT("Taken up part-fired: its empties are in the cylinder"), Gun->GetSpentInCylinder(), Empties);

	FWeaponBrass& Brass = Bullets->GetBrass();
	const int32 Before = Brass.GetLaunches();
	Gun->Reload();
	if (!TestTrue(TEXT("Reloading"), Gun->IsReloading()))
	{
		return false;
	}
	Gun->Tick(0.f);
	TestEqual(TEXT("Nothing out before the rod is punched"), Brass.GetLaunches(), Before);

	// On past the rod's punch (the reload's Eject step): the empties tumble out of the chambers' backs. The reload's clock is
	// the world's timer manager, which a test world never ticks: tick it as two frames would. A timer set this frame only
	// starts counting at the manager's next tick, and the manager ticks once a frame (a second tick in one frame does
	// nothing), so the first tick starts the reload's timer and the next frame's moves it on.
	FTimerManager& Timers = World->GetTimerManager();
	Timers.Tick(0.f);
	++GFrameCounter;
	Timers.Tick(Gun->GetStats().ReloadTime * 0.4f);
	if (!TestTrue(FString::Printf(TEXT("Past the punch (%.2f of the reload)"), Gun->GetReloadProgress()), Gun->GetReloadProgress() > 0.31f))
	{
		return false;
	}
	Gun->Tick(0.f);
	TestEqual(TEXT("Every empty out at the punch"), Brass.GetLaunches(), Before + Empties);
	TestEqual(TEXT("...all pistol cases"), Brass.NumOfKind(ECasingKind::Pistol), Empties);
	TestEqual(TEXT("...the cylinder empty of them"), Gun->GetSpentInCylinder(), 0);
	TArray<FVector> Chambers;
	FVector Back;
	if (TestTrue(TEXT("Its chambers"), Model->GetChamberRears(Chambers, Back)))
	{
		bool bFromChambers = true;
		for (const FVector& Location : Brass.GetLocations())
		{
			bFromChambers &= Chambers.ContainsByPredicate([&Location](const FVector& Chamber) { return FVector::Dist(Chamber, Location) < 0.5; });
		}
		TestTrue(TEXT("...each from a chamber's back"), bFromChambers);
	}
	Gun->Tick(0.f);
	TestEqual(TEXT("...and only once"), Brass.GetLaunches(), Before + Empties);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponFXDebrisTest, "Looter.Weapons.FX.Debris",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponFXDebrisTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	FWeaponFX Effects;
	Effects.Initialize(World);
	auto Run = [&Effects](float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += 1.f / 60.f)
		{
			Effects.Tick(1.f / 60.f, FVector(-500.0, 0.0, 170.0));
		}
	};

	// Splinters off a post on the ground: thrown, landing on the ground, lying there a moment, never sinking into it.
	const FPlane Ground(FVector::ZeroVector, FVector::UpVector);
	Effects.SpawnSurfaceImpact(FVector(0.0, 0.0, 30.0), FVector(-1.0, 0.0, 0.0), FVector(1.0, 0.0, -0.2), EImpactMaterial::Wood, 1.f, false, &Ground);
	Run(1.f);
	TestTrue(FString::Printf(TEXT("A second on, splinters lie on the ground (%d)"), Effects.NumResting()), Effects.NumResting() > 0);
	TestTrue(FString::Printf(TEXT("...none sunk into it (%.2f cm)"), Effects.LowestOverFloor()), Effects.LowestOverFloor() > -0.05f);
	Run(3.f);
	TestEqual(TEXT("A few seconds on, gone"), Effects.NumResting(), 0);

	// Without ground under it nothing lies anywhere.
	Effects.SpawnSurfaceImpact(FVector(0.0, 0.0, 30.0), FVector(-1.0, 0.0, 0.0), FVector(1.0, 0.0, -0.2), EImpactMaterial::Wood, 1.f, false);
	Run(1.f);
	TestEqual(TEXT("No floor: nothing lies"), Effects.NumResting(), 0);

	// A ricochet, a muzzle's wisp and a tracer's thread: each drawn, each gone in time.
	Run(3.f);
	const int32 Before = Effects.NumParticles();
	Effects.SpawnRicochet(FVector(0.0, 0.0, 10.0), FVector(1.0, 0.0, 0.2), 1.f);
	Effects.SpawnWisp(FVector(0.0, 0.0, 150.0), FVector(0.0, 0.0, 20.0), 0.2f, 2.f, 12.f, 1.4f);
	Effects.AddThread(FVector(100.0, 0.0, 150.0), FVector(250.0, 0.0, 150.0), 2.5f, 0.12f, 0.45f);
	TestTrue(TEXT("A ricochet's tracer, sparks and glint, a wisp and a thread"), Effects.NumParticles() >= Before + 8);
	Run(3.f);
	TestEqual(TEXT("...all gone a few seconds on"), Effects.NumParticles(), 0);
	Effects.Tick(1.f / 60.f, FVector::ZeroVector);
	TestTrue(TEXT("...and nothing left to tick"), Effects.IsIdle());
	Effects.Shutdown();
	return true;
}

#endif
