#include "Embers/Powers/EmberTollChain.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Core/LooterCharacter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureKnockComponent.h"
#include "Embers/EmberDefinition.h"
#include "Player/ViewKick.h"
#include "Weapons/WeaponFX.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FName StunName(TEXT("Stun"));

	/** The chain's look: its links, the hook at its head. */
	constexpr float LinkLength = 13.f;
	constexpr float LinkWidth = 3.f;
	constexpr float LinkGlow = 9.f;
	/** The haul is lifted this far while it's swept (the ground's small bumps don't stop it). */
	constexpr float SweepLift = 25.f;
	/** The chain snaps back to the hand over this long once it lets go. */
	constexpr float SnapSeconds = 0.18f;

	float EaseInOut(float U)
	{
		const float X = FMath::Clamp(U, 0.f, 1.f);
		return X * X * (3.f - 2.f * X);
	}

	FVector ChestOf(const ACreatureBase& Creature)
	{
		const float HalfHeight = Creature.GetCapsuleComponent() ? Creature.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 50.f;
		return Creature.GetActorLocation() + FVector(0.0, 0.0, HalfHeight * 0.25f);
	}
}

EEmberDenied UEmberTollChain::CanActivate(const FEmberContext& Context) const
{
	const ACharacter* Character = Context.Character;
	if (!Character)
	{
		return EEmberDenied::Refused;
	}
	if (bChainOut)
	{
		return EEmberDenied::Active;
	}
	const float Reach = Context.Definition && Context.Definition->Range > 0.f ? Context.Definition->Range : DefaultRange;
	// Nothing in his sights: nothing to hook (the socket shakes).
	return FindInSights(*Character, Reach, SightsDegrees) ? EEmberDenied::None : EEmberDenied::Refused;
}

void UEmberTollChain::Activate(const FEmberContext& Context)
{
	ACharacter* Character = Context.Character;
	if (!Character || !BeginUse(Context))
	{
		return;
	}
	const UEmberDefinition* Def = Context.Definition;
	Damage = ScaledDamage(Context, DefaultDamage);
	Stun = FMath::Max(Def ? Def->Extra(StunName, DefaultStun) : DefaultStun, 0.f);
	Seconds = Def && Def->DurationSeconds > 0.f ? Def->DurationSeconds : DefaultSeconds;
	ThrowTime = FMath::Min(ThrowSeconds, Seconds * 0.4f);
	const float Reach = Def && Def->Range > 0.f ? Def->Range : DefaultRange;
	Hooked = FindInSights(*Character, Reach, SightsDegrees);
	Time = 0.f;
	bBitten = false;
	bHauls = false;
	SnapBack = -1.f;
	bChainOut = Hooked.IsValid();
	LooterSound::Play2D(Character, LooterSoundCue::Ember::TollChainThrow);
	FViewKick Throw;
	Throw.Pitch = 0.3f;
	Throw.Yaw = -0.5f;
	Throw.Roll = 0.7f;
	Throw.Frequency = 8.f;
	Throw.Damping = 0.6f;
	Kick(Throw);
	StartLinger();
	UE_LOG(LogLooter, Log, TEXT("Toll Chain: thrown at %s"), *GetNameSafe(Hooked.Get()));
}

void UEmberTollChain::Tick(const FEmberContext& Context, float DeltaSeconds)
{
	StepChain(DeltaSeconds);
}

void UEmberTollChain::End(const FEmberContext& Context, bool bInterrupted)
{
	if (!bChainOut)
	{
		return;
	}
	if (!bInterrupted)
	{
		// Its time is up: the haul arrives (whatever of it is left, now).
		StepChain(FMath::Max(Seconds - Time, 0.f) + 0.001f);
	}
	if (bChainOut)
	{
		Release(false);
	}
}

FVector UEmberTollChain::HandNow() const
{
	const ACharacter* Character = Body.Get();
	return Character ? HandPoint(*Character, 20.f, 22.f, 30.f) : FVector::ZeroVector;
}

// ---------------------------------------------------------------------------
// The throw, the bite and the haul
// ---------------------------------------------------------------------------

