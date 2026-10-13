#include "Economy/WalletSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Economy/EconomyRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"

UWalletSubsystem* UWalletSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UWalletSubsystem>() : nullptr;
}

int32 UWalletSubsystem::SilverOf(const UObject* WorldContextObject)
{
	const UWalletSubsystem* Wallet = Get(WorldContextObject);
	return Wallet ? Wallet->GetSilver() : 0;
}

int32 UWalletSubsystem::PlayerLevelOf(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	return Progression ? FMath::Max(Progression->GetLevel(), 1) : 1;
}

bool UWalletSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

int32 UWalletSubsystem::Add(int32 Amount, ESilverChange Why)
{
	if (Amount <= 0)
	{
		return 0;
	}
	const int32 Before = Silver;
	Silver = static_cast<int32>(FMath::Min<int64>(static_cast<int64>(Silver) + Amount, LooterEconomy::MaxSilver));
	const int32 Added = Silver - Before;
	if (Added > 0)
	{
		OnSilverChanged.Broadcast(Silver, Added, Why);
	}
	return Added;
}

bool UWalletSubsystem::Spend(int32 Price)
{
	if (Price <= 0)
	{
		return Price == 0;
	}
	if (Price > Silver)
	{
		return false;
	}
	Silver -= Price;
	OnSilverChanged.Broadcast(Silver, -Price, ESilverChange::Spent);
	return true;
}

void UWalletSubsystem::SetSilver(int32 Amount)
{
	const int32 Before = Silver;
	Silver = FMath::Clamp(Amount, 0, LooterEconomy::MaxSilver);
	OnSilverChanged.Broadcast(Silver, Silver - Before, ESilverChange::Restored);
}

const FVendorOffer& UWalletSubsystem::RefreshOffer(int32 PlayerLevel, FRandomStream* Random)
{
	const double Now = GetClock();
	if (Offer.IsDue(PlayerLevel, Now))
	{
		FRandomStream Fresh(FMath::Rand());
		Offer.Roll(PlayerLevel, Now, Random ? *Random : Fresh);
	}
	return Offer;
}

void UWalletSubsystem::MarkOfferSold()
{
	Offer.bSold = true;
}

double UWalletSubsystem::GetClock() const
{
	const UWorld* World = GetWorld();
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	if (Sessions && Sessions->IsPlayingSession())
	{
		return Sessions->GetPlayedSecondsNow(World);
	}
	return World ? World->GetTimeSeconds() : 0.0;
}
