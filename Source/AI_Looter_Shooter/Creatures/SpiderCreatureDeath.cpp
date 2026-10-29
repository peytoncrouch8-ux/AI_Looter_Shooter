// ASpiderCreature's death (the user's 2026-10-14 ask: "slightly more dramatic, and the legs curl under their dead bodies,
// not to their sides or above them"): a short throe (it rears, fangs spread, front legs carried up, every leg kicking), a
// hard drop with one bounce on its death cry's knock (a thud and a puff of dust), then every leg folded under the body; its
// last twitches (SpiderCreatureReact.cpp), a last shudder and the sink. SpiderCreature.cpp calls it each frame it's posed.
// The killing hit's style (HITS, 2026-10-28; picked in SpiderCreatureHurt.cpp) times it: a head crit's "lights out" drops
// at once (no throe: the nose thrown back, down at 0.12 s, a quick curl), a blast's flails its legs wider in the air and
// curls them as it lands; every style ends with the legs curled under the body.
//
// Why it's built this way (tuned in a model of the rig against Looter.Creatures.Anim.SpiderDeathCurl's limits):
// - The old curl laid each femur out to the side with its knee raised 35 degrees: the knees stood up and out round the
//   body like a cage. A femur is about a metre long, so no knee can be tucked nearer its hip than that; here each femur is
//   swung toward the body's long axis (front legs ahead, back legs behind) and slopes down to its knee on the ground, and
//   the tibia folds back under the body, its tip on its own side of the centreline.
// - A back femur can't lie along the abdomen's underside (its hull goes into it), so the back pairs pass outside the
//   abdomen's flank.
// - The curl turns each bone evenly, the femur round the hip and the tibia round the knee. An IK target dragged round the
//   hip held the front knees up until halfway, then dropped them 15 cm a frame. A foot that would dip under the ground is
//   kept on it by turning the tibia up about the knee: the knee stays on its arc and the tip slides in along the dirt.
// - The body follows curves of the time, not a smoothing, so the drop lands on the cry's knock; the ground stops it over
//   a few frames (a dip of a centimetre or two), since a stop in one frame jolted every knee riding on it.

#include "Creatures/SpiderCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureReactComponent.h"
#include "Components/CapsuleComponent.h"

namespace
{
	// --- The body (seconds of a full-size death; cm at size 1; degrees) ---

	/** It rears to its apex over ApexSeconds, this much higher and more nose up than it stood, its fangs spread this wide. */
	constexpr float ApexSeconds = 0.2f;
	constexpr float ThroeRise = 14.f;
	constexpr float ThroeRear = 25.f;
	constexpr float ThroeFang = 38.f;
	/** Dead: its fangs' spread, its nose's tilt off the ground's slope, and how far it slumps over (to a side by its dice) as it lands. */
	constexpr float DeadFang = 20.f;
	constexpr float DeadPitch = 0.f;
	constexpr float DeadRoll = 2.f;
	constexpr float SlumpFrom = 0.25f;
	constexpr float SlumpSeconds = 0.5f;
	/**
	 * Landing: its fall's speed fades at this time constant as the ground takes it (about 2 cm of dip), its nose stops over
	 * ImpactPitchSeconds a little past its resting pitch and comes back over PitchSettleSeconds, and it bounces once, this
	 * high (its nose this much) over BounceSeconds.
	 */
	constexpr float ImpactSoftness = 0.03f;
	constexpr float ImpactPitchSeconds = 0.08f;
	constexpr float PitchSettleSeconds = 0.25f;
	constexpr float BounceHeight = 6.f;
	constexpr float BounceSeconds = 0.3f;
	constexpr float BounceNose = 1.5f;
	/** The abdomen: lifted this far as it rears (its tail up), slammed down this far by the drop (peaking this long after), wriggling. */
	constexpr float AbdomenRearLift = -12.f;
	constexpr float AbdomenSlam = 9.f;
	constexpr float AbdomenSlamPeak = 0.05f;
	constexpr float AbdomenWriggle = 4.f;
	constexpr float AbdomenWriggleHz = 6.f;
	constexpr float AbdomenWriggleFade = 0.2f;
	/** Its breathing goes out of it over this long. */
	constexpr float BreathOutSeconds = 0.6f;

