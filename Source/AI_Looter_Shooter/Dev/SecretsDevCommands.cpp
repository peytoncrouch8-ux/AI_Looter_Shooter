// Developer console commands for Sexton's secrets (lanes/SECRETS.md; not in shipping builds):
//   Looter.Secrets.List                  every secret here: its state, its price now, how many places it marks
//   Looter.Secrets.Buy <Id|all> [free]   buys one (or every one for sale) as the screen does; "free": no silver taken
//   Looter.Secrets.Forget [Id]           unsells one (every one without an id): its marks go
//   Looter.Secrets.Open                  Sexton's screen wherever the player stands (it doesn't close for distance)
//   Looter.GiveSilver <amount>           silver into the player's purse (to try the prices)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Economy/SecretBroker.h"
#include "Economy/SecretCatalog.h"
#include "Economy/SecretSubsystem.h"
#include "Economy/WalletSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "UI/Vendor/SecretsWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindSecretsWorld(UWorld* World)
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

	/** The level's secrets with a catalog, or null with a word in the log. */
	USecretSubsystem* FindSecrets(UWorld* World, const TCHAR* Command)
	{
		USecretSubsystem* Secrets = USecretSubsystem::Get(World);
		if (!Secrets || !Secrets->GetCatalog())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no secrets in this level (Ransom's Rest: run Tools/Unreal/create_secret_assets.py)."), Command);
			return nullptr;
		}
		return Secrets;
	}

	const TCHAR* StateWord(ESecretState State)
	{
		switch (State)
		{
		case ESecretState::Hidden:  return TEXT("not yet");
		case ESecretState::ForSale: return TEXT("for sale");
		case ESecretState::Sold:    return TEXT("SOLD (marked)");
		case ESecretState::Done:    return TEXT("done (found)");
		}
		return TEXT("?");
	}

	const TCHAR* BuyWord(ESecretBuy Result)
	{
		switch (Result)
		{
		case ESecretBuy::Bought:          return TEXT("bought");
		case ESecretBuy::NotForSale:      return TEXT("not for sale now");
		case ESecretBuy::AlreadySold:     return TEXT("sold already");
		case ESecretBuy::NotEnoughSilver: return TEXT("not enough silver (try free, or Looter.GiveSilver)");
		case ESecretBuy::NoTrade:         return TEXT("no story or wallet here");
		}
		return TEXT("?");
	}

	void SecretsList(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Secrets.List");
		World = FindSecretsWorld(World);
		USecretSubsystem* Secrets = World ? FindSecrets(World, Command) : nullptr;
		const UMissionRunner* Runner = World ? UMissionRunner::Get(World) : nullptr;
		if (!Secrets || !Runner || !Runner->IsActive())
		{
			return;
		}
		const FCampaignRecord& Record = Runner->GetCampaign();
		const USecretCatalog* Catalog = Secrets->GetCatalog();
		const int32 Level = UWalletSubsystem::PlayerLevelOf(World);
		const float Discount = Catalog->DiscountFor(Record);
		UE_LOG(LogLooter, Display, TEXT("%s: %s, %s; %d%% off; the player at level %d with %d silver."), Command, *Catalog->GetName(),
			Catalog->IsOpen(Record, Runner) ? TEXT("open") : TEXT("SHUT"), FMath::RoundToInt32(Discount * 100.f), Level, UWalletSubsystem::SilverOf(World));
		for (const FSecretOffer& Offer : Catalog->Secrets)
		{
			UE_LOG(LogLooter, Display, TEXT("  %-12s %-14s %4d silver, %d place(s): %s"), *Offer.Id.ToString(),
				StateWord(SecretRules::StateOf(Offer, Record, Runner)), SecretRules::PriceFor(Offer, Level, Discount),
				Secrets->CountPlaces(Offer, Record), *Offer.Title.ToString());
		}
	}

	void SecretsBuy(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Secrets.Buy");
		World = FindSecretsWorld(World);
		USecretSubsystem* Secrets = World ? FindSecrets(World, Command) : nullptr;
		if (!Secrets)
		{
			return;
		}
		if (Args.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s <Id|all> [free]"), Command);
			return;
		}
		const bool bFree = Args.Num() > 1 && Args[1].Equals(TEXT("free"), ESearchCase::IgnoreCase);
		TArray<FName> Ids;
		if (Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase))
		{
			for (const FSecretRow& Row : Secrets->GetRows())
			{
				if (Row.State == ESecretState::ForSale)
				{
					Ids.Add(Row.Id);
				}
			}
		}
		else
		{
			Ids.Add(FName(*Args[0]));
		}
		for (const FName Id : Ids)
		{
			UE_LOG(LogLooter, Display, TEXT("%s %s: %s."), Command, *Id.ToString(), BuyWord(Secrets->Buy(Id, bFree)));
		}
	}

	void SecretsForget(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Secrets.Forget");
		World = FindSecretsWorld(World);
		if (USecretSubsystem* Secrets = World ? FindSecrets(World, Command) : nullptr)
		{
			if (Args.IsEmpty())
			{
				UE_LOG(LogLooter, Display, TEXT("%s: %d unsold."), Command, Secrets->ForgetAll());
			}
			else
			{
				UE_LOG(LogLooter, Display, TEXT("%s %s: %s."), Command, *Args[0], Secrets->Forget(FName(*Args[0])) ? TEXT("unsold") : TEXT("wasn't sold"));
			}
		}
	}

	void SecretsOpen(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Secrets.Open");
		World = FindSecretsWorld(World);
		if (!World)
		{
			return;
		}
		APlayerController* Controller = World->GetFirstPlayerController();
		USecretBrokerComponent* Broker = nullptr;
		for (TActorIterator<AActor> It(World); It && !Broker; ++It)
		{
			Broker = It->FindComponentByClass<USecretBrokerComponent>();
		}
		if (!Broker || !Controller)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no one sells secrets in this level (Mister Sexton, build_area_main2.py)."), Command);
			return;
		}
		if (!Broker->IsOpen())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: %s isn't trading yet in this story (after Main 2)."), Command, *Broker->GetOwner()->GetActorNameOrLabel());
			return;
		}
		USecretsWidget::OpenFor(Controller, Broker, /*bInAnywhere*/ true);
	}

	void GiveSilver(const TArray<FString>& Args, UWorld* World)
	{
		World = FindSecretsWorld(World);
		UWalletSubsystem* Wallet = World ? UWalletSubsystem::Get(World) : nullptr;
		const int32 Amount = Args.IsEmpty() ? 500 : FCString::Atoi(*Args[0]);
		if (Wallet)
		{
			const int32 Added = Wallet->Add(Amount, ESilverChange::Found);
			UE_LOG(LogLooter, Display, TEXT("Looter.GiveSilver: +%d, %d carried."), Added, Wallet->GetSilver());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs SecretsListRegistration(TEXT("Looter.Secrets.List"),
		TEXT("Lists Sexton's secrets here: each one's state in this story, its price now and how many places it marks."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SecretsList));

	FAutoConsoleCommandWithWorldAndArgs SecretsBuyRegistration(TEXT("Looter.Secrets.Buy"),
		TEXT("Buys one of Sexton's secrets as his screen does: Looter.Secrets.Buy <Id|all> [free]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SecretsBuy));

	FAutoConsoleCommandWithWorldAndArgs SecretsForgetRegistration(TEXT("Looter.Secrets.Forget"),
		TEXT("Unsells one of Sexton's secrets (every one without an id): Looter.Secrets.Forget [Id]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SecretsForget));

	FAutoConsoleCommandWithWorldAndArgs SecretsOpenRegistration(TEXT("Looter.Secrets.Open"),
		TEXT("Opens Sexton's secrets screen wherever the player stands (once he trades in this story)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SecretsOpen));

	FAutoConsoleCommandWithWorldAndArgs GiveSilverRegistration(TEXT("Looter.GiveSilver"),
		TEXT("Silver into the player's purse: Looter.GiveSilver <amount> (500 without one)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveSilver));
}

#endif
