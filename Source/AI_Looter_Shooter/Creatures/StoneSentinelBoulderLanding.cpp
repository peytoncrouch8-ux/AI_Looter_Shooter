// AStoneSentinelBoulder's endings (StoneSentinelBoulder.h): coming down (the rock bursting, stone and dust thrown out, the
// ground cracked round it, the hurt and the shove to every player near, the view shaken, an impact scar fading after), and
// its marks cleared away.

#include "Creatures/StoneSentinelBoulder.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/CombatRules.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Creatures/StoneSentinelFX.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** It reaches a player this far above or below where it burst (cm, past their half height). */
	constexpr float BurstLevel = 160.f;
	/** The shove at full strength (cm/s), away and up. */
	constexpr float ShoveAway = 520.f;
	constexpr float ShoveUp = 240.f;
	/** The burst's shake on a view (0-1), how long, how far (cm). */
	constexpr float ImpactShake = 0.45f;
	constexpr float ImpactShakeSeconds = 0.55f;
	constexpr float ImpactShakeRadius = 2000.f;
	/** The cracks round it (cm wide, out to this share of the burst), and how long they stay open (s). */
	constexpr float CrackWidth = 20.f;
	constexpr float CrackShare = 0.85f;
	constexpr float CrackLingers = 2.f;
	/** The scar: this share of the burst across, shown this long and faded over the last FadeSeconds of it (s). */
	constexpr float ScarShare = 1.1f;
	constexpr float ScarSeconds = 5.f;
	constexpr float ScarFadeSeconds = 1.5f;
	/** The burst kept cheap: dust puffs (high, low), its stones (cm across), the dirt's smoke and the jet (cm) seen only from afar. */
	constexpr int32 CloudPuffs = 4;
	constexpr int32 FloorPuffs = 3;
	constexpr int32 StoneCount = 7;
	constexpr float StoneMin = 6.f;
	constexpr float StoneMax = 16.f;
	constexpr float DirtSmokeSize = 130.f;
	constexpr float DirtSmokeTravel = 120.f;
	constexpr float JetWidth = 90.f;
	constexpr float JetHeight = 180.f;
	/** The dust ring rolling out low from the burst. */
	const FLinearColor DustRingColor(0.42f, 0.38f, 0.33f);
}

void AStoneSentinelBoulder::Land(const FVector& Where, const FVector& Normal)
{
	const bool bHarmless = bDropped;
	Phase = EPhase::Landed;
	LandedAt = Where;
	ClearMark();
	UWorld* World = GetWorld();
	// No normal: it came down on a player (its burst sits on the ground under them, near enough).
	const FVector Up = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	const float Reach = Tuning.SplashRadius;

	// The burst, cheap (Main's PIE check 2026-10-31: thirty big dust cards at the lens took the frame from 80 to 12 fps): its
	// punch is the thud, the shake, the stones and the scar. A few short puffs where the camera isn't (StoneSentinelFX keeps
	// every card's grown edge 3 m off the lens, within a budget), stones of its thrower's own instanced chips (reused: nothing
	// spawned) or a few of the barrow's, a low ring of dust on the ground (one quad), and smoke only seen from afar.
	FGraveDustBurst Cloud;
	Cloud.Kind = EGraveDust::Stone;
	Cloud.Count = bHarmless ? 2 : CloudPuffs;
	Cloud.Spread = 70.f;
	Cloud.SpreadUp = 30.f;
	Cloud.StartSize = 50.f;
	Cloud.EndSize = 130.f;
	Cloud.Life = 0.9f;
	Cloud.Opacity = 0.32f;
	Cloud.Drift = FVector(0.0, 0.0, 40.0);
	Cloud.Outward = 180.f;
	StoneSentinelFX::Dust(this, Where + Up * 25.f, Cloud);
	FGraveDustBurst Floor;
	Floor.Kind = EGraveDust::Floor;
	Floor.Count = bHarmless ? 1 : FloorPuffs;
	Floor.Spread = 110.f;
	Floor.SpreadUp = 5.f;
	Floor.StartSize = 50.f;
	Floor.EndSize = 120.f;
	Floor.Life = 0.8f;
	Floor.Opacity = 0.26f;
	Floor.Drift = FVector(0.0, 0.0, 15.0);
	Floor.Outward = 220.f;
	StoneSentinelFX::Dust(this, Where, Floor);
	const int32 Stones = bHarmless ? 3 : StoneCount;
	if (AStoneSentinelCreature* Its = Thrower.Get(); Its && !Its->IsDead())
	{
		Its->ThrowStones(Where + Up * 20.f, Up, Stones, 560.f, StoneMin, StoneMax);
	}
	else if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Chunks(EGraveChunk::StoneChip, Where + Up * 20.f, Up, Stones, 520.f, 0.7f);
	}
	if (World && World->IsGameWorld())
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			// The dirt's smoke and the jet only where they can't fill the view.
			if (StoneSentinelFX::IsClearOfLens(this, Where, DirtSmokeSize, DirtSmokeTravel))
			{
				Effects.SpawnDirt(Where, Up, bHarmless ? 0.3f : 0.6f);
			}
			if (!bHarmless)
			{
				Effects.SpawnShockRing(Where + Up * 8.f, Up, Reach * 0.3f, Reach * 1.2f, 40.f, DustRingColor, 0.4f, 0.35f);
			}
		}
	}
	if (!bHarmless && StoneSentinelFX::IsClearOfLens(this, Where + Up * 110.f, JetWidth, JetHeight))
	{
		if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
		{
			Effects->Jet(Where, JetWidth, JetHeight, 0.6f);
		}
	}
	// The scar it leaves, fading (TickScar); the cracks (an actor of their own) only where there's no scar decal to show.
	// The scar decal is made once and laid again each landing.
	if (!bHarmless)
	{
		const float ScarSize = Reach * 2.f * ScarShare;
		if (Scar)
		{
			Scar->SetWorldLocation(Where);
			GravemotherAssets::SetDecalSize(Scar, ScarSize, ScarSize, 300.f);
		}
		else
		{
			Scar = GravemotherAssets::AddGroundDecal(*this, GravemotherAssets::DecalImpactScar, Where, FMath::FRandRange(0.f, 360.f),
				ScarSize, ScarSize, 2, 300.f);
		}
		GravemotherAssets::SetDecalOpacity(Scar, 1.f);
		bScarShown = Scar != nullptr;
	}
	if (World && !bScarShown)
	{
		// Its seam glows its thrower's tier colour, as the cracks its fists leave do.
		const AStoneSentinelCreature* Its = Thrower.Get();
		const FLinearColor Seam = Its ? UCreatureRankSettings::Get(Its->GetRank()).Color : FLinearColor(0.4f, 0.36f, 0.3f);
		if (AGroundCrack* Crack = AGroundCrack::Spawn(World, Where, Where + FVector::ForwardVector, CrackWidth, Seam, FMath::Rand()))
		{
			Crack->Burst(Where, Reach * (bHarmless ? 0.4f : CrackShare));
			Crack->Close(CrackLingers);
		}
	}
	LooterSound::PlayAt(this, FName(StoneSentinelCue::BoulderImpact), Where, bHarmless ? 0.6f : 1.f);
	if (!bHarmless)
	{
		BossCameraShake::Kick(this, Where, ImpactShake, ImpactShakeSeconds, ImpactShakeRadius);
		PlayersHit = HurtNear(Where);
		UE_LOG(LogLooter, Log, TEXT("%s's boulder came down after %.2f s: %d player(s) hit."), *GetNameSafe(Thrower.Get()), FlightTime, PlayersHit);
	}

	// The rock is gone; it ticks on only while its scar fades (TickScar), then it's idle for its thrower to tear up again.
	Body->SetHiddenInGame(true);
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScarAge = 0.f;
	if (!bScarShown)
	{
		GoIdle();
	}
}

