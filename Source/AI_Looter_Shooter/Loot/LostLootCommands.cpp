// Developer console commands for Lost Property (not in shipping builds): what's on the shelves and lying on the ground,
// handing every ground gun in now, emptying the shelves, cutting the ground's five minutes short, opening the screen.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Economy/WalletSubsystem.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootSubsystem.h"
#include "UI/Style/WeaponText.h"
#include "UI/World/LostLootWidget.h"
#include "Weapons/WeaponBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running play session when typed in the editor. */
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

	ULostLootSubsystem* FindLostLoot(UWorld* World, const TCHAR* Command)
	{
		ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(FindGameWorld(World));
		if (!LostLoot)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no level is being played."), Command);
		}
		return LostLoot;
	}

	void List(const TArray<FString>& Args, UWorld* World)
	{
		ULostLootSubsystem* LostLoot = FindLostLoot(World, TEXT("Looter.LostLoot.List"));
		if (!LostLoot)
		{
			return;
		}
		const FLostLootStore& Store = LostLoot->GetStore();
		const int32 Level = UWalletSubsystem::PlayerLevelOf(LostLoot);
		UE_LOG(LogLooter, Display, TEXT("Lost Property: %d / %d on the shelves, %d of %d shelving bought (next %d silver at level %d), %d given away to make room, %d waiting to be told of."),
			Store.Num(), Store.GetCapacity(), Store.Upgrades, LostLootRules::MaxUpgrades, LostLootRules::UpgradePrice(Store.Upgrades, Level), Level,
			Store.GivenAway, Store.Unannounced);
		const int32 NextToGo = Store.FindNextToGo();
		for (int32 Index = Store.Entries.Num() - 1; Index >= 0; --Index)
		{
			const FLostLootEntry& Entry = Store.Entries[Index];
			UE_LOG(LogLooter, Display, TEXT("  No. %s  %s %s, level %d  (%s)%s"), *FLostLootStore::TicketText(Entry.Ticket),
				*UEnum::GetDisplayValueAsText(Entry.Weapon.Rarity).ToString(), *LooterWeaponText::Name(Entry.Weapon), Entry.Weapon.Level,
				*LostLootRules::ReasonWords(Entry.Reason, Entry.Map).ToString(), Index == NextToGo ? TEXT("  <- next to go") : TEXT(""));
		}
		const TArray<AWeaponBase*> Ground = LostLoot->FindGroundWeapons();
		UE_LOG(LogLooter, Display, TEXT("On the ground: %d guns."), Ground.Num());
		for (const AWeaponBase* Gun : Ground)
		{
			UE_LOG(LogLooter, Display, TEXT("  %s %s, level %d: %.0f s left%s"), *UEnum::GetDisplayValueAsText(Gun->GetRarity()).ToString(),
				*LooterWeaponText::Name(Gun->GetInstance()), Gun->GetInstance().Level, LostLoot->GetLifeLeft(*Gun), Gun->IsHidden() ? TEXT(" (hidden: not aging)") : TEXT(""));
		}
	}

	void Send(const TArray<FString>& Args, UWorld* World)
	{
		if (ULostLootSubsystem* LostLoot = FindLostLoot(World, TEXT("Looter.LostLoot.Send")))
		{
			const int32 Sent = LostLoot->SendAllGroundWeapons(ELostLootReason::Console);
			UE_LOG(LogLooter, Display, TEXT("Looter.LostLoot.Send: %d guns handed in; %d / %d on the shelves."), Sent, LostLoot->GetStore().Num(),
				LostLoot->GetCapacity());
		}
	}

	void Clear(const TArray<FString>& Args, UWorld* World)
	{
		if (ULostLootSubsystem* LostLoot = FindLostLoot(World, TEXT("Looter.LostLoot.Clear")))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.LostLoot.Clear: %d guns gone from the shelves."), LostLoot->ClearStore());
		}
	}

	void Life(const TArray<FString>& Args, UWorld* World)
	{
		ULostLootSubsystem* LostLoot = FindLostLoot(World, TEXT("Looter.LostLoot.Life"));
		if (!LostLoot)
		{
			return;
		}
		const float Seconds = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
		const TArray<AWeaponBase*> Ground = LostLoot->FindGroundWeapons();
		for (AWeaponBase* Gun : Ground)
		{
			LostLoot->SetLifeLeft(*Gun, Seconds);
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.LostLoot.Life: %d guns on the ground have %.0f s left."), Ground.Num(), Seconds);
	}

	void Shelving(const TArray<FString>& Args, UWorld* World)
	{
		ULostLootSubsystem* LostLoot = FindLostLoot(World, TEXT("Looter.LostLoot.Shelving"));
		if (!LostLoot)
		{
			return;
		}
		FLostLootStore& Store = LostLoot->GetMutableStore();
		Store.Upgrades = FMath::Clamp(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0, 0, LostLootRules::MaxUpgrades);
		LostLoot->NotifyChanged();
		UE_LOG(LogLooter, Display, TEXT("Looter.LostLoot.Shelving: %d bought, room for %d."), Store.Upgrades, Store.GetCapacity());
	}

	void Open(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Player = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		if (!ULostLootWidget::OpenFor(Player, nullptr))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.LostLoot.Open: no player to open Lost Property for."));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ListCommand(
		TEXT("Looter.LostLoot.List"),
		TEXT("Looter.LostLoot.List: Lost Property's shelves (count, shelving, every ticket newest first, the next to go) and the guns lying on the ground with their time left."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&List));

	FAutoConsoleCommandWithWorldAndArgs SendCommand(
		TEXT("Looter.LostLoot.Send"),
		TEXT("Looter.LostLoot.Send: hands every gun lying on the ground in to Lost Property now, as leaving the area does."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Send));

	FAutoConsoleCommandWithWorldAndArgs ClearCommand(
		TEXT("Looter.LostLoot.Clear"),
		TEXT("Looter.LostLoot.Clear: empties Lost Property's shelves (the shelving bought stays)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Clear));

	FAutoConsoleCommandWithWorldAndArgs LifeCommand(
		TEXT("Looter.LostLoot.Life"),
		TEXT("Looter.LostLoot.Life [seconds, 10]: every gun on the ground has that long left before it goes to Lost Property."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Life));

	FAutoConsoleCommandWithWorldAndArgs ShelvingCommand(
		TEXT("Looter.LostLoot.Shelving"),
		TEXT("Looter.LostLoot.Shelving <0-5>: sets how much shelving is bought (room for 30 plus 10 each)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Shelving));

	FAutoConsoleCommandWithWorldAndArgs OpenCommand(
		TEXT("Looter.LostLoot.Open"),
		TEXT("Looter.LostLoot.Open: opens Lost Property's screen where the player stands, with no window."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Open));
}

#endif
