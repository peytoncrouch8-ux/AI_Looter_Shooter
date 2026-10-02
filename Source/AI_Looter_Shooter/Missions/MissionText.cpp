#include "Missions/MissionText.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

FString MissionText::ResolveKeys(const UWorld* World, const FString& Text)
{
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const ULocalPlayer* Player = Controller ? Controller->GetLocalPlayer() : nullptr;
	const UKeyBindingSubsystem* Keys = Player ? Player->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	auto KeyName = [Keys](const TCHAR* Id)
	{
		const FKey Key = Keys ? Keys->GetKey(Id) : FKey();
		return Key.IsValid() ? Key.GetDisplayName(/*bLongDisplayName*/ false).ToString() : FString(Id);
	};

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
		const FString Id = Text.Mid(Open + 1, Close - Open - 1);
		if (Id == TEXT("Move"))
		{
			Result += FString::Printf(TEXT("[%s %s %s %s]"), *KeyName(TEXT("MoveForward")), *KeyName(TEXT("MoveLeft")),
				*KeyName(TEXT("MoveBackward")), *KeyName(TEXT("MoveRight")));
		}
		else
		{
			Result += FString::Printf(TEXT("[%s]"), *KeyName(*Id));
		}
		Index = Close + 1;
	}
	return Result;
}
