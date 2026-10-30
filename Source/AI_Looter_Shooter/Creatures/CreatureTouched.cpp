#include "Creatures/CreatureTouched.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "UObject/Class.h"

ELooterElement CreatureTouched::ElementFor(const UClass* Kind)
{
	if (!Kind)
	{
		return ELooterElement::None;
	}
	// The kinds built on the Unpaid first: each of the dead has its own touch.
	if (Kind->IsChildOf(ABellRingerCreature::StaticClass()))
	{
		return ELooterElement::SoulLight;
	}
	if (Kind->IsChildOf(ADeadMinerCreature::StaticClass()))
	{
		return ELooterElement::Rot;
	}
	if (Kind->IsChildOf(AUnpaidCreature::StaticClass()))
	{
		return ELooterElement::GraveSalt;
	}
	if (Kind->IsChildOf(ASlimeCreature::StaticClass()))
	{
		return ELooterElement::Thunder;
	}
	if (Kind->IsChildOf(ASpiderCreature::StaticClass()))
	{
		return ELooterElement::Ember;
	}
	return ELooterElement::None;
}

ELooterElement CreatureTouched::CounterOf(ELooterElement Element)
{
	switch (Element)
	{
	case ELooterElement::Ember: return ELooterElement::GraveSalt;
	case ELooterElement::Thunder: return ELooterElement::GraveSalt;
	case ELooterElement::Rot: return ELooterElement::Ember;
	case ELooterElement::GraveSalt: return ELooterElement::Thunder;
	case ELooterElement::SoulLight: return ELooterElement::Rot;
	default: return ELooterElement::None;
	}
}

FString CreatureTouched::Word(ELooterElement Element)
{
	// The player's names (ElementRules::Word), in the tag's case: "LV 3 Restless Cinder-touched Brown Spider".
	switch (Element)
	{
	case ELooterElement::Ember: return TEXT("Cinder-touched");
	case ELooterElement::SoulLight: return TEXT("Hallow-touched");
	case ELooterElement::GraveSalt: return TEXT("Wardbreak-touched");
	case ELooterElement::Thunder: return TEXT("Thunder-touched");
	case ELooterElement::Rot: return TEXT("Rot-touched");
	default: return FString();
	}
}

bool CreatureTouched::Rolls(float Roll, float Chance, int32 AliveNow, int32 MaxAlive)
{
	return Chance > 0.f && Roll < Chance && AliveNow < MaxAlive;
}
