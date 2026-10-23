#include "Missions/MissionText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "UI/Inventory/LoadoutParts.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
	/** The local player's key bindings, or null outside a game. */
	const UKeyBindingSubsystem* FindKeyBindings(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
		return Player ? Player->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	}

	/** One binding's key by its short name ("E"), or the binding's own name when there's no key to ask about. */
	FString BoundKeyName(const UKeyBindingSubsystem* Keys, const FString& Id)
	{
		const FKey Key = Keys ? Keys->GetKey(FName(*Id)) : FKey();
		return Key.IsValid() ? Key.GetDisplayName(/*bLongDisplayName*/ false).ToString() : Id;
	}

	/** Where a page of the inventory is named instead of an action: {Page.Bestiary}. */
	const TCHAR* const PagePrefix = TEXT("Page.");

	/**
	 * An action's key, or for Move the four movement keys, space apart ("W A S D"), or for Page.<name> the number key that
	 * turns to that inventory page ("3" for the bestiary), read from the tabs' own table (LoadoutParts::PageNumberKey) so
	 * a step's words follow the tabs' order (Main 2's said "then 2" after the Embers tab came second: 2026-10-23).
	 */
	FString ActionKeyName(const UKeyBindingSubsystem* Keys, const FString& Id)
	{
		if (Id.StartsWith(PagePrefix, ESearchCase::CaseSensitive))
		{
			const FKey PageKey = LoadoutParts::PageNumberKey(LoadoutParts::PageNamed(Id.RightChop(FCString::Strlen(PagePrefix))));
			return PageKey.IsValid() ? PageKey.GetDisplayName(/*bLongDisplayName*/ false).ToString() : Id;
		}
		if (Id == TEXT("Move"))
		{
			return FString::Printf(TEXT("%s %s %s %s"), *BoundKeyName(Keys, TEXT("MoveForward")), *BoundKeyName(Keys, TEXT("MoveLeft")),
				*BoundKeyName(Keys, TEXT("MoveBackward")), *BoundKeyName(Keys, TEXT("MoveRight")));
		}
		return BoundKeyName(Keys, Id);
	}
}

FString MissionText::ResolveKeys(const UWorld* World, const FString& Text)
{
	const UKeyBindingSubsystem* Keys = FindKeyBindings(World);
	FString Result;
	int32 Index = 0;
	while (Index < Text.Len())
	{
		const int32 Open = Text.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index);
		const int32 Close = Open == INDEX_NONE ? INDEX_NONE : Text.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open);
		if (Close == INDEX_NONE)
		{
			Result += Text.Mid(Index);
			break;
		}
		Result += Text.Mid(Index, Open - Index);
		Result += FString::Printf(TEXT("[%s]"), *ActionKeyName(Keys, Text.Mid(Open + 1, Close - Open - 1)));
		Index = Close + 1;
	}
	return Result;
}

FString MissionText::KeyName(const UWorld* World, FName Action)
{
	return Action.IsNone() ? FString() : ActionKeyName(FindKeyBindings(World), Action.ToString());
}

FString MissionText::InSentence(const FString& Name)
{
	// Only the article as a word of its own ("Theodore's watch" stays as it is), and only with words after it.
	static const TCHAR* const Articles[] = { TEXT("The "), TEXT("An "), TEXT("A ") };
	for (const TCHAR* Article : Articles)
	{
		if (Name.StartsWith(Article, ESearchCase::CaseSensitive) && Name.Len() > FCString::Strlen(Article))
		{
			FString Lowered = Name;
			Lowered[0] = FChar::ToLower(Lowered[0]);
			return Lowered;
		}
	}
	return Name;
}

FText MissionText::InSentence(const FText& Name)
{
	const FString Words = Name.ToString();
	const FString Lowered = InSentence(Words);
	// Unchanged, the name keeps its own text (and its translation).
	return Lowered.Equals(Words, ESearchCase::CaseSensitive) ? Name : FText::FromString(Lowered);
}
