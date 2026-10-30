#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Elements/LooterElement.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"

// A Touched creature leans the guns it drops toward its element (the user's ruling, 2026-10-29): WeaponParts::Pick's Lean
// weighs that element's parts three times heavier. Looter.Loot.TouchedLean.

namespace
{
	const TCHAR* const LeanGuns[] = { TEXT("DA_AssaultRifle"), TEXT("DA_PumpShotgun"), TEXT("DA_Revolver"), TEXT("DA_SMG"),
		TEXT("DA_Sniper"), TEXT("DA_LMG") };
	const ELooterElement LeanElements[] = { ELooterElement::Ember, ELooterElement::SoulLight, ELooterElement::GraveSalt,
		ELooterElement::Thunder, ELooterElement::Rot };
	constexpr int32 LeanSeeds = 600;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLootTouchedLeanTest, "Looter.Loot.TouchedLean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLootTouchedLeanTest::RunTest(const FString& Parameters)
{
	int32 Plain = 0;
	int32 Leaned = 0;
	int32 Pairs = 0;
	for (const TCHAR* Asset : LeanGuns)
	{
		const UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Weapons/Data/%s.%s"), Asset, Asset));
		if (!TestNotNull(Asset, Definition))
		{
			continue;
		}
		for (const ELooterElement Element : LeanElements)
		{
			int32 PlainHere = 0;
			int32 LeanedHere = 0;
			for (int32 Seed = 0; Seed < LeanSeeds; ++Seed)
			{
				const FWeaponLook Without = WeaponParts::Pick(*Definition, Seed, EWeaponRarity::Legendary);
				const FWeaponLook None = WeaponParts::Pick(*Definition, Seed, EWeaponRarity::Legendary, {}, nullptr, ELooterElement::None);
				const FWeaponLook With = WeaponParts::Pick(*Definition, Seed, EWeaponRarity::Legendary, {}, nullptr, Element);
				// No lean is the pick as it always was, and a lean never changes how many slots there are.
				if (WeaponParts::PartKeys(Without) != WeaponParts::PartKeys(None) || With.Parts.Num() != Without.Parts.Num())
				{
					AddError(FString::Printf(TEXT("%s seed %d: a lean of None moved the pick"), Asset, Seed));
					return false;
				}
				PlainHere += WeaponParts::ElementOf(Without) == Element ? 1 : 0;
				LeanedHere += WeaponParts::ElementOf(With) == Element ? 1 : 0;
				// A gun that already has its parts keeps them, whatever the lean.
				if (Seed < 20)
				{
					const TArray<FName> Saved = WeaponParts::PartKeys(Without);
					if (WeaponParts::PartKeys(WeaponParts::Pick(*Definition, Seed, EWeaponRarity::Legendary, Saved, nullptr, Element)) != Saved)
					{
						AddError(FString::Printf(TEXT("%s seed %d: a lean changed the parts a gun was saved with"), Asset, Seed));
						return false;
					}
				}
			}
			if (PlainHere == 0 && LeanedHere == 0)
			{
				continue;
			}
			++Pairs;
			Plain += PlainHere;
			Leaned += LeanedHere;
			// The same draws with heavier weights: an element's share can only grow, give or take a little at the edges of a slot.
			TestTrue(FString::Printf(TEXT("%s %s: the lean does not lose them (%d plain, %d leaned)"), Asset, *UEnum::GetValueAsString(Element),
				PlainHere, LeanedHere), LeanedHere + LeanSeeds / 50 >= PlainHere);
		}
	}
	if (Pairs == 0)
	{
		AddInfo(TEXT("No gun offers an element part yet: nothing to lean"));
		return true;
	}
	TestTrue(FString::Printf(TEXT("A lean favours its element's parts over %d seeds (%d plain, %d leaned)"), LeanSeeds, Plain, Leaned), Leaned > Plain);
	TestTrue(FString::Printf(TEXT("...by a good margin, as a x3 weight does (%d plain, %d leaned)"), Plain, Leaned), Leaned * 4 >= Plain * 5);
	return true;
}

#endif
