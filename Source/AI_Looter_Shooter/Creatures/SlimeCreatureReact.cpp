// ASlimeCreature's reactions (the 2026-10-14 polish, "creatures that react"), over the hops and springs SlimeCreature.cpp and
// SlimeCreatureBody.cpp work: a hit's squash, its core's jiggle and its jelly leaning away from the shot and wobbling back,
// its death (the splat's shape is AnimateBody's), coming back, the notice beat (it squashes low and shudders), its entrance
// in a player's view (welling up out of a puddle), idle life (it sways, and now and then a bubble rises through it), and the
// ring of dust a hunting hop's landing kicks up. Moved here with the reactions it had, to keep SlimeCreature.cpp in size.
// The VFX pass (Saved/Vfx/PLAN.md): its kill's own layers and its gel puddle splashed out on the ground (CreatureKillLook).
// The hit-feel pass (HITS lane D, 2026-10-28): a hit squashes it mid-hop and in the telegraph too (a hop is knocked off its line), a core crit
// shoves the core to the gel's far wall and ripples it, a Big hit flattens it, a stagger flattens and quivers it, and a kill goes by its style
// (popped, splattered, an overkill swollen through its hit-stop); the wounds, drips and stagger cue are SlimeCreatureBody.cpp's.

#include "Creatures/SlimeCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/GunfeelReactTuning.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitPose.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** A hit squashes it to this height at a flinch strength of 1 (1 = rest; Gunfeel's FlinchStrength scales the dent), never under HurtSquashMost. */
	constexpr float HurtSquash = 0.82f;
	constexpr float HurtSquashMost = 0.55f;
	/** The core's jiggle grows with the hit up to this many times its own. */
	constexpr float HurtCoreMost = 2.5f;
	/**
	 * A hit leans the jelly away from it this far at strength 1 (degrees; a crit half again; 8 before every hit was sized,
	 * 10 so a rifle round reads at 20 m), never past HurtLeanMost, and it wobbles back on a loose spring.
	 */
	constexpr float HurtLean = 10.f;
	constexpr float HurtLeanMost = 30.f;
	constexpr float LeanOmega = 18.f;
	constexpr float LeanDamping = 0.3f;
	constexpr float LeanKickFade = 9.f;
	constexpr float LeanStep = 1.f / 120.f;
	/** At rest it sways this far either way (degrees, forward and to the side), slowly, each slime in its own time. */
	constexpr float SwayForward = 1.5f;
	constexpr float SwaySide = 1.2f;

	// --- The hit-feel pass (HITS lane D, 2026-10-28; each moves toward its number by Looter.Hits.SlimeScale) ---
	/** A hit on the gel's upper part (above this share of its height) squashes it this much more. */
	constexpr float TopShare = 0.65f;
	constexpr float TopSquash = 1.3f;
	/** A hit on its side leans it this much more, and a lean is this many degrees at strength 1 (10 before). */
	constexpr float SideLean = 1.3f;
	constexpr float HurtLeanNew = 12.f;
	/** A Big hit flattens it to this height. */
	constexpr float BigSquash = 0.6f;
	/** In the telegraph a hit dents the held squash this share of its depth, never lower than TelegraphFloor (so the leap still reads). */
	constexpr float TelegraphShare = 0.5f;
	constexpr float TelegraphFloor = 0.4f;
	/** Mid-hop the dent is a kick on the squash spring (velocity per unit of dent: the spring's 20/s makes the dip about that deep). */
	constexpr float AirKick = 16.f;
	/** A core crit's ripple: the squash shakes 9 Hz for 0.35 s, this much at first (height scale). */
	constexpr float RippleHz = 9.f;
	constexpr float RippleSeconds = 0.35f;
	constexpr float RippleSize = 0.12f;
	/** A stagger flattens it to this height and quivers 14 Hz (the kick sized for about +-0.04 on the spring). */
	constexpr float StaggerSquash = 0.62f;
	constexpr float QuiverHz = 14.f;
	constexpr float QuiverKick = 7.f;
	/** A core crit kill's gel is already this squashed as it dies: it deflates into its puddle. */
	constexpr float PoppedStart = 0.45f;

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
	// Three quick hits while it waits its turn may send it hopping aside (SlimeCreatureSplit.cpp).
	NoteHitForDodge();
	// How much of the hit-feel pass shows (Looter.Hits.SlimeScale; 0 is the slime as it was): each new number is its old one
	// moved this far toward the new.
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float Planned = FMath::Max(Feel.SlimeScale, 0.f);
	auto Mix = [Planned](float Old, float New) { return Old + (New - Old) * Planned; };
	const FCreatureHit& Hit = GetLastHit();
	const float Strength = Hit.FlinchStrength(FGunfeelReactTuning::Live());
	const bool bBig = Hit.Weight == EHitWeight::Big;
	const bool bTop = HitHeightShare(HitLocation) > TopShare;
	const float Max = Health->GetMaxHealth();
	const bool bWorn = Feel.Wounds && Max > 0.f && WoundStage(Health->GetHealth() / Max) >= 2;

	// A squash and a wobble, sized by the hit (Gunfeel: a revolver round dents it deep, a close blast flattens it); a hit on its
	// top squashes more. Mid-hop it is kicked on the spring, in the telegraph half as deep so the leap still reads.
	const float Dent = (1.f - HurtSquash) * Strength * (bTop ? Mix(1.f, TopSquash) : 1.f);
	float Squashed = FMath::Max(1.f - Dent, HurtSquashMost);
	if (bBig && Planned > 0.f)
	{
		Squashed = FMath::Min(Squashed, BigSquash);
	}
	if (Planned <= 0.f)
	{
		if (Hop == EHop::Ground && !bTelegraph && !Squash.IsRamping())
		{
			Squash.Ramp(Squashed, 0.05f);
		}
	}
	else if (!Squash.IsRamping())
	{
		if (bTelegraph)
		{
			Squash.Ramp(FMath::Max(Squash.Value - Dent * TelegraphShare, TelegraphFloor), 0.05f);
		}
		else if (Hop == EHop::Ground)
		{
			Squash.Ramp(Squashed, 0.05f);
		}
		else
		{
			Squash.Velocity -= Dent * AirKick;
		}
	}

	// The core jiggles away from the hit (harder on a crit, harder again when the slime is nearly gone); a core crit shoves it on
	// along the shot to the gel's far wall and sets the gel rippling.
	const FVector Away = GetActorTransform().InverseTransformVector(GetActorLocation() - HitLocation).GetSafeNormal();
	FVector Shoved = Away + FMath::VRand() * 0.5f;
	if (bCritical && Planned > 0.f)
	{
		const FVector Along = GetActorTransform().InverseTransformVector(Hit.Direction).GetSafeNormal();
		if (!Along.IsNearlyZero())
		{
			Shoved = Along + FMath::VRand() * 0.15f;
		}
		RippleAge = 0.f;
		RippleAmount = RippleSize * FMath::Min(Planned, 2.f);
	}
	const float CoreKick = (bCritical ? Mix(220.f, 320.f) : 130.f) * (bWorn ? Mix(1.f, 1.3f) : 1.f);
	CoreVelocity += Shoved * (CoreKick * FMath::Min(Strength, HurtCoreMost));

	// The jelly leans away from the hit and wobbles back (ReactLean): a hit on its front tips it back, one from its left to
	// its right; a hit on its side leans it more. A shotgun's pellets land together: the strongest kick stands.
	const float Hard = FMath::Min(Mix(HurtLean, HurtLeanNew) * (bCritical ? 1.5f : 1.f) * Strength * (bTop ? 1.f : Mix(1.f, SideLean)), HurtLeanMost);
	const FVector2D Kick(static_cast<float>(Away.X) * Hard, static_cast<float>(Away.Y) * Hard);
	if (Kick.SizeSquared() >= LeanKick.SizeSquared())
	{
		LeanKick = Kick;
	}

	if (Planned > 0.f)
	{
		// A hop in the air is knocked off its line by the shot (never past a ledge); a Big hit leaves a splat behind it.
		if (Hop == EHop::Air)
		{
			KnockInAir(Hit.Direction, Hit.BurstShare);
		}
		if (bBig)
		{
			CreatureKillLook::LayBigHitSplat(*this, Hit);
		}
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
	// A Restless or better slime bursts into slimelets where it lies (SlimeCreatureSplit.cpp).
	SplitOnDeath();
	// How it goes by the killing hit (HITS): a core crit pops (the gel is already deflating), a blast splatters, an overkill
	// swells on the killing frame, held by the hit-stop, and bursts when it ends (TickWounds).
	DeathStyle = DeathStyleFor(GetLastHit());
	bOverkillHeld = false;
	if (DeathStyle == EDeathStyle::Overkill)
	{
		Squash.Value = OverInflate;
		Squash.Velocity = 0.f;
		Squash.RampTime = Squash.RampLength = 0.f;
		OverkillHit = GetLastHit();
		bOverkillHeld = true;
		return;
	}
	if (DeathStyle == EDeathStyle::Popped)
	{
		Squash.Value = PoppedStart;
		Squash.Velocity = 0.f;
		Squash.RampTime = Squash.RampLength = 0.f;
	}
	// On the ground now: the kill's own layers (fat blobs with the shot, a gel mist and spray, the style's gel burst) and its puddle splashed out.
	CreatureKillLook::Play(*this, GetLastHit(), /*bSplat*/ true);
}

