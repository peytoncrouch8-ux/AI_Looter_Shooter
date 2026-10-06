// Developer console command for the level's lighting states (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "World/LightingStateSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
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

	/**
	 * Looter.Light [state] [seconds | now]: switches the level's light to one of its states (Day, Dusk) behind a short
	 * fade, eases into it over the seconds given, or switches at once with "now"; with no state, lists the level's states
	 * and the one in place. The subsystem logs everything a switch sets. Only in a game: switching the editor's level
	 * would change the lights it saves.
	 */
	void LightCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		ULightingStateSubsystem* Lighting = GameWorld ? GameWorld->GetSubsystem<ULightingStateSubsystem>() : nullptr;
		if (!Lighting)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Light: start the game first."));
			return;
		}
		if (Args.IsEmpty())
		{
			const FString MapName = GameWorld->GetMapName();
			const TArray<FName> Names = Lighting->GetStateNames();
			if (Names.IsEmpty())
			{
				UE_LOG(LogLooter, Display, TEXT("Looter.Light: %s has no lighting states."), *MapName);
				return;
			}
			const FName Pending = Lighting->GetPendingState();
			const FString Switching = Pending.IsNone() ? FString() : FString::Printf(TEXT(", switching to %s"), *Pending.ToString());
			const FString Listed = FString::JoinBy(Names, TEXT(", "), [](const FName& Each) { return Each.ToString(); });
			UE_LOG(LogLooter, Display, TEXT("Looter.Light: %s is in %s%s; its states: %s. Looter.Light <state> [seconds | now] switches."),
				*MapName, *Lighting->GetState().ToString(), *Switching, *Listed);
			return;
		}

		ELightingSwitch How = ELightingSwitch::Fade;
		float Seconds = 0.f;
		if (Args.Num() > 1)
		{
			if (Args[1].Equals(TEXT("now"), ESearchCase::IgnoreCase) || Args[1].Equals(TEXT("instant"), ESearchCase::IgnoreCase))
			{
				How = ELightingSwitch::Instant;
			}
			else if (Args[1].IsNumeric() && FCString::Atof(*Args[1]) > 0.f)
			{
				How = ELightingSwitch::Blend;
				Seconds = FCString::Atof(*Args[1]);
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Light: '%s' is neither a number of seconds nor 'now'."), *Args[1]);
				return;
			}
		}
		// The subsystem says why when it can't (no such state here), and logs the switch when it lands.
		Lighting->SetState(FName(*Args[0]), How, Seconds);
	}

	FAutoConsoleCommandWithWorldAndArgs LightCommandRegistration(
		TEXT("Looter.Light"),
		TEXT("Switches the level's lighting state behind a short fade, over some seconds, or now; with no state, lists them: Looter.Light [Day|Dusk] [seconds|now]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&LightCommand));
}

#endif
