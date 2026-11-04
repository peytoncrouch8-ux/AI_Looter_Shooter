#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponMagazineDrop.h"
#include "Weapons/WeaponModelComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"
#include "TimerManager.h"

// The thrown magazine (Phase 3 change 12; Docs/Work/2026-11-03-Phase3/RELOAD.md): its rules (who throws, when, its arc,
// bounce, pool slot, rest and sink), its pool of 8 on one instanced mesh per mesh and paint, and a real rifle throwing
// its old magazine the very frame its own part goes. The reload's springs are ReloadSnapTests.cpp.

namespace
{
	/** A console variable set to Value for the scope and put back after. */
	struct FSnapTestVariable
	{
		FSnapTestVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FSnapTestVariable()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		FString Was;
	};

	/** Every reload part. */
	const EWeaponReloadPart SnapTestParts[] = { EWeaponReloadPart::Magazine, EWeaponReloadPart::Helix, EWeaponReloadPart::Box, EWeaponReloadPart::Bolt,
		EWeaponReloadPart::Rocket, EWeaponReloadPart::Cylinder, EWeaponReloadPart::Pump };

	/** A solid floor, its top at Z (cm). */
	AActor* SnapTestFloor(UWorld* World, double Z)
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

	/** An engine shape thrown from Where (cm) at Velocity, at Scale. */
	FMagazineDropLaunch SnapTestLaunch(UStaticMesh* Mesh, const FVector& Where, const FVector& Velocity, const FVector& Scale)
	{
		FMagazineDropLaunch Launch;
		Launch.Mesh = Mesh;
		if (Mesh)
		{
			Launch.Materials.Add(Mesh->GetMaterial(0));
		}
		Launch.Transform = FTransform(FQuat::Identity, Where, Scale);
		Launch.WorldScale = Scale;
		Launch.Velocity = Velocity;
		Launch.Spin = FVector(0.0, 0.0, 3.0);
		Launch.Channel = ECC_GameTraceChannel2;
		return Launch;
	}

