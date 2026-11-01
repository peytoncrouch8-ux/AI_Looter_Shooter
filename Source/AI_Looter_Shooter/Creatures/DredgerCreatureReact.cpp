// ADredgerCreature: its hits (a flinch by the hit's weight and way, the head's whip, a crit breaking a lens, the hurt cry),
// its barks, its notice, and its death (limp, thrown by the killing shot, the soul-smog leaking out of its mask; the code
// fall when no ragdoll takes it), its sink and its return.

#include "Creatures/DredgerCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundRules.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GunfeelReactTuning.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerTuning.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsAsset.h"

namespace
{
	/** The world the last Dredger bark was in, and when (world seconds): every Dredger shares one voice's room. */
	TWeakObjectPtr<const UWorld> LastBarkWorld;
	double LastBarkAt = -1000.0;

	/** A hit's flinch at full strength (degrees: forward/back, sideways, twist), its head's whip, and how long each kick holds (s). */
	constexpr float FlinchAhead = 9.f;
	constexpr float FlinchSide = 7.f;
	constexpr float FlinchTwist = 10.f;
	constexpr float WhipDown = 20.f;
	constexpr float WhipSide = 12.f;
	constexpr float FlinchHold = 0.04f;
	/** Its hurt cry at most this often (s) and this likely; a kill always has its death cry instead. */
	constexpr double HurtCryGap = 0.9;
	constexpr float HurtCryChance = 0.4f;
	/** The code fall's length (s), and a corpse's sink (cm/s). */
	constexpr float FallSeconds = 0.8f;
	constexpr float SinkSpeed = 70.f;
	/** A ragdoll thrown along the killing shot (cm/s) and lifted, by the kill's style. */
	constexpr float ThrowBody = 260.f;
	constexpr float ThrowCrit = 380.f;
	constexpr float ThrowBlast = 720.f;
	constexpr float LiftBlast = 260.f;
	constexpr float ThrowOverkill = 1000.f;
	constexpr float LiftOverkill = 380.f;

	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	/** Each Dredger's own voice a little higher or lower (from its name), at its size's pitch. */
	float VoicePitchOf(const ACreatureBase& Dredger)
	{
		const uint32 Hash = GetTypeHash(Dredger.GetFName().ToString());
		return LooterSoundRules::PitchForSize(Dredger.GetSizeScale()) * (0.93f + 0.14f * static_cast<float>(Hash % 1000u) / 999.f);
	}
}

// ---------------------------------------------------------------------------
// Barks and noticing
// ---------------------------------------------------------------------------

bool ADredgerCreature::Bark(FName Cue, float Chance)
{
	const UWorld* World = GetWorld();
	if (!World || IsDead() || Cue.IsNone() || FMath::FRand() > Chance)
	{
		return false;
	}
	// One voice at a time: a squad's calls come one after another, never in a heap.
	const double Now = World->GetTimeSeconds();
	if (LastBarkWorld.Get() == World && Now - LastBarkAt < FDredgerTuning::Live().BarkGap)
	{
		return false;
	}
	LastBarkWorld = World;
	LastBarkAt = Now;
	LooterSound::PlayAttached(Cue, GetMesh(), TEXT("head"), 1.f, VoicePitchOf(*this));
	return true;
}

void ADredgerCreature::ResetBarks()
{
	LastBarkWorld.Reset();
	LastBarkAt = -1000.0;
}

void ADredgerCreature::OnNoticed()
{
	// The notice beat: a muffled shout through the mask, the lenses lighting as it turns on them, the gun coming up.
	Bark(LooterSoundCue::DredgerBark);
	FlareLenses(0.5f, 0.7f);
}

// ---------------------------------------------------------------------------
// Hits
// ---------------------------------------------------------------------------

void ADredgerCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	const FCreatureHit& Hit = GetLastHit();
	const float Strength = Hit.FlinchStrength(FGunfeelReactTuning::Live());
	// The shot's way in its own frame: a shot from in front (going back along its facing) rocks it back, one from its left
	// leans it right, one off its middle twists it.
	const FVector Way = GetActorQuat().UnrotateVector(Hit.Direction.IsNearlyZero() ? -GetActorForwardVector() : Hit.Direction.GetSafeNormal());
	const float Twist = FMath::Clamp(Hit.LateralOffset / 30.f, -1.f, 1.f);
	FlinchSpring.Add(FVector(Way.X * FlinchAhead, Way.Y * FlinchSide, Twist * FlinchTwist) * Strength, FlinchHold);
	if (Hit.Zone == ECreatureHitZone::Head)
	{
		HeadSpring.Add(FVector(Way.X * WhipDown, Way.Y * WhipSide, 0.f) * Strength, FlinchHold);
		if (bCritical)
		{
			BreakLens(HitLocation);
		}
	}

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Health->GetHealth() > 0.f && Now - LastHurtSound >= HurtCryGap && FMath::FRand() < HurtCryChance)
	{
		LastHurtSound = Now;
		LooterSound::PlayAttached(LooterSoundCue::DredgerHurt, GetMesh(), TEXT("head"), 0.9f, VoicePitchOf(*this));
	}
}

void ADredgerCreature::BreakLens(const FVector& HitLocation)
{
	// A crit on the mask cracks the lens nearer the shot out of it: glass flashes and sparks, and that eye goes dark.
	UStaticMeshComponent* Lens = nullptr;
	float Best = TNumericLimits<float>::Max();
	for (UStaticMeshComponent* Each : { LensLeft.Get(), LensRight.Get() })
	{
		if (Each && Each->IsVisible())
		{
			const float Distance = static_cast<float>(FVector::DistSquared(Each->GetComponentLocation(), HitLocation));
			if (Distance < Best)
			{
				Best = Distance;
				Lens = Each;
			}
		}
	}
	if (!Lens)
	{
		return;
	}
	Lens->SetVisibility(false);
	++BrokenLenses;
	if (FWeaponFX* Effects = EffectsIn(GetWorld()))
	{
		const FVector Out = GetActorForwardVector();
		Effects->SpawnFlash(Lens->GetComponentLocation(), Out, 0.35f);
		Effects->SpawnRicochet(Lens->GetComponentLocation(), Out, 0.5f);
	}
}

// ---------------------------------------------------------------------------
// Death, the sink, coming back
// ---------------------------------------------------------------------------

void ADredgerCreature::OnDied()
{
	DeadTime = 0.f;
	bSinking = false;
	Wants = FPoseWants();
	LooterSound::PlayAt(this, LooterSoundCue::DredgerDeath, GetMesh()->GetSocketLocation(TEXT("head")), 1.f, VoicePitchOf(*this));
	// Shot from in front it goes over backward; from behind, onto its face (the code fall's way; a ragdoll goes as thrown).
	const FCreatureHit& Hit = GetLastHit();
	FallWay = FVector::DotProduct(Hit.Direction, GetActorForwardVector()) > 0.f ? -1.f : 1.f;
	LeakSmog(1.f);
	// The kill's own layers (the gun's, its tier's flare and rings); no splat (a man leaves none).
	CreatureKillLook::Play(*this, Hit, /*bSplat*/ false);
	GoLimp();
}

void ADredgerCreature::LeakSmog(float Strength) const
{
	// What the Company breathes leaks out of the mask's filter as it goes: black-violet smog and a few motes of soul-light.
	FWeaponFX* Effects = EffectsIn(GetWorld());
	if (!Effects || !MaskFilter)
	{
		return;
	}
	const FVector Filter = MaskFilter->GetComponentLocation();
	for (int32 Puff = 0; Puff < 4; ++Puff)
	{
		const FVector Drift(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(40.f, 90.f));
		Effects->SpawnDustPuff(Filter, Drift * Strength, DredgerLook::Smog, 0.75f, 18.f, 70.f * Strength, 1.6f);
	}
	Effects->SpawnSoulTatters(Filter, FVector::UpVector, DredgerLook::SoulCyan, 45.f * Strength, 4);
}