	// --- The legs (by pair, front 0 to back 3) ---

	/** The pairs from this one back stay planted through the rear (they push it up); those before it ride up with the body. */
	constexpr int32 FirstPlantedPair = 2;
	/** A planted foot caught off the ground (mid-step, raised) is set down on the ground under it over this long. */
	constexpr float SetDownSeconds = 0.15f;
	/** A riding leg's foot is drawn this share of the way, over ThrowSeconds, toward a spot this far ahead of its hip, this many times its hip's way out, this high (the body's frame). */
	constexpr float ThrowSeconds = 0.25f;
	constexpr float ThrowAhead[FirstPlantedPair] = { 60.f, 30.f };
	constexpr float ThrowSpread[FirstPlantedPair] = { 1.6f, 2.2f };
	constexpr float ThrowHeight[FirstPlantedPair] = { 10.f, 0.f };
	constexpr float ThrowShare[FirstPlantedPair] = { 0.2f, 0.3f };
	/** Its kicks: out and up this far (cm), this many a second (by its dice), fading at FlailFade, each leg's starting up to FlailStagger in. */
	constexpr float FlailReach[4] = { 16.f, 22.f, 18.f, 12.f };
	constexpr float FlailHz[2] = { 5.5f, 7.f };
	constexpr float FlailFade = 0.16f;
	constexpr float FlailStagger = 0.06f;
	/**
	 * The curl: the back pair's starts at CurlFirst, each pair ahead of it CurlStagger later, up to CurlJitter more by its
	 * dice; each takes its pair's CurlLength (the front legs travel farthest from where the rear threw them, the back legs'
	 * knees farthest round), quickest a quarter of the way in (CurlPeak), so the legs give way at once and settle slowly.
	 */
	constexpr float CurlFirst = 0.3f;
	constexpr float CurlStagger = 0.05f;
	constexpr float CurlJitter = 0.08f;
	constexpr float CurlLength[4] = { 0.62f, 0.64f, 0.7f, 0.76f };
	constexpr float CurlPeak = 0.25f;

	constexpr bool CurlsEndInTime()
	{
		for (int32 Pair = 0; Pair < 4; ++Pair)
		{
			if (CurlFirst + CurlStagger * (3 - Pair) + CurlJitter + CurlLength[Pair] > ASpiderCreature::CurledSeconds)
			{
				return false;
			}
		}
		return true;
	}
	static_assert(CurlsEndInTime(), "every leg's curl must be done by ASpiderCreature::CurledSeconds");
	static_assert(ApexSeconds < ASpiderCreature::DropSeconds && CurlFirst < ASpiderCreature::DropSeconds,
		"the legs start giving way before the body lands");

	/**
	 * The dead pose (the body's frame at size 1, the thorax at DeadRide): each femur leaves its hip this many degrees from
	 * straight ahead (to its own side) and slopes down to its knee this high over the ground; its tip lies this far to its own
	 * side of the centreline, this high. The back pairs swing out past the abdomen's flank: along it, a femur's hull went
	 * into the abdomen's. The back pair was 146 degrees and 11 cm until the 2026-10-22 remodel (Spider.py: arched femurs, a
	 * longer coxa, spines and bristles standing up off them): swung 4 degrees wider and 2 cm lower, the arched femur's top
	 * keeps farther off the abdomen's flank, its knee still drawn in (about 77 of the test's 80 cm off the body's line). (The
	 * 4-5 cm Looter.Creatures.Anim.SpiderDeathCurl measured at either was the remodel's coxa rim, 8 cm behind the hip and
	 * buried in the carapace, turning into the abdomen's front where nothing shows; a yaw or height only traded it against
	 * the flank. Since 2026-10-29 the test leaves out what the thorax hides.)
	 */
	constexpr float DeadKneeYaw[4] = { 8.f, 22.f, 156.f, 142.f };
	constexpr float DeadKneeHeight[4] = { 12.f, 12.f, 12.f, 9.f };
	constexpr float DeadTipSide[4] = { 6.f, 18.f, 8.f, 30.f };
	constexpr float DeadTipHeight[4] = { 5.f, 3.f, 7.f, 3.f };
	/**
	 * The ground under a foot (cm): mid-curl its corner is rounded this widely (none at either end, so the curl ends exactly
	 * in the dead pose); through the throe it's soft this widely, eased in over FloorSoftIn, so a foot the falling body
	 * drives into it slows over a few frames rather than stopping in one.
	 */
	constexpr float FloorRounding = 20.f;
	constexpr float FloorSoft = 12.f;
	constexpr float FloorSoftIn = 0.12f;

