#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/CombatRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Creatures/SpiderCreature.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootDropComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "World/LightBeam.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The gun ideas acting in a test level (the rules alone are Looter.Weapons.Notches and .Curses): kills counted on the
// gun that made them, the cursed irons' drawbacks on the trigger, the reload and the holder's health, and the looks.

namespace
{
	UWeaponDefinition* LoadRifle()
	{
		return LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	}

	UWeaponDefinition* LoadShotgun()
	{
		return LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	}

	/** A rolled Rare rifle (or Definition) with a curse and kills of its choosing. */
	FWeaponInstanceData MakeGun(UWeaponDefinition* Definition, FName Curse = NAME_None, int32 Kills = 0, int32 Seed = 1234)
	{
		FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Definition, EWeaponRarity::Rare, 1);
		Gun.Seed = Seed;
		Gun.Parts.Reset();
		Gun.Curse = Curse;
		Gun.Kills = Kills;
		return Gun;
	}

	/**
	 * A holder as the game has one: a pawn with health (at its class's 100) and a weapon manager, begun play so its health
	 * is full and listening.
	 */
	APawn* SpawnHolder(UWorld* World, UHealthComponent*& OutHealth, UWeaponManagerComponent*& OutWeapons)
	{
		APawn* Holder = World->SpawnActor<APawn>();
		if (!Holder)
		{
			return nullptr;
		}
		OutHealth = NewObject<UHealthComponent>(Holder, TEXT("Health"));
		OutHealth->bShowDamageNumbers = false;
		Holder->AddInstanceComponent(OutHealth);
		OutHealth->RegisterComponent();
		OutWeapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
		Holder->AddInstanceComponent(OutWeapons);
		OutWeapons->RegisterComponent();
		OutWeapons->MaxWeapons = 3;
		Holder->DispatchBeginPlay();
		return Holder;
	}

	/**
	 * Damage as the game's bullets deliver it, from Causer (a gun, or whoever holds one). A test level never readies its
	 * actors for play, so the engine would drop events bound to their own functions: let them run, as in the game.
	 */
	void HurtWith(AActor* Victim, AActor* Causer, float Damage = 1.f)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, GetDefault<UDamageType>(), nullptr, Causer);
	}

	float CustomData(const UPrimitiveComponent* Part, int32 Index)
	{
		const TArray<float>& Data = Part->GetCustomPrimitiveData().Data;
		return Data.IsValidIndex(Index) ? Data[Index] : 0.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunIdeasNotchesPlayTest, "Looter.GunIdeas.Notches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunIdeasNotchesPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	UWeaponManagerComponent* Weapons = nullptr;
	APawn* Holder = SpawnHolder(World, Health, Weapons);
	AWeaponBase* Gun = Holder ? Weapons->GiveWeapon(MakeGun(Rifle)) : nullptr;
	if (!TestNotNull(TEXT("A holder with a gun in hand"), Gun))
	{
		return false;
	}

	// A creature the gun killed counts a notch on it.
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector(500.0, 0.0, 0.0), FRotator::ZeroRotator);
	ATargetDummy* Dummy = World->SpawnActor<ATargetDummy>(FVector(0.0, 500.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestNotNull(TEXT("A dummy"), Dummy))
	{
		return false;
	}
	for (AActor* Target : { static_cast<AActor*>(Spider), static_cast<AActor*>(Dummy) })
	{
		if (ULootDropComponent* Loot = Target->FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		Target->DispatchBeginPlay();
	}
	HurtWith(Spider, Gun);
	TestTrue(TEXT("The gun is what hurt the spider"), AWeaponBase::FindKillWeapon(Spider) == Gun);
	TestTrue(TEXT("The spider's kill counts on the gun"), UPlayerProgressionSubsystem::CreditKillWeapon(Spider, 1) == Gun);
	TestEqual(TEXT("One notch"), Gun->GetInstance().Kills, 1);

	// A dummy counts for nothing: notches can't be farmed on the practice range.
	HurtWith(Dummy, Gun);
	TestNull(TEXT("A dummy's kill counts on no gun"), UPlayerProgressionSubsystem::CreditKillWeapon(Dummy, 1));
	TestEqual(TEXT("Still one notch"), Gun->GetInstance().Kills, 1);

	// Damage dealt by the holder (or something of theirs) counts for the gun in their hand.
	HurtWith(Spider, Holder);
	TestTrue(TEXT("The holder's blow is the gun in hand's"), UPlayerProgressionSubsystem::CreditKillWeapon(Spider, 1) == Gun);
	TestEqual(TEXT("Two notches"), Gun->GetInstance().Kills, 2);

	// A milestone rebuilds the gun's stats: at 50 it's Blooded, +3% damage, and the tally shows ten marks.
	AWeaponBase* Blooding = Weapons->GiveWeapon(MakeGun(Rifle, NAME_None, WeaponNotches::BloodedKills - 1, 777));
	if (!TestNotNull(TEXT("A gun one kill short of Blooded"), Blooding))
	{
		return false;
	}
	const float DamageBefore = Blooding->GetStats().Damage;
	Blooding->AddKill();
	TestTrue(TEXT("Blooded"), WeaponNotches::TierFor(Blooding->GetInstance().Kills) == ENotchTier::Blooded);
	const float Expected = WeaponNotches::DamageMultiplier(WeaponNotches::BloodedKills) / WeaponNotches::DamageMultiplier(WeaponNotches::BloodedKills - 1);
	TestNearlyEqual(*FString::Printf(TEXT("Its damage grew by the milestone (%.2f -> %.2f)"), DamageBefore, Blooding->GetStats().Damage),
		Blooding->GetStats().Damage / FMath::Max(DamageBefore, 0.01f), Expected, 0.001f);
	const UWeaponModelComponent* Model = Blooding->FindComponentByClass<UWeaponModelComponent>();
	const UStaticMeshComponent* Tallied = Model ? Model->GetNotchPart() : nullptr;
	if (TestNotNull(TEXT("The gun has a part to cut the tally in"), Tallied))
	{
		TestEqual(TEXT("Ten marks cut at 50 kills"), CustomData(Tallied, UWeaponModelComponent::NotchMarksDataIndex),
			static_cast<float>(WeaponNotches::Marks(WeaponNotches::BloodedKills)));
		TestTrue(TEXT("Its row has a length"), CustomData(Tallied, UWeaponModelComponent::NotchRowDataIndex) != CustomData(Tallied, UWeaponModelComponent::NotchRowDataIndex + 2)
			|| CustomData(Tallied, UWeaponModelComponent::NotchRowDataIndex + 1) != CustomData(Tallied, UWeaponModelComponent::NotchRowDataIndex + 3));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunIdeasCurseShotsTest, "Looter.GunIdeas.CurseShots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunIdeasCurseShotsTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}

	// Unlucky misfires one shot in eight, from a seeded stream; lifted, or another gun, never.
	const FWeaponInstanceData Unlucky = MakeGun(Rifle, TEXT("Unlucky"));
	FRandomStream Rolls(20261008);
	constexpr int32 Shots = 8000;
	int32 Misfires = 0;
	for (int32 Shot = 0; Shot < Shots; ++Shot)
	{
		Misfires += WeaponCurseEffects::PlanShot(Unlucky, 30, Rolls).bMisfire ? 1 : 0;
	}
	const float Rate = static_cast<float>(Misfires) / Shots;
	TestTrue(FString::Printf(TEXT("About one in eight misfires (%.3f)"), Rate), Rate > 0.11f && Rate < 0.14f);
	FWeaponInstanceData Lifted = Unlucky;
	Lifted.bCurseLifted = true;
	const FWeaponInstanceData Plain = MakeGun(Rifle);
	const int32 SeedBefore = Rolls.GetCurrentSeed();
	bool bAnyMisfire = false;
	for (int32 Shot = 0; Shot < 500; ++Shot)
	{
		bAnyMisfire |= WeaponCurseEffects::PlanShot(Lifted, 30, Rolls).bMisfire || WeaponCurseEffects::PlanShot(Plain, 30, Rolls).bMisfire;
	}
	TestFalse(TEXT("A lifted curse and a plain gun never misfire"), bAnyMisfire);
	TestEqual(TEXT("...and never draw from the stream"), Rolls.GetCurrentSeed(), SeedBefore);

	// Greedy spends two rounds a shot, or the last one alone.
	const FWeaponInstanceData Greedy = MakeGun(Rifle, TEXT("Greedy"));
	TestEqual(TEXT("Greedy: two rounds a shot"), WeaponCurseEffects::PlanShot(Greedy, 10, Rolls).Rounds, 2);
	TestEqual(TEXT("Greedy: the last round alone"), WeaponCurseEffects::PlanShot(Greedy, 1, Rolls).Rounds, 1);
	TestEqual(TEXT("A plain gun: one round a shot"), WeaponCurseEffects::PlanShot(Plain, 10, Rolls).Rounds, 1);

	// ...on a real gun's trigger. Loose guns, so no draw time stands between the trigger and the shot.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AWeaponBase* GreedyGun = UWeaponRollLibrary::SpawnWeapon(World, Greedy, FTransform::Identity);
	FWeaponInstanceData LastRound = Greedy;
	LastRound.SavedMagazine = 1;
	AWeaponBase* LastRoundGun = UWeaponRollLibrary::SpawnWeapon(World, LastRound, FTransform::Identity);
	if (TestNotNull(TEXT("A Greedy gun"), GreedyGun) && TestNotNull(TEXT("A Greedy gun with one round"), LastRoundGun))
	{
		const int32 Full = GreedyGun->GetCurrentMagazine();
		GreedyGun->StartFire();
		GreedyGun->StopFire();
		TestEqual(TEXT("One pull spent two rounds"), GreedyGun->GetCurrentMagazine(), Full - 2);
		LastRoundGun->StartFire();
		LastRoundGun->StopFire();
		TestEqual(TEXT("The last round fired alone"), LastRoundGun->GetCurrentMagazine(), 0);
	}

	// Restless's doubled recoil reaches the kick the view and the gun read.
	AWeaponBase* RestlessGun = UWeaponRollLibrary::SpawnWeapon(World, MakeGun(Rifle, TEXT("Restless")), FTransform::Identity);
	AWeaponBase* PlainGun = UWeaponRollLibrary::SpawnWeapon(World, Plain, FTransform::Identity);
	if (TestNotNull(TEXT("A Restless gun and its plain twin"), RestlessGun) && PlainGun && PlainGun->GetRecoilProfile().MuzzleFlip > 0.f)
	{
		TestNearlyEqual(TEXT("Restless kicks twice as hard"), RestlessGun->GetRecoilProfile().MuzzleFlip / PlainGun->GetRecoilProfile().MuzzleFlip, 2.f, 0.01f);
	}

	// Critical hits use the gun's curse's multiplier: Unlucky's triple, Cold's extra quarter, the game's 1.5x otherwise.
	const float Roll = 0.5f;
	TestNearlyEqual(TEXT("Unlucky's crit triples"), LooterCombat::HitDamage(100.f, true, Roll, WeaponCurses::CritMultiplier(Unlucky)), 300.f, 0.01f);
	TestNearlyEqual(TEXT("Cold's crit"), LooterCombat::HitDamage(100.f, true, Roll, WeaponCurses::CritMultiplier(MakeGun(Rifle, TEXT("Cold")))), 187.5f, 0.01f);
	TestNearlyEqual(TEXT("A plain gun's crit"), LooterCombat::HitDamage(100.f, true, Roll, WeaponCurses::CritMultiplier(Plain)), 150.f, 0.01f);
	TestNearlyEqual(TEXT("No crit, no multiplier"), LooterCombat::HitDamage(100.f, false, Roll, WeaponCurses::CritMultiplier(Unlucky)), 100.f, 0.01f);
	TestEqual(TEXT("A plain shot's default is the game's rule"), FBulletShot().CritMultiplier, LooterCombat::CriticalHitMultiplier);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunIdeasCurseHealthTest, "Looter.GunIdeas.CurseHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunIdeasCurseHealthTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadRifle();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	UWeaponManagerComponent* Weapons = nullptr;
	APawn* Holder = SpawnHolder(World, Health, Weapons);
	AWeaponBase* PlainGun = Holder ? Weapons->GiveWeapon(MakeGun(Rifle)) : nullptr;
	// One kill short of lifting its curse (below).
	AWeaponBase* Grasping = Holder ? Weapons->GiveWeapon(MakeGun(Rifle, TEXT("Grasping"), WeaponNotches::CurseLiftKills - 1)) : nullptr;
	if (!TestNotNull(TEXT("A plain gun in hand"), PlainGun) || !TestNotNull(TEXT("A Grasping gun put away"), Grasping))
	{
		return false;
	}
	const float Full = Health->GetMaxHealth();
	TestNearlyEqual(TEXT("Put away, Grasping takes nothing"), Health->GetMaxHealth(), Full, 0.01f);

	// In hand: 15% less max health, and health keeps its share.
	Weapons->EquipSlot(1);
	TestTrue(TEXT("Grasping in hand"), Weapons->GetActiveWeapon() == Grasping);
	TestNearlyEqual(TEXT("Max health 85% with Grasping in hand"), Health->GetMaxHealth(), Full * 0.85f, 0.01f);
	TestNearlyEqual(TEXT("Still at full health"), Health->GetHealth(), Health->GetMaxHealth(), 0.01f);
	Health->SetHealth(Health->GetMaxHealth() * 0.5f);
	Weapons->EquipSlot(0);
	TestNearlyEqual(TEXT("Back to full max health with it put away"), Health->GetMaxHealth(), Full, 0.01f);
	TestNearlyEqual(TEXT("Still half health: swapping neither heals nor hurts"), Health->GetHealth(), Full * 0.5f, 0.01f);

	// The level's health and the curse's both hold.
	Weapons->EquipSlot(1);
	UPlayerProgressionSubsystem::ApplyLevelHealth(*Health, 5);
	const float Level5 = GetDefault<UHealthComponent>()->MaxHealth * UPlayerProgressionSubsystem::GetLevelRules().PlayerHealthScale(5);
	TestNearlyEqual(TEXT("A level-up with Grasping in hand: the level's health, less 15%"), Health->GetMaxHealth(), Level5 * 0.85f, 0.05f);
	Weapons->EquipSlot(0);
	TestNearlyEqual(TEXT("...and the level's health whole once it's put away"), Health->GetMaxHealth(), Level5, 0.05f);

	// Swapping never kills, however low.
	Weapons->EquipSlot(1);
	Health->SetHealth(1.f);
	Weapons->EquipSlot(0);
	Weapons->EquipSlot(1);
	TestTrue(FString::Printf(TEXT("Swapping at 1 health never kills (%.2f)"), Health->GetHealth()), !Health->IsDead() && Health->GetHealth() > 0.f);

	// Grasping's kills heal 5% of max health.
	Health->SetHealth(Health->GetMaxHealth() * 0.5f);
	const float BeforeHeal = Health->GetHealth();
	WeaponCurseEffects::ApplyKillPerks(*Grasping);
	TestNearlyEqual(TEXT("A Grasping kill heals 5% of max health"), Health->GetHealth() - BeforeHeal, Health->GetMaxHealth() * 0.05f, 0.01f);

	// The 100th notch lifts the curse in hand, and the max health comes back with it.
	Grasping->AddKill();
	TestTrue(TEXT("Lifted at 100 notches"), Grasping->GetInstance().bCurseLifted);
	TestNearlyEqual(TEXT("Lifted: the full max health in hand"), Health->GetMaxHealth(), Level5, 0.05f);

	// Hungry's reload costs 3% of max health, and never the last point.
	AWeaponBase* Hungry = Weapons->GiveWeapon(MakeGun(Rifle, TEXT("Hungry")));
	if (TestNotNull(TEXT("A Hungry gun"), Hungry))
	{
		Weapons->EquipSlot(2);
		Health->SetHealth(Health->GetMaxHealth());
		WeaponCurseEffects::PayReload(*Hungry);
		TestNearlyEqual(TEXT("A reload costs 3% of max health"), Health->GetHealth(), Health->GetMaxHealth() * 0.97f, 0.01f);
		Health->SetHealth(2.f);
		WeaponCurseEffects::PayReload(*Hungry);
		WeaponCurseEffects::PayReload(*Hungry);
		TestTrue(FString::Printf(TEXT("Reloads at 2 health leave at least 1 (%.2f)"), Health->GetHealth()), !Health->IsDead() && Health->GetHealth() >= 1.f - KINDA_SMALL_NUMBER);
		WeaponCurseEffects::PayReload(*PlainGun);
		TestTrue(TEXT("A plain gun's reload costs nothing"), FMath::IsNearlyEqual(WeaponCurseEffects::ReloadHealthCost(PlainGun->GetInstance(), 100.f), 0.f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunIdeasLooksTest, "Looter.GunIdeas.Looks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunIdeasLooksTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadRifle();
	UWeaponDefinition* Shotgun = LoadShotgun();
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle)
		|| !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();

	// Every bullpup body and every Ranchhand stock has its tally laid out (the bullpup's stocks are pads: its body takes it),
	// and a gun built with it cuts the tally there.
	struct FTallySlot
	{
		UWeaponDefinition* Definition;
		FName Slot;
	};
	for (const FTallySlot& Tally : { FTallySlot{ Rifle, TEXT("Body") }, FTallySlot{ Shotgun, TEXT("Stock") } })
	{
		const int32 SlotIndex = Tally.Definition->Parts.IndexOfByPredicate([&Tally](const FWeaponPartSlot& Slot) { return Slot.Name == Tally.Slot; });
		if (!TestTrue(FString::Printf(TEXT("%s has a %s slot"), *Tally.Definition->GetName(), *Tally.Slot.ToString()), SlotIndex != INDEX_NONE))
		{
			continue;
		}
		for (const FWeaponPartOption& Option : Tally.Definition->Parts[SlotIndex].Options)
		{
			const UStaticMesh* Mesh = Option.Mesh.Get();
			TestTrue(FString::Printf(TEXT("%s has a tally row"), *GetNameSafe(Mesh)), UWeaponModelComponent::HasTallyRow(Mesh));
			// A legendary allows every part; the slot gets this one.
			FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Tally.Definition, EWeaponRarity::Legendary, 1);
			if (Gun.Parts.IsEmpty())
			{
				Gun.Parts = WeaponParts::PartKeys(WeaponParts::Pick(*Tally.Definition, Gun.Seed, Gun.Rarity));
			}
			if (Gun.Parts.IsValidIndex(SlotIndex))
			{
				Gun.Parts[SlotIndex] = Option.Key;
			}
			Gun.Kills = 37;
			AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform::Identity);
			const UWeaponModelComponent* Model = Weapon ? Weapon->FindComponentByClass<UWeaponModelComponent>() : nullptr;
			const UStaticMeshComponent* Tallied = Model ? Model->GetNotchPart() : nullptr;
			TestTrue(FString::Printf(TEXT("The tally is cut in %s"), *GetNameSafe(Mesh)), Tallied && Tallied->GetStaticMesh() == Mesh);
			if (Tallied)
			{
				TestEqual(TEXT("37 kills: seven marks"), CustomData(Tallied, UWeaponModelComponent::NotchMarksDataIndex), 7.f);
				TestEqual(TEXT("No soul-light below 1,000"), CustomData(Tallied, UWeaponModelComponent::SoulLightDataIndex + 3), 0.f);
			}
		}
	}

	// A soul-forged gun carries its soul-light on every part, in its rarity's color; the row is full at 25 marks.
	AWeaponBase* Forged = UWeaponRollLibrary::SpawnWeapon(World, MakeGun(Shotgun, NAME_None, WeaponNotches::SoulForgedKills), FTransform::Identity);
	const UWeaponModelComponent* ForgedModel = Forged ? Forged->FindComponentByClass<UWeaponModelComponent>() : nullptr;
	if (TestNotNull(TEXT("A soul-forged shotgun"), ForgedModel))
	{
		const FLinearColor Rarity = UWeaponRollLibrary::GetRarityColor(Shotgun, EWeaponRarity::Rare);
		bool bAllLit = !ForgedModel->GetParts().IsEmpty();
		for (const UStaticMeshComponent* Part : ForgedModel->GetParts())
		{
			bAllLit &= FMath::IsNearlyEqual(CustomData(Part, UWeaponModelComponent::SoulLightDataIndex + 3), UWeaponModelComponent::SoulLightGlow)
				&& FMath::IsNearlyEqual(CustomData(Part, UWeaponModelComponent::SoulLightDataIndex), Rarity.R);
		}
		TestTrue(TEXT("Every part glows in its rarity's color"), bAllLit);
		const UStaticMeshComponent* Tallied = ForgedModel->GetNotchPart();
		TestTrue(TEXT("The row is full at 25 marks"), Tallied && CustomData(Tallied, UWeaponModelComponent::NotchMarksDataIndex) == static_cast<float>(WeaponNotches::MaxMarks));
	}

	// A cursed gun's loot beam gutters: low, sputtering nearly out now and then, catching again.
	float Lowest = 1.f;
	float Highest = 0.f;
	for (float Time = 0.f; Time < 20.f; Time += 1.f / 60.f)
	{
		const float Strength = LightBeams::GutterStrength(Time);
		Lowest = FMath::Min(Lowest, Strength);
		Highest = FMath::Max(Highest, Strength);
	}
	TestTrue(FString::Printf(TEXT("It sputters nearly out (%.2f) and burns up again (%.2f)"), Lowest, Highest), Lowest < 0.3f && Highest > 0.7f && Lowest > 0.f);

	// The reload's sounds land on its motion, in order: the magazine out, in, the bolt; four shells, then the pump.
	const TConstArrayView<FReloadStepAt> Magazine = LooterReload::Steps(EWeaponReloadPart::Magazine);
	const TConstArrayView<FReloadStepAt> Pump = LooterReload::Steps(EWeaponReloadPart::Pump);
	TestTrue(TEXT("Magazine: out, in, bolt"), Magazine.Num() == 3 && Magazine[0].Step == EReloadStep::MagOut && Magazine[1].Step == EReloadStep::MagIn
		&& Magazine[2].Step == EReloadStep::Bolt);
	TestTrue(TEXT("Pump: four shells, then the pump"), Pump.Num() == 5 && Pump[0].Step == EReloadStep::ShellIn && Pump[4].Step == EReloadStep::Pump);
	for (const TConstArrayView<FReloadStepAt>& Steps : { Magazine, Pump })
	{
		for (int32 Index = 0; Index < Steps.Num(); ++Index)
		{
			TestTrue(TEXT("Each step inside the reload, after the one before"), Steps[Index].Progress > 0.f && Steps[Index].Progress < 1.f
				&& (Index == 0 || Steps[Index].Progress > Steps[Index - 1].Progress));
		}
	}
	TestEqual(TEXT("No steps without a moving part"), LooterReload::Steps(EWeaponReloadPart::None).Num(), 0);

	// Impacts: creatures make their own hit sound, dummies ring, the world thuds.
	const FHitResult Nothing;
	TestTrue(TEXT("A creature's hit: its own sound"), UBulletSubsystem::ImpactCueOf(Nothing, EImpactSurface::Flesh).IsNone());
	TestTrue(TEXT("A dummy rings"), UBulletSubsystem::ImpactCueOf(Nothing, EImpactSurface::Target) == FName(LooterSoundCue::ImpactMetal));
	TestTrue(TEXT("The world thuds"), UBulletSubsystem::ImpactCueOf(Nothing, EImpactSurface::World) == FName(LooterSoundCue::ImpactWorld));
	return true;
}

#endif
