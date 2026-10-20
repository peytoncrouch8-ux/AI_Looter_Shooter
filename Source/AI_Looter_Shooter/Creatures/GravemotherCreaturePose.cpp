// AGravemotherCreature's poses for her own moves, laid over the spider's (ASpiderCreature::GetPoseOverride; the Barrow sheet's
// frames): for the Grave Clod her front legs dig into the floor, come up together with the clod between them, hold it over
// her head and fling it; for the Furrow her head goes low and her front legs into the floor through the heave, and stay
// ploughing down the line. And the stars wheeling over her head while a clod's burst has her stunned.

#include "Creatures/GravemotherCreature.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

namespace
{
	// The clod's tell (body frame at size 1 for raised feet; the actor's frame for feet on the ground).
	/** Digging in: head low, the front feet driven in ahead of her. */
	constexpr float DigPitch = -6.f;
	constexpr float DigRise = -8.f;
	const FVector2D DigGround(150.0, 50.0);
	constexpr float DigDepth = 20.f;
	/** Lifted: both front feet over her head, close together, the clod between them; she rears to hold it. */
	const FVector LiftedAir(60.0, 22.0, 110.0);
	const FVector2D LiftedGround(150.0, 26.0);
	constexpr float LiftPitch = 18.f;
	constexpr float LiftRise = 14.f;
	/** Drawn back before the throw, then flung forward. */
	const FVector DrawnAir(35.0, 22.0, 125.0);
	constexpr float DrawnPitch = 22.f;
	const FVector FlungAir(150.0, 30.0, 70.0);
	constexpr float FlingSeconds = 0.15f;

	// The Furrow.
	/** The heave: head low, front legs in the floor; a shudder once the ground starts to split. */
	constexpr float HeavePitch = -10.f;
	constexpr float HeaveRise = -12.f;
	const FVector2D HeaveGround(170.0, 45.0);
	constexpr float HeaveDig = 20.f;
	constexpr float HeaveShudder = 1.5f;
	constexpr float HeaveShudderHz = 9.f;
	/** The plough: lower still, the front legs ploughing ahead of her. */
	constexpr float PloughPitch = -12.f;
	constexpr float PloughRise = -10.f;
	const FVector2D PloughGround(185.0, 40.0);
	constexpr float PloughDig = 25.f;

	/** The stun's stars (the sheet's "Stun cue"): four of 37 cm on a ring 170 cm across over her head, wheeling. */
	constexpr int32 StarCount = 4;
	constexpr float StarSize = 37.f;
	constexpr float StarRing = 85.f;
	constexpr float StarTurnsPerSecond = 0.9f;
	constexpr float StarGlow = 24.f;
	const FLinearColor StarColor(1.f, 0.86f, 0.45f);
	/** Their trails: this many short dim pieces of arc behind each, this long (radians). */
	constexpr int32 TrailPieces = 3;
	constexpr float TrailArc = 0.5f;

	float Ease(float From, float To, float Share)
	{
		return FMath::Lerp(From, To, FMath::SmoothStep(0.f, 1.f, Share));
	}
}