	// --- Its last shudder, its drop's dust ---

	/** The shudder: its body rolls, tips and lifts this much (degrees, cm), its abdomen this much, its feet this much, quickly. */
	constexpr float ShudderRoll = 2.5f;
	constexpr float ShudderNose = 1.5f;
	constexpr float ShudderLift = 0.8f;
	constexpr float ShudderAbdomen = 3.f;
	constexpr float ShudderLeg = 3.f;
	constexpr float ShudderHz[5] = { 23.f, 19.f, 29.f, 27.f, 26.f };  // roll, nose, lift, abdomen, legs
	/** Where dust puffs out from under the body as it lands (the body's frame at size 1, each mirrored): under the thorax, the abdomen's middle and its rear. */
	constexpr float DustSpots[3][2] = { { 25.f, 32.f }, { -60.f, 40.f }, { -110.f, 22.f } };
	/** How hard the puffs are thrown out from under it (cm/s: KickFootfall carries a tenth of it on) and how big (times its size). */
	constexpr float DustThrow = 1200.f;
	constexpr float DustSize = 1.3f;

	float Smooth01(float X)
	{
		return FMath::SmoothStep(0.f, 1.f, X);
	}

	/** From rest to rest, its speed rising evenly to its peak at CurlPeak of the way, then falling evenly. */
	float CurlEase(float X)
	{
		X = FMath::Clamp(X, 0.f, 1.f);
		return X < CurlPeak ? X * X / CurlPeak : 1.f - FMath::Square(1.f - X) / (1.f - CurlPeak);
	}

	/** From turned Alpha of the way toward To (both unit length), the shortest way round. */
	FVector TurnWay(const FVector& From, const FVector& To, float Alpha)
	{
		return FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(From, To), Alpha).RotateVector(From);
	}

	// --- Kill styles (HITS, 2026-10-28; SpiderCreatureHurt.cpp picks one from the killing hit) ---

	/**
	 * A style's death against the body kill's (a full-size death's seconds): its apex and drop, how high and how far nose up
	 * it rears, its curl's share of the time and how much later it starts, its kicks' reach and its riding legs' throw.
	 */
	struct FDeathTimes
	{
		float Apex = ApexSeconds;
		float Drop = ASpiderCreature::DropSeconds;
		float Rise = ThroeRise;
		float Rear = ThroeRear;
		float CurlShare = 1.f;
		float CurlDelay = 0.f;
		float Flail = 1.f;
		float Throw = 1.f;
	};

	/** Lights out (a head crit): no throe, its nose thrown back by the shot as it drops at once, its legs curling in quick. */
	constexpr float LightsOutApex = 0.05f;
	constexpr float LightsOutDrop = 0.12f;
	constexpr float LightsOutSnap = 25.f;
	constexpr float LightsOutCurl = 0.6f;
	/** A blast: its legs flail wider in the air and curl under it as it lands, a little later than a body kill's. */
	constexpr float BlastFlail = 1.5f;
	constexpr float BlastCurlDelay = 0.12f;
	static_assert(CurlFirst + CurlStagger * 3 + CurlJitter + BlastCurlDelay + CurlLength[3] < 1.5f,
		"a blast's curl is done before its first twitch");

	FDeathTimes TimesFor(ASpiderCreature::EKillStyle Style)
	{
		FDeathTimes Times;
		if (Style == ASpiderCreature::EKillStyle::LightsOut)
		{
			Times.Apex = LightsOutApex;
			Times.Drop = LightsOutDrop;
			Times.Rise = 0.f;
			Times.Rear = LightsOutSnap;
			Times.CurlShare = LightsOutCurl;
			Times.Flail = 0.f;
			Times.Throw = 0.f;
		}
		else if (Style == ASpiderCreature::EKillStyle::Blast || Style == ASpiderCreature::EKillStyle::Overkill)
		{
			Times.CurlDelay = BlastCurlDelay;
			Times.Flail = BlastFlail;
		}
		return Times;
	}
}

