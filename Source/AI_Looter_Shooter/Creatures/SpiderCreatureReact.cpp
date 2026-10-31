// ASpiderCreature's reactions (the 2026-10-14 polish, "creatures that react"), over the gait SpiderCreature.cpp walks: a hit's
// jolt and its tilt away from the shot, its death's moments (the throe, drop and curl are SpiderCreatureDeath.cpp's; here the
// drop's thud, a last twitch or two before the body lies still, the shudder before it sinks), coming back, the notice beat
// (it rears with its front legs up and its fangs flared, hissing), its entrance in a player's view (climbing out of the
// dirt; idle life while it stands and the notice beat's pose are SpiderCreatureIdle.cpp's) and the dust its feet kick up
// running. Moved here with the reactions it had, to keep SpiderCreature.cpp under the size
// guide. The VFX pass (Saved/Vfx/PLAN.md): its kill's own layers as it dies (gore by the gun, mist, dust skirt, tier rings;
// CreatureKillLook), and its ichor pooling out from under it once the body lands. The hit feel pass (HITS, 2026-10-28) raised
// a living spider's kick and tilt here and hands each hit on to SpiderCreatureHurt.cpp (zones, stagger, wounds, kill styles).

#include "Creatures/SpiderCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/GunfeelReactTuning.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitReaction.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	/** A hit's kick holds this long (s) before it fades, at this share of its peak (the jolt's spring is SpiderCreature.cpp's). */
	constexpr float HurtKickHold = 0.06f;
	constexpr float HurtKickShare = 0.85f;
	/**
	 * A hit's kick at a flinch strength of 1 (cm at size 1; a crit's), and the most any is (Gunfeel's FlinchStrength runs
	 * 0.6 to 3.5). 9 (it was 7 before every hit was sized) so a rifle round's reads at 20 m.
	 */
	constexpr float HurtKickBody = 9.f;
	constexpr float HurtKickCrit = 18.f;
	constexpr float HurtKickMost = 40.f;
	/**
	 * A hit tilts the body away from the shot this far at strength 1 (degrees; a crit half again): nose up from the front,
	 * rolled from the side; never past HurtTiltMost. (5 and 7 before; the tilt is what reads farthest.)
	 */
	constexpr float HurtTiltPitch = 7.f;
	constexpr float HurtTiltRoll = 10.f;
	constexpr float HurtTiltMost = 28.f;
	/**
	 * The hit feel pass (HITS 2026-10-28) raised a living spider's body kick to 12 and its tilt to 9 and 13, all of it at
	 * Looter.Hits.SpiderScale 1 (0: the numbers above). The killing hit keeps those: its death shows the kill. From behind
	 * it lurches forward this far instead (the abdomen takes the shove: SpiderCreatureHurt.cpp).
	 */
	constexpr float FeltKickBody = 12.f;
	constexpr float FeltTiltPitch = 9.f;
	constexpr float FeltTiltRoll = 13.f;
	constexpr float BehindLurch = 6.f;
	/** The abdomen's kick on a hit tops out here (degrees). */
	constexpr float HurtAbdomenMost = 22.f;
	/** A flinch not from a hit (a boss's own jolt calling OnHurt): the old kick (body, crit) and tilt (pitch, roll). */
	constexpr float ScriptedKick[2] = { 7.f, 14.f };
	constexpr float ScriptedTilt[2] = { 5.f, 7.f };

	/**
	 * Its death's two twitches: when (a full-size death's seconds after it dies, both after the curl: a bigger one's come as
	 * much later as its death runs slower), each this long, the foot kicked this far (cm at size 1).
	 */
	constexpr float FirstTwitch[2] = { 1.5f, 1.9f };
	constexpr float SecondTwitch[2] = { 2.4f, 3.2f };
	static_assert(FirstTwitch[0] > ASpiderCreature::CurledSeconds, "a twitch comes once the legs are curled");
	constexpr float TwitchSeconds = 0.26f;
	constexpr float TwitchReach = 11.f;
	/** After its last twitch it lies still this long before its pose stops being worked out. */
	constexpr float CorpseSettleSeconds = 0.5f;

	/** Its feet throw dust running at this share of its chase speed or faster. */
	constexpr float FootfallPace = 0.55f;

}

