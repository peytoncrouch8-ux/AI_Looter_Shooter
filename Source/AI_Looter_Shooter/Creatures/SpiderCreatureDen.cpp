// ASpiderCreature out of a den in a wall (EAmbushEntrance::Den; the user's ask, 2026-10-31: the Sink floor's spiders crawl
// out of dens in the pit walls as the player reaches the floor, instead of standing there already). Its beats:
// - Hidden (DenHidden): unseen in the den's throat until its turn (SpiderAmbushRules::DenDelay): hidden, no collision, not
//   hurt, its brain held, passive (no target and no cry while nobody can see it).
// - Crawl (DenCrawl, Looter.Spider.Den.CrawlSeconds): seen, its hit zones on, deep in the throat (Looter.Spider.Den.Inside),
//   level, facing out. Its front legs come first: out of the mouth to touch the upper rim, re-set a hand further out, then
//   grip the lip's side; the second pair after them on the lip's lower side; then the body pulls itself out in heaves,
//   wriggling, its back legs braced on the throat's sides. The den's scrabble at the mouth (LooterSoundCue::SpiderDen),
//   grit and dust spilling off the lip.
// - Tip (DenTurn): over the lip and down onto the face under the mouth, head down, each pair stepping onto the face in turn,
//   front first, onto the cling's own spots, so the lurk's climb (Climbing: SpiderCreatureLeap.cpp, its pose
//   SpiderCreatureCling.cpp) takes over in step: down the face at Looter.Spider.Den.ClimbSpeed, a hop off the last meter,
//   the landing's thump and dust, then it hunts the player (FinishDenEmerge, from EndLurk).
//
// Why it's built this way:
// - The burrow has no collision and the contract fixes its mouth (170 x 95 cm, its lip 18 cm proud of the wall plane), so the
//   throat and the lip are worked out in the den's frame, not traced. Only the wall, the face under the mouth and the floor
//   are traced, once as it's sent in, and its feet's face spots once as it tips: six at once cost no per-frame queries.
// - Hits land all through it (its hit zones are on from the crawl; passive, it's never shoved off by a knock-back): the
//   jolt's springs shake its body as on a perch; a hard one (SpiderAmbushRules::KnockedOffDen) once it's out past the mouth
//   knocks it off, down as the lurk's hop with nobody to land on; killed, its body is brought out of the wall and comes down
//   as any wall death (FallFromPerch).
// - Cancelled cleanly: all of it is its own state on its own tick. A spawner taking it away destroys it; a story moving on
//   leaves it passive (its spawner's UpdateLeaving takes it once unseen); a level going takes everything.

#include "Creatures/SpiderCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitReaction.h"
#include "Creatures/AmbushSpawner.h"
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
	/** The den's wall is looked for back along the mouth's facing from this far out of it to this far in (cm). */
	constexpr float WallLookOut = 150.f;
	constexpr float WallLookIn = 300.f;
	/** The face under the mouth, the same way (cm). */
	constexpr float FaceLookOut = 150.f;
	constexpr float FaceLookIn = 250.f;
	/** The floor is looked for this far out from the wall under the mouth (cm, past its capsule), down to this far. */
	constexpr float FloorOut = 80.f;
	constexpr float FloorBelow = 1500.f;
	/** Its capsule clear of the wall (cm). */
	constexpr float WallClear = 4.f;
	/** Its second spill of grit comes this far along the crawl (as its body drags over the lip). */
	constexpr float GritAt = 0.55f;
	/** Its scrabble's pitch for a bigger spider: lower by the square root of its size. */
	constexpr float LeastPitch = 0.6f;
}

bool ASpiderCreature::IsInDen() const
{
	return Lurk.Phase == ESpiderLurkPhase::DenHidden || Lurk.Phase == ESpiderLurkPhase::DenCrawl || Lurk.Phase == ESpiderLurkPhase::DenTurn;
}

// ---------------------------------------------------------------------------
// Sent into its den, and its beats
// ---------------------------------------------------------------------------

