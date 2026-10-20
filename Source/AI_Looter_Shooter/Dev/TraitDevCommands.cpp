// Developer console commands for the creature traits (UCreatureTraitComponent; not in shipping builds): give the nearest
// creature exactly the traits named, to see and test each one in play, and list the traits of those near.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureTraitRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** How far from the player the commands look (cm). */
	constexpr double Reach = 5000.0;

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

	const APawn* FindPlayer(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	FString TraitList(const UCreatureTraitComponent& Traits)
	{
		TArray<FString> Names;
		for (const ECreatureTrait Trait : Traits.GetTraits())
		{
			Names.Add(CreatureTraitRules::Name(Trait));
		}
		return Names.IsEmpty() ? FString(TEXT("none")) : FString::Join(Names, TEXT(", "));
	}

	/** Looter.Traits.Give <Trait> [Trait...] | none: the nearest living creature to the player gets exactly these traits. */
	void GiveTraits(const TArray<FString>& Args, UWorld* InWorld)
	{
		const TCHAR* Command = TEXT("Looter.Traits.Give");
		UWorld* World = FindGameWorld(InWorld);
		const APawn* Player = FindPlayer(World);
		if (!Player)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no player in a level being played."), Command);
			return;
		}
		TArray<ECreatureTrait> Wanted;
		for (const FString& Arg : Args)
		{
			ECreatureTrait Trait;
			if (Arg.Equals(TEXT("none"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (!CreatureTraitRules::Parse(Arg, Trait))
			{
				UE_LOG(LogLooter, Warning, TEXT("%s: no trait \"%s\" (Fleet, Mending, Warded, GraveBurst, Rallying, Vengeful, Phasing)."), Command, *Arg);
				return;
			}
			Wanted.AddUnique(Trait);
		}
		ACreatureBase* Nearest = nullptr;
		double Best = Reach * Reach;
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			const double Distance = FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation());
			if (!It->IsDead() && !It->IsBossCreature() && Distance < Best)
			{
				Best = Distance;
				Nearest = *It;
			}
		}
		if (!Nearest)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: no living creature (not a boss) within %.0f m."), Command, Reach / 100.0);
			return;
		}
		UCreatureTraitComponent* Traits = UCreatureTraitComponent::FindOrAdd(*Nearest);
		Traits->SetTraits(Wanted);
		UE_LOG(LogLooter, Display, TEXT("%s: %s (%.0f m away) now has %s (until its rank is applied again)."), Command, *Nearest->GetName(),
			FMath::Sqrt(Best) / 100.0, *TraitList(*Traits));
	}

	/** Looter.Traits.Show: every creature within 50 m with traits: which, and what they're doing. */
	void ShowTraits(const TArray<FString>& Args, UWorld* InWorld)
	{
		UWorld* World = FindGameWorld(InWorld);
		const APawn* Player = FindPlayer(World);
		if (!Player)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Traits.Show: no player in a level being played."));
			return;
		}
		int32 Shown = 0;
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			const UCreatureTraitComponent* Traits = UCreatureTraitComponent::Find(*It);
			const double Distance = FVector::Dist(It->GetActorLocation(), Player->GetActorLocation());
			if (!Traits || Traits->GetTraits().IsEmpty() || Distance > Reach || It->IsDead())
			{
				continue;
			}
			const UHealthComponent* Health = It->FindComponentByClass<UHealthComponent>();
			UE_LOG(LogLooter, Display, TEXT("  %s (%s, %.0f m, %.0f%% health): %s; ward %.0f%%%s%s%s%s, %d rallied, %d blinks"), *It->GetName(),
				*UCreatureRankSettings::GetRankName(It->GetRank()), Distance / 100.0, Health ? Health->GetHealthPercent() * 100.f : 0.f, *TraitList(*Traits),
				Traits->GetWardFraction() * 100.f, Traits->IsMending() ? TEXT(", mending") : TEXT(""), Traits->IsEnraged() ? TEXT(", enraged") : TEXT(""),
				Traits->IsBlinking() ? TEXT(", blinking") : TEXT(""), Traits->IsSwelling() ? TEXT(", swelling") : TEXT(""), Traits->GetRalliedCount(),
				Traits->GetBlinkCount());
			++Shown;
		}
		UE_LOG(LogLooter, Display, TEXT("Looter.Traits.Show: %d creature(s) with traits within %.0f m."), Shown, Reach / 100.0);
	}

	FAutoConsoleCommandWithWorldAndArgs GiveCommand(
		TEXT("Looter.Traits.Give"),
		TEXT("Looter.Traits.Give <Trait> [Trait...] | none: the nearest living creature (50 m, not a boss) gets exactly these traits: ")
		TEXT("Fleet, Mending, Warded, GraveBurst, Rallying, Vengeful, Phasing (until its rank is applied again)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveTraits));

	FAutoConsoleCommandWithWorldAndArgs ShowCommand(
		TEXT("Looter.Traits.Show"),
		TEXT("Looter.Traits.Show: every creature with traits within 50 m: its traits, ward, mending, rage, blinks and whom it rallies."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ShowTraits));
}

#endif