float ASpiderCreature::GetDeathDropTime() const
{
	return TimesFor(KillStyle).Drop * DeathTimeScale(GetSizeScale());
}

// ---------------------------------------------------------------------------
// As it dies
// ---------------------------------------------------------------------------

void ASpiderCreature::RecordDeathPose()
{
	const float Scale = GetSizeScale();
	const float GroundZ = static_cast<float>(GetActorLocation().Z) - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	// The body as it was: a climb out of the dirt cut short folds into its height, a bite's lunge eases off over the rear,
	// a stagger's crouch (laid on after the body's smoothing: SpiderCreatureHurt.cpp) folds into its height and pitch.
	const FHitFeelPose Feel = HitFeelPose();
	const float StaggerDrop = Feel.Drop - static_cast<float>(HitTwist.Offset.Z);
	DeathHeight = (BodyZ + EmergeDrop() - StaggerDrop - GroundZ) / Scale;
	DeathPitch = BodyPitch + Feel.Pitch;
	const FDeathTimes Times = TimesFor(KillStyle);
	DeathRoll = BodyRoll;
	DeathLunge = LungeNow;
	UCreatureReactComponent* Reacts = GetReact();
	FRandomStream* Dice = Reacts ? &Reacts->GetRandom() : nullptr;
	auto Roll = [Dice](float Min, float Max) { return Dice ? Dice->FRandRange(Min, Max) : 0.5f * (Min + Max); };
	DeadRollSide = Roll(-1.f, 1.f) < 0.f ? -1.f : 1.f;
	for (FLeg& Leg : Legs)
	{
		// Where the solve last put it (a foot past its reach was drawn short of Leg.Foot), or its spot if it was never posed;
		// the ground under it (a foot mid-step or raised is over it); and where it is against the body.
		Leg.DeathFoot = Leg.PosedFoot.IsNearlyZero() ? Leg.Foot : Leg.PosedFoot;
		Leg.DeathGround = GroundUnder(Leg.DeathFoot);
		Leg.DeathLocal = BodyFrame.InverseTransformPosition(Leg.DeathFoot);
		Leg.HeldVelocity = FVector::ZeroVector;
		DeadPose(Leg, Leg.DeadKnee, Leg.DeadFoot);
		const int32 Pair = FMath::Clamp(Leg.Pair, 0, 3);
		Leg.CurlStart = (CurlFirst + CurlStagger * (3 - Pair) + Roll(0.f, CurlJitter)) * Times.CurlShare + Times.CurlDelay;
		Leg.FlailStart = Roll(0.f, FlailStagger);
		Leg.FlailHz = Roll(FlailHz[0], FlailHz[1]);
	}
}

void ASpiderCreature::DeadPose(const FLeg& Leg, FVector& OutKnee, FVector& OutFoot)
{
	const int32 Pair = FMath::Clamp(Leg.Pair, 0, 3);
	const float Side = Leg.Hip.Y < 0.0 ? -1.f : 1.f;
	// The femur, swung toward the body's long axis and sloping down from the hip to its knee on the ground.
	const float Yaw = FMath::DegreesToRadians(DeadKneeYaw[Pair]);
	const FVector Flat(FMath::Cos(Yaw), Side * FMath::Sin(Yaw), 0.f);
	const float Upper = FMath::Max(Leg.FemurLength, 1.f);
	const float Drop = FMath::Clamp(static_cast<float>(Leg.Hip.Z - (DeadKneeHeight[Pair] - DeadRide)) / Upper, -1.f, 1.f);
	OutKnee = Leg.Hip + (Flat * FMath::Sqrt(1.f - Drop * Drop) - FVector::UpVector * Drop) * Upper;
	// The tibia, folded back from the knee under the body to its tip's place on its side of the centreline: how far back
	// it reaches is its length's to say.
	const double Lower = FMath::Max(Leg.TibiaLength, 1.f);
	const double Across = Side * DeadTipSide[Pair] - OutKnee.Y;
	const double Down = (DeadTipHeight[Pair] - DeadRide) - OutKnee.Z;
	const double Along = Lower * Lower - Across * Across - Down * Down;
	OutFoot = Along > 0.0
		? OutKnee + FVector((Flat.X > 0.0 ? -1.0 : 1.0) * FMath::Sqrt(Along), Across, Down)
		: OutKnee + FVector(0.0, Across, Down).GetSafeNormal() * Lower;
}

