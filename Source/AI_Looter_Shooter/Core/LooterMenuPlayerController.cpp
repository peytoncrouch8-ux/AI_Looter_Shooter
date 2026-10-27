#include "Core/LooterMenuPlayerController.h"
#include "AI_Looter_Shooter.h"
#include "World/MinimapSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"

namespace
{
	TAutoConsoleVariable<float> CVarMenuDrift(TEXT("Looter.Menu.Drift"), 1.f,
		TEXT("The menu backdrop's slow camera drift (Lvl_MenuTeropa): 1 as made, 0 still."));
	TAutoConsoleVariable<float> CVarMenuParallax(TEXT("Looter.Menu.Parallax"), 1.f,
		TEXT("How much the menu backdrop's view turns toward the cursor: 1 as made, 0 not at all."));
	TAutoConsoleVariable<float> CVarMenuIntroSeconds(TEXT("Looter.Menu.IntroSeconds"), 5.f,
		TEXT("Seconds the menu backdrop's view takes to settle as the menu opens (0: no ease-in)."));

	// The drift's three waves (seconds a sway takes). Unrelated periods, so the sum never visibly repeats, and long ones:
	// at these the planet moves less than a pixel a frame, which reads as a camera floating, not a shake.
	constexpr float DriftYawPeriod = 53.f;
	constexpr float DriftPitchPeriod = 41.f;
	constexpr float DriftRollPeriod = 67.f;
	// Phases, so the three don't all start at their middle together (the opening frame already has a slight lean).
	constexpr float DriftPitchPhase = 1.3f;
	constexpr float DriftRollPhase = 0.7f;
	// Samples round a sphere's outline when finding its leftmost point: a degree apart, far finer than a pixel's worth.
	constexpr int32 OutlineSamples = 360;
}

ALooterMenuPlayerController::ALooterMenuPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	// The menu runs on the mouse alone.
	bShowMouseCursor = true;
	// The view stays on our own camera (the engine would otherwise look for one placed in the level).
	bAutoManageActiveCameraTarget = false;
}

void ALooterMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	OrbitCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Params);
	if (!OrbitCamera)
	{
		return;
	}
	OrbitCamera->GetCameraComponent()->SetConstraintAspectRatio(false);
	if (StartAuthoredView())
	{
		SetViewTarget(OrbitCamera);
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
	OrbitCamera->GetCameraComponent()->SetFieldOfView(FieldOfView);
	Angle = StartAngle;
	PlaceCamera();
	SetViewTarget(OrbitCamera);
}

bool ALooterMenuPlayerController::StartAuthoredView()
{
	const ACameraActor* Authored = nullptr;
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(MenuScene::ViewTag))
		{
			Authored = *It;
			break;
		}
	}
	if (!Authored)
	{
		return false;
	}
	bAuthoredView = true;
	BaseView = Authored->GetActorTransform();
	BaseView.SetScale3D(FVector::OneVector);
	const float Fov = Authored->GetCameraComponent()->FieldOfView;
	OrbitCamera->GetCameraComponent()->SetFieldOfView(Fov);
	Clock = 0.f;
	Parallax = FVector2D::ZeroVector;
	PlaceAuthoredView(0.f);

	// The scene's promise to the menu: the planet keeps right of the column even at the drift's and the cursor's reach.
	// Checked here as well as in Looter.Menu.Teropa.Scene, so a hand edit to the level shows in the log.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const float PlanetRadius = It->ActorHasTag(MenuScene::PlanetTag) ? PlanetRadiusOf(*It) : 0.f;
		if (PlanetRadius > 0.f)
		{
			const FRotator Worst(0.f, DriftReach.Yaw + ParallaxReach.X + IntroYaw, 0.f);
			const FTransform Turned(BaseView.GetRotation() * Worst.Quaternion(), BaseView.GetLocation());
			const float Limb = LeftmostNdcX(Turned, Fov, It->GetActorLocation(), PlanetRadius);
			if (Limb < MenuScene::ColumnEdgeNdc)
			{
				UE_LOG(LogLooter, Warning, TEXT("Menu backdrop: the planet reaches into the menu's column (left limb at %.2f, the column ends at %.2f)."),
					Limb, MenuScene::ColumnEdgeNdc);
			}
			break;
		}
	}
	return true;
}

void ALooterMenuPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!OrbitCamera)
	{
		return;
	}
	if (bAuthoredView)
	{
		PlaceAuthoredView(DeltaSeconds);
		return;
	}
	Angle = FMath::Fmod(Angle + DegreesPerSecond * DeltaSeconds, 360.f);
	PlaceCamera();
}

