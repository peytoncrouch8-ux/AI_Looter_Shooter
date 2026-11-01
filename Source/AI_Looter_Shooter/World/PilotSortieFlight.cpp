// APilotSortie: the Lamplighter's pose in each of Main 6's phases (the lift, her course out, the approach to her standoff,
// the haul into the berth, the berth, the glide under the gangway, home and the settle onto the ledge), and each phase's
// own end.

#include "World/PilotSortie.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "World/BargeBoarding.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"

namespace
{
	/** How much of her bob she keeps in each held pose (1: the full flight's sway). */
	constexpr float StandoffSway = 0.7f;
	constexpr float JumpSway = 0.25f;
	/** The haul's shudder: the chains snatching at her as they take up (degrees of roll toward the barge, Hz). */
	constexpr float PullShudder = 1.6f;
	constexpr float PullShudderHz = 2.2f;
	/** With no course placed she still gets out to the barge: the ledge to her standoff is one glide this long (s). */
	constexpr float NoCourseSeconds = 25.f;
	/** The settle's touch: the last share of it, when her keel finds the ledge. */
	constexpr float TouchAt = 0.92f;
	/** The tenders come at the latest this long into her course out (s). */
	constexpr float TenderCallLatest = 14.f;

	FTransform Level(const FTransform& Pose)
	{
		return FTransform(FRotator(0.0, Pose.Rotator().Yaw, 0.0), Pose.GetLocation(), FVector::OneVector);
	}
}

void APilotSortie::BuildCourses()
{
	if (!OutPath.Build(Course) && !Course.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: her course out needs two points or more (it has %d): she glides straight to the barge."),
			*GetActorNameOrLabel(), Course.Num());
	}
}

void APilotSortie::BuildHomeCourse()
{
	TArray<FVector> Points;
	Points.Add(LastCalm.GetLocation());
	if (!ReturnCourse.IsEmpty())
	{
		Points.Append(ReturnCourse);
	}
	else
	{
		for (int32 Index = Course.Num() - 1; Index >= 0; --Index)
		{
			Points.Add(Course[Index]);
		}
	}
	if (Points.Num() < 2)
	{
		// Nothing placed: straight back to over the ledge.
		if (const ALamplighterShip* Boat = Ship ? Ship.Get() : ShipNow.Get())
		{
			Points.Add(Boat->Moored.GetLocation() + FVector(0.0, 0.0, PilotLight::LiftRise));
		}
	}
	HomePath.Build(Points);
}

FTransform APilotSortie::Bobbed(const FTransform& Calm, float Sway) const
{
	float Heave = 0.f;
	float Roll = 0.f;
	float Pitch = 0.f;
	FLamplighterCourse::BobAt(Clock, FMath::Clamp(Sway, 0.f, 1.f), Heave, Roll, Pitch);
	FTransform Pose = Calm;
	Pose.AddToTranslation(FVector(0.0, 0.0, Heave));
	Pose.SetRotation(Calm.GetRotation() * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Roll))
		* FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Pitch)));
	return Pose;
}

