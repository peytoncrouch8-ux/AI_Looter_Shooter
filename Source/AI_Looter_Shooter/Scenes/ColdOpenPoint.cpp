// The cold open's second half: dusk on Ransom's Point, through Ellis's eyes (the skiff and the scene's start are
// ColdOpen.cpp's).

#include "Scenes/ColdOpen.h"
#include "AI_Looter_Shooter.h"
#include "Scenes/ColdOpenBeats.h"
#include "Scenes/ColdOpenCast.h"
#include "Scenes/ColdOpenSet.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SceneTimeline.h"
#include "Camera/CameraActor.h"
#include "Sound/SoundBase.h"

namespace ColdOpenBeats
{
	namespace
	{
		/** The view on the Point breathes a little, so it never sits like a tripod's (degrees, Hz). */
		constexpr float SwayDegrees = 0.35f;
		constexpr float SwayHz = 0.23f;

		/** The Deacon's shot kicks the view back (degrees). */
		constexpr float ShotKick = 5.f;

		/** Lying on the boards, the view tips up past Sexton into the sky and rolls (degrees). */
		constexpr float FallenPitch = 22.f;
		constexpr float FallenRoll = 18.f;

		FVector Mark(const FColdOpenState& State, const FVector& Local)
		{
			const AColdOpenSet* Set = State.Set.Get();
			return Set ? Set->ToWorld(Local) : Local;
		}

		FVector DeaconHead(const FColdOpenState& State)
		{
			const AColdOpenCast* Props = State.Props.Get();
			return Props ? Props->GetHead(AColdOpenSet::DeaconIndex) : FVector::ZeroVector;
		}

		FVector AbelChest(const FColdOpenState& State)
		{
			const AColdOpenCast* Props = State.Props.Get();
			return Props ? Props->GetAbelChest() : FVector::ZeroVector;
		}

		/** A slow, small sway on a held view. */
		FRotator Swayed(const FColdOpenState& State, const FRotator& View)
		{
			const float Time = SceneTime(State);
			return View + FRotator(SwayDegrees * FMath::Sin(UE_TWO_PI * SwayHz * Time), SwayDegrees * FMath::Sin(UE_TWO_PI * SwayHz * 0.71f * Time + 1.3f), 0.0);
		}

		/** Where Ellis's eyes are and what they look at, beat by beat (the views the moves go between). */
		FTransform FacingDeacon(const FColdOpenState& State, const FVector& Eye)
		{
			return FTransform(LookAt(Eye, DeaconHead(State)), Eye);
		}

		FTransform FacingAbel(const FColdOpenState& State)
		{
			const AColdOpenSet* Set = State.Set.Get();
			const FVector Eye = Mark(State, Set ? Set->EllisRail : FVector::ZeroVector);
			return FTransform(LookAt(Eye, AbelChest(State)), Eye);
		}

		FTransform Fallen(const FColdOpenState& State)
		{
			const AColdOpenSet* Set = State.Set.Get();
			const AColdOpenCast* Props = State.Props.Get();
			const FVector Eye = Mark(State, Set ? Set->EllisFallen : FVector::ZeroVector);
			FRotator View = LookAt(Eye, Props ? Props->GetSextonHead() : Eye + FVector::UpVector);
			View.Pitch = FMath::Min(View.Pitch + FallenPitch, 80.0);
			View.Roll = FallenRoll;
			return FTransform(View, Eye);
		}

		/** Between two views, eased. */
		void Between(const FColdOpenState& State, const FTransform& From, const FTransform& To, float Alpha)
		{
			const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);
			const FQuat Turned = FQuat::Slerp(From.GetRotation(), To.GetRotation(), Eased);
			SetView(State, FMath::Lerp(From.GetLocation(), To.GetLocation(), Eased), Turned.Rotator());
		}

