#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponMakers.h"
#include "Affixes/WeaponMakerTuning.h"
#include "Affixes/WeaponRollLibrary.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "HAL/IConsoleManager.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/Package.h"

// The gun makers (Affixes/WeaponMakers; Docs/Work/2026-10-22-Next/MAKERS.md): the table, the roll, the quirk on the
// stats, the part leanings, the paint, the name, the mark and old saves.

namespace
{
	const EWeaponRarity MakerRarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic,
		EWeaponRarity::Legendary };

	UWeaponDefinition* LoadMakerGun(const TCHAR* Name)
	{
		return LoadObject<UWeaponDefinition>(nullptr, *FString::Printf(TEXT("/Game/Weapons/Data/%s.%s"), Name, Name));
	}

	/** A gun made by hand with a known seed and maker (None: no maker), its parts picked as a roll picks them. */
	FWeaponInstanceData MakerGun(UWeaponDefinition& Definition, EWeaponRarity Rarity, int32 Seed, FName Maker)
	{
		FWeaponInstanceData Gun;
		Gun.Definition = &Definition;
		Gun.Rarity = Rarity;
		Gun.Level = 4;
		Gun.Seed = Seed;
		Gun.Maker = Maker;
		Gun.Parts = WeaponParts::PartKeys(WeaponParts::Pick(Definition, Seed, Rarity, {}, WeaponMakers::Of(Gun)));
		Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
		return Gun;
	}

	bool MakerNear(double A, double B)
	{
		return FMath::IsNearlyEqual(A, B, FMath::Abs(B) * 1.e-4 + 1.e-4);
	}

	bool SameMakerStats(const FWeaponStats& A, const FWeaponStats& B)
	{
		return A.Damage == B.Damage && A.FireRate == B.FireRate && A.MagazineSize == B.MagazineSize && A.ReloadTime == B.ReloadTime
			&& A.Spread == B.Spread && A.Range == B.Range && A.Recoil == B.Recoil && A.Handling == B.Handling && A.Zoom == B.Zoom;
	}

	/** Sets a cvar for the scope of a test and puts it back. */
	struct FMakerCVarScope
	{
		IConsoleVariable* Var = nullptr;
		FString Was;
		FMakerCVarScope(const TCHAR* Name, const TCHAR* Value)
			: Var(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Var)
			{
				Was = Var->GetString();
				Var->Set(Value, ECVF_SetByCode);
			}
		}
		~FMakerCVarScope()
		{
			if (Var)
			{
				Var->Set(*Was, ECVF_SetByCode);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMakersTableTest, "Looter.Weapons.Makers.Table",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponMakersTableTest::RunTest(const FString& Parameters)
{
	const TCHAR* const Keys[] = { TEXT("Roundhouse"), TEXT("SaintMaud"), TEXT("Windfall"), TEXT("Meridian") };
	TestEqual(TEXT("Four makers"), WeaponMakers::All().Num(), 4);
	TSet<int32> Cells;
	for (const TCHAR* Key : Keys)
	{
		const FWeaponMaker* Maker = WeaponMakers::Find(Key);
		if (!TestNotNull(FString::Printf(TEXT("%s is in the table (saved keys never change)"), Key), Maker))
		{
			continue;
		}
		TestFalse(FString::Printf(TEXT("%s has a name"), Key), Maker->Name.IsEmpty() || Maker->FullName.IsEmpty() || Maker->Fiction.IsEmpty()
			|| Maker->QuirkName.IsEmpty() || Maker->Place.IsEmpty());
		TestTrue(FString::Printf(TEXT("%s repaints the shell"), Key), Maker->Paints.ContainsByPredicate([](const FWeaponPaint& Paint)
		{
			return Paint.Slot == FName(TEXT("GunPolymerSand")) && !Paint.Colors.IsEmpty();
		}));
		// A salvage body (all GunSalvage rust, no shell slot) wears the maker's shell colour over its rust, so its paint and
		// its mark show (PIE 2026-10-22: a Saint Maud Salvager launcher stayed rust-teal, the mark lost in the rust).
		FWeaponLook Painted;
		WeaponMakers::PaintLook(*Maker, 1234, Painted);
		const FLinearColor* PaintedShell = Painted.Colors.Find(TEXT("GunPolymerSand"));
		const FLinearColor* PaintedSalvage = Painted.Colors.Find(TEXT("GunSalvage"));
		TestTrue(FString::Printf(TEXT("%s paints a salvage body as its shell"), Key), PaintedShell && PaintedSalvage && *PaintedShell == *PaintedSalvage);
		TestTrue(FString::Printf(TEXT("%s's mark is a cell of the atlas"), Key), Maker->MarkCell >= 1 && Maker->MarkCell <= 4);
		Cells.Add(Maker->MarkCell);
		TestEqual(FString::Printf(TEXT("%s's mark icon"), Key), Maker->MarkIcon, FName(*FString::Printf(TEXT("Maker%s"), Key)));
		TestNotNull(FString::Printf(TEXT("%s has cvars"), Key),
			IConsoleManager::Get().FindConsoleVariable(*FString::Printf(TEXT("Looter.Makers.%s.Damage"), Key)));
		float Total = 0.f;
		for (const EWeaponRarity Rarity : MakerRarities)
		{
			Total += WeaponMakers::Weight(*Maker, EWeaponKind::Rifle, Rarity);
		}
		TestTrue(FString::Printf(TEXT("%s makes rifles"), Key), Total > 0.f);
	}
	TestEqual(TEXT("Each maker its own mark"), Cells.Num(), 4);
	TestEqual(TEXT("Old guns become Roundhouse"), WeaponMakers::DefaultKey(), FName(TEXT("Roundhouse")));
	TestNull(TEXT("No maker for None"), WeaponMakers::Find(NAME_None));
	TestNull(TEXT("...or a key that's gone"), WeaponMakers::Find(TEXT("NoSuchMaker")));

	// The shipped numbers (the concept the user saw).
	const FWeaponMakerQuirk Meridian = WeaponMakerTuning::Quirk(TEXT("Meridian"));
	TestTrue(TEXT("Meridian: accuracy +30, range +20, handling +10, fire rate -10, magazine -20"), Meridian.Accuracy == 30.f
		&& Meridian.Range == 20.f && Meridian.Handling == 10.f && Meridian.FireRate == -10.f && Meridian.Magazine == -20.f);
	TestEqual(TEXT("Saint Maud: element parts x3"), WeaponMakerTuning::Quirk(TEXT("SaintMaud")).ElementPartBias, 3.f);
	TestEqual(TEXT("Windfall: effect parts x2"), WeaponMakerTuning::Quirk(TEXT("Windfall")).EffectPartBias, 2.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMakersParseTest, "Looter.Weapons.Makers.Parse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponMakersParseTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Odds at Common"), WeaponMakerTuning::OddsAt(TEXT("4,3,2,1.5,1"), EWeaponRarity::Common), 4.f);
	TestEqual(TEXT("Odds at Epic"), WeaponMakerTuning::OddsAt(TEXT("4, 3, 2, 1.5, 1"), EWeaponRarity::Epic), 1.5f);
	TestEqual(TEXT("A missing entry is 0"), WeaponMakerTuning::OddsAt(TEXT("4,3"), EWeaponRarity::Legendary), 0.f);
	TestEqual(TEXT("A bad entry is 0"), WeaponMakerTuning::OddsAt(TEXT("4,x,2,2,2"), EWeaponRarity::Uncommon), 0.f);
	TestEqual(TEXT("Never below 0"), WeaponMakerTuning::OddsAt(TEXT("-4,3,2,2,2"), EWeaponRarity::Common), 0.f);
	TestEqual(TEXT("A named kind"), WeaponMakerTuning::LeanFor(TEXT("LMG=1.5,Launcher=0.4"), EWeaponKind::Launcher), 0.4f);
	TestEqual(TEXT("Any case"), WeaponMakerTuning::LeanFor(TEXT("sniper = 2"), EWeaponKind::Sniper), 2.f);
	TestEqual(TEXT("An unnamed kind leans 1"), WeaponMakerTuning::LeanFor(TEXT("LMG=1.5"), EWeaponKind::Rifle), 1.f);
	TestEqual(TEXT("Nothing named: 1"), WeaponMakerTuning::LeanFor(TEXT(""), EWeaponKind::Shotgun), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMakersRollTest, "Looter.Weapons.Makers.Roll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponMakersRollTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("The same seed, the same maker"), WeaponMakers::Roll(EWeaponKind::Rifle, 12345, EWeaponRarity::Rare),
		WeaponMakers::Roll(EWeaponKind::Rifle, 12345, EWeaponRarity::Rare));
	constexpr int32 Rolls = 6000;
	auto Shares = [](EWeaponKind Kind, EWeaponRarity Rarity)
	{
		TMap<FName, float> Counted;
		for (int32 Seed = 0; Seed < Rolls; ++Seed)
		{
			Counted.FindOrAdd(WeaponMakers::Roll(Kind, Seed * 7919 + 101, Rarity)) += 1.f / Rolls;
		}
		return Counted;
	};
	for (const EWeaponKind Kind : { EWeaponKind::Rifle, EWeaponKind::Revolver, EWeaponKind::Launcher })
	{
		for (const EWeaponRarity Rarity : MakerRarities)
		{
			const TMap<FName, float> Rolled = Shares(Kind, Rarity);
			float Total = 0.f;
			for (const FWeaponMaker& Maker : WeaponMakers::All())
			{
				Total += WeaponMakers::Weight(Maker, Kind, Rarity);
			}
			for (const FWeaponMaker& Maker : WeaponMakers::All())
			{
				const float Expected = WeaponMakers::Weight(Maker, Kind, Rarity) / Total;
				TestTrue(FString::Printf(TEXT("%s %s: %s rolls by its weight (%.3f, expected %.3f)"), *UEnum::GetValueAsString(Kind),
					*UEnum::GetValueAsString(Rarity), *Maker.Key.ToString(), Rolled.FindRef(Maker.Key), Expected),
					FMath::Abs(Rolled.FindRef(Maker.Key) - Expected) < 0.025f);
			}
		}
	}
	// The user's promise: the common stock is common, the fine work comes with rarity.
	const TMap<FName, float> Common = Shares(EWeaponKind::Rifle, EWeaponRarity::Common);
	const TMap<FName, float> Legendary = Shares(EWeaponKind::Rifle, EWeaponRarity::Legendary);
	TestTrue(TEXT("Common rifles: mostly Roundhouse and Windfall"), Common.FindRef(TEXT("Roundhouse")) + Common.FindRef(TEXT("Windfall")) > 0.6f);
	TestTrue(TEXT("Legendary rifles: mostly Saint Maud and Meridian"), Legendary.FindRef(TEXT("SaintMaud")) + Legendary.FindRef(TEXT("Meridian")) > 0.5f);
	TestTrue(TEXT("Meridian leans to snipers, away from launchers"), Shares(EWeaponKind::Sniper, EWeaponRarity::Rare).FindRef(TEXT("Meridian"))
		> 2.f * Shares(EWeaponKind::Launcher, EWeaponRarity::Rare).FindRef(TEXT("Meridian")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMakersQuirkTest, "Looter.Weapons.Makers.Quirk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponMakersQuirkTest::RunTest(const FString& Parameters)
{
	FWeaponStats Base;
	Base.Damage = 40.f;
	Base.FireRate = 600.f;
	Base.MagazineSize = 30;
	Base.ReloadTime = 2.f;
	Base.Spread = 1.3f;
	Base.Range = 4000.f;
	Base.Recoil = 1.f;
	Base.Handling = 1.f;
	FWeaponMakerQuirk Quirk;
	Quirk.Damage = 10.f;
	Quirk.Accuracy = 30.f;
	Quirk.Range = 20.f;
	Quirk.FireRate = -10.f;
	Quirk.Reload = 15.f;
	Quirk.Recoil = -25.f;
	Quirk.Handling = -15.f;
	Quirk.Magazine = 30.f;
	FWeaponStats Rifle = Base;
	WeaponMakers::ApplyQuirk(Quirk, EWeaponKind::Rifle, Rifle);
	TestTrue(TEXT("Damage +10%"), MakerNear(Rifle.Damage, 44.f));
	TestTrue(TEXT("Accuracy +30%: 1/1.3 as wide"), MakerNear(Rifle.Spread, 1.3f / 1.3f));
	TestTrue(TEXT("Range +20%"), MakerNear(Rifle.Range, 4800.f));
	TestTrue(TEXT("Fire rate -10%"), MakerNear(Rifle.FireRate, 540.f));
	TestTrue(TEXT("Reload time +15%"), MakerNear(Rifle.ReloadTime, 2.3f));
	TestTrue(TEXT("Recoil -25%"), MakerNear(Rifle.Recoil, 0.75f));
	TestTrue(TEXT("Handling -15%"), MakerNear(Rifle.Handling, 0.85f));
	TestEqual(TEXT("Magazine +30%"), Rifle.MagazineSize, 39);
	for (const EWeaponKind Kind : { EWeaponKind::Revolver, EWeaponKind::Launcher })
	{
		FWeaponStats Shown = Base;
		WeaponMakers::ApplyQuirk(Quirk, Kind, Shown);
		TestEqual(FString::Printf(TEXT("%s: its rounds show, its magazine stays"), *UEnum::GetValueAsString(Kind)), Shown.MagazineSize, 30);
	}
	FWeaponMakerQuirk Small;
	Small.Magazine = -90.f;
	FWeaponStats One = Base;
	One.MagazineSize = 1;
	WeaponMakers::ApplyQuirk(Small, EWeaponKind::Rifle, One);
	TestEqual(TEXT("Never below one round"), One.MagazineSize, 1);
	FWeaponStats Untouched = Base;
	WeaponMakers::ApplyQuirk(FWeaponMakerQuirk(), EWeaponKind::Rifle, Untouched);
	TestTrue(TEXT("An empty quirk changes nothing"), SameMakerStats(Untouched, Base));

	const FWeaponMaker* Meridian = WeaponMakers::Find(TEXT("Meridian"));
	if (TestNotNull(TEXT("Meridian"), Meridian))
	{
		{
			FMakerCVarScope Off(TEXT("Looter.Makers.QuirkScale"), TEXT("0"));
			const FWeaponMakerQuirk None = WeaponMakers::QuirkOf(*Meridian);
			TestTrue(TEXT("Scale 0: no trade, no leaning"), None.Accuracy == 0.f && None.Magazine == 0.f && None.ElementPartBias == 1.f);
		}
		{
			FMakerCVarScope Tuned(TEXT("Looter.Makers.Meridian.Accuracy"), TEXT("50"));
			TestEqual(TEXT("A cvar tunes it live"), WeaponMakers::QuirkOf(*Meridian).Accuracy, 50.f);
		}
		TestEqual(TEXT("...and back"), WeaponMakers::QuirkOf(*Meridian).Accuracy, 30.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMakersGunsTest, "Looter.Weapons.Makers.Guns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponMakersGunsTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Rifle = LoadMakerGun(TEXT("DA_AssaultRifle"));
	UWeaponDefinition* Revolver = LoadMakerGun(TEXT("DA_Revolver"));
	if (!TestNotNull(TEXT("DA_AssaultRifle"), Rifle) || !TestNotNull(TEXT("DA_Revolver"), Revolver) || Rifle->Parts.IsEmpty())
	{
		return false;
	}

	// A new gun rolls its maker from its seed and is built with it.
	for (int32 Index = 0; Index < 40; ++Index)
	{
		const FWeaponInstanceData Rolled = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, MakerRarities[Index % 5], 3);
		TestNotNull(TEXT("A rolled gun has a maker"), WeaponMakers::Of(Rolled));
		TestEqual(TEXT("...the one its seed rolls"), Rolled.Maker, WeaponMakers::Roll(Rifle, Rolled.Seed, Rolled.Rarity));
		TestTrue(TEXT("...its stats as a weapon rebuilds them"), SameMakerStats(Rolled.Stats, UWeaponRollLibrary::ComputeInstanceStats(Rolled)));
		TestTrue(TEXT("...and its parts as it shows them"), WeaponParts::PartKeys(WeaponParts::Pick(Rolled)) == Rolled.Parts);
	}

	// No maker: exactly the numbers, look, wear and name a gun had before makers.
	for (int32 Seed = 0; Seed < 30; ++Seed)
	{
		const FWeaponInstanceData Plain = MakerGun(*Rifle, EWeaponRarity::Rare, Seed, NAME_None);
		TestTrue(TEXT("No maker: its stats as ever"), SameMakerStats(Plain.Stats,
			UWeaponRollLibrary::ComputeStatsWithParts(Rifle, Plain.Rarity, Plain.Level, Plain.Seed, Plain.Parts)));
		TestTrue(TEXT("No maker: its paint as ever"), WeaponParts::Pick(Plain).Colors.OrderIndependentCompareEqual(
			WeaponParts::Pick(*Rifle, Seed, Plain.Rarity, Plain.Parts).Colors));
		TestTrue(TEXT("No maker: no maker line"), LooterWeaponText::MakerString(Plain).IsEmpty() && LooterWeaponText::MakerQuirkLine(Plain).IsEmpty());
	}

	// Each maker: its trade on the same gun, its paint, its wear and its word.
	for (const FWeaponMaker& Maker : WeaponMakers::All())
	{
		const FWeaponMakerQuirk Quirk = WeaponMakers::QuirkOf(Maker);
		for (int32 Seed = 0; Seed < 20; ++Seed)
		{
			const FWeaponInstanceData Plain = MakerGun(*Rifle, EWeaponRarity::Uncommon, Seed, NAME_None);
			FWeaponInstanceData Made = Plain;
			Made.Maker = Maker.Key;
			Made.Stats = UWeaponRollLibrary::ComputeInstanceStats(Made);
			const FString What = FString::Printf(TEXT("%s seed %d"), *Maker.Key.ToString(), Seed);
			TestTrue(What + TEXT(": damage"), MakerNear(Made.Stats.Damage, Plain.Stats.Damage * (1.f + Quirk.Damage * 0.01f)));
			TestTrue(What + TEXT(": fire rate"), MakerNear(Made.Stats.FireRate, Plain.Stats.FireRate * (1.f + Quirk.FireRate * 0.01f)));
			TestTrue(What + TEXT(": spread"), MakerNear(Made.Stats.Spread, Plain.Stats.Spread / (1.f + Quirk.Accuracy * 0.01f)));
			TestEqual(What + TEXT(": magazine"), Made.Stats.MagazineSize,
				FMath::Max(1, FMath::RoundToInt(Plain.Stats.MagazineSize * (1.f + Quirk.Magazine * 0.01f))));
			const FWeaponLook Look = WeaponParts::Pick(Made);
			const FLinearColor* Shell = Look.Colors.Find(TEXT("GunPolymerSand"));
			const FWeaponPaint* MakerShell = Maker.Paints.FindByPredicate([](const FWeaponPaint& Paint) { return Paint.Slot == FName(TEXT("GunPolymerSand")); });
			TestTrue(What + TEXT(": its shell in the maker's paint"), Shell && MakerShell && MakerShell->Colors.ContainsByPredicate(
				[Shell](const FWeaponColorOption& Option) { return Option.Color.Equals(*Shell, 1.e-4f); }));
			TestTrue(What + TEXT(": the same parts"), WeaponParts::PartKeys(Look) == Plain.Parts);
			TestTrue(What + TEXT(": its wear"), MakerNear(WeaponParts::Wear(Made), FMath::Clamp(WeaponParts::Wear(Plain) + Quirk.Wear, 0.f, 1.f)));
			const FText Word = WeaponParts::NamePrefix(Look);
			const FString Kind = Rifle->DisplayName.ToString();
			TestEqualSensitive(What + TEXT(": its name"), WeaponParts::BaseName(Made),
				Word.IsEmpty() ? Maker.Name.ToString() + TEXT(" ") + Kind : WeaponParts::BaseName(Plain));
			TestEqualSensitive(What + TEXT(": its maker line"), LooterWeaponText::MakerString(Made), Maker.Name.ToString().ToUpper());
			TestTrue(What + TEXT(": its quirk line"), LooterWeaponText::MakerQuirkLine(Made).StartsWith(Maker.QuirkName.ToString().ToUpper()));
		}
	}

	// The revolver keeps its cylinder's rounds.
	for (int32 Seed = 0; Seed < 20; ++Seed)
	{
		const FWeaponInstanceData Plain = MakerGun(*Revolver, EWeaponRarity::Rare, Seed, NAME_None);
		FWeaponInstanceData Roundhouse = Plain;
		Roundhouse.Maker = TEXT("Roundhouse");
		TestEqual(TEXT("A Roundhouse Drover's cylinder holds what it shows"), UWeaponRollLibrary::ComputeInstanceStats(Roundhouse).MagazineSize,
			Plain.Stats.MagazineSize);
	}

	// A named gun has no maker, whatever its field says.
	UNamedWeaponDefinition* Named = NewObject<UNamedWeaponDefinition>(GetTransientPackage());
	FWeaponInstanceData NamedGun = MakerGun(*Rifle, EWeaponRarity::Epic, 7, TEXT("Meridian"));
	NamedGun.Named = Named;
	TestNull(TEXT("A named gun: no maker"), WeaponMakers::Of(NamedGun));
	TestTrue(TEXT("...no maker line"), LooterWeaponText::MakerString(NamedGun).IsEmpty());

	// The mark: on the first part with a place, in the maker's cell and colour.
	UWeaponDefinition* Marked = DuplicateObject(Rifle, GetTransientPackage());
	for (FWeaponPartOption& Option : Marked->Parts[0].Options)
	{
		Option.MarkPlace.X = 29.f;
		Option.MarkPlace.Z = -0.8f;
		Option.MarkPlace.Size = 4.f;
		Option.MarkPlace.Depth = 1.9f;
	}
	const FWeaponInstanceData MeridianGun = MakerGun(*Marked, EWeaponRarity::Rare, 3, TEXT("Meridian"));
	const FWeaponMarkPaint Mark = WeaponMakers::MarkFor(MeridianGun, WeaponParts::Pick(MeridianGun));
	TestTrue(TEXT("Marked on the body"), Mark.IsSet() && Mark.PartIndex == 0 && Mark.Place.Size == 4.f);
	TestEqual(TEXT("...in Meridian's cell"), Mark.Cell, static_cast<float>(WeaponMakers::Find(TEXT("Meridian"))->MarkCell));
	TestFalse(TEXT("No maker: no mark"), WeaponMakers::MarkFor(MakerGun(*Marked, EWeaponRarity::Rare, 3, NAME_None),
		WeaponParts::Pick(*Marked, 3, EWeaponRarity::Rare)).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMakersBiasTest, "Looter.Weapons.Makers.Bias",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponMakersBiasTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Rifle = LoadMakerGun(TEXT("DA_AssaultRifle"));
	if (!TestNotNull(TEXT("DA_AssaultRifle"), Rifle) || Rifle->Parts.IsEmpty())
	{
		return false;
	}
	constexpr int32 Guns = 1500;
	auto Count = [Rifle](FName MakerKey, EWeaponRarity Rarity, bool bElement)
	{
		int32 Found = 0;
		for (int32 Seed = 0; Seed < Guns; ++Seed)
		{
			const FWeaponInstanceData Gun = MakerGun(*Rifle, Rarity, Seed * 31 + 7, MakerKey);
			const bool bHas = bElement ? WeaponParts::ElementOf(Gun) != ELooterElement::None : WeaponParts::EffectOf(Gun) != EPartEffect::None;
			Found += bHas ? 1 : 0;
		}
		return Found;
	};
	const int32 PlainElements = Count(NAME_None, EWeaponRarity::Uncommon, true);
	const int32 MaudElements = Count(TEXT("SaintMaud"), EWeaponRarity::Uncommon, true);
	TestTrue(FString::Printf(TEXT("Saint Maud's rounds are blessed more often (%d vs %d of %d)"), MaudElements, PlainElements, Guns),
		PlainElements > 0 && MaudElements > 2 * PlainElements);
	const int32 PlainEffects = Count(NAME_None, EWeaponRarity::Epic, false);
	const int32 WindfallEffects = Count(TEXT("Windfall"), EWeaponRarity::Epic, false);
	TestTrue(FString::Printf(TEXT("Windfall's odd bits come more often (%d vs %d of %d)"), WindfallEffects, PlainEffects, Guns),
		PlainEffects > 0 && WindfallEffects * 10 > PlainEffects * 13);
	TestEqual(TEXT("Roundhouse doesn't lean"), Count(TEXT("Roundhouse"), EWeaponRarity::Uncommon, true), PlainElements);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMakersOldSaveTest, "Looter.Weapons.Makers.OldSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponMakersOldSaveTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Rifle = LoadMakerGun(TEXT("DA_AssaultRifle"));
	if (!TestNotNull(TEXT("DA_AssaultRifle"), Rifle))
	{
		return false;
	}
	auto RoundTrip = [](const FWeaponInstanceData& Gun)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		FObjectAndNameAsStringProxyArchive WriteProxy(Writer, false);
		FWeaponInstanceData Saved = Gun;
		FWeaponInstanceData::StaticStruct()->SerializeItem(WriteProxy, &Saved, nullptr);
		FWeaponInstanceData Read;
		FMemoryReader Reader(Bytes, true);
		FObjectAndNameAsStringProxyArchive ReadProxy(Reader, true);
		FWeaponInstanceData::StaticStruct()->SerializeItem(ReadProxy, &Read, nullptr);
		return Read;
	};
	const FWeaponInstanceData Old = MakerGun(*Rifle, EWeaponRarity::Rare, 5, NAME_None);
	TestEqual(TEXT("A gun saved before makers loads as Roundhouse"), RoundTrip(Old).Maker, FName(TEXT("Roundhouse")));
	TestEqual(TEXT("A gun with a maker keeps it"), RoundTrip(MakerGun(*Rifle, EWeaponRarity::Rare, 5, TEXT("Meridian"))).Maker,
		FName(TEXT("Meridian")));
	// A named gun takes none (its fix-up straight, as a transient named definition can't be found again by a load).
	FWeaponInstanceData Named = Old;
	Named.Named = NewObject<UNamedWeaponDefinition>(GetTransientPackage());
	TArray<uint8> Nothing;
	FMemoryReader Loading(Nothing, true);
	Named.PostSerialize(Loading);
	TestTrue(TEXT("A named gun takes none"), Named.Maker.IsNone());
	FWeaponInstanceData Unnamed = Old;
	Unnamed.PostSerialize(Loading);
	TestEqual(TEXT("...an unnamed one the default"), Unnamed.Maker, FName(TEXT("Roundhouse")));
	TestTrue(TEXT("No gun, no maker"), RoundTrip(FWeaponInstanceData()).Maker.IsNone());
	FWeaponInstanceData Gone = Old;
	Gone.Maker = TEXT("NoSuchMaker");
	TestNull(TEXT("A maker that's gone reads as none"), WeaponMakers::Of(RoundTrip(Gone)));
	return true;
}

#endif
