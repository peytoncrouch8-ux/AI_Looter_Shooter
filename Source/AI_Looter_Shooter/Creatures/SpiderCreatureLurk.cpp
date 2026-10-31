// ASpiderCreature's ambush (the user's pick, 2026-10-31: "spiders drop from walls/ledges"): a lurker holds its perch on a cliff
// face, a rock wall, a ledge's edge or a tree's trunk (its pose there: SpiderCreatureCling.cpp), still, its brain waiting,
// watching the player come; as they pass near or under it, walk on past it, stare at it, linger, or shoot it, it springs:
// a crouch and a shriek (the tell), a leap onto them (a bite if it lands on them) or, out of a leap's reach, a scramble down
// its wall (the leap, the climb and the landing: SpiderCreatureLeap.cpp); its mates follow a moment apart. Then it fights as
// any spider. The rules are SpiderAmbushRules'; the ambush that places them is AAmbushSpawner (Entrance Lurk, its Perches).
//
// Why it's built this way:
// - Held, not walking: its movement is off (MOVE_None) and its brain held each frame (HoldBrain), so nothing of the base's
//   hunt, spacing or unstick moves it off its wall; its own looks decide when it goes. A shot or its pack's call still
//   reach it (OnHurt, AlertTo), and its looks see them.
// - The leap is the movement's own fall with a launch worked out to land short of the player (SpiderAmbushRules::LeapVelocity),
//   so walls and the player's capsule still stop it; its aim is fixed as it leaves (dodgeable, as the Unpaid's lunge).
// - Killed on its wall or in the air its body comes down to the ground under it and its death plays from where it hung.

#include "Creatures/SpiderCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/AmbushSpawner.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/EncounterSubsystem.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Its looks at the player while it waits (s apart). */
	constexpr float SenseInterval = 0.15f;
	/** A wall's face is looked for back along its facing from this far out (cm), this far in (a tree's trunk: TrunkReach). */
	constexpr float FaceLookOut = 150.f;
	constexpr float FaceLookIn = 300.f;
	constexpr float TrunkReach = 150.f;
	/** Tries round the spot: up and down (cm), and turned (degrees). */
	constexpr float TryHeights[] = { 0.f, 60.f, -60.f, 120.f };
	constexpr float TryTurns[] = { 0.f, 25.f, -25.f, 50.f, -50.f };
	/** A ledge's top is looked for from this far over the spot down to this far under it (cm). */
	constexpr float LedgeLookAbove = 300.f;
	constexpr float LedgeLookBelow = 500.f;
	/** Its capsule clear of the surface on its perch (cm). */
	constexpr float PerchClear = 4.f;
	/** Its head lifted off the surface (degrees) as it watches someone near, and in its coil; eased at this rate (1/s). */
	constexpr float WatchRaise = 10.f;
	constexpr float CoilRaise = 16.f;
	constexpr float RaiseRate = 3.f;
}

bool ASpiderCreature::IsClinging() const
{
	return Lurk.Phase == ESpiderLurkPhase::Perched || Lurk.Phase == ESpiderLurkPhase::Coiling || Lurk.Phase == ESpiderLurkPhase::Climbing;
}

// ---------------------------------------------------------------------------
// Taking its perch
// ---------------------------------------------------------------------------

