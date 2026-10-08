#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

UCreatureVoiceComponent::UCreatureVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UCreatureVoiceComponent::FCries UCreatureVoiceComponent::CriesFor(const ACreatureBase& Creature)
{
	using namespace LooterSoundCue;
	// Most specific first: subclasses (the Gravemother, Abel) speak with their kind's voice, at their size's pitch.
	if (Creature.IsA<AUnpaidCreature>())
	{
		return { UnpaidAlert, UnpaidShriek, UnpaidHurt, UnpaidDeath };
	}
	if (Creature.IsA<ASlimeCreature>())
	{
		// A slime has no cry to hunt with: its hops say it's coming.
		return { NAME_None, SlimeAttack, SlimeHurt, SlimeDeath };
	}
	if (Creature.IsA<ASpiderCreature>())
	{
		return { SpiderAlert, SpiderAttack, SpiderHurt, SpiderDeath };
	}
	return {};
}

void UCreatureVoiceComponent::BeginPlay()
{
	Super::BeginPlay();
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!Creature)
	{
		return;
	}
	Cries = CriesFor(*Creature);
	if (UHealthComponent* Health = Creature->FindComponentByClass<UHealthComponent>())
	{
		Health->OnDamaged.AddDynamic(this, &UCreatureVoiceComponent::HandleDamaged);
		Health->OnDeath.AddDynamic(this, &UCreatureVoiceComponent::HandleDeath);
	}
}

void UCreatureVoiceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AlertTimer);
	}
	Super::EndPlay(EndPlayReason);
}

float UCreatureVoiceComponent::GetPitch() const
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	return LooterSoundRules::PitchForSize(Creature ? Creature->GetSizeScale() : 1.f);
}

void UCreatureVoiceComponent::Play(FName Cue, float VolumeScale) const
{
	const AActor* Owner = GetOwner();
	if (!Cue.IsNone() && Owner)
	{
		LooterSound::PlayAttached(Cue, Owner->GetRootComponent(), NAME_None, VolumeScale, GetPitch());
	}
}

void UCreatureVoiceComponent::HandleStateChanged(ECreatureState OldState, ECreatureState NewState)
{
	UWorld* World = GetWorld();
	if (!World || OldState == NewState)
	{
		return;
	}
	if (NewState == ECreatureState::Attack)
	{
		// The wind-up's cry is the attack's tell: heard before the strike, in time to step back.
		Play(Cries.Attack);
		return;
	}
	const bool bWasCalm = OldState == ECreatureState::Idle || OldState == ECreatureState::Wander || OldState == ECreatureState::Return;
	if (NewState == ECreatureState::Chase && bWasCalm && !Cries.Alert.IsNone() && World->GetTimeSeconds() >= NextAlert)
	{
		NextAlert = World->GetTimeSeconds() + AlertRest;
		World->GetTimerManager().SetTimer(AlertTimer, this, &UCreatureVoiceComponent::CryAlert, FMath::FRandRange(0.02f, AlertDelayMax), false);
	}
}

void UCreatureVoiceComponent::CryAlert()
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (Creature && !Creature->IsDead() && Creature->GetTarget())
	{
		Play(Cries.Alert);
	}
}

void UCreatureVoiceComponent::HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	const UWorld* World = GetWorld();
	const ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!World || !Creature)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	// The bullet going in, at the wound: a critical hit a little louder.
	if (Now >= NextHit)
	{
		NextHit = Now + HitInterval;
		LooterSound::PlayAt(this, LooterSoundCue::CreatureHit, HitLocation, bCritical ? 1.f : 0.8f, GetPitch());
	}
	// Its own cry now and then, never on the killing blow (the death cry follows at once).
	const UHealthComponent* Health = Creature->FindComponentByClass<UHealthComponent>();
	const bool bKilled = Health && Health->GetHealth() <= 0.f;
	if (!bKilled && !Cries.Hurt.IsNone() && Now >= NextHurtCry)
	{
		NextHurtCry = Now + HurtCryInterval * FMath::FRandRange(0.8f, 1.4f);
		Play(Cries.Hurt, bCritical ? 1.f : 0.85f);
	}
}

void UCreatureVoiceComponent::HandleDeath(AController* Killer)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AlertTimer);
	}
	Play(Cries.Death);
	// The player's own confirmation of the kill, heard flat whatever the distance.
	if (Killer && Killer->IsLocalPlayerController())
	{
		LooterSound::Play2D(Killer, LooterSoundCue::Kill);
	}
}
