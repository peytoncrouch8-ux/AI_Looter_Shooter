#include "Scenes/SkiffRide.h"
#include "AI_Looter_Shooter.h"
#include "Scenes/SceneCloudBank.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The bob comes in over this long after the ropes slip, so the ride starts exactly where the skiff was moored. */
	constexpr float BobInSeconds = 2.5f;

	/** The gas-bag's slow heave (cm), roll and pitch (degrees), each at its own pace (Hz) so they never fall into step. */
	constexpr float HeaveCm = 12.f;
	constexpr float HeaveHz = 0.21f;
	constexpr float RollDegrees = 1.2f;
	constexpr float RollHz = 0.17f;
	constexpr float PitchDegrees = 0.6f;
	constexpr float PitchHz = 0.29f;

	/** The cut from the jetty to the deck happens in black, fading in over this long. */
	constexpr float FadeInSeconds = 0.6f;

	/**
	 * The deck spot on a skiff with no Deck socket, from its pivot at the keel: where SM_Skiff_A_Packet's socket is. Skiff.py
	 * puts it 1.2 m toward the bow (Blender's -Y) and 0.43 m up; the importer turns Blender's -Y into Unreal's +X.
	 */
	const FVector FallbackDeck(120.0, 0.0, 43.0);

	/** What the ride's moves and moments share; they outlive the call that made them. */
	struct FRideState
	{
		TWeakObjectPtr<USceneSubsystem> Subsystem;
		TWeakObjectPtr<AActor> Boat;
		FTransform Moored;
		FVector Bow = FVector::ForwardVector;
		/** How far the player's view has been turned with the skiff so far (degrees). */
		float ViewTurned = 0.f;
	};

	/** The skiff's component with the Deck socket (its mesh), else its root. */
	USceneComponent* DeckCarrier(const AActor& Skiff)
	{
		TInlineComponentArray<USceneComponent*> Components(&Skiff);
		for (USceneComponent* Component : Components)
		{
			if (Component && Component->DoesSocketExist(SkiffRide::DeckSocket()))
			{
				return Component;
			}
		}
		return Skiff.GetRootComponent();
	}

	UTransitionScreenSubsystem* ScreenFor(const FRideState& Ride)
	{
		const USceneSubsystem* Subsystem = Ride.Subsystem.Get();
		return Subsystem ? UTransitionScreenSubsystem::Get(Subsystem) : nullptr;
	}

	/** As it starts: the player aboard, the cut hidden in black, the cloud bank ahead. */
	void StartRide(const FRideState& Ride)
	{
		USceneSubsystem* Subsystem = Ride.Subsystem.Get();
		AActor* Boat = Ride.Boat.Get();
		if (!Subsystem || !Boat)
		{
			return;
		}
		// It rides now, however it was placed.
		USceneComponent* Root = Boat->GetRootComponent();
		if (Root && Root->Mobility != EComponentMobility::Movable)
		{
			Root->SetMobility(EComponentMobility::Movable);
		}
		USceneComponent* Carrier = nullptr;
		const FVector Deck = SkiffRide::DeckSpotOf(*Boat, Carrier);
		const float Facing = Ride.Bow.Rotation().Yaw;
		Subsystem->CarryPlayer(Carrier, Deck, Facing);

		const APawn* Pawn = Subsystem->GetHeldPawn();
		APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		if (Controller && Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeInSeconds, FLinearColor::Black);
		}

		// The cloud bank's front stands on the course where the white is coming up, facing back along it.
		if (UWorld* World = Boat->GetWorld())
		{
			const FVector Front = SkiffRide::PoseAt(Ride.Moored, Ride.Bow, SkiffRide::CloudBankTime).GetLocation();
			const FRotator Heading(0.0, Facing + SkiffRide::TurnAt(SkiffRide::CloudBankTime), 0.0);
			if (ASceneCloudBank* Bank = ASceneCloudBank::Spawn(*World, FTransform(Heading, Front)))
			{
				Subsystem->AddSceneActor(Bank);
			}
		}
	}

	/** The skiff where the course has it Seconds in, and the player's view turned with it (never rocked). */
	void MoveSkiff(FRideState& Ride, float Seconds)
	{
		AActor* Boat = Ride.Boat.Get();
		if (!Boat)
		{
			return;
		}
		Boat->SetActorTransform(SkiffRide::PoseAt(Ride.Moored, Ride.Bow, Seconds), /*bSweep*/ false, nullptr, ETeleportType::None);
		const float Turned = SkiffRide::TurnAt(Seconds);
		if (USceneSubsystem* Subsystem = Ride.Subsystem.Get())
		{
			Subsystem->TurnHeldView(Turned - Ride.ViewTurned);
		}
		Ride.ViewTurned = Turned;
	}
}

FName SkiffRide::SceneName()
{
	return FName(TEXT("SkiffRide"));
}

FName SkiffRide::CastOffMoment()
{
	return FName(TEXT("CastOff"));
}

FName SkiffRide::WhiteoutMoment()
{
	return FName(TEXT("Whiteout"));
}

FName SkiffRide::DeckSocket()
{
	return FName(TEXT("Deck"));
}

float SkiffRide::DistanceAt(float Seconds)
{
	// Easing out of the moorings: the speed comes up along a smooth step over EaseOutSeconds, then holds at CruiseSpeed.
	const float Drift = FMath::Max(Seconds - CastOffTime, 0.f);
	if (Drift < EaseOutSeconds)
	{
		const float Eased = Drift / EaseOutSeconds;
		return CruiseSpeed * EaseOutSeconds * Eased * Eased * Eased * (1.f - 0.5f * Eased);
	}
	return CruiseSpeed * (Drift - 0.5f * EaseOutSeconds);
}

