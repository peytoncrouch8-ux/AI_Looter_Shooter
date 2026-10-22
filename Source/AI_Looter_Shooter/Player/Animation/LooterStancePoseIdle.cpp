#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "TwoBoneIK.h"

using namespace LooterStancePoseDetail;

// The idle standing still unarmed. Ellis's is the weary revenant (the user's picks, 2026-10-15: "Weary revenant" with
// breathing, glances around and hand fidgets; the armed stance kept as it was): a dead man who has been walking a long
// time, shoulders a little forward, head a little low, his weight shifting from leg to leg. Since 2026-10-22 every hero
// has their own stance on the same clock (FHeroIdle, the user's picks from CharArt sheet 2): the posture and the hips here,
// the hands' rests and their curl in LooterStancePoseIdleHands.cpp.

namespace
{
	/** A full shift of his weight, one leg to the other and back (s). */
	constexpr float ShiftPeriod = 18.f;
	/** The spells the glances, the finger flexes and the thumb in the belt are drawn in (s). */
	constexpr float GlanceSpell = 10.f;
	constexpr float FlexSpell = 6.f;
	constexpr float HookSpell = 24.f;
	/** A card's walk: a knuckle every KnuckleSeconds, then passed back under the hand in the rest of the cycle. */
	constexpr float KnuckleSeconds = 0.8f;
	constexpr float CardKnuckles = 3.f;

	/** The weight shift: the hips over the standing leg (cm), the free side's hip dropping (degrees), the chest's counter-tilt. */
	constexpr float HipShift = 2.5f;
	constexpr float HipRoll = 3.f;
	constexpr float ChestRoll = 2.f;
	/** The breath in (degrees): the chest up and back, the shoulders lifting. */
	constexpr float BreathChest = 0.8f;
	constexpr float BreathShoulders = 1.3f;
	/** A glance's share in the neck (the head takes the rest); a held turn's too. */
	constexpr float GlanceNeck = 0.4f;
	/** A lean's share down the spine (spine_01, _03, _05), as the crouch's lean. */
	constexpr float LeanShare[] = { 0.45f, 0.35f, 0.2f };
	/** A wide stance's knees kept this little bent short of straight (the leg's reach this share of its length). */
	constexpr float WideReach = 0.995f;

