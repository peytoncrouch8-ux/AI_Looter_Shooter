#include "Scenes/SceneCloudBank.h"
#include "AI_Looter_Shooter.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

namespace
{
	TAutoConsoleVariable<float> CVarCloudBrightness(
		TEXT("Looter.Scene.CloudBrightness"),
		1.f,
		TEXT("How bright a scene's cloud bank reads against the painted sky's clouds (1 as tuned; 0 leaves the bank out, the white alone)."));

	const TCHAR* QuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");

	/** The game's soft smoke puff (FWeaponFX's dust): unlit and translucent, colored and faded per instance (R, G, B, opacity). */
	const TCHAR* SmokeMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Smoke.M_FX_Smoke");
	constexpr int32 PuffFloats = 4;

	/** Lit tops and shaded, bluer undersides, about as bright as the painted sky's clouds (M_SkyClouds' LitColor, ShadeColor). */
	const FLinearColor LitColor(2.7f, 2.65f, 2.55f);
	const FLinearColor ShadeColor(1.55f, 1.65f, 1.85f);
	constexpr float PuffOpacity = 0.85f;

	/** A puff is gone this close to the camera, and whole from this far (cm). */
	constexpr float NearGone = 400.f;
	constexpr float NearFull = 1600.f;

	/** One puff of the bank: ahead of its front, to starboard and up (cm), its size, and its turn in its own plane. */
	struct FPuffLayout
	{
		float Ahead;
		float Starboard;
		float Up;
		float Size;
		float Roll;
	};

	// A wall of cloud: low, wide puffs at the front, larger and higher ones behind, and the middle thick enough to fly into.
	const FPuffLayout Layout[] = {
		{    0.f, -1100.f,  -200.f, 2600.f, 0.4f },
		{  200.f,  1200.f,  -400.f, 2800.f, 2.1f },
		{  600.f,    50.f,   500.f, 3000.f, 4.0f },
		{ 1000.f, -2600.f,   600.f, 3800.f, 1.2f },
		{ 1300.f,  2600.f,   900.f, 4000.f, 5.3f },
		{ 1600.f,  -500.f, -1100.f, 4200.f, 3.1f },
		{ 2000.f,   900.f,  1700.f, 4600.f, 0.9f },
		{ 2400.f, -1900.f,  2200.f, 5000.f, 2.7f },
		{ 2900.f,  1800.f,  -300.f, 5200.f, 4.6f },
		{ 3300.f,     0.f,  1000.f, 6000.f, 1.8f },
		{ 3800.f, -3400.f,  -800.f, 5400.f, 3.9f },
		{ 4200.f,  3500.f,  2400.f, 5800.f, 5.9f },
	};
}

ASceneCloudBank::ASceneCloudBank()
{
	PrimaryActorTick.bCanEverTick = true;
	// After the camera has its place for the frame, so the puffs face where it is now.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	Cards = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cards"));
	Cards->SetupAttachment(Root);
	Cards->SetMobility(EComponentMobility::Movable);
	Cards->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cards->SetCanEverAffectNavigation(false);
	Cards->CastShadow = false;
	Cards->bReceivesDecals = false;
}

ASceneCloudBank* ASceneCloudBank::Spawn(UWorld& World, const FTransform& Where)
{
	if (CVarCloudBrightness.GetValueOnGameThread() <= 0.f)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	ASceneCloudBank* Bank = World.SpawnActor<ASceneCloudBank>(ASceneCloudBank::StaticClass(), Where, Params);
	if (Bank && !Bank->Build())
	{
		Bank->Destroy();
		return nullptr;
	}
	return Bank;
}