// ---------------------------------------------------------------------------
// The body
// ---------------------------------------------------------------------------

void ASpiderCreature::DeadBody(float Time, float GroundZ, float& OutZ, float& OutPitch, float& OutRoll, float& OutFang) const
{
	const float Scale = GetSizeScale();
	const float U = Time / DeathTimeScale(Scale);
	// Its kill style's timing (a body kill's is the constants above; lights out drops at once, its nose thrown back).
	const FDeathTimes Times = TimesFor(KillStyle);
	const float Span = Times.Drop - Times.Apex;
	// The throe: from as it stood it rears to its apex (from rest, to rest), then falls ever faster, height and nose
	// together, to its dead ride and pitch at its drop. (Killed mid-bite, already reared, it rears no further.)
	const float Apex = FMath::Max(DeathHeight, RideHeight + Times.Rise);
	const float ApexPitch = FMath::Max(DeathPitch, GroundPitch + Times.Rear);
	const float RestPitch = GroundPitch + DeadPitch;
	float Height = DeadRide;
	float Pitch = RestPitch;
	if (U < Times.Apex)
	{
		const float Rise = Smooth01(U / Times.Apex);
		Height = FMath::Lerp(DeathHeight, Apex, Rise);
		Pitch = FMath::Lerp(DeathPitch, ApexPitch, Rise);
	}
	else if (U < Times.Drop)
	{
		const float Fallen = FMath::Square((U - Times.Apex) / Span);
		Height = Apex - (Apex - DeadRide) * Fallen;
		Pitch = ApexPitch - (ApexPitch - RestPitch) * Fallen;
	}
	else
	{
		// Landed still falling: the ground takes its speed over a few frames, its nose stops a little past its rest and comes
		// back, and it bounces once.
		const float After = U - Times.Drop;
		const float LandSpeed = -2.f * (Apex - DeadRide) / Span;
		Height = DeadRide + LandSpeed * After * FMath::Exp(-After / ImpactSoftness);
		const float Spin = -2.f * (ApexPitch - RestPitch) / Span;
		const float Past = 0.5f * Spin * ImpactPitchSeconds;
		Pitch = RestPitch + (After < ImpactPitchSeconds
			? Spin * (0.5f * After + ImpactPitchSeconds / (2.f * UE_PI) * FMath::Sin(UE_PI * After / ImpactPitchSeconds)) : Past);
		Pitch -= Past * Smooth01((After - ImpactPitchSeconds) / PitchSettleSeconds);
		if (After < BounceSeconds)
		{
			const float Bounce = FMath::Square(FMath::Sin(UE_PI * After / BounceSeconds));
			Height += BounceHeight * Bounce;
			Pitch += BounceNose * Bounce;
		}
	}
	// It keeps the ground's slope, slumped a little to one side as it lands.
	float Roll = FMath::Lerp(DeathRoll, GroundRoll + DeadRoll * DeadRollSide, Smooth01((U - SlumpFrom) / SlumpSeconds));
	// Its last shudder, just before it sinks.
	const float Shudder = DeathShudder(Time);
	if (Shudder > 0.f)
	{
		const float Into = Time - (CorpseTime - ShudderSeconds);
		Roll += ShudderRoll * Shudder * FMath::Sin(2.f * UE_PI * ShudderHz[0] * Into);
		Pitch += ShudderNose * Shudder * FMath::Sin(2.f * UE_PI * ShudderHz[1] * Into + 1.f);
		Height += ShudderLift * Shudder * FMath::Sin(2.f * UE_PI * ShudderHz[2] * Into);
	}
	OutZ = GroundZ + Height * Scale;
	// Then it sinks out of sight (a big body as much faster as it is bigger, so it's gone by the time it's hidden).
	if (Time > CorpseTime)
	{
		OutZ -= (Time - CorpseTime) * 45.f * Scale;
	}
	OutPitch = Pitch;
	OutRoll = Roll;
	OutFang = U < Times.Drop ? ThroeFang : DeadFang;
}

