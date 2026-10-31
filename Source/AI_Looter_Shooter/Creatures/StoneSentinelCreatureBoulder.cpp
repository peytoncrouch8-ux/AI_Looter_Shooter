// AStoneSentinelCreature's hurled boulder (the user's pick, 2026-10-31): bent to the ground it tears a rock out of it (the
// ground cracks, earth and roots pour off it, its Tear cue), lifts it overhead, winds back with its trunk turned round and
// hurls it on an arc at where its target will be (AStoneSentinelBoulder: the landing marked, a burst that hurts all round).
// The rock in its grip can be shot: broken there it bursts over its own head (CounterBoulder), hurting it; it reels, open.

#include "Creatures/StoneSentinelCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundRules.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/StoneSentinelBoulder.h"
#include "Creatures/StoneSentinelFX.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The rock starts this share of its size sunk in the ground, and takes this long (s) to come up out of it into its grip. */
	constexpr float SunkShare = 0.3f;
	constexpr float RiseSeconds = 0.35f;
	/** Sitting in its grip: this share of the rock's size over its fists once it's lifted (between its hands at the bottom). */
	constexpr float OnFistsShare = 0.45f;
	/** The arms' swing at the bottom of the heave and with the rock overhead (StoneSentinelPoses.cpp's Reach and Overhead). */
	constexpr float SwingLow = 40.f;
	constexpr float SwingHigh = 124.f;
	/** The stand-in's shoulder over its middle and its arm's reach to its fists (cm at size 1). */
	constexpr float StandInShoulder = 75.f;
	constexpr float StandInArm = 115.f;
	/** The tear: its cracks (cm wide and long at size 1, open for this long, s), the burst round the rock, its jolt. */
	constexpr float TearCrackWidth = 22.f;
	constexpr float TearCrackLength = 90.f;
	constexpr float TearCrackLingers = 2.f;
	constexpr float TearBurst = 150.f;
	constexpr float TearShake = 0.15f;
	/** Earth pouring off the rock as it comes up (pieces a second, up to its lift). */
	constexpr float PourRate = 5.f;
	/** The release's jolt, and the burst over its head (its rain of stone) when the rock is broken in its grip. */
	constexpr float HeaveShake = 0.12f;
	constexpr float CounterShake = 0.3f;
	/** Its next throw comes this many times its cooldown later after a broken one. */
	constexpr float CounterCooldownShare = 1.5f;
	/** A rock burst on its head rocks it back (degrees, as a hit's jolt). */
	constexpr float CounterJoltPitch = 7.f;
	constexpr float CounterJoltRoll = 3.f;
}

FVector AStoneSentinelCreature::GetBoulderHoldPoint() const
{
	const float Scale = GetSizeScale();
	// It sits between its hands at the bottom of the heave and on its fists once it's up.
	const float Lifted = FMath::Clamp((PoseNow.ArmSwing - SwingLow) / (SwingHigh - SwingLow), 0.f, 1.f);
	const FVector Up = FVector::UpVector * (BoulderTuning.Size * OnFistsShare * Lifted);
	const USkeletalMeshComponent* Body = GetMesh();
	if (!bStandIn && Body && Body->DoesSocketExist(FistLSocket) && Body->DoesSocketExist(FistRSocket))
	{
		return GetFistsPoint() + Up;
	}
	// The stand-in has no hands: where they'd be by its arms' swing.
	const float Swing = FMath::DegreesToRadians(PoseNow.ArmSwing + PoseNow.ElbowBend * 0.5f);
	const FVector Way = GetActorQuat().RotateVector(FVector(FMath::Sin(Swing), 0.f, -FMath::Cos(Swing)));
	return GetActorLocation() + FVector(0.0, 0.0, StandInShoulder * Scale) + Way * (StandInArm * Scale) + Up;
}

void AStoneSentinelCreature::BeginThrow()
{
	AttackWindup = BoulderTuning.Windup;
	AttackRecovery = BoulderTuning.Recovery;
	AttackCooldown = BoulderTuning.Breath;
	const float Scale = GetSizeScale();
	TearSpot = GroundUnder(GetActorLocation() + GetActorForwardVector() * (BoulderTuning.TearAhead * Scale));
	BoulderYaw = FMath::FRandRange(0.f, 360.f);
	// It plants its feet and bends to the ground: grit off it, its joints grinding (its WindUp cue, quieter).
	LooterSound::PlayAttached(FName(StoneSentinelCue::WindUp), GetRootComponent(), NAME_None, 0.7f, LooterSoundRules::PitchForSize(Scale) * 0.92f);
	StoneDust(GetFeet(), 3, 60.f * Scale, 40.f * Scale, /*bFloor*/ true);
}

