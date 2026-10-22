#include "Affixes/WeaponMakerTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Makers.* cvars. Each maker's are made from its shipped numbers when the module loads, so they exist in the
// console from the start and a key's cvars always match the table.

namespace
{
	FWeaponMakerQuirk MakeQuirk(float Damage, float Accuracy, float Range, float FireRate, float Reload, float Recoil, float Handling,
		float Magazine, float Wear = 0.f, float ElementPartBias = 1.f, float EffectPartBias = 1.f)
	{
		FWeaponMakerQuirk Quirk;
		Quirk.Damage = Damage;
		Quirk.Accuracy = Accuracy;
		Quirk.Range = Range;
		Quirk.FireRate = FireRate;
		Quirk.Reload = Reload;
		Quirk.Recoil = Recoil;
		Quirk.Handling = Handling;
		Quirk.Magazine = Magazine;
		Quirk.Wear = Wear;
		Quirk.ElementPartBias = ElementPartBias;
		Quirk.EffectPartBias = EffectPartBias;
		return Quirk;
	}

	// The concept the user saw (Saved/ArtPreviews/Weapons/Concepts/Makers.png). The common stock (Roundhouse, Windfall)
	// leads at Common and Uncommon, the fine work (Saint Maud, Meridian) at Rare and up: shares at Common 44/33/11/11%,
	// at Epic and Legendary 21/16/32/32%, before the kind's lean.
	const TArray<WeaponMakerTuning::FShipped>& Table()
	{
		static const TArray<WeaponMakerTuning::FShipped> Shipped = {
			// Rolling Stock: built heavy, holds more and kicks less, slow to swing and to reload.
			{ TEXT("Roundhouse"), MakeQuirk(0.f, 0.f, 0.f, 0.f, 15.f, -25.f, -15.f, 30.f),
				TEXT("4,3,2,2,2"), TEXT("LMG=1.5,Launcher=1.5,Rifle=1.2,Revolver=0.6") },
			// Blessed Iron: hits harder, fires and reloads slower, and its rounds are blessed more often (element parts).
			{ TEXT("SaintMaud"), MakeQuirk(10.f, 0.f, 0.f, -12.f, 10.f, 0.f, 0.f, 0.f, 0.f, 3.f, 1.f),
				TEXT("1,2,3,3,3"), TEXT("Shotgun=1.5,Revolver=1.5,Sniper=1.2,SMG=0.6,Launcher=0.8") },
			// Loose Fit: fast and quick to reload, wild and a little weak, full of odd bits (effect parts), always worn.
			{ TEXT("Windfall"), MakeQuirk(-5.f, -20.f, 0.f, 15.f, -20.f, 0.f, 0.f, 0.f, 0.25f, 1.f, 2.f),
				TEXT("3,3,2,1.5,1.5"), TEXT("SMG=1.8,Shotgun=1.3,Sniper=0.5") },
			// True Glass: lands where the glass says and reaches farther, comes up quick; slower, smaller magazines.
			{ TEXT("Meridian"), MakeQuirk(0.f, 30.f, 20.f, -10.f, 0.f, 0.f, 10.f, -20.f),
				TEXT("1,2,2.5,3,3"), TEXT("Sniper=2,Revolver=1.5,Rifle=1.2,Shotgun=0.7,SMG=0.6,LMG=0.4,Launcher=0.4") },
		};
		return Shipped;
	}

	struct FField
	{
		const TCHAR* Name;
		float FWeaponMakerQuirk::* Member;
		const TCHAR* Help;
	};

