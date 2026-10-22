#include "Affixes/WeaponMakers.h"
#include "Affixes/WeaponMakerTuning.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/PrimitiveComponent.h"

#define LOCTEXT_NAMESPACE "WeaponMakers"

namespace
{
	/** One colour of a paint, from its sRGB hex (as the Blender scripts and setup_gun_parts.py write them). */
	FWeaponColorOption Hex(uint32 Value, float Weight = 1.f)
	{
		FWeaponColorOption Option;
		Option.Color = FLinearColor(FColor((Value >> 16) & 0xFF, (Value >> 8) & 0xFF, Value & 0xFF));
		Option.Weight = Weight;
		return Option;
	}

	FWeaponPaint Paint(const TCHAR* Slot, TArray<FWeaponColorOption> Colors)
	{
		FWeaponPaint Result;
		Result.Slot = FName(Slot);
		Result.Colors = MoveTemp(Colors);
		return Result;
	}

	/**
	 * The gun parts' material slots a maker repaints (Tools/Blender/looter_guns.py names them): the shell, the grey
	 * fittings (grips, pads, guards), the black and blued metal, and the brass trim. The rarity glow, the element glows,
	 * wood and lenses keep theirs.
	 */
	const TCHAR* const Shell = TEXT("GunPolymerSand");
	const TCHAR* const Fittings = TEXT("GunPolymerGrey");
	const TCHAR* const BlackMetal = TEXT("GunBlackMetal");
	const TCHAR* const BluedMetal = TEXT("GunSteelBlued");
	const TCHAR* const Trim = TEXT("GunBrass");

	FWeaponMaker Make(const TCHAR* Key, FText Name, FText FullName, FText Place, FText Fiction, FText QuirkName, TArray<FWeaponPaint> Paints,
		int32 MarkCell, uint32 MarkHex)
	{
		FWeaponMaker Maker;
		Maker.Key = FName(Key);
		Maker.Name = MoveTemp(Name);
		Maker.FullName = MoveTemp(FullName);
		Maker.Place = MoveTemp(Place);
		Maker.Fiction = MoveTemp(Fiction);
		Maker.QuirkName = MoveTemp(QuirkName);
		if (const WeaponMakerTuning::FShipped* Shipped = WeaponMakerTuning::ShippedFor(Maker.Key))
		{
			Maker.Quirk = Shipped->Quirk;
			Maker.Odds = Shipped->Odds;
			Maker.Kinds = Shipped->Kinds;
		}
		Maker.Paints = MoveTemp(Paints);
		Maker.MarkCell = MarkCell;
		Maker.MarkColor = Hex(MarkHex).Color;
		Maker.MarkIcon = FName(*FString::Printf(TEXT("Maker%s"), Key));
		return Maker;
	}

