#include "UI/Bench/BenchStage.h"
#include "UI/Inventory/StageStudio.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"

namespace
{
	/** The bench's own spot among the inventory's stands (0 the loadout's, 1 the bestiary's). */
	constexpr int32 BenchStandIndex = 2;

	/** The studio lights for a model about two meters across (the loadout's stand-in); a gun scales them down. */
	struct FBenchLight
	{
		FVector Location;
		float Candelas;
	};
	const FBenchLight KeyLightSetup{ FVector(-300.f, -220.f, 280.f), 60.f };
	const FBenchLight FillLightSetup{ FVector(-260.f, 260.f, 110.f), 14.f };
	const FBenchLight RimLightSetup{ FVector(180.f, 120.f, 230.f), 90.f };
	constexpr float LightReach = 1200.f;
	constexpr float ReferenceRadius = 100.f;
	/** A gun is small: the lights come no closer than this share of their places, so they never sit inside it. */
	constexpr float SmallestLightScale = 0.35f;

	/** Seconds the picture keeps rendering after the gun or the turn changes, while textures stream in. */
	constexpr float SettleSeconds = 1.f;

	/** The same gun to look at: a change of part, paint or notches makes another. */
	bool IsSameGun(const FWeaponInstanceData& A, const FWeaponInstanceData& B)
	{
		return A.Definition == B.Definition && A.Named == B.Named && A.Seed == B.Seed && A.Rarity == B.Rarity && A.Level == B.Level
			&& A.Parts == B.Parts && A.Kills == B.Kills;
	}
}

ABenchStage::ABenchStage()
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

	// The loadout's studio: a warm key from the front left, a cool fill from the right and a cyan rim from behind.
	KeyLight = StageStudio::MakeLight(this, Root, TEXT("KeyLight"), KeyLightSetup.Location, FLinearColor(1.f, 0.93f, 0.84f), KeyLightSetup.Candelas, true);
	FillLight = StageStudio::MakeLight(this, Root, TEXT("FillLight"), FillLightSetup.Location, FLinearColor(0.72f, 0.84f, 1.f), FillLightSetup.Candelas, false);
	RimLight = StageStudio::MakeLight(this, Root, TEXT("RimLight"), RimLightSetup.Location, FLinearColor(0.45f, 0.8f, 1.f), RimLightSetup.Candelas, false);
}

void ABenchStage::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(StageStudio::Location(BenchStandIndex));
	RenderTarget = StageStudio::MakeRenderTarget(this, TEXT("BenchStageImage"), ImageWidth, ImageHeight);
	Capture->TextureTarget = RenderTarget;
	Capture->ShowOnlyActors.Add(this);
	Turn = DefaultTurn;
	PlaceCamera();
}

// ---------------------------------------------------------------------------
// The gun
// ---------------------------------------------------------------------------

void ABenchStage::ShowGun(const FWeaponInstanceData& Gun, bool bResetTurn)
{
	if (bResetTurn)
	{
		Turn = DefaultTurn;
	}
	if (bHasGun && IsSameGun(Shown, Gun))
	{
		PlaceCamera();
		return;
	}
	if (!Model)
	{
		Model = NewObject<UWeaponModelComponent>(this, TEXT("Gun"));
		Model->SetupAttachment(Turntable);
		Model->RegisterComponent();
	}
	Shown = Gun;
	SlotParts.Reset();
	bHasGun = Gun.Definition && Model->Assemble(Gun);
	if (!bHasGun)
	{
		// Only guns built from parts can be copied; one with its own mesh asset stands as nothing.
		Model->Clear();
		PlaceCamera();
		return;
	}
	for (UStaticMeshComponent* Part : Model->GetParts())
	{
		// Made and registered by the model already: the studio's settings need the part drawn again to take.
		StageStudio::SetupPrimitive(Part);
		Part->MarkRenderStateDirty();
	}

	// The model makes a part for each filled slot, in the definition's order: match them up, so a slot's part can be marked.
	const FWeaponLook Look = WeaponParts::Pick(Gun);
	const TArray<TObjectPtr<UStaticMeshComponent>>& Parts = Model->GetParts();
	int32 Next = 0;
	for (const FWeaponPartOption* Option : Look.Parts)
	{
		SlotParts.Add(Option && Parts.IsValidIndex(Next) ? Parts[Next++].Get() : nullptr);
	}

	// Float it over the turntable's axis, its middle on the axis so it turns on the spot, its underside Lift over the floor.
	FBox Box(ForceInit);
	for (const UStaticMeshComponent* Part : Parts)
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
	PlaceCamera();
}