float SkiffRide::TurnAt(float Seconds)
{
	// In step with the distance covered: a circle's arc, TurnDegrees of it by full white.
	return TurnDegrees * DistanceAt(Seconds) / DistanceAt(FullWhiteTime);
}

float SkiffRide::WhiteAt(float Seconds)
{
	return FMath::SmoothStep(WhiteStart, FullWhiteTime, Seconds);
}

FTransform SkiffRide::PoseAt(const FTransform& Moored, const FVector& Bow, float Seconds)
{
	const FVector Forward = FVector(Bow.X, Bow.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector Starboard = FVector::CrossProduct(FVector::UpVector, Forward);
	const double Distance = DistanceAt(Seconds);
	const double Turn = FMath::DegreesToRadians(static_cast<double>(TurnAt(Seconds)));
	// Along the arc: as far ahead and to starboard as a circle turned through Turn over Distance takes it.
	const double Ahead = Turn > UE_DOUBLE_KINDA_SMALL_NUMBER ? Distance * FMath::Sin(Turn) / Turn : Distance;
	const double Aside = Turn > UE_DOUBLE_KINDA_SMALL_NUMBER ? Distance * (1.0 - FMath::Cos(Turn)) / Turn : 0.0;

	const float Drift = FMath::Max(Seconds - CastOffTime, 0.f);
	const float BobIn = FMath::SmoothStep(0.f, BobInSeconds, Drift);
	const float Heave = BobIn * HeaveCm * FMath::Sin(UE_TWO_PI * HeaveHz * Drift);
	const float Roll = BobIn * RollDegrees * FMath::Sin(UE_TWO_PI * RollHz * Drift + 1.3f);
	const float Pitch = BobIn * PitchDegrees * FMath::Sin(UE_TWO_PI * PitchHz * Drift + 0.4f);

	const FVector Location = Moored.GetLocation() + Forward * Ahead + Starboard * Aside
		+ FVector(0.0, 0.0, Distance * ClimbPerLength + Heave);
	// Turned about the up axis, then rocked in the skiff's own frame: rolling about its length (its local X, bow to stern),
	// pitching about its beam (its local Y).
	const FQuat Rock = FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Roll)) * FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Pitch));
	const FQuat Rotation = FQuat(FVector::UpVector, Turn) * Moored.GetRotation() * Rock;
	return FTransform(Rotation, Location, Moored.GetScale3D());
}

FVector SkiffRide::BowOf(const AActor& Skiff)
{
	// The model's front (Blender's -Y) comes in as the mesh's +X, the way an actor faces; its gangplank is on its -Y side.
	const USceneComponent* Carrier = DeckCarrier(Skiff);
	const FTransform Frame = Carrier ? Carrier->GetComponentTransform() : Skiff.GetActorTransform();
	FVector Bow = Frame.TransformVectorNoScale(FVector::ForwardVector);
	Bow.Z = 0.0;
	return Bow.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
}

FVector SkiffRide::DeckSpotOf(const AActor& Skiff, USceneComponent*& OutCarrier)
{
	OutCarrier = DeckCarrier(Skiff);
	if (OutCarrier && OutCarrier->DoesSocketExist(DeckSocket()))
	{
		return OutCarrier->GetSocketLocation(DeckSocket());
	}
	return Skiff.GetActorTransform().TransformPosition(FallbackDeck);
}

FScenePlay SkiffRide::Make(USceneSubsystem& Scenes, AActor& Skiff, FOnSceneMoment OnWhiteout)
{
	const TSharedRef<FRideState> Ride = MakeShared<FRideState>();
	Ride->Subsystem = &Scenes;
	Ride->Boat = &Skiff;
	Ride->Moored = Skiff.GetActorTransform();
	Ride->Bow = BowOf(Skiff);

	FScenePlay Scene;
	Scene.Name = SceneName();
	Scene.bLookOnly = true;
	// Travel follows the white: the player stays on deck, held, until the level goes.
	Scene.bHoldAfterEnd = true;
	Scene.OnStart = [Ride]() { StartRide(*Ride); };

	Scene.Timeline.AddMove(0.f, FullWhiteTime, [Ride](float Alpha) { MoveSkiff(*Ride, Alpha * FullWhiteTime); });
	Scene.Timeline.AddMove(WhiteStart, FullWhiteTime - WhiteStart, [Ride](float Alpha)
	{
		if (UTransitionScreenSubsystem* Screen = ScreenFor(*Ride))
		{
			Screen->SetWhite(WhiteAt(WhiteStart + Alpha * (FullWhiteTime - WhiteStart)));
		}
	});

	Scene.Timeline.AddMoment(CastOffMoment(), CastOffTime);
	Scene.Timeline.AddMoment(WhiteoutMoment(), FullWhiteTime, [Ride]()
	{
		// Fully white: it holds now, through the trip's level load, until the level arrived at reveals it.
		if (UTransitionScreenSubsystem* Screen = ScreenFor(*Ride))
		{
			Screen->HoldWhite();
		}
	});

	Scene.AfterEnd = [OnWhiteout]() { OnWhiteout.ExecuteIfBound(); };
	Scene.OnReturn = [Ride]()
	{
		if (AActor* Boat = Ride->Boat.Get())
		{
			Boat->SetActorTransform(Ride->Moored, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
		}
	};
	return Scene;
}
