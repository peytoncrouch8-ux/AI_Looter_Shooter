#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTrigger.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

// Spamming the trigger (Looter.Weapons.Trigger.*): a click during a gun's cooldown is remembered only if it comes just
// before the gun is ready, and only once, so a flurry of clicks on a pump shotgun fires one shot and never a stack of
// them after the clicking has stopped. The rule alone, played out over random clicking, then on real guns.

namespace
{
	using WeaponTrigger::BufferSeconds;
	using WeaponTrigger::EPress;

	/**
	 * Plays Presses (times in seconds, in order; each one a click, pressed and let go at once) against a gun of Mode that
	 * fires every Interval seconds, as AWeaponBase::StartFireAt does: a ready gun fires, a press just before ready is
	 * remembered, and the remembered shot goes off when the gun is ready. Returns when each shot went off.
	 */
	TArray<double> PlayClicks(EWeaponFireMode Mode, double Interval, const TArray<double>& Presses)
	{
		TArray<double> Shots;
		double LastShot = -1000.0;
		double Waiting = -1.0;
		auto FireWaiting = [&](double ByTime)
		{
			if (Waiting >= 0.0 && Waiting <= ByTime)
			{
				Shots.Add(Waiting);
				LastShot = Waiting;
				Waiting = -1.0;
			}
		};
		for (const double Press : Presses)
		{
			FireWaiting(Press);
			const double Until = Interval - (Press - LastShot);
			switch (WeaponTrigger::Decide(Mode, Until, Waiting >= 0.0))
			{
			case EPress::FireNow:
				Shots.Add(Press);
				LastShot = Press;
				break;
			case EPress::Wait:
				Waiting = Press + Until;
				break;
			case EPress::Ignore:
				break;
			}
		}
		// After the last click, only a remembered press can still go off.
		FireWaiting(TNumericLimits<double>::Max());
		return Shots;
	}

