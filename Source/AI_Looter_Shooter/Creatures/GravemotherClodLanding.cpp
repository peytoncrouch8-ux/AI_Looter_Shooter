// AGravemotherClod's endings (GravemotherClod.h): coming down (a burst of dust and debris, the hurt and the shove to every
// player near, the rubble pile it leaves), bursting on her when it's shot in her grip, and its shadow cleared away.

#include "Creatures/GravemotherClod.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/CombatRules.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GravemotherRubble.h"
#include "Creatures/GroundCrack.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** It reaches a player this far above or below where it burst (cm, past their half height). */
	constexpr float BurstLevel = 150.f;
	/** The shove, as a creature's attack shoves (ACreatureBase::HitWithAttack), at full strength. */
	constexpr float ShoveAway = 450.f;
	constexpr float ShoveUp = 180.f;
	/** A burst on ground this steep or flatter leaves its rubble there; else on the ground below (cm down). */
	constexpr float FloorNormalZ = 0.6f;
	constexpr float RubbleDrop = 400.f;
}

void AGravemotherClod::Land(const FVector& Where, const FVector& Normal)
{
	const bool bHarmless = bDropped;
	Phase = EPhase::Landed;
	LandedAt = Where;
	ClearShadow();
	UWorld* World = GetWorld();
	// No normal: it came down on a player (its rubble goes on the ground under them).
	const bool bOnBody = Normal.IsNearlyZero();
	const FVector Up = bOnBody ? FVector::UpVector : Normal.GetSafeNormal();

	// The burst: fresh grave earth thrown up and spreading (the sheet's 0.1 / 0.4 / 1.0 s cloud), pale floor dust kicked
	// low round it, clumps, stones, splinters and bone flung out, a few clods.
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		FGraveDustBurst Cloud;
		Cloud.Count = bHarmless ? 8 : 22;
		Cloud.Spread = 60.f;
		Cloud.SpreadUp = 40.f;
		Cloud.StartSize = 80.f;
		Cloud.EndSize = 190.f;
		Cloud.Life = 1.5f;
		Cloud.Opacity = 0.36f;
		Cloud.Drift = FVector(0.0, 0.0, 45.0);
		Cloud.Outward = bHarmless ? 120.f : 230.f;
		Effects->Puffs(Where + Up * 20.f, Cloud);
		FGraveDustBurst Floor;
		Floor.Kind = EGraveDust::Floor;
		Floor.Count = bHarmless ? 3 : 6;
		Floor.Spread = 80.f;
		Floor.SpreadUp = 5.f;
		Floor.StartSize = 60.f;
		Floor.EndSize = 160.f;
		Floor.Life = 1.2f;
		Floor.Opacity = 0.28f;
		Floor.Drift = FVector(0.0, 0.0, 15.0);
		Floor.Outward = 260.f;
		Effects->Puffs(Where, Floor);
		Effects->Chunks(EGraveChunk::Debris, Where + Up * 15.f, Up, bHarmless ? 3 : 8, 650.f);
		Effects->Chunks(EGraveChunk::Clod, Where + Up * 15.f, Up, bHarmless ? 1 : 3, 480.f);
	}
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherClodBurst, Where, bHarmless ? 0.6f : 1.f);
	if (!bHarmless)
	{
		BossCameraShake::Kick(this, Where, 0.35f, 0.5f);
		PlayersHit = HurtNear(Where);

		// The rubble pile it leaves: where it lies, or on the ground under the wall or the player it struck.
		FVector Ground = Where;
		bool bGround = !bOnBody && Up.Z >= FloorNormalZ;
		if (!bGround && World)
		{
			FHitResult Below;
			const FVector From = Where + (bOnBody ? FVector::ZeroVector : FVector(Up.X, Up.Y, 0.0).GetSafeNormal() * 30.f) + FVector(0.0, 0.0, 20.0);
			bGround = World->LineTraceSingleByObjectType(Below, From, From - FVector(0.0, 0.0, RubbleDrop), FCollisionObjectQueryParams(ECC_WorldStatic),
				WorldQuery) && Below.ImpactNormal.Z >= FloorNormalZ;
			Ground = bGround ? FVector(Below.ImpactPoint) : Where;
		}
		if (bGround)
		{
			Rubble = AGravemotherRubble::Spawn(World, Ground, FMath::FRandRange(0.f, 360.f), Settings.RubbleSeconds, Settings.RubbleSettleSeconds);
		}
		UE_LOG(LogLooter, Log, TEXT("%s's clod came down after %.2f s: %d player(s) hit, rubble %s."), *GetNameSafe(Mother.Get()), FlightTime,
			PlayersHit, Rubble.IsValid() ? TEXT("left") : TEXT("none"));
	}

	// Gone a moment later (the tests read what it did meanwhile).
	Body->SetHiddenInGame(true);
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTickEnabled(false);
	SetLifeSpan(0.2f);
}

