#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatusComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Inventory/WeaponManagerComponent.h"
#include "TimerManager.h"
#include "Weapons/WeaponBase.h"

// Looter.Element.Apply: sets an element's status on the creatures round the player, once or again and again, as a hit
// from the gun in their hand (its looks, sounds and damage over time, no roll): to see a status's look and to measure
// statuses' cost in a fight (perf.ps1 -Exec "...,Looter.Element.Apply Ember 20 2").

namespace
{
	FTimerHandle RepeatTimer;
	TWeakObjectPtr<UWorld> RepeatWorld;

	int32 ApplyAround(UWorld& World, ELooterElement Element, float RadiusCm, float HitDamage)
	{
		APlayerController* Controller = World.GetFirstPlayerController();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			return 0;
		}
		const UWeaponManagerComponent* Weapons = Pawn->FindComponentByClass<UWeaponManagerComponent>();
		AActor* Gun = Weapons ? Weapons->GetActiveWeapon() : nullptr;
		int32 Set = 0;
		for (TActorIterator<ACreatureBase> It(&World); It; ++It)
		{
			ACreatureBase* Creature = *It;
			if (Creature->IsDead() || FVector::Dist(Creature->GetActorLocation(), Pawn->GetActorLocation()) > RadiusCm)
			{
				continue;
			}
			// Chance 1 and a roll of 0: it always comes up, at the creature's weakness.
			Set += UElementStatusComponent::TryApply(*Creature, Element, 1.f, HitDamage, false, 0.f, Creature->GetActorLocation(),
				Gun ? Gun : static_cast<AActor*>(Pawn), Controller) ? 1 : 0;
		}
		return Set;
	}

	void ApplyCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (UWorld* Old = RepeatWorld.Get())
		{
			Old->GetTimerManager().ClearTimer(RepeatTimer);
		}
		RepeatWorld.Reset();
		if (Args.Num() > 0 && Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogLooter, Display, TEXT("Looter.Element.Apply: off."));
			return;
		}
		const ELooterElement Element = ElementRules::Parse(Args.Num() > 0 ? Args[0] : FString(TEXT("Ember")));
		if (Element == ELooterElement::None)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Element.Apply <Ember|SoulLight|GraveSalt|off> [radius m] [every s] [hit damage]"));
			return;
		}
		const float RadiusCm = (Args.Num() > 1 ? FCString::Atof(*Args[1]) : 20.f) * 100.f;
		const float Every = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.f;
		const float HitDamage = Args.Num() > 3 ? FCString::Atof(*Args[3]) : 40.f;
		const int32 Set = ApplyAround(*World, Element, RadiusCm, HitDamage);
		UE_LOG(LogLooter, Display, TEXT("Looter.Element.Apply: %s on %d creature(s) within %.0f m%s."), *ElementRules::Word(Element), Set,
			RadiusCm / 100.f, Every > 0.f ? *FString::Printf(TEXT(", again every %.1f s"), Every) : TEXT(""));
		if (Every > 0.f)
		{
			RepeatWorld = World;
			const TWeakObjectPtr<UWorld> WeakWorld(World);
			World->GetTimerManager().SetTimer(RepeatTimer, FTimerDelegate::CreateLambda([WeakWorld, Element, RadiusCm, HitDamage]()
			{
				if (UWorld* Again = WeakWorld.Get())
				{
					ApplyAround(*Again, Element, RadiusCm, HitDamage);
				}
			}), Every, true);
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ApplyCommandRegistration(
		TEXT("Looter.Element.Apply"),
		TEXT("Looter.Element.Apply <Ember|SoulLight|GraveSalt|off> [radius m = 20] [every s = once] [hit damage = 40]: sets an element's ")
		TEXT("status on every creature within the radius of the player, as a hit of that damage from the gun in hand (no roll), once or ")
		TEXT("again every so often."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ApplyCommand));
}