bool ASceneCloudBank::Build()
{
	UStaticMesh* Quad = LoadObject<UStaticMesh>(nullptr, QuadPath);
	UMaterialInterface* Smoke = LoadObject<UMaterialInterface>(nullptr, SmokeMaterialPath);
	if (!Quad || !Smoke)
	{
		UE_LOG(LogLooter, Warning, TEXT("Cloud bank: %s or the engine's quad is missing, so the scene's cloud is the white alone."), SmokeMaterialPath);
		return false;
	}
	Cards->SetStaticMesh(Quad);
	Cards->SetMaterial(0, Smoke);
	Cards->SetNumCustomDataFloats(PuffFloats);

	float Lowest = TNumericLimits<float>::Max();
	float Highest = -TNumericLimits<float>::Max();
	for (const FPuffLayout& Each : Layout)
	{
		Lowest = FMath::Min(Lowest, Each.Up);
		Highest = FMath::Max(Highest, Each.Up);
	}
	Puffs.Reset();
	for (const FPuffLayout& Each : Layout)
	{
		FPuff& Puff = Puffs.AddDefaulted_GetRef();
		// The bank's forward axis is X, its starboard Y.
		Puff.Offset = FVector(Each.Ahead, Each.Starboard, Each.Up);
		Puff.Size = Each.Size;
		Puff.Roll = Each.Roll;
		Puff.Height = Highest > Lowest ? (Each.Up - Lowest) / (Highest - Lowest) : 0.5f;
	}
	Redraw(FindViewer());
	return true;
}

void ASceneCloudBank::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Puffs.IsEmpty())
	{
		Redraw(FindViewer());
	}
}

FVector ASceneCloudBank::FindViewer() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->PlayerCameraManager)
	{
		return Controller->PlayerCameraManager->GetCameraLocation();
	}
	// No player (a test level): seen from well in front, as the skiff first sees it.
	return GetActorLocation() - GetActorForwardVector() * 6000.0;
}

void ASceneCloudBank::Redraw(const FVector& Viewer)
{
	const FTransform Bank = GetActorTransform();
	const float Brightness = FMath::Max(CVarCloudBrightness.GetValueOnGameThread(), 0.f);

	// Back to front: the instances draw in order, and a translucent puff only blends right over what is behind it.
	struct FPlaced
	{
		FVector Location;
		float Distance;
		int32 Index;
	};
	TArray<FPlaced, TInlineAllocator<16>> Placed;
	for (int32 Index = 0; Index < Puffs.Num(); ++Index)
	{
		const FVector Location = Bank.TransformPosition(Puffs[Index].Offset);
		Placed.Add(FPlaced{ Location, static_cast<float>(FVector::Distance(Location, Viewer)), Index });
	}
	Placed.Sort([](const FPlaced& A, const FPlaced& B) { return A.Distance > B.Distance; });

	TArray<FTransform> Transforms;
	TArray<float> Data;
	Transforms.Reserve(Placed.Num());
	Data.Reserve(Placed.Num() * PuffFloats);
	for (const FPlaced& Each : Placed)
	{
		const FPuff& Puff = Puffs[Each.Index];
		FVector Facing = Viewer - Each.Location;
		if (!Facing.Normalize())
		{
			Facing = -Bank.GetRotation().GetForwardVector();
		}
		// The engine's quad lies in XY facing +Z, 100 across: turned to face the camera, then about its own middle.
		const FQuat Rotation = FRotationMatrix::MakeFromZ(Facing).ToQuat() * FQuat(FVector::UpVector, Puff.Roll);
		Transforms.Add(FTransform(Rotation, Each.Location, FVector(Puff.Size / 100.f, Puff.Size / 100.f, 1.f)));
		const FLinearColor Tint = (ShadeColor + (LitColor - ShadeColor) * Puff.Height) * Brightness;
		// Thin as the camera comes close, so passing through one never pops.
		const float Opacity = PuffOpacity * FMath::SmoothStep(NearGone, NearFull, Each.Distance);
		Data.Append({ Tint.R, Tint.G, Tint.B, Opacity });
	}

	// Moved in place while the count holds (it always does after the first draw), as FWeaponFX draws its puffs.
	if (Cards->GetInstanceCount() != Transforms.Num())
	{
		Cards->ClearInstances();
		Cards->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
	}
	else if (!Transforms.IsEmpty())
	{
		Cards->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
	}
	if (!Transforms.IsEmpty())
	{
		Cards->SetCustomData(0, Transforms.Num() - 1, Data, false);
	}
	Cards->MarkRenderStateDirty();
}