	const FField Fields[] = {
		{ TEXT("Damage"), &FWeaponMakerQuirk::Damage, TEXT("Damage, percent.") },
		{ TEXT("Accuracy"), &FWeaponMakerQuirk::Accuracy, TEXT("Accuracy, percent (+30 shoots 1/1.3 as wide).") },
		{ TEXT("Range"), &FWeaponMakerQuirk::Range, TEXT("Full-damage range, percent.") },
		{ TEXT("FireRate"), &FWeaponMakerQuirk::FireRate, TEXT("Fire rate, percent.") },
		{ TEXT("Reload"), &FWeaponMakerQuirk::Reload, TEXT("Reload time, percent (+ is slower).") },
		{ TEXT("Recoil"), &FWeaponMakerQuirk::Recoil, TEXT("Kick, percent (- kicks less).") },
		{ TEXT("Handling"), &FWeaponMakerQuirk::Handling, TEXT("Aim and swap speed, percent.") },
		{ TEXT("Magazine"), &FWeaponMakerQuirk::Magazine, TEXT("Magazine size, percent (never a revolver's or a launcher's).") },
		{ TEXT("Wear"), &FWeaponMakerQuirk::Wear, TEXT("Added to the gun's wear (0 fresh to 1 battered).") },
		{ TEXT("ElementPartBias"), &FWeaponMakerQuirk::ElementPartBias, TEXT("An element part's chance times this.") },
		{ TEXT("EffectPartBias"), &FWeaponMakerQuirk::EffectPartBias, TEXT("An effect part's chance times this.") },
	};

	/** One maker's cvars, made from its shipped numbers. */
	struct FMakerVars
	{
		FName Key;
		TArray<IConsoleVariable*> Fields;
		IConsoleVariable* Odds = nullptr;
		IConsoleVariable* Kinds = nullptr;
	};

	struct FMakerCVars
	{
		TArray<FMakerVars> Makers;
		TArray<FString> Names;

		FMakerCVars()
		{
			IConsoleManager& Console = IConsoleManager::Get();
			for (const WeaponMakerTuning::FShipped& Shipped : Table())
			{
				FMakerVars& Made = Makers.AddDefaulted_GetRef();
				Made.Key = FName(Shipped.Key);
				for (const FField& Field : Fields)
				{
					const FString& Name = Names.Add_GetRef(FString::Printf(TEXT("Looter.Makers.%s.%s"), Shipped.Key, Field.Name));
					Made.Fields.Add(Console.RegisterConsoleVariable(*Name, Shipped.Quirk.*Field.Member,
						*FString::Printf(TEXT("%s's quirk: %s"), Shipped.Key, Field.Help), ECVF_Default));
				}
				const FString& OddsName = Names.Add_GetRef(FString::Printf(TEXT("Looter.Makers.%s.Odds"), Shipped.Key));
				Made.Odds = Console.RegisterConsoleVariable(*OddsName, FString(Shipped.Odds),
					*FString::Printf(TEXT("%s's weight at each rarity, Common to Legendary (\"4,3,2,2,2\")."), Shipped.Key), ECVF_Default);
				const FString& KindsName = Names.Add_GetRef(FString::Printf(TEXT("Looter.Makers.%s.Kinds"), Shipped.Key));
				Made.Kinds = Console.RegisterConsoleVariable(*KindsName, FString(Shipped.Kinds),
					*FString::Printf(TEXT("%s's lean to each kind (\"LMG=1.5,Launcher=0.4\"; 1 unless named)."), Shipped.Key), ECVF_Default);
			}
		}

		~FMakerCVars()
		{
			// As TAutoConsoleVariable does: a reloaded module registers them afresh.
			IConsoleManager& Console = IConsoleManager::Get();
			for (const FString& Name : Names)
			{
				Console.UnregisterConsoleObject(*Name, false);
			}
		}

		const FMakerVars* Find(FName Key) const
		{
			return Makers.FindByPredicate([Key](const FMakerVars& Entry) { return Entry.Key == Key; });
		}
	};

	const FMakerCVars& Vars()
	{
		static FMakerCVars Made;
		return Made;
	}

	// Made when the module loads (a static's constructor), so the cvars are in the console before any gun is.
	struct FRegisterAtLoad
	{
		FRegisterAtLoad() { Vars(); }
	} RegisterAtLoad;

