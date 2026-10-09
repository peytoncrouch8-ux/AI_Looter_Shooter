#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/StageStudio.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"

namespace
{
	/** The loadout showcase's own spot among the inventory's stands (StageStudio::Location). */
	constexpr int32 ShowcaseStandIndex = 3;

	/** The studio lights for a model about two meters across; a gun scales them down (as on the bench's stand). */
	struct FShowcaseLight
	{
		FVector Location;
		float Candelas;
	};
	const FShowcaseLight KeyLightSetup{ FVector(-300.f, -220.f, 280.f), 60.f };
	const FShowcaseLight FillLightSetup{ FVector(-260.f, 260.f, 110.f), 16.f };
	const FShowcaseLight RimLightSetup{ FVector(180.f, 120.f, 230.f), 95.f };
	constexpr float LightReach = 1200.f;
	constexpr float ReferenceRadius = 100.f;
	/** A gun is small: the lights come no closer than this share of their places, so they never sit inside it. */
	constexpr float SmallestLightScale = 0.35f;
	/** The sway eases in over this long after the swing lands, or after a drag's pause, so it never starts with a jolt. */
	constexpr float SwayEaseSeconds = 1.2f;

	/** The same gun to look at: a change of part, paint or notches makes another. */
	bool IsSameGun(const FWeaponInstanceData& A, const FWeaponInstanceData& B)
	{
		return A.Definition == B.Definition && A.Named == B.Named && A.Seed == B.Seed && A.Rarity == B.Rarity && A.Level == B.Level
			&& A.Parts == B.Parts && A.Kills == B.Kills;
	}

	float EaseInOut(float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

ALoadoutGunStage::ALoadoutGunStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Turntable = CreateDefaultSubobject<USceneComponent>(TEXT("Turntable"));
	Turntable->SetupAttachment(Root);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	StageStudio::SetupCapture(Capture);

	// The inventory's studio: a warm key from the front left, a cool fill from the right and a cyan rim from behind.
	KeyLight = StageStudio::MakeLight(this, Root, TEXT("KeyLight"), KeyLightSetup.Location, FLinearColor(1.f, 0.93f, 0.84f), KeyLightSetup.Candelas, true);
	FillLight = StageStudio::MakeLight(this, Root, TEXT("FillLight"), FillLightSetup.Location, FLinearColor(0.72f, 0.84f, 1.f), FillLightSetup.Candelas, false);
	RimLight = StageStudio::MakeLight(this, Root, TEXT("RimLight"), RimLightSetup.Location, FLinearColor(0.45f, 0.8f, 1.f), RimLightSetup.Candelas, false);
}

void ALoadoutGunStage::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(StageStudio::Location(ShowcaseStandIndex));
	RenderTarget = StageStudio::MakeRenderTarget(this, TEXT("LoadoutShowcaseImage"), ImageWidth, ImageHeight);
	Capture->TextureTarget = RenderTarget;
	Capture->ShowOnlyActors.Add(this);
	PlaceCamera();
	PlaceTurn();
}

// ---------------------------------------------------------------------------
// The gun
// ---------------------------------------------------------------------------

void ALoadoutGunStage::ShowGun(const FWeaponInstanceData& Gun)
{
	if (bHasGun && IsSameGun(Shown, Gun))
	{
		return;
	}
	if (!Model)
	{
		Model = NewObject<UWeaponModelComponent>(this, TEXT("Gun"));
		Model->SetupAttachment(Turntable);
		Model->RegisterComponent();
	}
	Shown = Gun;
	bHasGun = Gun.Definition && Model->Assemble(Gun);
	if (!bHasGun)
	{
		// Only guns built from parts can be copied; one with its own mesh asset stands as nothing.
		Model->Clear();
		return;
	}
	for (UStaticMeshComponent* Part : Model->GetParts())
	{
		// Made and registered by the model already: the studio's settings need the part drawn again to take.
		StageStudio::SetupPrimitive(Part);
		Part->MarkRenderStateDirty();
	}

	// Float it over the turntable's axis, its middle on the axis so it turns on the spot, its underside Lift over the floor.
	FBox Box(ForceInit);
	for (const UStaticMeshComponent* Part : Model->GetParts())
	{
		if (const UStaticMesh* Mesh = Part->GetStaticMesh())
		{
			Box += Mesh->GetBoundingBox().TransformBy(Part->GetComponentTransform().GetRelativeTransform(Model->GetComponentTransform()));
		}
	}
	if (!Box.IsValid)
	{
		Box = FBox(FVector(-10.0), FVector(10.0));
	}
	const FVector Center = Box.GetCenter();
	Model->SetRelativeLocationAndRotation(FVector(-Center.X, -Center.Y, Lift - Box.Min.Z), FRotator::ZeroRotator);

	// Framed from the floor (its ring) to the gun's top, as wide as its length at any turn.
	const FVector Extent = Box.GetExtent();
	SubjectReach = FMath::Max(static_cast<float>(FMath::Max(Extent.X, Extent.Y)), 10.f);
	SubjectHeight = FMath::Max((Lift + static_cast<float>(Box.GetSize().Z)) * 0.5f, 10.f);
	RingRadius = FMath::Max(SubjectReach * 0.55f, 18.f);

	// Each new gun lands at the same view, swinging in: the moment it's chosen reads as a reveal.
	UserTurn = 0.f;
	SwingAge = 0.f;
	SinceDrag = 100.f;
	SwayClock = 0.f;
	PlaceCamera();
	PlaceTurn();
}