float ASpiderCreature::DeadAbdomenTilt(float Time) const
{
	const float U = Time / DeathTimeScale(GetSizeScale());
	const FDeathTimes Times = TimesFor(KillStyle);
	// Its tail up as it rears (the body's own pitch would swing it down into the ground), slammed down as it lands,
	// wriggling through the throe, and a tremor in the last shudder.
	const float Rear = Smooth01(U / Times.Apex) * (1.f - Smooth01((U - Times.Apex) / (Times.Drop - Times.Apex)));
	const float After = U - Times.Drop;
	const float Slam = After > 0.f ? After / AbdomenSlamPeak * FMath::Exp(1.f - After / AbdomenSlamPeak) : 0.f;
	const float Wriggle = AbdomenWriggle * FMath::Sin(2.f * UE_PI * AbdomenWriggleHz * U) * FMath::Exp(-U / AbdomenWriggleFade)
		* Smooth01(U / 0.05f);
	const float Shudder = ShudderAbdomen * DeathShudder(Time) * FMath::Sin(2.f * UE_PI * ShudderHz[3] * (Time - (CorpseTime - ShudderSeconds)));
	return AbdomenRearLift * Rear + AbdomenSlam * Slam + Wriggle + Shudder;
}

float ASpiderCreature::DeathBreath(float Time) const
{
	return 1.f - Smooth01(Time / DeathTimeScale(GetSizeScale()) / BreathOutSeconds);
}

float ASpiderCreature::DeathShudder(float Time) const
{
	const float Into = Time - (CorpseTime - ShudderSeconds);
	return Into > 0.f && Into < ShudderSeconds ? FMath::Sin(UE_PI * Into / ShudderSeconds) : 0.f;
}

// ---------------------------------------------------------------------------
// The legs
// ---------------------------------------------------------------------------