int32 AGravemotherClod::HurtNear(const FVector& Where)
{
	UWorld* World = GetWorld();
	if (!World || Damage <= 0.f)
	{
		return 0;
	}
	AActor* By = ThrowCauser.IsValid() ? ThrowCauser.Get() : this;
	int32 Hit = 0;
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Character = *It;
		const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (!Capsule || !UEnemyProjectileSubsystem::CanHurt(By, Character))
		{
			continue;
		}
		// To their body's side, flat, on its level.
		const FVector Middle = Capsule->GetComponentLocation();
		const float Side = FMath::Max(static_cast<float>(FVector::Dist2D(Where, Middle)) - Capsule->GetScaledCapsuleRadius(), 0.f);
		if (Side > Settings.BurstRadius || FMath::Abs(Where.Z - Middle.Z) > Capsule->GetScaledCapsuleHalfHeight() + BurstLevel)
		{
			continue;
		}
		const float Share = FMath::Lerp(1.f, Settings.EdgeShare, Side / FMath::Max(Settings.BurstRadius, 1.f));
		const float Dealt = LooterCombat::RollHitDamage(Damage * Share, false);
		UGameplayStatics::ApplyDamage(Character, Dealt, ThrowInstigator.Get(), By, UCreatureAttackDamageType::StaticClass());
		FVector Away = Middle - Where;
		Away.Z = 0.0;
		Away = Away.IsNearlyZero() ? FVector::ForwardVector : Away.GetSafeNormal();
		Character->LaunchCharacter((Away * ShoveAway + FVector(0.0, 0.0, ShoveUp)) * (Settings.Strength * Share), true, false);
		++Hit;
	}
	return Hit;
}

void AGravemotherClod::BurstOnMother()
{
	// Shot in her grip: it bursts over her, dirt raining on her head and back, and her shriek.
	const FVector Where = GetActorLocation();
	AGravemotherCreature* Her = Mother.Get();
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		FGraveDustBurst Rain;
		Rain.Count = 16;
		Rain.Spread = 70.f;
		Rain.SpreadUp = 30.f;
		Rain.StartSize = 70.f;
		Rain.EndSize = 180.f;
		Rain.Life = 1.7f;
		Rain.Opacity = 0.38f;
		Rain.Drift = FVector(0.0, 0.0, 10.0);
		Rain.Outward = 170.f;
		Rain.Fall = 0.3f;
		Effects->Puffs(Where, Rain);
		Effects->Chunks(EGraveChunk::Debris, Where, FVector::UpVector, 8, 260.f);
		Effects->Chunks(EGraveChunk::Clod, Where, FVector::UpVector, 3, 220.f);
	}
	if (Her)
	{
		LooterSound::PlayAttached(LooterSoundCue::GravemotherClodShot, Her->GetRootComponent());
	}
	else
	{
		LooterSound::PlayAt(this, LooterSoundCue::GravemotherClodShot, Where);
	}
	Phase = EPhase::Landed;
	LandedAt = Where;
	Body->SetHiddenInGame(true);
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorTickEnabled(false);
	SetLifeSpan(0.2f);
}

void AGravemotherClod::ClearShadow()
{
	if (Shadow)
	{
		Shadow->DestroyComponent();
		Shadow = nullptr;
	}
	if (AGroundCrack* Ring = ShadowCrack.Get())
	{
		Ring->Close(0.f);
	}
	ShadowCrack = nullptr;
}
