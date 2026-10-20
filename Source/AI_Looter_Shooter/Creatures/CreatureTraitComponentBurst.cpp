// UCreatureTraitComponent: Grave-burst. Dead, the body swells for a second (it grows and shivers, its heart glows brighter,
// its ring pulses on the ground faster and faster, the swell's rising sound): the tell to step out of the ring. Then it
// bursts: soul-light and its own body's burst, a ring blown out over the ground, players in reach hurt (by distance to their
// nearest side, never through a wall), knocked away and jolted; the corpse is gone in the blast.

#include "Creatures/CreatureTraitComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Embers/Powers/EmberFX.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 RingPoints = 36;
	/** The blast's ring runs out over the ground in this long (s). */
	constexpr float BlastRingSeconds = 0.3f;

	/** Every player's pawn the burst could catch: the game's players, and a test's stand-in (UEncounterSubsystem). */
	TArray<APawn*, TInlineAllocator<4>> PlayersOf(UWorld& World, const UObject* Context)
	{
		TArray<APawn*, TInlineAllocator<4>> Players;
		for (FConstPlayerControllerIterator It = World.GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* Player = It->Get();
			if (APawn* Pawn = Player ? Player->GetPawn() : nullptr)
			{
				Players.AddUnique(Pawn);
			}
		}
		if (const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(Context))
		{
			if (APawn* Pawn = Encounters->GetPlayer())
			{
				Players.AddUnique(Pawn);
			}
		}
		return Players;
	}
}

void UCreatureTraitComponent::StartSwell()
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	if (!Creature || !World || bSwelling || bBurst)
	{
		return;
	}
	const FGraveBurstTraitTuning& Tuning = UCreatureTraitSettings::Get().GraveBurst;
	bSwelling = true;
	SwellTime = 0.f;
	BurstAge = 1000.f;
	LastBurst.Reset();
	if (USkeletalMeshComponent* Body = Creature->GetMesh())
	{
		BaseMeshScale = Body->GetRelativeScale3D();
	}
	// Its reach laid on the ground once, where it lies (a corpse barely moves).
	BurstRing = EmberFX::GroundRing(*World, Feet(), Tuning.Radius, RingPoints, 250.f, 18.f);
	LooterSound::PlayAttached(LooterSoundCue::Tier::TraitSwell, Creature->GetRootComponent(), NAME_None, 1.f, LooterSoundRules::PitchForSize(Creature->GetSizeScale()));
	UpdateTickEnabled();
	UE_LOG(LogLooter, Verbose, TEXT("%s swells (Grave-burst): bursts in %.1f s, %.0f cm round"), *Creature->GetName(), Tuning.SwellSeconds, Tuning.Radius);
}

void UCreatureTraitComponent::CancelSwell()
{
	if (!bSwelling)
	{
		return;
	}
	bSwelling = false;
	ACreatureBase* Creature = GetCreature();
	if (USkeletalMeshComponent* Body = Creature ? Creature->GetMesh() : nullptr)
	{
		Body->SetRelativeScale3D(BaseMeshScale);
	}
}

