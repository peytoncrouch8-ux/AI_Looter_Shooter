#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "UObject/Package.h"

// Rarity that plays differently (the audit's change 5, 2026-11-03): a Rare or better gun's tier slot always rolls a part of
// its tier (Looter.Weapons.Parts.TierSlot, on made-up guns with known parts), and the parts sheets' weights (effect parts 3,
// element parts 1.5) make most blues and purples do something (Looter.Weapons.Parts.EffectOdds, by Monte Carlo over seeds
// on the game's guns: run Tools/Unreal/setup_gun_parts.py first, the definitions carry the sheets' weights).

namespace
{
	/** A console variable set for a test and put back after it. */
	class FTierScopedCVar
	{
	public:
		FTierScopedCVar(const TCHAR* Name, const TCHAR* Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Previous = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FTierScopedCVar()
		{
			if (Variable)
			{
				Variable->Set(*Previous, ECVF_SetByConsole);
			}
		}

	private:
		IConsoleVariable* Variable = nullptr;
		FString Previous;
	};

	FWeaponPartOption& AddTierOption(UWeaponDefinition& Gun, FName Slot, FName Key, EWeaponRarity MinRarity, float Weight = 1.f)
	{
		static UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		FWeaponPartSlot* Found = Gun.Parts.FindByPredicate([Slot](const FWeaponPartSlot& Part) { return Part.Name == Slot; });
		if (!Found)
		{
			Found = &Gun.Parts.AddDefaulted_GetRef();
			Found->Name = Slot;
		}
		FWeaponPartOption& Option = Found->Options.AddDefaulted_GetRef();
		Option.Key = Key;
		Option.Mesh = Cube;
		Option.MinRarity = MinRarity;
		Option.Weight = Weight;
		return Option;
	}

	/** Three slots of six Commons each, and a Rare and an Epic so faint (0.05) a plain draw almost never lands on them. */
	UWeaponDefinition* MakeFaintGun()
	{
		UWeaponDefinition* Gun = NewObject<UWeaponDefinition>(GetTransientPackage());
		const TCHAR* const Slots[] = { TEXT("Body"), TEXT("Barrel"), TEXT("Sight") };
		for (const TCHAR* Slot : Slots)
		{
			for (int32 Plain = 0; Plain < 6; ++Plain)
			{
				AddTierOption(*Gun, Slot, *FString::Printf(TEXT("Plain%d"), Plain), EWeaponRarity::Common);
			}
			AddTierOption(*Gun, Slot, TEXT("Blue"), EWeaponRarity::Rare, 0.05f);
			AddTierOption(*Gun, Slot, TEXT("Purple"), EWeaponRarity::Epic, 0.05f);
		}
		return Gun;
	}

	/** The best rarity among the look's parts. */
	EWeaponRarity BestPart(const FWeaponLook& Look)
	{
		EWeaponRarity Best = EWeaponRarity::Common;
		for (const FWeaponPartOption* Part : Look.Parts)
		{
			Best = Part && Part->MinRarity > Best ? Part->MinRarity : Best;
		}
		return Best;
	}