bool ASpiderCreature::BeginDenEmerge(const FSpiderPerchSpot& DenSpot, float Delay, AAmbushSpawner* DenOwner)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	UWorld* World = GetWorld();
	if (IsDead() || !Movement || !Capsule || !World)
	{
		return false;
	}
	const FSpiderDenTuning Tuning = FSpiderDenTuning::Live();
	const float Scale = GetSizeScale();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	FSpiderDenState Found;
	Found.Mouth = DenSpot.Spot;
	Found.Out = SpiderAmbushRules::FaceWay(DenSpot.FaceYaw);
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SpiderDen"), this, /*bTraceComplex*/ true);
	const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
	auto IsWall = [](const FHitResult& Hit)
	{
		const ESpiderSurface Surface = SpiderAmbushRules::SurfaceOf(Hit.ImpactNormal);
		return !Hit.bStartPenetrating && (Surface == ESpiderSurface::Wall || Surface == ESpiderSurface::Overhang);
	};
	// The wall the den is in, looked back at from in front of the mouth.
	FHitResult Hit;
	if (!World->LineTraceSingleByObjectType(Hit, Found.Mouth + Found.Out * WallLookOut, Found.Mouth - Found.Out * WallLookIn, Objects, Params)
		|| !IsWall(Hit))
	{
		return false;
	}
	// The floor at the wall's foot, under the mouth.
	if (!FindGround(Found.Mouth + Found.Out * (Radius + FloorOut), 50.f, FloorBelow, Found.Floor)
		|| Found.Floor.Z > Found.Mouth.Z - FSpiderDenTuning::MouthHalfHeight)
	{
		return false;
	}
	// The face under the mouth it tips onto, low enough that it climbs a little before it drops (none: it drops off the lip).
	const float Below = SpiderAmbushRules::DenTurnBelow(static_cast<float>(Found.Mouth.Z - Found.Floor.Z), Scale, Tuning);
	const FVector Under = Found.Mouth - FVector(0.0, 0.0, Below);
	Found.Normal = Found.Out;
	Found.ClingPoint = Under;
	if (World->LineTraceSingleByObjectType(Hit, Under + Found.Out * FaceLookOut, Under - Found.Out * FaceLookIn, Objects, Params) && IsWall(Hit))
	{
		Found.bFace = true;
		Found.ClingPoint = Hit.ImpactPoint;
		Found.Normal = Hit.ImpactNormal;
	}
	for (FVector& Spot : Found.FaceSpots)
	{
		Spot = Found.ClingPoint;
	}
	Found.Delay = FMath::Max(Delay, 0.f);
	Den = Found;

	// Its lurk's state carries the climb down after the tip: the face under the mouth, head down, its slower climb, nobody to
	// leap at (it drops and lands, then hunts: FinishDenEmerge).
	Lurk = FSpiderLurkState();
	Lurk.Owner = DenOwner;
	Lurk.bFromDen = true;
	Lurk.Perch = DenSpot;
	Lurk.Perch.Kind = ESpiderPerchKind::Wall;
	Lurk.Surface = ESpiderSurface::Wall;
	Lurk.Normal = Den.Normal;
	Lurk.Point = Den.ClingPoint;
	Lurk.Forward = SpiderAmbushRules::ClingForward(Den.Normal, ESpiderPerchKind::Wall, DenSpot.FaceYaw);
	Lurk.Watch = Lurk.Forward;
	Lurk.Foot = Den.Floor;
	Lurk.ClimbSpeed = Tuning.ClimbSpeed;
	Lurk.HopHeight = Tuning.HopHeight;
	Lurk.Phase = ESpiderLurkPhase::DenHidden;

	// Held at the mouth (its capsule just out of the wall), unseen and out of everything until its turn.
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	SetActorLocationAndRotation(Den.Mouth + Den.Out * (Radius + WallClear), FRotator(0.f, static_cast<float>(Den.Out.Rotation().Yaw), 0.f),
		false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(true);
	Den.CapsuleCollision = static_cast<uint8>(Capsule->GetCollisionEnabled());
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetHitVolumesEnabled(false);
	if (UHealthComponent* Life = FindComponentByClass<UHealthComponent>(); Life && !Life->bInvulnerable)
	{
		Life->bInvulnerable = true;
		Den.bMadeInvulnerable = true;
	}
	if (!IsPassive())
	{
		SetPassive(true);
		Den.bMadePassive = true;
	}
	HoldBrain(0.25f);
	UE_LOG(LogLooter, Log, TEXT("%s waits in its den at (%.0f, %.0f, %.0f), %.1f m over the floor, %.2f s; %s."), *GetName(),
		Den.Mouth.X, Den.Mouth.Y, Den.Mouth.Z, (Den.Mouth.Z - Den.Floor.Z) / 100.0, Den.Delay,
		Den.bFace ? TEXT("then down the face") : TEXT("no face under it: it drops off the lip"));
	return true;
}