	// The concept the user saw: Saved/ArtPreviews/Weapons/Concepts/Makers.png (Docs/Work/2026-10-22-Next/MAKERS.md).
	// Keys are saved with every gun: never rename one. Numbers: WeaponMakerTuning.cpp.
	TArray<FWeaponMaker> MakeTable()
	{
		TArray<FWeaponMaker> Table;
		Table.Add(Make(TEXT("Roundhouse"), LOCTEXT("RoundhouseName", "Roundhouse"), LOCTEXT("RoundhouseFull", "Roundhouse Rail Works"),
			LOCTEXT("RoundhousePlace", "Halfway Junction"),
			LOCTEXT("RoundhouseFiction", "The railroad's own shops in the Junction roundhouse. Built to be thrown off a flatcar and still fire."),
			LOCTEXT("RoundhouseQuirk", "Rolling Stock"),
			{ Paint(Shell, { Hex(0x7d2f24, 3.f), Hex(0x8e3a2a, 2.f), Hex(0x6b2a22, 1.f) }),
				Paint(Fittings, { Hex(0x2c2d30, 3.f), Hex(0x38393b, 1.f) }),
				Paint(BlackMetal, { Hex(0x2a2b2e) }), Paint(BluedMetal, { Hex(0x2f3034) }), Paint(Trim, { Hex(0xd9a441) }) },
			1, 0xefe3c4));
		Table.Add(Make(TEXT("SaintMaud"), LOCTEXT("SaintMaudName", "Saint Maud"), LOCTEXT("SaintMaudFull", "Saint Maud's Foundry"),
			LOCTEXT("SaintMaudPlace", "Furnace Hollow"),
			LOCTEXT("SaintMaudFiction", "The smith-sisters of Saint Maud's mission cast every barrel at her forge and bless it. Slow to make, slow to fire."),
			LOCTEXT("SaintMaudQuirk", "Blessed Iron"),
			{ Paint(Shell, { Hex(0xe8e0cc, 3.f), Hex(0xefe8d6, 2.f), Hex(0xd9cfb6, 1.f) }),
				Paint(Fittings, { Hex(0x3a3633, 3.f), Hex(0x2f2c2a, 1.f) }),
				Paint(BlackMetal, { Hex(0x34302c) }), Paint(BluedMetal, { Hex(0x3a3530) }), Paint(Trim, { Hex(0xf6dfa0) }) },
			// Deep antique gold: the bright gilt (c9962f) vanished on the ivory shell at arm's length.
			2, 0x9a6a1c));
		Table.Add(Make(TEXT("Windfall"), LOCTEXT("WindfallName", "Windfall"), LOCTEXT("WindfallFull", "Windfall Salvage"),
			LOCTEXT("WindfallPlace", "Crossroads Town, Skyreach"),
			LOCTEXT("WindfallFiction", "Sky salvagers bolt guns together from whatever the Gravewind drops. Fast, loose and full of odd bits."),
			LOCTEXT("WindfallQuirk", "Loose Fit"),
			{ Paint(Shell, { Hex(0x4f8f8a, 2.f), Hex(0x9a5534, 2.f), Hex(0x7d9fb5, 1.5f), Hex(0xd8a832, 1.f), Hex(0x6f7550, 1.f) }),
				Paint(Fittings, { Hex(0x8a765a, 2.f), Hex(0x6a6d70, 2.f), Hex(0x56503f, 1.f) }),
				Paint(BlackMetal, { Hex(0x6c6e70), Hex(0x5a5348) }), Paint(BluedMetal, { Hex(0x75777a) }), Paint(Trim, { Hex(0xb86b3f) }) },
			3, 0xe6b43a));
		Table.Add(Make(TEXT("Meridian"), LOCTEXT("MeridianName", "Meridian"), LOCTEXT("MeridianFull", "Meridian Instrument Co."),
			LOCTEXT("MeridianPlace", "Highwater Landing"),
			LOCTEXT("MeridianFiction", "They made the surveyors' transits that laid every mile of track. Since the gang came: rifles that land where the glass says."),
			LOCTEXT("MeridianQuirk", "True Glass"),
			{ Paint(Shell, { Hex(0x26354a, 3.f), Hex(0x2b3c55, 2.f), Hex(0x1f2a38, 1.f) }),
				Paint(Fittings, { Hex(0x4a3a2e, 2.f), Hex(0x3b2f26, 1.f) }),
				Paint(BlackMetal, { Hex(0xa9aeb3) }), Paint(BluedMetal, { Hex(0x8f969c) }), Paint(Trim, { Hex(0xc99a4a) }) },
			4, 0xdfe6ea));
		return Table;
	}

	/** Picks among Options by weight at Roll (0-1); null when none weighs anything. */
	const FWeaponColorOption* PickColor(const TArray<FWeaponColorOption>& Options, float Roll)
	{
		float Total = 0.f;
		for (const FWeaponColorOption& Option : Options)
		{
			Total += FMath::Max(Option.Weight, 0.f);
		}
		if (Total <= 0.f)
		{
			return nullptr;
		}
		float Remaining = Roll * Total;
		const FWeaponColorOption* Last = nullptr;
		for (const FWeaponColorOption& Option : Options)
		{
			if (Option.Weight <= 0.f)
			{
				continue;
			}
			Last = &Option;
			Remaining -= Option.Weight;
			if (Remaining < 0.f)
			{
				return &Option;
			}
		}
		return Last;
	}

	// Streams of the gun's seed of their own (so a maker never moves the parts', stats', wear's or curse's draws).
	constexpr uint32 RollSalt = 0x4D414B52u;   // "MAKR"
	constexpr uint32 PaintSalt = 0x4D504E54u;  // "MPNT"
}

TConstArrayView<FWeaponMaker> WeaponMakers::All()
{
	static const TArray<FWeaponMaker> Table = MakeTable();
	return Table;
}

