// ASpiderCreature's reactions (the 2026-10-14 polish, "creatures that react"), over the gait SpiderCreature.cpp walks: a hit's
// jolt and its tilt away from the shot, its death's moments (the throe, drop and curl are SpiderCreatureDeath.cpp's; here the
// drop's thud, a last twitch or two before the body lies still, the shudder before it sinks), coming back, the notice beat
// (it rears with its front legs up and its fangs flared, hissing), its entrance in a player's view (climbing out of the
// dirt), idle life while it stands (a front leg tapped, the front legs groomed at its fangs, each spider on its own dice)
// and the dust its feet kick up running. Moved here with the reactions it had, to keep SpiderCreature.cpp under the size
// guide. The VFX pass (Saved/Vfx/PLAN.md): its kill's own layers as it dies (gore by the gun, mist, dust skirt, tier rings;
// CreatureKillLook), and its ichor pooling out from under it once the body lands.

#include "Creatures/SpiderCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/GunfeelReactTuning.h"
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
	/** The abdomen's kick on a hit tops out here (degrees). */
	constexpr float HurtAbdomenMost = 22.f;
	/** A flinch not from a hit (a boss's own jolt calling OnHurt): the old kick (body, crit) and tilt (pitch, roll). */
	constexpr float ScriptedKick[2] = { 7.f, 14.f };
	constexpr float ScriptedTilt[2] = { 5.f, 7.f };

	/** The notice beat: it rears this far (degrees, and cm at size 1), its fangs spread this wide, its front legs this share of the way to raised. */
	constexpr float NoticeRear = 16.f;
	constexpr float NoticeRise = 10.f;
	constexpr float NoticeFang = 30.f;
	constexpr float NoticeLegRaise = 0.55f;

	/** Idle life: an act every this many seconds or so, a tap this long (lifted this high, cm at size 1) or a groom this long. */
	constexpr float IdleEvery[2] = { 3.5f, 8.f };
	constexpr float TapShare = 0.6f;
	constexpr float TapSeconds = 0.9f;
	constexpr float TapLift = 13.f;
	constexpr float GroomSeconds = 1.8f;
	constexpr float GroomRubHz = 3.2f;
	/** Where a groom draws the front feet: before the fangs (the body's frame at size 1: ahead, out to each side, below the thorax). */
	const FVector GroomSpot(60.f, 14.f, -18.f);

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

	/** Up over In seconds, held, and down over the last Out seconds of Length. */
	float SpiderEnvelope(float Time, float Length, float In, float Out)
	{
		return FMath::SmoothStep(0.f, In, Time) * (1.f - FMath::SmoothStep(Length - Out, Length, Time));
	}
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
	// Flinch away from the hit, sized by it (Gunfeel: a revolver round rocks it, a rifle round jerks it, a close blast
	// throws its body back); headshots harder. A shotgun's pellets land together: the strongest kick stands (the last
	// pellet carries the blast's whole share), they don't add up.
	// A flinch the body plays itself (the Gravemother's sink and stone jolts call this straight) keeps its old size.
	const bool bSized = GetLastHit().bFresh;
	const float Strength = GetLastHit().FlinchStrength(FGunfeelReactTuning::Live());
	const float Base = bSized ? (bCritical ? HurtKickCrit : HurtKickBody) : (bCritical ? ScriptedKick[1] : ScriptedKick[0]);
	const float Peak = FMath::Min(Base * Strength, HurtKickMost) * GetSizeScale();
	const FVector Away = (BodyFrame.GetLocation() - HitLocation).GetSafeNormal();
	const FVector Kick = Away * (Peak * HurtKickShare);
	if (Kick.SizeSquared() >= HurtKick.SizeSquared())
	{
		HurtKick = Kick;
		HurtHold = HurtKickHold;
		// And tilted away from the shot: a hit on its front lifts its nose, one from the side rolls it over away from it.
		const FVector Local = FRotator(0.f, GetActorRotation().Yaw, 0.f).UnrotateVector(Away);
		const float Hard = (bCritical ? 1.5f : 1.f) * Strength;
		const float Pitch = bSized ? HurtTiltPitch : ScriptedTilt[0];
		const float Roll = bSized ? HurtTiltRoll : ScriptedTilt[1];
		HurtTiltKick = FVector(FMath::Clamp(-Local.X * Pitch * Hard, -HurtTiltMost, HurtTiltMost),
			FMath::Clamp(Local.Y * Roll * Hard, -HurtTiltMost, HurtTiltMost), 0.f);
	}
	AbdomenKick = FMath::Max(AbdomenKick, FMath::Min((bCritical ? 10.f : 6.f) * Strength, HurtAbdomenMost));
}

void ASpiderCreature::OnDied()
{
	// Its throe starts from its body and each foot as they are now (a foot mid-step is set down), each leg on its own dice
	// (SpiderCreatureDeath.cpp); then the gait lets go of them. Before the climb out of the dirt is cut short: its height
	// folds into the body's.
	RecordDeathPose();
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
}

