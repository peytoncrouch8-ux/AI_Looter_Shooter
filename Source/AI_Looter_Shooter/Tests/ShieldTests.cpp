#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "HAL/IConsoleManager.h"
#include "Inventory/WeaponInventorySave.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "Shields/ShieldTuning.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RecoveryTestWorld.h"
#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudShieldBarWidget.h"
#include "UI/Inventory/LoadoutWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Weapons/WeaponDefinition.h"

// The player's ward (SHIELD, 2026-10-22; Docs/Work/2026-10-22-Next/SHIELD.md): its numbers and plain rules, the component
// standing in front of a real health component, the loot split, the save, and where the HUD's bar sits. Rules run on a
// default-made tuning and a definition made here, never on what's set in PIE or on the asset.

namespace
{
	constexpr EAutomationTestFlags ShieldTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** A plain ward of Rarity, level, seed and build on a definition made for the test (the shipped numbers). */
	FShieldInstanceData TestWard(UShieldDefinition* Definition, EWeaponRarity Rarity = EWeaponRarity::Common, int32 Level = 1, int32 Seed = 7,
		EShieldBuild Build = EShieldBuild::Standard, EShieldTrait Trait = EShieldTrait::None)
	{
		return ShieldRules::MakeWith(Definition, Rarity, Level, Seed, Build, Trait);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldTuningTest, "Looter.Shields.Tuning", ShieldTestFlags)

bool FShieldTuningTest::RunTest(const FString& Parameters)
{
	// The shipped numbers, and every one a live cvar.
	const FShieldTuning Tuning;
	TestEqual(TEXT("On"), Tuning.Enable, 1);
	TestEqual(TEXT("A break lets half its overflow through"), Tuning.BreakSpill, 0.5f);
	TestEqual(TEXT("A fifth of the guns a table drops come as wards"), Tuning.DropShare, 0.2f);
	TestEqual(TEXT("Health waits for the ward"), Tuning.HealthWaits, 1);
	TestEqual(TEXT("The first gun brings a ward"), Tuning.Starter, 1);
	for (const TCHAR* Name : { TEXT("Looter.Shield.Enable"), TEXT("Looter.Shield.CapacityScale"), TEXT("Looter.Shield.DelayScale"),
		TEXT("Looter.Shield.RateScale"), TEXT("Looter.Shield.BreakSpill"), TEXT("Looter.Shield.HealthWaits"), TEXT("Looter.Shield.Drop.Share"),
		TEXT("Looter.Shield.Starter"), TEXT("Looter.Shield.Nova.Damage"), TEXT("Looter.Shield.Nova.Radius"), TEXT("Looter.Shield.Nova.Core"),
		TEXT("Looter.Shield.Nova.Edge"), TEXT("Looter.Shield.Nova.BossCap"), TEXT("Looter.Shield.Barb.Share"), TEXT("Looter.Shield.Lodestone.Chance"),
		TEXT("Looter.Shield.Lodestone.Share"), TEXT("Looter.Shield.Harvest.Share"), TEXT("Looter.Shield.Grace.Below"), TEXT("Looter.Shield.Grace.Refill"),
		TEXT("Looter.Shield.HitSoundGap"), TEXT("Looter.Shield.Hud.ChipHold"), TEXT("Looter.Shield.Hud.ChipDrain"), TEXT("Looter.Shield.Hud.FlashSeconds"),
		TEXT("Looter.Shield.Hud.ShimmerSeconds") })
	{
		TestNotNull(FString::Printf(TEXT("%s is a console variable"), Name), IConsoleManager::Get().FindConsoleVariable(Name));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldStatsTest, "Looter.Shields.Stats", ShieldTestFlags)

bool FShieldStatsTest::RunTest(const FString& Parameters)
{
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	const FShieldInstanceData Common = TestWard(Definition);
	// A Common ward at level 1: 40 (+-8%) against 100 health, waits 3.2 s (+-8%), refills about 28% of itself a second.
	TestTrue(FString::Printf(TEXT("Common, level 1: about 40 (%.1f)"), Common.Stats.Capacity), FMath::Abs(Common.Stats.Capacity - 40.f) <= 40.f * 0.081f);
	TestTrue(FString::Printf(TEXT("...waits about 3.2 s (%.2f)"), Common.Stats.RechargeDelay), FMath::Abs(Common.Stats.RechargeDelay - 3.2f) <= 3.2f * 0.081f);
	const float Share = Common.Stats.RechargeRate / Common.Stats.Capacity;
	TestTrue(FString::Printf(TEXT("...refills about 28%% a second (%.3f)"), Share), FMath::Abs(Share - 0.28f) <= 0.28f * 0.081f);

	// The same seed: the same numbers, every time; another level grows the capacity 8% a level as health does.
	TestEqual(TEXT("Seeded: the same twice"), TestWard(Definition).Stats.Capacity, Common.Stats.Capacity);
	const FShieldInstanceData Eleven = TestWard(Definition, EWeaponRarity::Common, 11);
	TestEqual(TEXT("Level 11: 1.8x level 1's"), Eleven.Stats.Capacity, Common.Stats.Capacity * 1.8f, 0.01f);
	TestEqual(TEXT("...its wait unchanged"), Eleven.Stats.RechargeDelay, Common.Stats.RechargeDelay, 0.0001f);

	// Rarity climbs: a Legendary holds 1.75x a Common of its seed, wakes in 0.8 of the time and refills a larger share.
	const FShieldInstanceData Legendary = TestWard(Definition, EWeaponRarity::Legendary);
	TestEqual(TEXT("Legendary: 1.75x the capacity"), Legendary.Stats.Capacity, Common.Stats.Capacity * 1.75f, 0.01f);
	TestEqual(TEXT("...0.8x the wait"), Legendary.Stats.RechargeDelay, Common.Stats.RechargeDelay * 0.8f, 0.001f);
	TestEqual(TEXT("...1.2x the refill share"), Legendary.Stats.RechargeRate / Legendary.Stats.Capacity, Share * 1.2f, 0.001f);

	// Builds lean the numbers (the variance is the seed's, so the leans show exactly).
	const FShieldInstanceData Bulwark = TestWard(Definition, EWeaponRarity::Common, 1, 7, EShieldBuild::Bulwark);
	const FShieldInstanceData Quickset = TestWard(Definition, EWeaponRarity::Common, 1, 7, EShieldBuild::Quickset);
	TestEqual(TEXT("Bulwark: a fifth more ward"), Bulwark.Stats.Capacity, Common.Stats.Capacity * 1.2f, 0.01f);
	TestTrue(TEXT("...slower to wake"), Bulwark.Stats.RechargeDelay > Common.Stats.RechargeDelay);
	TestEqual(TEXT("Quickset: wakes in 0.7 of the time"), Quickset.Stats.RechargeDelay, Common.Stats.RechargeDelay * 0.7f, 0.001f);
	TestTrue(TEXT("...holds less"), Quickset.Stats.Capacity < Common.Stats.Capacity);

	// Traits: none below Rare; half the Rares, every Legendary; each of the five turns up; only Last Breath has an element.
	int32 LowTraits = 0;
	int32 RareTraits = 0;
	TSet<EShieldTrait> Seen;
	int32 Elementless = 0;
	constexpr int32 Seeds = 600;
	for (int32 Seed = 0; Seed < Seeds; ++Seed)
	{
		LowTraits += ShieldRules::Make(Definition, EWeaponRarity::Uncommon, 1, Seed).Trait != EShieldTrait::None ? 1 : 0;
		RareTraits += ShieldRules::Make(Definition, EWeaponRarity::Rare, 1, Seed).Trait != EShieldTrait::None ? 1 : 0;
		const FShieldInstanceData Top = ShieldRules::Make(Definition, EWeaponRarity::Legendary, 1, Seed);
		Seen.Add(Top.Trait);
		const bool bNova = Top.Trait == EShieldTrait::LastBreath;
		Elementless += (bNova == (Top.Element == ELooterElement::None)) ? 1 : 0;
	}
	TestEqual(TEXT("No Common or Uncommon ward has a trait"), LowTraits, 0);
	TestTrue(FString::Printf(TEXT("About half the Rares have one (%d of %d)"), RareTraits, Seeds), RareTraits > Seeds * 0.4f && RareTraits < Seeds * 0.6f);
	TestFalse(TEXT("Every Legendary has one"), Seen.Contains(EShieldTrait::None));
	TestEqual(TEXT("...and all five turn up"), Seen.Num(), static_cast<int32>(UE_ARRAY_COUNT(AllShieldTraits)));
	TestEqual(TEXT("Only Last Breath carries an element, and it always does"), Elementless, 0);
	// Its nova is any of the five elements, alike (Thunder and Rot since 2026-10-29).
	TSet<ELooterElement> NovaElements;
	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		NovaElements.Add(ShieldRules::NovaElementFor(Seed));
	}
	TestEqual(TEXT("Last Breath's nova: all five elements turn up"), NovaElements.Num(), 5);
	TestFalse(TEXT("...never none"), NovaElements.Contains(ELooterElement::None));

	// The starting ward: a plain Common ward box at the level given.
	const FShieldInstanceData Starter = ShieldRules::Starter(Definition, 4);
	TestTrue(TEXT("The starting ward: Common, Standard, no trait, level 4"), Starter.Rarity == EWeaponRarity::Common && Starter.Build == EShieldBuild::Standard
		&& Starter.Trait == EShieldTrait::None && Starter.Level == 4);
	TestEqual(TEXT("...named Ward Box"), ShieldText::Name(Starter), FString(TEXT("Ward Box")));
	TestEqual(TEXT("A Barbed Bulwark is named so"), ShieldText::Name(TestWard(Definition, EWeaponRarity::Rare, 1, 3, EShieldBuild::Bulwark, EShieldTrait::Barbwire)),
		FString(TEXT("Barbed Bulwark")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldRulesTest, "Looter.Shields.Rules", ShieldTestFlags)

bool FShieldRulesTest::RunTest(const FString& Parameters)
{
	const FShieldTuning Tuning;

	// Soaking: all of a hit while the ward holds more; the breaking hit lets half its overflow through; empty takes nothing.
	ShieldRules::FShieldSoak Soak = ShieldRules::Soak(40.f, 10.f, Tuning.BreakSpill);
	TestTrue(TEXT("40 soaks 10: 30 left, nothing through"), FMath::IsNearlyEqual(Soak.ChargeAfter, 30.f) && Soak.ToHealth == 0.f && !Soak.bBroke);
	Soak = ShieldRules::Soak(10.f, 30.f, Tuning.BreakSpill);
	TestTrue(TEXT("10 meets 30: broken, 10 through (half the 20 over)"), Soak.bBroke && Soak.ChargeAfter == 0.f && FMath::IsNearlyEqual(Soak.ToHealth, 10.f)
		&& FMath::IsNearlyEqual(Soak.Absorbed, 10.f));
	Soak = ShieldRules::Soak(0.f, 20.f, Tuning.BreakSpill);
	TestTrue(TEXT("Empty: all 20 through, no second break"), FMath::IsNearlyEqual(Soak.ToHealth, 20.f) && !Soak.bBroke);
	Soak = ShieldRules::Soak(25.f, 25.f, 1.f);
	TestTrue(TEXT("A hit of exactly what's left breaks it, nothing over"), Soak.bBroke && Soak.ToHealth == 0.f);

	// Filling: the wait first, any hit starts it over, then it fills at its rate and says so.
	{
		ShieldRules::FShieldCharge Charge;
		Charge.Hurt();
		TestEqual(TEXT("3 s into a 3.2 s wait: nothing"), static_cast<int32>(Charge.Step(3.f, 40.f, 3.2f, 10.f)), static_cast<int32>(ShieldRules::ERefill::None));
		TestEqual(TEXT("...still empty"), Charge.Charge, 0.f);
		TestEqual(TEXT("0.5 s on: it starts"), static_cast<int32>(Charge.Step(0.5f, 40.f, 3.2f, 10.f)), static_cast<int32>(ShieldRules::ERefill::Started));
		TestEqual(TEXT("...the 0.3 s past the wait filled 3"), Charge.Charge, 3.f, 0.001f);
		Charge.Hurt();
		TestEqual(TEXT("A hit: the whole wait again"), static_cast<int32>(Charge.Step(3.f, 40.f, 3.2f, 10.f)), static_cast<int32>(ShieldRules::ERefill::None));
		Charge.Step(0.2f, 40.f, 3.2f, 10.f);
		TestEqual(TEXT("Filled in steps: 10 s more reaches full"), static_cast<int32>(Charge.Step(10.f, 40.f, 3.2f, 10.f)), static_cast<int32>(ShieldRules::ERefill::Filled));
		TestTrue(TEXT("...full at 40, not filling"), Charge.IsFull(40.f) && !Charge.bFilling && FMath::IsNearlyEqual(Charge.Charge, 40.f));
		TestEqual(TEXT("Full: a step does nothing"), static_cast<int32>(Charge.Step(1.f, 40.f, 3.2f, 10.f)), static_cast<int32>(ShieldRules::ERefill::None));
		Charge.Step(0.f, 30.f, 3.2f, 10.f);
		TestEqual(TEXT("A smaller capacity: held at it"), Charge.Charge, 30.f);
	}

	// Last Breath: whole within its core, falling to its edge share at its reach, nothing past; strength and level scale it.
	TestEqual(TEXT("Nova at its heart: 260"), ShieldRules::NovaDamageAt(0.f, 1.f, 1.f, Tuning), 260.f, 0.01f);
	TestEqual(TEXT("...at its reach: 35%"), ShieldRules::NovaDamageAt(Tuning.NovaRadius, 1.f, 1.f, Tuning), 260.f * 0.35f, 0.01f);
	TestEqual(TEXT("...past it: nothing"), ShieldRules::NovaDamageAt(Tuning.NovaRadius + 1.f, 1.f, 1.f, Tuning), 0.f);
	TestEqual(TEXT("...an Epic's at level 6 (x1.25, x1.4)"), ShieldRules::NovaDamageAt(0.f, 1.25f, 1.4f, Tuning), 260.f * 1.25f * 1.4f, 0.01f);

	// The other traits' numbers.
	TestEqual(TEXT("Barbwire on a 50 ward: 40 a bite"), ShieldRules::BarbDamage(50.f, 1.f, Tuning), 40.f, 0.001f);
	TestEqual(TEXT("Lodestone: 40% a hit"), ShieldRules::LodestoneOdds(1.f, Tuning), 0.4f, 0.0001f);
	TestEqual(TEXT("...never past 90%"), ShieldRules::LodestoneOdds(5.f, Tuning), 0.9f, 0.0001f);
	TestEqual(TEXT("...a 30-round magazine: 3 rounds"), ShieldRules::LodestoneRounds(30, 1.f, Tuning), 3);
	TestEqual(TEXT("...a 5-round one: at least 1"), ShieldRules::LodestoneRounds(5, 1.f, Tuning), 1);
	TestEqual(TEXT("...no gun: none"), ShieldRules::LodestoneRounds(0, 1.f, Tuning), 0);
	TestEqual(TEXT("Harvest on a 50 ward: 10 a kill"), ShieldRules::HarvestRestore(50.f, 1.f, Tuning), 10.f, 0.001f);
	TestTrue(TEXT("Saint's Grace: broken and under a third"), ShieldRules::GraceAnswers(true, 0.f, 0.25f, Tuning));
	TestFalse(TEXT("...not with ward left"), ShieldRules::GraceAnswers(true, 5.f, 0.25f, Tuning));
	TestFalse(TEXT("...not above a third"), ShieldRules::GraceAnswers(true, 0.f, 0.5f, Tuning));
	TestFalse(TEXT("...not once used"), ShieldRules::GraceAnswers(false, 0.f, 0.25f, Tuning));
	TestFalse(TEXT("...not dead"), ShieldRules::GraceAnswers(true, 0.f, 0.f, Tuning));
	TestEqual(TEXT("...back to 60%"), ShieldRules::GraceCharge(50.f, 1.f, Tuning), 30.f, 0.001f);

	// Comparing: more ward is better, within 3% is the same, anything beats none.
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	const FShieldInstanceData Common = TestWard(Definition);
	const FShieldInstanceData Epic = TestWard(Definition, EWeaponRarity::Epic);
	TestEqual(TEXT("An Epic beats a Common"), ShieldRules::Compare(Epic, &Common), 1);
	TestEqual(TEXT("...a Common is weaker"), ShieldRules::Compare(Common, &Epic), -1);
	TestEqual(TEXT("...the same ward: the same"), ShieldRules::Compare(Common, &Common), 0);
	TestEqual(TEXT("...anything beats none"), ShieldRules::Compare(Common, nullptr), 1);
	TestTrue(TEXT("Two wards apart are told apart"), ShieldRules::Identity(Common) != ShieldRules::Identity(Epic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldOnHealthTest, "Looter.Shields.OnHealth", ShieldTestFlags)

bool FShieldOnHealthTest::RunTest(const FString& Parameters)
{
	// The component in front of a real health component: hits soak, the break lets half through, the wait and the fill.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	ACharacter* Player = RecoveryTestWorld::SpawnPlayer(World, FVector::ZeroVector, Health);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Its health"), Health))
	{
		return false;
	}
	UPlayerShieldComponent* Shield = UPlayerShieldComponent::FindOrAdd(*Player);
	if (!TestNotNull(TEXT("A ward on it"), Shield))
	{
		return false;
	}
	TestTrue(TEXT("Bound in front of the health"), Health->Shield.IsBoundToObject(Shield));

	// No ward worn: every hit goes to health, as before.
	RecoveryTestWorld::Hurt(Player, 10.f);
	TestEqual(TEXT("No ward: 10 off health"), Health->GetHealth(), 90.f, 0.001f);
	Health->ResetHealth();

	// A Common ward worn, full (about 40).
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	Shield->Equip(TestWard(Definition), true);
	const float Capacity = Shield->GetCapacity();
	TestTrue(TEXT("Worn and full"), Shield->HasWard() && Shield->IsFull());

	RecoveryTestWorld::Hurt(Player, 20.f);
	TestEqual(TEXT("A 20 hit: health untouched"), Health->GetHealth(), 100.f, 0.001f);
	TestEqual(TEXT("...the ward down 20"), Shield->GetCharge(), Capacity - 20.f, 0.001f);
	TestEqual(TEXT("...nothing lost from health"), Health->GetLastHealthLoss(), 0.f);
	TestEqual(TEXT("...a hit all the same (the wounds' wait starts over)"), static_cast<int32>(Health->GetHurtCount()), 2);
	TestTrue(TEXT("...health's ramp waits for the ward"), Shield->HoldsHealthBack());

	const float Left = Shield->GetCharge();
	RecoveryTestWorld::Hurt(Player, Left + 20.f);
	TestTrue(TEXT("The breaking hit: broken, empty"), Shield->IsBroken() && Shield->GetCharge() == 0.f && Shield->GetBreakCount() == 1u);
	TestEqual(TEXT("...half its 20 over reaches health"), Health->GetHealth(), 90.f, 0.001f);
	RecoveryTestWorld::Hurt(Player, 5.f);
	TestEqual(TEXT("Broken: the next hit is health's"), Health->GetHealth(), 85.f, 0.001f);

	// The wait, then the fill.
	const FShieldTuning Tuning;
	const float Delay = ShieldRules::LiveDelay(Shield->GetWard().Stats, Tuning);
	Shield->Advance(Delay - 0.1f);
	TestTrue(TEXT("Inside its wait: still empty"), Shield->GetCharge() == 0.f && !Shield->IsRefilling());
	Shield->Advance(0.3f);
	TestTrue(TEXT("Past it: filling, no longer broken"), Shield->IsRefilling() && !Shield->IsBroken() && Shield->GetCharge() > 0.f);
	Shield->Advance(30.f);
	TestTrue(TEXT("...and full"), Shield->IsFull() && Shield->GetFillCount() == 1u && !Shield->HoldsHealthBack());

	// Dead: the ward is out; alive again (a respawn), full.
	RecoveryTestWorld::Hurt(Player, 1000.f);
	Shield->Advance(0.1f);
	TestTrue(TEXT("Dead: the ward is out"), Health->IsDead() && Shield->GetCharge() == 0.f);
	Health->ResetHealth();
	Shield->Advance(0.1f);
	TestTrue(TEXT("A respawn: the ward full"), Shield->IsFull());

	// Looter.Shield.Enable 0: the ward takes nothing.
	IConsoleVariable* Enable = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Shield.Enable"));
	if (Enable)
	{
		const int32 Was = Enable->GetInt();
		Enable->Set(0, ECVF_SetByCode);
		const float Before = Health->GetHealth();
		RecoveryTestWorld::Hurt(Player, 10.f);
		TestEqual(TEXT("Switched off: the hit is health's"), Health->GetHealth(), Before - 10.f, 0.001f);
		Enable->Set(Was, ECVF_SetByCode);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldWardsTest, "Looter.Shields.Wards", ShieldTestFlags)

bool FShieldWardsTest::RunTest(const FString& Parameters)
{
	// Carrying: a tap takes a ward (worn when none is, else a spare), spares swap with the one worn, and the save keeps it all.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	ACharacter* Player = RecoveryTestWorld::SpawnPlayer(World, FVector::ZeroVector, Health);
	UPlayerShieldComponent* Shield = Player ? UPlayerShieldComponent::FindOrAdd(*Player) : nullptr;
	if (!TestNotNull(TEXT("A ward component"), Shield))
	{
		return false;
	}
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	const FShieldInstanceData First = TestWard(Definition, EWeaponRarity::Common, 1, 11);
	const FShieldInstanceData Second = TestWard(Definition, EWeaponRarity::Rare, 1, 12, EShieldBuild::Quickset, EShieldTrait::Harvest);
	TestTrue(TEXT("A tap with none worn: worn"), Shield->Take(First) && Shield->HasWard() && Shield->GetSpares().IsEmpty());
	TestTrue(TEXT("...a ward put on comes on empty, waiting"), Shield->GetCharge() == 0.f && Shield->IsBroken());
	TestTrue(TEXT("A second tap: a spare"), Shield->Take(Second) && Shield->GetSpares().Num() == 1);
	TestTrue(TEXT("Wearing the spare: the first takes its place"), Shield->EquipSpare(0) && Shield->GetWard().Seed == 12 && Shield->GetSpares()[0].Seed == 11);
	for (int32 Index = 1; Index < UPlayerShieldComponent::MaxSpares; ++Index)
	{
		Shield->Take(TestWard(Definition, EWeaponRarity::Common, 1, 100 + Index));
	}
	TestEqual(TEXT("Spares fill to the most"), Shield->GetSpares().Num(), UPlayerShieldComponent::MaxSpares);
	TestFalse(TEXT("...a tap then takes nothing"), Shield->Take(TestWard(Definition, EWeaponRarity::Common, 1, 999)));
	TestFalse(TEXT("...and the worn one can't be stowed"), Shield->StowWard());

	// The save: the worn ward, the spares, the starting ward's flag; it comes back full, its numbers rebuilt.
	FShieldInventorySave Saved;
	Shield->SaveWards(Saved);
	TestTrue(TEXT("Saved: the worn ward and the spares"), Saved.Equipped.Seed == 12 && Saved.Spares.Num() == UPlayerShieldComponent::MaxSpares);
	Shield->EmptyWards();
	TestFalse(TEXT("Emptied"), Shield->HasWard());
	FShieldInventorySave Edited = Saved;
	Edited.Equipped.Stats.Capacity = 1.f;
	Shield->RestoreWards(Edited);
	TestTrue(TEXT("Restored: the same ward worn"), Shield->HasWard() && Shield->GetWard().Seed == 12 && Shield->GetWard().Trait == EShieldTrait::Harvest);
	TestEqual(TEXT("...its numbers its own, not the save's"), Shield->GetWard().Stats.Capacity, Second.Stats.Capacity, 0.001f);
	TestTrue(TEXT("...full"), Shield->IsFull());
	TestEqual(TEXT("...the spares back"), Shield->GetSpares().Num(), UPlayerShieldComponent::MaxSpares);

	// An older session's inventory: no wards in it at all.
	const FWeaponInventorySave Old;
	TestFalse(TEXT("A save from before wards carries none"), Old.Shields.Equipped.IsSet() || Old.Shields.bStarterGiven);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldLootTest, "Looter.Shields.Loot", ShieldTestFlags)

bool FShieldLootTest::RunTest(const FString& Parameters)
{
	// Wards drop like guns: a share of a ward-dropping table's gun picks, in the picks' rarity and the kill's level. A table
	// that doesn't drop wards rolls exactly as before.
	UWeaponDefinition* Gun = NewObject<UWeaponDefinition>();
	ULootTable* Table = NewObject<ULootTable>();
	FLootTableEntry Entry;
	Entry.Weapon = Gun;
	Table->Entries.Add(Entry);
	Table->WeaponDropChance = 1.f;
	Table->MinWeaponDrops = 1;
	Table->MaxWeaponDrops = 1;
	Table->AmmoDropChance = 0.f;
	constexpr int32 Kills = 3000;

	{
		FRandomStream Random(20261022);
		int32 Wards = 0;
		for (int32 Kill = 0; Kill < Kills; ++Kill)
		{
			Wards += ULootLibrary::RollLoot(Table, 1, 0.f, Random).Shields.Num();
		}
		TestEqual(TEXT("Off: no wards"), Wards, 0);
	}

	Table->bDropsShields = true;
	const float Share = FShieldTuning::Live().DropShare;
	FRandomStream Random(20261022);
	int32 Wards = 0;
	int32 Guns = 0;
	int32 WrongLevel = 0;
	for (int32 Kill = 0; Kill < Kills; ++Kill)
	{
		const FLootRoll Roll = ULootLibrary::RollLoot(Table, 6, 0.f, Random);
		Wards += Roll.Shields.Num();
		Guns += Roll.Weapons.Num();
		for (const FShieldInstanceData& Ward : Roll.Shields)
		{
			WrongLevel += Ward.Level == 6 && Ward.IsSet() ? 0 : 1;
		}
	}
	const float WardShare = static_cast<float>(Wards) / Kills;
	TestTrue(FString::Printf(TEXT("About %.0f%% of the picks come as wards (%.1f%%)"), Share * 100.f, WardShare * 100.f), FMath::Abs(WardShare - Share) <= 0.03f);
	TestEqual(TEXT("...the rest as guns: one thing a kill, as before"), Guns + Wards, Kills);
	TestEqual(TEXT("...each at the kill's level"), WrongLevel, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldLoadoutTest, "Looter.Shields.Loadout", ShieldTestFlags)

bool FShieldLoadoutTest::RunTest(const FString& Parameters)
{
	// The loadout's WARD section: W / S walk from the slots into it, E on the worn ward goes to the spares, E on a spare wears
	// it, C stows, Q drops.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	ACharacter* Player = RecoveryTestWorld::SpawnPlayer(World, FVector::ZeroVector, Health);
	if (!TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Player);
	Inventory->RegisterComponent();
	Inventory->MaxWeapons = 3;
	UPlayerShieldComponent* Shield = UPlayerShieldComponent::FindOrAdd(*Player);
	UShieldDefinition* Definition = NewObject<UShieldDefinition>();
	Shield->Equip(TestWard(Definition, EWeaponRarity::Common, 1, 21), true);
	Shield->Take(TestWard(Definition, EWeaponRarity::Epic, 1, 22, EShieldBuild::Bulwark, EShieldTrait::SaintsGrace));

	ULoadoutWidget* Screen = CreateWidget<ULoadoutWidget>(World, ULoadoutWidget::StaticClass());
	if (!TestNotNull(TEXT("The screen"), Screen))
	{
		return false;
	}
	Screen->Open(nullptr, Inventory);
	Screen->TakeWidget();
	for (int32 Press = 0; Press < 3; ++Press)
	{
		Screen->HandleKey(EKeys::S);
	}
	ULoadoutWidget::FView View = Screen->GetView();
	TestTrue(TEXT("S past the three slots: the ward worn"), View.bOnWard && View.Cursor == 0);
	TestTrue(TEXT("...E goes to the spares"), !View.Prompts.IsEmpty() && View.Prompts[0] == TEXT("E: Choose a spare"));
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("E: on the spare"), View.bOnWard && View.Cursor == 1 && !View.Prompts.IsEmpty() && View.Prompts[0] == TEXT("E: Wear"));
	Screen->HandleKey(EKeys::E);
	View = Screen->GetView();
	TestTrue(TEXT("E: the spare worn, the cursor on it"), Shield->GetWard().Seed == 22 && View.bOnWard && View.Cursor == 0);
	TestTrue(TEXT("...the old one a spare"), Shield->GetSpares().Num() == 1 && Shield->GetSpares()[0].Seed == 21);
	Screen->HandleKey(EKeys::C);
	TestTrue(TEXT("C: stowed, none worn"), !Shield->HasWard() && Shield->GetSpares().Num() == 2);
	Screen->HandleKey(EKeys::S);
	Screen->HandleKey(EKeys::Q);
	TestEqual(TEXT("Q on a spare: dropped"), Shield->GetSpares().Num(), 1);
	Screen->HandleKey(EKeys::X);
	TestFalse(TEXT("X: a ward isn't inspected"), Screen->GetView().bInspecting);
	Screen->Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShieldHudPlaceTest, "Looter.Shields.HudPlace", ShieldTestFlags)

bool FShieldHudPlaceTest::RunTest(const FString& Parameters)
{
	// The ward bar sits over the health bar, clear of it, its left end behind the medallion's bezel, its right end and its
	// number inside the frame.
	using W = UHudShieldBarWidget;
	TestTrue(TEXT("Above the health bar, clear of it"), W::BarY + W::Thickness < UHudPlayerFrameWidget::HealthBarY);
	TestTrue(TEXT("...inside the frame's box"), W::BarY + W::BoxTop >= UHudPlayerFrameWidget::ScreenBottom - UHudPlayerFrameWidget::Height);
	// The medallion's bezel's upper-right edge: x - y = MedallionX - MedallionY + its reach. The bar's start lies behind it.
	const float BezelLine = UHudPlayerFrameWidget::MedallionX - UHudPlayerFrameWidget::MedallionY + UHudPlayerFrameWidget::MedallionReach;
	const float TrackStart = (W::BarX + W::TrackLeft) - (W::BarY + W::Mid);
	TestTrue(FString::Printf(TEXT("...its track starting at the bezel's edge (%.1f against %.1f)"), TrackStart, BezelLine), FMath::Abs(TrackStart - BezelLine) <= 1.5f);
	TestTrue(TEXT("...its box behind the bezel"), W::BarX - (W::BarY + W::Mid) < BezelLine);
	TestTrue(TEXT("...its right end short of the health bar's"), W::BarX + W::Length < UHudPlayerFrameWidget::HealthBarX + HudFrameShapes::HealthLength);
	TestTrue(TEXT("...its number inside the frame"), W::BarX + W::BoxLeft + W::Width <= UHudPlayerFrameWidget::Width);
	return true;
}

#endif
