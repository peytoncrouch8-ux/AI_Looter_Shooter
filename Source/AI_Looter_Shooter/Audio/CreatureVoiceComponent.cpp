#include "Audio/CreatureVoiceComponent.h"
#include "Audio/CreatureVoiceDirector.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"
#include "Weapons/BulletMarks.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"

UCreatureVoiceComponent::UCreatureVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UCreatureVoiceComponent::FCries UCreatureVoiceComponent::CriesFor(const ACreatureBase& Creature)
{
	using namespace LooterSoundCue;
	// Most specific first: subclasses (the Gravemother, Abel) speak with their kind's voice, at their size's pitch.
	// The Unpaid have no idle call: they mutter lines instead (their barks).
	// The Providence's dead miners (and the Foreman, one of them) have their own heave at the wind-up, so no Unpaid shriek goes
	// on top of it; their alert, hurt, death and bullet-hit voices are the Unpaid's. Before the Unpaid's, which they are one of.
	if (Creature.IsA<ADeadMinerCreature>())
	{
		return { UnpaidAlert, NAME_None, UnpaidHurt, UnpaidDeath, UnpaidHit, NAME_None };
	}
	if (Creature.IsA<AUnpaidCreature>())
	{
		return { UnpaidAlert, UnpaidShriek, UnpaidHurt, UnpaidDeath, UnpaidHit, NAME_None };
	}
	if (Creature.IsA<ASlimeCreature>())
	{
		// A slime has no cry to hunt with: its hops say it's coming.
		return { NAME_None, SlimeAttack, SlimeHurt, SlimeDeath, SlimeHit, Voice::SlimeIdle };
	}
	// The Spitter is a spider with its own hurt and death (its tell has its own hiss, so its attack cry stays the spider's).
	if (Creature.IsA<ASpitterSpider>())
	{
		return { SpiderAlert, SpiderAttack, LooterSoundCue::Spitter::Hurt, LooterSoundCue::Spitter::Death, SpiderHit, Voice::SpiderIdle };
	}
	if (Creature.IsA<ASpiderCreature>())
	{
		return { SpiderAlert, SpiderAttack, SpiderHurt, SpiderDeath, SpiderHit, Voice::SpiderIdle };
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
	// Its own voice, the same for the same creature every time; and its own dice for its barks.
	VoicePitch = CreatureBarks::VoicePitchFor(Creature->GetFName());
	Random.Initialize(static_cast<int32>(GetTypeHash(Creature->GetFName().ToString())));
	// The level's director hears of it: its idle mutters and calls, its pack's last words.
	if (UCreatureVoiceDirector* Director = UCreatureVoiceDirector::Get(this))
	{
		Director->Register(this);
	}
}

void UCreatureVoiceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(AlertTimer);
		Timers.ClearTimer(BarkTimer);
		Timers.ClearTimer(MurmurTimer);
	}
	if (UCreatureVoiceDirector* Director = UCreatureVoiceDirector::Get(this))
	{
		Director->Unregister(this);
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
	// An Unpaid that has seen the player may say so, once its wail has risen.
	if (NewState == ECreatureState::Chase && bWasCalm)
	{
		TryBark(ECreatureBark::Spot);
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
	// The bullet going in, at the wound, sounding like what it hits (a spider's shell, a slime's jelly, a ghost): a
	// critical hit a little louder.
	if (Now >= NextHit)
	{
		NextHit = Now + HitInterval;
		LooterSound::PlayAt(this, Cries.Hit, HitLocation, bCritical ? 1.f : 0.8f, GetPitch());
	}
	PlayHitLayers(Damage, bCritical, HitLocation, DamageCauser, *Creature);
	// A critical kill's confirm (HandleDeath) asks whether the blow that dealt it was one.
	LastHitTime = Now;
	bLastHitCritical = bCritical;
	// Its own cry now and then, never on the killing blow (the death cry follows at once).
	const UHealthComponent* Health = Creature->FindComponentByClass<UHealthComponent>();
	const bool bKilled = Health && Health->GetHealth() <= 0.f;
	if (!bKilled && !Cries.Hurt.IsNone() && Now >= NextHurtCry)
	{
		NextHurtCry = Now + HurtCryInterval * FMath::FRandRange(0.8f, 1.4f);
		Play(Cries.Hurt, bCritical ? 1.f : 0.85f);
		// Now and then a word after the cry (an Unpaid's).
		TryBark(ECreatureBark::Hurt);
	}
}

void UCreatureVoiceComponent::HandleDeath(AController* Killer)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(AlertTimer);
		Timers.ClearTimer(BarkTimer);
	}
	PendingBark.Reset();
	Play(Cries.Death);
	// Whatever it was saying stops; it may have last words, and a packmate near it may answer its fall.
	StopMurmur();
	TryBark(ECreatureBark::Death);
	if (UCreatureVoiceDirector* Director = UCreatureVoiceDirector::Get(this))
	{
		Director->NotifyDeath(*this);
	}
	// The player's own confirmation of the kill, heard flat whatever the distance; a critical blow's is brighter (a status
	// finishing it long after the last bullet is a plain kill).
	if (Killer && Killer->IsLocalPlayerController())
	{
		const UWorld* World = GetWorld();
		const bool bCriticalKill = bLastHitCritical && World && World->GetTimeSeconds() - LastHitTime <= KillCritWindow;
		const ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(this);
		const bool bBankHasCrit = Sounds && Sounds->HasSounds(LooterSoundCue::Gunfeel::KillCrit);
		LooterSound::Play2D(Killer, GunfeelSound::KillConfirmCue(bCriticalKill, bBankHasCrit));
	}
}

