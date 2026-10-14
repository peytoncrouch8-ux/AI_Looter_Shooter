// ASlimeCreature's reactions (the 2026-10-14 polish, "creatures that react"), over the hops and springs SlimeCreature.cpp and
// SlimeCreatureBody.cpp work: a hit's squash, its core's jiggle and its jelly leaning away from the shot and wobbling back,
// its death (the splat's shape is AnimateBody's), coming back, the notice beat (it squashes low and shudders), its entrance
// in a player's view (welling up out of a puddle), idle life (it sways, and now and then a bubble rises through it), and the
// ring of dust a hunting hop's landing kicks up. Moved here with the reactions it had, to keep SlimeCreature.cpp in size.

#include "Creatures/SlimeCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** A hit squashes it to this height (1 = rest). */
	constexpr float HurtSquash = 0.82f;
	/** A hit leans the jelly away from it this far (degrees; a crit half again), and it wobbles back on a loose spring. */
	constexpr float HurtLean = 8.f;
	constexpr float LeanOmega = 18.f;
	constexpr float LeanDamping = 0.3f;
	constexpr float LeanKickFade = 9.f;
	constexpr float LeanStep = 1.f / 120.f;
	/** At rest it sways this far either way (degrees, forward and to the side), slowly, each slime in its own time. */
	constexpr float SwayForward = 1.5f;
	constexpr float SwaySide = 1.2f;

	/** The notice beat: squashed low (its rest this much lower), shuddering: the squash kicked each way this often. */
	constexpr float NoticeSquash = 0.28f;
	constexpr float NoticeDip = 0.74f;
	constexpr float ShudderSeconds = 0.09f;
	constexpr float ShudderKick = 1.4f;

	/** A bubble rises through it every this many seconds while it rests. */
	constexpr float BubbleEvery[2] = { 2.5f, 5.5f };

	/** Welling up into the world: from a puddle this flat to past its own shape in this long, its brain held this long. */
	constexpr float SwellFrom = 0.08f;
	constexpr float SwellTo = 1.18f;
	constexpr float SwellSeconds = 0.35f;
	constexpr float SwellHold = 0.5f;
}

// ---------------------------------------------------------------------------
// Hits, death and coming back
// ---------------------------------------------------------------------------

void ASlimeCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	if (IsDead())
	{
		return;
	}
	// A squash and a wobble; the core jiggles away from the hit (harder on a crit).
	if (Hop == EHop::Ground && !bTelegraph && !Squash.IsRamping())
	{
		Squash.Ramp(HurtSquash, 0.05f);
	}
	const FVector Away = GetActorTransform().InverseTransformVector(GetActorLocation() - HitLocation).GetSafeNormal();
	CoreVelocity += (Away + FMath::VRand() * 0.5f) * (bCritical ? 220.f : 130.f);
	// The jelly leans away from the hit and wobbles back (ReactLean): a hit on its front tips it back, one from its left to
	// its right. A shotgun's pellets land together: the strongest kick stands.
	const float Hard = HurtLean * (bCritical ? 1.5f : 1.f);
	const FVector2D Kick(static_cast<float>(Away.X) * Hard, static_cast<float>(Away.Y) * Hard);
	if (Kick.SizeSquared() >= LeanKick.SizeSquared())
	{
		LeanKick = Kick;
	}
}

void ASlimeCreature::OnDied()
{
	Hop = EHop::Ground;
	bLeaping = false;
	bTelegraph = false;
	// Death stops all movement, mid-hop too: drop it onto the ground so the puddle lies there, not in the air.
	FVector Ground;
	if (FindGround(GetActorLocation(), 0.f, 1000.f, Ground))
	{
		SetActorLocation(Ground + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), false, nullptr,
			ETeleportType::TeleportPhysics);
	}
}

void ASlimeCreature::OnRespawned()
{
	Hop = EHop::Ground;
	HopTime = 0.f;
	BlockedHops = 0;
	Squash = FSquashSpring();
	CoreOffset = CoreVelocity = FVector::ZeroVector;
	Lean = LeanKick = LeanSpeed = FVector2D::ZeroVector;
	AnimateBody(0.f);
}

void ASlimeCreature::OnPoseThawed()
{
	// The springs stood still while it hopped on unposed, but every hop still kicked the core and started a squash. Carry on
	// from the shape it was last drawn in, with the springs at rest, so it doesn't pop if it's in view: the squashes and
	// kicks of hops long over are dropped, and the springs ease it back to shape. (It's never frozen mid-attack, so no
	// telegraph squash is lost.)
	Squash.Velocity = 0.f;
	Squash.RampTime = Squash.RampLength = 0.f;
	CoreVelocity = FVector::ZeroVector;
	LeanSpeed = FVector2D::ZeroVector;
	AnimateBody(0.f);
}