void UCreatureTraitComponent::TickSwell(float DeltaSeconds, FWeaponFX* Effects)
{
	const FGraveBurstTraitTuning& Tuning = UCreatureTraitSettings::Get().GraveBurst;
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	ACreatureBase* Creature = GetCreature();
	if (!Creature)
	{
		return;
	}
	// After the burst: its ring blown out over the ground.
	if (BurstAge < BlastRingSeconds)
	{
		BurstAge += DeltaSeconds;
		if (Effects)
		{
			const float Share = FMath::Clamp(BurstAge / BlastRingSeconds, 0.f, 1.f);
			EmberFX::DrawRing(*Effects, BurstCenter, FVector::UpVector, FMath::Max(Tuning.Radius * (0.4f + 0.8f * Share), 10.f), 40, 6.f, Color, 16.f * (1.f - Share));
		}
	}
	else if (BurstAge < 1000.f)
	{
		BurstAge = 1000.f;
		UpdateTickEnabled();
	}
	if (!bSwelling)
	{
		return;
	}
	SwellTime += DeltaSeconds;
	const float Seconds = FMath::Max(Tuning.SwellSeconds, 0.05f);
	const float Swell = FMath::Clamp(SwellTime / Seconds, 0.f, 1.f);
	// Its pulse quickens as it nears bursting: twice a second at first, ten times by the end.
	const float Beat = 0.5f + 0.5f * FMath::Sin(Clock * UE_TWO_PI * (2.f + 8.f * Swell));

	// The body grows, shivering harder toward the end.
	if (USkeletalMeshComponent* Body = Creature->GetMesh())
	{
		const float Grow = 1.f + (Tuning.SwellScale - 1.f) * Swell * Swell;
		const float Shiver = 1.f + 0.03f * Swell * FMath::Sin(Clock * UE_TWO_PI * 22.f);
		Body->SetRelativeScale3D(BaseMeshScale * (Grow * Shiver));
	}
	if (Effects)
	{
		const FVector Center = BodyCenter();
		const float Radius = BodyRadius();
		// Its heart glowing brighter and wider in time with the pulse...
		Effects->AddGlow(Center, Radius * (0.6f + 1.1f * Swell) * (0.85f + 0.3f * Beat), Color, 5.f + 20.f * Swell * (0.6f + 0.4f * Beat));
		// ...its reach on the ground pulsing with it, brighter each beat...
		EmberFX::DrawLine(*Effects, BurstRing, true, 5.f + 3.f * Swell, Color, (2.5f + 9.f * Swell) * (0.5f + 0.5f * Beat));
		// ...and motes drawn in from the ring to the body.
		if (BurstRing.Num() > 0 && Random.FRand() < 0.6f)
		{
			const FVector& From = BurstRing[Random.RandRange(0, BurstRing.Num() - 1)];
			Sparks.Spawn(From, (Center - From) * Random.FRandRange(1.8f, 2.4f), 0.4f, 3.f, Color, 10.f, 0.f, 0.f);
		}
	}
	if (SwellTime >= Seconds)
	{
		Burst();
	}
}

void UCreatureTraitComponent::Burst()
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	CancelSwell();
	if (!Creature || !World)
	{
		return;
	}
	const FGraveBurstTraitTuning& Tuning = UCreatureTraitSettings::Get().GraveBurst;
	const FLinearColor Color = FLinearColor::FromSRGBColor(Tuning.Color);
	bBurst = true;
	BurstAge = 0.f;
	const FVector Center = BodyCenter();
	BurstCenter = Feet() + FVector(0.0, 0.0, 15.0);

	// Seen and heard first, then the hurt (as the grave salt's burst is).
	if (FWeaponFX* Effects = GetEffects())
	{
		const float Size = FMath::Max(Creature->GetSizeScale(), 0.3f);
		Effects->SpawnFlash(Center, FVector::UpVector, 2.5f * Size);
		Effects->SpawnDeathBurst(EDeathBurst::SoulLight, Center, FVector::UpVector, Tuning.Radius * 0.8f, Color);
		const EDeathBurst Own = UCreatureHitReactionComponent::DeathBurstOf(*Creature);
		if (Own != EDeathBurst::None && Own != EDeathBurst::SoulLight)
		{
			Effects->SpawnDeathBurst(Own, Center, FVector::UpVector, 130.f * Size, UCreatureHitReactionComponent::DeathBurstTint(*Creature));
		}
		for (int32 Index = 0; Index < 10; ++Index)
		{
			const float Angle = UE_TWO_PI * Index / 10.f;
			const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			Effects->SpawnDustPuff(BurstCenter + Out * 30.f, Out * Random.FRandRange(380.f, 520.f) + FVector(0.0, 0.0, 40.0), EmberFX::DustColor(Creature), 0.45f,
				30.f, Random.FRandRange(120.f, 170.f), Random.FRandRange(0.8f, 1.2f));
		}
	}
	LooterSound::PlayAt(Creature, LooterSoundCue::Tier::TraitBurst, Center, 1.f, LooterSoundRules::PitchForSize(Creature->GetSizeScale()));
	HurtWithBurst(Center);
	KickViews(Center);
	// Nothing left of the body to lie there: it's hidden until it comes back (or goes, if spawned in play).
	Creature->SetActorHiddenInGame(true);
	UpdateTickEnabled();
	UE_LOG(LogLooter, Log, TEXT("%s bursts (Grave-burst): %d caught"), *Creature->GetName(), LastBurst.Num());
}

