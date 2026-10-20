#include "Embers/Powers/EmberTripwire.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Embers/EmberDefinition.h"
#include "Embers/Powers/EmberPropActor.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName StunName(TEXT("Stun"));

	/** A surveyor's stake: the fence's post scaled down, sunk a little into the ground. */
	const FVector StakeScale(0.45f, 0.45f, 0.42f);
	constexpr float StakeSink = 8.f;
	/**
	 * The wire: thin, glinting, drawn in pieces so it can shiver. Thicker and brighter than a real wire: at 0.9 cm it was
	 * lost in daylight at 6 m (2026-10-20 play check), and the player has to see where they laid it.
	 */
	constexpr int32 WirePieces = 16;
	constexpr float WireWidth = 1.6f;
	constexpr float WireGlow = 7.f;
	/** A trip's shiver: how far the wire swings (cm), how fast, how soon it's still. */
	constexpr float TwangAmplitude = 9.f;
	constexpr float TwangRate = 55.f;
	constexpr float TwangSeconds = 0.6f;
	/** A body's feet this far over or under the wire are on another level (cm). */
	constexpr float CrossHeight = 120.f;
	/** The wire's walkable rise between steps (cm, on top of the step's own length for a slope). */
	constexpr float StepRise = 40.f;
}

FTripwirePlan UEmberTripwire::PlanWire(const ACharacter& Character, float Range)
{
	FTripwirePlan Wire;
	const UWorld* World = Character.GetWorld();
	if (!World || Range <= 0.f)
	{
		return Wire;
	}
	FVector Eye;
	FVector Aim;
	GetAim(Character, Eye, Aim);
	FVector Flat = FVector(Aim.X, Aim.Y, 0.0).GetSafeNormal();
	if (Flat.IsNearlyZero())
	{
		Flat = Character.GetActorForwardVector().GetSafeNormal2D();
	}
	// Where the aim meets the ground near enough, else a few metres ahead on the ground.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("TripwirePlan"), &Character);
	FVector Center;
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Eye, Eye + Aim * Reach, FCollisionObjectQueryParams(ECC_WorldStatic), Params) && Hit.ImpactNormal.Z >= 0.6)
	{
		Center = Hit.ImpactPoint;
	}
	else if (!FindGround(*World, FeetOf(Character) + Flat * FallbackAhead, 200.f, 400.f, Center, nullptr, &Character))
	{
		return Wire;
	}

	// Each half out across the aim, step by step over the ground, never off a drop or through anything solid.
	const FVector Across = FVector::CrossProduct(FVector::UpVector, Flat).GetSafeNormal();
	const FCollisionShape Thread = FCollisionShape::MakeSphere(5.f);
	const FVector Lift(0.0, 0.0, WireHeight);
	FVector Ends[2];
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float Sign = Side == 0 ? 1.f : -1.f;
		FVector Last = Center;
		float Covered = 0.f;
		while (Covered + StepLength <= Range * 0.5f + 1.f)
		{
			const FVector Next = Center + Across * (Sign * (Covered + StepLength));
			const float Rise = StepRise + StepLength;
			FHitResult Ground;
			if (!World->LineTraceSingleByObjectType(Ground, FVector(Next.X, Next.Y, Last.Z + Rise), FVector(Next.X, Next.Y, Last.Z - Rise),
					FCollisionObjectQueryParams(ECC_WorldStatic), Params)
				|| Ground.bStartPenetrating || Ground.ImpactNormal.Z < 0.6)
			{
				break;
			}
			FHitResult Block;
			if (World->SweepSingleByObjectType(Block, Last + Lift, Ground.ImpactPoint + Lift, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
					Thread, Params))
			{
				break;
			}
			Last = Ground.ImpactPoint;
			Covered += StepLength;
		}
		Ends[Side] = Last;
	}
	Wire.A = Ends[0];
	Wire.B = Ends[1];
	Wire.Length = static_cast<float>(FVector::Dist(Wire.A, Wire.B));
	return Wire;
}

EEmberDenied UEmberTripwire::CanActivate(const FEmberContext& Context) const
{
	if (!Context.Character)
	{
		return EEmberDenied::Refused;
	}
	if (bStrung)
	{
		return EEmberDenied::Active;
	}
	const float Range = Context.Definition && Context.Definition->Range > 0.f ? Context.Definition->Range : DefaultRange;
	return PlanWire(*Context.Character, Range).IsValid(MinLength) ? EEmberDenied::None : EEmberDenied::Refused;
}