		void CutToPoint(FColdOpenState& State)
		{
			AColdOpenCast* Props = State.Props.Get();
			const AColdOpenSet* Set = State.Set.Get();
			if (Props)
			{
				Props->ShowPoint(true);
			}
			if (Set)
			{
				const FTransform View = FacingDeacon(State, Mark(State, Set->EllisView));
				SetView(State, View.GetLocation(), View.Rotator());
			}
			UE_LOG(LogLooter, Log, TEXT("Cold open: dusk on Ransom's Point."));
		}

		/** Where Ellis's eyes are now: the Deacon's gun points there. */
		FVector EllisEyes(const FColdOpenState& State)
		{
			const USceneSubsystem* Scenes = State.Scenes.Get();
			const ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
			return Camera ? Camera->GetActorLocation() : FVector::ZeroVector;
		}
	}

	void AddPointBeats(FScenePlay& Scene, const FStateRef& State, float Cut)
	{
		FSceneTimeline& Timeline = Scene.Timeline;
		using namespace ColdOpen;
		const AColdOpenSet* Set = State->Set.Get();
		const FVector View = Set ? Set->EllisView : FVector::ZeroVector;
		const FVector Rail = Set ? Set->EllisRail : FVector::ZeroVector;

		// The views: facing the Deacon over the ember, a step to the rail to see Abel, back to the Deacon, the fall.
		Timeline.AddMove(Cut, ToRailAt, [State, View](float)
		{
			const FTransform Facing = FacingDeacon(*State, Mark(*State, View));
			SetView(*State, Facing.GetLocation(), Swayed(*State, Facing.Rotator()));
		});
		Timeline.AddMove(Cut + ToRailAt, ToRailSeconds, [State, View](float Alpha)
		{
			Between(*State, FacingDeacon(*State, Mark(*State, View)), FacingAbel(*State), Alpha);
		});
		Timeline.AddMove(Cut + ToRailAt + ToRailSeconds, TurnBackAt - ToRailAt - ToRailSeconds, [State](float)
		{
			const FTransform Facing = FacingAbel(*State);
			SetView(*State, Facing.GetLocation(), Swayed(*State, Facing.Rotator()));
		});
		Timeline.AddMove(Cut + TurnBackAt, TurnBackSeconds, [State, Rail](float Alpha)
		{
			Between(*State, FacingAbel(*State), FacingDeacon(*State, Mark(*State, Rail)), Alpha);
		});
		Timeline.AddMove(Cut + TurnBackAt + TurnBackSeconds, FallAt - TurnBackAt - TurnBackSeconds, [State, Rail](float)
		{
			FTransform Facing = FacingDeacon(*State, Mark(*State, Rail));
			FRotator Look = Swayed(*State, Facing.Rotator());
			// The shot kicks the view up and back for a moment.
			if (State->KickAt >= 0.f)
			{
				Look.Pitch += ShotKick * FMath::Exp(-12.f * FMath::Max(SceneTime(*State) - State->KickAt, 0.f));
			}
			SetView(*State, Facing.GetLocation(), Look);
		});
		Timeline.AddMove(Cut + FallAt, FallSeconds, [State, Rail](float Alpha)
		{
			// Falling: slow to start, then all at once onto the boards, the sky turning over as it goes.
			const FTransform From = FacingDeacon(*State, Mark(*State, Rail));
			const FTransform To = Fallen(*State);
			const float Drop = FMath::Square(FMath::Clamp(Alpha * 1.15f, 0.f, 1.f));
			const FQuat Turned = FQuat::Slerp(From.GetRotation(), To.GetRotation(), FMath::SmoothStep(0.f, 1.f, Alpha));
			SetView(*State, FMath::Lerp(From.GetLocation(), To.GetLocation(), Drop), Turned.Rotator());
		});
		Timeline.AddMove(Cut + FallAt + FallSeconds, PointSeconds - FallAt - FallSeconds, [State](float)
		{
			const FTransform Lying = Fallen(*State);
			SetView(*State, Lying.GetLocation(), Swayed(*State, Lying.Rotator()));
		});

		// Abel up the bluff path, Ned's shot, the fall and the lantern rolling off.
		Timeline.AddMove(Cut + AbelRunsAt, AbelRunSeconds, [State](float Alpha)
		{
			if (AColdOpenCast* Props = State->Props.Get())
			{
				Props->RunAbel(Alpha);
			}
		});
		Timeline.AddMove(Cut + NedFiresAt, AbelFallSeconds, [State](float Alpha)
		{
			if (AColdOpenCast* Props = State->Props.Get())
			{
				Props->DropAbel(Alpha);
			}
		});
		Timeline.AddMove(Cut + NedFiresAt + 0.1f, LanternRollSeconds, [State](float Alpha)
		{
			if (AColdOpenCast* Props = State->Props.Get())
			{
				Props->RollLantern(Alpha);
			}
		});

		Timeline.AddMoment(TEXT("Point"), Cut, [State]() { CutToPoint(*State); });
		Timeline.AddMoment(TEXT("PointFadeIn"), Cut + 0.1f, [State]() { Fade(*State, 1.f, 0.f, CutFadeIn, false); });
		Timeline.AddMoment(TEXT("Bell"), Cut + BellAt, [State]()
		{
			const AColdOpenSet* Played = State->Set.Get();
			Sound(*State, Played ? Played->BellSound.Get() : nullptr);
			Say(*State, ELine::Bell);
		});
		Timeline.AddMoment(TEXT("PutHerBack"), Cut + PutHerBackAt, [State]() { Say(*State, ELine::PutHerBack); });
		Timeline.AddMoment(TEXT("El"), Cut + ElAt, [State]()
		{
			const AColdOpenSet* Played = State->Set.Get();
			const FVector Abel = AbelChest(*State);
			Sound(*State, Played ? Played->CallSound.Get() : nullptr, &Abel);
			Say(*State, ELine::El);
		});
		Timeline.AddMoment(TEXT("NedAims"), Cut + NedAimsAt, [State]()
		{
			if (AColdOpenCast* Props = State->Props.Get())
			{
				Props->Aim(AColdOpenSet::NedIndex, Props->GetAbelChest());
			}
		});
		Timeline.AddMoment(TEXT("NedFires"), Cut + NedFiresAt, [State]()
		{
			AColdOpenCast* Props = State->Props.Get();
			if (Props && !IsSkipping(*State))
			{
				Props->Fire(AColdOpenSet::NedIndex, Props->GetAbelChest());
			}
		});
		Timeline.AddMoment(TEXT("DeaconAims"), Cut + DeaconAimsAt, [State]()
		{
			if (AColdOpenCast* Props = State->Props.Get())
			{
				Props->Aim(AColdOpenSet::DeaconIndex, EllisEyes(*State));
			}
		});
		Timeline.AddMoment(TEXT("Sorry"), Cut + SorryAt, [State]() { Say(*State, ELine::Sorry); });
		Timeline.AddMoment(TEXT("DeaconFires"), Cut + DeaconFiresAt, [State]()
		{
			AColdOpenCast* Props = State->Props.Get();
			if (Props && !IsSkipping(*State))
			{
				Props->Fire(AColdOpenSet::DeaconIndex, EllisEyes(*State));
				State->KickAt = SceneTime(*State);
			}
		});
		Timeline.AddMoment(TEXT("SkyTurns"), Cut + FallAt, [State]()
		{
			// Far along the railing, a tall man in a stovepipe hat sits with his legs crossed, watching.
			if (AColdOpenCast* Props = State->Props.Get())
			{
				Props->ShowSexton(true);
			}
		});
		Timeline.AddMoment(TEXT("Black"), Cut + BlackAt, [State]() { Fade(*State, 0.f, 1.f, FadeSeconds, true); });
		Timeline.AddMoment(TEXT("End"), Cut + PointSeconds);
	}
}
