#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* const NotchRiflePath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	const TCHAR* const NotchShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");

	/** A rolled gun with a known seed, as one drops: its parts kept with it and its stats rolled. */
	FWeaponInstanceData MakeNotchGun(UWeaponDefinition& Definition, EWeaponRarity Rarity, int32 Seed)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = &Definition;
		Gun.Rarity = Rarity;
		Gun.Level = 4;
		Gun.Seed = Seed;
		Gun.Parts = WeaponParts::PartKeys(WeaponParts::Pick(Definition, Seed, Rarity));
		Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
		return Gun;
	}

	bool NotchNear(double A, double B)
	{
		return FMath::IsNearlyEqual(A, B, FMath::Abs(B) * 1.e-5 + 1.e-5);
	}

	/** Every stat but damage exactly the same. */
	bool SameButDamage(const FWeaponStats& A, const FWeaponStats& B)
	{
		return A.FireRate == B.FireRate && A.MagazineSize == B.MagazineSize && A.ReloadTime == B.ReloadTime && A.Spread == B.Spread
			&& A.Range == B.Range && A.PelletsPerShot == B.PelletsPerShot && A.Recoil == B.Recoil && A.Handling == B.Handling && A.Zoom == B.Zoom;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponNotchesTest, "Looter.Weapons.Notches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponNotchesTest::RunTest(const FString& Parameters)
{
	// Every gun counts its kills: tally marks in its stock (one per 5, the row full at 25), and it wakes at 50 (Blooded,
	// +3% damage), 250 (Named, +6% and a nickname from its kind's list by its seed) and 1,000 (Soul-forged, +10%), saying
	// so on the HUD. A cursed iron's drawback lifts at 100. The count, the curse and its lifting are saved with the gun.
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, NotchRiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, NotchShotgunPath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}

	// Tiers, damage and marks at and around each milestone.
	struct FTierCase
	{
		int32 Kills;
		ENotchTier Tier;
		float Damage;
		int32 Marks;
	};
	const FTierCase Cases[] = {
		{ -3, ENotchTier::None, 1.f, 0 }, { 0, ENotchTier::None, 1.f, 0 }, { 4, ENotchTier::None, 1.f, 0 }, { 5, ENotchTier::None, 1.f, 1 },
		{ 49, ENotchTier::None, 1.f, 9 }, { 50, ENotchTier::Blooded, 1.03f, 10 }, { 124, ENotchTier::Blooded, 1.03f, 24 },
		{ 125, ENotchTier::Blooded, 1.03f, 25 }, { 249, ENotchTier::Blooded, 1.03f, 25 }, { 250, ENotchTier::Named, 1.06f, 25 },
		{ 999, ENotchTier::Named, 1.06f, 25 }, { 1000, ENotchTier::SoulForged, 1.10f, 25 }, { 250000, ENotchTier::SoulForged, 1.10f, 25 } };
	for (const FTierCase& Case : Cases)
	{
		TestEqual(FString::Printf(TEXT("%d kills: its tier"), Case.Kills), WeaponNotches::TierFor(Case.Kills), Case.Tier);
		TestTrue(FString::Printf(TEXT("%d kills: %.2fx damage"), Case.Kills, Case.Damage), NotchNear(WeaponNotches::DamageMultiplier(Case.Kills), Case.Damage));
		TestEqual(FString::Printf(TEXT("%d kills: its marks"), Case.Kills), WeaponNotches::Marks(Case.Kills), Case.Marks);
	}
	TestEqual(TEXT("The row is full at 25 marks"), WeaponNotches::Marks(WeaponNotches::MaxMarks * WeaponNotches::KillsPerMark), WeaponNotches::MaxMarks);
	TestTrue(TEXT("The tiers' words"), WeaponNotches::TierName(ENotchTier::Blooded).ToString() == TEXT("Blooded")
		&& WeaponNotches::TierName(ENotchTier::Named).ToString() == TEXT("Named") && WeaponNotches::TierName(ENotchTier::SoulForged).ToString() == TEXT("Soul-forged")
		&& WeaponNotches::TierName(ENotchTier::None).IsEmpty());

	// Counting, one kill at a time: it wakes exactly three times, each at its milestone.
	const FWeaponInstanceData Fresh = MakeNotchGun(*Rifle, EWeaponRarity::Rare, 4127);
	FWeaponInstanceData Counted = Fresh;
	TArray<TPair<int32, ENotchTier>> Woke;
	for (int32 Kill = 1; Kill <= WeaponNotches::SoulForgedKills; ++Kill)
	{
		const WeaponNotches::FKillResult Result = WeaponNotches::AddKill(Counted);
		if (Result.Reached != ENotchTier::None)
		{
			Woke.Emplace(Counted.Kills, Result.Reached);
		}
		if (Result.bCurseLifted)
		{
			AddError(TEXT("An uncursed gun has no curse to lift"));
		}
	}
	TestEqual(TEXT("Every kill counted"), Counted.Kills, WeaponNotches::SoulForgedKills);
	TestTrue(TEXT("Woke at 50, 250 and 1,000, and only then"), Woke.Num() == 3
		&& Woke[0].Key == WeaponNotches::BloodedKills && Woke[0].Value == ENotchTier::Blooded
		&& Woke[1].Key == WeaponNotches::NamedKills && Woke[1].Value == ENotchTier::Named
		&& Woke[2].Key == WeaponNotches::SoulForgedKills && Woke[2].Value == ENotchTier::SoulForged);
	TestTrue(TEXT("Counting changes nothing else"), Counted.Seed == Fresh.Seed && Counted.Parts == Fresh.Parts && Counted.Curse.IsNone() && !Counted.bCurseLifted);

	// The damage: only damage, by the tier's multiplier, on top of everything the gun rolled; under 50 kills, not a bit.
	for (const int32 Kills : { 1, 49, 50, 250, 1000 })
	{
		FWeaponInstanceData Notched = Fresh;
		Notched.Kills = Kills;
		const FWeaponStats Stats = UWeaponRollLibrary::ComputeInstanceStats(Notched);
		TestTrue(FString::Printf(TEXT("%d kills: its damage"), Kills), NotchNear(Stats.Damage, Fresh.Stats.Damage * WeaponNotches::DamageMultiplier(Kills)));
		TestTrue(FString::Printf(TEXT("%d kills: nothing else"), Kills), SameButDamage(Stats, Fresh.Stats));
		if (Kills < WeaponNotches::BloodedKills)
		{
			TestTrue(FString::Printf(TEXT("%d kills: the very same damage"), Kills), Stats.Damage == Fresh.Stats.Damage);
		}
	}

	// The nickname lists: a dozen or more each, every name its own, none shared between the kinds.
	const TConstArrayView<const TCHAR*> RifleNames = WeaponNotches::Nicknames(EWeaponKind::Rifle);
	const TConstArrayView<const TCHAR*> ShotgunNames = WeaponNotches::Nicknames(EWeaponKind::Shotgun);
	TestTrue(TEXT("A dozen rifle nicknames"), RifleNames.Num() >= 12);
	TestTrue(TEXT("A dozen shotgun nicknames"), ShotgunNames.Num() >= 12);
	TSet<FString> AllNames;
	for (const TConstArrayView<const TCHAR*>& List : { RifleNames, ShotgunNames })
	{
		for (const TCHAR* Name : List)
		{
			TestFalse(FString::Printf(TEXT("%s is its own"), Name), FString(Name).IsEmpty() || AllNames.Contains(Name));
			AllNames.Add(Name);
		}
	}

	// A nickname once Named, from its kind's list, picked by its seed: every copy the same, and every name turns up.
	TMap<FString, int32> RifleSeen;
	for (int32 Seed = 0; Seed < 600; ++Seed)
	{
		FWeaponInstanceData Gun = MakeNotchGun(*Rifle, EWeaponRarity::Uncommon, Seed);
		Gun.Kills = WeaponNotches::NamedKills - 1;
		if (!WeaponNotches::Nickname(Gun).IsEmpty())
		{
			AddError(FString::Printf(TEXT("Seed %d: a nickname before 250 kills"), Seed));
		}
		Gun.Kills = WeaponNotches::NamedKills;
		const FString Nickname = WeaponNotches::Nickname(Gun);
		FWeaponInstanceData Copy = Gun;
		Copy.Kills = 5000;
		Copy.Level = 30;
		Copy.Rarity = EWeaponRarity::Legendary;
		if (Nickname.IsEmpty() || Nickname != WeaponNotches::Nickname(Copy) || !RifleNames.Contains(Nickname))
		{
			AddError(FString::Printf(TEXT("Seed %d: no steady rifle nickname (%s)"), Seed, *Nickname));
		}
		RifleSeen.FindOrAdd(Nickname)++;

		FWeaponInstanceData Pump = MakeNotchGun(*Shotgun, EWeaponRarity::Uncommon, Seed);
		Pump.Kills = WeaponNotches::NamedKills;
		if (!ShotgunNames.Contains(WeaponNotches::Nickname(Pump)))
		{
			AddError(FString::Printf(TEXT("Seed %d: a shotgun named from another list"), Seed));
		}
	}
	TestEqual(TEXT("Every rifle nickname turns up"), RifleSeen.Num(), RifleNames.Num());

	// Its name: as before until Named, then with its nickname in quotes.
	FWeaponInstanceData Gun = Fresh;
	const FString Base = WeaponParts::BaseName(Gun);
	TestEqualSensitive(TEXT("Unnotched: its name as ever"), LooterWeaponText::Name(Gun), Base);
	Gun.Kills = WeaponNotches::NamedKills;
	const FString Nickname = WeaponNotches::Nickname(Gun);
	TestEqualSensitive(TEXT("Named: its nickname in quotes"), LooterWeaponText::Name(Gun), FString::Printf(TEXT("%s \"%s\""), *Base, *Nickname));

	// A named gun has a name already: no nickname, however many notches.
	UNamedWeaponDefinition* Keepsake = NewObject<UNamedWeaponDefinition>(CreatePackage(nullptr), TEXT("DA_Named_TestNotchKeepsake"), RF_Transient);
	Keepsake->Weapon = Shotgun;
	Keepsake->DisplayName = FText::FromString(TEXT("Keepsake"));
	FWeaponInstanceData Kept = MakeNotchGun(*Shotgun, EWeaponRarity::Epic, 11);
	Kept.Named = Keepsake;
	Kept.Kills = 5000;
	TestTrue(TEXT("A named gun has no nickname"), WeaponNotches::Nickname(Kept).IsEmpty());
	TestEqualSensitive(TEXT("...and keeps its own name"), LooterWeaponText::Name(Kept), FString(TEXT("Keepsake")));

	// The HUD's messages name the gun and say what it gained.
	Gun.Kills = WeaponNotches::BloodedKills;
	const FString Blooded = WeaponNotches::MilestoneMessage(Gun, ENotchTier::Blooded).ToString();
	TestTrue(FString::Printf(TEXT("Blooded: %s"), *Blooded), Blooded.StartsWith(Base) && Blooded.Contains(TEXT("Blooded")) && Blooded.Contains(TEXT("+3%")));
	Gun.Kills = WeaponNotches::NamedKills;
	const FString NamedWords = WeaponNotches::MilestoneMessage(Gun, ENotchTier::Named).ToString();
	TestTrue(FString::Printf(TEXT("Named: %s"), *NamedWords), NamedWords.StartsWith(Base) && NamedWords.Contains(Nickname) && NamedWords.Contains(TEXT("+6%")));
	Gun.Kills = WeaponNotches::SoulForgedKills;
	const FString Forged = WeaponNotches::MilestoneMessage(Gun, ENotchTier::SoulForged).ToString();
	TestTrue(FString::Printf(TEXT("Soul-forged: %s"), *Forged), Forged.StartsWith(LooterWeaponText::Name(Gun)) && Forged.Contains(TEXT("Soul-forged"))
		&& Forged.Contains(TEXT("+10%")));
	TestTrue(TEXT("No milestone, no message"), WeaponNotches::MilestoneMessage(Gun, ENotchTier::None).IsEmpty());

	// The cards' count.
	FWeaponInstanceData Card = Fresh;
	TestTrue(TEXT("No notches, no count"), LooterWeaponText::NotchesString(Card).IsEmpty());
	Card.Kills = 1;
	TestEqualSensitive(TEXT("One notch"), LooterWeaponText::NotchesString(Card), FString(TEXT("1 NOTCH")));
	Card.Kills = 137;
	TestEqualSensitive(TEXT("137 notches"), LooterWeaponText::NotchesString(Card), FString(TEXT("137 NOTCHES")));

	// A cursed iron carried to 100 notches: the drawback lifts with the hundredth kill, once, and the curse stays.
	FWeaponInstanceData Cursed = Fresh;
	Cursed.Curse = TEXT("Hungry");
	Cursed.Kills = WeaponNotches::CurseLiftKills - 2;
	TestFalse(TEXT("At 99: not yet"), WeaponNotches::AddKill(Cursed).bCurseLifted || Cursed.bCurseLifted);
	TestTrue(TEXT("At 99: its drawback still bites"), WeaponCurses::DrawbackActive(Cursed));
	const WeaponNotches::FKillResult Hundredth = WeaponNotches::AddKill(Cursed);
	TestTrue(TEXT("At 100: lifted"), Hundredth.bCurseLifted && Cursed.bCurseLifted && !WeaponCurses::DrawbackActive(Cursed));
	TestEqual(TEXT("...still Hungry, its perk kept"), Cursed.Curse, FName(TEXT("Hungry")));
	TestTrue(TEXT("...and said"), WeaponNotches::CurseLiftedMessage(Cursed).ToString().Contains(TEXT("curse is lifted")));
	TestFalse(TEXT("Lifted once"), WeaponNotches::AddKill(Cursed).bCurseLifted);
	FWeaponInstanceData Salted = Fresh;
	Salted.Curse = TEXT("Cold");
	Salted.bCurseLifted = true;
	Salted.Kills = WeaponNotches::CurseLiftKills - 1;
	TestFalse(TEXT("Already lifted: nothing to lift at 100"), WeaponNotches::AddKill(Salted).bCurseLifted);
	FWeaponInstanceData Unknown = Fresh;
	Unknown.Curse = TEXT("NoSuchCurseAnyMore");
	Unknown.Kills = WeaponNotches::CurseLiftKills - 1;
	TestFalse(TEXT("A curse that no longer exists doesn't lift"), WeaponNotches::AddKill(Unknown).bCurseLifted);

	// Saved with the gun, in hand or in the backpack; a gun without them reads none.
	FWeaponInstanceData Carried = Fresh;
	Carried.Kills = 137;
	Carried.Curse = TEXT("Cold");
	Carried.bCurseLifted = true;
	Carried.Stats = UWeaponRollLibrary::ComputeInstanceStats(Carried);
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->bHasInventory = true;
	Save->Inventory.Equipped = { Carried };
	Save->Inventory.ActiveSlot = 0;
	Save->Inventory.Backpack = { Fresh };
	TArray<uint8> Bytes;
	const ULooterSessionSave* Read = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	if (!TestNotNull(TEXT("Written and read back"), Read) || !TestEqual(TEXT("The gun in hand"), Read->Inventory.Equipped.Num(), 1)
		|| !TestEqual(TEXT("The gun in the backpack"), Read->Inventory.Backpack.Num(), 1))
	{
		return false;
	}
	const FWeaponInstanceData& Back = Read->Inventory.Equipped[0];
	TestTrue(TEXT("Its notches, curse and lifting come back"), Back.Kills == 137 && Back.Curse == FName(TEXT("Cold")) && Back.bCurseLifted);
	TestTrue(TEXT("...and its stats with them"), NotchNear(UWeaponRollLibrary::ComputeInstanceStats(Back).Damage, Carried.Stats.Damage));
	const FWeaponInstanceData& Plain = Read->Inventory.Backpack[0];
	TestTrue(TEXT("A plain gun stays plain"), Plain.Kills == 0 && Plain.Curse.IsNone() && !Plain.bCurseLifted);

	Keepsake->MarkAsGarbage();
	return true;
}

#endif
