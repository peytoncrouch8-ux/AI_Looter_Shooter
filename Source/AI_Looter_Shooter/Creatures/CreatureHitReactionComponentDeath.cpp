// UCreatureHitReactionComponent (CreatureHitReactionComponent.h): its death: the kind's burst, the killer's punch and the
// corpse knocked along the killing shot (an overkill's farther, a blast's lifted: HITS). Split from
// CreatureHitReactionComponent.cpp (the hits) to keep each under the size guide.

#include "Creatures/CreatureHitReactionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Player/CameraShakeModifier.h"
#include "Player/ViewKick.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/GunKindsTuning.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Chitin as the burst's plates show it: the spider's brown hide, darker than its lit back. */
	const FColor SpiderChitin(0x4E, 0x34, 0x1E);
	/** Slime.py's gel tint. */
	const FColor SlimeGel(0xB8, 0xF0, 0x88);

	/** A revolver's crit kill bursts at the crit spot only if that's this near the body's middle (cm at size 1). */
	constexpr float CritSpotReach = 250.f;

	/** A boss (as CreatureHitReactionComponent.cpp's): its show runs its death. */
	bool IsBoss(const ACreatureBase& Creature)
	{
		return Creature.GetRank() == ECreatureRank::Boss || Creature.FindComponentByClass<UBossComponent>() != nullptr;
	}
}

EDeathBurst UCreatureHitReactionComponent::DeathBurstOf(const ACreatureBase& Creature)
{
	if (Creature.IsA<AUnpaidCreature>())
	{
		return EDeathBurst::SoulLight;
	}
	if (Creature.IsA<ASlimeCreature>())
	{
		return EDeathBurst::Gel;
	}
	if (Creature.IsA<ASpiderCreature>())
	{
		return EDeathBurst::Shell;
	}
	return EDeathBurst::None;
}

FLinearColor UCreatureHitReactionComponent::DeathBurstTint(const ACreatureBase& Creature)
{
	if (const AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(&Creature))
	{
		return Unpaid->GetCoalColor();
	}
	if (Creature.IsA<ASlimeCreature>())
	{
		return FLinearColor::FromSRGBColor(SlimeGel);
	}
	return FLinearColor::FromSRGBColor(SpiderChitin);
}

FName UCreatureHitReactionComponent::DeathBurstCue(EDeathBurst Kind)
{
	switch (Kind)
	{
	case EDeathBurst::Shell: return LooterSoundCue::SpiderBurst;
	case EDeathBurst::Gel: return LooterSoundCue::SlimeSplat;
	case EDeathBurst::SoulLight: return LooterSoundCue::UnpaidDissolve;
	default: return NAME_None;
	}
}

void UCreatureHitReactionComponent::HandleDeath(AController* Killer)
{
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!Creature || IsBoss(*Creature))
	{
		return;
	}
	const FGunfeelReactTuning Tuning = FGunfeelReactTuning::Live();
	if (BurstsLikeItsKind(LastHit))
	{
		SpawnDeathBurst(*Creature, Tuning);
	}

	// The killer's view punches in: a crit kill, or a Gravebound or worse, harder.
	APlayerController* Player = Cast<APlayerController>(Killer);
	if (Player && Player->IsLocalController())
	{
		const bool bHeavy = LastHit.bCritical || LooterRanks::IsAtLeast(Creature->GetRank(), ECreatureRank::Epic);
		UCameraShakeModifier::Kick(Player, ViewKicks::ForKill(bHeavy));
	}
	if (KnocksCorpse(LastHit))
	{
		StartCorpseKnock(*Creature, Tuning);
	}
}

void UCreatureHitReactionComponent::SpawnDeathBurst(ACreatureBase& Creature, const FGunfeelReactTuning& Tuning)
{
	UWorld* World = GetWorld();
	const EDeathBurst Kind = DeathBurstOf(Creature);
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (Kind == EDeathBurst::None || !Bullets)
	{
		return;
	}
	// From the middle of the body as drawn (a spider's legs spread wide, but its middle is its body).
	const USkeletalMeshComponent* Body = Creature.GetMesh();
	FVector Center = Body ? FVector(Body->Bounds.Origin) : Creature.GetActorLocation();
	float Size = 100.f * Creature.GetSizeScale();
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);

	// The precision guns: a revolver's or a sniper's crit kill bursts bigger (the sniper's bigger still), from the crit spot
	// itself, with a last spurt out of it.
	const bool bPreciseGun = LastHit.Kind == EWeaponKind::Revolver || LastHit.Kind == EWeaponKind::Sniper;
	const bool bPrecise = bPreciseGun && LastHit.bCritical && !LastHit.Location.IsNearlyZero()
		&& FVector::Dist(LastHit.Location, Center) < CritSpotReach * FMath::Max(Creature.GetSizeScale(), 0.3f);
	if (bPrecise)
	{
		const float Burst = Tuning.KillRevolverBurst * (LastHit.Kind == EWeaponKind::Sniper ? GunKinds::KillSniperBurst() : 1.f);
		Center = LastHit.Location;
		Size *= Burst;
		Effects.SpawnCreatureHit(Center, -LastHit.Direction, LastHit.Direction, CreatureBlood::Of(&Creature), CreatureBlood::TintOf(&Creature),
			Burst, true);
	}
	Effects.SpawnDeathBurst(Kind, Center, LastHit.Direction, Size, DeathBurstTint(Creature));
	LooterSound::PlayAt(this, DeathBurstCue(Kind), Center, 1.f, LooterSoundRules::PitchForSize(Creature.GetSizeScale()));
	UE_LOG(LogLooter, Verbose, TEXT("%s: death burst %d%s"), *Creature.GetName(), static_cast<int32>(Kind), bPrecise ? TEXT(" (revolver crit)") : TEXT(""));
}

