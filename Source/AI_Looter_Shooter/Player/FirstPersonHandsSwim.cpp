#include "Player/FirstPersonHandsSwim.h"
#include "Player/FirstPersonHandsComponent.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "UI/HUD/LooterHUD.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// The first-person hands swimming (round 4): a breaststroke's keys for the right hand (the left mirrors it), treading water's
// sculling, and UFirstPersonHandsComponent::PlaceSwimHands, which puts both hands there once the gun is lowered away. Camera
// space: X ahead, Y right, Z up; cm. Every hold sits low and out of the crosshair's way.

namespace
{
	TAutoConsoleVariable<float> CVarTreadReach(TEXT("Looter.Hands.Swim.TreadReach"), 4.f,
		TEXT("How wide the hands' circles are treading water (cm)."));
	TAutoConsoleVariable<float> CVarTreadPace(TEXT("Looter.Hands.Swim.TreadPace"), 3.2f,
		TEXT("How fast the hands circle treading water (radians a second)."));

	struct FStrokeKey
	{
		float Phase;
		FVector Hold;
		FVector Fingers;
		FVector Palm;
		FVector Forearm;
	};

	/**
	 * The right hand's breaststroke: tucked under the chin, reaching out ahead palm down, sweeping out and back palm outward,
	 * pulling down under the frame, and round to the tuck again. The push (SwimRules::StrokePushAt, 0.3) is the reach's end.
	 */
	const FStrokeKey StrokeKeys[] = {
		{ 0.00f, FVector(22.f, 6.f, -22.f), FVector(1.f, -0.15f, 0.35f), FVector(0.f, 0.2f, -1.f), FVector(-0.55f, 0.25f, -0.8f) },
		{ 0.28f, FVector(44.f, 4.f, -13.f), FVector(1.f, -0.05f, 0.05f), FVector(0.f, 0.3f, -0.95f), FVector(-0.95f, 0.15f, -0.3f) },
		{ 0.52f, FVector(36.f, 30.f, -16.f), FVector(0.6f, 0.75f, -0.25f), FVector(-0.5f, 0.8f, -0.3f), FVector(-0.75f, -0.1f, -0.65f) },
		{ 0.76f, FVector(24.f, 22.f, -28.f), FVector(0.35f, 0.45f, -0.82f), FVector(-0.9f, 0.1f, -0.35f), FVector(-0.3f, -0.25f, -0.92f) },
	};
	constexpr int32 NumKeys = UE_ARRAY_COUNT(StrokeKeys);

	/** Treading water: the circles' middle, and the hand flat, palm down, fingers ahead and a little out. */
	const FVector TreadMiddle(30.f, 17.f, -25.f);
	const FVector TreadFingers(1.f, 0.25f, -0.1f);
	const FVector TreadPalm(0.f, 0.15f, -1.f);
	const FVector TreadForearm(-0.7f, 0.2f, -0.65f);

	FHandPlacement KeyPlacement(const FStrokeKey& Key)
	{
		return FHandPlacement::Make(Key.Hold, Key.Fingers.GetSafeNormal(), Key.Palm.GetSafeNormal(), FHandFingers::Of(EHandGrip::Open),
			Key.Forearm.GetSafeNormal());
	}
}

namespace FirstPersonHandsSwim
{
	FHandPlacement Mirrored(const FHandPlacement& Placement)
	{
		const FVector Flip(1.f, -1.f, 1.f);
		return FHandPlacement::Make(Placement.Hold * Flip, Placement.GetFingersDir() * Flip, Placement.GetPalmDir() * Flip, Placement.Fingers,
			Placement.Forearm * Flip);
	}

	FHandPlacement Stroke(EFirstPersonHand Hand, float Phase)
	{
		const float P = FMath::Fmod(FMath::Fmod(Phase, 1.f) + 1.f, 1.f);
		// The key at or before the phase, and the next one round the cycle.
		int32 From = NumKeys - 1;
		for (int32 Index = 0; Index < NumKeys; ++Index)
		{
			if (StrokeKeys[Index].Phase <= P)
			{
				From = Index;
			}
		}
		const int32 To = (From + 1) % NumKeys;
		const float Start = StrokeKeys[From].Phase;
		const float End = To == 0 ? 1.f : StrokeKeys[To].Phase;
		const float T = FMath::SmoothStep(0.f, 1.f, (P - Start) / FMath::Max(End - Start, 1.e-3f));
		const FHandPlacement Right = FHandPlacement::Blend(KeyPlacement(StrokeKeys[From]), KeyPlacement(StrokeKeys[To]), T);
		return Hand == EFirstPersonHand::Right ? Right : Mirrored(Right);
	}

	FHandPlacement Tread(EFirstPersonHand Hand, float Time)
	{
		// The two hands half a turn apart, each sculling its own small circle (wider across than up and down).
		const float Angle = Time * CVarTreadPace.GetValueOnAnyThread() + (Hand == EFirstPersonHand::Left ? UE_PI : 0.f);
		const float Reach = CVarTreadReach.GetValueOnAnyThread();
		const FVector Hold = TreadMiddle + FVector(FMath::Cos(Angle) * Reach, FMath::Sin(Angle) * Reach * 0.75f, FMath::Sin(Angle) * Reach * 0.35f);
		const FHandPlacement Right = FHandPlacement::Make(Hold, TreadFingers.GetSafeNormal(), TreadPalm.GetSafeNormal(), FHandFingers::Of(EHandGrip::Open),
			TreadForearm.GetSafeNormal());
		return Hand == EFirstPersonHand::Right ? Right : Mirrored(Right);
	}

	FHandPlacement Pose(EFirstPersonHand Hand, float Phase, float Effort, float Time)
	{
		return FHandPlacement::Blend(Tread(Hand, Time), Stroke(Hand, Phase), FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Effort, 0.f, 1.f)));
	}
}

bool UFirstPersonHandsComponent::PlaceSwimHands(float DeltaTime)
{
	// Only while swimming, once the gun is low enough to let go of, and only the first-person view's own hands, never behind
	// a menu.
	const APawn* Owner = Cast<APawn>(GetOwner());
	const UPlayerLocomotionComponent* Moves = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const float Share = Moves ? Moves->GetSwimHandsShare() : 0.f;
	const UPlayerViewComponent* PlayerView = View.Get();
	if (Share <= 0.f || !PlayerView || !PlayerView->IsFirstPerson())
	{
		return false;
	}
	const APlayerController* Player = Cast<APlayerController>(Owner->GetController());
	const ALooterHUD* Hud = Player ? Player->GetHUD<ALooterHUD>() : nullptr;
	if (Hud && Hud->IsMenuOpen())
	{
		return false;
	}
	const UWorld* World = GetWorld();
	const float Time = World ? static_cast<float>(World->GetTimeSeconds()) : 0.f;
	// Up from their rest under the frame as the gun goes down, back there before it comes up.
	for (const EFirstPersonHand Hand : { EFirstPersonHand::Left, EFirstPersonHand::Right })
	{
		const FHandPlacement Swim = FirstPersonHandsSwim::Pose(Hand, Moves->GetSwimStrokePhase(), Moves->GetSwimEffort(), Time);
		Targets[FirstPersonHands::Index(Hand)] = FHandPlacement::Blend(FirstPersonHandsMotion::BareRest(Hand), Swim, Share);
	}
	// No gun's jobs to ease between, no tool.
	bHaveSupport = false;
	CatchUp = 0.f;
	SupportJob = EHandsSupportJob::Foregrip;
	OneHandBlend = 0.f;
	bToolInHand = false;
	return true;
}