	void SnapTestAdvance(UWeaponMagazineDrop& Drops, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += 1.f / 60.f)
		{
			Drops.Advance(1.f / 60.f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReloadDropMagRulesTest, "Looter.Gunfeel.Reload.DropMag.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReloadDropMagRulesTest::RunTest(const FString& Parameters)
{
	// Who throws: the magazine-like reloads; the pump's shells, the cylinder's empties (brass) and a fired rocket don't.
	for (const EWeaponReloadPart Part : SnapTestParts)
	{
		const bool bThrows = Part == EWeaponReloadPart::Magazine || Part == EWeaponReloadPart::Helix || Part == EWeaponReloadPart::Box
			|| Part == EWeaponReloadPart::Bolt;
		TestTrue(*FString::Printf(TEXT("%s throws its old one: %d"), *UEnum::GetValueAsString(Part), bThrows ? 1 : 0), MagazineDrop::Drops(Part) == bThrows);
	}
	TestFalse(TEXT("No part, nothing"), MagazineDrop::Drops(EWeaponReloadPart::None));

	// When: as it clears the well (the sniper's a beat later, its bolt first), hidden until the fresh one shows.
	TestTrue(TEXT("The rifle's as it clears the well"), FMath::IsNearlyEqual(MagazineDrop::DropAt(EWeaponReloadPart::Magazine), LooterReload::MagazineClearAt));
	TestTrue(TEXT("...the sniper's a little later"), MagazineDrop::DropAt(EWeaponReloadPart::Bolt) > MagazineDrop::DropAt(EWeaponReloadPart::Magazine));
	TestFalse(TEXT("Not hidden before it's thrown"), MagazineDrop::Hides(EWeaponReloadPart::Magazine, LooterReload::MagazineClearAt - 0.001f));
	TestTrue(TEXT("Hidden as it's thrown"), MagazineDrop::Hides(EWeaponReloadPart::Magazine, LooterReload::MagazineClearAt));
	TestFalse(TEXT("...shown again with the fresh one"), MagazineDrop::Hides(EWeaponReloadPart::Magazine, LooterReload::FreshMagazineAt));
	bool bShown = true;
	LooterReload::MagazineTravel(LooterReload::FreshMagazineAt, bShown);
	TestTrue(TEXT("...which shows there"), bShown);
	TestFalse(TEXT("A pump hides nothing"), MagazineDrop::Hides(EWeaponReloadPart::Pump, 0.35f));

	// Its flight: gravity's arc; a bounce that loses most of its fall (more in the dirt); the pool's oldest goes.
	TestTrue(TEXT("Falls 4.9 m in a second"), MagazineDrop::Arc(FVector::ZeroVector, FVector::ZeroVector, 1.f).Equals(FVector(0.0, 0.0, -490.0), 0.01));
	const FVector Hard = MagazineDrop::Bounce(FVector(100.0, 0.0, -400.0), FVector::UpVector, false);
	const FVector Soft = MagazineDrop::Bounce(FVector(100.0, 0.0, -400.0), FVector::UpVector, true);
	TestTrue(FString::Printf(TEXT("Off stone: up %.0f, on %.0f"), Hard.Z, Hard.X), Hard.Z > 0.0 && Hard.Z < 400.0 * 0.3 && Hard.X > 0.0 && Hard.X < 100.0);
	TestTrue(TEXT("...in the dirt, less"), Soft.Z < Hard.Z && Soft.X < Hard.X);
	const TArray<float> OneFree = { 3.f, -1.f, 9.f };
	const TArray<float> AllOut = { 3.f, 12.f, 9.f };
	TestEqual(TEXT("A free slot first"), MagazineDrop::PickSlot(OneFree), 1);
	TestEqual(TEXT("...else the oldest"), MagazineDrop::PickSlot(AllOut), 1);
	TestEqual(TEXT("A pool of 8"), MagazineDrop::PoolSize, 8);

	// Its rest: lying on its flattest side, as high as half that; sinking only once its rest is up.
	const FVector Extent(4.f, 1.5f, 9.f);
	const FQuat Lying = MagazineDrop::RestRotation(FQuat(FVector(1.0, 1.0, 0.0).GetSafeNormal(), 0.7), FVector::UpVector, Extent);
	TestTrue(TEXT("On its flat side (its thin axis up)"), FMath::Abs(FVector::DotProduct(Lying.GetAxisY(), FVector::UpVector)) > 0.999);
	TestEqual(TEXT("...half its thickness up"), MagazineDrop::RestLift(Extent), 1.5f);
	const float Rest = ReloadTuning::Default::DropMagRest;
	TestTrue(TEXT("Lies 20 s"), FMath::IsNearlyEqual(Rest, 20.f));
	TestEqual(TEXT("...not sinking while it lies"), MagazineDrop::SinkShare(Rest - 0.1f, Rest), 0.f);
	TestTrue(TEXT("...half sunk half way through"), FMath::IsNearlyEqual(MagazineDrop::SinkShare(Rest + MagazineDrop::SinkSeconds * 0.5f, Rest), 0.5f, 1.e-4f));
	TestEqual(TEXT("...gone after"), MagazineDrop::SinkShare(Rest + MagazineDrop::SinkSeconds + 1.f, Rest), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReloadDropMagPoolTest, "Looter.Gunfeel.Reload.DropMag.Pool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReloadDropMagPoolTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!TestNotNull(TEXT("The engine cube"), Cube) || !TestNotNull(TEXT("...and sphere"), Sphere)
		|| !TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponMagazineDrop* Drops = World->GetSubsystem<UWeaponMagazineDrop>();
	if (!TestNotNull(TEXT("The thrown magazines' pool"), Drops))
	{
		return false;
	}
	SnapTestFloor(World, 0.0);
	TestFalse(TEXT("Asleep with nothing out"), Drops->IsTickable());

	// Ten thrown: the last eight kept, all on one drawer (one mesh, one paint).
	const FVector MagazineSize(0.2f, 0.1f, 0.3f);
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Drops->Drop(SnapTestLaunch(Cube, FVector(Index * 40.0, 0.0, 150.0), FVector(0.0, 50.0, 0.0), MagazineSize));
	}
	TestEqual(TEXT("The last 8 kept"), Drops->Num(), MagazineDrop::PoolSize);
	TestEqual(TEXT("...all flying"), Drops->NumFlying(), MagazineDrop::PoolSize);
	TestEqual(TEXT("...on one instanced mesh"), Drops->NumDrawers(), 1);
	TestTrue(TEXT("...ticking"), Drops->IsTickable());
	const TArray<FVector> Thrown = Drops->GetLocations();
	TestFalse(TEXT("...the oldest two gone"), Thrown.ContainsByPredicate([](const FVector& At) { return At.X < 70.0; }));

	// They fall, bounce and lie on the floor on their flat sides.
	{
		const FSnapTestVariable Rest(TEXT("Looter.Gunfeel.Reload.DropMag.Rest"), 2.f);
		SnapTestAdvance(*Drops, 3.f);
		TestEqual(TEXT("Three seconds on, none flying"), Drops->NumFlying(), 0);
		TestEqual(TEXT("...all lying"), Drops->Num(), MagazineDrop::PoolSize);
		const float Lift = MagazineDrop::RestLift(FVector(50.f) * MagazineSize);
		bool bOnFloor = true;
		for (const FVector& At : Drops->GetLocations())
		{
			bOnFloor &= FMath::IsNearlyEqual(At.Z, static_cast<double>(Lift), 0.5);
		}
		TestTrue(FString::Printf(TEXT("...on the floor on their flat sides (%.1f cm up)"), Lift), bOnFloor);

		// Another mesh gets its own drawer; one over the edge falls away.
		Drops->Drop(SnapTestLaunch(Sphere, FVector(0.0, 0.0, 150.0), FVector::ZeroVector, FVector(0.1f)));
		TestEqual(TEXT("Another mesh, its own drawer"), Drops->NumDrawers(), 2);
		Drops->Drop(SnapTestLaunch(Cube, FVector(5000.0, 0.0, 150.0), FVector::ZeroVector, MagazineSize));

		// The rest is up (2 s here; the sphere's lands a second after it's thrown) and they sink away; the pool sleeps again.
		SnapTestAdvance(*Drops, 2.f + MagazineDrop::SinkSeconds + 2.f);
		TestEqual(TEXT("Sunk away after their rest"), Drops->Num(), 0);
		Drops->Advance(1.f / 60.f);
		TestFalse(TEXT("...asleep again"), Drops->IsTickable());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReloadDropMagGunTest, "Looter.Gunfeel.Reload.DropMag.Gun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReloadDropMagGunTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	FTestWorldWrapper WorldWrapper;
	if (!TestNotNull(TEXT("The rifle"), Rifle) || !TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	const FSnapTestVariable Thrown(TEXT("Looter.Gunfeel.Reload.DropMag"), 1.f);
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponMagazineDrop* Drops = World->GetSubsystem<UWeaponMagazineDrop>();
	APawn* Holder = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("The pool"), Drops) || !TestNotNull(TEXT("A holder"), Holder))
	{
		return false;
	}
	SnapTestFloor(World, -200.0);
	UHealthComponent* Health = NewObject<UHealthComponent>(Holder, TEXT("Health"));
	Health->bShowDamageNumbers = false;
	Holder->AddInstanceComponent(Health);
	Health->RegisterComponent();
	UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
	Holder->AddInstanceComponent(Weapons);
	Weapons->RegisterComponent();
	Weapons->MaxWeapons = 3;
	Holder->DispatchBeginPlay();
	FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1);
	Instance.SavedMagazine = 5;
	AWeaponBase* Gun = Weapons->GiveWeapon(Instance);
	const UWeaponModelComponent* Model = Gun ? Gun->FindComponentByClass<UWeaponModelComponent>() : nullptr;
	UStaticMeshComponent* Part = Model ? MagazineDrop::FindReloadPart(*Model, *Rifle) : nullptr;
	if (!TestNotNull(TEXT("A rifle in hand"), Gun) || !TestNotNull(TEXT("...built from parts"), Model) || !TestNotNull(TEXT("...its magazine part"), Part))
	{
		return false;
	}
	Weapons->AddAmmo(Gun->GetAmmoType(), 200);
	TestTrue(TEXT("It throws its magazines"), MagazineDrop::Throws(*Gun, *Model));

	// Frame by frame through a tactical reload (the reload's clock is the world's timer manager, which a test world never
	// ticks: tick it as frames would; a timer set this frame starts counting at the next).
	Gun->Reload();
	FTimerManager& Timers = World->GetTimerManager();
	Timers.Tick(0.f);
	constexpr float Frame = 1.f / 60.f;
	bool bShownUntilThrown = true;
	bool bHiddenAsThrown = false;
	bool bShownAgain = false;
	float ThrownAt = -1.f;
	float FromPart = -1.f;
	for (int32 Step = 0; Step < 400 && Gun->IsReloading(); ++Step)
	{
		++GFrameCounter;
		Timers.Tick(Frame);
		Gun->Tick(Frame);
		const float Progress = Gun->GetReloadProgress();
		if (ThrownAt < 0.f && Drops->Num() > 0)
		{
			ThrownAt = Progress;
			bHiddenAsThrown = !Part->IsVisible();
			FromPart = static_cast<float>(FVector::Dist(Drops->GetLocations()[0], Part->Bounds.Origin));
		}
		else if (ThrownAt < 0.f)
		{
			bShownUntilThrown &= Part->IsVisible();
		}
		bShownAgain |= ThrownAt >= 0.f && Progress > LooterReload::FreshMagazineAt + 0.01f && Part->IsVisible();
	}
	TestTrue(FString::Printf(TEXT("Thrown as it clears the well (at %.3f)"), ThrownAt), ThrownAt >= LooterReload::MagazineClearAt
		&& ThrownAt < LooterReload::MagazineClearAt + 0.03f);
	TestEqual(TEXT("...once"), Drops->Num(), 1);
	TestTrue(TEXT("The gun's own magazine shown until then"), bShownUntilThrown);
	TestTrue(TEXT("...hidden the very frame it's thrown"), bHiddenAsThrown);
	TestTrue(FString::Printf(TEXT("...thrown from where it was (%.2f cm)"), FromPart), FromPart >= 0.f && FromPart < 1.f);
	TestTrue(TEXT("The fresh one shows in the well"), bShownAgain);
	TestFalse(TEXT("The reload done"), Gun->IsReloading());
	SnapTestAdvance(*Drops, 3.f);
	TestTrue(TEXT("The old one lies on the floor"), Drops->Num() == 1 && Drops->NumFlying() == 0 && Drops->GetLocations()[0].Z < -180.0);
	return true;
}

#endif
