#include "Elements/ElementStatusComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureBase.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementTuning.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

// UElementStatusComponent: setting statuses, dealing their damage, a smite's leap, the slow and the sounds. The looks are
// in ElementStatusComponentLooks.cpp.

namespace
{
	int32 IndexOf(ELooterElement Element)
	{
		return FMath::Clamp(static_cast<int32>(Element), 0, 3);
	}

	/**
	 * Tick sounds heard at once across every creature: a horde burning together crackles, it doesn't roar. The last one's
	 * time per element; world time, so a new play session starts with it long past.
	 */
	double LastTickCue[4] = { -1000.0, -1000.0, -1000.0, -1000.0 };
	constexpr double TickCueGap = 0.12;
	/** A status set again on one creature within this long sounds once (a rifle's stream of hits). */
	constexpr double ApplyCueGap = 0.25;
	constexpr float TickCueVolume = 0.55f;
}

int32 UElementStatusComponent::DealingDepth = 0;

UElementStatusComponent::UElementStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// After the creature has moved this frame, so its look sits on it.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

UElementStatusComponent* UElementStatusComponent::Find(const AActor* Target)
{
	return Target ? Target->FindComponentByClass<UElementStatusComponent>() : nullptr;
}

UElementStatusComponent* UElementStatusComponent::FindOrAdd(AActor& Target)
{
	if (UElementStatusComponent* Existing = Find(&Target))
	{
		return Existing;
	}
	UElementStatusComponent* Added = NewObject<UElementStatusComponent>(&Target, TEXT("ElementStatus"));
	Added->RegisterComponent();
	return Added;
}

bool UElementStatusComponent::TryApply(AActor& Target, ELooterElement Element, float Chance, float HitDamage, bool bCritical,
	float Roll, const FVector& HitLocation, AActor* Causer, AController* Instigator)
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(&Target);
	const UHealthComponent* Health = Target.FindComponentByClass<UHealthComponent>();
	if (Element == ELooterElement::None || !Creature || Creature->IsDead() || !Health || Health->IsDead() || HitDamage <= 0.f)
	{
		return false;
	}
	const FElementTuning Tuning = FElementTuning::Live();
	const EElementAffinity Affinity = ElementRules::Affinity(Element, ElementRules::TargetOf(&Target), bCritical);
	if (Roll >= ElementRules::HitChance(Chance, Affinity, Tuning))
	{
		return false;
	}
	FindOrAdd(Target)->Apply(Element, HitDamage, Affinity, HitLocation, Causer, Instigator);
	return true;
}

void UElementStatusComponent::Apply(ELooterElement Element, float HitDamage, EElementAffinity Affinity, const FVector& HitLocation,
	AActor* Causer, AController* Instigator)
{
	if (Element == ELooterElement::None || !IsOwnerAlive())
	{
		return;
	}
	const FElementTuning Tuning = FElementTuning::Live();
	const FElementTarget Target = ElementRules::TargetOf(GetOwner());
	const bool bWasSalted = Status.IsSalted();
	Status.Apply(Element, HitDamage, Affinity, Target.bBoss, Tuning);
	const int32 Index = IndexOf(Element);
	Causers[Index] = Causer;
	Instigators[Index] = Instigator;

	// Salt slows the living (never a boss: nothing new holds up a boss's moves); a slow already on it keeps the stronger.
	if (Element == ELooterElement::GraveSalt && !Target.bBoss && Tuning.GraveSaltSlow < 1.f)
	{
		UMovementSlowComponent::Apply(Cast<ACharacter>(GetOwner()), Tuning.GraveSaltSlow, Status.GetSaltTimeLeft());
	}

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastApplyCue[Index] >= ApplyCueGap || Now < LastApplyCue[Index] || (Element == ELooterElement::GraveSalt && !bWasSalted))
	{
		LastApplyCue[Index] = Now;
		PlayCue(Element, EElementCue::Apply, HitLocation, Affinity == EElementAffinity::Weak ? 1.f : 0.8f);
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s: %s set by a %.1f hit (%s)"), *GetNameSafe(GetOwner()), *ElementRules::Word(Element), HitDamage,
		Affinity == EElementAffinity::Weak ? TEXT("weak") : (Affinity == EElementAffinity::Resist ? TEXT("resists") : TEXT("neutral")));
	SetComponentTickEnabled(true);
}

void UElementStatusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UElementStatusComponent::Advance(float DeltaSeconds)
{
	// A death (by anything) ends every status: the look goes with the body's own death, nothing burns on a corpse.
	if (!IsOwnerAlive())
	{
		Status.Clear();
		SetComponentTickEnabled(false);
		return;
	}
	const FElementTuning Tuning = FElementTuning::Live();
	TArray<FElementTick> Ticks;
	Status.Advance(DeltaSeconds, Tuning, Ticks);
	for (const FElementTick& Tick : Ticks)
	{
		if (Deal(Tick))
		{
			PlayKillLook(Tick.Element);
			PlayCue(Tick.Element, EElementCue::Kill, BodyPoint(0.3f));
			Status.Clear();
			break;
		}
	}
	if (Status.IsActive())
	{
		PlayLooks(DeltaSeconds);
	}
	else
	{
		SetComponentTickEnabled(false);
	}
}

