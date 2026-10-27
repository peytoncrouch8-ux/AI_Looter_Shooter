// AGravemotherCreature's Grave Clod (the user's call, 2026-10-15/19): her attack with its own timing. The wind-up is the tell
// (the user's 1.2 s): her front legs dig into the floor, tear a clod of grave earth up at TearAt and lift it overhead by
// LiftedAt (her pose: GravemotherCreaturePose.cpp); the strike lobs it at where her target stands (about 1.5 s to 20 m, its
// shadow on the ground where it will come down: AGravemotherClod). Shot in her grip, it bursts on her and stuns her (her
// boss's stagger for the user's 2 s, "Stunned" on her bar, stars over her head); her grip lost to a stagger or her death,
// it falls and bursts harmlessly.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Creatures/GravemotherClod.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/SpitterGlob.h"
#include "Creatures/SpitterRules.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** Her lob's way is looked at this often (s) while her clod is ready, and up to this share of its flight. */
	constexpr float LobCheckEvery = 0.25f;
	constexpr float LobCheckShare = 0.85f;
	constexpr int32 LobCheckPieces = 6;
	/** The clod is full-grown in her grip by this share of the way from TearAt to LiftedAt. */
	constexpr float GrownShare = 0.6f;
	/** Her front feet tear at the floor this often (s) before the clod comes up, and earth sifts off it this often. */
	constexpr float DigEvery = 0.15f;
	constexpr float SiftEvery = 0.08f;
	/** Where she lobs from, seen before the tell (her grip overhead, at a spider's size): up and a little ahead. */
	constexpr float LobFromUp = 150.f;
	constexpr float LobFromAhead = 60.f;
	/** A lob that meets something this soon after leaving her grip is blocked (s; her shortest lob flies 0.8 s). */
	constexpr float ClodMinClearSeconds = 0.25f;

	/** Where a pawn's feet are. */
	FVector FeetOf(const APawn& Pawn)
	{
		const ACharacter* Character = Cast<ACharacter>(&Pawn);
		const float Half = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
		return Pawn.GetActorLocation() - FVector(0.0, 0.0, Half);
	}
}

// ---------------------------------------------------------------------------
// When she lobs
// ---------------------------------------------------------------------------

bool AGravemotherCreature::WantsToClod() const
{
	const APawn* Victim = GetTarget();
	// Her clods fly at her fight's player: only while her fight is on.
	if (ClodCooldownLeft > 0.f || !Victim || IsDead() || HeldClod.IsValid() || !Boss || !Boss->IsFighting() || !IsValidTarget(Victim))
	{
		return false;
	}
	const float Scale = GetSizeScale();
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	return Distance >= Clods.MinDistance * Scale && Distance <= Clods.MaxDistance * Scale && IsLobClear();
}