void UEmberTripwire::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!World || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	const float Range = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	Damage = ScaledDamage(Context, DefaultDamage);
	Stun = FMath::Max(Def ? Def->Extra(StunName, DefaultStun) : DefaultStun, 0.f);
	Plan = PlanWire(*Character, Range);
	Time = 0.f;
	Twang = -1.f;
	Slack = -1.f;
	LastFeet.Reset();
	Tripped.Reset();
	bStrung = Plan.IsValid(MinLength);
	if (!bStrung)
	{
		return;
	}
	// Two stakes knocked in, the wire drawn taut between them.
	const float Yaw = (Plan.B - Plan.A).GetSafeNormal2D().Rotation().Yaw;
	StakeA = AEmberPropActor::Place(World, StakePath, Plan.A - FVector(0.0, 0.0, StakeSink), Yaw, StakeScale);
	StakeB = AEmberPropActor::Place(World, StakePath, Plan.B - FVector(0.0, 0.0, StakeSink), Yaw + 180.f, StakeScale);
	LooterSound::Play2D(Character, LooterSoundCue::Ember::TripwireSet);
	if (FWeaponFX* FX = Effects())
	{
		const FLinearColor Dust = EmberFX::DustColor(Character);
		for (const FVector& Foot : { Plan.A, Plan.B })
		{
			for (int32 Index = 0; Index < 5; ++Index)
			{
				const FVector Out = (Random.GetUnitVector() * FVector(1.0, 1.0, 0.0) + FVector(0.0, 0.0, 0.6)).GetSafeNormal();
				FX->SpawnDustPuff(Foot + FVector(0.0, 0.0, 6.0), Out * Random.FRandRange(80.f, 160.f), Dust, 0.35f, 10.f, Random.FRandRange(40.f, 60.f),
					Random.FRandRange(0.5f, 0.8f));
			}
		}
	}
	// It's strung: a glint run along it.
	Twang = 0.f;
	TwangAt = 0.5f;
	FViewKick Knock;
	Knock.Pitch = -0.3f;
	Knock.Frequency = 9.f;
	Knock.Damping = 0.6f;
	Kick(Knock);
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Tripwire: %.0f cm strung, %.1f and %.1f s down for whatever crosses it"), Plan.Length, Damage, Stun);
}

void UEmberTripwire::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	StepWire(DeltaSeconds);
}

void UEmberTripwire::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bStrung)
	{
		return;
	}
	bStrung = false;
	PullStakes();
	UE_LOG(LogLooter, Log, TEXT("Tripwire: pulled, %d tripped"), Tripped.Num());
}

FVector UEmberTripwire::WireAt(float U) const
{
	return FMath::Lerp(Plan.A, Plan.B, U) + FVector(0.0, 0.0, WireHeight);
}

// ---------------------------------------------------------------------------
// The wire, frame by frame
// ---------------------------------------------------------------------------

void UEmberTripwire::StepWire(float DeltaSeconds)
{
	UWorld* World = GetBodyWorld();
	if (!bStrung || !World)
	{
		return;
	}
	Time += FMath::Max(DeltaSeconds, 0.f);
	const FVector Middle = WireAt(0.5f);
	const float Near = Plan.Length * 0.5f + 600.f;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead() || FVector::Dist2D(Creature->GetActorLocation(), Middle) > Near)
		{
			continue;
		}
		FVector BodyMiddle;
		float BodyRadius = 0.f;
		float HalfHeight = 0.f;
		BodyOf(*Creature, BodyMiddle, BodyRadius, HalfHeight);
		const FVector Feet = BodyMiddle - FVector(0.0, 0.0, HalfHeight);
		FVector* Before = LastFeet.Find(Creature);
		if (!Before)
		{
			LastFeet.Add(Creature, Feet);
			continue;
		}
		const FVector Was = *Before;
		*Before = Feet;
		if (Tripped.Contains(Creature))
		{
			continue;
		}
		// Across the wire since last frame (its feet's path crossing it, seen from above), or standing on it; at its height.
		FVector Crossing;
		const bool bCrossed = FMath::SegmentIntersection2D(Was, Feet, Plan.A, Plan.B, Crossing);
		const FVector Flat2D(Feet.X, Feet.Y, 0.0);
		const FVector OnWire = FMath::ClosestPointOnSegment(Flat2D, FVector(Plan.A.X, Plan.A.Y, 0.0), FVector(Plan.B.X, Plan.B.Y, 0.0));
		const bool bOnIt = FVector::Dist(Flat2D, OnWire) <= BodyRadius * 0.6f;
		if (!bCrossed && !bOnIt)
		{
			continue;
		}
		const float U = Plan.Length > 0.f ? FMath::Clamp(static_cast<float>(FVector::Dist2D(Plan.A, bCrossed ? Crossing : OnWire)) / Plan.Length, 0.f, 1.f) : 0.5f;
		if (FMath::Abs(Feet.Z - WireAt(U).Z) > CrossHeight)
		{
			continue;
		}
		FVector Going = (Feet - Was).GetSafeNormal2D();
		if (Going.IsNearlyZero())
		{
			const UCharacterMovementComponent* Movement = Creature->GetCharacterMovement();
			Going = Movement ? Movement->Velocity.GetSafeNormal2D() : FVector::ZeroVector;
		}
		if (Going.IsNearlyZero())
		{
			Going = (Flat2D - OnWire).GetSafeNormal2D();
		}
		TwangAt = U;
		Trip(*Creature, Going);
	}
}