void ASpiderCreature::TickDen(float DeltaSeconds)
{
	const FSpiderDenTuning Tuning = FSpiderDenTuning::Live();
	const float Scale = GetSizeScale();
	switch (Lurk.Phase)
	{
	case ESpiderLurkPhase::DenHidden:
		if (Lurk.Clock >= Den.Delay)
		{
			StartDenCrawl();
		}
		break;
	case ESpiderLurkPhase::DenCrawl:
	{
		const float Seconds = SpiderAmbushRules::DenCrawlSeconds(Scale, Tuning);
		if (Den.GritBeats == 1 && Lurk.Clock >= Seconds * GritAt)
		{
			// Its body dragging over the lip.
			SpillDenGrit(0.5f);
			++Den.GritBeats;
		}
		if (Lurk.Clock >= Seconds)
		{
			StartDenTurn();
		}
		break;
	}
	case ESpiderLurkPhase::DenTurn:
	{
		const float Seconds = SpiderAmbushRules::DenTurnSeconds(Scale, Tuning);
		if (Lurk.Clock >= Seconds)
		{
			EndDenTurn();
			break;
		}
		// Its capsule follows its body from the mouth down to the face.
		const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
		const FVector From = Den.Mouth + Den.Out * (Radius + WallClear);
		const FVector To = Den.ClingPoint + Den.Normal * (Radius + WallClear);
		const float Along = FMath::SmoothStep(0.f, 1.f, Lurk.Clock / Seconds);
		SetActorLocation(FMath::Lerp(From, To, static_cast<double>(Along)), false, nullptr, ETeleportType::TeleportPhysics);
		break;
	}
	default:
		break;
	}
}

void ASpiderCreature::StartDenCrawl()
{
	Lurk.Phase = ESpiderLurkPhase::DenCrawl;
	Lurk.Clock = 0.f;
	// Seen and in reach from here (its hold stays until it's down: nothing shoves it off the wall, nor sets it on anyone).
	ComeOutOfDen(/*bHunt*/ false);
	// Claws scrabbling inside the hollow, grit trickling out: the player hears it at the mouth as it starts out.
	LooterSound::PlayAt(this, FName(LooterSoundCue::SpiderDen), Den.Mouth, 1.f, FMath::Max(1.f / FMath::Sqrt(FMath::Max(GetSizeScale(), 0.1f)), LeastPitch));
	Den.GritBeats = 0;
	SpillDenGrit(1.f);
	Den.GritBeats = 1;
	AbdomenKick = 0.f;
	HurtKick = HurtOffset = HurtVelocity = FVector::ZeroVector;
	HurtTiltKick = HurtTilt = HurtTiltVelocity = FVector::ZeroVector;
	HurtHold = 0.f;
	SteppingGroup = INDEX_NONE;
	// Posed in the throat at once: never a frame of its resting pose.
	AnimateDen(0.f);
}

void ASpiderCreature::StartDenTurn()
{
	if (!Den.bFace)
	{
		// No face under the mouth: it drops straight off the lip.
		EndDenTurn();
		return;
	}
	Lurk.Phase = ESpiderLurkPhase::DenTurn;
	Lurk.Clock = 0.f;
	// Where each foot will grip the face as it hangs head down under the mouth (SpiderCreatureDenPose.cpp).
	PlanDenFaceSpots();
	// Its abdomen dragging over the lip, its claws on the rock.
	SpillDenGrit(0.6f);
	Voice->Play(FName(SpiderAmbushCue::Climb), 0.7f);
}

void ASpiderCreature::EndDenTurn()
{
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	Lurk.Prey = nullptr;
	Lurk.Clock = 0.f;
	if (!Den.bFace)
	{
		// Off the lip's bottom, out from the wall: the lurk's hop down, landing on the floor before the den.
		Lurk.Point = Den.Mouth + Den.Out * FSpiderDenTuning::LipProud - FVector(0.0, 0.0, FSpiderDenTuning::MouthHalfHeight);
		Lurk.Normal = Den.Out;
		Launch();
		return;
	}
	// Head down on the face under the mouth: the lurk's scramble down takes it from here (slower, a lower hop: ClimbTuning).
	Lurk.Phase = ESpiderLurkPhase::Climbing;
	Lurk.Point = Den.ClingPoint;
	Lurk.Normal = Den.Normal;
	Lurk.Watch = Lurk.Forward;
	Lurk.Raise = 0.f;
	Lurk.ClimbLookIn = 0.f;
	Lurk.ClimbCueIn = 0.3f;
	FVector Flat(Den.Normal.X, Den.Normal.Y, 0.0);
	if (!Flat.Normalize())
	{
		Flat = Den.Out;
	}
	SetActorLocationAndRotation(Lurk.Point + Lurk.Normal * (Radius + WallClear), FRotator(0.f, static_cast<float>(Flat.Rotation().Yaw), 0.f), false,
		nullptr, ETeleportType::TeleportPhysics);
}