void ASpiderCreature::PoseDeadLeg(const FLeg& Leg, int32 Index, float Time, const FVector& Hip, const FVector& Outward, const FVector& Up,
	const FVector& Pole, FVector& OutKnee, FVector& OutFoot) const
{
	const float Scale = GetSizeScale();
	const float U = Time / DeathTimeScale(Scale);
	const float Upper = Leg.FemurLength * Scale;
	const float Lower = Leg.TibiaLength * Scale;
	const int32 Pair = FMath::Clamp(Leg.Pair, 0, 3);
	const bool bPlanted = Pair >= FirstPlantedPair;
	// Its kill style's kicks, throw and curl (a body kill's: all as the constants say).
	const FDeathTimes Times = TimesFor(KillStyle);

	// Where its foot would be without its curl: a back foot planted where it stood (set down if it was off the ground), a
	// front or middle one riding up with the body as it rears, drawn up and in a little; each kicking out and up, from rest,
	// fading fast.
	FVector From;
	if (bPlanted)
	{
		From = FMath::Lerp(Leg.DeathFoot, Leg.DeathGround, Smooth01(U / SetDownSeconds));
	}
	else
	{
		const FVector Spot(Leg.Hip.X + ThrowAhead[Pair], Leg.Hip.Y * ThrowSpread[Pair], ThrowHeight[Pair]);
		From = BodyFrame.TransformPosition(FMath::Lerp(Leg.DeathLocal, Spot, ThrowShare[Pair] * Times.Throw * Smooth01(U / ThrowSeconds)));
	}
	const float Kicking = U - Leg.FlailStart;
	if (Kicking > 0.f)
	{
		const FVector KickWay = ((bPlanted ? FVector::UpVector : Up) + Outward * 0.6f).GetSafeNormal();
		const float Kick = 0.5f * (1.f - FMath::Cos(2.f * UE_PI * Leg.FlailHz * Kicking)) * FMath::Exp(-Kicking / FlailFade);
		From += KickWay * (FlailReach[Pair] * Times.Flail * Scale * Kick);
	}
	// That foot as the walking bend holds it: where the curl starts from.
	FVector KneeFrom;
	FVector FootFrom;
	SolveTwoBone(Hip, From, Upper, Lower, Pole, KneeFrom, FootFrom);

	// The curl: the femur turns round the hip and the tibia round the knee, each evenly, to their dead ways.
	const float Alpha = CurlEase((U - Leg.CurlStart) / (CurlLength[Pair] * Times.CurlShare));
	const FVector DeadKneeWay = BodyFrame.TransformVectorNoScale((Leg.DeadKnee - Leg.Hip).GetSafeNormal());
	const FVector DeadTibiaWay = BodyFrame.TransformVectorNoScale((Leg.DeadFoot - Leg.DeadKnee).GetSafeNormal());
	const FVector KneeWay = TurnWay((KneeFrom - Hip).GetSafeNormal(UE_SMALL_NUMBER, DeadKneeWay), DeadKneeWay, Alpha);
	const FVector TibiaWay = TurnWay((FootFrom - KneeFrom).GetSafeNormal(UE_SMALL_NUMBER, DeadTibiaWay), DeadTibiaWay, Alpha);
	const FVector Knee = Hip + KneeWay * Upper;
	FVector TipTarget = Knee + TibiaWay * Lower;

	// The ground, from under where it stood to under where it lies (its tip's height under its dead place, so a body on a
	// slope lays its feet on the slope). A foot that would go under it is kept on it, the tibia turned up about the knee.
	const double FloorEnd = BodyFrame.TransformPosition(Leg.DeadFoot).Z - DeadTipHeight[Pair] * Scale;
	const double Floor = FMath::Lerp(static_cast<double>(Leg.DeathGround.Z), FloorEnd, static_cast<double>(Alpha));
	const double Under = Floor - TipTarget.Z;
	const double Round = FMath::Max(FloorRounding * FMath::Sin(UE_PI * Alpha), FloorSoft * Smooth01(U / FloorSoftIn) * (1.f - Alpha)) * Scale;
	const double Lift = 0.5 * (Under + FMath::Sqrt(Under * Under + Round * Round));
	if (Lift > 0.01)
	{
		const double Reach = Lower;
		const double Drop = FMath::Clamp(Knee.Z - (TipTarget.Z + Lift), -Reach, Reach);
		FVector Heading(TibiaWay.X, TibiaWay.Y, 0.0);
		if (!Heading.Normalize())
		{
			Heading = FVector(DeadTibiaWay.X, DeadTibiaWay.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, Outward);
		}
		TipTarget = Knee + Heading * FMath::Sqrt(FMath::Max(Reach * Reach - Drop * Drop, 0.0)) - FVector::UpVector * Drop;
	}

	// A last twitch or two once curled (SpiderCreatureReact.cpp), and the last shudder: they move the foot's target only.
	TipTarget += DeathTwitch(Index, Time, Outward, Up);
	const float Shudder = DeathShudder(Time);
	if (Shudder > 0.f)
	{
		const float Into = Time - (CorpseTime - ShudderSeconds);
		TipTarget += Up * (ShudderLeg * Scale * Shudder * FMath::Sin(2.f * UE_PI * ShudderHz[4] * Into + 1.7f * Index));
	}
	// The solve keeps the bones their length and puts the knee back on its arc: bent toward it, square to the hip-to-foot
	// line (never near zero: the knee is always well off that line, folded).
	const FVector Line = (TipTarget - Hip).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
	SolveTwoBone(Hip, TipTarget, Upper, Lower, KneeWay - Line * FVector::DotProduct(KneeWay, Line), OutKnee, OutFoot);
}

// ---------------------------------------------------------------------------
// The drop
// ---------------------------------------------------------------------------

void ASpiderCreature::LandDeadBody()
{
	// Its body's thud into the dirt (the shell's clack in it), on its death cry's knock.
	Voice->Play(FName(LooterSoundCue::React::SpiderThud));
	UCreatureReactComponent* Reacts = GetReact();
	if (!Reacts)
	{
		return;
	}
	// A puff from under each side of the thorax and the abdomen, thrown out from under the body (the level's dust allowance,
	// only near a player, tinted by the ground: KickFootfall's rules).
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	for (const auto& Spot : DustSpots)
	{
		for (const float Side : { -1.f, 1.f })
		{
			const FVector Under(Spot[0], Spot[1] * Side, 0.f);
			const FVector Out = Yaw.RotateVector(FVector(Spot[0] * 0.3f, Spot[1] * Side, 0.f).GetSafeNormal());
			Reacts->KickFootfall(Feet + Yaw.RotateVector(Under * Scale), Out * (DustThrow * FMath::Sqrt(Scale)), DustSize * Scale);
		}
	}
}
