#include "Missions/MissionText.h"
#include "Settings/KeyBindingSubsystem.h"
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

	/** An action's key, or for Move the four movement keys, space apart ("W A S D"). */
	FString ActionKeyName(const UKeyBindingSubsystem* Keys, const FString& Id)
	{
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
