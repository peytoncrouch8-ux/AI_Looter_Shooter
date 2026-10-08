#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/CombatRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* const CurseRiflePath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	const EWeaponRarity CurseRarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic, EWeaponRarity::Legendary };

	/** A rolled gun with a known seed, its parts kept and its stats rolled, uncursed. */
	FWeaponInstanceData MakeCurseGun(UWeaponDefinition& Definition, EWeaponRarity Rarity, int32 Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = &Definition;
		Gun.Rarity = Rarity;
		Gun.Level = 5;
		Gun.Seed = Seed;
		Gun.Parts = WeaponParts::PartKeys(WeaponParts::Pick(Definition, Seed, Rarity));
		Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
		return Gun;
	}

	/** The gun with a curse laid on it (lifted or not), its stats rebuilt as a weapon rebuilds them. */
	FWeaponInstanceData WithCurse(const FWeaponInstanceData& Gun, const TCHAR* Key, bool bLifted = false)
	{
		FWeaponInstanceData Cursed = Gun;
		Cursed.Curse = Key;
		Cursed.bCurseLifted = bLifted;
		Cursed.Stats = UWeaponRollLibrary::ComputeInstanceStats(Cursed);
		return Cursed;
	}

	bool CurseNear(double A, double B)
	{
		return FMath::IsNearlyEqual(A, B, FMath::Abs(B) * 1.e-5 + 1.e-5);
	}

	/** Exactly the same numbers. */
	bool SameStats(const FWeaponStats& A, const FWeaponStats& B)
	{
		return A.Damage == B.Damage && A.FireRate == B.FireRate && A.MagazineSize == B.MagazineSize && A.ReloadTime == B.ReloadTime
			&& A.Spread == B.Spread && A.Range == B.Range && A.PelletsPerShot == B.PelletsPerShot && A.Recoil == B.Recoil
			&& A.Handling == B.Handling && A.Zoom == B.Zoom;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponCursesTest, "Looter.Weapons.Curses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponCursesTest::RunTest(const FString& Parameters)
{
	// Cursed irons: six curses by keys that never change, each a strong perk and a real drawback. 6% of the Rare, Epic and
	// Legendary guns that drop roll one from their seed and rarity, the same every time; commons, uncommons, named guns and
	// guns that don't drop (rewards, the rack, starting guns) never. Damage, fire rate and recoil reach the stats; a curse
	// changes critical hits; Cold in hand stops the sprint. Lifted, the drawback goes and the perk stays.
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, CurseRiflePath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}

	// The table: the six by their keys, each worded for the cards, each with a perk and a drawback that do something.
	const TCHAR* const Keys[] = { TEXT("Hungry"), TEXT("Greedy"), TEXT("Restless"), TEXT("Cold"), TEXT("Grasping"), TEXT("Unlucky") };
	TestEqual(TEXT("Six curses"), WeaponCurses::All().Num(), static_cast<int32>(UE_ARRAY_COUNT(Keys)));
	for (const TCHAR* Key : Keys)
	{
		const FWeaponCurse* Curse = WeaponCurses::Find(Key);
		if (!TestNotNull(FString::Printf(TEXT("%s is a curse"), Key), Curse))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: its key, name, perk and drawback"), Key), Curse->Key == FName(Key) && Curse->Name.ToString() == Key
			&& !Curse->Perk.IsEmpty() && !Curse->Drawback.IsEmpty());
		const bool bPerk = Curse->DamageMultiplier != 1.f || Curse->FireRateMultiplier != 1.f || Curse->CritDamageMultiplier != 1.f
			|| Curse->CritMultiplierOverride > 0.f || Curse->KillLootLuck > 0.f || Curse->KillHealShare > 0.f;
		const bool bDrawback = Curse->ReloadHealthCostShare > 0.f || Curse->RoundsPerShot > 1 || Curse->RecoilMultiplier != 1.f || Curse->bBlocksSprint
			|| Curse->MaxHealthMultiplier != 1.f || Curse->MisfireChance > 0.f;
		TestTrue(FString::Printf(TEXT("%s: a perk and a drawback"), Key), bPerk && bDrawback);
	}
	TestNull(TEXT("No curse by no key"), WeaponCurses::Find(NAME_None));
	TestNull(TEXT("No curse by an unknown key"), WeaponCurses::Find(TEXT("NoSuchCurse")));

	// The spec's numbers.
	const FWeaponCurse* Hungry = WeaponCurses::Find(TEXT("Hungry"));
	const FWeaponCurse* Greedy = WeaponCurses::Find(TEXT("Greedy"));
	const FWeaponCurse* Restless = WeaponCurses::Find(TEXT("Restless"));
	const FWeaponCurse* Cold = WeaponCurses::Find(TEXT("Cold"));
	const FWeaponCurse* Grasping = WeaponCurses::Find(TEXT("Grasping"));
	const FWeaponCurse* Unlucky = WeaponCurses::Find(TEXT("Unlucky"));
	if (!Hungry || !Greedy || !Restless || !Cold || !Grasping || !Unlucky)
	{
		return false;
	}
	TestTrue(TEXT("Hungry: +30% damage; 3% of max health a reload"), Hungry->DamageMultiplier == 1.3f && Hungry->ReloadHealthCostShare == 0.03f);
	TestTrue(TEXT("Greedy: +0.5 luck on its kills; 2 rounds a shot"), Greedy->KillLootLuck == 0.5f && Greedy->RoundsPerShot == 2);
	TestTrue(TEXT("Restless: +40% fire rate; double recoil"), Restless->FireRateMultiplier == 1.4f && Restless->RecoilMultiplier == 2.f);
	TestTrue(TEXT("Cold: +20% damage, +25% critical damage; no sprinting"), Cold->DamageMultiplier == 1.2f && Cold->CritDamageMultiplier == 1.25f
		&& Cold->bBlocksSprint);
	TestTrue(TEXT("Grasping: kills heal 5%; 15% less max health"), Grasping->KillHealShare == 0.05f && Grasping->MaxHealthMultiplier == 0.85f);
	TestTrue(TEXT("Unlucky: triple critical hits; one shot in eight misfires"), Unlucky->CritMultiplierOverride == 3.f && Unlucky->MisfireChance == 0.125f);

	// The roll: 6% of Rare and up over many seeds, none below, every curse turning up, the same gun always the same.
	const int32 NumSeeds = 20000;
	for (const EWeaponRarity Rarity : CurseRarities)
	{
		int32 Cursed = 0;
		TMap<FName, int32> ByCurse;
		for (int32 Seed = 0; Seed < NumSeeds; ++Seed)
		{
			FWeaponInstanceData Gun;
			Gun.Definition = Rifle;
			Gun.Rarity = Rarity;
			Gun.Seed = Seed;
			const FName Rolled = WeaponCurses::Roll(Gun);
			if (WeaponCurses::Roll(Gun) != Rolled)
			{
				AddError(FString::Printf(TEXT("Seed %d rolled two curses"), Seed));
			}
			if (!Rolled.IsNone())
			{
				++Cursed;
				ByCurse.FindOrAdd(Rolled)++;
			}
		}
		const FString Tier = UEnum::GetDisplayValueAsText(Rarity).ToString();
		if (Rarity < EWeaponRarity::Rare)
		{
			TestEqual(FString::Printf(TEXT("%s guns are never cursed"), *Tier), Cursed, 0);
			continue;
		}
		const float Share = static_cast<float>(Cursed) / NumSeeds;
		TestTrue(FString::Printf(TEXT("%s: about 6%% cursed (%.2f%%)"), *Tier, Share * 100.f), Share > 0.05f && Share < 0.07f);
		TestEqual(FString::Printf(TEXT("%s: every curse turns up"), *Tier), ByCurse.Num(), static_cast<int32>(UE_ARRAY_COUNT(Keys)));
		for (const TPair<FName, int32>& Each : ByCurse)
		{
			TestTrue(FString::Printf(TEXT("%s: %s is a curse, about a sixth of them (%d)"), *Tier, *Each.Key.ToString(), Each.Value),
				WeaponCurses::Find(Each.Key) && Each.Value > Cursed / 12 && Each.Value < Cursed / 3);
		}
	}

	// A named gun is one gun, never a cursed copy.
	UNamedWeaponDefinition* Keepsake = NewObject<UNamedWeaponDefinition>(CreatePackage(nullptr), TEXT("DA_Named_TestCurseKeepsake"), RF_Transient);
	Keepsake->Weapon = Rifle;
	Keepsake->DisplayName = FText::FromString(TEXT("Keepsake"));
	int32 NamedCursed = 0;
	for (int32 Seed = 0; Seed < 2000; ++Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = Rifle;
		Gun.Named = Keepsake;
		Gun.Rarity = EWeaponRarity::Legendary;
		Gun.Seed = Seed;
		NamedCursed += WeaponCurses::Roll(Gun).IsNone() ? 0 : 1;
	}
	TestEqual(TEXT("Named guns are never cursed"), NamedCursed, 0);

	// The stats: only the plain numbers, on top of everything else the gun rolled.
	const FWeaponInstanceData Plain = MakeCurseGun(*Rifle, EWeaponRarity::Epic, 4242);
	const FWeaponStats& Base = Plain.Stats;
	auto ButFor = [&Base](float Damage, float FireRate, float Recoil)
	{
		FWeaponStats Expected = Base;
		Expected.Damage *= Damage;
		Expected.FireRate *= FireRate;
		Expected.Recoil *= Recoil;
		return Expected;
	};
	auto Matches = [](const FWeaponStats& A, const FWeaponStats& B)
	{
		return CurseNear(A.Damage, B.Damage) && CurseNear(A.FireRate, B.FireRate) && CurseNear(A.Recoil, B.Recoil) && A.MagazineSize == B.MagazineSize
			&& A.ReloadTime == B.ReloadTime && A.Spread == B.Spread && A.Range == B.Range && A.Handling == B.Handling && A.Zoom == B.Zoom;
	};
	TestTrue(TEXT("Hungry: +30% damage"), Matches(WithCurse(Plain, TEXT("Hungry")).Stats, ButFor(1.3f, 1.f, 1.f)));
	TestTrue(TEXT("Cold: +20% damage"), Matches(WithCurse(Plain, TEXT("Cold")).Stats, ButFor(1.2f, 1.f, 1.f)));
	TestTrue(TEXT("Restless: +40% fire rate, double recoil"), Matches(WithCurse(Plain, TEXT("Restless")).Stats, ButFor(1.f, 1.4f, 2.f)));
	TestTrue(TEXT("Restless lifted: the fire rate stays, the recoil goes"), Matches(WithCurse(Plain, TEXT("Restless"), true).Stats, ButFor(1.f, 1.4f, 1.f)));
	TestTrue(TEXT("Hungry lifted: still +30% damage"), Matches(WithCurse(Plain, TEXT("Hungry"), true).Stats, ButFor(1.3f, 1.f, 1.f)));
	for (const TCHAR* InPlay : { TEXT("Greedy"), TEXT("Grasping"), TEXT("Unlucky"), TEXT("NoSuchCurse") })
	{
		TestTrue(FString::Printf(TEXT("%s: the stats as they were (it acts in play)"), InPlay), SameStats(WithCurse(Plain, InPlay).Stats, Base));
	}

	// Critical hits: LooterCombat's 1.5x, Cold's a quarter more, Unlucky's triple; perks, so lifting keeps them.
	TestEqual(TEXT("Uncursed: the usual critical hit"), WeaponCurses::CritMultiplier(Plain), LooterCombat::CriticalHitMultiplier);
	TestEqual(TEXT("Hungry: the usual critical hit"), WeaponCurses::CritMultiplier(WithCurse(Plain, TEXT("Hungry"))), LooterCombat::CriticalHitMultiplier);
	TestEqual(TEXT("Cold: +25% critical damage"), WeaponCurses::CritMultiplier(WithCurse(Plain, TEXT("Cold"))), LooterCombat::CriticalHitMultiplier * 1.25f);
	TestEqual(TEXT("Cold lifted: still"), WeaponCurses::CritMultiplier(WithCurse(Plain, TEXT("Cold"), true)), LooterCombat::CriticalHitMultiplier * 1.25f);
	TestEqual(TEXT("Unlucky: triple"), WeaponCurses::CritMultiplier(WithCurse(Plain, TEXT("Unlucky"))), 3.f);
	TestEqual(TEXT("Unlucky lifted: still triple"), WeaponCurses::CritMultiplier(WithCurse(Plain, TEXT("Unlucky"), true)), 3.f);

	// The drawback in force until lifted; an unknown key is no curse at all.
	TestFalse(TEXT("Uncursed: no drawback"), WeaponCurses::DrawbackActive(Plain) || WeaponCurses::Of(Plain) != nullptr);
	TestTrue(TEXT("Cursed: its drawback"), WeaponCurses::DrawbackActive(WithCurse(Plain, TEXT("Greedy"))));
	TestFalse(TEXT("Lifted: no drawback"), WeaponCurses::DrawbackActive(WithCurse(Plain, TEXT("Greedy"), true)));
	TestTrue(TEXT("Lifted: still cursed, for its perk"), WeaponCurses::Of(WithCurse(Plain, TEXT("Greedy"), true)) == Greedy);
	TestFalse(TEXT("An unknown key: no curse"), WeaponCurses::DrawbackActive(WithCurse(Plain, TEXT("NoSuchCurse"))));

	// The labels' words.
	TestTrue(TEXT("Uncursed: no curse word"), LooterWeaponText::CurseString(Plain).IsEmpty());
	TestEqualSensitive(TEXT("Cursed: its name"), LooterWeaponText::CurseString(WithCurse(Plain, TEXT("Hungry"))), FString(TEXT("HUNGRY")));
	TestEqualSensitive(TEXT("Lifted: said so"), LooterWeaponText::CurseString(WithCurse(Plain, TEXT("Hungry"), true)), FString(TEXT("HUNGRY · LIFTED")));

	// Only guns that drop: what a reward, the rack or a starting gun is made with never curses, a kill's or a chest's
	// does, its stats with the curse in them.
	int32 Given = 0;
	for (int32 Index = 0; Index < 500; ++Index)
	{
		Given += UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Legendary, 1).Curse.IsNone() ? 0 : 1;
	}
	TestEqual(TEXT("Guns made for rewards, the rack and the start are never cursed"), Given, 0);

	UWeaponDefinition* RareOnly = DuplicateObject(Rifle, GetTransientPackage());
	for (const EWeaponRarity Rarity : CurseRarities)
	{
		FWeaponRarityInfo Info = RareOnly->GetRarityInfo(Rarity);
		Info.Weight = Rarity == EWeaponRarity::Rare ? 1.f : 0.f;
		RareOnly->RarityTable.Add(Rarity, Info);
	}
	ULootTable* Table = NewObject<ULootTable>();
	FLootTableEntry Entry;
	Entry.Weapon = RareOnly;
	Table->Entries.Add(Entry);
	Table->WeaponDropChance = 1.f;
	Table->MinWeaponDrops = 10;
	Table->MaxWeaponDrops = 10;
	Table->AmmoDropChance = 0.f;
	FRandomStream Random(20261008);
	int32 Dropped = 0;
	int32 DroppedCursed = 0;
	for (int32 Kill = 0; Kill < 400; ++Kill)
	{
		const FLootRoll Roll = ULootLibrary::RollLoot(Table, 3, 0.f, Random);
		for (const FWeaponInstanceData& Gun : Roll.Weapons)
		{
			if (Gun.Rarity != EWeaponRarity::Rare)
			{
				continue;
			}
			++Dropped;
			if (Gun.Curse.IsNone())
			{
				continue;
			}
			++DroppedCursed;
			if (Gun.Curse != WeaponCurses::Roll(Gun) || !SameStats(Gun.Stats, UWeaponRollLibrary::ComputeInstanceStats(Gun)))
			{
				AddError(FString::Printf(TEXT("A dropped %s gun isn't its seed's curse, or its stats leave the curse out"), *Gun.Curse.ToString()));
			}
		}
	}
	const float DroppedShare = Dropped > 0 ? static_cast<float>(DroppedCursed) / Dropped : 0.f;
	TestTrue(FString::Printf(TEXT("About 6%% of the Rare guns that drop are cursed (%d of %d)"), DroppedCursed, Dropped),
		Dropped > 3900 && DroppedShare > 0.04f && DroppedShare < 0.08f);

	// In hand: Cold stops the sprint while its drawback is in force; lifted, it lets the player run and keeps its perk.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	APawn* Holder = WorldWrapper.GetTestWorld()->SpawnActor<APawn>();
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Holder);
	Inventory->RegisterComponent();
	// One slot: each gun given goes straight into the hand.
	Inventory->MaxWeapons = 1;
	TestTrue(TEXT("Nothing in hand: no curse, sprinting allowed"), !WeaponCurses::InHand(Holder) && !WeaponCurses::BlocksSprint(Holder));
	TestTrue(TEXT("No holder: no curse"), !WeaponCurses::InHand(nullptr) && !WeaponCurses::BlocksSprint(nullptr) && !WeaponCurses::DrawbackInHand(nullptr));

	struct FHandCase
	{
		const TCHAR* Key;
		bool bLifted;
		bool bBlocks;
	};
	const FHandCase Hands[] = { { TEXT("Cold"), false, true }, { TEXT("Cold"), true, false }, { TEXT("Hungry"), false, false }, { nullptr, false, false } };
	for (const FHandCase& Hand : Hands)
	{
		const FWeaponInstanceData Gun = Hand.Key ? WithCurse(Plain, Hand.Key, Hand.bLifted) : Plain;
		const FString What = FString::Printf(TEXT("%s%s in hand"), Hand.Key ? Hand.Key : TEXT("Uncursed"), Hand.bLifted ? TEXT(" (lifted)") : TEXT(""));
		AWeaponBase* Weapon = Inventory->GiveWeapon(Gun);
		if (!TestNotNull(*What, Weapon) || !TestTrue(FString::Printf(TEXT("%s: it's the one in hand"), *What), Inventory->GetActiveWeapon() == Weapon))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: its curse"), *What), WeaponCurses::InHand(Holder) == (Hand.Key ? WeaponCurses::Find(Hand.Key) : nullptr));
		TestTrue(FString::Printf(TEXT("%s: its drawback"), *What), (WeaponCurses::DrawbackInHand(Holder) != nullptr) == (Hand.Key && !Hand.bLifted));
		TestEqual(FString::Printf(TEXT("%s: sprinting"), *What), WeaponCurses::BlocksSprint(Holder), Hand.bBlocks);
		TestTrue(FString::Printf(TEXT("%s: the weapon keeps its curse"), *What), Weapon->GetInstance().Curse == Gun.Curse
			&& Weapon->GetInstance().bCurseLifted == Gun.bCurseLifted && SameStats(Weapon->GetStats(), Gun.Stats));
	}

	RareOnly->MarkAsGarbage();
	Keepsake->MarkAsGarbage();
	return true;
}

#endif
