// The Unpaid's hits as they show on its body (HITS, 2026-10-28; Docs/Work/2026-10-28-Core/HITS.md, "The Unpaid"): where a
// shot lands moves that part (a head knocked back with its jaw dropped and its hat off, an arm flung, a strip jerked, the
// coal swelling and spitting sparks out of its back), every hit whips the head after the hips and twists the spine by how
// far off its middle the wound is, a stagger folds it at the waist, its coal gutters as it weakens, and how it's killed
// shows (snuffed, torn, scattered). FHurtChannels keeps the numbers here (plain: the tests drive it); UnpaidHurtEffects.cpp
// does the world's part on the Unpaid's public face. UnpaidCreatureRig.cpp lays the channels on the bones,
// UnpaidCreatureArms.cpp flings the arms by their shares, UnpaidCreatureMoods.cpp runs the stagger, wounds, death looks and the
// hat (UnpaidHat.cpp). Bosses on its rig (Abel, the Foreman) keep their own: no hat, no stagger pose, no kill styles (their
// deaths are their show). Every new number scales with Looter.Hits.UnpaidScale (0: as before HITS); each layer has its
// Looter.Hits.* switch.

#include "Creatures/UnpaidBodyParts.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitPose.h"
#include "Combat/HitReaction.h"
#include "Elements/LooterElement.h"
#include "Weapons/WeaponTypes.h"
#include "Math/RandomStream.h"

namespace
{
	using UnpaidBody::EKillStyle;

	/** The pelvis's flinch (degrees per unit of flinch): today's, and HITS' bigger one that reads at 25 m. */
	constexpr float OldPelvisGain = 8.f;
	constexpr float NewPelvisGain = 11.f;
	/**
	 * The whiplash: the chest lags the hips' jolt by this share of it, the head by this share for this long (s) before it
	 * lets go over this long and whips on after the body by WhipShare of the hips' peak. The flinch's spring peaks at about
	 * FlinchPeakShare of its kick (UnpaidCreatureArms.cpp: kick 0.6, a crit 1, times FlinchKickGain, at most MaxFlinchKick).
	 */
	constexpr float ChestLag = 0.35f;
	constexpr float HeadLag = 0.5f;
	constexpr float HeadLagSeconds = 0.04f;
	constexpr float HeadRelease = 0.05f;
	constexpr float WhipShare = 0.5f;
	constexpr float FlinchPeakShare = 0.65f;
	constexpr float FlinchKickGain = 1.54f;
	constexpr float MaxFlinchKick = 3.8f;
	/** The twist: this many degrees per unit of flinch per TwistOffset cm the wound is off its middle, at most MaxTwist. */
	constexpr float TwistPerUnit = 10.f;
	constexpr float TwistOffset = 25.f;
	constexpr float MaxTwist = 22.f;

	/** A head hit: the head knocked this far per unit (at most MaxHeadKnock), the neck half of it, the jaw dropped; its hat off from HatOffStrength. */
	constexpr float HeadKnockPerUnit = 18.f;
	constexpr float MaxHeadKnock = 35.f;
	constexpr float NeckShare = 0.5f;
	constexpr float JawDrop = 12.f;
	constexpr float HatOffStrength = 1.2f;
	/** From behind, its head whips round toward the shooter this far (degrees), held this long (s). */
	constexpr float BehindTurn = 25.f;
	constexpr float BehindTurnHold = 0.15f;
	/** An arm hit: that arm flung this many times as hard, the other this share; its fingers fan this much more (degrees per finger) for a moment. */
	constexpr float HitArmShare = 2.f;
	constexpr float OtherArmShare = 0.4f;
	constexpr float SplayPulse = 14.f;
	constexpr float SplayIn = 0.03f;
	constexpr float SplayOut = 0.12f;
	/** A shroud hit, through the cloth: that chain jerked this many times as hard, the body's flinch this share. */
	constexpr float ShroudJerk = 2.f;
	constexpr float ShroudBodyShare = 0.5f;
	/** A coal crit: the coal swells by this share (and spits sparks: UnpaidHurtEffects.cpp). */
	constexpr float CoalCritPulse = 0.4f;
	/** A Big hit: the strips jerked this much harder, and snapped straight (from the front) this long (s). */
	constexpr float BigStripJerk = 2.5f;
	constexpr float BigSnapSeconds = 0.2f;

	/** Its stagger: folded at the waist this far away from the hit (degrees), sunk this far (cm). */
	constexpr float FoldDegrees = 18.f;
	constexpr float StaggerSinkCm = 15.f;