void UEmberTripwire::Trip(ACreatureBase& Creature, const FVector& Going)
{
	Tripped.Add(&Creature);
	const FVector At = WireAt(TwangAt);
	Hurt(Creature, Damage * Random.FRandRange(0.9f, 1.1f), Creature.GetActorLocation(), Going);
	if (!Creature.IsDead() && !IsBoss(Creature))
	{
		if (UCreatureKnockComponent* Knock = Creature.GetKnock())
		{
			// Its feet taken out: pitched forward a short way and left lying, reeling.
			Knock->Knock(Going.IsNearlyZero() ? Creature.GetActorForwardVector() : Going, TripThrow, TripHeight, Stun);
		}
	}
	Twang = 0.f;
	LooterSound::PlayAt(&Creature, LooterSoundCue::Ember::TripwireTrip, At);
	if (FWeaponFX* FX = Effects())
	{
		const FLinearColor Dust = EmberFX::DustColor(&Creature);
		for (int32 Index = 0; Index < 7; ++Index)
		{
			const FVector Out = (Going + Random.GetUnitVector() * 0.8 + FVector(0.0, 0.0, 0.5)).GetSafeNormal();
			FX->SpawnDustPuff(At + Out * 10.f, Out * Random.FRandRange(120.f, 240.f), Dust, 0.4f, 14.f, Random.FRandRange(60.f, 90.f), Random.FRandRange(0.6f, 1.f));
		}
	}
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Sparks.Spawn(At, Random.GetUnitVector() * Random.FRandRange(120.f, 260.f), Random.FRandRange(0.15f, 0.3f), 1.f, Color, 16.f, 500.f, 3.f);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Tripwire: %s tripped"), *Creature.GetName());
}

void UEmberTripwire::PullStakes()
{
	const UWorld* World = GetBodyWorld();
	if (World && !World->bIsTearingDown)
	{
		LooterSound::PlayAt(GetBodyWorld(), LooterSoundCue::Ember::TripwireSlack, WireAt(0.5f));
		Slack = 0.f;
		StartLinger();
	}
	for (const TWeakObjectPtr<AEmberPropActor>& Stake : { StakeA, StakeB })
	{
		if (AEmberPropActor* Prop = Stake.Get())
		{
			Prop->Remove();
		}
	}
	StakeA.Reset();
	StakeB.Reset();
}

bool UEmberTripwire::Linger(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	FWeaponFX* FX = Effects();
	if (FX && (bStrung || Slack >= 0.f))
	{
		// The wire: taut and still, shivering after a trip, or slack and falling once pulled.
		if (Twang >= 0.f)
		{
			Twang += Dt;
			if (Twang >= TwangSeconds)
			{
				Twang = -1.f;
			}
		}
		if (Slack >= 0.f)
		{
			Slack += Dt;
		}
		const float SlackFade = Slack >= 0.f ? FMath::Clamp(1.f - Slack / 0.4f, 0.f, 1.f) : 1.f;
		const float Shiver = Twang >= 0.f ? TwangAmplitude * FMath::Exp(-Twang / (TwangSeconds * 0.3f)) * FMath::Sin(Twang * TwangRate) : 0.f;
		// A glint running along it now and then.
		const float Glint = FMath::Frac(Time / 1.6f);
		FVector Previous = WireAt(0.f);
		for (int32 Index = 1; Index <= WirePieces; ++Index)
		{
			const float U = Index / static_cast<float>(WirePieces);
			const float Sag = Slack >= 0.f ? 4.f * U * (1.f - U) * FMath::Min(Slack, 0.4f) * 60.f : 0.f;
			const float Swing = Shiver * FMath::Sin(UE_PI * U) * (1.f - 0.6f * FMath::Abs(U - TwangAt));
			const FVector Point = WireAt(U) + FVector(0.0, 0.0, Swing - Sag);
			const float Lit = FMath::Max(1.f - FMath::Abs(U - Glint) * 8.f, 0.f);
			FX->AddStreak(Previous, Point, WireWidth * (1.f + Lit), Color, (WireGlow + 10.f * Lit) * SlackFade);
			Previous = Point;
		}
		if (Slack >= 0.4f)
		{
			Slack = -1.f;
		}
	}
	Sparks.Tick(Dt, FX);
	return bStrung || Slack >= 0.f || !Sparks.IsEmpty();
}