// ---------------------------------------------------------------------------
// Hits, death and coming back
// ---------------------------------------------------------------------------

void ASpiderCreature::OnAttackStarted()
{
	AbdomenKick = -6.f; // abdomen tips down as the front rears up
}

void ASpiderCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	// On its perch, shot: it springs (SpiderCreatureLurk.cpp). In a fight, hurt enough lately: it backs off (SpiderCreatureHunt.cpp).
	{
		const FCreatureHit& Taken = GetLastHit();
		if (Taken.bFresh && !Taken.bKilled)
		{
			if (Lurk.Phase == ESpiderLurkPhase::Perched)
			{
				Lurk.bHurt = true;
			}
			else if (Lurk.Phase == ESpiderLurkPhase::None)
			{
				NoteHurtForHunt(Taken);
			}
			else if (Lurk.bFromDen)
			{
				// Coming out of a den: a hard hit knocks it off its wall (SpiderCreatureDen.cpp).
				NoteDenHit(Taken);
			}
		}
	}
	// Flinch away from the hit, sized by it (Gunfeel: a revolver round rocks it, a rifle round jerks it, a close blast
	// throws its body back); headshots harder. A shotgun's pellets land together: the strongest kick stands (the last
	// pellet carries the blast's whole share), they don't add up.
	// A flinch the body plays itself (the Gravemother's sink and stone jolts call this straight) keeps its old size.
	const FCreatureHit& Hit = GetLastHit();
	const bool bSized = Hit.bFresh;
	const float Strength = Hit.FlinchStrength(FGunfeelReactTuning::Live());
	// The hit feel's share of the raise (a living spider's sized hits only).
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float Felt = bSized && !Hit.bKilled ? FMath::Clamp(static_cast<float>(Feel.SpiderScale), 0.f, 2.f) : 0.f;
	const float BodyKick = HurtKickBody + (FeltKickBody - HurtKickBody) * Felt;
	const float Base = bSized ? (bCritical ? HurtKickCrit : BodyKick) : (bCritical ? ScriptedKick[1] : ScriptedKick[0]);
	float Peak = FMath::Min(Base * Strength, HurtKickMost) * GetSizeScale();
	FVector Away = (BodyFrame.GetLocation() - HitLocation).GetSafeNormal();
	const bool bBehind = Felt > 0.f && Feel.Zones && Hit.Side == EHitSide::Back;
	if (bBehind)
	{
		// Shot from behind, it lurches forward along its facing, a shorter jolt than a shove.
		Away = GetActorForwardVector().GetSafeNormal2D();
		Peak = FMath::Min(BehindLurch * Strength * Felt, HurtKickMost) * GetSizeScale();
	}
	const FVector Kick = Away * (Peak * HurtKickShare);
	if (Kick.SizeSquared() >= HurtKick.SizeSquared())
	{
		HurtKick = Kick;
		HurtHold = HurtKickHold;
		// And tilted away from the shot: a hit on its front lifts its nose, one from the side rolls it over away from it.
		const FVector Local = FRotator(0.f, GetActorRotation().Yaw, 0.f).UnrotateVector(Away);
		const float Hard = (bCritical ? 1.5f : 1.f) * Strength;
		const float Pitch = bSized ? HurtTiltPitch + (FeltTiltPitch - HurtTiltPitch) * Felt : ScriptedTilt[0];
		const float Roll = bSized ? HurtTiltRoll + (FeltTiltRoll - HurtTiltRoll) * Felt : ScriptedTilt[1];
		HurtTiltKick = FVector(FMath::Clamp(-Local.X * Pitch * Hard, -HurtTiltMost, HurtTiltMost),
			FMath::Clamp(Local.Y * Roll * Hard, -HurtTiltMost, HurtTiltMost), 0.f);
	}
	AbdomenKick = FMath::Max(AbdomenKick, FMath::Min((bCritical ? 10.f : 6.f) * Strength, HurtAbdomenMost));
	// Where it was hit, on top (SpiderCreatureHurt.cpp): its twist, a leg's buckle, a head crit's snap, the abdomen's wobble.
	if (bSized)
	{
		ReactToHitFeel(Hit, HitLocation, Strength);
		// A Big hit that didn't kill it leaves a small splat behind it along the shot (its rules and budget are CreatureKillLook's).
		if (!Hit.bKilled && Hit.Weight == EHitWeight::Big)
		{
			CreatureKillLook::LayBigHitSplat(*this, Hit);
		}
	}
}

