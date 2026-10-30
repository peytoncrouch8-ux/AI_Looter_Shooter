// Looter.Menu.Key: a key pressed on the menu that's open, through Slate as a real key is (not shipped). The play-checks'
// injected Interact (play.py press, an Enhanced Input action) never reaches a menu: menus read their keys from Slate
// (NativeOnKeyDown), so the board and the choice cards looked deaf to E while a real key worked (2026-10-23, round 3).
// In the editor the console's own text box keeps the keyboard after a command, so this hands the menu the focus first.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/**
	 * The menus by class, the one over the others first (their viewport layers: the pause menu's settings and its list 40,
	 * one at a time; the offer and choice card 30, the world screens 26, the boards, the vendor and the bench 25, the
	 * inventory's pages 20).
	 */
	const TCHAR* const MenuOrder[] = {
		TEXT("SettingsMenuWidget"),
		TEXT("PauseMenuWidget"),
		TEXT("MissionOfferWidget"),
		TEXT("BountyBoardWidget"),
		TEXT("LostLootWidget"),
		TEXT("BossShrineWidget"),
		TEXT("NoticeBoardWidget"),
		TEXT("StationBoardWidget"),
		TEXT("VendorWidget"),
		TEXT("BenchWidget"),
		TEXT("LoadoutWidget"),
		TEXT("EmbersWidget"),
		TEXT("BestiaryWidget"),
		TEXT("MissionsWidget"),
		TEXT("MapWidget"),
		TEXT("HeroSelectWidget"),
	};

	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/** The open menu on top in World: the first of MenuOrder in the viewport and shown, else any focusable widget there. */
	UUserWidget* FindTopMenu(const UWorld* World)
	{
		TArray<UUserWidget*> Open;
		for (TObjectIterator<UUserWidget> It; It; ++It)
		{
			UUserWidget* Widget = *It;
			if (Widget && Widget->GetWorld() == World && Widget->IsInViewport() && Widget->IsVisible())
			{
				Open.Add(Widget);
			}
		}
		for (const TCHAR* Name : MenuOrder)
		{
			if (UUserWidget* const* Found = Open.FindByPredicate([Name](const UUserWidget* Each) { return Each->GetClass()->GetName() == Name; }))
			{
				return *Found;
			}
		}
		UUserWidget* const* Focusable = Open.FindByPredicate([](const UUserWidget* Each) { return Each->IsFocusable(); });
		return Focusable ? *Focusable : nullptr;
	}

	/** The key down (or up) to Menu, with the keyboard handed to it first. */
	void SendKey(UUserWidget& Menu, const FKey& Key, bool bDown)
	{
		FSlateApplication& Slate = FSlateApplication::Get();
		Menu.SetKeyboardFocus();
		const FKeyEvent Event(Key, Slate.GetModifierKeys(), /*UserIndex*/ 0, /*bIsRepeat*/ false, /*CharacterCode*/ 0, /*KeyCode*/ 0);
		if (bDown)
		{
			Slate.ProcessKeyDownEvent(Event);
		}
		else
		{
			Slate.ProcessKeyUpEvent(Event);
		}
	}

	void PressMenuKey(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		if (Args.IsEmpty() || !GameWorld || !FSlateApplication::IsInitialized())
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Menu.Key <key: E, Enter, Escape, W, S...> [seconds held]: in a running game."));
			return;
		}
		const FKey Key(*Args[0]);
		if (!Key.IsValid())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Menu.Key: no key called %s."), *Args[0]);
			return;
		}
		UUserWidget* Menu = FindTopMenu(GameWorld);
		if (!Menu)
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Menu.Key %s: no menu is open."), *Key.ToString());
			return;
		}
		const float Held = Args.Num() > 1 ? FMath::Max(FCString::Atof(*Args[1]), 0.f) : 0.f;
		UE_LOG(LogLooter, Display, TEXT("Looter.Menu.Key %s: to %s%s."), *Key.ToString(), *Menu->GetClass()->GetName(),
			Held > 0.f ? *FString::Printf(TEXT(", held %.1f s"), Held) : TEXT(""));
		SendKey(*Menu, Key, /*bDown*/ true);
		if (Held <= 0.f)
		{
			SendKey(*Menu, Key, /*bDown*/ false);
			return;
		}
		// Held (a choice card's ring): let go after the time, to the same menu if it's still up.
		TWeakObjectPtr<UUserWidget> Target = Menu;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Target, Key](float) {
			if (UUserWidget* Still = Target.Get(); Still && Still->IsInViewport())
			{
				SendKey(*Still, Key, /*bDown*/ false);
			}
			return false;
		}), Held);
	}

	FAutoConsoleCommandWithWorldAndArgs MenuKeyCommand(
		TEXT("Looter.Menu.Key"),
		TEXT("Looter.Menu.Key <key> [seconds held]: presses a key on the open menu on top (the board, an offer or choice card, a page) "
			 "through Slate, as a real key; E takes, Enter accepts, Escape closes. A choice card wants E held: Looter.Menu.Key E 1.2."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PressMenuKey));
}

#endif
