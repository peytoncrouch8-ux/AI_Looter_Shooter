// ASpiderCreature's ambush, once it springs (SpiderCreatureLurk.cpp has the perch, the waiting and the spring): its leap off its
// surface (the movement's own fall, launched to land short of its prey: SpiderAmbushRules::LeapVelocity; its aim fixed as
// it leaves), its scramble head first down a wall (glued to the surface as it goes, hopping off once low, or off the end of
// the surface), its landing (a bite if it came down on them, a thump, dust), and its death on the wall or in the air (the
// body brought down to the ground under it, its death playing from where it hung).

#include "Creatures/SpiderCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureReactComponent.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Its capsule clear of the surface by this much (cm) on its perch and as it leaves. */
	constexpr float PerchClear = 4.f;
	constexpr float LaunchClear = 8.f;
	/** A hop off its surface with nobody to go for lands this far out from under it (cm at size 1). */
	constexpr float HopOut = 120.f;
	/** Climbing: its glue to the surface looks this far out and in (cm at size 1); its look down is this often (s); it gives up after this long (s). */
	constexpr float ClimbLookOut = 80.f;
	constexpr float ClimbLookIn = 140.f;
	constexpr float ClimbGroundEvery = 0.1f;
	constexpr float ClimbMostSeconds = 8.f;
	constexpr float ClimbCueEvery = 0.45f;
	/** A landing throws dust from under it here (the body's frame at size 1: ahead, out to each side). */
	const FVector2D LandDust[2] = { FVector2D(45.f, 45.f), FVector2D(-60.f, 40.f) };
	constexpr float LandDustThrow = 260.f;
	/** Its head eased back flat to the surface as it climbs (1/s). */
	constexpr float RaiseRate = 3.f;
}

// ---------------------------------------------------------------------------
// The leap, the climb down, the landing
// ---------------------------------------------------------------------------

void ASpiderCreature::Launch()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Movement || !Capsule)
	{
		EndLurk();
		return;
	}
	const FSpiderLurkTuning Tuning = FSpiderLurkTuning::Live();
	const float Scale = GetSizeScale();
	const float Half = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const bool bLedge = Lurk.Surface == ESpiderSurface::Ledge;
	FVector From = bLedge ? Lurk.Point + FVector(0.0, 0.0, Half + LaunchClear) : Lurk.Point + Lurk.Normal * (Radius + LaunchClear);
	// Low on a wall (a climb's end) its capsule mustn't start in the ground.
	FVector Ground;
	if (FindGround(From, 0.f, Half + 10.f, Ground))
	{
		From.Z = FMath::Max(From.Z, Ground.Z + Half + LaunchClear);
	}
	// Where it comes down: short of its prey if it can reach them, else on the ground out from under it.
	APawn* Prey = Lurk.Prey.Get();
	FVector Landing;
	const bool bAtPrey = Prey && !Prey->IsActorBeingDestroyed()
		&& FVector::Dist(From, Prey->GetActorLocation()) <= Tuning.LeapReach * Scale;
	if (bAtPrey)
	{
		const FVector PreyFeet = Prey->GetActorLocation() - FVector(0.0, 0.0, Prey->GetSimpleCollisionHalfHeight());
		Landing = SpiderAmbushRules::LeapLanding(From, PreyFeet, Prey->GetVelocity(), Scale, Tuning);
	}
	else
	{
		FVector Out(Lurk.Normal.X, Lurk.Normal.Y, 0.0);
		if (!Out.Normalize())
		{
			Out = SpiderAmbushRules::FaceWay(Lurk.Perch.FaceYaw);
		}
		Landing = Lurk.Foot + Out * (HopOut * Scale);
	}
	if (FindGround(Landing + FVector(0.0, 0.0, 120.0), 120.f, 600.f, Ground))
	{
		Landing = Ground;
	}
	const FVector To = Landing + FVector(0.0, 0.0, Half + 2.f);
	const float Seconds = SpiderAmbushRules::LeapSeconds(From, To);
	const FVector Launched = SpiderAmbushRules::LeapVelocity(From, To, Seconds, Movement->GetGravityZ());
	// The body leaves from how it clung (AnimateBody turns it off the surface over the leap's first moments).
	Lurk.LeftFrom = BodyFrame.GetRotation();
	Lurk.LeftAt = BodyFrame.GetLocation();
	Lurk.LeapFrom = From;
	Lurk.LeapTo = To;
	BodyZ = static_cast<float>(Lurk.LeftAt.Z);
	BodyPitch = 0.f;
	BodyRoll = 0.f;
	bBodyInitialized = true;
	for (FLeg& Leg : Legs)
	{
		Leg.HeldVelocity = FVector::ZeroVector;
		Leg.bStepping = false;
	}
	SteppingGroup = INDEX_NONE;
	const FVector Facing = (To - From).GetSafeNormal2D();
	const float Yaw = Facing.IsNearlyZero() ? static_cast<float>(GetActorRotation().Yaw) : static_cast<float>(Facing.Rotation().Yaw);
	SetActorLocationAndRotation(From, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = Launched;
	Lurk.Phase = ESpiderLurkPhase::Leaping;
	Lurk.Clock = 0.f;
	Gait.bRead = false;
}