	TAutoConsoleVariable<float> CVarQuirkScale(TEXT("Looter.Makers.QuirkScale"), 1.f,
		TEXT("Every gun maker's handling trade times this (0: off, for comparisons)."), ECVF_Default);
	TAutoConsoleVariable<FString> CVarDefault(TEXT("Looter.Makers.Default"), TEXT("Roundhouse"),
		TEXT("The maker a gun saved before makers existed takes when it loads."), ECVF_Default);
}

TConstArrayView<WeaponMakerTuning::FShipped> WeaponMakerTuning::Shipped()
{
	return Table();
}

const WeaponMakerTuning::FShipped* WeaponMakerTuning::ShippedFor(FName Key)
{
	return Table().FindByPredicate([Key](const FShipped& Entry) { return Key == FName(Entry.Key); });
}

float WeaponMakerTuning::QuirkScale()
{
	return FMath::Max(CVarQuirkScale->GetFloat(), 0.f);
}

FName WeaponMakerTuning::DefaultKey()
{
	return FName(*CVarDefault->GetString().TrimStartAndEnd());
}

FWeaponMakerQuirk WeaponMakerTuning::Quirk(FName Key)
{
	FWeaponMakerQuirk Live;
	const FMakerVars* Maker = Vars().Find(Key);
	if (!Maker)
	{
		const FShipped* Entry = ShippedFor(Key);
		return Entry ? Entry->Quirk : Live;
	}
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Fields)) && Index < Maker->Fields.Num(); ++Index)
	{
		if (Maker->Fields[Index])
		{
			Live.*Fields[Index].Member = Maker->Fields[Index]->GetFloat();
		}
	}
	// Leanings can't go below nothing.
	Live.ElementPartBias = FMath::Max(Live.ElementPartBias, 0.f);
	Live.EffectPartBias = FMath::Max(Live.EffectPartBias, 0.f);
	return Live;
}

FString WeaponMakerTuning::Odds(FName Key)
{
	const FMakerVars* Maker = Vars().Find(Key);
	if (Maker && Maker->Odds)
	{
		return Maker->Odds->GetString();
	}
	const FShipped* Entry = ShippedFor(Key);
	return Entry ? FString(Entry->Odds) : FString();
}

FString WeaponMakerTuning::Kinds(FName Key)
{
	const FMakerVars* Maker = Vars().Find(Key);
	if (Maker && Maker->Kinds)
	{
		return Maker->Kinds->GetString();
	}
	const FShipped* Entry = ShippedFor(Key);
	return Entry ? FString(Entry->Kinds) : FString();
}

float WeaponMakerTuning::OddsAt(const FString& Odds, EWeaponRarity Rarity)
{
	TArray<FString> Entries;
	Odds.ParseIntoArray(Entries, TEXT(","), /*InCullEmpty*/ false);
	const int32 Index = static_cast<int32>(Rarity);
	if (!Entries.IsValidIndex(Index))
	{
		return 0.f;
	}
	const FString Entry = Entries[Index].TrimStartAndEnd();
	return Entry.IsNumeric() ? FMath::Max(FCString::Atof(*Entry), 0.f) : 0.f;
}

float WeaponMakerTuning::LeanFor(const FString& Kinds, EWeaponKind Kind)
{
	const UEnum* KindEnum = StaticEnum<EWeaponKind>();
	const FString Name = KindEnum ? KindEnum->GetNameStringByValue(static_cast<int64>(Kind)) : FString();
	TArray<FString> Entries;
	Kinds.ParseIntoArray(Entries, TEXT(","));
	for (const FString& Entry : Entries)
	{
		FString Left, Right;
		if (Entry.Split(TEXT("="), &Left, &Right) && Left.TrimStartAndEnd().Equals(Name, ESearchCase::IgnoreCase))
		{
			const FString Value = Right.TrimStartAndEnd();
			return Value.IsNumeric() ? FMath::Max(FCString::Atof(*Value), 0.f) : 1.f;
		}
	}
	return 1.f;
}
