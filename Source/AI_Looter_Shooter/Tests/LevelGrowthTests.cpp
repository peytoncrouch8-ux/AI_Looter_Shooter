#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Progression/LevelRules.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Weapons/WeaponDefinition.h"

// Growth a level, compounding (the user, 2026-10-29 night: "+9% a level, compounding" in place of +8% of level 1's): gun
// damage, creature health and damage, ward capacity and the player's health alike; level 1 as it was. Experience is its own
// (FLevelRules::KillXP).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLevelGrowthCompoundTest, "Looter.Progression.CompoundGrowth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLevelGrowthCompoundTest::RunTest(const FString& Parameters)
{
	// The rule itself.
	TestEqual(TEXT("Level 1: x1"), FLevelRules::Compounded(0.09f, 1), 1.f);
	TestEqual(TEXT("Below level 1: x1"), FLevelRules::Compounded(0.09f, 0), 1.f);
	TestEqual(TEXT("Level 2: x1.09"), FLevelRules::Compounded(0.09f, 2), 1.09f, 1e-5f);
	TestEqual(TEXT("Level 7: x1.09^6 (1.677)"), FLevelRules::Compounded(0.09f, 7), FMath::Pow(1.09f, 6.f), 1e-4f);
	TestEqual(TEXT("A negative growth: none"), FLevelRules::Compounded(-0.5f, 9), 1.f);
	TestTrue(TEXT("Each level the same share more"), FMath::IsNearlyEqual(FLevelRules::Compounded(0.09f, 31) / FLevelRules::Compounded(0.09f, 30),
		FLevelRules::Compounded(0.09f, 3) / FLevelRules::Compounded(0.09f, 2), 1e-4f));

	// The shipped numbers: enemies, guns and wards all +9%, so a gun of an enemy's level takes the same hits at any level.
	const FLevelRules Shipped;
	TestEqual(TEXT("Enemies grow 9% a level"), Shipped.EnemyGrowth, 0.09f);
	TestEqual(TEXT("...and guns' damage the same"), GetDefault<UWeaponDefinition>()->DamagePerLevel, Shipped.EnemyGrowth);
	TestEqual(TEXT("...and wards' capacity the same"), GetDefault<UShieldDefinition>()->CapacityPerLevel, Shipped.EnemyGrowth);
	TestEqual(TEXT("...and the player's health the same (it keeps the enemies' pace)"), Shipped.PlayerHealthScale(7), Shipped.EnemyScale(7), 1e-5f);
	TestEqual(TEXT("An enemy at level 7: x1.677"), Shipped.EnemyScale(7), FLevelRules::Compounded(0.09f, 7), 1e-5f);

	// A gun: its damage at level 7 is its level 1 damage x1.09^6, the same seed and parts.
	UWeaponDefinition* Rifle = NewObject<UWeaponDefinition>();
	Rifle->BaseStats.Damage = 100.f;
	FWeaponInstanceData Gun;
	Gun.Definition = Rifle;
	Gun.Seed = 1029;
	Gun.Level = 1;
	const float AtOne = UWeaponRollLibrary::ComputeInstanceStats(Gun).Damage;
	Gun.Level = 7;
	const float AtSeven = UWeaponRollLibrary::ComputeInstanceStats(Gun).Damage;
	TestTrue(FString::Printf(TEXT("A gun at level 7 hits x%.3f its level 1 (x1.677)"), AtSeven / FMath::Max(AtOne, 0.001f)),
		AtOne > 0.f && FMath::IsNearlyEqual(AtSeven / AtOne, FLevelRules::Compounded(0.09f, 7), 1e-3f));

	// A ward: its capacity the same way, the same seed.
	const UShieldDefinition* Ward = GetDefault<UShieldDefinition>();
	const float WardOne = ShieldRules::ComputeStats(*Ward, EWeaponRarity::Common, 1, 77, EShieldBuild::Standard).Capacity;
	const float WardSeven = ShieldRules::ComputeStats(*Ward, EWeaponRarity::Common, 7, 77, EShieldBuild::Standard).Capacity;
	TestTrue(TEXT("A ward at level 7 holds x1.677 its level 1"), WardOne > 0.f && FMath::IsNearlyEqual(WardSeven / WardOne, FLevelRules::Compounded(0.09f, 7), 1e-3f));

	// Ransom's Rest (levels 1-7) against the old rule (1 + 0.08 x (level - 1)): level 1 the same, the gap opening a few
	// points a level (+0.9% at 2, +4.4% at 4, +13.3% at 7). Guns and enemies move together, so kill times don't change.
	TestEqual(TEXT("Ransom's Rest's level 1: as it was"), Shipped.EnemyScale(1), 1.f);
	for (int32 Level = 2; Level <= 7; ++Level)
	{
		const float Old = 1.f + 0.08f * static_cast<float>(Level - 1);
		const float New = Shipped.EnemyScale(Level);
		AddInfo(FString::Printf(TEXT("Level %d: x%.3f (was x%.3f, %+.1f%%)"), Level, New, Old, (New / Old - 1.f) * 100.f));
		TestTrue(FString::Printf(TEXT("Level %d: above the old rule, never more than 15%% above"), Level), New > Old && New / Old < 1.15f);
	}
	return true;
}

#endif