void ADredgerCreature::GoLimp()
{
	USkeletalMeshComponent* Body = GetMesh();
	const UWorld* World = GetWorld();
	// No ragdoll in a test level, without a physics asset or with Looter.Dredger.Body.Ragdoll 0: the code fall instead.
	if (FDredgerTuning::Live().Ragdoll == 0 || !World || !World->IsGameWorld() || !Body->GetPhysicsAsset())
	{
		return;
	}
	bLimp = true;
	// Free of the capsule (whatever the death's corpse knock does with the actor, the body goes as thrown), seen by nothing
	// but the world: shots, pawns and cameras pass through it.
	Body->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Body->SetCollisionProfileName(FName(TEXT("Ragdoll")));
	Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	Body->SetSimulatePhysics(true);
	Body->WakeAllRigidBodies();

	// Thrown along the killing shot by the kill's style: a rifle's crumple, a crit's snap, a blast's throw, an overkill's lift.
	const FCreatureHit& Hit = GetLastHit();
	const FVector Way = Hit.Direction.IsNearlyZero() ? -GetActorForwardVector() : Hit.Direction.GetSafeNormal();
	float Throw = ThrowBody;
	float Lift = 0.f;
	switch (CreatureKillLook::StyleOf(Hit))
	{
	case CreatureKillLook::EStyle::Crit:
		Throw = ThrowCrit;
		break;
	case CreatureKillLook::EStyle::Blast:
		Throw = ThrowBlast;
		Lift = LiftBlast;
		break;
	case CreatureKillLook::EStyle::Overkill:
		Throw = ThrowOverkill;
		Lift = LiftOverkill;
		break;
	case CreatureKillLook::EStyle::Body:
	default:
		break;
	}
	Body->SetAllPhysicsLinearVelocity(Way * (Throw * 0.35f) + FVector::UpVector * (Lift * 0.4f));
	const FName Bone = Hit.Bone.IsNone() ? FName(TEXT("spine_03")) : Hit.Bone;
	Body->AddImpulseToAllBodiesBelow(Way * Throw + FVector::UpVector * Lift, Bone, /*bVelChange*/ true, /*bIncludeSelf*/ true);
}

void ADredgerCreature::TickCorpse(float DeltaSeconds)
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!bLimp)
	{
		// The code fall over its feet, then the body sinks as the others' do.
		BodyInput.Fall = FMath::Clamp(DeadTime / FallSeconds, 0.f, 1.f);
		BodyInput.FallWay = FallWay;
		BodyInput.Speed = 0.f;
		BodyInput.Flinch = FVector::ZeroVector;
		BodyInput.HeadWhip = FVector2D::ZeroVector;
		if (DeadTime >= CorpseTime)
		{
			Body->AddRelativeLocation(FVector(0.f, 0.f, -SinkSpeed * DeltaSeconds));
		}
		return;
	}
	// Limp: once it has lain a while it sinks through the ground (nothing holds it), before its actor is hidden.
	if (!bSinking && DeadTime >= CorpseTime)
	{
		bSinking = true;
		Body->SetCollisionResponseToAllChannels(ECR_Ignore);
		Body->SetEnableGravity(false);
	}
	if (bSinking)
	{
		Body->SetAllPhysicsLinearVelocity(FVector(0.f, 0.f, -SinkSpeed));
	}
}

void ADredgerCreature::OnRespawned()
{
	USkeletalMeshComponent* Body = GetMesh();
	if (bLimp)
	{
		Body->SetSimulatePhysics(false);
		Body->SetEnableGravity(true);
		Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Body->SetCollisionObjectType(ECC_WorldDynamic);
		Body->SetCollisionResponseToAllChannels(ECR_Ignore);
		Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
		Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		bLimp = false;
	}
	Body->SetRelativeTransform(MeshRest);
	bSinking = false;
	DeadTime = 0.f;
	Wants = FPoseWants();
	FlinchSpring.Reset();
	HeadSpring.Reset();
	CrouchNow = 0.f;
	LeanNow = 0.f;
	BraceNow = 0.f;
	ReloadWeightNow = 0.f;
	Shouldered = 0.f;
	BrokenLenses = 0;
	const bool bKit = FDredgerTuning::Live().Kit != 0;
	LensLeft->SetVisibility(bKit);
	LensRight->SetVisibility(bKit);
	BodyInput.Fall = 0.f;
}
