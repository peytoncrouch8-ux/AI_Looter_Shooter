// AStoneSentinelCreature's slam and its tread. The slam: both fists heaved overhead through a long wind-up (its WindUp cue,
// the climbing fists the tell), its aim set for the last part of it, then brought down at its end on whoever stands in reach
// in front: the ground cracks out ahead (AGroundCrack), stone and grit fly, the view shakes. Bent over its fists a moment
// after: open (SetExposed), the window to punish it. The tread: a heavy footfall each stride, grit and pebbles off its knee,
// a jolt on a near player's view.

#include "Creatures/StoneSentinelCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/StoneSentinelFX.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** A footfall's place without the model's feet: this far to its side of its middle (cm, at size 1). */
	constexpr float FootSide = 22.f;
	/** Below this speed (cm/s) it's standing: no stride, no footfalls. */
	constexpr float StandingSpeed = 15.f;
	/** The chance a footfall shakes pebbles off its knee. */
	constexpr float KneeGritChance = 0.5f;
}

void AStoneSentinelCreature::BeginSlam()
{
	// Its own timing every slam (nothing of its kind changes it: no Fleet), the tell its cue and the fists climbing (the pose).
	AttackWindup = Tuning.SlamWindup;
	AttackRecovery = Tuning.SlamRecovery;
	AttackCooldown = Tuning.SlamCooldown;
	const float Scale = GetSizeScale();
	LooterSound::PlayAttached(FName(StoneSentinelCue::WindUp), GetRootComponent(), NAME_None, 1.f, LooterSoundRules::PitchForSize(Scale));
	// It plants its feet: grit off the ground round them.
	StoneDust(GetFeet(), 3, 60.f * Scale, 40.f * Scale, /*bFloor*/ true);
}

void AStoneSentinelCreature::SlamStrike()
{
	const float Scale = GetSizeScale();
	APawn* Victim = GetTarget();
	bool bHit = false;
	if (IsValidTarget(Victim))
	{
		const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
		if (StoneSentinelRules::IsSlamHit(GetActorForwardVector(), ToVictim, Tuning, Scale) && IsSlamClear(*Victim))
		{
			HitWithAttack(Victim, ToVictim.GetSafeNormal2D());
			bHit = true;
		}
	}

	// The fists meet the ground: a crack runs out ahead and bursts round them, stone and grit fly up, a column of dust, the
	// boom, and every view near it shakes.
	const FVector Impact = GroundUnder(GetFistsPoint());
	const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
	if (AGroundCrack* Crack = AGroundCrack::Spawn(GetWorld(), Impact, Impact + Ahead * (Tuning.SlamCrackLength * Scale),
		Tuning.SlamCrackWidth * Scale, UCreatureRankSettings::Get(GetRank()).Color, FMath::Rand()))
	{
		Crack->SetOpen(1.f);
		Crack->Burst(Impact, Tuning.SlamBurstRadius * Scale);
		Crack->Close(Tuning.SlamCrackLingers);
	}
	LooterSound::PlayAt(this, FName(StoneSentinelCue::Slam), Impact, 1.f, LooterSoundRules::PitchForSize(Scale));
	ThrowChips(Impact + FVector(0.0, 0.0, 10.0), FVector::UpVector, 6, 420.f, 5.f, 14.f);
	StoneDust(Impact, 4, 100.f * Scale, 90.f * Scale, /*bFloor*/ true);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		if (StoneSentinelFX::IsClearOfLens(this, Impact + FVector(0.0, 0.0, 80.0 * Scale), 90.f * Scale, 160.f * Scale))
		{
			Effects->Jet(Impact, 90.f * Scale, 160.f * Scale, 0.6f);
		}
		Effects->Chunks(EGraveChunk::StoneChip, Impact, FVector::UpVector, 4, 420.f, 0.7f);
	}
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		if (StoneSentinelFX::IsClearOfLens(this, Impact, 130.f, 120.f))
		{
			Effects.SpawnDirt(Impact, FVector::UpVector, 0.6f);
		}
	}
	BossCameraShake::Kick(this, Impact, Tuning.SlamShake, Tuning.SlamShakeSeconds, Tuning.SlamShakeRadius * Scale);
	// Bent over its fists: open a moment, its shoulders shedding grit.
	SetExposed(Tuning.ExposedAfterSlam);
	ShedGrit(3);
	UE_LOG(LogLooter, Verbose, TEXT("%s slams%s."), *GetName(), bHit ? *FString::Printf(TEXT(" %s"), *GetNameSafe(Victim)) : TEXT(" the ground"));
}