void AStoneSentinelCreature::TickThrow(float DeltaSeconds)
{
	const float Time = GetStateTime();
	if (!bBoulderTorn && !bBoulderCountered && Time >= BoulderTuning.TearAt && Time < AttackWindup)
	{
		TearBoulder();
	}
	AStoneSentinelBoulder* Rock = HeldBoulder.Get();
	if (!Rock || !Rock->IsHeld())
	{
		return;
	}
	// Up out of the ground into its grip, then carried there as its arms go.
	const FVector Sunk = TearSpot - FVector(0.0, 0.0, BoulderTuning.Size * SunkShare);
	const float Out = FMath::SmoothStep(0.f, 1.f, FMath::Clamp((Time - BoulderTuning.TearAt) / RiseSeconds, 0.f, 1.f));
	const FVector Where = FMath::Lerp(Sunk, GetBoulderHoldPoint(), static_cast<double>(Out));
	Rock->Hold(Where, static_cast<float>(GetActorRotation().Yaw) + BoulderYaw);
	// Earth and roots pouring off it as it comes up.
	if (Time < BoulderTuning.LiftedAt && FMath::FRand() < PourRate * DeltaSeconds)
	{
		if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
		{
			Effects->Chunks(EGraveChunk::Debris, Where, FVector::DownVector, 1, 60.f, 0.5f);
		}
		StoneDust(Where - FVector(0.0, 0.0, BoulderTuning.Size * 0.4f), 1, 40.f, 15.f, /*bFloor*/ true);
	}
}

void AStoneSentinelCreature::TearBoulder()
{
	bBoulderTorn = true;
	UWorld* World = GetWorld();
	const float Scale = GetSizeScale();
	const FVector Sunk = TearSpot - FVector(0.0, 0.0, BoulderTuning.Size * SunkShare);
	// Its own rock torn up again when it lies idle (nothing new made); a new one kept the first time, a spare if its own is still out.
	if (AStoneSentinelBoulder* Own = KeptBoulder.Get(); Own && Own->IsIdle())
	{
		Own->Retear(Sunk, BoulderTuning);
		HeldBoulder = Own;
	}
	else
	{
		HeldBoulder = AStoneSentinelBoulder::Spawn(*this, Sunk, BoulderTuning, /*bInKeep*/ !Own);
		if (!Own)
		{
			KeptBoulder = HeldBoulder;
		}
	}
	// The ground splitting round it as it comes free: cracks, a burst of earth and stone, a jolt underfoot, the tearing.
	const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
	if (AGroundCrack* Crack = AGroundCrack::Spawn(World, TearSpot - Ahead * (TearCrackLength * Scale), TearSpot + Ahead * (TearCrackLength * Scale),
		TearCrackWidth * Scale, UCreatureRankSettings::Get(GetRank()).Color, FMath::Rand()))
	{
		Crack->SetOpen(1.f);
		Crack->Burst(TearSpot, TearBurst * Scale);
		Crack->Close(TearCrackLingers);
	}
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Chunks(EGraveChunk::Debris, TearSpot, FVector::UpVector, 4, 380.f);
		Effects->Chunks(EGraveChunk::Clod, TearSpot, FVector::UpVector, 2, 300.f);
	}
	StoneDust(TearSpot, 3, 110.f * Scale, 80.f * Scale, /*bFloor*/ true);
	if (World && World->IsGameWorld())
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			if (StoneSentinelFX::IsClearOfLens(this, TearSpot, 130.f, 120.f))
			{
				Effects.SpawnDirt(TearSpot, FVector::UpVector, 0.8f);
			}
		}
	}
	LooterSound::PlayAt(this, FName(StoneSentinelCue::Tear), TearSpot, 1.f, LooterSoundRules::PitchForSize(Scale));
	BossCameraShake::Kick(this, TearSpot, TearShake, 0.35f, 1200.f * Scale);
	Jolt.X += 2.f;
}