	/** Wounded: the coal's flicker (its heat, plus or minus) and its beat (Hz), a puff of ember smoke this often (s), the gutter's sound every so often (s). */
	constexpr float FlickerHeat = 0.15f;
	constexpr float FlickerHz = 7.f;
	constexpr float PuffSeconds = 0.4f;
	constexpr float GutterEvery[2] = { 2.6f, 3.4f };

	/** Snuffed: the coal swells to SnuffStart and is out within SnuffSeconds; it stands stiff StiffSeconds first. */
	constexpr float SnuffStart = 1.5f;
	constexpr float SnuffSeconds = 0.12f;
	constexpr float SnuffedCoal = 0.02f;
	constexpr float StiffSeconds = 0.1f;
	/** Torn: thrown back this far (degrees) and along the shot this far (cm) over TornSeconds. */
	constexpr float TornTilt = 35.f;
	constexpr float TornShift = 40.f;
	constexpr float TornSeconds = 0.2f;
	/** Scattered: faded out within this long (s). */
	constexpr float ScatterSeconds = 0.3f;

	float Ease(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/** The share HITS' new numbers take of the way from Old (1: all the way to New). */
	float Toward(float Old, float New, float Share)
	{
		return FMath::Max(Old + (New - Old) * Share, 0.f);
	}

	/** Level, unit: a push or a shot's way along the ground (straight back when there's none). */
	FVector LevelWay(const FVector& Way, const FVector& Otherwise = FVector::BackwardVector)
	{
		const FVector Level(Way.X, Way.Y, 0.0);
		return Level.IsNearlyZero() ? Otherwise : Level.GetSafeNormal();
	}
}

// ---------------------------------------------------------------------------
// FHurtChannels: the numbers
// ---------------------------------------------------------------------------

void UnpaidBody::FHurtChannels::ClearShares()
{
	BodyShare = 1.f;
	ArmShare[0] = ArmShare[1] = 1.f;
	ChainJerk[0] = ChainJerk[1] = ChainJerk[2] = 1.f;
	bHatOff = false;
	bCoalSparks = false;
}

void UnpaidBody::FHurtChannels::OnHit(const FCreatureHit& Hit, float Strength, int32 Arm, int32 Chain, const FVector& InAway,
	const FVector& ToShooter, bool bHatOn, bool bBoss)
{
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float Scale = FMath::Max(Feel.UnpaidScale, 0.f);
	ClearShares();
	Away = InAway;
	// Every hit jolts the hips harder than it used to (a boss's too: it's the one flinch it has).
	PelvisGain = Toward(OldPelvisGain, NewPelvisGain, Scale);
	NewShare = bBoss ? 0.f : Scale;
	if (NewShare <= 0.f)
	{
		return;
	}
	const float S = Strength;
	// Along its own forward: below 0 pushed back (a shot from in front), above 0 from behind.
	const float Push = static_cast<float>(Away.X);
	FVector Knock = FVector::ZeroVector;
	float KnockHold = 0.08f;

	switch (Hit.Zone)
	{
	case ECreatureHitZone::Head:
		// Knocked back (from in front; forward from behind), the neck taking half, the jaw dropped; hard enough, its hat off.
		Knock.X = Push * FMath::Min(HeadKnockPerUnit * S, MaxHeadKnock) * NewShare;
		Jaw.Add(FVector(JawDrop * FMath::Min(S, 1.5f) * NewShare, 0.0, 0.0), 0.08f);
		bHatOff = bHatOn && Feel.Hat && S >= HatOffStrength;
		break;
	case ECreatureHitZone::Arm:
		if (Arm == 0 || Arm == 1)
		{
			// That arm flung, the other hardly; its fingers fanned out for a moment.
			ArmShare[Arm] = Toward(1.f, HitArmShare, NewShare);
			ArmShare[1 - Arm] = Toward(1.f, OtherArmShare, NewShare);
			SplayTime[Arm] = 0.f;
		}
		break;
	case ECreatureHitZone::Shroud:
		// Through the cloth: the chain it hit jerks hard, the body hardly flinches.
		ChainJerk[Chain >= 0 && Chain < 3 ? Chain : 0] = Toward(1.f, ShroudJerk, NewShare);
		BodyShare = Toward(1.f, ShroudBodyShare, NewShare);
		break;
	case ECreatureHitZone::Core:
		if (Hit.bCritical)
		{
			// On the coal: it swells and spits sparks out of its back along the shot (UnpaidHurtPose::OnHit throws them).
			Coal.Add(FVector(CoalCritPulse * NewShare, 0.0, 0.0), 0.05f);
			bCoalSparks = true;
		}
		break;
	default:
		break;
	}

	// From behind: it lurches forward (the pelvis's flinch, by its push) and its head whips round toward whoever shot it.
	if (Hit.Side == EHitSide::Back)
	{
		Knock.Z = (ToShooter.Y >= 0.0 ? 1.f : -1.f) * BehindTurn * NewShare;
		KnockHold = BehindTurnHold;
	}
	if (!Knock.IsNearlyZero())
	{
		HeadKnock.Add(Knock, KnockHold);
	}

	// A Big hit: the strips snap out along the shot.
	if (Hit.Weight == EHitWeight::Big)
	{
		ChainJerk[1] *= Toward(1.f, BigStripJerk, NewShare);
		ChainJerk[2] *= Toward(1.f, BigStripJerk, NewShare);
		if (Push < 0.f)
		{
			SnapLeft = BigSnapSeconds;
		}
	}

	// The twist: a wound off its middle turns its spine (the shot's torque on it; LateralOffset's sign is the turn's).
	const float Twisted = FMath::Clamp(TwistPerUnit * S * Hit.LateralOffset / TwistOffset, -MaxTwist, MaxTwist) * NewShare;
	if (FMath::Abs(Twisted) > 0.1f)
	{
		Twist.Add(FVector(0.0, 0.0, Twisted), 0.05f);
	}

	// The whiplash: the head holds back as the hips jolt, then lets go and whips on after them.
	const float Kick = FMath::Min((Hit.bCritical ? 1.f : 0.6f) * FlinchKickGain * S * BodyShare, MaxFlinchKick);
	if (FMath::Abs(Push) > 0.05f)
	{
		HeadHold = 1.f;
		HeadLagLeft = HeadLagSeconds;
		WhipDegrees = WhipShare * PelvisGain * FlinchPeakShare * Kick * Push * NewShare;
	}
}

UnpaidBody::EKillStyle UnpaidBody::FHurtChannels::OnKilled(const FCreatureHit& Hit, const FVector& InAway, bool bBoss)
{
	KillAway = LevelWay(InAway);
	Kill = EKillStyle::Body;
	Stagger = 0.f;
	bStaggered = false;
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	// A boss's death is its fight's show; a status's kill is its element's (CreatureKillLook); the switch off, today's.
	if (bBoss || !Feel.KillStyles || Hit.Element != ELooterElement::None)
	{
		return Kill;
	}
	if (Hit.Weight == EHitWeight::Overkill)
	{
		Kill = EKillStyle::Scattered;
	}
	else if (Hit.bCritical && Hit.Zone == ECreatureHitZone::Core)
	{
		// Precision is paid first: a coal crit's kill snuffs it, even from a shotgun.
		Kill = EKillStyle::Snuffed;
	}
	else if (Hit.Kind == EWeaponKind::Shotgun || Hit.Kind == EWeaponKind::Launcher)
	{
		Kill = EKillStyle::Torn;
	}
	return Kill;
}

void UnpaidBody::FHurtChannels::Step(float Dt)
{
	if (Dt <= 0.f)
	{
		return;
	}
	for (FHitSpring* Spring : { &Twist, &HeadKnock, &Whip, &Jaw, &Coal })
	{
		if (!Spring->IsSettled())
		{
			Spring->Step(Dt);
		}
	}
	if (HeadLagLeft > 0.f)
	{
		HeadLagLeft -= Dt;
		if (HeadLagLeft <= 0.f && FMath::Abs(WhipDegrees) > 0.01f)
		{
			Whip.Add(FVector(WhipDegrees, 0.0, 0.0), 0.02f);
			WhipDegrees = 0.f;
		}
	}
	else
	{
		HeadHold = FMath::Max(HeadHold - Dt / HeadRelease, 0.f);
	}
	for (float& Time : SplayTime)
	{
		if (Time >= 0.f)
		{
			Time += Dt;
			Time = Time > SplayIn + SplayOut ? -1.f : Time;
		}
	}
	SnapLeft = FMath::Max(SnapLeft - Dt, 0.f);
}

int32 UnpaidBody::FHurtChannels::TickStagger(float Alpha)
{
	Stagger = NewShare > 0.f ? FMath::Clamp(Alpha, 0.f, 1.f) : 0.f;
	if (Stagger > 0.f && !bStaggered)
	{
		// Folded away from the hit that broke it.
		bStaggered = true;
		FoldAway = LevelWay(Away);
		return 1;
	}
	if (Stagger <= 0.f && bStaggered)
	{
		bStaggered = false;
		return -1;
	}
	return 0;
}

float UnpaidBody::FHurtChannels::TickWound(float HealthShare, float Dt, float Time, FRandomStream& Dice, bool& bOutPuff, bool& bOutGutter)
{
	bOutPuff = false;
	bOutGutter = false;
	if (!FHitFeelTuning::Live().Wounds || NewShare <= 0.f)
	{
		Wound = 0;
		const float Back = -Flicker;
		Flicker = 0.f;
		return Back;
	}
	Wound = WoundStage(HealthShare);
	// Hurt, its coal flickers (two beats, so it never pulses like a clock), added to its heat as a change so its flare
	// still fades as ever under it.
	const float Noise = Wound >= 1
		? FlickerHeat * NewShare * (0.65f * FMath::Sin(2.f * UE_PI * FlickerHz * Time) + 0.35f * FMath::Sin(2.f * UE_PI * 11.3f * Time + 1.3f))
		: 0.f;
	const float Change = Noise - Flicker;
	Flicker = Noise;
	if (Wound >= 1)
	{
		GutterClock -= Dt;
		if (GutterClock <= 0.f)
		{
			GutterClock = Dice.FRandRange(GutterEvery[0], GutterEvery[1]);
			bOutGutter = true;
		}
	}
	else
	{
		// Its first gutter comes soon after the wound that brings it under half.
		GutterClock = 0.4f;
	}
	if (Wound >= 2)
	{
		PuffClock -= Dt;
		if (PuffClock <= 0.f)
		{
			PuffClock = PuffSeconds;
			bOutPuff = true;
		}
	}
	return Change;
}

float UnpaidBody::FHurtChannels::ChestPitch(float PelvisFlinch) const
{
	return -ChestLag * NewShare * PelvisFlinch;
}

float UnpaidBody::FHurtChannels::NeckPitch() const
{
	return NeckShare * static_cast<float>(HeadKnock.Offset.X);
}

float UnpaidBody::FHurtChannels::HeadPitch(float PelvisFlinch) const
{
	return -HeadLag * NewShare * HeadHold * PelvisFlinch + static_cast<float>(HeadKnock.Offset.X + Whip.Offset.X);
}

float UnpaidBody::FHurtChannels::SplayFor(int32 Side) const
{
	return Side >= 0 && Side < 2 && SplayTime[Side] >= 0.f ? SplayPulse * NewShare * HitEnvelope(SplayTime[Side], SplayIn, SplayOut) : 0.f;
}

float UnpaidBody::FHurtChannels::CoalDeathScale(float DeathTime) const
{
	if (Kill != EKillStyle::Snuffed)
	{
		return 1.f;
	}
	// A last swell, then out (never quite 0: a bone scaled to nothing has no way to point).
	return FMath::Lerp(SnuffStart, SnuffedCoal, FMath::Clamp(DeathTime / SnuffSeconds, 0.f, 1.f));
}

FVector2D UnpaidBody::FHurtChannels::StaggerFold() const
{
	// The rig's Roll tips the top to the left: pushed to its right, it folds over to the right.
	return FVector2D(FoldDegrees * Stagger * FoldAway.X, -FoldDegrees * Stagger * FoldAway.Y);
}

float UnpaidBody::FHurtChannels::StaggerSink() const
{
	return StaggerSinkCm * Stagger;
}

float UnpaidBody::FHurtChannels::BobShare() const
{
	return Wound >= 2 ? 0.5f : 1.f;
}

FVector2D UnpaidBody::FHurtChannels::KillTilt(float DeathTime) const
{
	if (Kill != EKillStyle::Torn)
	{
		return FVector2D::ZeroVector;
	}
	const float Thrown = TornTilt * Ease(DeathTime / TornSeconds);
	return FVector2D(Thrown * KillAway.X, -Thrown * KillAway.Y);
}

FVector UnpaidBody::FHurtChannels::KillShift(float DeathTime) const
{
	return Kill == EKillStyle::Torn ? KillAway * (TornShift * Ease(DeathTime / TornSeconds)) : FVector::ZeroVector;
}

bool UnpaidBody::FHurtChannels::IsStiff(float DeathTime) const
{
	return Kill == EKillStyle::Snuffed && DeathTime < StiffSeconds;
}

float UnpaidBody::FHurtChannels::KillPhase(float DeathTime) const
{
	return Kill == EKillStyle::Scattered ? FMath::Clamp(DeathTime / ScatterSeconds, 0.f, 1.f) : -1.f;
}

void UnpaidBody::FHurtChannels::ArmFlings(float Fling, const float Share[2], float MaxSpeed, float OutFling[2])
{
	for (int32 Side = 0; Side < 2; ++Side)
	{
		OutFling[Side] = FMath::Min(Fling * Share[Side], MaxSpeed);
	}
	// Both capped, the one hit could come out no faster than the other: the other is kept to half of it.
	if (!FMath::IsNearlyEqual(Share[0], Share[1]))
	{
		const int32 Less = Share[0] < Share[1] ? 0 : 1;
		OutFling[Less] = FMath::Min(OutFling[Less], 0.5f * OutFling[1 - Less]);
	}
}

