// Developer console commands for the soul flask (not in shipping builds): fill it, drink from it, and see where it stands.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerFlaskComponent.h"
#include "Player/PlayerFlaskRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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

	/** The player's flask, logged when there's none (no game running, or a pawn without one). */
	UPlayerFlaskComponent* FindFlask(UWorld* World, const TCHAR* Command)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		UPlayerFlaskComponent* Flask = UPlayerFlaskComponent::Find(Controller ? Controller->GetPawn() : nullptr);
		if (!Flask)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player with a soul flask (start the game first)."), Command);
		}
		return Flask;
	}

	const TCHAR* BlockName(EFlaskBlock Block)
	{
		switch (Block)
		{
		case EFlaskBlock::None: return TEXT("ready");
		case EFlaskBlock::Empty: return TEXT("empty");
		case EFlaskBlock::FullHealth: return TEXT("health full");
		case EFlaskBlock::Busy: return TEXT("hands busy");
		case EFlaskBlock::Dead: return TEXT("dead");
		case EFlaskBlock::Scene: return TEXT("in a scene");
		case EFlaskBlock::Menu: return TEXT("behind a menu");
		case EFlaskBlock::Moving: return TEXT("mid-mantle or vault");
		}
		return TEXT("?");
	}

	/** Looter.Flask.Fill [n]: n charges' worth of soul poured in (all of them by default), through the soul's own way in. */
	void Fill(const TArray<FString>& Args, UWorld* World)
	{
		UPlayerFlaskComponent* Flask = FindFlask(World, TEXT("Looter.Flask.Fill"));
		if (!Flask)
		{
			return;
		}
		if (Args.Num() == 0)
		{
			Flask->RefillAll();
		}
		else
		{
			// A fraction pours part of a charge (0.5: half a pip), to see the HUD's liquid level.
			const float Charges = FCString::Atof(*Args[0]);
			Flask->AddSoul(Charges * FFlaskRules::SoulPerCharge);
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Flask.Fill: %d of %d charges, %.0f soul toward the next."), Flask->GetCharges(),
			UPlayerFlaskComponent::GetMaxCharges(), Flask->GetSoul());
	}

	/** Looter.Flask.Drink: presses the Flask key (refused as the key would be; at full health, hurt first with "hurt"). */
	void Drink(const TArray<FString>& Args, UWorld* World)
	{
		UPlayerFlaskComponent* Flask = FindFlask(World, TEXT("Looter.Flask.Drink"));
		if (!Flask)
		{
			return;
		}
		// "hurt": takes the player down to 40% first, without it being a hit (no killer, no numbers), to watch the heal.
		UHealthComponent* Health = Flask->GetOwner()->FindComponentByClass<UHealthComponent>();
		if (Args.Num() > 0 && Args[0].Equals(TEXT("hurt"), ESearchCase::IgnoreCase) && Health && !Health->IsDead())
		{
			Health->Drain(Health->GetHealth() - Health->GetMaxHealth() * 0.4f);
		}
		const EFlaskBlock Block = Flask->GetBlock();
		const bool bStarted = Flask->TryDrink();
		UE_LOG(LogLooter, Display, TEXT("Looter.Flask.Drink: %s (%d charges left)."), bStarted ? TEXT("drinking") : BlockName(Block), Flask->GetCharges());
	}

	/** Looter.Flask.Status: the charges, the soul, the drink under way and what a press would do now. */
	void Status(const TArray<FString>& Args, UWorld* World)
	{
		UPlayerFlaskComponent* Flask = FindFlask(World, TEXT("Looter.Flask.Status"));
		if (!Flask)
		{
			return;
		}
		const UHealthComponent* Health = Flask->GetOwner()->FindComponentByClass<UHealthComponent>();
		UE_LOG(LogLooter, Display, TEXT("Looter.Flask.Status: %d of %d charges, %.0f soul toward the next (%.0f%%); %s%s; a press: %s; %d drunk with this body; health %.0f / %.0f."),
			Flask->GetCharges(), UPlayerFlaskComponent::GetMaxCharges(), Flask->GetSoul(), Flask->GetSoulShare() * 100.f,
			Flask->IsDrinking() ? *FString::Printf(TEXT("drinking, %.2f s in"), Flask->GetDrinkTime()) : TEXT("not drinking"),
			Flask->IsHealing() ? TEXT(", healing") : TEXT(""), BlockName(Flask->GetBlock()), Flask->GetDrinkCount(),
			Health ? Health->GetHealth() : 0.f, Health ? Health->GetMaxHealth() : 0.f);
	}

	FAutoConsoleCommandWithWorldAndArgs FillCommand(
		TEXT("Looter.Flask.Fill"),
		TEXT("Looter.Flask.Fill [charges]: fills the soul flask (with a number, pours that many charges' worth of soul, 0.5 for half a pip)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Fill));

	FAutoConsoleCommandWithWorldAndArgs DrinkCommand(
		TEXT("Looter.Flask.Drink"),
		TEXT("Looter.Flask.Drink [hurt]: presses the Flask key (\"hurt\" first takes the player down to 40% health to watch the heal)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Drink));

	FAutoConsoleCommandWithWorldAndArgs StatusCommand(
		TEXT("Looter.Flask.Status"),
		TEXT("Looter.Flask.Status: prints the soul flask's charges and soul, a drink under way and what a press would do now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Status));
}

#endif