bool ASpiderCreature::FindPerchSurface(const FSpiderPerchSpot& Perch, FSpiderLurkState& Out) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	// The world's static geometry: the terrain, the cliff kit, rocks and trees placed by the build (volumes skipped).
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SpiderPerch"), this, /*bTraceComplex*/ true);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	FHitResult Hit;
	if (Perch.Kind == ESpiderPerchKind::Ledge)
	{
		if (World->LineTraceSingleByObjectType(Hit, Perch.Spot + FVector(0.0, 0.0, LedgeLookAbove), Perch.Spot - FVector(0.0, 0.0, LedgeLookBelow),
			Objects, Params) && !Hit.bStartPenetrating && SpiderAmbushRules::SurfaceOf(Hit.ImpactNormal) == ESpiderSurface::Ledge)
		{
			Out.Point = Hit.ImpactPoint;
			Out.Normal = Hit.ImpactNormal;
			Out.Surface = ESpiderSurface::Ledge;
			return true;
		}
		return false;
	}
	// A wall or a trunk: looked back at from in front of it; the face is rough and a trunk is thin, so a few tries round the spot.
	const FVector Way = SpiderAmbushRules::FaceWay(Perch.FaceYaw);
	const float Reach = Perch.Kind == ESpiderPerchKind::Tree ? TrunkReach : FaceLookIn;
	for (const float Height : TryHeights)
	{
		for (const float Turn : TryTurns)
		{
			const FVector Look = FRotator(0.f, Turn, 0.f).RotateVector(Way);
			const FVector Aim = Perch.Spot + FVector(0.0, 0.0, Height);
			if (!World->LineTraceSingleByObjectType(Hit, Aim + Look * FaceLookOut, Aim - Look * Reach, Objects, Params) || Hit.bStartPenetrating)
			{
				continue;
			}
			const ESpiderSurface Surface = SpiderAmbushRules::SurfaceOf(Hit.ImpactNormal);
			if (Surface == ESpiderSurface::Wall || Surface == ESpiderSurface::Overhang)
			{
				Out.Point = Hit.ImpactPoint;
				Out.Normal = Hit.ImpactNormal;
				Out.Surface = Surface;
				return true;
			}
		}
	}
	return false;
}