void ASpiderCreature::OnRespawned()
{
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
	if (bRigReady)
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
	if (!bRigReady)
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
// Idle life, its death's twitches, its footfalls
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
	UCreatureReactComponent* Reacts = GetReact();
	if (IsDead())
	{
		// The body hits the ground on its death cry's knock: a thud and a puff of dust (SpiderCreatureDeath.cpp).
		const float Time = GetStateTime();
		const float Lands = DropSeconds * DeathTimeScale(GetSizeScale());
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
	if (!Reacts)
	{
		return;
	}
	FRandomStream& Dice = Reacts->GetRandom();
	if (!bIdleClockSet)
	{
		bIdleClockSet = true;
		NextIdleAct = Dice.FRandRange(1.f, IdleEvery[1]);
	}
	// Standing about with nobody to hunt, now and then it taps a leg or grooms; anything else takes its legs back at once.
	const bool bCalm = GetCreatureState() == ECreatureState::Idle && GetVelocity().SizeSquared2D() < 100.0 && !IsNoticing()
		&& EmergeTime < 0.f && !IsPoseFrozen();
	IdleWeight = FMath::FInterpTo(IdleWeight, bCalm ? 1.f : 0.f, DeltaSeconds, bCalm ? 4.f : 12.f);
	if (IdleAct != EIdleAct::None)
	{
		IdleActTime += DeltaSeconds;
		const float Length = IdleAct == EIdleAct::Tap ? TapSeconds : GroomSeconds;
		if (IdleActTime >= Length || (!bCalm && IdleWeight < 0.01f))
		{
			IdleAct = EIdleAct::None;
			IdleActTime = 0.f;
		}
		return;
	}
	if (!bCalm)
	{
		return;
	}
	NextIdleAct -= DeltaSeconds;
	if (NextIdleAct <= 0.f)
	{
		IdleAct = Dice.FRand() < TapShare ? EIdleAct::Tap : EIdleAct::Groom;
		IdleActTime = 0.f;
		IdleSide = Dice.FRand() < 0.5f ? -1.f : 1.f;
		NextIdleAct = Dice.FRandRange(IdleEvery[0], IdleEvery[1]);
	}
}

void ASpiderCreature::ReactBody(float& TargetZ, float& TargetPitch, float& TargetFang) const
{
	const float Notice = NoticePose;
	if (Notice > 0.f)
	{
		// Seen: reared, nose up, fangs spread wide.
		TargetPitch += NoticeRear * Notice;
		TargetZ += NoticeRise * GetSizeScale() * Notice;
		TargetFang = FMath::Max(TargetFang, NoticeFang * Notice);
	}
	if (IdleAct == EIdleAct::Groom && IdleWeight > 0.f)
	{
		// Its front lifted a little to the legs it draws up, its fangs working over them.
		const float Groom = IdleWeight * SpiderEnvelope(IdleActTime, GroomSeconds, 0.35f, 0.4f);
		TargetPitch += 3.f * Groom;
		TargetFang += Groom * (6.f + 6.f * FMath::Sin(2.f * UE_PI * 4.f * IdleActTime));
	}
}

FVector ASpiderCreature::ReactFootOffset(const FLeg& Leg, const FTransform& Body) const
{
	if (Leg.Pair != 0)
	{
		return FVector::ZeroVector;
	}
	FVector Offset = FVector::ZeroVector;
	const float Notice = NoticePose;
	if (Notice > 0.001f)
	{
		// The front legs come up and reach as it rears (the bite's raise, lower).
		const FVector Raised = Body.TransformPosition(FVector(Leg.Hip.X + 55.f, Leg.Hip.Y * 1.4f, 25.f));
		Offset += (Raised - Leg.Foot) * (NoticeLegRaise * Notice);
	}
	if (IdleAct == EIdleAct::None || IdleWeight <= 0.f || Leg.bStepping)
	{
		return Offset;
	}
	const FQuat Turn = Body.GetRotation();
	if (IdleAct == EIdleAct::Tap && Leg.Side == IdleSide)
	{
		// Lifted, two quick taps on the ground, set down again.
		const float Lift = SpiderEnvelope(IdleActTime, TapSeconds, 0.18f, 0.2f);
		const float TapClock = FMath::Clamp((IdleActTime - 0.2f) / 0.5f, 0.f, 1.f);
		const float Dip = 0.5f * (1.f - FMath::Cos(2.f * UE_PI * 2.f * TapClock));
		Offset += (Turn.GetUpVector() * (TapLift * (1.f - 0.75f * Dip)) + Turn.GetForwardVector() * 6.f) * (GetSizeScale() * Lift * IdleWeight);
	}
	else if (IdleAct == EIdleAct::Groom)
	{
		// Both front legs drawn up to the fangs and rubbed over them, one then the other.
		const float Groom = SpiderEnvelope(IdleActTime, GroomSeconds, 0.35f, 0.4f) * IdleWeight;
		const float Rub = 6.f * Leg.Side * FMath::Sin(2.f * UE_PI * GroomRubHz * IdleActTime);
		const FVector AtFangs = Body.TransformPosition(FVector(GroomSpot.X + Rub, GroomSpot.Y * Leg.Side, GroomSpot.Z));
		Offset += (AtFangs - Leg.Foot) * Groom;
	}
	return Offset;
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