void ASpiderCreature::TickClimb(float DeltaSeconds)
{
	// Its own climb's speed and hop (a den spider's slower: SpiderCreatureDen.cpp), else the lurk's.
	const FSpiderLurkTuning Tuning = SpiderAmbushRules::ClimbTuning(Lurk);
	const float Scale = GetSizeScale();
	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!World || !Capsule)
	{
		Launch();
		return;
	}
	// Head first down its surface at a scramble, glued to it: the surface looked for again under where it's got to.
	const FVector Next = Lurk.Point + Lurk.Forward * (Tuning.ClimbSpeed * Scale * DeltaSeconds);
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SpiderClimb"), this, /*bTraceComplex*/ true);
	FHitResult Hit;
	const bool bHeld = World->LineTraceSingleByObjectType(Hit, Next + Lurk.Normal * (ClimbLookOut * Scale), Next - Lurk.Normal * (ClimbLookIn * Scale),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params) && !Hit.bStartPenetrating;
	const ESpiderSurface Surface = bHeld ? SpiderAmbushRules::SurfaceOf(Hit.ImpactNormal) : ESpiderSurface::None;
	if (Surface != ESpiderSurface::Wall && Surface != ESpiderSurface::Overhang)
	{
		// Off the end of its surface (a lip, a ledge, the ground's slope): it jumps the rest.
		Launch();
		return;
	}
	Lurk.Point = Hit.ImpactPoint;
	Lurk.Normal = FMath::Lerp(Lurk.Normal, FVector(Hit.ImpactNormal), static_cast<double>(FMath::Min(8.f * DeltaSeconds, 1.f))).GetSafeNormal();
	Lurk.Forward = SpiderAmbushRules::ClingForward(Lurk.Normal, Lurk.Perch.Kind, Lurk.Perch.FaceYaw);
	Lurk.Watch = Lurk.Forward;
	Lurk.Raise = FMath::FInterpTo(Lurk.Raise, 0.f, DeltaSeconds, RaiseRate);
	SetActorLocation(Lurk.Point + Lurk.Normal * (Capsule->GetScaledCapsuleRadius() + PerchClear), false, nullptr, ETeleportType::TeleportPhysics);
	// Claws on rock as it goes.
	Lurk.ClimbCueIn -= DeltaSeconds;
	if (Lurk.ClimbCueIn <= 0.f)
	{
		Lurk.ClimbCueIn = ClimbCueEvery;
		Voice->Play(FName(SpiderAmbushCue::Climb), 0.8f);
	}
	// Low enough over the ground, it hops off (at its prey if it can reach them).
	Lurk.ClimbLookIn -= DeltaSeconds;
	if (Lurk.ClimbLookIn <= 0.f)
	{
		Lurk.ClimbLookIn = ClimbGroundEvery;
		FVector Out(Lurk.Normal.X, Lurk.Normal.Y, 0.0);
		Out = Out.GetSafeNormal();
		const FVector Body = Lurk.Point + Lurk.Normal * (Tuning.ClingRide * Scale);
		FVector Ground;
		Lurk.ClimbOver = FindGround(Body + Out * (40.f * Scale), 20.f, 3000.f, Ground) ? static_cast<float>(Body.Z - Ground.Z) : 3000.f;
	}
	if (SpiderAmbushRules::ClimbDone(Lurk.ClimbOver, Scale, Tuning) || Lurk.Clock > ClimbMostSeconds)
	{
		Launch();
	}
}