void ABenchStage::ClearGun()
{
	if (Model)
	{
		Model->Clear();
	}
	SlotParts.Reset();
	Shown = FWeaponInstanceData();
	bHasGun = false;
	PlaceCamera();
}

bool ABenchStage::GetSlotCorners(int32 SlotIndex, TArray<FVector>& OutCorners) const
{
	OutCorners.Reset();
	const UStaticMeshComponent* Part = SlotParts.IsValidIndex(SlotIndex) ? SlotParts[SlotIndex].Get() : nullptr;
	const UStaticMesh* Mesh = Part ? Part->GetStaticMesh() : nullptr;
	if (!Mesh)
	{
		return false;
	}
	// The part's own box, turned with it: its corners hug it however the gun is turned.
	const FBox Local = Mesh->GetBoundingBox();
	const FTransform& ToWorld = Part->GetComponentTransform();
	for (int32 Corner = 0; Corner < 8; ++Corner)
	{
		const FVector Point((Corner & 1) ? Local.Max.X : Local.Min.X, (Corner & 2) ? Local.Max.Y : Local.Min.Y, (Corner & 4) ? Local.Max.Z : Local.Min.Z);
		OutCorners.Add(ToWorld.TransformPosition(Point));
	}
	return true;
}

// ---------------------------------------------------------------------------
// The camera and the picture
// ---------------------------------------------------------------------------

void ABenchStage::SetActive(bool bInActive)
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
	}
}

void ABenchStage::AddTurn(float Degrees)
{
	Turn = FRotator::NormalizeAxis(Turn + Degrees);
	PlaceCamera();
}

void ABenchStage::PlaceCamera()
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
	// Turn 0 faces the gun's muzzle at the camera.
	Turntable->SetRelativeRotation(FRotator(0.f, 180.f + Turn, 0.f));

	// The lights keep their places around the gun as its size changes: nearer and dimmer by the square of the distance.
	const float Scale = FMath::Max(FMath::Max(SubjectReach, SubjectHeight) / ReferenceRadius, SmallestLightScale);
	const TPair<UPointLightComponent*, const FBenchLight*> Lights[] = {
		{ KeyLight.Get(), &KeyLightSetup }, { FillLight.Get(), &FillLightSetup }, { RimLight.Get(), &RimLightSetup } };
	for (const TPair<UPointLightComponent*, const FBenchLight*>& Light : Lights)
	{
		Light.Key->SetRelativeLocation(Light.Value->Location * Scale);
		Light.Key->SetAttenuationRadius(LightReach * Scale);
		Light.Key->SetIntensity(Light.Value->Candelas * Scale * Scale);
	}
	SettleTime = SettleSeconds;
}

void ABenchStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// A gun doesn't move by itself: a new picture only for a moment after a change.
	if (SettleTime > 0.f)
	{
		Capture->CaptureSceneDeferred();
	}
	SettleTime -= DeltaSeconds;
}

bool ABenchStage::ProjectToImage(const FVector& WorldLocation, FVector2D& OutUV) const
{
	return StageStudio::ProjectToImage(Capture, ImageWidth, ImageHeight, WorldLocation, OutUV);
}

FVector ABenchStage::GetFloorCenter() const
{
	return Turntable->GetComponentLocation();
}

FVector ABenchStage::GetTowardCamera() const
{
	const FVector Toward = (Capture->GetComponentLocation() - GetFloorCenter()).GetSafeNormal2D();
	return Toward.IsNearlyZero() ? FVector::BackwardVector : Toward;
}
