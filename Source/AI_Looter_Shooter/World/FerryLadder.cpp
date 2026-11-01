// AFerryLadder: the rope ladder from Gravewind Point's deck down to the Ferryman's landing, climbed as a short scene.

#include "World/FerryLadder.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Scenes/FerryLadderClimb.h"
#include "Scenes/SceneSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "FerryLadder"

namespace
{
	const TCHAR* LadderCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* LadderCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* LadderIronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");
	const TCHAR* LadderRopePath = TEXT("/Game/Art/Materials/MI_Canvas.MI_Canvas");
	const TCHAR* LadderRungPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	/** The stanchions: this tall, this far back from the edge and apart (cm). */
	constexpr float StanchionTall = 110.f;
	constexpr float StanchionBack = 8.f;
	constexpr float StanchionApart = 32.f;
	/** The ladder's face hangs this far out from the edge, the ropes this far apart, the rungs this wide (cm). */
	constexpr float FaceOut = 12.f;
	constexpr float RopeApart = 25.f;
	constexpr float RopeWidth = 3.f;
	constexpr float RungLength = 56.f;
	constexpr float RungGirth = 4.f;
	/** The top rung under the edge, the bottom one over the planks (cm). */
	constexpr float TopRungDown = 20.f;
	constexpr float BottomRungUp = 30.f;

	void StretchLadderPart(UStaticMeshComponent* Part, const FVector& A, const FVector& B, float Width)
	{
		if (!Part)
		{
			return;
		}
		const FVector Span = B - A;
		Part->SetRelativeLocationAndRotation((A + B) * 0.5, FRotationMatrix::MakeFromZ(Span.GetSafeNormal()).Rotator());
		Part->SetRelativeScale3D(FVector(Width / 100.0, Width / 100.0, FMath::Max(Span.Size(), 1.0) / 100.0));
	}
}

