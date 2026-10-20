// UEmberComponent: whose powers these are. The player's character plays the session's hero (the hero select's pick, or
// Looter.Hero's with no session: USessionSubsystem::FindPlayedHero); anything else the Blueprint's HeroDefinition, else
// Ellis's. One power object is made per owned power with code; a power still waiting for its code is owned (the page
// lists it) but can't fire.

#include "Embers/EmberComponent.h"
#include "AI_Looter_Shooter.h"
#include "Core/LooterCharacter.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Embers/HeroDefinition.h"
#include "Session/SessionSubsystem.h"

const UHeroDefinition* UEmberComponent::FindHeroToBind() const
{
	// The player's body plays the hero picked; a test's bare actor, or a world with no sessions (a test level), keeps its own.
	if (Cast<ALooterCharacter>(GetOwner()))
	{
		if (const UHeroDefinition* Played = USessionSubsystem::FindPlayedHero(this))
		{
			return Played;
		}
	}
	if (HeroDefinition)
	{
		return HeroDefinition;
	}
	return LoadObject<UHeroDefinition>(nullptr, DefaultHeroPath(), nullptr, LOAD_NoWarn);
}

void UEmberComponent::BindHero()
{
	if (Hero)
	{
		return;
	}
	Hero = FindHeroToBind();
	if (!Hero)
	{
		UE_LOG(LogLooter, Warning, TEXT("Embers: no hero (%s is missing: run Tools/Unreal/create_ember_assets.py)"), DefaultHeroPath());
		return;
	}

	// One power object per owned power, by the hero's index; a power with no class (or an abstract one) gets none and can't fire.
	const TArray<TObjectPtr<UEmberDefinition>>& Owned = Hero->Powers;
	Powers.Reset();
	Powers.SetNum(Owned.Num());
	for (int32 Index = 0; Index < Owned.Num(); ++Index)
	{
		const UEmberDefinition* Def = Owned[Index];
		UClass* PowerClass = Def ? Def->PowerClass.Get() : nullptr;
		if (!PowerClass || PowerClass->HasAnyClassFlags(CLASS_Abstract))
		{
			UE_LOG(LogLooter, Warning, TEXT("Embers: %s has no power class; it can't be used"), Def ? *Def->Id.ToString() : TEXT("(an empty slot)"));
			continue;
		}
		Powers[Index] = NewObject<UEmberPower>(this, PowerClass);
	}

	// A new game equips the hero's first (a saved session puts its own back afterwards: RestoreEmbers).
	EquippedIndex = Owned.Num() > 0 ? 0 : INDEX_NONE;
	ActiveIndex = INDEX_NONE;
	State = EEmberState::Ready;
	CooldownLeft = CooldownTotal = ActiveLeft = ActiveTotal = 0.f;
	SetComponentTickEnabled(false);
	UE_LOG(LogLooter, Log, TEXT("Embers: %s with %d power(s), %s equipped"), *Hero->Id.ToString(), Owned.Num(), *GetEquippedId().ToString());
	OnEquipped.Broadcast(GetEquippedId());
}

void UEmberComponent::RebindHero()
{
	// A running power leaves the world as it found it before its hero goes; the new hero starts fresh, their first equipped.
	Interrupt();
	Hero = nullptr;
	BindHero();
	OnStateChanged.Broadcast(State);
}