void AStoneSentinelCreature::ReleaseBoulder()
{
	bBoulderThrown = true;
	BoulderReadyIn = FMath::Max(BoulderReadyIn, BoulderTuning.Cooldown);
	AStoneSentinelBoulder* Rock = HeldBoulder.Get();
	if (bBoulderCountered || !Rock || !Rock->IsHeld())
	{
		return;
	}
	APawn* Victim = GetTarget();
	if (!IsValidTarget(Victim))
	{
		DropBoulder();
		return;
	}
	// Where they'll be: their feet on the ground (a jump doesn't lift the aim), their run carried on for the flight.
	const FVector Start = Rock->GetActorLocation();
	const ACharacter* Character = Cast<ACharacter>(Victim);
	const float Half = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
	FVector Feet = Victim->GetActorLocation() - FVector(0.0, 0.0, Half);
	FVector Ground;
	if (FindGround(Feet, 60.f, 600.f, Ground))
	{
		Feet = Ground;
	}
	const float Guess = StoneSentinelMoves::FlightSeconds(static_cast<float>(FVector::Dist2D(Start, Feet)), BoulderTuning);
	const FVector Aim = StoneSentinelMoves::AimPoint(Feet, Victim->GetVelocity(), Guess, BoulderTuning);
	const float Seconds = StoneSentinelMoves::FlightSeconds(static_cast<float>(FVector::Dist2D(Start, Aim)), BoulderTuning);
	const FVector Velocity = StoneSentinelMoves::LobVelocity(Start, Aim, Seconds, BoulderTuning);
	// It hits as hard as its slam does times the throw's Strength, against the fight's difficulty.
	Rock->Throw(Velocity, AttackDamage * BoulderTuning.Strength * GetFightDamageScale(), GetController(), this);
	const float Scale = GetSizeScale();
	LooterSound::PlayAt(this, FName(StoneSentinelCue::Heave), Start, 1.f, LooterSoundRules::PitchForSize(Scale));
	StoneDust(Start, 3, 60.f * Scale, 30.f * Scale);
	ShedGrit(3);
	BossCameraShake::Kick(this, GetFeet(), HeaveShake, 0.3f, 900.f * Scale);
	UE_LOG(LogLooter, Verbose, TEXT("%s hurls its boulder at %s (%.2f s)."), *GetName(), *GetNameSafe(Victim), Seconds);
}

void AStoneSentinelCreature::DropBoulder()
{
	if (AStoneSentinelBoulder* Rock = HeldBoulder.Get(); Rock && Rock->IsHeld())
	{
		Rock->Drop();
	}
}

void AStoneSentinelCreature::CounterBoulder(AController* By, const FVector& Where)
{
	if (Move != EStoneSentinelMove::Boulder || bBoulderThrown || bBoulderCountered || IsDead() || GetCreatureState() != ECreatureState::Attack)
	{
		return;
	}
	bBoulderCountered = true;
	AStoneSentinelBoulder* Rock = HeldBoulder.Get();
	HeldBoulder.Reset();
	const float Scale = GetSizeScale();

	// The rock bursts over its head: stone rains down on it, dust pours, the shatter; it rocks back.
	ThrowChips(Where, FVector::UpVector, 8, 280.f, 8.f * Scale, 18.f * Scale, 0.8f, 1.4f);
	StoneDust(Where, 3, 100.f * Scale, 60.f * Scale);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		FGraveDustBurst Rain;
		Rain.Kind = EGraveDust::Stone;
		Rain.Count = 4;
		Rain.Spread = 60.f * Scale;
		Rain.SpreadUp = 30.f;
		Rain.StartSize = 50.f * Scale;
		Rain.EndSize = 120.f * Scale;
		Rain.Life = 1.f;
		Rain.Opacity = 0.36f;
		Rain.Drift = FVector(0.0, 0.0, 10.0);
		Rain.Outward = 150.f;
		Rain.Fall = 0.3f;
		StoneSentinelFX::Dust(this, Where, Rain);
		Effects->Chunks(EGraveChunk::StoneChip, Where, FVector::UpVector, 4, 300.f, 0.8f);
	}
	LooterSound::PlayAttached(FName(StoneSentinelCue::BoulderShatter), GetRootComponent(), NAME_None, 1.f, LooterSoundRules::PitchForSize(Scale));
	BossCameraShake::Kick(this, Where, CounterShake, 0.45f, 1400.f * Scale);
	Jolt.X += CounterJoltPitch;
	Jolt.Y += FMath::FRandRange(-CounterJoltRoll, CounterJoltRoll);

	// Its throw cut short: the strike comes at once and throws nothing, then a long reel; open all through it.
	AttackWindup = FMath::Max(GetStateTime(), 0.05f);
	AttackRecovery = BoulderTuning.CounterStun;
	BoulderReadyIn = BoulderTuning.Cooldown * CounterCooldownShare;
	SetExposed(BoulderTuning.CounterExposed);
	if (APawn* Shooter = By ? By->GetPawn() : nullptr)
	{
		AlertTo(Shooter);
	}
	// The rock's weight on its own head: a share of its health, theirs who broke it (its stone's guard already down).
	const float Hurt = Health->GetMaxHealth() * BoulderTuning.CounterShare;
	if (Hurt > 0.f)
	{
		UGameplayStatics::ApplyDamage(this, Hurt, By, Rock ? static_cast<AActor*>(Rock) : static_cast<AActor*>(this), UDamageType::StaticClass());
	}
	UE_LOG(LogLooter, Log, TEXT("%s: its boulder was broken in its grip; it reels %.1f s."), *GetName(), BoulderTuning.CounterStun);
}
