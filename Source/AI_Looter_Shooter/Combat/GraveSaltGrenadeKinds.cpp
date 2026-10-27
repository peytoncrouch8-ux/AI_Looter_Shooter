#include "Combat/GraveSaltGrenade.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltCinderBed.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/GraveSaltTuning.h"
#include "Combat/GrenadeKinds.h"
#include "Combat/GrenadeKindsTuning.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

// AGraveSaltGrenade's kinds (round 4, GrenadeKinds): the tin each kind wears, the pitch jar sticking, and what each does
// after its burst (the pitch's tar, the scatter's brood, the cinder's bed). The flight, fuse and burst are
// GraveSaltGrenade.cpp's.

namespace
{
	/** The engine's cylinder stands in, sized like the tin, until a tin is imported. */
	const TCHAR* StandInPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const FVector StandInScale(FGraveSaltRules::JarRadius * 0.02f, FGraveSaltRules::JarRadius * 0.02f, FGraveSaltRules::JarMiddle * 0.02f);

	/**
	 * A small jar of the scatter's brood: half the tin's size (7.5 cm across), sweeping a 5 cm ball (between its half
	 * width and its middle's height, as the whole jar's 9 cm is).
	 */
	constexpr float BroodScale = 0.5f;
	constexpr float BroodBallRadius = 5.f;
	/** A small jar is flung from this far off the burst's surface, clear of what it burst on. */
	constexpr float BroodLiftOff = 15.f;
	/** A pitch jar sits this far off what it stuck to (cm): its tin on the surface, not in it. */
	constexpr float StickStandOff = 2.f;
	/** The stick is heard this loud at least, louder the faster it hit (cm/s for full). */
	constexpr float StickQuietest = 0.55f;
	constexpr float StickFullSpeed = 1200.f;
}

UStaticMesh* AGraveSaltGrenade::FindJarMesh()
{
	return FindJarMesh(EGrenadeKind::Salt);
}

UStaticMesh* AGraveSaltGrenade::FindJarMesh(EGrenadeKind Kind)
{
	// Looked up when first needed rather than in a constructor, so a build before the models' import only stands in the
	// salt tin (or a cylinder) instead of failing to find it. /Game/Art/Throwables is cooked whole (DefaultGame.ini).
	// Looked for again while one isn't there (once a throw at most), so an import during a session is picked up.
	static TWeakObjectPtr<UStaticMesh> Meshes[GrenadeKinds::Num];
	const int32 Index = FMath::Clamp(static_cast<int32>(Kind), 0, GrenadeKinds::Num - 1);
	if (!Meshes[Index].IsValid())
	{
		Meshes[Index] = LoadObject<UStaticMesh>(nullptr, GrenadeKinds::MeshPath(Kind), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (UStaticMesh* Mesh = Meshes[Index].Get())
	{
		return Mesh;
	}
	return Kind == EGrenadeKind::Salt ? nullptr : FindJarMesh(EGrenadeKind::Salt);
}

AGraveSaltGrenade* AGraveSaltGrenade::ThrowBrood(UWorld* World, const FVector& Start, const FVector& StartVelocity, APawn* By, float ByLevelScale,
	float Fuse)
{
	if (!World)
	{
		return nullptr;
	}
	const FTransform Transform(StartVelocity.Rotation(), Start);
	AGraveSaltGrenade* Brood = World->SpawnActorDeferred<AGraveSaltGrenade>(AGraveSaltGrenade::StaticClass(), Transform, By, By,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Brood)
	{
		Brood->SetKind(EGrenadeKind::Scatter, true);
		Brood->FuseSeconds = FMath::Max(Fuse, 0.05f);
		Brood->FinishSpawning(Transform);
		Brood->Launch(StartVelocity, By, ByLevelScale);
	}
	return Brood;
}

void AGraveSaltGrenade::SetKind(EGrenadeKind InKind, bool bInBrood)
{
	Kind = InKind;
	bBrood = bInBrood;
	BallRadius = bBrood ? BroodBallRadius : FGraveSaltRules::CollisionRadius;
	if (HasActorBegunPlay())
	{
		ApplyJarMesh();
	}
}

void AGraveSaltGrenade::ApplyJarMesh()
{
	const float Scale = bBrood ? BroodScale : 1.f;
	// A small jar is a plain little salt tin (the scatter jar's own tin, capsules and all, would read as a toy of it).
	if (UStaticMesh* Mesh = FindJarMesh(bBrood ? EGrenadeKind::Salt : Kind))
	{
		Jar->SetStaticMesh(Mesh);
		Jar->SetRelativeScale3D(FVector(Scale));
	}
	else
	{
		Jar->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, StandInPath));
		Jar->SetRelativeScale3D(StandInScale * Scale);
	}
	// About its middle, not its base: the pivot is under the tin.
	Jar->SetRelativeLocation(FVector(0.f, 0.f, -FGraveSaltRules::JarMiddle * Scale));
}

