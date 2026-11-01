#pragma once

#include "CoreMinimal.h"

class UMissionRunner;
class UObject;
struct FCampaignRecord;

/**
 * The Ledger (Docs/Story.md, "The game's systems": "Bestiary: Sexton's Ledger"; Docs/Areas/RansomsRest.md, Main 2): the
 * bestiary's story pages open when Main 2 reaches its last step ("Open the Ledger"), and stay open for good. It lists the
 * bestiary's pages and the ones written in it only (UBestiaryEntry::bLedgerOnly: the story's characters, the seven names).
 * On screen the whole book is "The Warden's Log" since 2026-10-31 (the user's call, the Wardens story: no longer Sexton's
 * book or voice); the code keeps the Ledger's names. Read from the campaign record, so the book is the same on any map.
 */
namespace Ledger
{
	/** The mission whose deal hands it over (Main 2, "Shall We Talk Business?"), and its step that asks to open it (from 0). */
	inline const FName Mission(TEXT("Main2"));
	inline constexpr int32 HandedOverStep = 3;

	/** Sexton has handed it over: Main 2 is on its Ledger step or past it, or finished. */
	bool IsOpen(const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr);

	/** The same in WorldContextObject's level, from its mission runner's campaign record; not without one. */
	bool IsOpenIn(const UObject* WorldContextObject);

	/** The book's name, on the inventory's tab and in its key hints: "Warden's Log", open or not (bOpen kept for callers). */
	const TCHAR* BookName(bool bOpen);

	/** The book's own words, the log's, open or not (the open one has a blank for a name's whereabouts; the pages' are their own). */
	struct FWords
	{
		/** Over the list of entries: "The Warden's Log". */
		FString ListTitle;
		/** What an entry is called, one and many, in the list's count: "entry", "entries". */
		FString One;
		FString Many;
		/** Under an entry not met yet, in the list and on its page. */
		FString NotMet;
		/** Its page's help, under the blank numbers (a creature, an enemy). */
		FString NotMetHelp;
		/** A story character's page not met yet. */
		FString NotMetPersonHelp;
		/** The page with no entries at all: its header, and its help. */
		FString Empty;
		FString EmptyHelp;
		/** A Ledger name's whereabouts until the story finds them: a blank to fill in. */
		FString BlankWhereabouts;
	};

	const FWords& Words(bool bOpen);
}