bool UElementStatusComponent::Deal(const FElementTick& Tick)
{
	AActor* Owner = GetOwner();
	if (!Owner || Tick.Damage <= 0.f)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (!Tick.bSmite)
	{
		// A burn's or the salt's piece: a crackle or a hiss on it, a horde's together heard as a few.
		const int32 Index = IndexOf(Tick.Element);
		if (Now - LastTickCue[Index] >= TickCueGap || Now < LastTickCue[Index])
		{
			LastTickCue[Index] = Now;
			PlayCue(Tick.Element, EElementCue::Tick, BodyPoint(0.2f), TickCueVolume);
		}
		TGuardValue<int32> Dealing(DealingDepth, DealingDepth + 1);
		return SendDamage(*Owner, Tick.Element, Tick.Damage);
	}

	// A smite: the light it gathered bursts, and leaps to the next of the dead within reach for a share of it.
	const FElementTuning Tuning = FElementTuning::Live();
	const FVector Heart = BodyPoint(0.35f);
	AActor* Next = Tuning.SoulLightChainShare > 0.f ? FindNextOfTheDead(World, Heart, Tuning.SoulLightChainRadius, Owner) : nullptr;
	FVector NextHeart = Next ? Next->GetActorLocation() : FVector::ZeroVector;
	PlaySmiteLook(Heart, Next ? &NextHeart : nullptr);
	PlayCue(ELooterElement::SoulLight, EElementCue::Tick, Heart);
	if (Next)
	{
		SendDamage(*Next, ELooterElement::SoulLight, Tick.Damage * Tuning.SoulLightChainShare);
	}
	return SendDamage(*Owner, ELooterElement::SoulLight, Tick.Damage);
}

bool UElementStatusComponent::SendDamage(AActor& Victim, ELooterElement Element, float Damage)
{
	UHealthComponent* Health = Victim.FindComponentByClass<UHealthComponent>();
	if (!Health || Health->IsDead() || Damage <= 0.f)
	{
		return false;
	}
	const int32 Index = IndexOf(Element);
	Dealt[Index] += Damage;
	// From the gun that set it, so that gun gets the notch (AWeaponBase::FindKillWeapon); its holder gets the XP. A gun gone
	// since (sold, dropped and lost) leaves the holder as the causer: the gun in their hand gets it then.
	AController* Instigator = Instigators[Index].Get();
	AActor* Causer = Causers[Index].Get();
	if (!Causer && Instigator)
	{
		Causer = Instigator->GetPawn();
	}
	// Somewhere on the body, a little apart each time, so the pieces' numbers don't stack into one.
	FVector Center;
	float Radius = 0.f;
	float HalfHeight = 0.f;
	GetBody(Center, Radius, HalfHeight);
	if (&Victim != GetOwner())
	{
		Center = Victim.GetActorLocation();
	}
	const FVector Where = Center + FVector(FMath::FRandRange(-0.5f, 0.5f) * Radius, FMath::FRandRange(-0.5f, 0.5f) * Radius,
		FMath::FRandRange(0.f, 0.5f) * HalfHeight);
	FHitResult Hit(&Victim, nullptr, Where, FVector::UpVector);
	Hit.ImpactPoint = Where;
	Hit.Location = Where;
	UGameplayStatics::ApplyPointDamage(&Victim, Damage, FVector::DownVector, Hit, Instigator, Causer, ElementRules::DamageTypeFor(Element));
	return Health->IsDead();
}

float UElementStatusComponent::GetDealt(ELooterElement Element) const
{
	return Dealt[IndexOf(Element)];
}

AActor* UElementStatusComponent::FindNextOfTheDead(const UWorld* World, const FVector& Center, float Radius, const AActor* Except)
{
	if (!World || Radius <= 0.f)
	{
		return nullptr;
	}
	AActor* Nearest = nullptr;
	double NearestSquared = FMath::Square(static_cast<double>(Radius));
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (Creature == Except || Creature->IsDead() || Creature->IsActorBeingDestroyed()
			|| !ElementRules::IsTheDead(ElementRules::BodyOf(Creature->GetClass())))
		{
			continue;
		}
		const double Squared = FVector::DistSquared(Creature->GetActorLocation(), Center);
		if (Squared <= NearestSquared)
		{
			NearestSquared = Squared;
			Nearest = Creature;
		}
	}
	return Nearest;
}

bool UElementStatusComponent::IsOwnerAlive() const
{
	const AActor* Owner = GetOwner();
	const ACreatureBase* Creature = Cast<ACreatureBase>(Owner);
	const UHealthComponent* Health = Owner ? Owner->FindComponentByClass<UHealthComponent>() : nullptr;
	return Owner && !Owner->IsActorBeingDestroyed() && Health && !Health->IsDead() && (!Creature || !Creature->IsDead());
}

void UElementStatusComponent::GetBody(FVector& OutCenter, float& OutRadius, float& OutHalfHeight) const
{
	const AActor* Owner = GetOwner();
	OutCenter = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	OutRadius = 40.f;
	OutHalfHeight = 60.f;
	if (Owner)
	{
		Owner->GetSimpleCollisionCylinder(OutRadius, OutHalfHeight);
		OutRadius = FMath::Max(OutRadius, 10.f);
		OutHalfHeight = FMath::Max(OutHalfHeight, 10.f);
	}
}

FVector UElementStatusComponent::BodyPoint(float Up) const
{
	FVector Center;
	float Radius = 0.f;
	float HalfHeight = 0.f;
	GetBody(Center, Radius, HalfHeight);
	return Center + FVector(0.f, 0.f, HalfHeight * Up);
}

FWeaponFX* UElementStatusComponent::Effects() const
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return nullptr;
	}
	Bullets->GetEffects().Initialize(World);
	return &Bullets->GetEffects();
}

void UElementStatusComponent::PlayCue(ELooterElement Element, EElementCue Event, const FVector& Where, float Volume) const
{
	const FName Cue = ElementRules::Cue(Element, Event);
	if (!Cue.IsNone())
	{
		LooterSound::PlayAt(this, Cue, Where, Volume);
	}
}