bool ASpiderCreature::BeginLurk(const FSpiderPerchSpot& Perch, AAmbushSpawner* LurkOwner)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (IsDead() || !Movement || !Capsule)
	{
		return false;
	}
	const float Half = Capsule->GetScaledCapsuleHalfHeight();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, Half);
	Lurk = FSpiderLurkState();
	Lurk.Perch = Perch;
	Lurk.Owner = LurkOwner;
	const bool bFound = FindPerchSurface(Perch, Lurk);
	if (!bFound)
	{
		// Nothing to hold near its spot: it lurks crouched low on the ground where it was spawned, as still.
		Lurk.Point = Feet;
		Lurk.Normal = FVector::UpVector;
		Lurk.Surface = ESpiderSurface::Ledge;
		Lurk.Perch.Kind = ESpiderPerchKind::Ledge;
		Lurk.Perch.FaceYaw = static_cast<float>(GetActorRotation().Yaw);
	}
	Lurk.Forward = SpiderAmbushRules::ClingForward(Lurk.Normal, Lurk.Perch.Kind, Lurk.Perch.FaceYaw);
	Lurk.Watch = Lurk.Forward;
	// The ground its distances are measured from: under the perch, out from its surface (before a ledge, over its edge).
	FVector Out(Lurk.Normal.X, Lurk.Normal.Y, 0.0);
	if (!Out.Normalize() || Lurk.Surface == ESpiderSurface::Ledge)
	{
		Out = SpiderAmbushRules::FaceWay(Lurk.Perch.FaceYaw);
	}
	FVector Ground;
	Lurk.Foot = bFound && FindGround(Lurk.Point + Out * (Radius + 40.f), 100.f, 3000.f, Ground) ? Ground : Feet;
	// Held there: no walking or falling, its brain waiting (TickLurk), its capsule just off the surface.
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	const FVector Place = Lurk.Surface == ESpiderSurface::Ledge ? Lurk.Point + FVector(0.0, 0.0, Half + PerchClear)
		: Lurk.Point + Lurk.Normal * (Radius + PerchClear);
	SetActorLocationAndRotation(Place, FRotator(0.f, static_cast<float>(Out.Rotation().Yaw), 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	Lurk.Phase = ESpiderLurkPhase::Perched;
	Lurk.SenseIn = GetReact() ? GetReact()->GetRandom().FRandRange(0.f, SenseInterval) : 0.f;
	HoldBrain(0.25f);
	if (bRigReady)
	{
		PlantOnSurface();
		AnimateCling(0.f);
	}
	UE_LOG(LogLooter, Log, TEXT("%s lurks %s at (%.0f, %.0f, %.0f), %.0f m over the ground under it."), *GetName(),
		!bFound ? TEXT("on the ground (no surface near its perch)") : Lurk.Surface == ESpiderSurface::Ledge ? TEXT("on a ledge") : TEXT("on a wall"),
		Lurk.Point.X, Lurk.Point.Y, Lurk.Point.Z, (Lurk.Point.Z - Lurk.Foot.Z) / 100.0);
	return bFound;
}

void ASpiderCreature::NoteMateSprang(APawn* Prey, float Delay)
{
	if (Lurk.Phase == ESpiderLurkPhase::Perched && Lurk.CallIn < 0.f)
	{
		Lurk.CallIn = FMath::Max(Delay, 0.f);
		Lurk.Prey = Prey;
	}
}

void ASpiderCreature::DevSpring(APawn* Prey)
{
	if (Lurk.Phase != ESpiderLurkPhase::Perched)
	{
		return;
	}
	const FSpiderLurkTuning Tuning = FSpiderLurkTuning::Live();
	const FVector Body = Lurk.Point + Lurk.Normal * (Tuning.ClingRide * GetSizeScale());
	const float Distance = Prey ? static_cast<float>(FVector::Dist(Body, Prey->GetActorLocation())) : TNumericLimits<float>::Max();
	const float Rise = Prey ? static_cast<float>(Prey->GetActorLocation().Z - Lurk.Foot.Z) : 0.f;
	const bool bLeap = SpiderAmbushRules::CanLeap(Distance, Rise, Prey != nullptr, GetSizeScale(), Tuning) || Lurk.Surface == ESpiderSurface::Ledge;
	Spring(Prey, bLeap ? ESpiderSpring::Leap : ESpiderSpring::Climb);
}

// ---------------------------------------------------------------------------
// Waiting, and springing
// ---------------------------------------------------------------------------

void ASpiderCreature::TickLurk(float DeltaSeconds)
{
	if (Lurk.Phase == ESpiderLurkPhase::None)
	{
		return;
	}
	if (IsDead())
	{
		// Its death took it down (OnDied: FallFromPerch).
		Lurk.Phase = ESpiderLurkPhase::None;
		return;
	}
	const FSpiderLurkTuning Tuning = FSpiderLurkTuning::Live();
	const float Scale = GetSizeScale();
	Lurk.Clock += DeltaSeconds;
	Lurk.SinceCue += DeltaSeconds;
	switch (Lurk.Phase)
	{
	case ESpiderLurkPhase::Perched:
	{
		if (Lurk.CallIn >= 0.f)
		{
			Lurk.CallIn = FMath::Max(Lurk.CallIn - DeltaSeconds, 0.f);
		}
		Lurk.SenseIn -= DeltaSeconds;
		const bool bDue = Lurk.bHurt || Lurk.CallIn == 0.f || Lurk.SenseIn <= 0.f;
		if (bDue)
		{
			const float Interval = SenseInterval - FMath::Min(Lurk.SenseIn, 0.f);
			Lurk.SenseIn = SenseInterval;
			APawn* Prey = nullptr;
			const FSpiderLurkSense Sense = SenseLurk(Interval, Prey);
			const bool bGo = Tuning.Enabled == 0 || SpiderAmbushRules::WantsToSpring(Sense, Tuning);
			if (bGo && Prey)
			{
				Spring(Prey, SpiderAmbushRules::ChooseSpring(Sense, Lurk.Perch.Kind, Scale, Tuning));
				break;
			}
			if (Lurk.bHurt)
			{
				// Shot by someone it can't go for (off its ground, out of sight): down off its surface, to hunt from the ground.
				Spring(nullptr, Lurk.Surface == ESpiderSurface::Ledge ? ESpiderSpring::Leap : ESpiderSpring::Climb);
				break;
			}
			if (Lurk.CallIn == 0.f && !Prey)
			{
				// A mate's call at a player it can't go for (off its ground): it stays where it is.
				Lurk.CallIn = -1.f;
			}
		}
		// Watching: its head turned along the surface toward the player near it, lifted off it a little; else back to its hang.
		const bool bWatching = !Lurk.LastPreySpot.IsZero() && FVector::Dist2D(Lurk.Foot, Lurk.LastPreySpot) <= Tuning.WatchRadius * Scale;
		const FVector Body = Lurk.Point + Lurk.Normal * (Tuning.ClingRide * Scale);
		Lurk.Watch = SpiderAmbushRules::TurnWatch(Lurk.Watch, Lurk.Forward, bWatching ? Lurk.LastPreySpot - Body : Lurk.Forward, Lurk.Normal,
			Tuning.WatchMost, Tuning.WatchRate * DeltaSeconds);
		Lurk.Raise = FMath::FInterpTo(Lurk.Raise, bWatching ? WatchRaise : 0.f, DeltaSeconds, RaiseRate);
		break;
	}
	case ESpiderLurkPhase::Coiling:
		// Pressed to its surface, head up, fangs wide: its shriek is the player's warning (SpiderAmbushCue::Leap).
		Lurk.Raise = FMath::FInterpTo(Lurk.Raise, CoilRaise, DeltaSeconds, RaiseRate * 3.f);
		if (Lurk.Clock >= Tuning.CoilSeconds)
		{
			Launch();
		}
		break;
	case ESpiderLurkPhase::Leaping:
	{
		const UCharacterMovementComponent* Movement = GetCharacterMovement();
		if (Movement && Movement->IsMovingOnGround() && Lurk.Clock > 0.05f)
		{
			Land();
		}
		else if (Lurk.Clock > Tuning.LeapTimeout)
		{
			EndLurk();
		}
		break;
	}
	case ESpiderLurkPhase::Climbing:
		TickClimb(DeltaSeconds);
		break;
	case ESpiderLurkPhase::Landing:
		if (Lurk.Clock >= Tuning.LandRecover)
		{
			EndLurk();
		}
		break;
	case ESpiderLurkPhase::DenHidden:
	case ESpiderLurkPhase::DenCrawl:
	case ESpiderLurkPhase::DenTurn:
		// Coming out of a den in a wall (SpiderCreatureDen.cpp); its climb down is the Climbing above.
		TickDen(DeltaSeconds);
		break;
	default:
		break;
	}
	// Its brain waits through all of it (the base's hunt would walk it off its wall or steer it in the air).
	if (Lurk.Phase != ESpiderLurkPhase::None)
	{
		HoldBrain(DeltaSeconds + 0.05f);
	}
}

FSpiderLurkSense ASpiderCreature::SenseLurk(float Interval, APawn*& OutPrey)
{
	const FSpiderLurkTuning Tuning = FSpiderLurkTuning::Live();
	const float Scale = GetSizeScale();
	FSpiderLurkSense Sense;
	Sense.SpringRadius = Lurk.Perch.SpringRadius;
	Sense.bHurt = Lurk.bHurt;
	Sense.bCalled = Lurk.CallIn == 0.f;
	Sense.bHunting = GetTarget() != nullptr;
	OutPrey = nullptr;
	UWorld* World = GetWorld();
	APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	// Its prey: whoever its pack or a shot set it on, a mate's call's, else the player (the encounters' player: a test
	// level's stand-in too).
	APawn* Pawn = GetTarget();
	if (!Pawn && Sense.bCalled)
	{
		Pawn = Lurk.Prey.Get();
	}
	if (!Pawn)
	{
		const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
		Pawn = Encounters ? static_cast<APawn*>(Encounters->GetPlayer()) : (Player ? static_cast<APawn*>(Player->GetPawn()) : nullptr);
	}
	if (!Pawn || !IsValidTarget(Pawn))
	{
		Lurk.LookedAt = 0.f;
		Lurk.Still = 0.f;
		return Sense;
	}
	const FVector Body = Lurk.Point + Lurk.Normal * (Tuning.ClingRide * Scale);
	const FVector PreyAt = Pawn->GetActorLocation();
	const FVector PreyFeet = PreyAt - FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight());
	Sense.FlatDistance = static_cast<float>(FVector::Dist2D(Lurk.Foot, PreyAt));
	Sense.Distance = static_cast<float>(FVector::Dist(Body, PreyAt));
	Sense.Rise = static_cast<float>(PreyFeet.Z - Lurk.Foot.Z);
	Lurk.LastPreySpot = PreyAt;
	// A clear line only when it matters (near, or set on them): one trace a look.
	const float Near = FMath::Max3(Tuning.PassRadius, Tuning.LurkCueRadius, Sense.SpringRadius) * 1.2f;
	if (World && (Sense.FlatDistance <= Near || Sense.bHurt || Sense.bCalled || Sense.bHunting))
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SpiderLurkSight), false, this);
		Params.AddIgnoredActor(Pawn);
		FHitResult Hit;
		Sense.bSees = !World->LineTraceSingleByChannel(Hit, Body + Lurk.Normal * (25.f * Scale), PreyAt + FVector(0.0, 0.0, 40.0), ECC_Visibility, Params);
	}
	// Walking on past it, away.
	const FVector Velocity(Pawn->GetVelocity().X, Pawn->GetVelocity().Y, 0.0);
	const float Speed = static_cast<float>(Velocity.Size());
	const FVector Away = FVector(PreyAt.X - Lurk.Foot.X, PreyAt.Y - Lurk.Foot.Y, 0.0).GetSafeNormal();
	Sense.bMovingAway = Speed > 120.f && FVector::DotProduct(Velocity, Away) > 0.35 * Speed;
	// Standing about near it, and staring at it.
	Lurk.Still = Speed < 60.f && Sense.FlatDistance <= Tuning.PassRadius ? Lurk.Still + Interval : 0.f;
	if (Player && Player->GetPawn() == Pawn)
	{
		FVector EyeAt;
		FRotator EyeTurn;
		Player->GetPlayerViewPoint(EyeAt, EyeTurn);
		const bool bLooking = Sense.bSees && FVector::DotProduct(EyeTurn.Vector(), (Body - EyeAt).GetSafeNormal()) >= Tuning.LookCone;
		Lurk.LookedAt = bLooking ? Lurk.LookedAt + Interval : FMath::Max(Lurk.LookedAt - 2.f * Interval, 0.f);
	}
	Sense.LookedAtSeconds = Lurk.LookedAt;
	Sense.StillSeconds = Lurk.Still;
	// Its faint chitter as they come near: the player who listens hears it before it springs.
	if (Sense.bSees && Sense.FlatDistance <= Tuning.LurkCueRadius && Lurk.SinceCue >= Tuning.LurkCueGap)
	{
		Voice->Play(FName(SpiderAmbushCue::Lurk), 0.8f);
		Lurk.SinceCue = 0.f;
	}
	OutPrey = Pawn;
	return Sense;
}

