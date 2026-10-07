// AHobBird: his body, his perches and his flights (his rig is HobBirdRig.cpp's).

#include "Story/HobBird.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLineSet.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** An asset by path when it's in this checkout, else null (as AUnpaidCreature finds its models). */
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

	/** Open, his wings reach about 46 cm either side of the folded mesh's bounds: kept on screen through a flap. */
	constexpr float WingBoundsScale = 4.f;

	/** Flying in from away: this far behind his perch and this high over it, he starts (cm). */
	constexpr float ArriveBehind = 700.f;
	constexpr float ArriveAbove = 450.f;

	/** A flight's arc over the straight line between perches (cm). */
	constexpr float FlightArc = 60.f;

	/** Closer than this to the new perch, he's already on it (cm). */
	constexpr float SameSpot = 10.f;
}

AHobBird::AHobBird()
{
	static USkeletalMesh* const Model = FindIfMade<USkeletalMesh>(TEXT("/Game/Art/Creatures/SK_Hob.SK_Hob"));

	// The story character's body is a person-sized stand-in: here it's the frame his model hangs on, unscaled, so the
	// story character's turn toward whoever talks to him turns all of him. It has no shape or collision of its own.
	Body->SetStaticMesh(nullptr);
	Body->SetRelativeLocation(FVector::ZeroVector);
	Body->SetRelativeScale3D(FVector::OneVector);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// His speaker point at his head; he turns quickly to whoever talks to him, as birds do. His rig does his breathing.
	SpeakerPoint->SetRelativeLocation(FVector(8.0, 0.0, 27.0));
	SpeakerPoint->SpeakerName = NSLOCTEXT("LooterStory", "HobName", "Hob");
	TurnSpeed = 400.f;
	BreathHeight = 0.f;

	if (Model)
	{
		// His feet grip the perch at the origin, his front along +X: on the body's frame as he is. No hit zones and no
		// collision: a friend, whom shots pass through.
		Bird = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Bird"));
		Bird->SetupAttachment(Body);
		// Only the asset here: its pose is set up as the component registers.
		Bird->SetSkinnedAsset(Model);
		Bird->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Bird->SetGenerateOverlapEvents(false);
		Bird->SetBoundsScale(WingBoundsScale);
		return;
	}

	// No model in this checkout: a black bird of the engine's plain shapes, perched.
	static UStaticMesh* const Sphere = FindIfMade<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static UStaticMesh* const Cone = FindIfMade<UStaticMesh>(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static UMaterialInterface* const Black = FindIfMade<UMaterialInterface>(TEXT("/Game/Art/Materials/MI_PaintBlack.MI_PaintBlack"));
	auto MakePart = [this](const TCHAR* Name, UStaticMesh* Shape, const FVector& At, const FRotator& Turn, const FVector& Size)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Body);
		Part->SetStaticMesh(Shape);
		// The engine's shapes are 100 cm across.
		Part->SetRelativeLocationAndRotation(At, Turn);
		Part->SetRelativeScale3D(Size / 100.0);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Black)
		{
			Part->SetMaterial(0, Black);
		}
		StandIn.Add(Part);
	};
	MakePart(TEXT("StandInBody"), Sphere, FVector(0.0, 0.0, 17.0), FRotator(14.0, 0.0, 0.0), FVector(30.0, 16.0, 18.0));
	MakePart(TEXT("StandInHead"), Sphere, FVector(13.0, 0.0, 28.0), FRotator::ZeroRotator, FVector(11.0, 10.0, 11.0));
	MakePart(TEXT("StandInBeak"), Cone, FVector(21.0, 0.0, 27.5), FRotator(-90.0, 0.0, 0.0), FVector(3.6, 3.6, 8.0));
}

void AHobBird::BeginPlay()
{
	bRigReady = SetupRig();
	// The story character's start reads the story: wherever it has him as the level begins, he's simply there.
	Super::BeginPlay();
	bBegun = true;
}

// ---------------------------------------------------------------------------
// Perches
// ---------------------------------------------------------------------------

