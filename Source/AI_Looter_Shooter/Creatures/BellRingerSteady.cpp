#include "Creatures/BellRingerSteady.h"
#include "Creatures/BellRingerRing.h"
#include "Creatures/CreatureBase.h"
#include "Components/CapsuleComponent.h"

namespace
{
	/** The aura's ring round a creature's feet: its body's width and a little more (cm at size 1). */
	constexpr float AuraRadius = 55.f;
}

UBellRingerSteady::UBellRingerSteady()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

FLinearColor UBellRingerSteady::AuraColor()
{
	// The Unpaid's rim light, a little warmer: soul-light, no rarity color.
	return FLinearColor::FromSRGBColor(FColor(0xDC, 0xEC, 0xE6));
}

UBellRingerSteady* UBellRingerSteady::Find(const ACreatureBase& Creature)
{
	return Creature.FindComponentByClass<UBellRingerSteady>();
}

UBellRingerSteady* UBellRingerSteady::Apply(ACreatureBase& Creature, float Seconds, float InSpeedScale, float InCooldownScale)
{
	if (Creature.IsDead() || Seconds <= 0.f)
	{
		return nullptr;
	}
	UBellRingerSteady* Steady = Find(Creature);
	if (Steady && Steady->bApplied)
	{
		// Again before it ran out: it lasts longer, it doesn't quicken twice.
		Steady->TimeLeft = FMath::Max(Steady->TimeLeft, Seconds);
		return Steady;
	}
	Steady = NewObject<UBellRingerSteady>(&Creature, NAME_None, RF_Transient);
	Creature.AddInstanceComponent(Steady);
	Steady->RegisterComponent();
	Steady->Begin(Seconds, InSpeedScale, InCooldownScale);
	return Steady;
}

void UBellRingerSteady::Begin(float Seconds, float InSpeedScale, float InCooldownScale)
{
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!Creature)
	{
		return;
	}
	SpeedScale = FMath::Max(InSpeedScale, 0.01f);
	CooldownScale = FMath::Max(InCooldownScale, 0.01f);
	TimeLeft = Seconds;
	Creature->ChaseSpeed *= SpeedScale;
	Creature->AttackCooldown *= CooldownScale;
	bApplied = true;
	const float HalfHeight = Creature->GetCapsuleComponent() ? Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
	Aura = ABellRingerRing::SpawnAura(Creature, HalfHeight, AuraRadius * Creature->GetSizeScale(), AuraColor());
}

void UBellRingerSteady::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UBellRingerSteady::Advance(float DeltaSeconds)
{
	if (!bApplied)
	{
		return;
	}
	TimeLeft -= DeltaSeconds;
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (TimeLeft <= 0.f || !Creature || Creature->IsDead())
	{
		End();
	}
}

void UBellRingerSteady::PutPaceBack()
{
	if (!bApplied)
	{
		return;
	}
	bApplied = false;
	TimeLeft = 0.f;
	if (ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner()))
	{
		Creature->ChaseSpeed /= SpeedScale;
		Creature->AttackCooldown /= CooldownScale;
	}
	if (ABellRingerRing* Ring = Aura.Get())
	{
		Ring->Finish();
	}
	Aura.Reset();
}

void UBellRingerSteady::End()
{
	PutPaceBack();
	DestroyComponent();
}

void UBellRingerSteady::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Taken away mid-quickening (the creature removed, the level closing): its pace goes back all the same.
	PutPaceBack();
	Super::EndPlay(EndPlayReason);
}