void ALooterMenuPlayerController::PlaceAuthoredView(float DeltaSeconds)
{
	Clock += DeltaSeconds;

	// The cursor, as a screen position (-1..1, +y up); centred when there is none (a gamepad, the window unfocused).
	FVector2D Target = FVector2D::ZeroVector;
	int32 SizeX = 0, SizeY = 0;
	float MouseX = 0.f, MouseY = 0.f;
	GetViewportSize(SizeX, SizeY);
	if (SizeX > 0 && SizeY > 0 && GetMousePosition(MouseX, MouseY))
	{
		Target.X = FMath::Clamp(MouseX / SizeX * 2.f - 1.f, -1.f, 1.f);
		Target.Y = FMath::Clamp(1.f - MouseY / SizeY * 2.f, -1.f, 1.f);
	}
	// Eased toward it (the first frame starts centred, so the view never jumps to wherever the cursor was left).
	if (DeltaSeconds > 0.f)
	{
		Parallax = FMath::Vector2DInterpTo(Parallax, Target, DeltaSeconds, ParallaxSpeed);
	}

	FRotator Turn = DriftAt(Clock, DriftReach, CVarMenuDrift.GetValueOnGameThread())
		+ ParallaxFor(Parallax, ParallaxReach, CVarMenuParallax.GetValueOnGameThread());
	Turn.Yaw += IntroYawAt(Clock, CVarMenuIntroSeconds.GetValueOnGameThread(), IntroYaw);
	OrbitCamera->SetActorLocationAndRotation(BaseView.GetLocation(), BaseView.GetRotation() * Turn.Quaternion());
}

FRotator ALooterMenuPlayerController::DriftAt(float Seconds, const FRotator& Reach, float Scale)
{
	const float Wave = UE_TWO_PI * Seconds;
	return FRotator(
		Reach.Pitch * Scale * FMath::Sin(Wave / DriftPitchPeriod + DriftPitchPhase),
		Reach.Yaw * Scale * FMath::Sin(Wave / DriftYawPeriod),
		Reach.Roll * Scale * FMath::Sin(Wave / DriftRollPeriod + DriftRollPhase));
}

FRotator ALooterMenuPlayerController::ParallaxFor(const FVector2D& CursorNdc, const FVector2D& Reach, float Scale)
{
	// Toward the cursor: right turns the view right (yaw up), up tips it up (pitch up).
	return FRotator(CursorNdc.Y * Reach.Y * Scale, CursorNdc.X * Reach.X * Scale, 0.f);
}

float ALooterMenuPlayerController::IntroYawAt(float Elapsed, float Seconds, float Yaw)
{
	if (Seconds <= 0.f || Elapsed >= Seconds)
	{
		return 0.f;
	}
	// Ease out (cubic): it moves most at the start and comes to rest without a stop.
	const float Left = 1.f - FMath::Clamp(Elapsed / Seconds, 0.f, 1.f);
	return Yaw * Left * Left * Left;
}

float ALooterMenuPlayerController::LeftmostNdcX(const FTransform& View, float FovDegrees, const FVector& Centre, float SphereRadius)
{
	// In the view's own axes: X ahead, Y right, Z up. The outline is the circle where the eye's rays graze the sphere.
	const FVector Local = View.InverseTransformPositionNoScale(Centre);
	const double Dist = Local.Size();
	const double TanHalf = FMath::Tan(FMath::DegreesToRadians(FovDegrees * 0.5));
	if (Dist <= SphereRadius || TanHalf <= 0.0)
	{
		return -TNumericLimits<float>::Max();
	}
	const FVector Axis = Local / Dist;
	const FVector RingCentre = Axis * (Dist - SphereRadius * SphereRadius / Dist);
	const double RingRadius = SphereRadius * FMath::Sqrt(1.0 - FMath::Square(SphereRadius / Dist));
	FVector U = FVector::CrossProduct(Axis, FVector::UpVector);
	if (!U.Normalize())
	{
		U = FVector::RightVector;
	}
	const FVector V = FVector::CrossProduct(U, Axis);
	double Leftmost = TNumericLimits<double>::Max();
	for (int32 Step = 0; Step < OutlineSamples; ++Step)
	{
		const double A = UE_DOUBLE_TWO_PI * Step / OutlineSamples;
		const FVector Point = RingCentre + (U * FMath::Cos(A) + V * FMath::Sin(A)) * RingRadius;
		if (Point.X <= UE_KINDA_SMALL_NUMBER)
		{
			return -TNumericLimits<float>::Max();
		}
		Leftmost = FMath::Min(Leftmost, Point.Y / Point.X / TanHalf);
	}
	return static_cast<float>(Leftmost);
}

float ALooterMenuPlayerController::PlanetRadiusOf(const AActor* Planet)
{
	const AStaticMeshActor* Mesh = Cast<AStaticMeshActor>(Planet);
	const UMaterialInterface* Material = Mesh && Mesh->GetStaticMeshComponent() ? Mesh->GetStaticMeshComponent()->GetMaterial(0) : nullptr;
	float Value = 0.f;
	return Material && Material->GetScalarParameterValue(FHashedMaterialParameterInfo(MenuScene::RadiusParameter), Value) ? Value : 0.f;
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
