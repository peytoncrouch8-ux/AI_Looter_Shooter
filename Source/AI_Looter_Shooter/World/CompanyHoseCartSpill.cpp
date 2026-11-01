// ACompanyHoseCart: the stop: the valve shut or the tank burst, the black spill and its smog, the missions told, the
// crew's next wave rising out of it, Vane's complaint from the loudspeaker.

#include "World/CompanyHoseCart.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLineSet.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "World/CompanyLoudspeaker.h"
#include "World/LamplighterEffects.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The burst's shake: felt this hard at the tank, dying over this long, out to this far (cm). */
	constexpr float CartBurstShake = 0.45f;
	constexpr float CartBurstShakeSeconds = 0.5f;
	constexpr float CartBurstShakeReach = 2500.f;
	/** The smog coughing up off the spill: this many puffs. */
	constexpr int32 CoughPuffs = 6;
}

bool ACompanyHoseCart::StopPumping(AController* By, bool bBurst)
{
	if (State != EHoseCartState::Pumping)
	{
		return false;
	}
	State = EHoseCartState::Stopped;
	bStoppedHere = true;
	bWasBurst = bBurst;
	SpillSpread = 0.f;
	LooterSound::Stop(PumpSound.Get(), 0.05f);
	PumpSound.Reset();

	const FVector Wheel = GetValveLocation();
	if (bBurst)
	{
		BurstTank(By);
	}
	else
	{
		// The wheel clanks home, the hose kicks off its nozzle with a snap.
		LooterSound::PlayAt(this, LooterSoundCue::AnchorSet, Wheel, 0.8f, 1.15f);
		LamplighterEffects::Sparks(this, Wheel, FVector::UpVector, 6, 0.7f);
	}
	LooterSound::PlayAt(this, LooterSoundCue::DredgerHoseSnap, GetActorTransform().TransformPosition(HoseEnd));
	CoughSmog();

	// The missions hear it about this cart (Main 2's "Stop the hose-cart"), then the crew's dead rise; Hob names them, and
	// Vane complains once he has (the cart's tick waits for quiet: Vane's radio would drop Hob's words).
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(StoppedEvent, this));
	}
	RaiseSpill();
	bComplainPending = !LoudspeakerTag.IsNone() && StopBroadcast != nullptr;
	ComplainWaited = 0.f;
	if (SpillLines && !SpillLines->Lines.IsEmpty())
	{
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			Captions->Play(SpillLines->Lines, ECaptionPlay::Interrupt);
		}
	}
	ShowState();
	UE_LOG(LogLooter, Log, TEXT("%s: stopped (%s)."), *GetActorNameOrLabel(), bBurst ? TEXT("its tank burst") : TEXT("its valve shut"));
	return true;
}

void ACompanyHoseCart::BurstTank(AController* By)
{
	const FVector Center = GetTankCenter();
	LooterSound::PlayAt(this, LooterSoundCue::DredgerTankBurst, Center);
	BossCameraShake::Kick(this, Center, CartBurstShake, CartBurstShakeSeconds, CartBurstShakeReach);

	// The souls it drank let go at once (the hose-man's tank's look: DredgerHoseManTank.cpp): a cyan blast, a ring, torn
	// souls and smog rolling off.
	if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		FVfxExplosion Blast;
		Blast.Center = Center;
		Blast.Up = FVector::UpVector;
		Blast.Radius = FMath::Max(BurstRadius, 150.f);
		Blast.Fire = DredgerLook::SoulCyan;
		Blast.bSoul = true;
		Blast.Ground = EImpactMaterial::Unknown;
		Blast.bOnGround = false;
		Effects.SpawnExplosion(Blast);
		Effects.SpawnShockRing(Center, FVector::UpVector, 30.f, Blast.Radius * 1.2f, 18.f, DredgerLook::SoulCyan, 6.f, 0.45f);
		Effects.SpawnSoulTatters(Center, FVector::UpVector, DredgerLook::SoulCyan, 90.f, 12);
	}

	// Whoever stands at it is hurt (its crew: the hose-man at his cart); the player is spared, it was their shot.
	UWorld* World = GetWorld();
	if (!World || BurstRadius <= 0.f)
	{
		return;
	}
	const double ReachSquared = FMath::Square(static_cast<double>(BurstRadius));
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Other = *It;
		if (!Other || Other->IsPlayerControlled() || FVector::DistSquared(Other->GetActorLocation(), Center) > ReachSquared)
		{
			continue;
		}
		const UHealthComponent* OtherHealth = Other->FindComponentByClass<UHealthComponent>();
		if (OtherHealth && !OtherHealth->IsDead())
		{
			UGameplayStatics::ApplyDamage(Other, OtherHealth->GetMaxHealth() * BurstShare, By, this, UDamageType::StaticClass());
		}
	}
}

void ACompanyHoseCart::CoughSmog()
{
	// Black smog coughing up off the spill as what the hose drank comes out of it.
	const FVector Ground = GetActorTransform().TransformPosition(FVector(40.0, 0.0, 30.0));
	for (int32 Puff = 0; Puff < CoughPuffs; ++Puff)
	{
		const FVector Drift(FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(-90.f, 90.f), FMath::FRandRange(70.f, 150.f));
		LamplighterEffects::SmokePuff(this, Ground + FVector(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), 0.0), Drift);
	}
}

void ACompanyHoseCart::RaiseSpill()
{
	if (CrewSpawnerId.IsNone())
	{
		return;
	}
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	AEncounterSpawner* Crew = Encounters ? Encounters->FindSpawner(CrewSpawnerId) : nullptr;
	if (!Crew)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no encounter %s for the spill's dead."), *GetActorNameOrLabel(), *CrewSpawnerId.ToString());
		return;
	}
	// Stopped before its crew had even come out (the player crept up unseen): the crew first, at their own spots.
	if (Crew->GetWavesStarted() == 0)
	{
		Crew->TriggerWave();
	}
	// The next wave rises at the spill: the spots relative to the cart, as the spawner keeps its own (relative to it).
	if (SpillSpots.Num() > 0)
	{
		const FTransform Cart = GetActorTransform();
		const FTransform Spawner = Crew->GetActorTransform();
		TArray<FVector> Spots;
		Spots.Reserve(SpillSpots.Num());
		for (const FVector& Spot : SpillSpots)
		{
			Spots.Add(Spawner.InverseTransformPositionNoScale(Cart.TransformPositionNoScale(Spot)));
		}
		Crew->SpawnPoints = MoveTemp(Spots);
	}
	if (!Crew->TriggerWave())
	{
		UE_LOG(LogLooter, Log, TEXT("%s: the spill brings no wave from %s (%s)."), *GetActorNameOrLabel(), *CrewSpawnerId.ToString(),
			*Crew->Describe());
	}
}

void ACompanyHoseCart::Complain()
{
	if (LoudspeakerTag.IsNone() || !StopBroadcast)
	{
		return;
	}
	// A loudspeaker shot out before the valve shut says nothing: the player already shut him up.
	if (ACompanyLoudspeaker* Horn = ACompanyLoudspeaker::FindTagged(this, LoudspeakerTag))
	{
		Horn->Broadcast(StopBroadcast->Lines);
	}
}