	const TCHAR* const TierGuns[] = { TEXT("DA_AssaultRifle"), TEXT("DA_PumpShotgun"), TEXT("DA_Revolver"), TEXT("DA_SMG"), TEXT("DA_Sniper"),
		TEXT("DA_LMG"), TEXT("DA_Launcher") };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartTierSlotTest, "Looter.Weapons.Parts.TierSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartTierSlotTest::RunTest(const FString& Parameters)
{
	FTierScopedCVar On(TEXT("Looter.Loot.Parts.TierSlot"), TEXT("1"));
	UWeaponDefinition* Gun = MakeFaintGun();
	constexpr int32 Seeds = 600;
	int32 Chosen[3] = { 0, 0, 0 };
	for (int32 Seed = 0; Seed < Seeds; ++Seed)
	{
		for (const EWeaponRarity Rarity : { EWeaponRarity::Rare, EWeaponRarity::Epic })
		{
			const FWeaponLook Look = WeaponParts::Pick(*Gun, Seed, Rarity);
			const int32 Slot = WeaponParts::TierSlot(*Gun, Seed, Rarity);
			// The same seed, the same gun: its tier slot and every part.
			if (WeaponParts::PartKeys(Look) != WeaponParts::PartKeys(WeaponParts::Pick(*Gun, Seed, Rarity)) || Slot != WeaponParts::TierSlot(*Gun, Seed, Rarity))
			{
				AddError(FString::Printf(TEXT("Seed %d: picked differently twice"), Seed));
				return false;
			}
			// However faint, a part of its own tier in its tier slot.
			const FWeaponPartOption* Tiered = Look.Parts.IsValidIndex(Slot) ? Look.Parts[Slot] : nullptr;
			if (!Tiered || Tiered->MinRarity != Rarity)
			{
				AddError(FString::Printf(TEXT("Seed %d %s: slot %d holds no part of its tier"), Seed, *UEnum::GetValueAsString(Rarity), Slot));
				return false;
			}
			Chosen[FMath::Clamp(Slot, 0, 2)] += Rarity == EWeaponRarity::Rare ? 1 : 0;
		}
		TestEqual(TEXT("An Uncommon has no tier slot"), WeaponParts::TierSlot(*Gun, Seed, EWeaponRarity::Uncommon), INDEX_NONE);
	}
	// Each slot about a third of the time (200 expected; 140 is over five spreads under).
	TestTrue(FString::Printf(TEXT("Any slot can be the tier slot (%d, %d, %d of %d)"), Chosen[0], Chosen[1], Chosen[2], Seeds),
		Chosen[0] > 140 && Chosen[1] > 140 && Chosen[2] > 140);

	// The gun's own tier; a Legendary with no Legendary part rolls an Epic one; under Rare, or with nothing better than
	// Uncommon, no tier at all.
	TestEqual(TEXT("A Rare's tier"), WeaponParts::TierSlotRarity(*Gun, EWeaponRarity::Rare), EWeaponRarity::Rare);
	TestEqual(TEXT("A Legendary without Legendary parts: Epic"), WeaponParts::TierSlotRarity(*Gun, EWeaponRarity::Legendary), EWeaponRarity::Epic);
	TestEqual(TEXT("An Uncommon: none"), WeaponParts::TierSlotRarity(*Gun, EWeaponRarity::Uncommon), EWeaponRarity::Common);
	int32 LegendaryEpics = 0;
	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		LegendaryEpics += BestPart(WeaponParts::Pick(*Gun, Seed, EWeaponRarity::Legendary)) == EWeaponRarity::Epic ? 1 : 0;
	}
	TestEqual(TEXT("...so every Legendary has its Epic part"), LegendaryEpics, 200);
	UWeaponDefinition* Plain = NewObject<UWeaponDefinition>(GetTransientPackage());
	AddTierOption(*Plain, TEXT("Body"), TEXT("Old"), EWeaponRarity::Common);
	AddTierOption(*Plain, TEXT("Body"), TEXT("Fine"), EWeaponRarity::Uncommon);
	TestEqual(TEXT("Nothing better than Uncommon: no tier slot"), WeaponParts::TierSlot(*Plain, 7, EWeaponRarity::Epic), INDEX_NONE);

	// A tier part that doesn't fit (a long magazine on a stub barrel) hands the rule on to the next slot.
	UWeaponDefinition* Stubby = NewObject<UWeaponDefinition>(GetTransientPackage());
	AddTierOption(*Stubby, TEXT("Barrel"), TEXT("Stub"), EWeaponRarity::Common).Length = 5.f;
	AddTierOption(*Stubby, TEXT("Magazine"), TEXT("Box"), EWeaponRarity::Common);
	FWeaponPartOption& Long = AddTierOption(*Stubby, TEXT("Magazine"), TEXT("LongBox"), EWeaponRarity::Rare);
	Long.Requires.Slot = TEXT("Barrel");
	Long.Requires.MinLength = 20.f;
	AddTierOption(*Stubby, TEXT("Sight"), TEXT("Iron"), EWeaponRarity::Common, 50.f);
	AddTierOption(*Stubby, TEXT("Sight"), TEXT("Scope"), EWeaponRarity::Rare, 0.05f);
	int32 Handed = 0;
	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		const FWeaponLook Look = WeaponParts::Pick(*Stubby, Seed, EWeaponRarity::Rare);
		const bool bMagazineSlot = WeaponParts::TierSlot(*Stubby, Seed, EWeaponRarity::Rare) == 1;
		TestTrue(TEXT("The long box never on the stub"), Look.Parts[1] && Look.Parts[1]->Key != FName(TEXT("LongBox")));
		TestTrue(TEXT("...and every Rare still has its Rare part, the scope"), Look.Parts[2] && Look.Parts[2]->Key == FName(TEXT("Scope")));
		Handed += bMagazineSlot ? 1 : 0;
	}
	TestTrue(FString::Printf(TEXT("The magazine was the tier slot for some (%d of 200)"), Handed), Handed > 50);