void AGraveSaltGrenade::StickTo(const FHitResult& Hit)
{
	const FVector Normal = Hit.ImpactNormal.IsNearlyZero() ? FVector::UpVector : FVector(Hit.ImpactNormal.GetSafeNormal());
	const float Speed = static_cast<float>(Velocity.Size());
	SetActorLocation(Hit.Location + Normal * StickStandOff);
	Velocity = FVector::ZeroVector;
	bStuck = true;
	StickNormal = Normal;
	LooterSound::PlayAt(this, GrenadeKindCue::PitchStick, GetActorLocation(), FMath::GetMappedRangeValueClamped(FVector2f(0.f, StickFullSpeed),
		FVector2f(StickQuietest, 1.f), Speed));

	// What it holds on to: a creature (its bone, so it rides the body as it moves), or anything else that moves; the
	// ground and walls hold it where it is.
	UPrimitiveComponent* Surface = Hit.GetComponent();
	AActor* HitActor = Hit.GetActor();
	const bool bCreature = HitActor && GraveSaltBurst::CanHurt(HitActor, Thrower.Get());
	if (Surface && Surface->Mobility == EComponentMobility::Movable)
	{
		AttachToComponent(Surface, FAttachmentTransformRules::KeepWorldTransform, Hit.BoneName);
	}
	if (bCreature)
	{
		StuckTo = HitActor;
		// On a body its burst rises from where it is, whatever way that face looks.
		StickNormal = FVector::UpVector;
		// It won't ride a charge all the way to the player: it goes within the stuck fuse.
		FuseSeconds = FMath::Min(FuseSeconds, Age + FGrenadeKindTuning::Current().PitchStuckFuse);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Pitch jar: stuck to %s after %.2f s"), *GetNameSafe(HitActor), Age);
}

FVector AGraveSaltGrenade::BurstUp() const
{
	return bStuck ? StickNormal : FVector::UpVector;
}

void AGraveSaltGrenade::AfterBurst(const FGraveSaltBurstResult& Result, const FVector& Up)
{
	UWorld* World = GetWorld();
	if (!World || bBrood)
	{
		return;
	}
	const FGrenadeKindTuning Kinds = FGrenadeKindTuning::Current();
	switch (Kind)
	{
	case EGrenadeKind::Pitch:
		// The tar: what the burst hurt and didn't kill walks slowly a while (never a boss: a boss fight's pace is its own).
		for (const FHitResult& Hit : Result.HitResults)
		{
			ACharacter* Body = Cast<ACharacter>(Hit.GetActor());
			const UHealthComponent* Health = Body ? Body->FindComponentByClass<UHealthComponent>() : nullptr;
			if (Health && !Health->IsDead() && !GraveSaltBurst::IsBoss(Body) && Kinds.PitchSlowSeconds > 0.f)
			{
				UMovementSlowComponent::Apply(Body, Kinds.PitchSlow, Kinds.PitchSlowSeconds);
			}
		}
		break;
	case EGrenadeKind::Scatter:
	{
		// The brood: small jars flung out round it, each on its own fuse so they pop one after another.
		TArray<FVector> Launches;
		GrenadeKinds::BroodVelocities(Kinds.ScatterCount, Up, Kinds.BroodSpeed, Kinds.BroodLift, Random, Launches);
		const FVector From = GetActorLocation() + Up * BroodLiftOff;
		for (int32 Index = 0; Index < Launches.Num(); ++Index)
		{
			ThrowBrood(World, From, Launches[Index], Thrower.Get(), LevelScale, GrenadeKinds::BroodFuse(Index, Launches.Num(), Random.FRand(), Kinds));
		}
		break;
	}
	case EGrenadeKind::Cinder:
		// The coals: a bed of them burning on the ground under it for a few seconds.
		AGraveSaltCinderBed::Spawn(World, GetActorLocation(), Thrower.Get(), LevelScale);
		break;
	default:
		break;
	}
}
