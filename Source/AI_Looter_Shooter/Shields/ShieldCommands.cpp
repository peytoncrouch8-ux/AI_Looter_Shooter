#include "AI_Looter_Shooter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldPickup.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"

// The ward's dev commands (not shipping): give one, look at the one worn, break it or fill it on cue.
//   Looter.Shield.Give <Rarity> [level] [trait] [build] [wear]   a ward on the ground in front of the player (or worn)
//   Looter.Shield.State                                          the ward worn and the spares, in the log
//   Looter.Shield.Break | Fill                                   the ward worn broken as a hit breaks it (traits too), or full

namespace
{
	UPlayerShieldComponent* PlayerShield(UWorld* World, APawn** OutPawn = nullptr)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (OutPawn)
		{
			*OutPawn = Pawn;
		}
		return Pawn ? UPlayerShieldComponent::FindOrAdd(*Pawn) : nullptr;
	}

	template <typename EnumType>
	bool ParseEnum(const FString& Text, EnumType& Out)
	{
		const UEnum* Enum = StaticEnum<EnumType>();
		const int64 Value = Enum ? Enum->GetValueByNameString(Text) : INDEX_NONE;
		if (Value != INDEX_NONE)
		{
			Out = static_cast<EnumType>(Value);
			return true;
		}
		// Case doesn't matter at the console.
		for (int32 Index = 0; Enum && Index < Enum->NumEnums() - 1; ++Index)
		{
			if (Enum->GetNameStringByIndex(Index).Equals(Text, ESearchCase::IgnoreCase))
			{
				Out = static_cast<EnumType>(Enum->GetValueByIndex(Index));
				return true;
			}
		}
		return false;
	}

	void GiveCommand(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = nullptr;
		UPlayerShieldComponent* Shield = PlayerShield(World, &Pawn);
		if (!Shield || !Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Shield.Give: no player pawn."));
			return;
		}
		EWeaponRarity Rarity = EWeaponRarity::Rare;
		int32 Level = 1;
		EShieldTrait Trait = EShieldTrait::None;
		EShieldBuild Build = EShieldBuild::Standard;
		bool bTraitGiven = false;
		bool bBuildGiven = false;
		bool bWear = false;
		for (int32 Index = 0; Index < Args.Num(); ++Index)
		{
			const FString& Arg = Args[Index];
			if (Arg.IsNumeric())
			{
				Level = FMath::Max(FCString::Atoi(*Arg), 1);
			}
			else if (Arg.Equals(TEXT("wear"), ESearchCase::IgnoreCase))
			{
				bWear = true;
			}
			else if (ParseEnum(Arg, Rarity))
			{
			}
			else if (ParseEnum(Arg, Trait))
			{
				bTraitGiven = true;
			}
			else if (ParseEnum(Arg, Build))
			{
				bBuildGiven = true;
			}
			else
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Shield.Give: '%s' isn't a rarity, trait (LastBreath, Barbwire, Lodestone, Harvest, SaintsGrace), ")
					TEXT("build (Standard, Bulwark, Quickset), level or 'wear'."), *Arg);
			}
		}
		const int32 Seed = FMath::Rand();
		FShieldInstanceData Ward = ShieldRules::Make(UShieldDefinition::GetWardBox(), Rarity, Level, Seed);
		if (bTraitGiven || bBuildGiven)
		{
			Ward = ShieldRules::MakeWith(Ward.Definition, Rarity, Level, Seed, bBuildGiven ? Build : Ward.Build, bTraitGiven ? Trait : Ward.Trait);
		}
		if (bWear)
		{
			Shield->Equip(Ward, true);
		}
		else
		{
			AShieldPickup::DropWard(World, Ward, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 200.f,
				Pawn->GetActorForwardVector() * 150.f + FVector(0.f, 0.f, 300.f));
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Shield.Give: %s %s, level %d, %.0f / %.1f s / %.1f per s%s"), *UEnum::GetDisplayValueAsText(Rarity).ToString(),
			*ShieldText::Name(Ward), Level, Ward.Stats.Capacity, Ward.Stats.RechargeDelay, Ward.Stats.RechargeRate, bWear ? TEXT(", worn") : TEXT(""));
	}

	void LogWard(const TCHAR* What, const FShieldInstanceData& Ward)
	{
		UE_LOG(LogLooter, Log, TEXT("  %s: %s %s, level %d, %.0f / %.1f s / %.1f per s  %s"), What, *UEnum::GetDisplayValueAsText(Ward.Rarity).ToString(),
			*ShieldText::Name(Ward), Ward.Level, Ward.Stats.Capacity, Ward.Stats.RechargeDelay, Ward.Stats.RechargeRate, *ShieldText::TraitLine(Ward));
	}

	void StateCommand(const TArray<FString>& Args, UWorld* World)
	{
		const UPlayerShieldComponent* Shield = PlayerShield(World);
		if (!Shield)
		{
			return;
		}
		UE_LOG(LogLooter, Log, TEXT("Shield: %.0f of %.0f%s%s, wait left %.1f s; hits %u, breaks %u, fills %u"), Shield->GetCharge(), Shield->GetCapacity(),
			Shield->IsBroken() ? TEXT(", broken") : TEXT(""), Shield->IsRefilling() ? TEXT(", filling") : TEXT(""), Shield->GetWaitLeft(),
			Shield->GetHitCount(), Shield->GetBreakCount(), Shield->GetFillCount());
		if (Shield->HasWard())
		{
			LogWard(TEXT("worn"), Shield->GetWard());
		}
		for (const FShieldInstanceData& Spare : Shield->GetSpares())
		{
			LogWard(TEXT("spare"), Spare);
		}
	}

	void BreakCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (UPlayerShieldComponent* Shield = PlayerShield(World))
		{
			// A hit just bigger than what's left, as a bite would land: the break, its sound, its traits.
			const FVector Where = Shield->GetOwner() ? Shield->GetOwner()->GetActorLocation() : FVector::ZeroVector;
			Shield->SoakHit(Shield->GetCharge() + 0.01f, false, Where);
		}
	}

	void FillCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (UPlayerShieldComponent* Shield = PlayerShield(World))
		{
			Shield->SetChargeForTest(Shield->GetCapacity());
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GiveRegistration(TEXT("Looter.Shield.Give"),
		TEXT("Looter.Shield.Give <Common|Uncommon|Rare|Epic|Legendary> [level] [trait] [build] [wear]: a ward in front of the player ")
		TEXT("(worn at once with 'wear'). Traits: LastBreath, Barbwire, Lodestone, Harvest, SaintsGrace; builds: Standard, Bulwark, Quickset."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveCommand));
	FAutoConsoleCommandWithWorldAndArgs StateRegistration(TEXT("Looter.Shield.State"),
		TEXT("Looter.Shield.State: the ward worn and the spares carried, in the log."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&StateCommand));
	FAutoConsoleCommandWithWorldAndArgs BreakRegistration(TEXT("Looter.Shield.Break"),
		TEXT("Looter.Shield.Break: the ward worn broken as a hit breaks it (its sound, the HUD's break, its trait)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BreakCommand));
	FAutoConsoleCommandWithWorldAndArgs FillRegistration(TEXT("Looter.Shield.Fill"),
		TEXT("Looter.Shield.Fill: the ward worn full at once."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FillCommand));
}