	// A saved gun keeps its plain parts, tier slot or not; a fresh roll with the rule off is as before (faint parts stay rare).
	const TArray<FName> Saved = { TEXT("Plain0"), TEXT("Plain1"), TEXT("Plain2") };
	TestTrue(TEXT("A saved gun keeps its parts"), WeaponParts::PartKeys(WeaponParts::Pick(*Gun, 3, EWeaponRarity::Epic, Saved)) == Saved);
	{
		FTierScopedCVar Off(TEXT("Looter.Loot.Parts.TierSlot"), TEXT("0"));
		int32 Tiered = 0;
		for (int32 Seed = 0; Seed < Seeds; ++Seed)
		{
			Tiered += BestPart(WeaponParts::Pick(*Gun, Seed, EWeaponRarity::Rare)) >= EWeaponRarity::Rare ? 1 : 0;
		}
		TestEqual(TEXT("Off: no tier slot"), WeaponParts::TierSlot(*Gun, 1, EWeaponRarity::Rare), INDEX_NONE);
		// Three draws at 0.05 / 6.05 each: about 2.5% of guns (15 expected).
		TestTrue(FString::Printf(TEXT("Off: the faint parts stay faint (%d of %d)"), Tiered, Seeds), Tiered < 60);
	}
	for (UWeaponDefinition* Made : { Gun, Plain, Stubby })
	{
		Made->MarkAsGarbage();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartEffectOddsTest, "Looter.Weapons.Parts.EffectOdds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartEffectOddsTest::RunTest(const FString& Parameters)
{
	// Over many seeds (no maker): most blues and purples carry an effect or an element, every Rare and up its tier part,
	// and the rifle's effect odds near the plan's (a Rare ~1 in 3, an Epic ~4 in 5; LOOT.md has every gun's).
	FTierScopedCVar On(TEXT("Looter.Loot.Parts.TierSlot"), TEXT("1"));
	constexpr int32 Seeds = 2000;
	for (const TCHAR* Asset : TierGuns)
	{
		const UWeaponDefinition* Definition = LoadObject<UWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Weapons/Data/%s.%s"), Asset, Asset));
		if (!TestNotNull(Asset, Definition))
		{
			continue;
		}
		for (const EWeaponRarity Rarity : { EWeaponRarity::Rare, EWeaponRarity::Epic, EWeaponRarity::Legendary })
		{
			const FString What = FString::Printf(TEXT("%s %s"), Asset, *UEnum::GetDisplayValueAsText(Rarity).ToString());
			const EWeaponRarity Tier = WeaponParts::TierSlotRarity(*Definition, Rarity);
			const bool bOffersEffect = Definition->Parts.ContainsByPredicate([Rarity](const FWeaponPartSlot& Slot)
			{
				return Slot.Options.ContainsByPredicate([Rarity](const FWeaponPartOption& Option) { return Option.Effect != EPartEffect::None && Option.MinRarity <= Rarity; });
			});
			int32 Effects = 0;
			int32 Either = 0;
			int32 Tiered = 0;
			for (int32 Seed = 0; Seed < Seeds; ++Seed)
			{
				const FWeaponLook Look = WeaponParts::Pick(*Definition, Seed, Rarity);
				const bool bEffect = WeaponParts::EffectOf(Look) != EPartEffect::None;
				Effects += bEffect ? 1 : 0;
				Either += bEffect || WeaponParts::ElementOf(Look) != ELooterElement::None ? 1 : 0;
				Tiered += BestPart(Look) >= Tier ? 1 : 0;
			}
			TestEqual(FString::Printf(TEXT("%s: every one has a part of its tier (%s)"), *What, *UEnum::GetDisplayValueAsText(Tier).ToString()), Tiered, Seeds);
			if (Rarity == EWeaponRarity::Legendary)
			{
				continue;
			}
			// Before the change a Rare carried an effect or element 23-32% of the time; now the least is ~45% (LOOT.md).
			TestTrue(FString::Printf(TEXT("%s: most do something (%d of %d with an effect or element)"), *What, Either, Seeds), Either * 100 >= Seeds * 38);
			if (bOffersEffect)
			{
				const int32 Least = Rarity == EWeaponRarity::Rare ? 25 : 45;
				TestTrue(FString::Printf(TEXT("%s: an effect %d%% of the time (at least %d%%)"), *What, Effects * 100 / Seeds, Least), Effects * 100 >= Seeds * Least);
			}
			if (FCString::Strcmp(Asset, TEXT("DA_AssaultRifle")) == 0)
			{
				// The plan's numbers (a Rare ~40%, an Epic ~70%) with the sheets as they are: 33% and 83% (was 7% and 28%).
				const bool bNear = Rarity == EWeaponRarity::Rare ? Effects * 100 >= Seeds * 25 && Effects * 100 <= Seeds * 45
					: Effects * 100 >= Seeds * 65 && Effects * 100 <= Seeds * 95;
				TestTrue(FString::Printf(TEXT("%s: the rifle's effect odds near the plan's (%d of %d)"), *What, Effects, Seeds), bNear);
			}
		}
	}
	return true;
}

#endif
