#include "World/WindowShutter.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	/** After it hits the casing it bounces back out this far (degrees) and settles, over this long (s). */
	constexpr float BounceDegrees = 6.f;
	constexpr float BounceSeconds = 0.16f;
}

AWindowShutter::AWindowShutter()
{
	// The tick runs only for a slam.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetMobility(EComponentMobility::Static);
	RootComponent = Hinge;

	// No collision, as the model has none: a shutter never stops a body or a shot.
	Leaf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Leaf"));
	Leaf->SetupAttachment(Hinge);
	Leaf->SetMobility(EComponentMobility::Movable);
	Leaf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Leaf->SetGenerateOverlapEvents(false);
}

void AWindowShutter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// In the editor it shows as it starts the game before Main 3: open against the wall.
	Swing = GetOpenSwing();
	ApplySwing();
}

void AWindowShutter::BeginPlay()
{
	Super::BeginPlay();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AWindowShutter::HandleMissionsChanged);
	}
	// The town knows Ellis is back once the story is past Main 3: shut as the level begins. Before that, open, watching.
	if (IsStoryShut())
	{
		ShutNow();
	}
	else
	{
		OpenNow();
	}
}

void AWindowShutter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetLooking(false);
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void AWindowShutter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceSlam(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Open and shut
// ---------------------------------------------------------------------------

float AWindowShutter::GetOpenSwing() const
{
	if (!FMath::IsNearlyZero(OpenYaw))
	{
		return OpenYaw;
	}
	// Its leaf reaches one way from the hinge; opening, its free edge swings out toward the street and round the other
	// way, to lie flat against the wall (a left shutter reaches toward -Y as it faces out, and opens positive).
	const UStaticMesh* Mesh = Leaf ? Leaf->GetStaticMesh() : nullptr;
	const bool bReachesRight = Mesh && Mesh->GetBoundingBox().GetCenter().Y > 0.0;
	return bReachesRight ? -OpenDegrees : OpenDegrees;
}

void AWindowShutter::OpenNow()
{
	State = EWindowShutterState::Open;
	Swing = GetOpenSwing();
	Delay = 0.f;
	Clock = 0.f;
	ApplySwing();
	SetActorTickEnabled(false);
	SetLooking(true);
}

void AWindowShutter::ShutNow()
{
	State = EWindowShutterState::Shut;
	Swing = 0.f;
	ApplySwing();
	SetActorTickEnabled(false);
	SetLooking(false);
}

bool AWindowShutter::NoticePlayer(const FVector& PlayerLocation)
{
	if (State != EWindowShutterState::Open
		|| FVector::DistSquared(PlayerLocation, GetActorLocation()) > FMath::Square(static_cast<double>(SlamRadius)))
	{
		return false;
	}
	// Each its own moment, so a row of windows slams one after another, not as one.
	Slam(FMath::FRandRange(0.f, FMath::Max(MaxDelay, 0.f)));
	UE_LOG(LogLooter, Verbose, TEXT("%s: the living shutter their windows."), *GetActorNameOrLabel());
	return true;
}

void AWindowShutter::Slam(float InDelay)
{
	if (State == EWindowShutterState::Shut || State == EWindowShutterState::Slamming)
	{
		return;
	}
	State = EWindowShutterState::Noticed;
	Delay = FMath::Max(InDelay, 0.f);
	Clock = 0.f;
	SwingFrom = Swing;
	bBanged = false;
	SetLooking(false);
	SetActorTickEnabled(true);
}

void AWindowShutter::AdvanceSlam(float DeltaSeconds)
{
	if (State == EWindowShutterState::Noticed)
	{
		Delay -= DeltaSeconds;
		if (Delay > 0.f)
		{
			return;
		}
		// The rest of this step swings.
		DeltaSeconds = -Delay;
		Delay = 0.f;
		State = EWindowShutterState::Slamming;
		Clock = 0.f;
	}
	if (State != EWindowShutterState::Slamming)
	{
		return;
	}
	Clock += DeltaSeconds;
	const float Duration = FMath::Max(SlamSeconds, 0.05f);
	if (Clock < Duration)
	{
		// Flung shut: it speeds up all the way to the casing.
		const float Alpha = Clock / Duration;
		Swing = SwingFrom * (1.f - Alpha * Alpha);
	}
	else
	{
		if (!bBanged)
		{
			bBanged = true;
			if (SlamSound && Leaf)
			{
				UGameplayStatics::PlaySoundAtLocation(this, SlamSound, Leaf->Bounds.Origin);
			}
		}
		const float After = Clock - Duration;
		if (After < BounceSeconds)
		{
			// A little bounce back off the casing, the way it came, and shut.
			Swing = FMath::Sign(SwingFrom) * BounceDegrees * FMath::Sin(UE_PI * After / BounceSeconds);
		}
		else
		{
			Swing = 0.f;
			State = EWindowShutterState::Shut;
			SetActorTickEnabled(false);
		}
	}
	ApplySwing();
}

void AWindowShutter::ApplySwing()
{
	if (Leaf)
	{
		Leaf->SetRelativeRotation(FRotator(0.f, Swing, 0.f));
	}
}

// ---------------------------------------------------------------------------
// The player and the story
// ---------------------------------------------------------------------------

void AWindowShutter::SetLooking(bool bLooking)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FTimerManager& Timers = World->GetTimerManager();
	if (!bLooking)
	{
		Timers.ClearTimer(LookTimer);
		return;
	}
	if (!Timers.IsTimerActive(LookTimer))
	{
		// From a random point of its interval, so a row of shutters doesn't look on the same frame.
		Timers.SetTimer(LookTimer, this, &AWindowShutter::LookForPlayer, LookSeconds, /*bLoop*/ true, FMath::FRandRange(0.05f, LookSeconds));
	}
}

void AWindowShutter::LookForPlayer()
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	if (Player)
	{
		NoticePlayer(Player->GetActorLocation());
	}
}

bool AWindowShutter::IsStoryShut() const
{
	if (ShutWhen.IsEmpty())
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && ShutWhen.IsMet(Runner->GetCampaign(), Runner);
}

void AWindowShutter::RefreshStory()
{
	if (State == EWindowShutterState::Open && IsStoryShut())
	{
		Slam(FMath::FRandRange(0.f, FMath::Max(MaxDelay, 0.f)));
	}
}

void AWindowShutter::HandleMissionsChanged()
{
	RefreshStory();
}