void UCreatureHitReactionComponent::StartCorpseKnock(ACreatureBase& Creature, const FGunfeelReactTuning& Tuning)
{
	// A big body takes a killing shot without sliding off (a Soulfed monster's own death is its show).
	const FVector Away = LastHit.Direction.GetSafeNormal2D();
	if (Away.IsNearlyZero() || LooterRanks::IsAtLeast(Creature.GetRank(), ECreatureRank::Legendary))
	{
		return;
	}
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float Size = FMath::Max(Creature.GetSizeScale(), 0.3f);
	// By the gun: a shotgun throws it with a lift, a rifle crumples it where it stands, anything else as ever; an overkill
	// farther, a blast's overkill higher.
	KnockVelocity = Away * (CorpseKnockFor(LastHit.Kind, LastHit.bCritical, LastHit.Weight, Tuning, Feel) / FMath::Sqrt(Size));
	KnockLift = CorpseLiftFor(LastHit.Kind, LastHit.Weight, Tuning, Feel);
	KnockLiftNow = 0.f;
	KnockTime = 0.f;
	// Its height over the ground is measured on the first step, after the body's own death has settled it (a slime
	// killed mid-hop drops to the ground first).
	bKnockMeasured = false;
	// Ticks on the corpse's own clock: the kill's hit-stop holds it, then it goes.
	SetComponentTickEnabled(true);
}

void UCreatureHitReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	StepCorpseKnock(DeltaTime);
}

void UCreatureHitReactionComponent::StepCorpseKnock(float DeltaTime)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	KnockTime += DeltaTime;
	if (!Owner || !World || KnockTime > CorpseKnockSeconds || KnockVelocity.SizeSquared() < 100.f)
	{
		EndCorpseKnock();
		return;
	}
	// Where it lies under its lift: the ground rules go by that, the lift only rides on top.
	const FVector From = Owner->GetActorLocation() - FVector(0.0, 0.0, KnockLiftNow);
	const ACreatureBase* Creature = Cast<ACreatureBase>(Owner);
	const float Size = Creature ? FMath::Max(Creature->GetSizeScale(), 0.3f) : 1.f;
	FVector Ground;
	if (!bKnockMeasured)
	{
		bKnockMeasured = true;
		if (!FindGround(From, 50.f * Size, 300.f * Size, Ground))
		{
			EndCorpseKnock();
			return;
		}
		KnockGroundOffset = static_cast<float>(From.Z - Ground.Z);
	}
	const FVector To = From + KnockVelocity * DeltaTime;
	KnockVelocity *= FMath::Exp(-9.f * DeltaTime);

	// Never through anything solid: a short sweep at the body's height (a volume it starts inside doesn't count)...
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CorpseKnock), false, Owner);
	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeSphere(20.f * Size), Params) && !Hit.bStartPenetrating;
	// ...and never off a drop or up a step: it keeps its height over the ground it lies on.
	const bool bGround = !bBlocked && FindGround(To, 60.f * Size, 120.f * Size, Ground);
	const float NewZ = bGround ? static_cast<float>(Ground.Z) + KnockGroundOffset : 0.f;
	if (!bGround || FMath::Abs(NewZ - static_cast<float>(From.Z)) > 40.f * Size)
	{
		EndCorpseKnock();
		return;
	}
	// A shotgun's throw lifts it in a low arc over the slide, back down on the ground as it ends.
	KnockLiftNow = KnockLift * FMath::Sin(UE_PI * FMath::Clamp(KnockTime / CorpseKnockSeconds, 0.f, 1.f));
	Owner->SetActorLocation(FVector(To.X, To.Y, NewZ + KnockLiftNow));
}

void UCreatureHitReactionComponent::EndCorpseKnock()
{
	AActor* Owner = GetOwner();
	if (Owner && KnockLiftNow > 0.f)
	{
		Owner->SetActorLocation(Owner->GetActorLocation() - FVector(0.0, 0.0, KnockLiftNow));
	}
	KnockLiftNow = 0.f;
	KnockVelocity = FVector::ZeroVector;
	SetComponentTickEnabled(false);
}