void ASlimeCreature::OnRespawned()
{
	Hop = EHop::Ground;
	HopTime = 0.f;
	BlockedHops = 0;
	Squash = FSquashSpring();
	CoreOffset = CoreVelocity = FVector::ZeroVector;
	Lean = LeanKick = LeanSpeed = FVector2D::ZeroVector;
	// The hit-feel pass's marks gone with it: whole again, standing, not dripping.
	DeathStyle = EDeathStyle::Body;
	bOverkillHeld = false;
	RippleAge = 100.f;
	DripClock = 0.f;
	bStaggerCued = false;
	WoundScale = 1.f;
	GetMesh()->SetRelativeScale3D(FVector::OneVector);
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
	// The notice beat's low squash; a stagger flattens it to StaggerSquash (its quiver is TickReactions' kicks); a core crit's ripple on top.
	float Lower = -NoticeSquash * GetNoticeStrength() - (1.f - StaggerSquash) * GetStaggerShare();
	if (RippleAge < RippleSeconds)
	{
		Lower += RippleAmount * FMath::Sin(UE_TWO_PI * RippleHz * RippleAge) * (1.f - RippleAge / RippleSeconds);
	}
	return Lower;
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
	// Its wounds, stagger and an overkill's wait on its hit-stop (SlimeCreatureBody.cpp), and a core crit's ripple moves on.
	TickWounds(DeltaSeconds);
	RippleAge += DeltaSeconds;
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
	// A stagger quivers it: the squash kicked one way and the other, 14 times a second.
	if (GetStaggerShare() > 0.f && !Squash.IsRamping())
	{
		QuiverClock -= DeltaSeconds;
		if (QuiverClock <= 0.f)
		{
			QuiverClock = 0.5f / QuiverHz;
			QuiverSign = -QuiverSign;
			Squash.Velocity += QuiverSign * QuiverKick * GetStaggerShare();
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
