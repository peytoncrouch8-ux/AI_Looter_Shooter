// ABarnCat's pose, from the model's table (World/AnimalPoseTables.h), only while on screen: what it does blended from
// the table's poses and eased from what it did, with the small things on top: a slow breath, the tail's lazy sway and
// a flick of its tip, an ear's twitch, the tail up in greeting, and its head turned to whoever it watches.

#include "World/BarnCat.h"
#include "World/AnimalPoseTables.h"
#include "Components/PoseableMeshComponent.h"

namespace
{
	/** What it eases from when it changes what it does (s). */
	constexpr float CatChangeSeconds = 0.3f;

	/** A stretch's parts (s): up on its feet first, then into the stretch; out of it at the end. */
	constexpr float StretchUp = 0.6f;
	constexpr float StretchInto = 0.4f;
	constexpr float StretchOut = 0.5f;

	/** The head's widest look (degrees about up and up and down), shared by the neck and the head, and how fast it follows. */
	constexpr float LookMost = 70.f;
	constexpr float LookTilt = 25.f;
	constexpr float NeckShare = 0.45f;
	constexpr float LookRate = 5.f;

	/** The small things: a breath (a second, cm), an ear's twitch (s, degrees), the tail tip's flick (s, degrees). */
	constexpr float BreathRate = 0.3f;
	constexpr float BreathRise = 0.3f;
	constexpr float TwitchSeconds = 0.25f;
	constexpr float TwitchDegrees = 25.f;
	constexpr float FlickSeconds = 0.5f;
	constexpr float FlickDegrees = 28.f;
}

void ABarnCat::UpdateLook(float DeltaSeconds)
{
	const bool bLooks = bWatching && Doing != ECatDoing::Slink && Doing != ECatDoing::Hidden && Doing != ECatDoing::Stretch;
	Watch = DeltaSeconds > 0.f ? FMath::FInterpTo(Watch, bLooks && Doing == ECatDoing::Loaf ? 1.f : 0.f, DeltaSeconds, 3.f) : Watch;
	float YawGoal = 0.f;
	float PitchGoal = 0.f;
	if (bLooks)
	{
		// To their face, from about its head, as far as the neck and head go together.
		const FVector Head = Position + FRotator(0.f, Yaw, 0.f).RotateVector(FVector(14.0, 0.0, 22.0));
		const FVector To = WatchAt - Head;
		const float Off = FMath::FindDeltaAngleDegrees(Yaw, FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(To.Y, To.X))));
		// The loaf_watch pose turns the head some way by itself.
		YawGoal = FMath::Clamp(Off, -LookMost, LookMost) - WatchPoseYaw * Watch;
		PitchGoal = FMath::Clamp(FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(To.Z, To.Size2D()))), -LookTilt, LookTilt);
	}
	if (DeltaSeconds > 0.f)
	{
		HeadYaw = FMath::FInterpTo(HeadYaw, YawGoal, DeltaSeconds, LookRate);
		HeadPitch = FMath::FInterpTo(HeadPitch, PitchGoal, DeltaSeconds, LookRate);
	}
}

void ABarnCat::PoseFigure(float DeltaSeconds)
{
	if (Doing == ECatDoing::Hidden)
	{
		return;
	}
	FAnimalPose Built;
	BuildPose(Built);
	// Changing what it does, it eases from where it was.
	if (PosedDoing != Doing)
	{
		PosedDoing = Doing;
		From = Pose;
		FromBlend = From.IsEmpty() ? 1.f : 0.f;
	}
	if (FromBlend < 1.f)
	{
		FromBlend = FMath::Min(1.f, FromBlend + DeltaSeconds / CatChangeSeconds);
		AnimalPose::Blend(From, Built, AnimalPose::Ease(FromBlend), Built);
	}
	Pose = Built;
	if (!Figure)
	{
		return;
	}
	Figure->SetWorldLocationAndRotation(Position, FRotator(Pitch, Yaw, 0.f));
	if (Rig.IsBound())
	{
		Rig.Apply(*Figure, Built);
	}
}