AFerryLadder::AFerryLadder()
{
	PrimaryActorTick.bCanEverTick = false;
	Tags.Add(FName(TEXT("FerryLadder")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(LadderCubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(LadderCylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(LadderIronPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Rope(LadderRopePath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Rung(LadderRungPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakePart = [&](const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Look, bool bSolid)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(Mesh);
		Part->SetMaterial(0, Look);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCollisionProfileName(bSolid ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		return Part;
	};
	StanchionLeft = MakePart(TEXT("StanchionLeft"), Cylinder.Object, Iron.Object, true);
	StanchionRight = MakePart(TEXT("StanchionRight"), Cylinder.Object, Iron.Object, true);
	Crossbar = MakePart(TEXT("Crossbar"), Cube.Object, Iron.Object, false);
	RopeLeft = MakePart(TEXT("RopeLeft"), Cylinder.Object, Rope.Object, false);
	RopeRight = MakePart(TEXT("RopeRight"), Cylinder.Object, Rope.Object, false);
	for (int32 Index = 0; Index < RungCount; ++Index)
	{
		const FName Name(*FString::Printf(TEXT("Rung%d"), Index));
		UStaticMeshComponent* Made = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Made->SetupAttachment(Root);
		Made->SetStaticMesh(Cylinder.Object);
		Made->SetMaterial(0, Rung.Object);
		Made->SetCanEverAffectNavigation(false);
		Made->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Rungs.Add(Made);
	}

	DownPrompt = LOCTEXT("Down", "Climb down to the ferry");
	UpPrompt = LOCTEXT("Up", "Climb up");
	Hang();
}

void AFerryLadder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Hang();
}

void AFerryLadder::BeginPlay()
{
	Super::BeginPlay();
	Hang();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AFerryLadder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AFerryLadder::Hang()
{
	for (const float Side : { -1.f, 1.f })
	{
		UStaticMeshComponent* Stanchion = Side < 0.f ? StanchionLeft.Get() : StanchionRight.Get();
		StretchLadderPart(Stanchion, FVector(-StanchionBack, Side * StanchionApart, 0.0), FVector(-StanchionBack, Side * StanchionApart, StanchionTall), 6.f);
		// Each rope from the crossbar, over the edge, down the drop to the planks.
		UStaticMeshComponent* Rope = Side < 0.f ? RopeLeft.Get() : RopeRight.Get();
		StretchLadderPart(Rope, FVector(-StanchionBack, Side * RopeApart, StanchionTall - 6.0), FVector(FaceOut, Side * RopeApart, -Drop + 4.0), RopeWidth);
	}
	if (Crossbar)
	{
		Crossbar->SetRelativeLocation(FVector(-StanchionBack, 0.0, StanchionTall - 2.0));
		Crossbar->SetRelativeScale3D(FVector(0.06, (StanchionApart * 2.0 + 10.0) / 100.0, 0.06));
	}
	// The rungs across the ropes, evenly from under the edge to over the planks (a rung's axis across the ladder: its Y).
	const FVector Top(FaceOut, 0.0, -TopRungDown);
	const FVector Bottom(FaceOut, 0.0, -Drop + BottomRungUp);
	for (int32 Index = 0; Index < Rungs.Num(); ++Index)
	{
		const float Along = Rungs.Num() > 1 ? static_cast<float>(Index) / (Rungs.Num() - 1) : 0.f;
		const FVector At = FMath::Lerp(Top, Bottom, static_cast<double>(Along));
		StretchLadderPart(Rungs[Index], At - FVector(0.0, RungLength * 0.5, 0.0), At + FVector(0.0, RungLength * 0.5, 0.0), RungGirth);
	}
}

FVector AFerryLadder::GetTopFeet() const
{
	return GetActorTransform().TransformPosition(FVector(-TopStandBack, 0.0, 0.0));
}

FVector AFerryLadder::GetBottomFeet() const
{
	return GetActorTransform().TransformPosition(FVector(FaceOut + BottomStandOut, 0.0, -Drop));
}

FVector AFerryLadder::GetTopRung() const
{
	return GetActorTransform().TransformPosition(FVector(FaceOut, 0.0, -TopRungDown));
}

FVector AFerryLadder::GetBottomRung() const
{
	return GetActorTransform().TransformPosition(FVector(FaceOut, 0.0, -Drop + BottomRungUp));
}

float AFerryLadder::GetTopYaw() const
{
	return static_cast<float>(FRotator::NormalizeAxis(GetActorRotation().Yaw + 180.0));
}

float AFerryLadder::GetBottomYaw() const
{
	return static_cast<float>(FRotator::NormalizeAxis(GetActorRotation().Yaw));
}

FTransform AFerryLadder::GetPerch() const
{
	return FTransform(FRotator(0.0, GetActorRotation().Yaw, 0.0), GetActorTransform().TransformPosition(FVector(-StanchionBack, 0.0, StanchionTall + 3.0)));
}

bool AFerryLadder::IsAtTop(const FVector& Location) const
{
	return Location.Z > GetActorLocation().Z - Drop * 0.5f;
}

const APawn* AFerryLadder::FindPlayer() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	return Controller ? Controller->GetPawn() : nullptr;
}

FInteractionOptions AFerryLadder::GetInteractionOptions(const UInteractionComponent& User) const
{
	const AActor* Who = User.GetOwner();
	FInteractionOptions Options;
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Who && !IsAtTop(Who->GetActorLocation()) ? UpPrompt : DownPrompt;
	return Options;
}

bool AFerryLadder::Interact(UInteractionComponent& User, bool bHeld)
{
	APawn* Climber = Cast<APawn>(User.GetOwner());
	return Climber && Climb(Climber, IsAtTop(Climber->GetActorLocation()));
}

TOptional<FVector> AFerryLadder::GetInteractionLocation() const
{
	const APawn* Player = FindPlayer();
	const bool bTop = !Player || IsAtTop(Player->GetActorLocation());
	return TOptional<FVector>(bTop ? GetActorTransform().TransformPosition(FVector(-StanchionBack, 0.0, StanchionTall * 0.7))
		: GetActorTransform().TransformPosition(FVector(FaceOut, 0.0, -Drop + 100.0)));
}

TOptional<FInteractionBox> AFerryLadder::GetInteractionBox() const
{
	const APawn* Player = FindPlayer();
	const bool bTop = !Player || IsAtTop(Player->GetActorLocation());
	// The stanchions and the ladder's head from the deck; its foot and its lowest rungs from the landing.
	return bTop ? InteractionFocus::BoxAround(GetActorTransform().TransformPosition(FVector(0.0, 0.0, StanchionTall * 0.5)),
			FVector(40.0, 55.0, StanchionTall * 0.6), GetActorQuat())
		: InteractionFocus::BoxAround(GetActorTransform().TransformPosition(FVector(FaceOut, 0.0, -Drop + 90.0)),
			FVector(30.0, 45.0, 90.0), GetActorQuat());
}

bool AFerryLadder::Climb(APawn* Climber, bool bDown)
{
	if (!Climber)
	{
		return false;
	}
	UWorld* World = GetWorld();
	USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (Scenes && World && USceneSubsystem::AreScenesOn(World) && !Scenes->IsPlaying()
		&& Scenes->Play(FerryLadderClimb::Make(*Scenes, *this, *Climber, bDown)))
	{
		return true;
	}
	// No scene (tours, perf runs, one playing already): straight to the far end, standing.
	const FVector Feet = bDown ? GetBottomFeet() : GetTopFeet();
	const float Yaw = bDown ? GetBottomYaw() : GetTopYaw();
	Climber->TeleportTo(Feet + FVector(0.0, 0.0, Climber->GetSimpleCollisionHalfHeight() + 2.0), FRotator(0.0, Yaw, 0.0),
		/*bIsATest*/ false, /*bNoCheck*/ true);
	if (APlayerController* Controller = Cast<APlayerController>(Climber->GetController()))
	{
		Controller->SetControlRotation(FRotator(0.0, Yaw, 0.0));
	}
	UE_LOG(LogLooter, Log, TEXT("%s: climbed %s at once (no scene)."), *GetActorNameOrLabel(), bDown ? TEXT("down") : TEXT("up"));
	return true;
}

#undef LOCTEXT_NAMESPACE