const FWeaponMaker* WeaponMakers::Find(FName Key)
{
	if (Key.IsNone())
	{
		return nullptr;
	}
	for (const FWeaponMaker& Maker : All())
	{
		if (Maker.Key == Key)
		{
			return &Maker;
		}
	}
	return nullptr;
}

FName WeaponMakers::DefaultKey()
{
	const FName Key = WeaponMakerTuning::DefaultKey();
	return Find(Key) ? Key : All()[0].Key;
}

const FWeaponMaker* WeaponMakers::Of(const FWeaponInstanceData& Gun)
{
	// A named gun is one gun, made as it is (Heirloom): no maker's numbers, paint, mark or name.
	return Gun.Named ? nullptr : Find(Gun.Maker);
}

float WeaponMakers::Weight(const FWeaponMaker& Maker, EWeaponKind Kind, EWeaponRarity Rarity)
{
	return WeaponMakerTuning::OddsAt(WeaponMakerTuning::Odds(Maker.Key), Rarity)
		* WeaponMakerTuning::LeanFor(WeaponMakerTuning::Kinds(Maker.Key), Kind);
}

FName WeaponMakers::Roll(EWeaponKind Kind, int32 Seed, EWeaponRarity Rarity)
{
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), RollSalt)));
	const float Pick = Random.FRand();
	float Total = 0.f;
	for (const FWeaponMaker& Maker : All())
	{
		Total += Weight(Maker, Kind, Rarity);
	}
	if (Total <= 0.f)
	{
		return DefaultKey();
	}
	float Remaining = Pick * Total;
	FName Last = DefaultKey();
	for (const FWeaponMaker& Maker : All())
	{
		const float MakerWeight = Weight(Maker, Kind, Rarity);
		if (MakerWeight <= 0.f)
		{
			continue;
		}
		Last = Maker.Key;
		Remaining -= MakerWeight;
		if (Remaining < 0.f)
		{
			return Maker.Key;
		}
	}
	return Last;
}

FName WeaponMakers::Roll(const UWeaponDefinition* Definition, int32 Seed, EWeaponRarity Rarity)
{
	return Roll(Definition ? Definition->Kind : EWeaponKind::None, Seed, Rarity);
}

FWeaponMakerQuirk WeaponMakers::QuirkOf(const FWeaponMaker& Maker)
{
	FWeaponMakerQuirk Quirk = WeaponMakerTuning::Quirk(Maker.Key);
	const float Scale = WeaponMakerTuning::QuirkScale();
	if (Scale != 1.f)
	{
		for (float* Percent : { &Quirk.Damage, &Quirk.Accuracy, &Quirk.Range, &Quirk.FireRate, &Quirk.Reload, &Quirk.Recoil, &Quirk.Handling,
			&Quirk.Magazine, &Quirk.Wear })
		{
			*Percent *= Scale;
		}
		Quirk.ElementPartBias = FMath::Lerp(1.f, Quirk.ElementPartBias, Scale);
		Quirk.EffectPartBias = FMath::Lerp(1.f, Quirk.EffectPartBias, Scale);
	}
	return Quirk;
}

bool WeaponMakers::ChangesMagazine(EWeaponKind Kind)
{
	// A revolver's cylinder is modeled with its chambers and a launcher's rocket shows in its mouth: their capacity is
	// the part's, as drawn.
	return Kind != EWeaponKind::Revolver && Kind != EWeaponKind::Launcher;
}

void WeaponMakers::ApplyQuirk(const FWeaponMakerQuirk& Quirk, EWeaponKind Kind, FWeaponStats& InOutStats)
{
	// Percent of the finished stat, after the parts and outside their caps, so a maker always shows. As the parts do, a
	// change never takes a stat below 5% of itself.
	auto Scale = [](float Percent) { return FMath::Max(1.f + Percent * 0.01f, 0.05f); };
	InOutStats.Damage *= Scale(Quirk.Damage);
	InOutStats.FireRate *= Scale(Quirk.FireRate);
	InOutStats.ReloadTime *= Scale(Quirk.Reload);
	// +30% accuracy shoots 1/1.3 as wide.
	InOutStats.Spread /= Scale(Quirk.Accuracy);
	InOutStats.Range *= Scale(Quirk.Range);
	InOutStats.Recoil *= Scale(Quirk.Recoil);
	InOutStats.Handling *= Scale(Quirk.Handling);
	if (Quirk.Magazine != 0.f && ChangesMagazine(Kind))
	{
		InOutStats.MagazineSize = FMath::Max(1, FMath::RoundToInt(InOutStats.MagazineSize * Scale(Quirk.Magazine)));
	}
}