void ASpiderCreature::ComeOutOfDen(bool bHunt)
{
	SetActorHiddenInGame(false);
	if (Den.CapsuleCollision != static_cast<uint8>(ECollisionEnabled::NoCollision) && !IsDead())
	{
		GetCapsuleComponent()->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(Den.CapsuleCollision));
	}
	Den.CapsuleCollision = static_cast<uint8>(ECollisionEnabled::NoCollision);
	if (!IsDead())
	{
		SetHitVolumesEnabled(true);
	}
	if (Den.bMadeInvulnerable)
	{
		if (UHealthComponent* Life = FindComponentByClass<UHealthComponent>())
		{
			Life->bInvulnerable = false;
		}
		Den.bMadeInvulnerable = false;
	}
	if (bHunt && Den.bMadePassive)
	{
		SetPassive(false);
		Den.bMadePassive = false;
	}
}

void ASpiderCreature::FinishDenEmerge()
{
	Lurk.bFromDen = false;
	Lurk.ClimbSpeed = 0.f;
	Lurk.HopHeight = 0.f;
	// Its spawner's still (a story moving on sends its creatures away, passive: it stays so, and goes once unseen).
	const AAmbushSpawner* Spawner = Lurk.Owner.Get();
	const bool bSentAway = Spawner && !Spawner->GetAliveCreatures().Contains(this);
	ComeOutOfDen(/*bHunt*/ !bSentAway);
	if (bSentAway || IsDead())
	{
		return;
	}
	// Down: it comes for the player (its alert cry as it turns on them).
	const UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(this);
	APawn* Player = Encounters ? static_cast<APawn*>(Encounters->GetPlayer()) : nullptr;
	if (!Player)
	{
		const APlayerController* First = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		Player = First ? First->GetPawn() : nullptr;
	}
	if (Player)
	{
		AlertTo(Player);
	}
	UE_LOG(LogLooter, Log, TEXT("%s is out of its den and down: it hunts %s."), *GetName(), *GetNameSafe(GetTarget()));
}

// ---------------------------------------------------------------------------
// Hit, knocked off, killed
// ---------------------------------------------------------------------------

void ASpiderCreature::NoteDenHit(const FCreatureHit& Hit)
{
	if (!Lurk.bFromDen || IsDead() || Hit.bKilled
		|| !SpiderAmbushRules::KnockedOffDen(Hit.Weight >= EHitWeight::Big, Hit.BurstShare, FSpiderDenTuning::Live()))
	{
		return;
	}
	if (Lurk.Phase == ESpiderLurkPhase::DenCrawl || Lurk.Phase == ESpiderLurkPhase::DenTurn)
	{
		const FVector Body = bRigReady ? BodyFrame.GetLocation() : DenBodyNow().GetLocation();
		const float OutOf = static_cast<float>(FVector::DotProduct(Body - Den.Mouth, Den.Out));
		if (OutOf < -0.2f * FSpiderDenTuning().Inside * GetSizeScale())
		{
			// Still wedged in its throat: it flinches and comes on.
			return;
		}
		// Torn off the lip from where it is: down out from the wall.
		Lurk.Point = Body - Den.Out * OutOf;
		Lurk.Normal = Den.Out;
	}
	else if (Lurk.Phase != ESpiderLurkPhase::Climbing)
	{
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("%s is knocked off the wall coming out of its den."), *GetName());
	Lurk.Prey = nullptr;
	Launch();
}

void ASpiderCreature::FallFromDen()
{
	// Seen again (its collision stays off: it's dead), and not held unhurt.
	SetActorHiddenInGame(false);
	Den.CapsuleCollision = static_cast<uint8>(ECollisionEnabled::NoCollision);
	if (Den.bMadeInvulnerable)
	{
		if (UHealthComponent* Life = FindComponentByClass<UHealthComponent>())
		{
			Life->bInvulnerable = false;
		}
		Den.bMadeInvulnerable = false;
	}
	const float Scale = GetSizeScale();
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (Lurk.Phase == ESpiderLurkPhase::DenHidden || !bRigReady)
	{
		// Never seen: its body set down at the wall's foot under the mouth.
		const FVector Feet = Den.Floor + Den.Out * (Radius + FloorOut * 0.5f);
		BodyFrame = FTransform(FRotator(0.f, static_cast<float>(Den.Out.Rotation().Yaw), 0.f), Feet + FVector(0.0, 0.0, RideHeight * Scale), FVector(Scale));
	}
	else
	{
		// Its body brought out of the wall (the throat is inside the rock), and down from there.
		const FVector At = BodyFrame.GetLocation();
		const float OutOf = static_cast<float>(FVector::DotProduct(At - Den.Mouth, Den.Out));
		const float Least = Radius * 0.6f;
		if (OutOf < Least)
		{
			BodyFrame.SetLocation(At + Den.Out * (Least - OutOf));
		}
	}
	Lurk.Normal = Den.Out;
	Lurk.Foot = Den.Floor;
	Lurk.bFromDen = false;
	FallFromPerch();
}