	/** A number 0..1 for spell Index of a kind (Salt), the same every run. */
	float Draw(int32 Index, uint32 Salt)
	{
		uint32 Hash = static_cast<uint32>(Index) * 2654435761u ^ (Salt * 2246822519u + 374761393u);
		Hash ^= Hash >> 15;
		Hash *= 2246822519u;
		Hash ^= Hash >> 13;
		Hash *= 3266489917u;
		Hash ^= Hash >> 16;
		return static_cast<float>(Hash & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
	}

	/** 0, eased up to 1 over In from Start, held Hold, eased back to 0 over Out (all s, on T's clock). */
	float Bump(float T, float Start, float In, float Hold, float Out)
	{
		if (T <= Start || T >= Start + In + Hold + Out)
		{
			return 0.f;
		}
		if (T < Start + In)
		{
			return FMath::SmoothStep(0.f, 1.f, (T - Start) / In);
		}
		if (T <= Start + In + Hold)
		{
			return 1.f;
		}
		return 1.f - FMath::SmoothStep(0.f, 1.f, (T - Start - In - Hold) / Out);
	}

	/** A spell's number and how far into it Time is. */
	int32 SpellOf(float Time, float Spell, float& OutInto)
	{
		const int32 Index = FMath::FloorToInt(Time / Spell);
		OutInto = Time - static_cast<float>(Index) * Spell;
		return Index;
	}

	/** One hand's flex in its spell (Offset staggers the two hands, Salt draws their own). */
	float Flex(float Time, float Offset, uint32 Salt)
	{
		float Into = 0.f;
		const int32 Spell = SpellOf(Time + Offset, FlexSpell, Into);
		return Draw(Spell, Salt) > 0.4f ? Bump(Into, 0.5f + 3.f * Draw(Spell, Salt + 1), 0.4f, 0.5f, 0.6f) : 0.f;
	}

	bool HasRest(const FHeroIdle& Style, EHeroIdleRest Rest)
	{
		return Style.LeftRest == Rest || Style.RightRest == Rest;
	}
}

FLooterIdleStance LooterStancePose::DescribeIdle(float Time)
{
	return DescribeIdle(Time, FHeroIdle());
}

FLooterIdleStance LooterStancePose::DescribeIdle(float Time, const FHeroIdle& Style)
{
	FLooterIdleStance Idle;
	const float T = FMath::Max(Time, 0.f);

	// He leans on one leg a while, then the other: a sine held near its ends.
	const float Sway = FMath::Sin(UE_TWO_PI * T / ShiftPeriod);
	Idle.WeightShift = FMath::Sign(Sway) * FMath::Sqrt(FMath::Abs(Sway));
	const float BreathPeriod = FMath::Max(Style.BreathPeriod, 0.5f);
	Idle.Breath = 0.5f - 0.5f * FMath::Cos(UE_TWO_PI * T / BreathPeriod);
	Idle.Breaths = FMath::FloorToInt(T / BreathPeriod);
	if (Style.SwayDepth > 0.f)
	{
		Idle.Sway = 0.5f - 0.5f * FMath::Cos(UE_TWO_PI * T / FMath::Max(Style.SwayPeriod, 0.5f));
	}

	// Most spells, a look off to one side and back.
	float Into = 0.f;
	const int32 Glance = SpellOf(T, GlanceSpell, Into);
	if (Draw(Glance, 1) > 0.25f)
	{
		const float Look = Bump(Into, 1.5f + 4.f * Draw(Glance, 2), 0.7f, 1.f + 1.2f * Draw(Glance, 3), 0.8f) * FMath::Max(Style.Glances, 0.f);
		const float Side = Draw(Glance, 4) > 0.5f ? 1.f : -1.f;
		Idle.GlanceYaw = Look * Side * (18.f + 17.f * Draw(Glance, 5));
		Idle.GlancePitch = Look * (-3.f + 6.f * Draw(Glance, 6));
	}

	// Some spells, the thumb in the belt a few seconds (a BeltThumb rest only: the others hold their rest).
	const int32 Hook = SpellOf(T, HookSpell, Into);
	if (HasRest(Style, EHeroIdleRest::BeltThumb) && Draw(Hook, 11) > 0.35f)
	{
		Idle.ThumbHook = Bump(Into, 2.f + 6.f * Draw(Hook, 12), 0.9f, 5.f + 4.f * Draw(Hook, 13), 0.9f);
	}

	// The fingers flex now and then, each hand on its own (the right not while it's on the belt).
	Idle.FlexLeft = Flex(T, 0.f, 7);
	Idle.FlexRight = Flex(T, 2.7f, 9) * (1.f - Idle.ThumbHook);

	// A card walks the knuckles index to pinky, turning over at each, and the thumb passes it back under to the index.
	if (HasRest(Style, EHeroIdleRest::CardHand))
	{
		const float InCycle = FMath::Fmod(T, CardCycleSeconds);
		const float Walk = KnuckleSeconds * CardKnuckles;
		if (InCycle < Walk)
		{
			const float Steps = InCycle / KnuckleSeconds;
			const float Step = FMath::FloorToFloat(Steps);
			const float Eased = Step + FMath::SmoothStep(0.f, 1.f, Steps - Step);
			Idle.Card = Eased;
			Idle.CardSpin = UE_PI * Eased;
		}
		else
		{
			const float Back = FMath::SmoothStep(0.f, 1.f, (InCycle - Walk) / FMath::Max(CardCycleSeconds - Walk, 0.01f));
			Idle.Card = CardKnuckles * (1.f - Back);
			Idle.CardSpin = UE_PI * CardKnuckles;
		}
	}
	return Idle;
}

void LooterStancePoseDetail::ApplyIdle(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FLegBones (&Legs)[2], FCompactPoseBoneIndex Pelvis, const FVector& Forward, const FVector& Right)
{
	const float Alpha = FMath::Clamp(Stance.IdleAlpha, 0.f, 1.f);
	if (Alpha <= UE_KINDA_SMALL_NUMBER || !Pelvis.IsValid())
	{
		return;
	}
	const FHeroIdle& Style = Stance.Idle;
	const FLooterIdleStance Idle = LooterStancePose::DescribeIdle(Stance.IdleTime, Style);
	const FVector Up = FVector::UpVector;
	auto Radians = [Alpha](float Degrees) { return FMath::DegreesToRadians(Degrees * Alpha); };
	const FCompactPoseBoneIndex Spine[] = { FindBone(Bones, TEXT("spine_01")), FindBone(Bones, TEXT("spine_03")), FindBone(Bones, TEXT("spine_05")) };
	const FCompactPoseBoneIndex Neck = FindBone(Bones, TEXT("neck_01"));
	const FCompactPoseBoneIndex Head = FindBone(Bones, TEXT("head"));

	// The weight onto one leg (a cocked hip held over it, the slow shift riding on that): the hips over it, the free side's
	// hip dropping; back over the heels, dipping with a piston's sway, settling to reach feet set wide; the legs bent back
	// onto the planted feet (moved out for a wide stance).
	const float Shift = Style.HipCock + Style.WeightShift * Idle.WeightShift;
	const float Wide = FMath::Max(Style.StanceWidth, 0.f) * Alpha;
	FTransform Feet[UE_ARRAY_COUNT(Legs)];
	FTransform Hips = Pose.GetComponentSpaceTransform(Pelvis);
	float WideDrop = 0.f;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legs); ++Index)
	{
		Feet[Index] = Pose.GetComponentSpaceTransform(Legs[Index].Foot);
		if (Wide > UE_KINDA_SMALL_NUMBER)
		{
			// How far the hips must come down for this leg, unstretched, to reach its foot set Wide further out.
			const FVector Thigh = Pose.GetComponentSpaceTransform(Legs[Index].Thigh).GetLocation();
			const FVector Reach = Feet[Index].GetLocation() - Thigh;
			const double Length = Reach.Size() * WideReach;
			const double Across = FMath::Abs(FVector::DotProduct(Reach, Right)) + Wide;
			const double Ahead = FVector::DotProduct(Reach, Forward);
			const double Down = FMath::Sqrt(FMath::Max(Length * Length - Across * Across - Ahead * Ahead, 0.0));
			WideDrop = FMath::Max(WideDrop, static_cast<float>(-Reach.Z - Down));
			Feet[Index].AddToTranslation(Right * (Legs[Index].Side * Wide));
		}
	}
	Hips.AddToTranslation(Right * (Shift * HipShift * Alpha) - Forward * (Style.HipsBack * Alpha)
		- Up * (Style.SwayDepth * Idle.Sway * Alpha + WideDrop));
	// Turning about Forward lowers his left side: weight on the right (+) drops the left hip.
	Hips.SetRotation((FQuat(Forward, Radians(Shift * HipRoll)) * Hips.GetRotation()).GetNormalized());
	SetBone(Pose, Pelvis, Hips);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legs); ++Index)
	{
		const FLegBones& Leg = Legs[Index];
		FTransform Thigh = Pose.GetComponentSpaceTransform(Leg.Thigh);
		FTransform Calf = Pose.GetComponentSpaceTransform(Leg.Calf);
		FTransform Foot = Pose.GetComponentSpaceTransform(Leg.Foot);
		AnimationCore::SolveTwoBoneIK(Thigh, Calf, Foot, Calf.GetLocation() + Forward * 60.f + Right * (Leg.Side * Wide), Feet[Index].GetLocation(),
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Foot.SetRotation(Feet[Index].GetRotation());
		const FBoneTransform Chain[] = { FBoneTransform(Leg.Thigh, Thigh), FBoneTransform(Leg.Calf, Calf), FBoneTransform(Leg.Foot, Foot) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
	}

	// A stoop (+) or a lean back (-) down the whole spine; the head goes with it (the posture below lifts it as it wants).
	if (!FMath::IsNearlyZero(Style.Lean))
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Spine); ++Index)
		{
			RotateBone(Pose, Spine[Index], FQuat(Right, Radians(Style.Lean * LeanShare[Index])));
		}
	}

	// The chest leans back the other way over the hips, forward a little (weary) and up with the breath; turning about
	// Right tips forward.
	const float Breath = Idle.Breath * Style.BreathDepth;
	RotateBone(Pose, Spine[1], FQuat(Forward, Radians(-Shift * ChestRoll)) * FQuat(Right, Radians(-0.6f * BreathChest * Breath)));
	RotateBone(Pose, Spine[2], FQuat(Right, Radians(Style.ChestSlump - BreathChest * Breath)));

	// The shoulders rounded forward and lifting with the breath. About Up, a right shoulder comes forward the negative way and
	// a left one the positive; about Forward, a right one rises the positive way.
	for (const float Side : { -1.f, 1.f })
	{
		const FCompactPoseBoneIndex Clavicle = FindBone(Bones, Side > 0.f ? TEXT("clavicle_r") : TEXT("clavicle_l"));
		RotateBone(Pose, Clavicle, FQuat(Forward, Radians(Side * BreathShoulders * Breath)) * FQuat(Up, Radians(-Side * Style.ShoulderRound)));
	}

	// The head as the stance holds it (a little low; a look off to the horizon) and a glance aside on it (+ yaw to his right:
	// about Up, Forward turns toward Right).
	const float Yaw = Idle.GlanceYaw + Style.HeadYaw;
	const float Pitch = Idle.GlancePitch + Style.HeadPitch;
	RotateBone(Pose, Neck, FQuat(Up, Radians(Yaw * GlanceNeck)) * FQuat(Right, Radians(Style.NeckDown - Pitch * GlanceNeck)));
	RotateBone(Pose, Head, FQuat(Up, Radians(Yaw * (1.f - GlanceNeck))) * FQuat(Right, Radians(-Style.HeadUp - Pitch * (1.f - GlanceNeck))));

	// The hands to their rests (LooterStancePoseIdleHands.cpp); the fingers are curled on the local pose after.
	ApplyIdleRests(Pose, Bones, Stance, Idle, Pelvis, Forward, Right);
}
