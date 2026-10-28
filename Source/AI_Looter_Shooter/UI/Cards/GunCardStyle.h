#pragma once

#include "CoreMinimal.h"

struct FWeaponInstanceData;

/**
 * A gun card's maker look (the user's pick, 2026-10-28: "Wide Sweep", Docs/Work/2026-10-28-UICharacter): the sweep across
 * the card's top in the maker's colours, the trim along its foot, its deco, the mark and word in its ink, and the sound the
 * card opens with. Every maker's card has the same layout; this is what tells them apart. A gun without a maker (a named
 * gun, an old one) takes the plain gunmetal look.
 */
namespace GunCardStyle
{
	/** What a sweep wears besides its colours. */
	enum class EDeco : uint8
	{
		None,
		/** Roundhouse: brass rivets, two at each end. */
		Rivets,
		/** Saint Maud: a gilt diamond between two short rules. */
		Gilt,
		/** Windfall: a strip of tape and two bolt heads. */
		Patch,
		/** Meridian: a surveyor's ruler, ticks along the foot (every fifth long, in the trim). */
		Ticks
	};

	struct FStyle
	{
		/** The maker's key (FWeaponMaker::Key), none for the plain look. */
		FName Key;
		/** "ROUNDHOUSE": the word on the sweep (the rarity's word for a gun without a maker). */
		FText Word;
		/** "ROLLING STOCK": the quirk's name, right of the sweep on a card that compares nothing. */
		FText Quirk;
		/**
		 * The sweep's colours left to right: two blend (a gradient), three or more are plates side by side (Windfall's
		 * patchwork), at PlateStops.
		 */
		TArray<FLinearColor> Plates;
		/** Where each plate after the first starts (0-1 along the sweep); empty for a gradient. */
		TArray<float> PlateStops;
		/** The mark's and word's colour on the sweep. */
		FLinearColor Ink = FLinearColor::White;
		/** The 3 px line along the sweep's foot, and the deco's metal. */
		FLinearColor Trim = FLinearColor::White;
		EDeco Deco = EDeco::None;
		/** Its Inked mark (InkedIconData: "MakerRoundhouse"), none for the plain look. */
		FName MarkIcon;
		/** The maker's sound as its card opens (LooterSoundCue::Card), none for the plain look. */
		FName OpenCue;
	};

	/** The look of Gun's card: its maker's, or the plain one. */
	AI_LOOTER_SHOOTER_API const FStyle& For(const FWeaponInstanceData& Gun);

	/** A maker's look by its key; the plain look for an unknown key or none. */
	AI_LOOTER_SHOOTER_API const FStyle& ForMaker(FName MakerKey);

	/** The plain gunmetal look (no maker). */
	AI_LOOTER_SHOOTER_API const FStyle& Plain();
}