void ASpiderCreature::OnDied()
{
	// Its throe starts from its body and each foot as they are now (a foot mid-step is set down), each leg on its own dice
	// (SpiderCreatureDeath.cpp); then the gait lets go of them. Before the climb out of the dirt is cut short: its height
	// folds into the body's. Its style first (the killing hit's: SpiderCreatureHurt.cpp), whose timing the pose is laid out
	// by; a stagger's crouch folds into where its death starts, then lets go.
	KillStyle = PickKillStyle(GetLastHit());
	// Killed on its wall or in the air: its body comes down to the ground under it first, its death playing from where it
	// hung (SpiderCreatureLurk.cpp). Its cunning's moves stop.
	if (IsInDen())
	{
		// In its den's mouth: brought out of the wall first (SpiderCreatureDen.cpp).
		FallFromDen();
	}
	else if (Lurk.Phase != ESpiderLurkPhase::None)
	{
		FallFromPerch();
	}
	Hunt = FSpiderHuntState();
	bCirclingNow = false;
	RecordDeathPose();
	StaggerShown = 0.f;
	for (FLeg& Leg : Legs)
	{
		Leg.BuckleAge = -1.f;
		Leg.Skid = FVector::ZeroVector;
		Leg.bLimp = false;
	}
	for (FLeg& Leg : Legs)
	{
		Leg.bStepping = false;
	}
	SteppingGroup = INDEX_NONE;
	IdleAct = EIdleAct::None;
	IdleWeight = 0.f;
	EmergeTime = -1.f;
	// Curled up, a leg or two kicks its last: when and which, by its own dice.
	if (UCreatureReactComponent* Reacts = GetReact())
	{
		FRandomStream& Dice = Reacts->GetRandom();
		const int32 Count = FMath::Max(Legs.Num(), 1);
		const float Slower = DeathTimeScale(GetSizeScale());
		TwitchAt[0] = Dice.FRandRange(FirstTwitch[0], FirstTwitch[1]) * Slower;
		TwitchAt[1] = Dice.FRandRange(SecondTwitch[0], SecondTwitch[1]) * Slower;
		TwitchLeg[0] = Dice.RandRange(0, Count - 1);
		TwitchLeg[1] = (TwitchLeg[0] + Dice.RandRange(2, 6)) % Count;
	}
	// The kill's own layers over its burst: gore thrown by the gun that killed it, a wet mist, the dust it drops in, its
	// tier's rings. Its pool spreads once the body lands (TickReactions).
	CreatureKillLook::Play(*this, GetLastHit(), /*bSplat*/ false);
	// Its style's own layer: an overkill's chitin plates (the sound and splats are CreatureKillLook's).
	PlayKillStyle(GetLastHit());
}

void ASpiderCreature::OnRespawned()
{
	ResetHitFeel();
	Lurk = FSpiderLurkState();
	Den = FSpiderDenState();
	Hunt = FSpiderHuntState();
	Gait.bRead = false;
	Gait.bWasAirborne = false;
	Gait.LandDip = Gait.LandDipSpeed = 0.f;
	IdleAct = EIdleAct::None;
	IdleWeight = 0.f;
	EmergeTime = -1.f;
	if (bRigReady)
	{
		PlantLegs();
		AnimateBody(0.f);
		AnimateLegs(0.f);
	}
}

