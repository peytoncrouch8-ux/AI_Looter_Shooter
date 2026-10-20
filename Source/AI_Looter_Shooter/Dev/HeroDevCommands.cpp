// Developer console commands for the hero pick (not in shipping builds):
//   Looter.Hero            the roster (playable or in the works, and why), the hero played now and the no-session pick
//   Looter.Hero <Id>       the hero a play with no session plays (Looter.PlayLevel, a level straight from the editor), put
//                          on at once when such a play is running (body, powers, HUD portrait); a session keeps its own hero
// Looter.Hero.Unlock 1 (a console variable, HeroDefinition.cpp) lets heroes in the works be picked too: on the hero
// select and here (they play with Ellis's body and no powers until their model, retargeter and powers are in the game).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Player/FirstPersonHandsComponent.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/HudPortraitWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/** The game world the command was typed in, or the running play session's (from the editor). */
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

	USessionSubsystem* FindSessions(UWorld* GameWorld)
	{
		const UGameInstance* GameInstance = GameWorld ? GameWorld->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<USessionSubsystem>() : nullptr;
	}

	/** Why a hero isn't playable yet, in a few words ("no SK_Crane, no power code"), or "playable". */
	FString PlayableWords(const UHeroDefinition& Hero)
	{
		TArray<FString> Missing;
		if (!Hero.LoadFigureMesh())
		{
			Missing.Add(FString::Printf(TEXT("no mesh (%s)"), *Hero.GetFigureMeshPath().GetAssetName()));
		}
		if (!Hero.LoadRetargeter())
		{
			Missing.Add(FString::Printf(TEXT("no retargeter (%s)"), *Hero.GetRetargeterPath().GetAssetName()));
		}
		if (!Hero.HasAllPowerCode())
		{
			int32 WithCode = 0;
			for (const UEmberDefinition* Power : Hero.Powers)
			{
				WithCode += Power && Power->PowerClass.Get() ? 1 : 0;
			}
			Missing.Add(FString::Printf(TEXT("power code for %d of %d"), WithCode, Hero.Powers.Num()));
		}
		return Missing.IsEmpty() ? FString(TEXT("playable")) : FString::Printf(TEXT("in the works: %s"), *FString::Join(Missing, TEXT(", ")));
	}

	void ListHeroes(UWorld* GameWorld)
	{
		const USessionSubsystem* Sessions = FindSessions(GameWorld);
		UE_LOG(LogLooter, Display, TEXT("Heroes (Looter.Hero.Unlock %s):"), UHeroDefinition::AreLockedHeroesUnlocked() ? TEXT("on") : TEXT("off"));
		for (const FName Id : UHeroDefinition::RosterIds())
		{
			const UHeroDefinition* Hero = UHeroDefinition::Load(Id);
			UE_LOG(LogLooter, Display, TEXT("  %-7s %s"), *Id.ToString(),
				Hero ? *FString::Printf(TEXT("%s, %s: %s"), *Hero->DisplayName.ToString(), *Hero->Role.ToString(), *PlayableWords(*Hero))
					 : TEXT("no DA_Hero_ asset (run Tools/Unreal/create_ember_assets.py)"));
		}
		UE_LOG(LogLooter, Display, TEXT("Played now: %s%s. With no session: %s."),
			Sessions ? *Sessions->GetPlayedHeroId().ToString() : TEXT("(no game running)"),
			Sessions && Sessions->IsPlayingSession() ? *FString::Printf(TEXT(" (session %d's)"), Sessions->GetActiveSession() + 1) : TEXT(""),
			*USessionSubsystem::GetNoSessionHeroId().ToString());
	}

	/**
	 * Puts the hero played on everything in GameWorld wearing one: the player's powers, body and first-person arms, the
	 * loadout's stand-in, the HUD's portrait.
	 */
	void WearHeroNow(UWorld* GameWorld)
	{
		for (FConstPlayerControllerIterator It = GameWorld->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* Player = It->Get();
			if (UEmberComponent* Embers = UEmberComponent::Find(Player ? Player->GetPawn() : nullptr))
			{
				Embers->RebindHero();
			}
		}
		for (TObjectIterator<UBodyFigureComponent> It; It; ++It)
		{
			if (It->GetWorld() == GameWorld && It->HasBegunPlay())
			{
				It->WearHero();
			}
		}
		for (TObjectIterator<UFirstPersonHandsComponent> It; It; ++It)
		{
			if (It->GetWorld() == GameWorld && It->HasBegunPlay())
			{
				It->WearHero();
			}
		}
		// The HUD's portrait window too.
		const UHeroDefinition* Played = USessionSubsystem::FindPlayedHero(GameWorld);
		for (TObjectIterator<UHudPortraitWidget> It; It; ++It)
		{
			if (Played && It->GetWorld() == GameWorld)
			{
				It->SetHero(Played->Id);
			}
		}
	}

	void SetHero(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		if (Args.IsEmpty())
		{
			ListHeroes(GameWorld);
			UE_LOG(LogLooter, Display, TEXT("Usage: Looter.Hero <Ellis|Odessa|Crane|Gauge|Pike>"));
			return;
		}
		const FName* Found = UHeroDefinition::RosterIds().FindByPredicate([&Args](FName Id) { return Id.ToString().Equals(Args[0], ESearchCase::IgnoreCase); });
		const UHeroDefinition* Hero = Found ? UHeroDefinition::Load(*Found) : nullptr;
		if (!Hero)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Hero: no hero %s (the roster: Ellis, Odessa, Crane, Gauge, Pike)."), *Args[0]);
			return;
		}
		if (!Hero->CanBePicked())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Hero: %s is %s. Looter.Hero.Unlock 1 lets them be picked (Ellis's body, no powers)."),
				*Hero->Id.ToString(), *PlayableWords(*Hero));
			return;
		}
		USessionSubsystem* Sessions = FindSessions(GameWorld);
		if (Sessions && Sessions->IsPlayingSession())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Hero: session %d plays %s, and a session keeps its hero. Looter.Hero is for plays with no session (Looter.PlayLevel)."),
				Sessions->GetActiveSession() + 1, *Sessions->GetPlayedHeroId().ToString());
			return;
		}
		USessionSubsystem::SetNoSessionHeroId(Hero->Id);
		UE_LOG(LogLooter, Display, TEXT("Looter.Hero: plays with no session play %s (%s)."), *Hero->Id.ToString(), *PlayableWords(*Hero));
		if (GameWorld && Sessions)
		{
			WearHeroNow(GameWorld);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs HeroCommand(
		TEXT("Looter.Hero"),
		TEXT("Looter.Hero <Ellis|Odessa|Crane|Gauge|Pike>: the hero a play with no session plays (put on at once in such a play). "
			"With no id: the roster, playable or in the works and why."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SetHero));
}

#endif