void ASpiderCreature::Spring(APawn* Prey, ESpiderSpring How)
{
	Lurk.Prey = Prey;
	Lurk.Clock = 0.f;
	Lurk.bHurt = false;
	// It has been watching them: its hunt begins with no notice beat, and its pack hears it go.
	if (Prey)
	{
		AlertTo(Prey);
	}
	if (AAmbushSpawner* Spawner = Lurk.Owner.Get())
	{
		Spawner->NoteLurkerSprang(*this, Prey);
	}
	if (How == ESpiderSpring::Climb && Lurk.Surface != ESpiderSurface::Ledge)
	{
		Lurk.Phase = ESpiderLurkPhase::Climbing;
		Lurk.ClimbLookIn = 0.f;
		Lurk.ClimbCueIn = 0.f;
		Voice->Play(FName(CreatureReactCue::SpiderNotice));
		UE_LOG(LogLooter, Log, TEXT("%s springs: down its wall at %s."), *GetName(), *GetNameSafe(Prey));
		return;
	}
	Lurk.Phase = ESpiderLurkPhase::Coiling;
	AbdomenKick = 8.f;
	Voice->Play(FName(SpiderAmbushCue::Leap));
	UE_LOG(LogLooter, Log, TEXT("%s springs: a leap at %s."), *GetName(), *GetNameSafe(Prey));
}