void UEmberTollChain::StepChain(float DeltaSeconds)
{
	if (!bChainOut)
	{
		return;
	}
	ACreatureBase* Creature = Hooked.Get();
	ACharacter* Character = Body.Get();
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!Creature || !Character || !World || (Creature->IsDead() && !bBitten))
	{
		Release(false);
		return;
	}
	Time += FMath::Max(DeltaSeconds, 0.f);
	const FVector Hand = HandNow();
	if (!bBitten)
	{
		// The head flying out, the links paying out behind it.
		const float U = FMath::Clamp(Time / FMath::Max(ThrowTime, 0.01f), 0.f, 1.f);
		DrawChain(Hand, FMath::Lerp(Hand, ChestOf(*Creature), U), 0.5f);
		if (U < 1.f)
		{
			return;
		}
		Bite();
		if (!bChainOut)
		{
			return;
		}
	}
	if (bHauls && !Creature->IsDead())
	{
		// Dragged along the ground toward him, quick in the middle, easing in at his feet.
		const float U = FMath::Clamp((Time - ThrowTime) / FMath::Max(Seconds - ThrowTime, 0.01f), 0.f, 1.f);
		const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		FVector Wanted = FMath::Lerp(HaulStart, HaulEnd, EaseInOut(U));
		FVector Ground;
		if (FindGround(*World, Wanted, 180.f, 320.f, Ground, nullptr, Creature))
		{
			Wanted.Z = Ground.Z + HalfHeight + 2.0;
		}
		else
		{
			Wanted.Z = LastHaul.Z;
		}
		// Stopped short by anything solid on the way (never by another creature or the player: the capsule's own
		// responses, pawns and loose bodies ignored), swept a little over the ground.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TollChainHaul), false, Creature);
		Params.AddIgnoredActor(Character);
		const UCapsuleComponent* Capsule = Creature->GetCapsuleComponent();
		FCollisionResponseParams Response(Capsule->GetCollisionResponseToChannels());
		Response.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
		Response.CollisionResponse.SetResponse(ECC_PhysicsBody, ECR_Ignore);
		Response.CollisionResponse.SetResponse(ECC_Destructible, ECR_Ignore);
		const FVector Lift(0.0, 0.0, SweepLift);
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius() * 0.9f,
			FMath::Max(HalfHeight - SweepLift, Capsule->GetScaledCapsuleRadius()));
		FHitResult Block;
		const FVector Was = Creature->GetActorLocation();
		if (World->SweepSingleByChannel(Block, Was + Lift, Wanted + Lift, FQuat::Identity, Capsule->GetCollisionObjectType(), Shape, Params, Response)
			&& !Block.bStartPenetrating)
		{
			Wanted = FMath::Lerp(Was, Wanted, FMath::Max(Block.Time - 0.05f, 0.f));
			bHauls = false;
		}
		Creature->SetActorLocation(Wanted, false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Movement = Creature->GetCharacterMovement())
		{
			Movement->Velocity = DeltaSeconds > 0.f ? (Wanted - Was) / DeltaSeconds : FVector::ZeroVector;
		}
		LastHaul = Wanted;
		// Dust kicked up off the ground as it's dragged.
		HaulDust += static_cast<float>(FVector::Dist2D(Was, Wanted));
		FWeaponFX* FX = Effects();
		while (FX && HaulDust >= 90.f)
		{
			HaulDust -= 90.f;
			const FVector Feet = Wanted - FVector(0.0, 0.0, HalfHeight);
			FX->SpawnDustPuff(Feet + FVector(0.0, 0.0, 8.0), (Was - Wanted).GetSafeNormal2D() * 80.f + FVector(0.0, 0.0, 50.0),
				EmberFX::DustColor(Creature), 0.35f, 18.f, Random.FRandRange(70.f, 100.f), Random.FRandRange(0.6f, 0.9f));
		}
		if (!bHauls || U >= 1.f)
		{
			DrawChain(Hand, ChestOf(*Creature), 0.12f);
			Release(true);
			return;
		}
	}
	DrawChain(Hand, ChestOf(*Creature), bHauls ? 0.12f : 0.25f);
	if (Time >= Seconds)
	{
		Release(bHauls);
	}
}

void UEmberTollChain::Bite()
{
	ACreatureBase* Creature = Hooked.Get();
	ACharacter* Character = Body.Get();
	if (!Creature || !Character)
	{
		Release(false);
		return;
	}
	bBitten = true;
	const FVector Chest = ChestOf(*Creature);
	const FVector Toward = (Character->GetActorLocation() - Creature->GetActorLocation()).GetSafeNormal2D();
	Hurt(*Creature, Damage * Random.FRandRange(0.9f, 1.1f), Chest, -Toward);
	LooterSound::PlayAt(Creature, LooterSoundCue::Ember::TollChainHook, Chest);
	for (int32 Index = 0; Index < 12; ++Index)
	{
		const FVector Out = (Toward + Random.GetUnitVector()).GetSafeNormal();
		Sparks.Spawn(Chest, Out * Random.FRandRange(200.f, 500.f), Random.FRandRange(0.15f, 0.35f), 1.3f, Color, 22.f, 900.f, 2.f);
	}
	// A pull of the view toward it as the chain bites.
	FViewKick Bitten;
	Bitten.Pitch = -0.45f;
	Bitten.FieldOfView = -0.015f;
	Bitten.Frequency = 9.f;
	Bitten.Damping = 0.6f;
	Kick(Bitten);
	if (Creature->IsDead())
	{
		// The bite killed it: nothing to haul.
		bHauls = false;
		return;
	}

	// A boss's fight keeps its rules: the chain goes taut and snaps back. Anything else reels and is hauled in.
	UCreatureKnockComponent* Knock = Creature->GetKnock();
	bHauls = !IsBoss(*Creature) && Knock != nullptr;
	if (!bHauls)
	{
		return;
	}
	Knock->Daze(FMath::Max(Seconds - Time, 0.f) + Stun);
	HaulStart = Creature->GetActorLocation();
	LastHaul = HaulStart;
	HaulDust = 0.f;
	const float BodyRadius = Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleRadius() : 35.f;
	const float ItsRadius = Creature->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Feet = Character->GetActorLocation();
	const FVector Away = (HaulStart - Feet).GetSafeNormal2D();
	FVector HaulTo = Feet + (Away.IsNearlyZero() ? Character->GetActorForwardVector() : Away) * (BodyRadius + ItsRadius + HaulGap);
	HaulTo.Z = HaulStart.Z;
	// A tougher rank and a bigger body give less ground (Dust Devil's rule).
	const float Share = UCreatureKnockComponent::FlingShare(Creature->GetRank()) / FMath::Sqrt(FMath::Max(Creature->GetSizeScale(), 0.3f));
	HaulEnd = FMath::Lerp(HaulStart, HaulTo, FMath::Clamp(Share, 0.f, 1.f));
}