void ASpiderCreature::Land()
{
	const FSpiderLurkTuning Tuning = FSpiderLurkTuning::Live();
	const float Scale = GetSizeScale();
	Lurk.Phase = ESpiderLurkPhase::Landing;
	Lurk.Clock = 0.f;
	const FVector Here = GetActorLocation();
	const FVector Feet = Here - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	APawn* Prey = Lurk.Prey.Get();
	if (Prey && IsValidTarget(Prey))
	{
		const FVector PreyAt = Prey->GetActorLocation();
		const FVector Toward = (PreyAt - Here).GetSafeNormal2D();
		if (!Toward.IsNearlyZero())
		{
			SetActorRotation(FRotator(0.f, static_cast<float>(Toward.Rotation().Yaw), 0.f));
		}
		// Down on them: a bite as it lands (its fangs into them, a shove), at a share of its bite.
		const FVector PreyFeet = PreyAt - FVector(0.0, 0.0, Prey->GetSimpleCollisionHalfHeight());
		if (SpiderAmbushRules::LandingBites(Feet, PreyFeet, Scale, Tuning))
		{
			HitWithAttack(Prey, Toward, Tuning.LeapBiteShare);
			Voice->Play(FName(LooterSoundCue::SpiderAttack));
			FangOpen = 0.f;
		}
	}
	// A heavy landing: its thump, eight feet striking, grit thrown out from under it (the dip is the gait's: PlantAfterFall).
	Voice->Play(FName(SpiderAmbushCue::Land));
	if (UCreatureReactComponent* Reacts = GetReact())
	{
		const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
		for (const FVector2D& Spot : LandDust)
		{
			for (const float Side : { -1.f, 1.f })
			{
				const FVector Out = Yaw.RotateVector(FVector(Spot.X, Spot.Y * Side, 0.f).GetSafeNormal());
				Reacts->KickFootfall(Feet + Yaw.RotateVector(FVector(Spot.X, Spot.Y * Side, 0.f) * Scale), Out * LandDustThrow, Scale);
			}
		}
	}
}

void ASpiderCreature::EndLurk()
{
	Lurk.Phase = ESpiderLurkPhase::None;
	Lurk.Clock = 0.f;
	// Let go of its surface (a leap that never came down): the movement takes it from here.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement && Movement->MovementMode == MOVE_None && !IsDead())
	{
		Movement->SetMovementMode(MOVE_Falling);
	}
	Gait.bRead = false;
	// Down out of a den: now it hunts (SpiderCreatureDen.cpp).
	if (Lurk.bFromDen)
	{
		FinishDenEmerge();
	}
}

void ASpiderCreature::FallFromPerch()
{
	// Killed on its surface or in the air: the actor (its death's ground) set on the ground under its body, its body kept
	// where it hung, so its death's throe and drop play from there down (SpiderCreatureDeath.cpp: DeathHeight from BodyZ).
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Half = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 62.f;
	const FTransform Hung = BodyFrame;
	const FVector At = Hung.GetLocation();
	FVector Out(Lurk.Normal.X, Lurk.Normal.Y, 0.0);
	Out = Out.GetSafeNormal();
	FVector Ground;
	if (!FindGround(At, 30.f, 4000.f, Ground) && !FindGround(At + Out * (60.f * GetSizeScale()), 30.f, 4000.f, Ground))
	{
		Ground = Lurk.Foot;
	}
	const FRotator Pose = Hung.Rotator();
	SetActorLocationAndRotation(FVector(At.X, At.Y, Ground.Z + Half), FRotator(0.f, Pose.Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	ComponentToWorld = GetMesh()->GetComponentTransform();
	BodyZ = static_cast<float>(At.Z);
	BodyPitch = static_cast<float>(Pose.Pitch);
	BodyRoll = static_cast<float>(Pose.Roll);
	bBodyInitialized = true;
	LungeNow = 0.f;
	HurtOffset = HurtVelocity = FVector::ZeroVector;
	Lurk.Phase = ESpiderLurkPhase::None;
}