void ALoadoutGunStage::ClearGun()
{
	if (Model)
	{
		Model->Clear();
	}
	Shown = FWeaponInstanceData();
	bHasGun = false;
}

// ---------------------------------------------------------------------------
// The camera, the turn and the picture
// ---------------------------------------------------------------------------

void ALoadoutGunStage::SetActive(bool bInActive)
{
	if (bActive == bInActive)
	{
		return;
	}
	bActive = bInActive;
	SetActorTickEnabled(bActive);
	for (UPointLightComponent* Light : { KeyLight.Get(), FillLight.Get(), RimLight.Get() })
	{
		Light->SetVisibility(bActive);
	}
	if (bActive)
	{
		PlaceCamera();
		PlaceTurn();
	}
}

void ALoadoutGunStage::AddTurn(float Degrees)
{
	UserTurn = FRotator::NormalizeAxis(UserTurn + Degrees);
	SinceDrag = 0.f;
	PlaceTurn();
}

void ALoadoutGunStage::PlaceCamera()
{
	// The camera backs off until the gun and its ring fit, whatever its size: its reach across (the same at any turn, so
	// turning it never pushes anything out of the picture) and its height, each filling Fill of the picture. Its near end
	// is closer to the camera than its middle and looks bigger, so the camera keeps half its reach more.
	const float HalfWide = FMath::DegreesToRadians(FieldOfView * 0.5f);
	const float HalfHigh = FMath::Atan(FMath::Tan(HalfWide) * ImageHeight / ImageWidth);
	const float FitAcross = SubjectReach / FMath::Tan(HalfWide);
	const float FitUp = SubjectHeight / FMath::Tan(HalfHigh);
	const float Distance = FMath::Max(FitAcross, FitUp) / FMath::Max(Fill, 0.1f) + SubjectReach * 0.5f;
	const FRotator View(CameraPitch, 0.f, 0.f);
	Capture->SetRelativeLocationAndRotation(FVector(0.f, 0.f, SubjectHeight) - View.Vector() * Distance, View);
	Capture->FOVAngle = FieldOfView;

	// The lights keep their places around the gun as its size changes: nearer and dimmer by the square of the distance.
	const float Scale = FMath::Max(FMath::Max(SubjectReach, SubjectHeight) / ReferenceRadius, SmallestLightScale);
	const TPair<UPointLightComponent*, const FShowcaseLight*> Lights[] = {
		{ KeyLight.Get(), &KeyLightSetup }, { FillLight.Get(), &FillLightSetup }, { RimLight.Get(), &RimLightSetup } };
	for (const TPair<UPointLightComponent*, const FShowcaseLight*>& Light : Lights)
	{
		Light.Key->SetRelativeLocation(Light.Value->Location * Scale);
		Light.Key->SetAttenuationRadius(LightReach * Scale);
		Light.Key->SetIntensity(Light.Value->Candelas * Scale * Scale);
	}
}

void ALoadoutGunStage::PlaceTurn()
{
	// The swing: from SwingDegrees round to the view, fast at first and slowing as it lands (an ease-out cubic).
	const float SwingAlpha = FMath::Clamp(SwingAge / FMath::Max(SwingSeconds, 0.01f), 0.f, 1.f);
	const float Swing = SwingDegrees * FMath::Cube(1.f - SwingAlpha);
	// The sway: eased in after the swing lands and again after a drag's pause.
	const float AfterSwing = EaseInOut((SwingAge - SwingSeconds) / SwayEaseSeconds);
	const float AfterDrag = EaseInOut((SinceDrag - SwayResumeSeconds) / SwayEaseSeconds);
	const float Sway = SwayDegrees * AfterSwing * AfterDrag * FMath::Sin(UE_TWO_PI * SwayClock / FMath::Max(SwayPeriod, 0.5f));
	// Turn 0 faces the gun's muzzle at the camera.
	Turntable->SetRelativeRotation(FRotator(0.f, 180.f + DefaultTurn + UserTurn + Swing + Sway, 0.f));
}

void ALoadoutGunStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SwingAge += DeltaSeconds;
	SinceDrag += DeltaSeconds;
	// The sway's clock stands still while the sway is held back, so it comes back from where it is, not with a jump.
	if (SwingAge > SwingSeconds && SinceDrag > SwayResumeSeconds)
	{
		SwayClock += DeltaSeconds;
	}
	PlaceTurn();
	// The gun moves all the time it's shown, so it's drawn every frame: one small capture of a few parts.
	if (bHasGun)
	{
		Capture->CaptureSceneDeferred();
	}
}

bool ALoadoutGunStage::ProjectToImage(const FVector& WorldLocation, FVector2D& OutUV) const
{
	return StageStudio::ProjectToImage(Capture, ImageWidth, ImageHeight, WorldLocation, OutUV);
}

FVector ALoadoutGunStage::GetFloorCenter() const
{
	return Turntable->GetComponentLocation();
}

FVector ALoadoutGunStage::GetTowardCamera() const
{
	const FVector Toward = (Capture->GetComponentLocation() - GetFloorCenter()).GetSafeNormal2D();
	return Toward.IsNearlyZero() ? FVector::BackwardVector : Toward;
}
