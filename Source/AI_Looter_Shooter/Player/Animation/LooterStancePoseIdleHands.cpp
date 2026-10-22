#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "BoneContainer.h"
#include "BonePose.h"

using namespace LooterStancePoseDetail;

// The idle's hands (FHeroIdle's rests, the user's stances of 2026-10-22): each arm's wrist taken to its rest by the arm IK
// the belt hook always used, the hand turned there, and every idle hand's fingers curled at rest (CharArt item 2: no flat,
// splayed fingers), the rest's own hold on top. The hidden mannequin is posed; every hero's hands are pinned to its hands,
// so the hero's FHeroIdle offsets move each rest onto their own body. Numbers are the full-size mannequin's (cm, degrees).

namespace
{
	/** A finger flex at each joint (degrees), on top of the rest. */
	constexpr float FlexDegrees = 22.f;
	/** A card lifts the finger it's crossing this much (degrees, at each joint: the finger straightens under it). */
	constexpr float CardLift = 18.f;

	/** What a rest's wrist is placed from. */
	enum class EAnchor : uint8
	{
		Hips,
		Chest,
		Shoulder
	};

	/**
	 * One rest: the wrist from its anchor and the elbow's way from the shoulder (ahead, out from the body's middle on the
	 * hand's side, up), the fingers' way and the palm's facing (the same frame), and the fingers' hold.
	 */
	struct FRestHold
	{
		EAnchor Anchor = EAnchor::Hips;
		FVector Wrist = FVector::ZeroVector;
		FVector Elbow = FVector::ZeroVector;
		FVector Fingers = FVector(0.f, 0.f, -1.f);
		FVector Palm = FVector(0.f, -1.f, 0.f);
		FHandCurl Curl;
	};

	FHandCurl MakeCurl(float Knuckle, float Middle, float Tip, float Spread, float ThumbIn, float ThumbMiddle, float ThumbTip)
	{
		FHandCurl Curl;
		Curl.Fingers[0] = Knuckle;
		Curl.Fingers[1] = Middle;
		Curl.Fingers[2] = Tip;
		Curl.Spread = Spread;
		Curl.Thumb[0] = ThumbIn;
		Curl.Thumb[1] = ThumbMiddle;
		Curl.Thumb[2] = ThumbTip;
		return Curl;
	}

	FRestHold RestHold(EHeroIdleRest Rest)
	{
		FRestHold Hold;
		switch (Rest)
		{
		case EHeroIdleRest::BeltThumb:
			// Ellis's since 2026-10-15: the wrist at the belt's front by the hip, the elbow out and back, the fingers hanging over
			// the hip with the palm to the body; the thumb straight down behind the belt.
			Hold.Anchor = EAnchor::Hips;
			Hold.Wrist = FVector(12.f, 15.f, 8.f);
			Hold.Elbow = FVector(-10.f, 25.f, -20.f);
			Hold.Fingers = FVector(0.15f, 0.f, -1.f);
			Hold.Palm = FVector(-0.35f, -0.75f, 0.f);
			Hold.Curl = MakeCurl(30.f, 40.f, 25.f, 3.f, 0.f, 0.f, 0.f);
			break;
		case EHeroIdleRest::HipHand:
			// On the side of the hip above the belt, the fingers forward and down over it, the palm on it, the elbow out and back.
			Hold.Anchor = EAnchor::Hips;
			Hold.Wrist = FVector(2.f, 17.f, 13.f);
			Hold.Elbow = FVector(-12.f, 28.f, -16.f);
			Hold.Fingers = FVector(0.6f, 0.1f, -0.75f);
			Hold.Palm = FVector(0.f, -1.f, 0.1f);
			Hold.Curl = MakeCurl(25.f, 30.f, 20.f, 2.f, 10.f, 5.f, 0.f);
			break;
		case EHeroIdleRest::ChainHand:
			// Closed on the chain where it crosses the chest (right shoulder to left hip): the hand at the breastbone, the
			// fingers round it toward the middle and up, the palm to the chest, the elbow down at the side.
			Hold.Anchor = EAnchor::Chest;
			Hold.Wrist = FVector(15.f, 3.f, -9.f);
			Hold.Elbow = FVector(4.f, 14.f, -28.f);
			Hold.Fingers = FVector(0.15f, -0.8f, 0.55f);
			Hold.Palm = FVector(-1.f, 0.f, 0.f);
			Hold.Curl = MakeCurl(60.f, 70.f, 40.f, 3.f, 30.f, 15.f, 10.f);
			break;
		case EHeroIdleRest::StrapThumb:
			// The thumb hooked under the pack's strap at the upper chest, the hand hanging below it over the strap, the palm to
			// the chest, the elbow down and a little out.
			Hold.Anchor = EAnchor::Chest;
			Hold.Wrist = FVector(14.f, 11.f, -14.f);
			Hold.Elbow = FVector(-4.f, 12.f, -28.f);
			Hold.Fingers = FVector(0.1f, -0.15f, -1.f);
			Hold.Palm = FVector(-1.f, -0.1f, 0.f);
			Hold.Curl = MakeCurl(40.f, 50.f, 30.f, 3.f, 0.f, 0.f, 0.f);
			break;
		case EHeroIdleRest::CardHand:
			// Raised before the shoulder, the forearm up, the fingers up and a little in, the back of the hand out toward
			// whoever looks (the card walks over the knuckles), the elbow down at the side.
			Hold.Anchor = EAnchor::Shoulder;
			Hold.Wrist = FVector(22.f, 2.f, -6.f);
			Hold.Elbow = FVector(2.f, 16.f, -30.f);
			Hold.Fingers = FVector(0.25f, -0.3f, 0.92f);
			Hold.Palm = FVector(-0.85f, -0.3f, 0.f);
			Hold.Curl = MakeCurl(40.f, 40.f, 25.f, 2.f, 25.f, 15.f, 10.f);
			break;
		default:
			// Hanging: a loose rest curl, the thumb in (CharArt item 2: ~35/45/30).
			Hold.Curl = MakeCurl(35.f, 45.f, 30.f, 3.f, 20.f, 15.f, 10.f);
			break;
		}
		return Hold;
	}