	TArray<double> Evenly(double From, double Step, int32 Count)
	{
		TArray<double> Times;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Times.Add(From + Step * Index);
		}
		return Times;
	}

	UWeaponDefinition* LoadDefinition(const TCHAR* Path)
	{
		return LoadObject<UWeaponDefinition>(nullptr, Path);
	}

	/**
	 * A loose gun (no holder, so no draw time stands between the trigger and the shot) from a rolled instance. Every roll
	 * picks its own parts, and the parts set the fire rate and the magazine, so guns that are compared by one gun's
	 * interval and rounds must all be spawned from the same instance.
	 */
	AWeaponBase* SpawnLoose(UWorld* World, const FWeaponInstanceData& Gun)
	{
		return UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform::Identity);
	}

	FWeaponInstanceData RollGun(UWeaponDefinition* Definition)
	{
		return UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTriggerRuleTest, "Looter.Weapons.Trigger.Rule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponTriggerRuleTest::RunTest(const FString& Parameters)
{
	// What a single press does.
	for (const EWeaponFireMode Mode : { EWeaponFireMode::SemiAuto, EWeaponFireMode::FullAuto, EWeaponFireMode::Burst })
	{
		TestTrue(TEXT("A ready gun fires"), WeaponTrigger::Decide(Mode, 0.0, false) == EPress::FireNow);
		TestTrue(TEXT("...a gun past ready too"), WeaponTrigger::Decide(Mode, -0.4, false) == EPress::FireNow);
		TestTrue(TEXT("A shot already on its way: another press adds nothing"), WeaponTrigger::Decide(Mode, 0.05, true) == EPress::Ignore
			&& WeaponTrigger::Decide(Mode, 0.0, true) == EPress::Ignore);
	}
	for (const EWeaponFireMode Mode : { EWeaponFireMode::SemiAuto, EWeaponFireMode::Burst })
	{
		TestTrue(TEXT("Just before ready, a press is remembered"), WeaponTrigger::Decide(Mode, 0.05, false) == EPress::Wait);
		TestTrue(TEXT("...up to the buffer's edge"), WeaponTrigger::Decide(Mode, BufferSeconds, false) == EPress::Wait);
		TestTrue(TEXT("...a hair earlier and it is lost"), WeaponTrigger::Decide(Mode, BufferSeconds + 0.001, false) == EPress::Ignore);
		TestTrue(TEXT("...and a click mid-pump is lost"), WeaponTrigger::Decide(Mode, 0.6, false) == EPress::Ignore);
	}
	TestTrue(TEXT("A held full-auto trigger waits for the gun, however long"), WeaponTrigger::Decide(EWeaponFireMode::FullAuto, 0.6, false) == EPress::Wait);
	TestTrue(TEXT("The buffer is about a tenth of a second"), BufferSeconds > 0.08 && BufferSeconds < 0.2);

	// Ten rapid presses in one pump interval (0.8 s): one shot, and no delayed one after the clicking stops.
	constexpr double Pump = 0.8;
	TArray<double> Shots = PlayClicks(EWeaponFireMode::SemiAuto, Pump, Evenly(0.0, 0.05, 10));
	TestEqual(TEXT("Ten clicks in the first half of the pump: one shot"), Shots.Num(), 1);
	TestTrue(TEXT("...at the first click"), Shots.Num() == 1 && FMath::IsNearlyZero(Shots[0]));

	// The tenth click lands just before the gun is ready: that one is remembered, and fires right after (never more).
	Shots = PlayClicks(EWeaponFireMode::SemiAuto, Pump, Evenly(0.0, 0.08, 10));
	TestEqual(TEXT("Ten clicks across the pump, the last just before ready: two shots"), Shots.Num(), 2);
	TestTrue(TEXT("...the second the moment the gun is ready"), Shots.Num() == 2 && FMath::IsNearlyEqual(Shots[1], Pump, 1e-6));

	// A last click a little too early is simply lost.
	Shots = PlayClicks(EWeaponFireMode::SemiAuto, Pump, Evenly(0.0, 0.05, 14));
	TestEqual(TEXT("Clicking until 0.65 s, short of the buffer: one shot"), Shots.Num(), 1);
	Shots = PlayClicks(EWeaponFireMode::SemiAuto, Pump, { 0.0, 0.67 });
	TestEqual(TEXT("A click just outside the buffer is lost"), Shots.Num(), 1);
	Shots = PlayClicks(EWeaponFireMode::SemiAuto, Pump, { 0.0, 0.69 });
	TestEqual(TEXT("A click just inside it is remembered"), Shots.Num(), 2);

	// Several clicks inside the buffer remember one shot, not several.
	Shots = PlayClicks(EWeaponFireMode::SemiAuto, Pump, { 0.0, 0.70, 0.72, 0.74, 0.76, 0.78, 0.79 });
	TestEqual(TEXT("A flurry just before ready: one extra shot"), Shots.Num(), 2);

	// Nobody has to stop to be fast: mashing keeps the gun's full rate, shot after shot exactly an interval apart.
	Shots = PlayClicks(EWeaponFireMode::SemiAuto, Pump, Evenly(0.0, 0.03, 200));
	bool bSteady = Shots.Num() >= 7;
	for (int32 Index = 1; Index < Shots.Num(); ++Index)
	{
		bSteady &= FMath::IsNearlyEqual(Shots[Index] - Shots[Index - 1], Pump, 1e-6);
	}
	TestTrue(FString::Printf(TEXT("Mashing for 6 s fires at the gun's rate (%d shots)"), Shots.Num()), bSteady);

	// Whatever the clicking, the rate is respected, no shot outruns the clicks, and nothing goes off late: every shot
	// comes at a click, or within the buffer after the click that was remembered.
	FRandomStream Random(20261013);
	int32 BadRate = 0;
	int32 TooMany = 0;
	int32 Late = 0;
	for (int32 Trial = 0; Trial < 600; ++Trial)
	{
		const EWeaponFireMode Mode = Trial % 2 == 0 ? EWeaponFireMode::SemiAuto : EWeaponFireMode::Burst;
		const double Interval = Random.FRandRange(0.2f, 1.2f);
		TArray<double> Presses;
		double Time = Random.FRandRange(0.f, 0.5f);
		for (int32 Press = Random.RandRange(1, 30); Press > 0; --Press)
		{
			Presses.Add(Time);
			Time += Random.FRandRange(0.01f, 0.45f);
		}
		const TArray<double> Fired = PlayClicks(Mode, Interval, Presses);
		TooMany += Fired.Num() > Presses.Num() ? 1 : 0;
		for (int32 Index = 0; Index < Fired.Num(); ++Index)
		{
			BadRate += Index > 0 && Fired[Index] - Fired[Index - 1] < Interval - 1e-9 ? 1 : 0;
			const double Shot = Fired[Index];
			const bool bAtAClick = Presses.ContainsByPredicate([Shot](double Press) { return FMath::IsNearlyEqual(Press, Shot, 1e-9); });
			const bool bAfterAClick = Presses.ContainsByPredicate([Shot](double Press) { return Press <= Shot && Shot <= Press + BufferSeconds + 1e-9; });
			Late += bAtAClick || bAfterAClick ? 0 : 1;
		}
	}
	TestEqual(TEXT("Never more shots than clicks"), TooMany, 0);
	TestEqual(TEXT("Never two shots closer than the gun's interval"), BadRate, 0);
	TestEqual(TEXT("Never a shot late: all at a click, or within the buffer after one"), Late, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponTriggerGunTest, "Looter.Weapons.Trigger.Gun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponTriggerGunTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Shotgun = LoadDefinition(TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	UWeaponDefinition* Revolver = LoadDefinition(TEXT("/Game/Weapons/Data/DA_Revolver.DA_Revolver"));
	UWeaponDefinition* Rifle = LoadDefinition(TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Shotgun loads"), Shotgun)
		|| !TestNotNull(TEXT("Revolver loads"), Revolver) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// The level never ticks, so a gun's cooldown is played out by handing it the time of each click.
	const double Start = World->GetTimeSeconds();

	// The pump and the six-gun are semi-automatic: one shot a click.
	for (UWeaponDefinition* Definition : { Shotgun, Revolver })
	{
		const FString Name = Definition->GetName();
		const FWeaponInstanceData Rolled = RollGun(Definition);
		TestEqual(*FString::Printf(TEXT("%s is semi-automatic"), *Name), Definition->FireMode, EWeaponFireMode::SemiAuto);
		AWeaponBase* Gun = SpawnLoose(World, Rolled);
		if (!TestNotNull(*FString::Printf(TEXT("%s spawns"), *Name), Gun))
		{
			continue;
		}
		const double Interval = Gun->GetStats().GetSecondsBetweenShots();
		const int32 Full = Gun->GetCurrentMagazine();
		if (!TestTrue(*FString::Printf(TEXT("%s has room for a buffer in its interval (%.2f s) and rounds to spare"), *Name, Interval),
			Interval > 2.0 * BufferSeconds && Full >= 4))
		{
			continue;
		}

		Gun->StartFireAt(Start);
		Gun->StopFire();
		TestEqual(*FString::Printf(TEXT("%s: the first click fires"), *Name), Gun->GetCurrentMagazine(), Full - 1);
		TestFalse(*FString::Printf(TEXT("%s: nothing waits behind it"), *Name), Gun->HasShotPending());

		// Ten clicks in the cooldown, up to just short of the buffer: none fires, none is kept for later.
		const double Early = Interval - BufferSeconds - 0.02;
		bool bAllIgnored = true;
		for (int32 Click = 1; Click <= 10; ++Click)
		{
			Gun->StartFireAt(Start + Early * Click / 10.0);
			Gun->StopFire();
			bAllIgnored &= !Gun->HasShotPending() && Gun->GetCurrentMagazine() == Full - 1;
		}
		TestTrue(*FString::Printf(TEXT("%s: ten clicks mid-cooldown fire nothing and queue nothing"), *Name), bAllIgnored);

		// A click in the buffer is remembered, once however many follow, and survives letting go (the player did click).
		Gun->StartFireAt(Start + Interval - BufferSeconds * 0.5);
		TestTrue(*FString::Printf(TEXT("%s: a click just before ready is remembered"), *Name), Gun->HasShotPending());
		TestEqual(*FString::Printf(TEXT("%s: ...to fire when ready, not before"), *Name), Gun->GetCurrentMagazine(), Full - 1);
		Gun->StartFireAt(Start + Interval - BufferSeconds * 0.25);
		Gun->StopFire();
		TestTrue(*FString::Printf(TEXT("%s: ...and kept after the trigger is let go"), *Name), Gun->HasShotPending());
		TestEqual(*FString::Printf(TEXT("%s: only the one is kept"), *Name), Gun->GetCurrentMagazine(), Full - 1);

		// The edge of the buffer, on a gun of its own.
		AWeaponBase* Edge = SpawnLoose(World, Rolled);
		if (TestNotNull(*FString::Printf(TEXT("%s: another"), *Name), Edge))
		{
			Edge->StartFireAt(Start);
			Edge->StopFire();
			Edge->StartFireAt(Start + Interval - BufferSeconds - 0.005);
			TestFalse(*FString::Printf(TEXT("%s: a click just outside the buffer is lost"), *Name), Edge->HasShotPending());
			Edge->StartFireAt(Start + Interval - BufferSeconds + 0.005);
			TestTrue(*FString::Printf(TEXT("%s: a click just inside it is remembered"), *Name), Edge->HasShotPending());
		}

		// Once the gun is ready a click fires at once, as it always did.
		AWeaponBase* Ready = SpawnLoose(World, Rolled);
		if (TestNotNull(*FString::Printf(TEXT("%s: and one more"), *Name), Ready))
		{
			Ready->StartFireAt(Start);
			Ready->StopFire();
			Ready->StartFireAt(Start + Interval + 0.01);
			Ready->StopFire();
			TestEqual(*FString::Printf(TEXT("%s: a click once ready fires"), *Name), Ready->GetCurrentMagazine(), Full - 2);
			TestFalse(*FString::Printf(TEXT("%s: ...and a semi-auto gun doesn't go on firing"), *Name), Ready->HasShotPending());
		}

		// A held trigger on a semi-auto gun fires once: it does not repeat.
		AWeaponBase* Held = SpawnLoose(World, Rolled);
		if (TestNotNull(*FString::Printf(TEXT("%s: a last one"), *Name), Held))
		{
			Held->StartFireAt(Start);
			TestTrue(*FString::Printf(TEXT("%s: the trigger is held"), *Name), Held->WantsToFire());
			TestEqual(*FString::Printf(TEXT("%s: holding it fires one shot"), *Name), Held->GetCurrentMagazine(), Full - 1);
			TestFalse(*FString::Printf(TEXT("%s: ...and queues no more"), *Name), Held->HasShotPending());
		}

		// The melee strike still holds the gun's fire back, and a click made mid-swing is not kept for after it.
		AWeaponBase* Swinging = SpawnLoose(World, Rolled);
		if (TestNotNull(*FString::Printf(TEXT("%s: and one for the swing"), *Name), Swinging))
		{
			Swinging->BeginMeleeSwing();
			Swinging->StartFireAt(Start);
			Swinging->StopFire();
			TestTrue(*FString::Printf(TEXT("%s: no shot mid-swing, none kept"), *Name), Swinging->GetCurrentMagazine() == Full && !Swinging->HasShotPending());
			Swinging->EndMeleeSwing(true);
			TestTrue(*FString::Printf(TEXT("%s: ...and none after it"), *Name), Swinging->GetCurrentMagazine() == Full && !Swinging->HasShotPending());
		}
	}

	// The rifle holds its trigger: it waits for the gun, however long, and letting go clears it, so nothing comes late.
	if (Rifle->FireMode == EWeaponFireMode::FullAuto)
	{
		AWeaponBase* Gun = SpawnLoose(World, RollGun(Rifle));
		if (TestNotNull(TEXT("A rifle"), Gun))
		{
			const int32 Full = Gun->GetCurrentMagazine();
			Gun->StartFireAt(Start);
			TestEqual(TEXT("Rifle: the first shot"), Gun->GetCurrentMagazine(), Full - 1);
			TestTrue(TEXT("Rifle: held, the next is on its way"), Gun->HasShotPending());
			Gun->StopFire();
			TestFalse(TEXT("Rifle: let go, nothing is kept"), Gun->HasShotPending());
			Gun->StartFireAt(Start + 0.001);
			TestTrue(TEXT("Rifle: a press mid-cooldown waits for the gun while held"), Gun->HasShotPending());
			Gun->StopFire();
			TestFalse(TEXT("Rifle: let go again, nothing is kept"), Gun->HasShotPending());
			TestEqual(TEXT("Rifle: no extra rounds went"), Gun->GetCurrentMagazine(), Full - 1);
		}
	}

	// A reload still takes the trigger: a click during one fires nothing and keeps nothing.
	APawn* Holder = World->SpawnActor<APawn>();
	if (TestNotNull(TEXT("A holder"), Holder))
	{
		UHealthComponent* Health = NewObject<UHealthComponent>(Holder, TEXT("Health"));
		Health->bShowDamageNumbers = false;
		Holder->AddInstanceComponent(Health);
		Health->RegisterComponent();
		UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
		Holder->AddInstanceComponent(Weapons);
		Weapons->RegisterComponent();
		Weapons->MaxWeapons = 3;
		Holder->DispatchBeginPlay();
		FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Shotgun, EWeaponRarity::Rare, 1);
		Instance.SavedMagazine = 2;
		AWeaponBase* Gun = Weapons->GiveWeapon(Instance);
		if (TestNotNull(TEXT("A shotgun in hand"), Gun))
		{
			Weapons->AddAmmo(Gun->GetAmmoType(), 30);
			Gun->Reload();
			if (TestTrue(TEXT("Reloading"), Gun->IsReloading()))
			{
				Gun->StartFireAt(Start + 5.0);
				Gun->StopFire();
				TestTrue(TEXT("A click mid-reload fires nothing and keeps nothing"), Gun->GetCurrentMagazine() == 2 && !Gun->HasShotPending());
			}
		}
	}
	return true;
}

#endif
