#include "Core/LooterMenuPlayerController.h"
#include "World/MinimapSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ALooterMenuPlayerController::ALooterMenuPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	// The menu runs on the mouse alone.
	bShowMouseCursor = true;
	// The view stays on the circling camera (the engine would otherwise look for one placed in the level).
	bAutoManageActiveCameraTarget = false;
}

void ALooterMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	// The island: everything walkable, as the minimap measures it.
	FBox Ground(ForceInit);
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(MinimapTags::Ground))
		{
			Ground += It->GetComponentsBoundingBox();
		}
	}
	if (Ground.IsValid)
	{
		Center = Ground.GetCenter();
		Radius = FMath::Max(FMath::Max(Ground.GetExtent().X, Ground.GetExtent().Y), 1000.f);
		Top = Ground.Max.Z;
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	OrbitCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
	if (OrbitCamera)
	{
		OrbitCamera->GetCameraComponent()->SetFieldOfView(FieldOfView);
		OrbitCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
		Angle = StartAngle;
		PlaceCamera();
		SetViewTarget(OrbitCamera);
	}
}

void ALooterMenuPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (OrbitCamera)
	{
		Angle = FMath::Fmod(Angle + DegreesPerSecond * DeltaSeconds, 360.f);
		PlaceCamera();
	}
}

void ALooterMenuPlayerController::PlaceCamera()
{
	// Out on the circle at Angle (0 north along X, 90 east along Y), a little above the ground's top.
	const float Radians = FMath::DegreesToRadians(Angle);
	const FVector Out(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
	const FVector Eye = FVector(Center.X, Center.Y, Top) + Out * Radius * Distance + FVector::UpVector * Radius * Height;
	const FVector Aim = FVector(Center.X, Center.Y, Top - Radius * AimDrop);
	FRotator View = (Aim - Eye).Rotation();
	View.Yaw -= AimOffset;
	OrbitCamera->SetActorLocationAndRotation(Eye, View);
}