void UCreatureTraitComponent::HurtWithBurst(const FVector& Center)
{
	ACreatureBase* Creature = GetCreature();
	UWorld* World = GetWorld();
	if (!Creature || !World)
	{
		return;
	}
	const FGraveBurstTraitTuning& Tuning = UCreatureTraitSettings::Get().GraveBurst;
	// Its attack as it was (its level and tier counted; any rage it died in already taken back).
	const float Heart = FMath::Max(Creature->AttackDamage, 0.f) * Tuning.DamageShare;
	AController* Instigator = Creature->GetController();
	for (APawn* Pawn : PlayersOf(*World, this))
	{
		if (!Pawn)
		{
			continue;
		}
		const FGraveSaltBody Body = GraveSaltBurst::BodyOf(*Pawn);
		const float Distance = FGraveSaltRules::DistanceToBody(Center, Body);
		const float Share = CreatureTraitRules::BurstShare(Distance, Tuning);
		if (Share <= 0.f || !GraveSaltBurst::CanSee(*World, Center, Body, Pawn, Creature))
		{
			continue;
		}
		const float Damage = Heart * Share;
		FCreatureBurstVictim& Victim = LastBurst.AddDefaulted_GetRef();
		Victim.Actor = Pawn;
		Victim.Damage = Damage;
		Victim.Distance = Distance;
		if (Damage > 0.f)
		{
			UGameplayStatics::ApplyDamage(Pawn, Damage, Instigator, Creature, UCreatureAttackDamageType::StaticClass());
		}
		// Thrown out of it, as a heavy hit shoves them, harder nearer the heart.
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			FVector Away = Character->GetActorLocation() - Center;
			Away.Z = 0.0;
			if (!Away.Normalize())
			{
				Away = Creature->GetActorForwardVector();
			}
			Character->LaunchCharacter(Away * (Tuning.KnockSpeed * Share) + FVector(0.0, 0.0, Tuning.KnockLift * Share), true, false);
		}
	}
}

void UCreatureTraitComponent::KickViews(const FVector& Center) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const float Reach = UCreatureTraitSettings::Get().GraveBurst.KickRadius;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Player = It->Get();
		const APawn* Pawn = Player && Player->IsLocalController() ? Player->GetPawn() : nullptr;
		if (!Pawn || Reach <= 0.f)
		{
			continue;
		}
		const float Share = 1.f - FMath::Clamp(static_cast<float>(FVector::Dist(Pawn->GetActorLocation(), Center)) / Reach, 0.f, 1.f);
		if (Share <= 0.f)
		{
			continue;
		}
		// A thump through the view: up and a lean, the view widening a little; a short shake under it.
		FViewKick Thump;
		Thump.Pitch = 1.3f * Share;
		Thump.Roll = (FMath::FRand() < 0.5f ? -0.7f : 0.7f) * Share;
		Thump.FieldOfView = 0.025f * Share;
		Thump.Frequency = 6.f;
		Thump.Damping = 0.45f;
		UCameraShakeModifier::Kick(Player, Thump);
		if (UCameraShakeModifier* Modifier = UCameraShakeModifier::FindOrAdd(Player))
		{
			Modifier->AddShake(0.5f * Share, 0.35f);
		}
	}
}