void WeaponMakers::ApplyToStats(const FWeaponInstanceData& Gun, FWeaponStats& InOutStats)
{
	if (const FWeaponMaker* Maker = Of(Gun))
	{
		ApplyQuirk(QuirkOf(*Maker), Gun.Definition ? Gun.Definition->Kind : EWeaponKind::None, InOutStats);
	}
}

float WeaponMakers::PartWeight(const FWeaponMaker* Maker, const FWeaponPartOption& Option)
{
	if (!Maker || (Option.Element == ELooterElement::None && Option.Effect == EPartEffect::None))
	{
		return Option.Weight;
	}
	const FWeaponMakerQuirk Quirk = QuirkOf(*Maker);
	float Weight = Option.Weight;
	if (Option.Element != ELooterElement::None)
	{
		Weight *= Quirk.ElementPartBias;
	}
	if (Option.Effect != EPartEffect::None)
	{
		Weight *= Quirk.EffectPartBias;
	}
	return Weight;
}

void WeaponMakers::PaintLook(const FWeaponMaker& Maker, int32 Seed, FWeaponLook& InOutLook)
{
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), PaintSalt)));
	for (const FWeaponPaint& MakerPaint : Maker.Paints)
	{
		// One draw per paint whatever it holds, so a paint added later never moves the ones before it.
		const float Roll = Random.FRand();
		if (const FWeaponColorOption* Color = PickColor(MakerPaint.Colors, Roll))
		{
			InOutLook.Colors.Add(MakerPaint.Slot, Color->Color);
		}
	}
}

float WeaponMakers::ExtraWear(const FWeaponInstanceData& Gun)
{
	const FWeaponMaker* Maker = Of(Gun);
	return Maker ? QuirkOf(*Maker).Wear : 0.f;
}

FText WeaponMakers::NameWord(const FWeaponInstanceData& Gun)
{
	const FWeaponMaker* Maker = Of(Gun);
	return Maker ? Maker->Name : FText::GetEmpty();
}

FWeaponMarkPaint WeaponMakers::MarkFor(const FWeaponInstanceData& Gun, const FWeaponLook& Look)
{
	FWeaponMarkPaint Mark;
	const FWeaponMaker* Maker = Of(Gun);
	if (!Maker || Maker->MarkCell <= 0)
	{
		return Mark;
	}
	for (int32 Index = 0; Index < Look.Parts.Num(); ++Index)
	{
		const FWeaponPartOption* Part = Look.Parts[Index];
		if (Part && Part->MarkPlace.IsSet())
		{
			Mark.PartIndex = Index;
			Mark.Cell = static_cast<float>(Maker->MarkCell);
			Mark.Place = Part->MarkPlace;
			Mark.Color = Maker->MarkColor;
			break;
		}
	}
	return Mark;
}

void WeaponMakers::PaintMark(UPrimitiveComponent& Part, const FWeaponMarkPaint& Mark)
{
	if (!Mark.IsSet())
	{
		Part.SetCustomPrimitiveDataFloat(MarkDataIndex, 0.f);
		return;
	}
	const float Data[MarkDataCount] = { Mark.Cell, Mark.Place.X, Mark.Place.Z, Mark.Place.Size, Mark.Place.Depth,
		Mark.Color.R, Mark.Color.G, Mark.Color.B };
	for (int32 Offset = 0; Offset < MarkDataCount; ++Offset)
	{
		Part.SetCustomPrimitiveDataFloat(MarkDataIndex + Offset, Data[Offset]);
	}
}

void FWeaponInstanceData::PostSerialize(const FArchive& Ar)
{
	// A rolled gun from a save made before makers existed: it takes the default maker as it loads. Named guns have none;
	// a gun made by hand and never saved keeps None (no maker) in memory.
	if (Ar.IsLoading() && Maker.IsNone() && Definition != nullptr && Named == nullptr)
	{
		Maker = WeaponMakers::DefaultKey();
	}
}

#undef LOCTEXT_NAMESPACE