bool AGravemotherCreature::GetPoseOverride(FSpiderPoseOverride& Out) const
{
	if (IsDead() || bReeling || GetCreatureState() != ECreatureState::Attack)
	{
		return false;
	}
	const float Time = GetStateTime();
	if (Move == EGravemotherMove::Clod && !bClodCountered)
	{
		const float Tell = Clods.Tell;
		if (Time > Tell + FlingSeconds)
		{
			// Thrown: the spider's own recovery brings her down.
			return false;
		}
		Out.OwnShare = 0.f;
		Out.bFrontFeet = true;
		Out.FrontOmega = 20.f;
		Out.Fang = 18.f;
		if (Time < Clods.TearAt)
		{
			// Digging in.
			const float In = FMath::Clamp(Time / 0.2f, 0.f, 1.f);
			Out.Pitch = DigPitch * In;
			Out.Rise = DigRise * In;
			Out.FrontGround = DigGround;
			Out.GroundShare = 1.f;
			Out.DigDepth = DigDepth * In;
			Out.FrontAir = LiftedAir;
		}
		else if (Time < Clods.LiftedAt)
		{
			// Lifting it out of the ground and up over her head.
			const float Up = FMath::Clamp((Time - Clods.TearAt) / FMath::Max(Clods.LiftedAt - Clods.TearAt, 0.05f), 0.f, 1.f);
			Out.Pitch = Ease(DigPitch, LiftPitch, Up);
			Out.Rise = Ease(DigRise, LiftRise, Up);
			Out.FrontGround = FMath::Lerp(DigGround, LiftedGround, Up);
			Out.GroundShare = 1.f - FMath::SmoothStep(0.f, 1.f, Up);
			Out.DigDepth = DigDepth * (1.f - Up);
			Out.FrontAir = LiftedAir;
		}
		else if (Time < Tell)
		{
			// Held overhead, drawn back for the throw.
			const float Back = FMath::Clamp((Time - Clods.LiftedAt) / FMath::Max(Tell - Clods.LiftedAt, 0.05f), 0.f, 1.f);
			Out.Pitch = Ease(LiftPitch, DrawnPitch, Back);
			Out.Rise = LiftRise;
			Out.FrontAir = FMath::Lerp(LiftedAir, DrawnAir, FMath::SmoothStep(0.f, 1.f, Back));
		}
		else
		{
			// Flung forward.
			const float Fling = FMath::Clamp((Time - Tell) / FlingSeconds, 0.f, 1.f);
			Out.Pitch = Ease(DrawnPitch, -4.f, Fling);
			Out.Rise = Ease(LiftRise, 0.f, Fling);
			Out.Lunge = 20.f * FMath::Sin(Fling * UE_PI);
			Out.FrontAir = FMath::Lerp(DrawnAir, FlungAir, Fling);
			Out.FrontOmega = 35.f;
		}
		return true;
	}
	if (Move == EGravemotherMove::Furrow && FurrowState == EGravemotherFurrow::Heave)
	{
		const float In = FMath::Clamp(Time / 0.25f, 0.f, 1.f);
		const float Shudder = bAimTaken ? HeaveShudder * FMath::Sin(Time * 2.f * UE_PI * HeaveShudderHz) : 0.f;
		Out.OwnShare = 0.f;
		Out.Pitch = HeavePitch * In + Shudder;
		Out.Rise = HeaveRise * In;
		Out.Fang = 22.f;
		Out.bFrontFeet = true;
		Out.FrontGround = HeaveGround;
		Out.GroundShare = 1.f;
		Out.DigDepth = HeaveDig * In;
		Out.FrontOmega = 18.f;
		return true;
	}
	if (Move == EGravemotherMove::Furrow && FurrowState == EGravemotherFurrow::Plough)
	{
		Out.OwnShare = 0.f;
		Out.Pitch = PloughPitch;
		Out.Rise = PloughRise;
		Out.Fang = 30.f;
		Out.bFrontFeet = true;
		Out.FrontGround = PloughGround;
		Out.GroundShare = 1.f;
		Out.DigDepth = PloughDig;
		Out.FrontOmega = 30.f;
		return true;
	}
	return false;
}

void AGravemotherCreature::DrawStunStars()
{
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World && World->IsGameWorld() ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets || IsDead())
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	// Over her head, fading in as she's stunned and out as she comes to.
	const float Scale = GetSizeScale();
	const FVector Head = GetActorLocation() + GetActorForwardVector() * (60.f * Scale)
		+ FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 40.f * Scale);
	const float Left = Clods.StunSeconds - StunClock;
	const float Fade = FMath::Clamp(StunClock / 0.1f, 0.f, 1.f) * FMath::Clamp(Left / 0.2f, 0.f, 1.f);
	if (Fade <= 0.f)
	{
		return;
	}
	const float Turn = StunClock * 2.f * UE_PI * StarTurnsPerSecond;
	for (int32 Index = 0; Index < StarCount; ++Index)
	{
		const float Angle = Turn + 2.f * UE_PI * Index / StarCount;
		const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		// Each bobs a little as it goes round, so the ring reads as stars wheeling rather than a hoop.
		const FVector Star = Head + Out * StarRing + FVector(0.0, 0.0, FMath::Sin(2.f * Angle) * 8.f);
		const FVector Along(-Out.Y, Out.X, 0.f);
		const float Half = StarSize * 0.5f;
		// A four-pointed star: two crossed streaks, the upright one a little longer.
		Effects.AddStreak(Star - FVector(0.0, 0.0, Half * 1.15f), Star + FVector(0.0, 0.0, Half * 1.15f), 4.f, StarColor, StarGlow * Fade);
		Effects.AddStreak(Star - Along * Half, Star + Along * Half, 4.f, StarColor, StarGlow * Fade);
		// Its trail: a few dim pieces of the ring behind it.
		FVector Last = Star;
		for (int32 Piece = 1; Piece <= TrailPieces; ++Piece)
		{
			const float Back = Angle - TrailArc * Piece / TrailPieces;
			const FVector Point = Head + FVector(FMath::Cos(Back), FMath::Sin(Back), 0.f) * StarRing + FVector(0.0, 0.0, FMath::Sin(2.f * Back) * 8.f);
			Effects.AddStreak(Point, Last, 3.f * (1.f - 0.2f * Piece), StarColor, StarGlow * 0.3f * Fade * (1.f - 0.25f * Piece));
			Last = Point;
		}
	}
}
