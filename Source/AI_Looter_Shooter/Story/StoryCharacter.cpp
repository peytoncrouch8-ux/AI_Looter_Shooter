#include "Story/StoryCharacter.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Story/SpeakerPointComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AStoryCharacter::AStoryCharacter()
{
	// Ticks only while it's shown (SetShown): its pose.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// A person-sized stand-in (the engine's cylinder is 1 m across and 1 m tall, about its middle): 1.8 m tall, standing
	// on the actor's spot. Solid, so the player bumps into it and the crosshair's line finds it; world-dynamic, so traces
	// for the ground (world-static: creatures' footing, scattering, settling) never land on its head.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Placeholder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetStaticMesh(Placeholder.Object);
	Body->SetRelativeLocation(FVector(0.0, 0.0, 90.0));
	Body->SetRelativeScale3D(FVector(0.5, 0.5, 1.8));
	Body->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	// About head height: where the player looks to talk.
	SpeakerPoint = CreateDefaultSubobject<USpeakerPointComponent>(TEXT("SpeakerPoint"));
	SpeakerPoint->SetupAttachment(Root);
	SpeakerPoint->SetRelativeLocation(FVector(0.0, 0.0, 160.0));
}

void AStoryCharacter::BeginPlay()
{
	Super::BeginPlay();
	CapturePlacedBody();
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.AddUObject(this, &AStoryCharacter::HandleTalked);
	}
	// Whether it's there follows the story: looked at again whenever a mission starts, steps on or finishes.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AStoryCharacter::HandleMissionsChanged);
	}
	RefreshShown();
}

void AStoryCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	if (SpeakerPoint)
	{
		SpeakerPoint->OnTalked.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AStoryCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePose(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Talking: its speaker point's
// ---------------------------------------------------------------------------

FInteractionOptions AStoryCharacter::GetInteractionOptions(const UInteractionComponent& User) const
{
	return SpeakerPoint ? SpeakerPoint->GetInteractionOptions(User) : FInteractionOptions::None();
}

bool AStoryCharacter::Interact(UInteractionComponent& User, bool bHeld)
{
	return SpeakerPoint && SpeakerPoint->Interact(User, bHeld);
}

TOptional<FVector> AStoryCharacter::GetInteractionLocation() const
{
	return SpeakerPoint ? SpeakerPoint->GetInteractionLocation() : TOptional<FVector>();
}

void AStoryCharacter::HandleTalked(USpeakerPointComponent& Point, AActor* Listener)
{
	TalkingTo = Listener;
}

// ---------------------------------------------------------------------------
// In the story or not
// ---------------------------------------------------------------------------

void AStoryCharacter::HandleMissionsChanged()
{
	RefreshShown();
}

void AStoryCharacter::RefreshShown()
{
	SetShown(IsStoryShown());
}

bool AStoryCharacter::IsStoryShown() const
{
	if (ShownWhen.IsEmpty())
	{
		return true;
	}
	// The session's record (a test's in tests). Without a mission runner there's no story for it to be in.
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && ShownWhen.IsMet(Runner->GetCampaign(), Runner);
}

void AStoryCharacter::SetShown(bool bInShown)
{
	const bool bFirst = !bShownApplied;
	if (bShown == bInShown && !bFirst)
	{
		return;
	}
	bShown = bInShown;
	bShownApplied = true;
	// Out of the story it's out of the world: unseen, nothing to bump into, nobody to talk to, no pose to move.
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	SetActorTickEnabled(bShown);
	if (bFirst)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s: %s as play begins (%s)."), *GetActorNameOrLabel(), bShown ? TEXT("shown") : TEXT("hidden"),
			*ShownWhen.Describe());
	}
	else
	{
		UE_LOG(LogLooter, Log, TEXT("%s: %s by the story (%s)."), *GetActorNameOrLabel(), bShown ? TEXT("shown") : TEXT("hidden"),
			*ShownWhen.Describe());
	}
}

// ---------------------------------------------------------------------------
// The pose
// ---------------------------------------------------------------------------

void AStoryCharacter::CapturePlacedBody()
{
	if (bPlacedBodyCaptured || !Body)
	{
		return;
	}
	bPlacedBodyCaptured = true;
	PlacedBody = Body->GetRelativeTransform();
}

void AStoryCharacter::UpdatePose(float DeltaSeconds)
{
	if (!Body)
	{
		return;
	}
	CapturePlacedBody();
	PoseClock += DeltaSeconds;

	// While its lines play it turns to whoever it's talking to; after, back to how it was placed.
	float WantedYaw = 0.f;
	const AActor* Listener = TalkingTo.Get();
	if (Listener && SpeakerPoint && SpeakerPoint->IsTalking())
	{
		const FVector ToListener = Listener->GetActorLocation() - GetActorLocation();
		if (!ToListener.IsNearlyZero())
		{
			const double PlacedYaw = GetActorRotation().Yaw + PlacedBody.Rotator().Yaw;
			WantedYaw = static_cast<float>(FRotator::NormalizeAxis(ToListener.Rotation().Yaw - PlacedYaw));
		}
	}
	TurnYaw = TurnSpeed > 0.f ? FMath::FixedTurn(TurnYaw, WantedYaw, TurnSpeed * DeltaSeconds) : 0.f;

	// A slow breath: the body rises and settles a little, so it never stands like a post.
	const float Breath = BreathSeconds > 0.f ? FMath::Sin(2.f * UE_PI * PoseClock / BreathSeconds) : 0.f;
	const FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(TurnYaw));
	Body->SetRelativeLocationAndRotation(PlacedBody.GetLocation() + FVector(0.0, 0.0, BreathHeight * Breath), Turn * PlacedBody.GetRotation());
}