void UCreatureVoiceComponent::PlayHitLayers(float Damage, bool bCritical, const FVector& HitLocation, const AActor* DamageCauser,
	const ACreatureBase& Creature)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();

	// The gun that dealt it (a status's damage has none: no layers, only the plain hit).
	const AWeaponBase* Gun = Cast<const AWeaponBase>(DamageCauser);
	const UWeaponDefinition* Definition = Gun ? Gun->GetInstance().Definition : nullptr;
	if (Definition)
	{
		const EWeaponKind Kind = Definition->Kind;
		bool bHeavy = GunfeelSound::IsHeavy(FBulletMarks::GunScale(Kind), GunfeelSound::CreatureHeavyFrom());
		// A shotgun's pellets each take little; together they are one blow, thumped once when they add up to a heavy share.
		if (Kind == EWeaponKind::Shotgun)
		{
			const UHealthComponent* Health = Creature.FindComponentByClass<UHealthComponent>();
			const float MaxHealth = Health ? Health->GetMaxHealth() : 0.f;
			bHeavy |= MaxHealth > 0.f && GunfeelSound::BlastIsHeavy(Blast, Now, Damage / MaxHealth, GunfeelSound::CreatureBlastWindow(),
				GunfeelSound::CreatureHeavyShare());
		}
		const float Volume = GunfeelSound::CreatureHeavyVolume();
		if (bHeavy && Volume > 0.f && Now >= NextHeavy)
		{
			NextHeavy = Now + HitInterval;
			LooterSound::PlayAt(this, LooterSoundCue::Gunfeel::CreatureHitHeavy, HitLocation, Volume, GetPitch());
		}
	}

	// A critical cracks what the body is made of: a shell, jelly, coal.
	const FName Crack = bCritical ? GunfeelSound::CritCueFor(CreatureBlood::Of(&Creature)) : FName(NAME_None);
	const float CritVolume = GunfeelSound::CreatureCritVolume();
	if (!Crack.IsNone() && CritVolume > 0.f && Now >= NextCrit)
	{
		NextCrit = Now + HitInterval;
		LooterSound::PlayAt(this, Crack, HitLocation, CritVolume, GetPitch());
	}
}