int32 AStoneSentinelBoulder::HurtNear(const FVector& Where)
{
	UWorld* World = GetWorld();
	if (!World || Damage <= 0.f)
	{
		return 0;
	}
	AActor* By = ThrowCauser.IsValid() ? ThrowCauser.Get() : this;
	int32 Hit = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (!Capsule || !UEnemyProjectileSubsystem::CanHurt(By, Character))
		{
			continue;
		}
		// To their body's side, flat, on its level.
		const FVector Middle = Capsule->GetComponentLocation();
		const float Side = FMath::Max(static_cast<float>(FVector::Dist2D(Where, Middle)) - Capsule->GetScaledCapsuleRadius(), 0.f);
		const float Share = StoneSentinelMoves::SplashShare(Side, Tuning);
		if (Share <= 0.f || FMath::Abs(Where.Z - Middle.Z) > Capsule->GetScaledCapsuleHalfHeight() + BurstLevel)
		{
			continue;
		}
		const float Dealt = LooterCombat::RollHitDamage(Damage * Share, false);
		UGameplayStatics::ApplyDamage(Character, Dealt, ThrowInstigator.Get(), By, UCreatureAttackDamageType::StaticClass());
		if (const AStoneSentinelCreature* Its = Thrower.Get())
		{
			// For the fight log: what landed, by whom.
			UFightDirectorSubsystem::Note(*Its, EFightEvent::AttackLanded, Dealt);
		}
		FVector Away = Middle - Where;
		Away.Z = 0.0;
		Away = Away.IsNearlyZero() ? FVector::ForwardVector : Away.GetSafeNormal();
		Character->LaunchCharacter((Away * ShoveAway + FVector(0.0, 0.0, ShoveUp)) * Share, true, false);
		++Hit;
	}
	return Hit;
}

void AStoneSentinelBoulder::ClearMark()
{
	// Kept for the next throw, hidden.
	GravemotherAssets::SetDecalOpacity(Shadow, 0.f);
	if (AGroundCrack* Ring = MarkCrack.Get())
	{
		Ring->Close(0.f);
	}
	MarkCrack = nullptr;
}

void AStoneSentinelBoulder::TickScar(float DeltaSeconds)
{
	if (!Scar)
	{
		return;
	}
	ScarAge += DeltaSeconds;
	const float Left = ScarSeconds - ScarAge;
	GravemotherAssets::SetDecalOpacity(Scar, FMath::Clamp(Left / ScarFadeSeconds, 0.f, 1.f));
	if (Left <= 0.f)
	{
		GoIdle();
	}
}