void APilotSortie::AdvancePose(float DeltaSeconds, FTransform& OutPose, FTransform& OutCalm)
{
	const ALamplighterShip* Boat = FindShip();
	OutPose = Boat ? Boat->GetActorTransform() : GetActorTransform();
	OutPose.SetScale3D(FVector::OneVector);
	OutCalm = Level(OutPose);
	switch (Phase)
	{
	case EPilotPhase::LiftOff:
	{
		const FTransform Start = OutPath.IsValid() ? OutPath.CalmPoseAt(0.f) : Level(GlideFrom) * FTransform(FVector(0.0, 0.0, PilotLight::LiftRise));
		const float Alpha = PhaseTime / PilotLight::LiftSeconds;
		OutCalm = Level(PilotLight::LiftPose(GlideFrom, Start, Alpha));
		OutPose = Bobbed(PilotLight::LiftPose(GlideFrom, Start, Alpha), FMath::Clamp(Alpha, 0.f, 1.f));
		break;
	}
	case EPilotPhase::Cruise:
	{
		if (!OutPath.IsValid())
		{
			// No course: a long glide from where she lifted to her standoff (the approach takes it from there).
			OutCalm = PilotLight::Glide(GlideFrom, PilotLight::StandoffFor(GetBerth()), PhaseTime / NoCourseSeconds);
			OutPose = Bobbed(OutCalm, 1.f);
			break;
		}
		const float Length = OutPath.GetLength();
		// Slower while the tenders fight her (a steadier deck to shoot from); never past her course's end.
		const float Wanted = bTendersCalled && !bTendersDown ? PilotLight::FightSpeed : PilotLight::CruiseSpeed;
		const float Target = PilotLight::SpeedFor(Length - Distance, Wanted);
		Speed = FLamplighterCourse::SpeedToward(Speed, Target, DeltaSeconds);
		Distance = FMath::Min(Distance + Speed * DeltaSeconds, Length);
		OutPose = OutPath.PoseAt(Distance, Speed, Clock, 1.f);
		OutCalm = OutPath.CalmPoseAt(Distance);
		break;
	}
	case EPilotPhase::Approach:
	{
		const FTransform Standoff = PilotLight::StandoffFor(GetBerth());
		OutCalm = PilotLight::Glide(GlideFrom, Standoff, PhaseTime / PilotLight::ApproachSeconds);
		OutPose = Bobbed(OutCalm, 1.f - 0.3f * FMath::Clamp(PhaseTime / PilotLight::ApproachSeconds, 0.f, 1.f));
		break;
	}
	case EPilotPhase::Alongside:
		OutCalm = PilotLight::StandoffFor(GetBerth());
		OutPose = Bobbed(OutCalm, StandoffSway);
		break;
	case EPilotPhase::Pulling:
	{
		const float Alpha = FMath::Clamp(PhaseTime / PilotLight::PullSeconds, 0.f, 1.f);
		OutCalm = PilotLight::Glide(GlideFrom, GetBerth(), Alpha);
		// The chains snatch at her as they take up: a shudder toward the barge, dying as she comes home to the berth.
		const float Shudder = PullShudder * (1.f - Alpha) * FMath::Sin(UE_TWO_PI * PullShudderHz * PhaseTime);
		OutPose = Bobbed(OutCalm, StandoffSway * (1.f - Alpha));
		OutPose.SetRotation(OutPose.GetRotation() * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Shudder)));
		break;
	}
	case EPilotPhase::Boarded:
		// Dead still in the berth: the plank's ends never move under the player's feet.
		OutCalm = OutPose = GetBerth();
		break;
	case EPilotPhase::Jump:
	{
		const FTransform Under = GetJumpBerth();
		if (!bAtJump)
		{
			const float Alpha = PhaseTime / PilotLight::JumpGlideSeconds;
			OutCalm = PilotLight::Glide(GlideFrom, Under, Alpha);
			if (Alpha >= 1.f)
			{
				bAtJump = true;
				LandfallRules::Say(this, PilotLight::JumpLines(), /*bInterrupt*/ true);
				UE_LOG(LogLooter, Log, TEXT("The Pilot Light: she's under the barge's broken gangway; waiting for the jump."));
			}
		}
		else
		{
			// Holding under the gangway as the dying barge lists and drifts: she follows it.
			OutCalm = Under;
		}
		OutPose = Bobbed(OutCalm, JumpSway);
		break;
	}
	case EPilotPhase::Falling:
		OutPose = LastPose;
		OutCalm = LastCalm;
		break;
	case EPilotPhase::Home:
	{
		if (!HomePath.IsValid())
		{
			OutPose = LastPose;
			OutCalm = LastCalm;
			break;
		}
		const float Length = HomePath.GetLength();
		const float Target = PilotLight::SpeedFor(Length - Distance, PilotLight::HomeSpeed);
		Speed = FLamplighterCourse::SpeedToward(Speed, Target, DeltaSeconds);
		Distance = FMath::Min(Distance + Speed * DeltaSeconds, Length);
		OutPose = HomePath.PoseAt(Distance, Speed, Clock, 1.f);
		OutCalm = HomePath.CalmPoseAt(Distance);
		break;
	}
	case EPilotPhase::Settling:
	{
		const FTransform Moored = Boat ? Level(Boat->Moored) : OutCalm;
		const float Alpha = FMath::Clamp(PhaseTime / PilotLight::SettleSeconds, 0.f, 1.f);
		OutCalm = PilotLight::SettlePose(GlideFrom, Moored, Alpha);
		OutPose = Bobbed(OutCalm, 1.f - Alpha);
		break;
	}
	default:
		break;
	}
}