void ASpiderCreature::OnPoseThawed()
{
	if (!bRigReady)
	{
		return;
	}
	Gait.bRead = false;
	// Coming out of its den: posed where its beats have it now (SpiderCreatureDen.cpp).
	if (IsInDen())
	{
		AnimateDen(0.f);
		return;
	}
	// On its perch: its feet put back on its surface (SpiderCreatureCling.cpp).
	if (IsClinging())
	{
		PlantOnSurface();
		AnimateCling(0.f);
		return;
	}
	// Its feet stayed where they last stood while it walked on unposed. This usually happens just before a view reaches
	// it, but a quick turn can show it, so only what's out of place moves: a step half taken lands, feet it walked away
	// from are put down under it afresh, and feet still near their spots stay down for the gait to step in as it walks.
	const FVector ActorLocation = GetActorLocation();
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const float Scale = GetSizeScale();
	const float StepThreshold = StepThresholdAt(static_cast<float>(GetVelocity().Size2D())) * Scale;
	for (FLeg& Leg : Legs)
	{
		if (Leg.bStepping)
		{
			Leg.Foot = Leg.StepTo;
			Leg.bStepping = false;
			Leg.StepAlpha = 1.f;
		}
		const FVector Rest = ActorLocation + Yaw.RotateVector(Leg.Rest * Scale);
		if (Leg.bNeedsReset || FVector::Dist2D(Leg.Foot, Rest) > StepThreshold)
		{
			Leg.Foot = GroundUnder(Rest);
			Leg.bNeedsReset = false;
		}
	}
	SteppingGroup = INDEX_NONE;
	// The body settles over its feet at once: it may have walked up or down a slope since it was last posed.
	bBodyInitialized = false;
	AnimateBody(0.f);
	AnimateLegs(0.f);
}

void ASpiderCreature::OnSizeChanged()
{
	// Promoted (or resized) in play: its feet go down where the new size puts them, and the body settles over them at once.
	if (bRigReady && IsInDen())
	{
		AnimateDen(0.f);
	}
	else if (bRigReady && IsClinging())
	{
		PlantOnSurface();
		AnimateCling(0.f);
	}
	else if (bRigReady)
	{
		PlantLegs();
		AnimateBody(0.f);
		AnimateLegs(0.f);
	}
}

void ASpiderCreature::SetHitVolumesEnabled(bool bEnabled)
{
	GetMesh()->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

bool ASpiderCreature::IsCorpseSettled() const
{
	// Still from after its last twitch until its last shudder, just before it sinks.
	const float Time = GetStateTime();
	return IsDead() && Time > FMath::Max(TwitchAt[0], TwitchAt[1]) + TwitchSeconds + CorpseSettleSeconds && Time < CorpseTime - ShudderSeconds;
}

// ---------------------------------------------------------------------------
// The notice beat and its entrance
// ---------------------------------------------------------------------------

void ASpiderCreature::OnNoticed()
{
	// It rears as it sees them (ReactBody, ReactFootOffset): the abdomen tips down, and the hiss of chitin under its alert
	// cry. (An idle act under way gives its legs back as its weight eases off: TickReactions.)
	AbdomenKick = -5.f;
	Voice->Play(FName(CreatureReactCue::SpiderNotice));
}

void ASpiderCreature::PlayEntrance()
{
	// A lurker comes its ambush's way (on its perch), never out of the dirt.
	if (!bRigReady || Lurk.Phase != ESpiderLurkPhase::None)
	{
		return;
	}
	// Out of its burrow: the body starts under the ground with its legs reaching up out of it to the surface, and climbs
	// out (EmergeDrop) while its brain waits, dirt thrown up round it.
	EmergeTime = 0.f;
	HoldBrain(EmergeSeconds);
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (UCreatureReactComponent* Reacts = GetReact())
	{
		Reacts->ThrowGraveDirt(Feet, Scale, 0.8f);
	}
	Voice->Play(FName(CreatureReactCue::SpiderEmerge));
	// Posed under the ground at once: never a frame standing on top first.
	PlantLegs();
	AnimateBody(0.f);
	AnimateLegs(0.f);
}

float ASpiderCreature::EmergeDrop() const
{
	if (EmergeTime < 0.f)
	{
		return 0.f;
	}
	const float Out = FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(EmergeTime / EmergeSeconds, 0.f, 1.f), 2.f);
	return -(RideHeight + 30.f) * GetSizeScale() * (1.f - Out);
}

