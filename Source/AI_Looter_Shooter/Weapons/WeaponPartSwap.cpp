#include "Weapons/WeaponPartSwap.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"

bool FBoxedWeaponPart::operator==(const FBoxedWeaponPart& Other) const
{
	return Definition == Other.Definition && Slot == Other.Slot && Key == Other.Key;
}

namespace
{
	int32 FindSlot(const UWeaponDefinition& Definition, FName Slot)
	{
		return Definition.Parts.IndexOfByPredicate([Slot](const FWeaponPartSlot& Each) { return Each.Name == Slot; });
	}

	/** The slot's option by key, only when it has its model: a part without one is never built (Pick would put another in its place). */
	const FWeaponPartOption* FindModeled(const FWeaponPartSlot& Slot, FName Key)
	{
		return Key.IsNone() ? nullptr : Slot.Options.FindByPredicate([Key](const FWeaponPartOption& Option) { return Option.Mesh && Option.Key == Key; });
	}

	/**
	 * The keys the gun is built from now, one per slot: its saved parts, or the seed's picks for a gun saved before guns
	 * kept their parts (and the seed's pick in place of a saved part that's gone), so a fitting keeps what the gun shows.
	 */
	TArray<FName> CurrentKeys(const FWeaponInstanceData& Gun)
	{
		return WeaponParts::PartKeys(WeaponParts::Pick(Gun));
	}
}

bool WeaponPartSwap::CanModify(const FWeaponInstanceData& Gun)
{
	// A named gun (Heirloom, and the signature legendaries, which are named guns too) is one gun with fixed parts.
	return Gun.Definition && !Gun.Named && !Gun.Definition->Parts.IsEmpty();
}

WeaponPartSwap::ECheck WeaponPartSwap::CanFit(const FWeaponInstanceData& Gun, const FBoxedWeaponPart& Part)
{
	if (Gun.Named)
	{
		return ECheck::NamedGun;
	}
	// The same kind of gun: a part only knows its own kind's slots and sockets.
	const UWeaponDefinition* Definition = Gun.Definition;
	if (!Definition || Part.Definition != Definition)
	{
		return ECheck::WrongKind;
	}
	const int32 Index = FindSlot(*Definition, Part.Slot);
	if (Index == INDEX_NONE)
	{
		return ECheck::NoSuchSlot;
	}
	const FWeaponPartOption* Option = FindModeled(Definition->Parts[Index], Part.Key);
	if (!Option)
	{
		return ECheck::NoSuchPart;
	}
	const FWeaponLook Look = WeaponParts::Pick(Gun);
	if (!Look.Parts.IsValidIndex(Index))
	{
		return ECheck::NoSuchSlot;
	}
	if (Look.Parts[Index] && Look.Parts[Index]->Key == Option->Key)
	{
		return ECheck::SamePart;
	}
	// Rarity unlocks the better parts at the bench as it does on the guns that drop.
	if (Option->MinRarity > Gun.Rarity)
	{
		return ECheck::RarityTooLow;
	}
	// Its own need: the part in the earlier slot it depends on is long enough (the 8-shell tube on a barrel of 46 cm or more).
	if (!Option->Requires.Slot.IsNone())
	{
		const int32 Needed = FindSlot(*Definition, Option->Requires.Slot);
		if (Needed == INDEX_NONE || Needed >= Index || !Look.Parts[Needed] || Look.Parts[Needed]->Length < Option->Requires.MinLength)
		{
			return ECheck::NeedsOtherPart;
		}
	}
	// The others' needs: a part that depends on this slot still has what it needs once the new part is in.
	for (int32 Other = 0; Other < Look.Parts.Num(); ++Other)
	{
		const FWeaponPartOption* Dependent = Look.Parts[Other];
		if (Other != Index && Dependent && Dependent->Requires.Slot == Part.Slot && Option->Length < Dependent->Requires.MinLength)
		{
			return ECheck::BreaksOtherPart;
		}
	}
	return ECheck::Ok;
}

