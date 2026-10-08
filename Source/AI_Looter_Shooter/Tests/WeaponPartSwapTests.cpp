#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponPartSwap.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	using ECheck = WeaponPartSwap::ECheck;

	const TCHAR* const SwapRiflePath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	const TCHAR* const SwapShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");
	const EWeaponRarity SwapRarities[] = { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic, EWeaponRarity::Legendary };

	int32 SwapSlot(const UWeaponDefinition& Definition, FName Slot)
	{
		return Definition.Parts.IndexOfByPredicate([Slot](const FWeaponPartSlot& Each) { return Each.Name == Slot; });
	}

	/** A rolled gun with a known seed, with the parts asked for ("Barrel=Short") in place of the ones it rolled. */
	FWeaponInstanceData MakeSwapGun(UWeaponDefinition& Definition, EWeaponRarity Rarity, int32 Seed, const TArray<FString>& With = {})
	{
		FWeaponInstanceData Gun;
		Gun.Definition = &Definition;
		Gun.Rarity = Rarity;
		Gun.Level = 6;
		Gun.Seed = Seed;
		Gun.Parts = WeaponParts::PartKeys(WeaponParts::Pick(Definition, Seed, Rarity));
		for (const FString& Part : With)
		{
			FString Slot;
			FString Key;
			Part.Split(TEXT("="), &Slot, &Key);
			const int32 Index = SwapSlot(Definition, FName(*Slot));
			if (Gun.Parts.IsValidIndex(Index))
			{
				Gun.Parts[Index] = FName(*Key);
			}
		}
		Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
		return Gun;
	}

	FBoxedWeaponPart Boxed(UWeaponDefinition* Definition, const TCHAR* Slot, const TCHAR* Key)
	{
		FBoxedWeaponPart Part;
		Part.Definition = Definition;
		Part.Slot = Slot;
		Part.Key = Key ? FName(Key) : NAME_None;
		return Part;
	}

	FName KeyIn(const FWeaponInstanceData& Gun, const TCHAR* Slot)
	{
		const int32 Index = Gun.Definition ? SwapSlot(*Gun.Definition, Slot) : INDEX_NONE;
		return Gun.Parts.IsValidIndex(Index) ? Gun.Parts[Index] : NAME_None;
	}

	/** Exactly the same numbers. */
	bool SameSwapStats(const FWeaponStats& A, const FWeaponStats& B)
	{
		return A.Damage == B.Damage && A.FireRate == B.FireRate && A.MagazineSize == B.MagazineSize && A.ReloadTime == B.ReloadTime
			&& A.Spread == B.Spread && A.Range == B.Range && A.PelletsPerShot == B.PelletsPerShot && A.Recoil == B.Recoil
			&& A.Handling == B.Handling && A.Zoom == B.Zoom;
	}

	bool SameTotals(const FWeaponPartTotals& A, const FWeaponPartTotals& B)
	{
		return A.Damage == B.Damage && A.Accuracy == B.Accuracy && A.Range == B.Range && A.FireRate == B.FireRate && A.Reload == B.Reload
			&& A.Recoil == B.Recoil && A.Handling == B.Handling && A.Magazine == B.Magazine && A.Zoom == B.Zoom;
	}

	/**
	 * The parts' rolls as guns have always drawn them, written out here on purpose: each part from a stream of the gun's
	 * seed and the part's key, its stats in a fixed order, then capped. If WeaponParts::CombinedStats ever draws otherwise,
	 * every gun already found would change its numbers, and this test says so.
	 */
	FWeaponPartTotals DrawnAsEver(const FWeaponLook& Look, int32 Seed)
	{
		FWeaponPartTotals Totals;
		for (int32 Index = 0; Index < Look.Parts.Num(); ++Index)
		{
			const FWeaponPartOption* Part = Look.Parts[Index];
			if (!Part)
			{
				continue;
			}
			const uint32 PartHash = Part->Key.IsNone() ? static_cast<uint32>(Index) : GetTypeHash(Part->Key);
			FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), HashCombine(PartHash, 0x51ED270Bu))));
			const FWeaponPartStats& Stats = Part->Stats;
			Totals.Damage += Stats.Damage.At(Random.FRand());
			Totals.Accuracy += Stats.Accuracy.At(Random.FRand());
			Totals.Range += Stats.Range.At(Random.FRand());
			Totals.FireRate += Stats.FireRate.At(Random.FRand());
			Totals.Reload += Stats.Reload.At(Random.FRand());
			Totals.Recoil += Stats.Recoil.At(Random.FRand());
			Totals.Handling += Stats.Handling.At(Random.FRand());
			Totals.Magazine = Stats.Magazine > 0 ? Stats.Magazine : Totals.Magazine;
			Totals.Zoom = Stats.Zoom > 0.f ? Stats.Zoom : Totals.Zoom;
		}
		auto Cap = [](float Total, const WeaponParts::FCap& Limit) { return FMath::Clamp(Total, Limit.Min, Limit.Max); };
		Totals.Damage = Cap(Totals.Damage, WeaponParts::DamageCap);
		Totals.Accuracy = Cap(Totals.Accuracy, WeaponParts::AccuracyCap);
		Totals.Range = Cap(Totals.Range, WeaponParts::RangeCap);
		Totals.FireRate = Cap(Totals.FireRate, WeaponParts::FireRateCap);
		Totals.Reload = Cap(Totals.Reload, WeaponParts::ReloadCap);
		Totals.Recoil = Cap(Totals.Recoil, WeaponParts::RecoilCap);
		Totals.Handling = Cap(Totals.Handling, WeaponParts::HandlingCap);
		return Totals;
	}

	/** The word the gun's highest-priority named part gives it (the earlier slot wins a tie), worked out here. */
	FString ExpectedWord(const FWeaponInstanceData& Gun)
	{
		const FWeaponPartOption* Namer = nullptr;
		for (const FWeaponPartOption* Part : WeaponParts::Pick(Gun).Parts)
		{
			Namer = Part && !Part->NamePrefix.IsEmpty() && (!Namer || Part->NamePriority > Namer->NamePriority) ? Part : Namer;
		}
		return Namer ? Namer->NamePrefix.ToString() : FString();
	}

	const TCHAR* CheckName(ECheck Check)
	{
		switch (Check)
		{
		case ECheck::Ok: return TEXT("Ok");
		case ECheck::NamedGun: return TEXT("NamedGun");
		case ECheck::WrongKind: return TEXT("WrongKind");
		case ECheck::NoSuchSlot: return TEXT("NoSuchSlot");
		case ECheck::NoSuchPart: return TEXT("NoSuchPart");
		case ECheck::RarityTooLow: return TEXT("RarityTooLow");
		case ECheck::NeedsOtherPart: return TEXT("NeedsOtherPart");
		case ECheck::BreaksOtherPart: return TEXT("BreaksOtherPart");
		case ECheck::SamePart: return TEXT("SamePart");
		default: return TEXT("?");
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPartSwapTest, "Looter.Weapons.PartSwap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponPartSwapTest::RunTest(const FString& Parameters)
{
	// The gunsmith's bench: a part from the box fits a rolled gun of its kind, in its slot, when the gun's rarity allows it
	// and the parts' needs hold both ways (the long tube on a long barrel); named guns are left alone. Fitting changes one
	// key and nothing else of the gun; the part lands where the gun's seed puts it, so moving it back and forth never
	// rerolls it; the name's word follows the parts' NamePriority; a longer barrel moves the muzzle. Guns already found keep
	// exactly the numbers they had.
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, SwapRiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, SwapShotgunPath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}

	// Guns already found: their parts roll as they always have, and with no notches or curse their stats are untouched.
	for (UWeaponDefinition* Definition : { Rifle, Shotgun })
	{
		for (int32 Seed = 0; Seed < 150; ++Seed)
		{
			for (const EWeaponRarity Rarity : SwapRarities)
			{
				const FWeaponLook Look = WeaponParts::Pick(*Definition, Seed, Rarity);
				if (!SameTotals(WeaponParts::CombinedStats(Look, Seed), DrawnAsEver(Look, Seed)))
				{
					AddError(FString::Printf(TEXT("%s seed %d: its parts roll differently than they always have"), *Definition->GetName(), Seed));
				}
				const FWeaponInstanceData Gun = MakeSwapGun(*Definition, Rarity, Seed);
				if (!SameSwapStats(Gun.Stats, UWeaponRollLibrary::ComputeStatsWithParts(Definition, Rarity, Gun.Level, Seed, Gun.Parts)))
				{
					AddError(FString::Printf(TEXT("%s seed %d: a plain gun's stats changed"), *Definition->GetName(), Seed));
				}
			}
		}
	}

	// Who can be changed: rolled guns; never a named gun, nor nothing.
	const FWeaponInstanceData Rolled = MakeSwapGun(*Shotgun, EWeaponRarity::Rare, 31, { TEXT("Barrel=Field") });
	TestTrue(TEXT("A rolled gun can be changed"), WeaponPartSwap::CanModify(Rolled));
	TestFalse(TEXT("No gun, nothing to change"), WeaponPartSwap::CanModify(FWeaponInstanceData()));
	UNamedWeaponDefinition* Keepsake = NewObject<UNamedWeaponDefinition>(CreatePackage(nullptr), TEXT("DA_Named_TestSwapKeepsake"), RF_Transient);
	Keepsake->Weapon = Shotgun;
	Keepsake->DisplayName = FText::FromString(TEXT("Keepsake"));
	FWeaponInstanceData NamedGun = Rolled;
	NamedGun.Named = Keepsake;
	NamedGun.Stats = UWeaponRollLibrary::ComputeInstanceStats(NamedGun);
	const FBoxedWeaponPart Trap = Boxed(Shotgun, TEXT("Barrel"), TEXT("Trap"));
	FBoxedWeaponPart Out;
	TestFalse(TEXT("A named gun can't be changed"), WeaponPartSwap::CanModify(NamedGun));
	TestEqual(TEXT("...nor take a part"), WeaponPartSwap::CanFit(NamedGun, Trap), ECheck::NamedGun);
	FWeaponInstanceData NamedTried = NamedGun;
	TestTrue(TEXT("...and fitting one changes nothing"), !WeaponPartSwap::Fit(NamedTried, Trap, Out) && NamedTried.Parts == NamedGun.Parts
		&& SameSwapStats(NamedTried.Stats, NamedGun.Stats) && Out.Key.IsNone());
	TestTrue(TEXT("...nor can it be scrapped for parts"), WeaponPartSwap::ScrapChoices(NamedGun).IsEmpty());
	TestTrue(TEXT("...and its preview is itself"), SameSwapStats(WeaponPartSwap::PreviewStats(NamedGun, Trap), NamedGun.Stats));

	// The shotgun's long tube needs a barrel of its length or more, as the parts list says.
	const FWeaponPartOption* Tube8 = WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Magazine"), TEXT("Tube8")));
	const FWeaponPartOption* Short = WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Barrel"), TEXT("Short")));
	const FWeaponPartOption* Ported = WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Barrel"), TEXT("Ported")));
	const FWeaponPartOption* Field = WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Barrel"), TEXT("Field")));
	const FWeaponPartOption* Defender = WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Barrel"), TEXT("Defender")));
	if (!TestTrue(TEXT("The shotgun's tubes and barrels as its parts list says"), Tube8 && Short && Ported && Field && Defender
		&& Tube8->Requires.Slot == FName(TEXT("Barrel")) && Short->Length < Tube8->Requires.MinLength && Ported->Length < Tube8->Requires.MinLength
		&& Field->Length >= Tube8->Requires.MinLength && Defender->Length >= Tube8->Requires.MinLength))
	{
		return false;
	}

	// The fitting rules, one at a time.
	const FWeaponInstanceData ShortBarrel = MakeSwapGun(*Shotgun, EWeaponRarity::Uncommon, 5, { TEXT("Barrel=Short"), TEXT("Magazine=Tube6") });
	const FWeaponInstanceData LongTube = MakeSwapGun(*Shotgun, EWeaponRarity::Uncommon, 6, { TEXT("Barrel=Field"), TEXT("Magazine=Tube8") });
	const FWeaponInstanceData Common = MakeSwapGun(*Shotgun, EWeaponRarity::Common, 7, { TEXT("Body=Standard") });
	const FWeaponInstanceData Uncommon = MakeSwapGun(*Shotgun, EWeaponRarity::Uncommon, 8, { TEXT("Body=Standard") });
	struct FFitCase
	{
		const TCHAR* What;
		const FWeaponInstanceData* Gun;
		FBoxedWeaponPart Part;
		ECheck Expected;
	};
	const FFitCase Cases[] = {
		{ TEXT("The long tube on a short barrel"), &ShortBarrel, Boxed(Shotgun, TEXT("Magazine"), TEXT("Tube8")), ECheck::NeedsOtherPart },
		{ TEXT("A long barrel in place of the short"), &ShortBarrel, Boxed(Shotgun, TEXT("Barrel"), TEXT("Field")), ECheck::Ok },
		{ TEXT("A short barrel under the long tube"), &LongTube, Boxed(Shotgun, TEXT("Barrel"), TEXT("Short")), ECheck::BreaksOtherPart },
		{ TEXT("A ported barrel under the long tube"), &LongTube, Boxed(Shotgun, TEXT("Barrel"), TEXT("Ported")), ECheck::BreaksOtherPart },
		{ TEXT("Another long barrel under the long tube"), &LongTube, Boxed(Shotgun, TEXT("Barrel"), TEXT("Defender")), ECheck::Ok },
		{ TEXT("The barrel it has"), &LongTube, Boxed(Shotgun, TEXT("Barrel"), TEXT("Field")), ECheck::SamePart },
		{ TEXT("An Epic body on a Common gun"), &Common, Boxed(Shotgun, TEXT("Body"), TEXT("Armored")), ECheck::RarityTooLow },
		{ TEXT("A Common body on a Common gun"), &Common, Boxed(Shotgun, TEXT("Body"), TEXT("Classic")), ECheck::Ok },
		{ TEXT("An Uncommon body on an Uncommon gun"), &Uncommon, Boxed(Shotgun, TEXT("Body"), TEXT("Heritage")), ECheck::Ok },
		{ TEXT("A Rare body on an Uncommon gun"), &Uncommon, Boxed(Shotgun, TEXT("Body"), TEXT("Skeleton")), ECheck::RarityTooLow },
		{ TEXT("A rifle's part on a shotgun"), &Common, Boxed(Rifle, TEXT("Body"), TEXT("Standard")), ECheck::WrongKind },
		{ TEXT("A slot it doesn't have"), &Common, Boxed(Shotgun, TEXT("Wing"), TEXT("Standard")), ECheck::NoSuchSlot },
		{ TEXT("A part that doesn't exist"), &Common, Boxed(Shotgun, TEXT("Body"), TEXT("Gold")), ECheck::NoSuchPart },
		{ TEXT("No part at all"), &Common, Boxed(Shotgun, TEXT("Body"), nullptr), ECheck::NoSuchPart } };
	for (const FFitCase& Case : Cases)
	{
		const ECheck Check = WeaponPartSwap::CanFit(*Case.Gun, Case.Part);
		TestTrue(FString::Printf(TEXT("%s: %s (%s)"), Case.What, CheckName(Case.Expected), CheckName(Check)), Check == Case.Expected);
		FWeaponInstanceData Tried = *Case.Gun;
		const bool bFitted = WeaponPartSwap::Fit(Tried, Case.Part, Out);
		TestEqual(FString::Printf(TEXT("%s: fitted only when it fits"), Case.What), bFitted, Check == ECheck::Ok);
		if (!bFitted)
		{
			TestTrue(FString::Printf(TEXT("%s: refused, the gun is as it was"), Case.What), Tried.Parts == Case.Gun->Parts
				&& SameSwapStats(Tried.Stats, Case.Gun->Stats) && Out.Key.IsNone());
		}
	}

	// A fitting: one key changes, the old part comes out, the rest of the gun (notches and curse too) stays, and its stats
	// are what the preview showed.
	FWeaponInstanceData Fitted = LongTube;
	Fitted.Kills = 137;
	Fitted.Curse = TEXT("Restless");
	Fitted.Stats = UWeaponRollLibrary::ComputeInstanceStats(Fitted);
	const FWeaponInstanceData Before = Fitted;
	const FBoxedWeaponPart DefenderPart = Boxed(Shotgun, TEXT("Barrel"), TEXT("Defender"));
	const FWeaponStats Preview = WeaponPartSwap::PreviewStats(Fitted, DefenderPart);
	TestTrue(TEXT("A preview leaves the gun as it is"), Fitted.Parts == Before.Parts && SameSwapStats(Fitted.Stats, Before.Stats));
	if (TestTrue(TEXT("The Defender barrel fitted"), WeaponPartSwap::Fit(Fitted, DefenderPart, Out)))
	{
		TestTrue(TEXT("The field barrel comes out for the box"), Out == Boxed(Shotgun, TEXT("Barrel"), TEXT("Field")));
		int32 Changed = 0;
		for (int32 Index = 0; Index < Fitted.Parts.Num() && Index < Before.Parts.Num(); ++Index)
		{
			Changed += Fitted.Parts[Index] != Before.Parts[Index] ? 1 : 0;
		}
		TestTrue(TEXT("One key changed: the barrel's"), Changed == 1 && Fitted.Parts.Num() == Before.Parts.Num() && KeyIn(Fitted, TEXT("Barrel")) == FName(TEXT("Defender")));
		TestTrue(TEXT("Its seed, rarity, level, notches and curse stay"), Fitted.Seed == Before.Seed && Fitted.Rarity == Before.Rarity
			&& Fitted.Level == Before.Level && Fitted.Kills == 137 && Fitted.Curse == FName(TEXT("Restless")) && !Fitted.bCurseLifted);
		TestTrue(TEXT("Its stats rebuilt with them"), SameSwapStats(Fitted.Stats, UWeaponRollLibrary::ComputeInstanceStats(Fitted)));
		TestTrue(TEXT("...as the preview showed"), SameSwapStats(Fitted.Stats, Preview));
	}

	// Back and forth: the part goes on and comes off, and the gun is exactly what it was, every time.
	FWeaponInstanceData Mine = Rolled;
	TArray<FName> TrapKeys = Rolled.Parts;
	TrapKeys[SwapSlot(*Shotgun, TEXT("Barrel"))] = TEXT("Trap");
	const FWeaponStats RolledWithTrap = UWeaponRollLibrary::ComputeStatsWithParts(Shotgun, Rolled.Rarity, Rolled.Level, Rolled.Seed, TrapKeys);
	for (int32 Round = 0; Round < 3; ++Round)
	{
		FBoxedWeaponPart TookOff;
		FBoxedWeaponPart Back;
		const bool bOn = WeaponPartSwap::Fit(Mine, Trap, TookOff);
		const FWeaponStats WithTrap = Mine.Stats;
		const bool bOff = WeaponPartSwap::Fit(Mine, TookOff, Back);
		TestTrue(FString::Printf(TEXT("Round %d: on and off again"), Round + 1), bOn && bOff && Back == Trap && TookOff == Boxed(Shotgun, TEXT("Barrel"), TEXT("Field")));
		TestTrue(FString::Printf(TEXT("Round %d: the very gun it was"), Round + 1), Mine.Parts == Rolled.Parts && SameSwapStats(Mine.Stats, Rolled.Stats));
		TestTrue(FString::Printf(TEXT("Round %d: the trap barrel lands where this gun's seed puts it"), Round + 1), SameSwapStats(WithTrap, RolledWithTrap));
	}

	// From one gun to another: on each it lands where that gun's seed puts it, as if the gun had rolled with it.
	FWeaponInstanceData Other = MakeSwapGun(*Shotgun, EWeaponRarity::Rare, 977, { TEXT("Barrel=Defender") });
	FWeaponInstanceData Moved = Rolled;
	FBoxedWeaponPart FromMoved;
	FBoxedWeaponPart FromOther;
	TestTrue(TEXT("The trap barrel onto one gun, then the next"), WeaponPartSwap::Fit(Moved, Trap, FromMoved) && WeaponPartSwap::Fit(Other, Trap, FromOther));
	TestTrue(TEXT("On the first, as if rolled with it"), SameSwapStats(Moved.Stats, UWeaponRollLibrary::ComputeStatsWithParts(Shotgun, Moved.Rarity, Moved.Level,
		Moved.Seed, Moved.Parts)));
	TestTrue(TEXT("On the next, as if rolled with it"), SameSwapStats(Other.Stats, UWeaponRollLibrary::ComputeStatsWithParts(Shotgun, Other.Rarity, Other.Level,
		Other.Seed, Other.Parts)));
	TestTrue(TEXT("The barrels they gave up go to the box"), FromMoved == Boxed(Shotgun, TEXT("Barrel"), TEXT("Field"))
		&& FromOther == Boxed(Shotgun, TEXT("Barrel"), TEXT("Defender")));

	// The name's word follows NamePriority: the trap barrel outranks the scout scope, which outranks the skeleton body.
	const FWeaponPartOption* TrapPart = WeaponPartSwap::FindOption(Trap);
	const FWeaponPartOption* Scout = WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Sight"), TEXT("Scout")));
	const FWeaponPartOption* Skeleton = WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Body"), TEXT("Skeleton")));
	if (TestTrue(TEXT("Three named parts in order of priority"), TrapPart && Scout && Skeleton && !TrapPart->NamePrefix.IsEmpty() && !Scout->NamePrefix.IsEmpty()
		&& !Skeleton->NamePrefix.IsEmpty() && TrapPart->NamePriority > Scout->NamePriority && Scout->NamePriority > Skeleton->NamePriority))
	{
		FWeaponInstanceData Renamed = MakeSwapGun(*Shotgun, EWeaponRarity::Rare, 44, { TEXT("Body=Standard"), TEXT("Barrel=Field"), TEXT("Muzzle=Crown"),
			TEXT("Magazine=Tube6"), TEXT("Sight=Flip"), TEXT("Stock=Skeleton"), TEXT("Pump=Walnut") });
		const FString Kind = Shotgun->DisplayName.ToString();
		auto WordIs = [this, &Renamed, &Kind](const TCHAR* Step, const FWeaponPartOption* Namer)
		{
			const FString Word = Namer ? Namer->NamePrefix.ToString() : FString();
			TestEqualSensitive(FString::Printf(TEXT("%s: its word"), Step), WeaponParts::NamePrefix(WeaponParts::Pick(Renamed)).ToString(), Word);
			TestEqualSensitive(FString::Printf(TEXT("%s: by the priority rule"), Step), ExpectedWord(Renamed), Word);
			TestEqualSensitive(FString::Printf(TEXT("%s: its name"), Step), LooterWeaponText::Name(Renamed), Word.IsEmpty() ? Kind : Word + TEXT(" ") + Kind);
		};
		WordIs(TEXT("Plain parts"), nullptr);
		TestTrue(TEXT("The trap barrel fitted"), WeaponPartSwap::Fit(Renamed, Trap, Out));
		WordIs(TEXT("With the trap barrel"), TrapPart);
		TestTrue(TEXT("The scout scope fitted"), WeaponPartSwap::Fit(Renamed, Boxed(Shotgun, TEXT("Sight"), TEXT("Scout")), Out));
		WordIs(TEXT("With the scout scope too"), TrapPart);
		TestTrue(TEXT("The skeleton body fitted"), WeaponPartSwap::Fit(Renamed, Boxed(Shotgun, TEXT("Body"), TEXT("Skeleton")), Out));
		WordIs(TEXT("With the skeleton body too"), TrapPart);
		TestTrue(TEXT("The field barrel back"), WeaponPartSwap::Fit(Renamed, Boxed(Shotgun, TEXT("Barrel"), TEXT("Field")), Out));
		WordIs(TEXT("Without the trap barrel"), Scout);
	}

	// Scrapping offers one part per filled slot, each the gun's own.
	const TArray<FBoxedWeaponPart> Choices = WeaponPartSwap::ScrapChoices(Rolled);
	TestEqual(TEXT("One choice per slot"), Choices.Num(), Shotgun->Parts.Num());
	for (int32 Index = 0; Index < Choices.Num() && Index < Shotgun->Parts.Num(); ++Index)
	{
		const FBoxedWeaponPart& Choice = Choices[Index];
		TestTrue(FString::Printf(TEXT("Choice %d: the gun's own part"), Index), Choice.Definition == Shotgun && Choice.Slot == Shotgun->Parts[Index].Name
			&& Choice.Key == Rolled.Parts[Index] && WeaponPartSwap::CanFit(Rolled, Choice) == ECheck::SamePart);
	}

	// Finding a part's option, and the bench's words for every answer.
	TestTrue(TEXT("A boxed part finds its option"), TrapPart && TrapPart->Key == FName(TEXT("Trap")));
	TestNull(TEXT("An unknown part finds none"), WeaponPartSwap::FindOption(Boxed(Shotgun, TEXT("Barrel"), TEXT("Gold"))));
	TestNull(TEXT("An empty box slot finds none"), WeaponPartSwap::FindOption(FBoxedWeaponPart()));
	TSet<FString> Words;
	for (const ECheck Check : { ECheck::Ok, ECheck::NamedGun, ECheck::WrongKind, ECheck::NoSuchSlot, ECheck::NoSuchPart, ECheck::RarityTooLow,
		ECheck::NeedsOtherPart, ECheck::BreaksOtherPart, ECheck::SamePart })
	{
		const FString Said = WeaponPartSwap::CheckText(Check).ToString();
		TestFalse(FString::Printf(TEXT("%s has its own words (%s)"), CheckName(Check), *Said), Said.IsEmpty() || Words.Contains(Said));
		Words.Add(Said);
	}

	// A new barrel moves the muzzle by its length: the assembled gun reads its muzzle from the barrel it's built with.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AActor* Holder = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
	UWeaponModelComponent* Model = NewObject<UWeaponModelComponent>(Holder);
	Holder->SetRootComponent(Model);
	Model->RegisterComponent();
	FWeaponInstanceData Muzzled = MakeSwapGun(*Shotgun, EWeaponRarity::Rare, 12, { TEXT("Barrel=Short"), TEXT("Magazine=Tube6") });
	if (TestTrue(TEXT("Built with the short barrel"), Model->Assemble(Muzzled)))
	{
		const double ShortMuzzle = Model->GetMuzzle().X;
		TestTrue(TEXT("The trap barrel fitted"), WeaponPartSwap::Fit(Muzzled, Trap, Out));
		TestTrue(TEXT("Built again with the trap barrel"), Model->Assemble(Muzzled));
		const double TrapMuzzle = Model->GetMuzzle().X;
		TestTrue(FString::Printf(TEXT("The longer barrel moves the muzzle forward (%.1f to %.1f cm)"), ShortMuzzle, TrapMuzzle), TrapMuzzle > ShortMuzzle + 5.0);
	}
	Model->Clear();

	Keepsake->MarkAsGarbage();
	return true;
}

#endif