// ---------------------------------------------------------------------------
// Its clocks (idle life: SpiderCreatureIdle.cpp), its death's twitches, its footfalls
// ---------------------------------------------------------------------------

void ASpiderCreature::TickReactions(float DeltaSeconds)
{
	if (EmergeTime >= 0.f)
	{
		EmergeTime += DeltaSeconds;
		if (EmergeTime >= EmergeSeconds)
		{
			EmergeTime = -1.f;
		}
	}
	if (IsDead())
	{
		// The body hits the ground on its death cry's knock: a thud and a puff of dust (SpiderCreatureDeath.cpp).
		const float Time = GetStateTime();
		const float Lands = GetDeathDropTime();
		if (Time >= Lands && Time - DeltaSeconds < Lands)
		{
			LandDeadBody();
			// Its ichor pools out from under the body as it lies (the VFX pass's splat).
			CreatureKillLook::LaySplat(*this);
		}
		// Each twitch jolts the abdomen too, with a faint dry tick of chitin; its last shudder has a fainter one.
		for (const float At : TwitchAt)
		{
			if (Time >= At && Time - DeltaSeconds < At)
			{
				AbdomenKick = 5.f;
				Voice->Play(FName(CreatureReactCue::SpiderTwitch), 0.5f);
			}
		}
		const float Shudders = CorpseTime - ShudderSeconds;
		if (Time >= Shudders && Time - DeltaSeconds < Shudders)
		{
			Voice->Play(FName(CreatureReactCue::SpiderTwitch), 0.35f);
		}
		return;
	}
	// The notice pose eases after the beat's own curve, so a beat a hit cuts short lets the legs down rather than dropping them.
	NoticePose = DeltaSeconds > 0.f ? FMath::FInterpTo(NoticePose, GetNoticeStrength(), DeltaSeconds, 14.f) : GetNoticeStrength();
	// Its idle life: a tap, a groom, a weight shift (SpiderCreatureIdle.cpp).
	TickIdleLife(DeltaSeconds);
}

FVector ASpiderCreature::DeathTwitch(int32 LegIndex, float Time, const FVector& Outward, const FVector& Up) const
{
	FVector Kick = FVector::ZeroVector;
	for (int32 Twitch = 0; Twitch < 2; ++Twitch)
	{
		const float Into = Time - TwitchAt[Twitch];
		if (LegIndex == TwitchLeg[Twitch] && Into > 0.f && Into < TwitchSeconds)
		{
			// Out and up and back, easing from rest to rest.
			Kick += (Outward * 0.6f + Up) * (TwitchReach * GetSizeScale() * FMath::Square(FMath::Sin(UE_PI * Into / TwitchSeconds)));
		}
	}
	return Kick;
}

void ASpiderCreature::NoteFootLanded(const FLeg& Leg, float Speed)
{
	if (Speed < ChaseSpeed * FootfallPace || IsPoseFrozen())
	{
		return;
	}
	// Every other landing: eight feet stepping a few times a second each would be a cloud, not footfalls.
	if ((++FootfallsLanded & 1) == 0)
	{
		return;
	}
	if (UCreatureReactComponent* Reacts = GetReact())
	{
		Reacts->KickFootfall(Leg.Foot, GetVelocity(), GetSizeScale());
	}
}