void ASlimeCreature::SetHitVolumesEnabled(bool bEnabled)
{
	GetMesh()->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

// ---------------------------------------------------------------------------
// The notice beat and its entrance
// ---------------------------------------------------------------------------

void ASlimeCreature::OnNoticed()
{
	// It squashes down low and shudders (ReactSquash, TickReactions), its core jolted up: seen.
	if (Hop == EHop::Ground && !bTelegraph)
	{
		Squash.Ramp(NoticeDip, 0.1f);
	}
	CoreVelocity += FVector(0.f, 0.f, 60.f);
	Voice->Play(FName(CreatureReactCue::SlimeNotice));
}

float ASlimeCreature::ReactSquash() const
{
	return -NoticeSquash * GetNoticeStrength();
}

void ASlimeCreature::PlayEntrance()
{
	// Welling up out of a puddle: from flat it swells up past its shape and wobbles into it, while its brain waits.
	Hop = EHop::Ground;
	Squash.Value = SwellFrom;
	Squash.Velocity = 0.f;
	Squash.Ramp(SwellTo, SwellSeconds);
	CoreOffset = FVector(0.f, 0.f, -15.f);
	CoreVelocity = FVector(0.f, 0.f, 120.f);
	HoldBrain(SwellHold);
	if (UCreatureReactComponent* Reacts = GetReact())
	{
		const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		Reacts->ThrowGraveDirt(Feet, GetSizeScale(), 0.3f);
	}
	Voice->Play(FName(CreatureReactCue::SlimeSwell));
	AnimateBody(0.f);
}

// ---------------------------------------------------------------------------
// Its lean, idle life and its landings
// ---------------------------------------------------------------------------

void ASlimeCreature::TickReactions(float DeltaSeconds)
{
	UCreatureReactComponent* Reacts = GetReact();
	const bool bResting = !IsDead() && Hop == EHop::Ground && !bTelegraph && GetCreatureState() == ECreatureState::Idle;
	// At rest it sways a little, in its own time; anything else and it stands up straight (on the ground's tilt).
	FVector2D Rest = FVector2D::ZeroVector;
	if (bResting)
	{
		const float Phase = 2.f * UE_PI * (Reacts ? Reacts->GetSeed() : 0.f);
		Rest = FVector2D(SwayForward * FMath::Sin(AnimTime * 0.8f + Phase), SwaySide * FMath::Sin(AnimTime * 0.61f + 2.f * Phase));
	}
	// The lean's loose spring, in short even steps as its other springs are (a slow-ticking slime wobbles as a near one).
	const float Seconds = FMath::Min(DeltaSeconds, FCreatureUpdateRate::MaxInterval);
	if (Seconds > 0.f)
	{
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / LeanStep - 0.01f));
		const float Step = Seconds / Steps;
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			LeanKick -= LeanKick * FMath::Min(LeanKickFade * Step, 1.f);
			LeanSpeed += ((Rest + LeanKick - Lean) * (LeanOmega * LeanOmega) - LeanSpeed * (2.f * LeanDamping * LeanOmega)) * Step;
			Lean += LeanSpeed * Step;
		}
	}
	if (!Reacts || IsDead() || IsPoseFrozen())
	{
		return;
	}
	FRandomStream& Dice = Reacts->GetRandom();
	// Its notice beat's shudder: the squash kicked one way and the other, the core jiggling.
	if (IsNoticing() && !Squash.IsRamping())
	{
		ShudderClock -= DeltaSeconds;
		if (ShudderClock <= 0.f)
		{
			ShudderClock = ShudderSeconds;
			ShudderSign = -ShudderSign;
			Squash.Velocity += ShudderSign * ShudderKick * GetNoticeStrength();
			CoreVelocity += FVector(Dice.FRandRange(-30.f, 30.f), Dice.FRandRange(-30.f, 30.f), 0.f);
		}
	}
	if (!bResting)
	{
		return;
	}
	if (!bBubbleClockSet)
	{
		bBubbleClockSet = true;
		NextBubble = Dice.FRandRange(0.5f, BubbleEvery[1]);
	}
	NextBubble -= DeltaSeconds;
	if (NextBubble <= 0.f)
	{
		NextBubble = Dice.FRandRange(BubbleEvery[0], BubbleEvery[1]);
		// A bubble rising through it: the core nudged up and aside, and a little swell as it reaches the top.
		CoreVelocity += FVector(Dice.FRandRange(-25.f, 25.f), Dice.FRandRange(-25.f, 25.f), Dice.FRandRange(55.f, 80.f));
		if (!Squash.IsRamping())
		{
			Squash.Ramp(1.06f, 0.1f);
		}
		Voice->Play(FName(CreatureReactCue::SlimeBubble), 0.4f);
	}
}

FQuat ASlimeCreature::ReactLean() const
{
	// Forward: its top toward its front (about its right-hand axis); to the side: its top toward its right.
	return FQuat(FVector::RightVector, FMath::DegreesToRadians(Lean.X)) * FQuat(FVector::ForwardVector, -FMath::DegreesToRadians(Lean.Y));
}

void ASlimeCreature::NoteLanding()
{
	UCreatureReactComponent* Reacts = GetReact();
	if (!Reacts || (!GetTarget() && !bLeaping) || IsPoseFrozen())
	{
		return;
	}
	// A ring of dust splashed out round its foot by the landing: more for a leap.
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FRandomStream& Dice = Reacts->GetRandom();
	const int32 Puffs = bLeaping ? 4 : 2;
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		const float Angle = 2.f * UE_PI * (Index + Dice.FRand() * 0.5f) / Puffs;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		Reacts->KickFootfall(Feet + Out * (FootRadius * 0.8f * Scale), Out * (250.f * Scale), Scale * 1.2f);
	}
}