	/** How strongly a side's rest shows (0-1): a belt thumb only in its spells. */
	float RestShare(EHeroIdleRest Rest, const FLooterIdleStance& Idle)
	{
		return Rest == EHeroIdleRest::BeltThumb ? Idle.ThumbHook : 1.f;
	}

	FHandCurl Blend(const FHandCurl& A, const FHandCurl& B, float Share)
	{
		FHandCurl Out;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Out.Fingers[Index] = FMath::Lerp(A.Fingers[Index], B.Fingers[Index], Share);
			Out.Thumb[Index] = FMath::Lerp(A.Thumb[Index], B.Thumb[Index], Share);
		}
		Out.Spread = FMath::Lerp(A.Spread, B.Spread, Share);
		return Out;
	}
}

void LooterStancePoseDetail::ApplyIdleRests(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FLooterIdleStance& Idle, FCompactPoseBoneIndex Pelvis, const FVector& Forward, const FVector& Right)
{
	const float Alpha = FMath::Clamp(Stance.IdleAlpha, 0.f, 1.f);
	const FVector Up = FVector::UpVector;
	const FCompactPoseBoneIndex Chest = FindBone(Bones, TEXT("spine_05"));
	for (const float Side : { -1.f, 1.f })
	{
		const bool bRight = Side > 0.f;
		const EHeroIdleRest Rest = bRight ? Stance.Idle.RightRest : Stance.Idle.LeftRest;
		const FVector& Offset = bRight ? Stance.Idle.RightRestOffset : Stance.Idle.LeftRestOffset;
		const float Share = Alpha * RestShare(Rest, Idle);
		if (Share <= UE_KINDA_SMALL_NUMBER || (Rest == EHeroIdleRest::Hang && Offset.IsNearlyZero()))
		{
			continue;
		}
		const FCompactPoseBoneIndex UpperArm = FindBone(Bones, bRight ? TEXT("upperarm_r") : TEXT("upperarm_l"));
		const FCompactPoseBoneIndex LowerArm = FindBone(Bones, bRight ? TEXT("lowerarm_r") : TEXT("lowerarm_l"));
		const FCompactPoseBoneIndex Hand = FindBone(Bones, bRight ? TEXT("hand_r") : TEXT("hand_l"));
		if (!UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
		{
			continue;
		}
		// The rest's frame: ahead, out from the body's middle on this hand's side, up.
		auto Place = [&Forward, &Right, &Up, Side](const FVector& Local) { return Forward * Local.X + Right * (Side * Local.Y) + Up * Local.Z; };
		const FVector Shoulder = Pose.GetComponentSpaceTransform(UpperArm).GetLocation();

		if (Rest == EHeroIdleRest::Hang)
		{
			// The hanging hand moved for this body (Gauge's clear of the boiler), the elbow following half as far and out.
			const FVector Wrist = Pose.GetComponentSpaceTransform(Hand).GetLocation() + Place(Offset);
			const FVector Elbow = Pose.GetComponentSpaceTransform(LowerArm).GetLocation() + Place(Offset * 0.5f) + Place(FVector(0.f, 5.f, 0.f));
			SolveArm(Pose, UpperArm, LowerArm, Hand, Wrist, Elbow, Share, /*bTurnHand*/ true);
			continue;
		}

		const FRestHold Hold = RestHold(Rest);
		FVector Anchor = Shoulder;
		if (Hold.Anchor == EAnchor::Hips)
		{
			Anchor = Pose.GetComponentSpaceTransform(Pelvis).GetLocation();
		}
		else if (Hold.Anchor == EAnchor::Chest && Chest.IsValid())
		{
			Anchor = Pose.GetComponentSpaceTransform(Chest).GetLocation();
		}
		const FVector Wrist = Anchor + Place(Hold.Wrist + Offset);
		const FVector Elbow = Shoulder + Place(Hold.Elbow) + Place(Offset);
		SolveArm(Pose, UpperArm, LowerArm, Hand, Wrist, Elbow, Share, /*bTurnHand*/ false);

		FQuat Turn;
		if (HandTurnFor(Bones, bRight, Place(Hold.Fingers), Place(Hold.Palm), Turn))
		{
			FTransform HandNow = Pose.GetComponentSpaceTransform(Hand);
			HandNow.SetRotation(FQuat::Slerp(HandNow.GetRotation(), Turn, Share).GetNormalized());
			SetBone(Pose, Hand, HandNow);
		}
	}
}

void LooterStancePoseDetail::ApplyIdleFingers(FCompactPose& Pose, const FLooterStanceInput& Stance)
{
	const float Alpha = FMath::Clamp(Stance.IdleAlpha, 0.f, 1.f);
	if (Alpha <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FHeroIdle& Style = Stance.Idle;
	const FLooterIdleStance Idle = LooterStancePose::DescribeIdle(Stance.IdleTime, Style);
	const FHandCurl Loose = RestHold(EHeroIdleRest::Hang).Curl;
	for (const bool bRight : { false, true })
	{
		const EHeroIdleRest Rest = bRight ? Style.RightRest : Style.LeftRest;
		// A flex now and then, smaller on a hand holding something (a card hand ripples instead).
		const float Flex = (bRight ? Idle.FlexRight : Idle.FlexLeft) * FlexDegrees
			* (Rest == EHeroIdleRest::Hang || Rest == EHeroIdleRest::BeltThumb ? 1.f : Rest == EHeroIdleRest::CardHand ? 0.f : 0.4f);
		if (Style.RestCurl <= UE_KINDA_SMALL_NUMBER)
		{
			// No rest curl (Looter.Pose.On 0): the animation's open hand flexing, as before 2026-10-22.
			FlexHand(Pose, bRight, Flex * Alpha);
			continue;
		}
		// The rest's hold, from the loose hang as far as the rest shows (a belt thumb in its spells), scaled by the hero's curl.
		FHandCurl Curl = Blend(Loose, RestHold(Rest).Curl, RestShare(Rest, Idle));
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Curl.Fingers[Index] *= Style.RestCurl;
			Curl.Thumb[Index] *= Style.RestCurl;
		}
		if (Rest == EHeroIdleRest::CardHand && Idle.Card >= 0.f)
		{
			// The finger the card is crossing straightens under it.
			for (int32 Finger = 0; Finger < 4; ++Finger)
			{
				Curl.PerFinger[Finger] = -CardLift * FMath::Max(0.f, 1.f - FMath::Abs(Idle.Card - static_cast<float>(Finger)));
			}
		}
		CurlHand(Pose, bRight, Curl, Alpha, Flex);
	}
}
