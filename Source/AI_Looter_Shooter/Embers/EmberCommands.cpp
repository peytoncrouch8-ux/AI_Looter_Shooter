// Developer console commands for the ember powers (UEmberComponent): fire one past its cooldown, equip one, show where they
// stand (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Embers/HeroDefinition.h"
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

	/** The player's ember component in the level being played, or null (with a warning naming Command). */
	UEmberComponent* FindEmbers(UWorld* World, const TCHAR* Command)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		UEmberComponent* Embers = UEmberComponent::Find(Controller ? Controller->GetPawn() : nullptr);
		if (!Embers)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player with ember powers in a level being played."), Command);
		}
		return Embers;
	}

	FString OwnedIds(const UEmberComponent& Embers)
	{
		TArray<FString> Ids;
		for (const UEmberDefinition* Def : Embers.GetOwned())
		{
			Ids.Add(Def ? Def->Id.ToString() : TEXT("(empty)"));
		}
		return Ids.IsEmpty() ? FString(TEXT("none")) : FString::Join(Ids, TEXT(", "));
	}

	/**
	 * Looter.Embers.Use [id]: fires the equipped power (or the named one, equipped first) past its cooldown; a running one
	 * ends first. The body's own gate still holds (dead, a scene, a menu, a mantle, a swap), and so does the power's say.
	 */
	void UseEmber(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Embers.Use");
		UEmberComponent* Embers = FindEmbers(World, Command);
		if (!Embers)
		{
			return;
		}
		Embers->Interrupt();
		if (Args.Num() > 0)
		{
			const FName Id(*Args[0]);
			if (Embers->GetEquippedId() != Id && !Embers->Equip(Id))
			{
				UE_LOG(LogLooter, Warning, TEXT("%s: the hero owns no \"%s\" (owned: %s)."), Command, *Args[0], *OwnedIds(*Embers));
				return;
			}
		}
		// Past the cooldown: the equipped one put back as a load would, ready.
		FEmberSave Ready;
		Ready.Equipped = Embers->GetEquippedId();
		Ready.CooldownLeft = 0.f;
		Embers->RestoreEmbers(Ready);
		const EEmberDenied Why = Embers->TryUse();
		if (Why == EEmberDenied::None)
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s used."), Command, *Embers->GetEquippedId().ToString());
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: %s not used (%s)."), Command, *Embers->GetEquippedId().ToString(), *UEnum::GetValueAsString(Why));
		}
	}

	/** Looter.Embers.Equip <id>: equips an owned power (not while one runs). */
	void EquipEmber(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Embers.Equip");
		UEmberComponent* Embers = FindEmbers(World, Command);
		if (!Embers)
		{
			return;
		}
		if (Args.Num() == 0)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s <id>: one of %s."), Command, *OwnedIds(*Embers));
			return;
		}
		const FName Id(*Args[0]);
		if (Embers->GetEquippedId() == Id)
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s is equipped already."), Command, *Id.ToString());
		}
		else if (Embers->IsActive())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: %s is running; it finishes first."), Command, *Embers->GetEquippedId().ToString());
		}
		else if (Embers->Equip(Id))
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s equipped."), Command, *Id.ToString());
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: the hero owns no \"%s\" (owned: %s)."), Command, *Args[0], *OwnedIds(*Embers));
		}
	}

	/** Looter.Embers.State: the hero, the powers owned, the one equipped, its state, its clocks and what blocks a press now. */
	void ShowState(const TArray<FString>& Args, UWorld* World)
	{
		const TCHAR* Command = TEXT("Looter.Embers.State");
		const UEmberComponent* Embers = FindEmbers(World, Command);
		if (!Embers)
		{
			return;
		}
		const UHeroDefinition* Hero = Embers->GetHero();
		UE_LOG(LogLooter, Log, TEXT("%s: hero %s, %d power(s); level scale %.2f"), Command, Hero ? *Hero->Id.ToString() : TEXT("(none)"),
			Embers->NumOwned(), Embers->GetLevelScale());
		const TArray<TObjectPtr<UEmberDefinition>>& Owned = Embers->GetOwned();
		for (int32 Index = 0; Index < Owned.Num(); ++Index)
		{
			const UEmberDefinition* Def = Owned[Index];
			if (!Def)
			{
				UE_LOG(LogLooter, Log, TEXT("%s:   %d (empty)"), Command, Index);
				continue;
			}
			const UEmberPower* Power = Embers->GetPowerFor(Def->Id);
			UE_LOG(LogLooter, Log, TEXT("%s: %s %d %s (%s): cooldown %.1f s, duration %.1f s, damage %.0f, range %.0f, radius %.0f, strength %.2f; %s"),
				Command, Index == Embers->GetEquippedIndex() ? TEXT("*") : TEXT(" "), Index, *Def->Id.ToString(), *Def->DisplayName.ToString(),
				Def->CooldownSeconds, Def->DurationSeconds, Def->Damage, Def->Range, Def->Radius, Def->Strength,
				Power ? *Power->GetClass()->GetName() : TEXT("NO POWER CLASS"));
		}
		UE_LOG(LogLooter, Log, TEXT("%s: %s %s; active %.1f s left (%.0f%%), cooldown %.1f s left (%.0f%% to go); a press now: %s"), Command,
			*Embers->GetEquippedId().ToString(), *UEnum::GetValueAsString(Embers->GetState()), Embers->GetActiveLeft(),
			100.f * Embers->GetActiveFraction(), Embers->GetCooldownLeft(), 100.f * Embers->GetCooldownFraction(),
			*UEnum::GetValueAsString(Embers->GetBlock()));
	}

	FAutoConsoleCommandWithWorldAndArgs UseCommand(
		TEXT("Looter.Embers.Use"),
		TEXT("Looter.Embers.Use [id]: fires the equipped ember power (or the named one, equipped first) past its cooldown; ")
		TEXT("ids: the hero's own (Looter.Embers.State lists them: DustDevil, LuckyStreak, TollChain, SlagBomb, Spyglass...)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UseEmber));

	FAutoConsoleCommandWithWorldAndArgs EquipCommand(
		TEXT("Looter.Embers.Equip"),
		TEXT("Looter.Embers.Equip <id>: equips an owned ember power (not while one runs); ids: the hero's own (Looter.Embers.State lists them)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&EquipEmber));

	FAutoConsoleCommandWithWorldAndArgs StateCommand(
		TEXT("Looter.Embers.State"),
		TEXT("Looter.Embers.State: the hero's ember powers, the one equipped, its state and clocks, and what blocks a press now."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowState));
}

#endif
