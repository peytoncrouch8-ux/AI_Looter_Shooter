// AChapelBell: the rope's grip, the bell's swing and its tolls.

#include "World/ChapelBell.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AChapelBell::BellTag(TEXT("Bell_Chapel"));

namespace
{
	/** The chapel's bell (Art/Models/Buildings/Chapel.py's ChapelBell). */
	const TCHAR* BellPath = TEXT("/Game/Art/Buildings/SM_ChapelBell.SM_ChapelBell");

	/** The rope's grip: the woollen sally a hand closes on, about 40 cm long (half sizes, cm). */
	constexpr double GripHalfWidth = 8.0;
	constexpr double GripHalfHeight = 22.0;

	/** The shortest swing and ring it allows (s), whatever its settings say. */
	constexpr float MinSwingSeconds = 0.2f;
	constexpr float MinRingSeconds = 0.5f;

	/** An asset found only once it's imported, so the class works, and its tests run, without it. In a constructor. */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}

AChapelBell::AChapelBell()
{
	// Ticks only while it swings.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// What the Interact key's line (the Visibility channel) finds; nothing else meets it. World dynamic, so the traces that
	// look for the ground among world-static things (the minimap's bake, the scatter, the spawners) never do.
	Grip = CreateDefaultSubobject<UBoxComponent>(TEXT("Grip"));
	Grip->SetupAttachment(Root);
	Grip->SetMobility(EComponentMobility::Movable);
	Grip->InitBoxExtent(FVector(GripHalfWidth, GripHalfWidth, GripHalfHeight));
	Grip->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Grip->SetCollisionObjectType(ECC_WorldDynamic);
	Grip->SetCollisionResponseToAllChannels(ECR_Ignore);
	Grip->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Grip->SetGenerateOverlapEvents(false);
	Grip->SetCanEverAffectNavigation(false);
	Grip->SetHiddenInGame(true);

	// It swings high over everyone's head in the belfry: no collision, so moving it costs no physics.
	static UStaticMesh* const BellModel = FindIfMade<UStaticMesh>(BellPath);
	Bell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bell"));
	Bell->SetupAttachment(Root);
	Bell->SetMobility(EComponentMobility::Movable);
	Bell->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Bell->SetGenerateOverlapEvents(false);
	Bell->SetCanEverAffectNavigation(false);
	Bell->SetStaticMesh(BellModel);

	Prompt = NSLOCTEXT("LooterChapel", "RingPrompt", "Ring the chapel bell");
	Tags.Add(BellTag);
}

void AChapelBell::BeginPlay()
{
	Super::BeginPlay();
	// Hanging as the level placed it: that's still.
	CaptureRest();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AChapelBell::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AChapelBell::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// Being rung
// ---------------------------------------------------------------------------

FInteractionOptions AChapelBell::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// A bell still swinging is never focused, nor one the story doesn't let ring yet.
	Options.bUsable = CanRing();
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = Prompt;
	return Options;
}

bool AChapelBell::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && Ring(User.GetOwner());
}

TOptional<FVector> AChapelBell::GetInteractionLocation() const
{
	// The rope's grip, where a hand closes on it.
	return Grip ? TOptional<FVector>(Grip->GetComponentLocation()) : TOptional<FVector>();
}

bool AChapelBell::CanRing() const
{
	if (IsRinging())
	{
		return false;
	}
	if (RingWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && RingWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AChapelBell::Ring(AActor* ByWhom)
{
	if (!CanRing())
	{
		return false;
	}
	CaptureRest();
	RingTime = 0.f;
	Swing = 0.f;
	Strokes = 0;
	SetActorTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("%s: the chapel bell rings%s."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" (rung by %s)"), *ByWhom->GetName()) : TEXT(""));
	return true;
}

// ---------------------------------------------------------------------------
// The swing and the tolls
// ---------------------------------------------------------------------------

float AChapelBell::SwingAt(float Time, float Degrees, float Period, float Seconds)
{
	if (Time <= 0.f || Seconds <= 0.f || Time >= Seconds)
	{
		return 0.f;
	}
	// A swing that dies away, as the jetty's bell swings: a sine under a falling envelope, still by Seconds.
	const float Left = 1.f - Time / Seconds;
	return Degrees * Left * Left * FMath::Sin(2.f * UE_PI * Time / FMath::Max(Period, MinSwingSeconds));
}

void AChapelBell::Advance(float DeltaSeconds)
{
	if (RingTime < 0.f)
	{
		return;
	}
	const float Period = FMath::Max(SwingSeconds, MinSwingSeconds);
	const float Lasts = FMath::Max(RingSeconds, MinRingSeconds);
	const float Before = RingTime;
	RingTime += FMath::Max(DeltaSeconds, 0.f);

	// The clapper strikes at each end of a swing: a quarter of a swing after the pull, then every half swing. Each stroke
	// passed in this step tolls, as hard as the swing still is, while it's still a swing worth the name.
	const float FirstStroke = Period * 0.25f;
	const float StrokeEvery = Period * 0.5f;
	auto StrokesBy = [FirstStroke, StrokeEvery](float Time)
	{
		return Time < FirstStroke ? 0 : FMath::FloorToInt((Time - FirstStroke) / StrokeEvery) + 1;
	};
	const int32 Now = StrokesBy(FMath::Min(RingTime, Lasts));
	for (int32 Stroke = StrokesBy(Before); Stroke < Now; ++Stroke)
	{
		const float Left = 1.f - (FirstStroke + Stroke * StrokeEvery) / Lasts;
		const float Strength = Left * Left;
		if (Strength >= TollWhileAbove)
		{
			Toll(Strength);
		}
	}

	if (RingTime >= Lasts)
	{
		// Still again: it can be rung once more.
		RingTime = -1.f;
		Swing = 0.f;
		SetActorTickEnabled(false);
	}
	else
	{
		Swing = SwingAt(RingTime, SwingDegrees, Period, Lasts);
	}
	ApplySwing();
}

void AChapelBell::Toll(float Strength)
{
	++Strokes;
	// From the belfry, quieter as the swing dies.
	LooterSound::PlayAt(this, LooterSoundCue::ChapelBellToll, Bell ? Bell->GetComponentLocation() : GetActorLocation(),
		FMath::Clamp(Strength, 0.1f, 1.f));
}

void AChapelBell::CaptureRest()
{
	if (bRestCaptured || !Bell)
	{
		return;
	}
	bRestCaptured = true;
	BellRest = Bell->GetRelativeTransform();
}

void AChapelBell::ApplySwing()
{
	if (!Bell)
	{
		return;
	}
	CaptureRest();
	// About its own axis (its X) from where it hangs: the turn comes first, in the bell's own frame.
	const FQuat Turn(FVector::ForwardVector, FMath::DegreesToRadians(Swing));
	Bell->SetRelativeRotation(BellRest.GetRotation() * Turn);
}