void UEmberTollChain::Release(bool bArrived)
{
	bChainOut = false;
	ACreatureBase* Creature = Hooked.Get();
	if (Creature)
	{
		if (UCharacterMovementComponent* Movement = Creature->GetCharacterMovement())
		{
			Movement->Velocity = FVector::ZeroVector;
		}
		if (bArrived && bBitten)
		{
			// Thrown down at his feet: a ring of dust and a jolt.
			if (FWeaponFX* FX = Effects())
			{
				const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				const FVector Feet = Creature->GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
				const FLinearColor Dust = EmberFX::DustColor(Creature);
				for (int32 Index = 0; Index < 9; ++Index)
				{
					const float Angle = 2.f * UE_PI * Index / 9.f;
					const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
					FX->SpawnDustPuff(Feet + Out * 30.f + FVector(0.0, 0.0, 10.0), Out * Random.FRandRange(180.f, 300.f) + FVector(0.0, 0.0, 60.0), Dust, 0.45f,
						22.f, Random.FRandRange(80.f, 110.f), Random.FRandRange(0.8f, 1.2f));
				}
			}
			FViewKick Arrive;
			Arrive.Pitch = -0.3f;
			Arrive.Frequency = 10.f;
			Arrive.Damping = 0.6f;
			Kick(Arrive);
		}
		SnapFrom = ChestOf(*Creature);
	}
	// The chain snaps back to the hand (its linger).
	SnapBack = 0.f;
	StartLinger();
	UE_LOG(LogLooter, Verbose, TEXT("Toll Chain: let go of %s (%s)"), *GetNameSafe(Creature), bArrived ? TEXT("hauled in") : TEXT("short"));
}

bool UEmberTollChain::Linger(float DeltaSeconds)
{
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	if (SnapBack >= 0.f && !bChainOut)
	{
		SnapBack += Dt;
		const float U = FMath::Clamp(SnapBack / SnapSeconds, 0.f, 1.f);
		const FVector Hand = HandNow();
		DrawChain(Hand, FMath::Lerp(SnapFrom, Hand, U * U), 0.35f * (1.f - U));
		if (U >= 1.f)
		{
			SnapBack = -1.f;
		}
	}
	Sparks.Tick(Dt, Effects());
	return bChainOut || SnapBack >= 0.f || !Sparks.IsEmpty();
}

void UEmberTollChain::DrawChain(const FVector& From, const FVector& To, float Slack) const
{
	FWeaponFX* FX = Effects();
	const float Length = static_cast<float>(FVector::Dist(From, To));
	if (!FX || Length < 5.f)
	{
		return;
	}
	// Links along a gentle sag (slack while it flies, nearly straight once taut), each turned a little from the last so they
	// read as a chain, and the hook bright at its head.
	const FVector Along = (To - From) / Length;
	const FVector Side = FVector::CrossProduct(Along, FVector::UpVector).GetSafeNormal();
	const int32 Links = FMath::Clamp(FMath::CeilToInt(Length / LinkEvery), 2, 160);
	for (int32 Index = 0; Index < Links; ++Index)
	{
		const float U = (Index + 0.5f) / Links;
		const FVector Sag = FVector(0.0, 0.0, -4.f * U * (1.f - U) * Slack * FMath::Min(Length, 1200.f) * 0.12f);
		const FVector Middle = FMath::Lerp(From, To, U) + Sag;
		const FVector Twist = (Index % 2 ? Side : FVector::UpVector) * 1.2f;
		FX->AddStreak(Middle - Along * (LinkLength * 0.5f) + Twist, Middle + Along * (LinkLength * 0.5f) - Twist, LinkWidth, Color, LinkGlow);
	}
	FX->AddStreak(To - Along * 16.f, To, 3.6f, Color, LinkGlow * 2.5f);
	FX->AddStreak(To - Along * 8.f - Side * 7.f, To - Along * 2.f, 2.4f, Color, LinkGlow * 2.f);
}