void ABarnCat::BuildPose(FAnimalPose& Out) const
{
	auto Table = [](ECatPose Which, bool bMirrored = false) -> const FAnimalPose&
	{
		return AnimalPoseTables::CatPose(Which, bMirrored);
	};
	const AnimalPoseTables::FCatBones& Bones = AnimalPoseTables::CatBones();
	const bool bJumping = Pitch != 0.f;
	bool bTailUp = false;
	bool bResting = false;
	switch (Doing)
	{
	case ECatDoing::Sit:
		Out = Table(ECatPose::Sit);
		bResting = true;
		break;
	case ECatDoing::Stretch:
	{
		const float T = Clock;
		if (T < StretchUp)
		{
			Out = Table(ECatPose::Stand);
		}
		else if (T < CatTimes::Stretch - StretchOut)
		{
			AnimalPose::Blend(Table(ECatPose::Stand), Table(ECatPose::Stretch), AnimalPose::Ease((T - StretchUp) / StretchInto), Out);
		}
		else
		{
			AnimalPose::Blend(Table(ECatPose::Stretch), Table(ECatPose::Stand),
				AnimalPose::Ease((T - (CatTimes::Stretch - StretchOut)) / StretchOut), Out);
		}
		break;
	}
	case ECatDoing::Walk:
	case ECatDoing::Emerge:
		AnimalPose::StrideBlend(Table(ECatPose::WalkA), Table(ECatPose::WalkB), Stride, Out);
		if (bJumping)
		{
			// Gathered for the jump: legs drawn in under it.
			AnimalPose::Blend(Out, Table(ECatPose::Stand), 0.5f, Out);
		}
		break;
	case ECatDoing::Greet:
		Out = Table(ECatPose::Stand);
		bTailUp = true;
		break;
	case ECatDoing::Slink:
		if (bRunning)
		{
			// Low and quick: the slink's stride and its mirror, ears flat.
			AnimalPose::StrideBlend(Table(ECatPose::Slink), Table(ECatPose::Slink, true), Stride, Out);
		}
		else
		{
			// Put out, not frightened: a low walk.
			AnimalPose::StrideBlend(Table(ECatPose::WalkA), Table(ECatPose::WalkB), Stride, Out);
			AnimalPose::Blend(Out, Table(ECatPose::Slink), 0.45f, Out);
		}
		break;
	case ECatDoing::Loaf:
	case ECatDoing::Hidden:
	default:
		AnimalPose::Blend(Table(ECatPose::Loaf), Table(ECatPose::LoafWatch), Watch, Out);
		bResting = true;
		break;
	}

	// Resting: a slow breath lifting its chest, the tail's tip swaying lazily.
	if (bResting)
	{
		const float Rise = BreathRise * FMath::Sin(UE_TWO_PI * BreathRate * Breath);
		AnimalPose::Nudge(Out, Bones.Spine2, FVector(0.0, 0.0, Rise));
		AnimalPose::Nudge(Out, Bones.Spine1, FVector(0.0, 0.0, Rise * 0.5f));
		for (int32 Link = 3; Link < Bones.Tail.Num(); ++Link)
		{
			AnimalPose::Bend(Out, Bones.Tail[Link], 0.f, 4.f * FMath::Sin(Breath * 0.7f + Link * 0.6f));
		}
	}
	// Greeting: the tail up, its tip hooked over.
	if (bTailUp && Bones.Tail.Num() >= 4)
	{
		AnimalPose::Bend(Out, Bones.Tail[0], -55.f, 0.f);
		AnimalPose::Bend(Out, Bones.Tail[1], -10.f, 0.f);
		AnimalPose::Bend(Out, Bones.Tail[2], 12.f, 0.f);
		AnimalPose::Bend(Out, Bones.Tail[3], 18.f, 0.f);
	}
	// The tip's flick, now and then.
	if (FlickClock < FlickSeconds && Bones.Tail.Num() >= 6)
	{
		const float Swing = FMath::Sin(UE_PI * FlickClock / FlickSeconds) * FlickSide;
		AnimalPose::Bend(Out, Bones.Tail[4], 0.f, FlickDegrees * 0.6f * Swing);
		AnimalPose::Bend(Out, Bones.Tail[5], 0.f, FlickDegrees * Swing);
	}
	// An ear's twitch.
	if (TwitchClock < TwitchSeconds && Doing != ECatDoing::Slink)
	{
		const float Twitch = FMath::Sin(UE_PI * TwitchClock / TwitchSeconds) * TwitchDegrees;
		AnimalPose::Bend(Out, bTwitchLeft ? Bones.EarL : Bones.EarR, 0.f, 0.f, bTwitchLeft ? -Twitch : Twitch);
	}
	// Its look, carried by the neck and the head.
	AnimalPose::Look(Out, Bones.Neck, HeadYaw * NeckShare, HeadPitch * 0.4f);
	AnimalPose::Look(Out, Bones.Head, HeadYaw * (1.f - NeckShare), HeadPitch * 0.6f);
}