bool WeaponPartSwap::Fit(FWeaponInstanceData& Gun, const FBoxedWeaponPart& Part, FBoxedWeaponPart& OutReplaced)
{
	OutReplaced = FBoxedWeaponPart();
	if (CanFit(Gun, Part) != ECheck::Ok)
	{
		return false;
	}
	const int32 Index = FindSlot(*Gun.Definition, Part.Slot);
	TArray<FName> Keys = CurrentKeys(Gun);
	OutReplaced.Definition = Gun.Definition;
	OutReplaced.Slot = Part.Slot;
	OutReplaced.Key = Keys[Index];

	// One key changes; the seed, rarity, level, paint, kills and curse stay, so the gun is still itself with a new part.
	Keys[Index] = Part.Key;
	Gun.Parts = MoveTemp(Keys);
	Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
	return true;
}

TArray<FBoxedWeaponPart> WeaponPartSwap::ScrapChoices(const FWeaponInstanceData& Gun)
{
	TArray<FBoxedWeaponPart> Choices;
	if (!CanModify(Gun))
	{
		return Choices;
	}
	const FWeaponLook Look = WeaponParts::Pick(Gun);
	for (int32 Index = 0; Index < Look.Parts.Num() && Index < Gun.Definition->Parts.Num(); ++Index)
	{
		if (const FWeaponPartOption* Option = Look.Parts[Index])
		{
			FBoxedWeaponPart& Choice = Choices.AddDefaulted_GetRef();
			Choice.Definition = Gun.Definition;
			Choice.Slot = Gun.Definition->Parts[Index].Name;
			Choice.Key = Option->Key;
		}
	}
	return Choices;
}

FWeaponStats WeaponPartSwap::PreviewStats(const FWeaponInstanceData& Gun, const FBoxedWeaponPart& Part)
{
	// Any part of the gun's kind for its slot, fitting or not, so the bench can show what a part would do before the gun
	// can take it. A part of another kind, or a named gun, shows the gun as it is.
	FWeaponInstanceData Preview = Gun;
	const UWeaponDefinition* Definition = Gun.Definition;
	const int32 Index = Definition && !Gun.Named && Part.Definition == Definition ? FindSlot(*Definition, Part.Slot) : INDEX_NONE;
	if (Index != INDEX_NONE && FindModeled(Definition->Parts[Index], Part.Key))
	{
		Preview.Parts = CurrentKeys(Gun);
		if (Preview.Parts.IsValidIndex(Index))
		{
			Preview.Parts[Index] = Part.Key;
		}
	}
	return UWeaponRollLibrary::ComputeInstanceStats(Preview);
}

const FWeaponPartOption* WeaponPartSwap::FindOption(const FBoxedWeaponPart& Part)
{
	const UWeaponDefinition* Definition = Part.Definition;
	const int32 Index = Definition ? FindSlot(*Definition, Part.Slot) : INDEX_NONE;
	if (Index == INDEX_NONE || Part.Key.IsNone())
	{
		return nullptr;
	}
	return Definition->Parts[Index].Options.FindByPredicate([&Part](const FWeaponPartOption& Option) { return Option.Key == Part.Key; });
}

FText WeaponPartSwap::CheckText(ECheck Check)
{
	// Short enough for a part's row on the bench. The only needs in the parts lists are a long tube's on a shotgun's barrel,
	// so the two needs read as barrels.
	switch (Check)
	{
	case ECheck::Ok:
		return FText::FromString(TEXT("FITS"));
	case ECheck::NamedGun:
		return FText::FromString(TEXT("A NAMED GUN'S PARTS ARE ITS OWN"));
	case ECheck::WrongKind:
		return FText::FromString(TEXT("FITS ANOTHER KIND OF GUN"));
	case ECheck::NoSuchSlot:
		return FText::FromString(TEXT("NO PLACE FOR IT ON THIS GUN"));
	case ECheck::NoSuchPart:
		return FText::FromString(TEXT("NO LONGER MADE"));
	case ECheck::RarityTooLow:
		return FText::FromString(TEXT("NEEDS A RARER GUN"));
	case ECheck::NeedsOtherPart:
		return FText::FromString(TEXT("NEEDS A LONGER BARREL"));
	case ECheck::BreaksOtherPart:
		return FText::FromString(TEXT("TOO SHORT FOR THE MAGAZINE"));
	case ECheck::SamePart:
		return FText::FromString(TEXT("ALREADY FITTED"));
	default:
		return FText::GetEmpty();
	}
}