bool AStoneSentinelCreature::IsSlamClear(const APawn& Victim) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	// The world's solid shapes only (volumes skipped, bodies aren't world-static), and never its own plinth: it reaches over
	// a waist-high block. Its middle to theirs, or to their feet (one stood on a ledge's lip).
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SentinelSlam"), this, false);
	Params.AddIgnoredActor(&Victim);
	if (const AStaticMeshActor* Stand = Plinth.Get())
	{
		Params.AddIgnoredActor(Stand);
	}
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	const FVector From = GetActorLocation();
	const float Half = Victim.GetSimpleCollisionHalfHeight();
	for (const FVector& To : { Victim.GetActorLocation(), Victim.GetActorLocation() - FVector(0.0, 0.0, Half * 0.7f) })
	{
		if (!World->LineTraceTestByObjectType(From, To, Solid, Params))
		{
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// Its tread
// ---------------------------------------------------------------------------

void AStoneSentinelCreature::TickGait(float DeltaSeconds)
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float Speed = Movement && Movement->IsMovingOnGround() ? static_cast<float>(GetVelocity().Size2D()) : 0.f;
	// Its stride shows as it gets going and settles as it stops, never a snap.
	Walking = FMath::FInterpTo(Walking, Speed > StandingSpeed ? 1.f : 0.f, DeltaSeconds, 4.f);
	if (Speed <= StandingSpeed)
	{
		return;
	}
	// A whole stride cycle is two steps (left, then right).
	const float Stride = FMath::Max(Tuning.StepLength * GetSizeScale(), 1.f);
	const float From = GaitPhase;
	const float To = From + Speed * DeltaSeconds / (Stride * 2.f);
	if (StoneSentinelRules::StepBetween(From, To))
	{
		// The half-mark it stepped over: whole ones the left foot, halves the right.
		const int32 Mark = FMath::FloorToInt32(From * 2.f) + 1;
		Footfall(Mark % 2 == 0);
	}
	GaitPhase = FMath::Frac(To);
}

void AStoneSentinelCreature::Footfall(bool bLeft)
{
	const float Scale = GetSizeScale();
	const int32 Side = bLeft ? 0 : 1;
	FVector Foot = GetFeet() + GetActorRightVector() * ((bLeft ? -FootSide : FootSide) * Scale);
	if (Rig.bReady && Rig.Foot[Side] != INDEX_NONE)
	{
		Foot = GetMesh()->GetBoneLocation(GetMesh()->GetBoneName(Rig.Foot[Side]));
	}
	Foot = GroundUnder(Foot);
	LooterSound::PlayAt(this, FName(StoneSentinelCue::Step), Foot, 0.9f, LooterSoundRules::PitchForSize(Scale));
	StoneDust(Foot, 2, 45.f * Scale, 20.f * Scale, /*bFloor*/ true);
	// Tons coming down: felt in the view of whoever is near (more the nearer), and pebbles shaken off the knee above it.
	if (Tuning.StepShake > 0.f)
	{
		BossCameraShake::Kick(this, Foot, Tuning.StepShake * Walking, Tuning.StepShakeSeconds, Tuning.StepShakeRadius * Scale);
	}
	if (FMath::FRand() < KneeGritChance)
	{
		ShedGrit(1, Rig.Calf[Side]);
	}
}