int32 AHobBird::FindPerch() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	for (int32 Index = 0; Index < Perches.Num(); ++Index)
	{
		const FStoryCondition& When = Perches[Index].When;
		if (When.IsEmpty() || (Runner && When.IsMet(Runner->GetCampaign(), Runner)))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void AHobBird::RefreshShown()
{
	const int32 Wanted = IsStoryShown() ? FindPerch() : INDEX_NONE;
	if (Wanted == INDEX_NONE)
	{
		// No perch for this point in the story: he's away.
		Perch = INDEX_NONE;
		FlightLeft = 0.f;
		SetShown(false);
		return;
	}
	SetShown(true);
	if (Wanted != Perch)
	{
		FlyTo(Wanted);
	}
}

void AHobBird::FlyTo(int32 Index)
{
	const FHobPerch& To = Perches[Index];
	const bool bFromAway = Perch == INDEX_NONE;
	Perch = Index;
	if (!bBegun)
	{
		// As the level begins he's simply where the story has him, with nothing to say.
		SetActorLocationAndRotation(To.Location, FRotator(0.0, To.Yaw, 0.0));
		FlightLeft = 0.f;
		return;
	}
	// A new perch in the same spot (the story moved on, he stayed): no flight, only what he has to say there.
	if (!bFromAway && FVector::DistSquared(GetActorLocation(), To.Location) < FMath::Square(SameSpot))
	{
		Land();
		return;
	}
	// From away he drops in from above and behind his perch, so he lands facing out; otherwise from where he is.
	FlightFrom = bFromAway ? To.Location + FRotator(0.0, To.Yaw + 180.0, 0.0).Vector() * ArriveBehind + FVector(0.0, 0.0, ArriveAbove)
		: GetActorLocation();
	FlightTo = To.Location;
	FlightTotal = FMath::Max(FlightSeconds, 0.1f);
	FlightLeft = FlightTotal;
	SetActorLocation(FlightFrom);
	UE_LOG(LogLooter, Log, TEXT("%s: flies to perch %d (%s)."), *GetActorNameOrLabel(), Index + 1, *To.When.Describe());
}

void AHobBird::FinishFlight()
{
	if (FlightLeft > 0.f)
	{
		Land();
	}
}

void AHobBird::Land()
{
	FlightLeft = 0.f;
	WingOpen = 0.f;
	WingFlap = 0.f;
	if (!Perches.IsValidIndex(Perch))
	{
		return;
	}
	const FHobPerch& At = Perches[Perch];
	SetActorLocationAndRotation(At.Location, FRotator(0.0, At.Yaw, 0.0));
	// Only Ellis hears him; his remarks wait their turn behind whatever is being said.
	const TArray<FStoryLine> Said = GetArrivalLines(At);
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (Captions && !Said.IsEmpty())
	{
		Captions->Play(Said, ECaptionPlay::Queue);
	}
}

TArray<FStoryLine> AHobBird::GetArrivalLines(const FHobPerch& At) const
{
	TArray<FStoryLine> Said = At.ArrivalSet ? At.ArrivalSet->Lines : At.Arrival;
	for (FStoryLine& Line : Said)
	{
		if (Line.Speaker.IsEmpty() && SpeakerPoint)
		{
			Line.Speaker = SpeakerPoint->SpeakerName;
		}
	}
	return Said;
}

// ---------------------------------------------------------------------------
// Pose
// ---------------------------------------------------------------------------

void AHobBird::UpdatePose(float DeltaSeconds)
{
	if (FlightLeft > 0.f)
	{
		FlightLeft = FMath::Max(FlightLeft - DeltaSeconds, 0.f);
		const float Alpha = 1.f - FlightLeft / FlightTotal;
		const float Eased = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);
		const FVector Where = FMath::Lerp(FlightFrom, FlightTo, Eased) + FVector(0.0, 0.0, FlightArc * FMath::Sin(UE_PI * Alpha));
		FVector Heading = FlightTo - FlightFrom;
		Heading.Z = 0.0;
		const float PerchYaw = Perches.IsValidIndex(Perch) ? Perches[Perch].Yaw : GetActorRotation().Yaw;
		const float FlightYaw = Heading.IsNearlyZero() ? PerchYaw : Heading.Rotation().Yaw;
		// Turning to face out as he comes down onto the perch.
		const float Yaw = FlightYaw + FRotator::NormalizeAxis(PerchYaw - FlightYaw) * FMath::SmoothStep(0.6f, 1.f, Alpha);
		SetActorLocationAndRotation(Where, FRotator(0.0, Yaw, 0.0));
		PoseRig(DeltaSeconds);
		if (FlightLeft <= 0.f)
		{
			Land();
		}
		return;
	}
	Super::UpdatePose(DeltaSeconds);
	PoseRig(DeltaSeconds);
}