bool AGravemotherCreature::IsLobClear() const
{
	if (LobCheckedAt >= 0.f && Clock - LobCheckedAt < LobCheckEvery)
	{
		return bLobClear;
	}
	LobCheckedAt = Clock;
	const UWorld* World = GetWorld();
	const APawn* Victim = GetTarget();
	if (!World || !Victim)
	{
		bLobClear = false;
		return false;
	}
	// The arc she'd throw now, in a few straight pieces up to where it comes down onto her target: a lob clears low cover,
	// but not a wall or her den's overhang. Blocked, she does something else (bites, ploughs, closes in).
	const float Scale = GetSizeScale();
	const FVector Start = GetActorLocation() + FVector(0.0, 0.0, LobFromUp * Scale) + GetActorForwardVector() * (LobFromAhead * Scale);
	const FVector LandAt = FeetOf(*Victim);
	const float Seconds = GravemotherFight::ClodFlightSeconds(static_cast<float>(FVector::Dist2D(Start, LandAt)), Clods.FlightAt20m);
	const FVector Velocity = SpitterRules::LobVelocity(Start, LandAt, Seconds, Clods.Gravity);
	const FCollisionQueryParams Query = ASpitterGlob::MakeWorldQuery(World, this);
	// Room over her for the clod held high on her raised forelegs: under the den's roof the clod rose into the rock and
	// "came down" on it the frame it was thrown (DEMO, 2026-10-27). No room, no clod: she bites or ploughs in there.
	{
		const FVector Middle = GetActorLocation();
		const FVector Top = Start + FVector(0.0, 0.0, AGravemotherClod::FullSize);
		FHitResult Roof;
		if (World->LineTraceSingleByObjectType(Roof, Middle, FVector(Middle.X, Middle.Y, Top.Z), FCollisionObjectQueryParams(ECC_WorldStatic), Query)
			|| World->LineTraceSingleByObjectType(Roof, FVector(Middle.X, Middle.Y, Top.Z), Top, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
		{
			bLobClear = false;
			return false;
		}
	}
	FVector From = Start;
	for (int32 Piece = 1; Piece <= LobCheckPieces; ++Piece)
	{
		const FVector To = SpitterRules::LobPoint(Start, Velocity, Seconds * LobCheckShare * Piece / LobCheckPieces, Clods.Gravity);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
		{
			bLobClear = false;
			return false;
		}
		From = To;
	}
	bLobClear = true;
	return true;
}

FVector AGravemotherCreature::GripSpot() const
{
	// Between her front feet, the clod resting on them (a little over their middle).
	const FVector Left = GetPosedFoot(0, -1.f);
	const FVector Right = GetPosedFoot(0, 1.f);
	const float Grown = HeldClod.IsValid() ? 1.f : 0.3f;
	return (Left + Right) * 0.5f + FVector(0.0, 0.0, AGravemotherClod::FullSize * 0.35f * Grown);
}

// ---------------------------------------------------------------------------
// The tell, the throw
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginClod()
{
	Move = EGravemotherMove::Clod;
	AttackWindup = Clods.Tell;
	AttackRecovery = Clods.Recovery;
	bClodTorn = false;
	bClodCountered = false;
	DigDirtIn = 0.f;
	SiftIn = 0.f;
	DigSound = LooterSound::Start(this, LooterSoundCue::GravemotherClodDig, GetRootComponent());
	UE_LOG(LogLooter, Verbose, TEXT("%s digs in to tear up a clod."), *GetName());
}

void AGravemotherCreature::TickClod(float DeltaSeconds)
{
	AGravemotherClod* Clod = HeldClod.Get();
	const bool bTelling = Move == EGravemotherMove::Clod && GetCreatureState() == ECreatureState::Attack && !bReeling
		&& GetStateTime() < AttackWindup;
	if (!bTelling)
	{
		// Her grip lost (the attack cut short) with the clod in it: it falls.
		if (Clod)
		{
			DropClod();
		}
		return;
	}
	const float Time = GetStateTime();
	if (!bClodTorn)
	{
		// Her front legs tear at the floor.
		DigDirtIn -= DeltaSeconds;
		if (DigDirtIn <= 0.f)
		{
			DigDirtIn = DigEvery;
			for (const float Side : { -1.f, 1.f })
			{
				KickDirt(GetPosedFoot(0, Side), 0.25f);
			}
		}
		if (Time >= Clods.TearAt)
		{
			// Torn up out of the floor between her front feet.
			bClodTorn = true;
			const FVector Spot = (GetPosedFoot(0, -1.f) + GetPosedFoot(0, 1.f)) * 0.5f;
			Clod = AGravemotherClod::Spawn(*this, Spot, Clods);
			HeldClod = Clod;
			KickDirt(Spot, 0.7f);
			if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
			{
				Effects->Chunks(EGraveChunk::Clod, Spot + FVector(0.0, 0.0, 20.0), FVector::UpVector, 2, 350.f);
			}
		}
		return;
	}
	if (!Clod)
	{
		return;
	}
	// Up in her grip, growing to its full size as it comes out of the ground, earth sifting off it.
	const float Span = FMath::Max(Clods.LiftedAt - Clods.TearAt, 0.05f) * GrownShare;
	const float Grown = FMath::Lerp(0.3f, 1.f, FMath::Clamp((Time - Clods.TearAt) / Span, 0.f, 1.f));
	const FVector Left = GetPosedFoot(0, -1.f);
	const FVector Right = GetPosedFoot(0, 1.f);
	Clod->Hold((Left + Right) * 0.5f + FVector(0.0, 0.0, AGravemotherClod::FullSize * 0.35f * Grown), Grown);
	SiftIn -= DeltaSeconds;
	if (SiftIn <= 0.f)
	{
		SiftIn = SiftEvery;
		if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
		{
			Effects->Chunks(EGraveChunk::Debris, Clod->GetActorLocation() - FVector(0.0, 0.0, AGravemotherClod::FullSize * 0.3f * Grown),
				-FVector::UpVector, 1, 60.f, 0.4f);
		}
	}
}

void AGravemotherCreature::ThrowClod()
{
	ClodCooldownLeft = Pace.ClodCooldown;
	LooterSound::Stop(DigSound.Get(), 0.15f);
	DigSound = nullptr;
	AGravemotherClod* Clod = HeldClod.Get();
	if (bClodCountered || !Clod)
	{
		return;
	}
	// At where her target stands now (the shadow shows where it'll come down; step off it).
	const APawn* Victim = GetTarget();
	const FVector From = Clod->GetActorLocation();
	const FVector Aim = Victim ? FeetOf(*Victim) : GetFeet() + GetActorForwardVector() * (Clods.MaxDistance * GetSizeScale() * 0.6f);
	const float Seconds = GravemotherFight::ClodFlightSeconds(static_cast<float>(FVector::Dist2D(From, Aim)), Clods.FlightAt20m);
	const FVector Velocity = SpitterRules::LobVelocity(From, Aim, Seconds, Clods.Gravity);
	// Its way out of her grip blocked at once (a roof over her, a wall she backed into during the tell; her shortest lob is
	// 0.8 s): it would burst in her own claws. She loses her grip instead and it falls harmlessly.
	FVector BlockedAt;
	FVector BlockedNormal;
	float BlockedAfter = 0.f;
	FCollisionQueryParams Query = ASpitterGlob::MakeWorldQuery(GetWorld(), this);
	Query.AddIgnoredActor(Clod);
	if (AGravemotherClod::PredictLanding(GetWorld(), From, Velocity, Clods.Gravity, ClodMinClearSeconds, Query, BlockedAt, BlockedNormal,
		BlockedAfter, 3))
	{
		UE_LOG(LogLooter, Log, TEXT("%s: no room to lob her clod (blocked %.2f s out at %s): it falls from her grip."), *GetName(), BlockedAfter,
			*BlockedAt.ToCompactString());
		DropClod();
		return;
	}
	Clod->Throw(Velocity, AttackDamage * Clods.Strength, GetController(), this);
	ThrownClod = Clod;
	HeldClod = nullptr;
	UE_LOG(LogLooter, Log, TEXT("%s lobs a clod at %s: %.1f m, %.2f s."), *GetName(), *GetNameSafe(Victim), FVector::Dist2D(From, Aim) / 100.f, Seconds);
}

void AGravemotherCreature::DropClod()
{
	LooterSound::Stop(DigSound.Get(), 0.1f);
	DigSound = nullptr;
	if (AGravemotherClod* Clod = HeldClod.Get())
	{
		Clod->Drop();
		ThrownClod = Clod;
	}
	HeldClod = nullptr;
}

void AGravemotherCreature::CounterClod(AController* By)
{
	// Shot in her grip: it burst on her (the clod's look). Nothing's thrown, and she's stunned.
	bClodCountered = true;
	HeldClod = nullptr;
	LooterSound::Stop(DigSound.Get(), 0.1f);
	DigSound = nullptr;
	ClodCooldownLeft = Pace.ClodCooldown;
	if (APawn* Shooter = By ? By->GetPawn() : nullptr)
	{
		AlertTo(Shooter);
	}
	bStunned = true;
	StunClock = 0.f;
	if (!Boss || !Boss->BeginStagger(Clods.StunSeconds, GravemotherFight::StunCallout()))
	{
		// Her boss can't stagger her (not fighting): her own reel for as long.
		BeginReel();
		OwnReelLeft = Clods.StunSeconds;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: her clod was shot in her grip; stunned %.1f s."), *GetName(), Clods.StunSeconds);
}