void APilotSortie::AdvanceBeats()
{
	switch (Phase)
	{
	case EPilotPhase::LiftOff:
		if (PhaseTime >= PilotLight::LiftSeconds)
		{
			EnterPhase(EPilotPhase::Cruise, /*bFresh*/ false);
		}
		break;
	case EPilotPhase::Cruise:
	{
		const float Length = OutPath.GetLength();
		const float CallAt = FMath::Max(PilotLight::TenderCallLeast, Length * PilotLight::TenderCallShare);
		if (!bTendersCalled && (Distance >= CallAt || PhaseTime >= TenderCallLatest))
		{
			CallTenders();
		}
		const bool bAtEnd = OutPath.IsValid() ? Distance >= Length - 1.f : PhaseTime >= NoCourseSeconds;
		// Should her course run out with the tenders still up, she hangs there and fights them.
		if (bAtEnd && bTendersDown)
		{
			EnterPhase(EPilotPhase::Approach, /*bFresh*/ false);
		}
		break;
	}
	case EPilotPhase::Approach:
		if (PhaseTime >= PilotLight::ApproachSeconds)
		{
			EnterPhase(EPilotPhase::Alongside, /*bFresh*/ false);
		}
		break;
	case EPilotPhase::Alongside:
		if (NumLaunchersBiting() >= 2)
		{
			EnterPhase(EPilotPhase::Pulling, /*bFresh*/ false);
		}
		break;
	case EPilotPhase::Pulling:
		if (PhaseTime >= PilotLight::PullSeconds)
		{
			EnterPhase(EPilotPhase::Boarded, /*bFresh*/ false);
			LandfallRules::Say(this, PilotLight::PlankLines());
		}
		break;
	case EPilotPhase::Home:
		if (!HomePath.IsValid() || Distance >= HomePath.GetLength() - 1.f)
		{
			EnterPhase(EPilotPhase::Settling, /*bFresh*/ false);
		}
		break;
	case EPilotPhase::Settling:
	{
		ALamplighterShip* Boat = FindShip();
		if (Boat && !bSettleTouched && PhaseTime >= PilotLight::SettleSeconds * TouchAt)
		{
			bSettleTouched = true;
			// Her keel finds the ledge: a soft groan through her, the dust kicked up round her, Junie's anchor going down.
			LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, Boat->GetActorLocation(), 0.55f, 0.85f);
			LooterSound::PlayAt(this, LooterSoundCue::AnchorSet, Boat->GetAnchorPoint(), 0.8f);
			LamplighterEffects::DustSkirt(this, Boat->GetActorLocation(), 1100.f, 16);
			LamplighterEffects::Shake(this, 0.2f, 0.5f);
			Boat->BraceCrew(1.f);
		}
		if (PhaseTime >= PilotLight::SettleSeconds)
		{
			EnterPhase(EPilotPhase::Idle, /*bFresh*/ false);
		}
		break;
	}
	default:
		break;
	}
}

FTransform APilotSortie::GetBerth() const
{
	if (const ABargeBoarding* Here = FindBarge())
	{
		FTransform Berth = Here->GetBerth();
		Berth.SetScale3D(FVector::OneVector);
		return Berth;
	}
	return BerthWithoutBarge;
}

FTransform APilotSortie::GetJumpBerth() const
{
	if (const ABargeBoarding* Here = FindBarge())
	{
		FTransform Under = Here->GetJumpBerth();
		Under.SetScale3D(FVector::OneVector);
		return Under;
	}
	// No barge: a little out and down from the berth, as the dying barge would leave her.
	return FTransform(FVector(-800.0, -300.0, -350.0)) * BerthWithoutBarge;
}

FVector APilotSortie::GetGrappleAnchor(int32 Index) const
{
	if (const ABargeBoarding* Here = FindBarge())
	{
		return Here->GetGrappleAnchor(Index);
	}
	// No barge: its rail would be about here, beside her berth's starboard (her frame).
	return BerthWithoutBarge.TransformPosition(FVector(Index == 0 ? 600.0 : -600.0, ALamplighterShip::RailOut + 500.0,
		ALamplighterShip::DeckTop + 300.0));
}
