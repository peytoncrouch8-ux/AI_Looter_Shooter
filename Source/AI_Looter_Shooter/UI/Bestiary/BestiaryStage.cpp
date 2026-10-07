#include "UI/Bestiary/BestiaryStage.h"
#include "Bestiary/BestiaryEntry.h"
#include "UI/Inventory/StageStudio.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "AnimationRuntime.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The studio lights for a model about two meters across (the loadout's stand-in); bigger and smaller models scale them. */
	struct FStudioLight
	{
		FVector Location;
		float Candelas;
	};
	const FStudioLight KeyLightSetup{ FVector(-300.f, -220.f, 280.f), 60.f };
	const FStudioLight FillLightSetup{ FVector(-260.f, 260.f, 110.f), 14.f };
	const FStudioLight RimLightSetup{ FVector(180.f, 120.f, 230.f), 90.f };
	constexpr float LightReach = 1200.f;
	constexpr float ReferenceRadius = 100.f;

	/** Seconds the picture keeps rendering after the model or the turn changes, while textures stream in. */
	constexpr float SettleSeconds = 1.f;
}

ABestiaryStage::ABestiaryStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// After the model has animated, so the picture shows this frame's pose.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Turntable = CreateDefaultSubobject<USceneComponent>(TEXT("Turntable"));
	Turntable->SetupAttachment(Root);

	Model = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Model"));
	Model->SetupAttachment(Turntable);
	// Only the stage's camera ever sees it, so it must animate whether or not the main view renders it.
	Model->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Model->PrimaryComponentTick.bStartWithTickEnabled = false;
	Model->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	StageStudio::SetupPrimitive(Model);

	StillModel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StillModel"));
	StillModel->SetupAttachment(Turntable);
	StillModel->SetVisibility(false);
	StageStudio::SetupPrimitive(StillModel);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	StageStudio::SetupCapture(Capture);

	// The loadout's studio: a warm key from the front left, a cool fill from the right and a cyan rim from behind.
	KeyLight = StageStudio::MakeLight(this, Root, TEXT("KeyLight"), KeyLightSetup.Location, FLinearColor(1.f, 0.93f, 0.84f), KeyLightSetup.Candelas, true);
	FillLight = StageStudio::MakeLight(this, Root, TEXT("FillLight"), FillLightSetup.Location, FLinearColor(0.72f, 0.84f, 1.f), FillLightSetup.Candelas, false);
	RimLight = StageStudio::MakeLight(this, Root, TEXT("RimLight"), RimLightSetup.Location, FLinearColor(0.45f, 0.8f, 1.f), RimLightSetup.Candelas, false);
}

void ABestiaryStage::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(StageStudio::Location(1));
	RenderTarget = StageStudio::MakeRenderTarget(this, TEXT("BestiaryStageImage"), ImageWidth, ImageHeight);
	Capture->TextureTarget = RenderTarget;
	Capture->ShowOnlyActors.Add(this);
	PlaceCamera();
}

void ABestiaryStage::ShowEntry(const UBestiaryEntry* Entry)
{
	USkeletalMesh* Mesh = Entry ? Entry->LoadPreviewMesh() : nullptr;
	if (Model->GetSkeletalMeshAsset() != Mesh)
	{
		Model->SetSkeletalMeshAsset(Mesh);
	}
	// Dressed as the actor is in the world.
	Model->EmptyOverrideMaterials();
	if (Entry)
	{
		const TArray<UMaterialInterface*> Materials = Entry->GetPreviewMaterials(Mesh);
		for (int32 Slot = 0; Slot < Materials.Num(); ++Slot)
		{
			Model->SetMaterial(Slot, Materials[Slot]);
		}
	}
	Model->SetVisibility(Mesh != nullptr);
	if (UAnimationAsset* Animation = Entry ? Entry->PreviewAnimation.LoadSynchronous() : nullptr)
	{
		Model->PlayAnimation(Animation, true);
	}
	else
	{
		// No animation: the model stands in the pose it was modeled in.
		Model->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Model->SetAnimation(nullptr);
	}

	// A figure that isn't skinned (Sexton seated on his rail) stands as a still model instead.
	UStaticMesh* Still = !Mesh && Entry ? Entry->LoadPreviewStaticMesh() : nullptr;
	if (StillModel->GetStaticMesh() != Still)
	{
		StillModel->SetStaticMesh(Still);
	}
	StillModel->SetVisibility(Still != nullptr);

	FBox Box = Mesh ? Mesh->GetBounds().GetBox() : (Still ? Still->GetBounds().GetBox() : FBox(ForceInit));
	ShowParts(Entry, Mesh, Box);
	ShowStillParts(Entry, Still, Box);
	if (Mesh || Still)
	{
		// Stand it on the floor, centered over the turntable so it turns on the spot, its front along the turntable's +X.
		const FRotator Facing(0.f, Entry->PreviewYaw, 0.f);
		const FVector Center = Facing.RotateVector(Box.GetCenter());
		USceneComponent* Shown = Mesh ? static_cast<USceneComponent*>(Model) : static_cast<USceneComponent*>(StillModel);
		Shown->SetRelativeLocationAndRotation(FVector(-Center.X, -Center.Y, -Box.Min.Z), Facing);

		const FVector Extent = Box.GetExtent();
		SubjectReach = FMath::Max(FMath::Max(Extent.X, Extent.Y), 10.f);
		SubjectHeight = FMath::Max(Extent.Z, 10.f);
		RingRadius = FMath::Max(SubjectReach * 0.8f, 30.f);
		if (Still)
		{
			// A still model stands on a base of its own (Sexton's length of rail on its deck boards): the ring takes in its
			// corners, so the boards don't poke out past it.
			const FBox Base = Still->GetBounds().GetBox();
			const FVector2D Middle(Box.GetCenter().X, Box.GetCenter().Y);
			for (const FVector2D Corner : { FVector2D(Base.Min.X, Base.Min.Y), FVector2D(Base.Min.X, Base.Max.Y),
				FVector2D(Base.Max.X, Base.Min.Y), FVector2D(Base.Max.X, Base.Max.Y) })
			{
				RingRadius = FMath::Max(RingRadius, static_cast<float>(FVector2D::Distance(Corner, Middle)) * 1.04f);
			}
		}
	}
	Turn = DefaultTurn;
	PlaceCamera();
}

void ABestiaryStage::ShowParts(const UBestiaryEntry* Entry, const USkeletalMesh* Mesh, FBox& InOutBox)
{
	// Worn as in the world (the Unpaid's hat on its hat bone), and framed with the body: each part's box where it sits on
	// the body at rest.
	const TArray<FBestiaryStandPart> Worn = Entry && Mesh ? Entry->GetPreviewParts(Mesh) : TArray<FBestiaryStandPart>();
	for (int32 Index = 0; Index < FMath::Max(Worn.Num(), Parts.Num()); ++Index)
	{
		if (!Worn.IsValidIndex(Index))
		{
			Parts[Index]->SetStaticMesh(nullptr);
			Parts[Index]->SetVisibility(false);
			continue;
		}
		const FBestiaryStandPart& Part = Worn[Index];
		if (!Parts.IsValidIndex(Index))
		{
			UStaticMeshComponent* Added = NewObject<UStaticMeshComponent>(this);
			Added->SetupAttachment(Model, Part.Bone);
			Added->RegisterComponent();
			StageStudio::SetupPrimitive(Added);
			Parts.Add(Added);
		}
		UStaticMeshComponent* Shown = Parts[Index];
		Shown->AttachToComponent(Model, FAttachmentTransformRules::KeepRelativeTransform, Part.Bone);
		Shown->SetRelativeTransform(Part.Relative);
		Shown->SetStaticMesh(Part.Mesh);
		Shown->EmptyOverrideMaterials();
		for (int32 Slot = 0; Slot < Part.Materials.Num(); ++Slot)
		{
			Shown->SetMaterial(Slot, Part.Materials[Slot]);
		}
		Shown->SetVisibility(true);
		const int32 Bone = Mesh->GetRefSkeleton().FindBoneIndex(Part.Bone);
		const FTransform OnBody = Part.Relative * FAnimationRuntime::GetComponentSpaceTransformRefPose(Mesh->GetRefSkeleton(), Bone);
		InOutBox += Part.Mesh->GetBounds().GetBox().TransformBy(OnBody);
	}
}

void ABestiaryStage::ShowStillParts(const UBestiaryEntry* Entry, const UStaticMesh* Still, FBox& InOutBox)
{
	// On the still model's sockets, snapped (Sexton on his rail's Sit socket, his ledger on its Ledger socket), and
	// framed with it: each part's box where its socket puts it. A part whose mesh or socket is missing is left off.
	TArray<TPair<UStaticMesh*, const UStaticMeshSocket*>> Wanted;
	if (Entry && Still)
	{
		for (const FBestiaryStillPart& Part : Entry->PreviewStillParts)
		{
			UStaticMesh* PartMesh = Part.Mesh.LoadSynchronous();
			const UStaticMeshSocket* Socket = PartMesh ? Still->FindSocket(Part.Socket) : nullptr;
			if (Socket)
			{
				Wanted.Emplace(PartMesh, Socket);
			}
		}
	}
	for (int32 Index = 0; Index < FMath::Max(Wanted.Num(), StillParts.Num()); ++Index)
	{
		if (!Wanted.IsValidIndex(Index))
		{
			StillParts[Index]->SetStaticMesh(nullptr);
			StillParts[Index]->SetVisibility(false);
			continue;
		}
		if (!StillParts.IsValidIndex(Index))
		{
			UStaticMeshComponent* Added = NewObject<UStaticMeshComponent>(this);
			Added->SetupAttachment(StillModel);
			Added->RegisterComponent();
			StageStudio::SetupPrimitive(Added);
			StillParts.Add(Added);
		}
		UStaticMesh* PartMesh = Wanted[Index].Key;
		const UStaticMeshSocket* Socket = Wanted[Index].Value;
		UStaticMeshComponent* Shown = StillParts[Index];
		Shown->AttachToComponent(StillModel, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket->SocketName);
		Shown->SetStaticMesh(PartMesh);
		Shown->SetVisibility(true);
		const FTransform OnBase(Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale);
		InOutBox += PartMesh->GetBounds().GetBox().TransformBy(OnBase);
	}
}

bool ABestiaryStage::HasModel() const
{
	return Model->GetSkeletalMeshAsset() != nullptr || StillModel->GetStaticMesh() != nullptr;
}

void ABestiaryStage::SetActive(bool bInActive)
{
	if (bActive == bInActive)
	{
		return;
	}
	bActive = bInActive;
	SetActorTickEnabled(bActive);
	Model->SetComponentTickEnabled(bActive);
	for (UPointLightComponent* Light : { KeyLight.Get(), FillLight.Get(), RimLight.Get() })
	{
		Light->SetVisibility(bActive);
	}
	if (bActive)
	{
		PlaceCamera();
	}
}

void ABestiaryStage::AddTurn(float Degrees)
{
	Turn = FRotator::NormalizeAxis(Turn + Degrees);
	PlaceCamera();
}

void ABestiaryStage::PlaceCamera()
{
	// The camera backs off until the model fits, whatever its size: its widest reach across (the same at any turn, so
	// turning it never pushes anything out of the picture) and its height up and down, each filling Fill of the picture.
	// Its near side is closer to the camera than its middle and looks bigger, so the camera keeps half its reach more.
	const float HalfWide = FMath::DegreesToRadians(FieldOfView * 0.5f);
	const float HalfHigh = FMath::Atan(FMath::Tan(HalfWide) * ImageHeight / ImageWidth);
	const float FitAcross = SubjectReach / FMath::Tan(HalfWide);
	const float FitUp = SubjectHeight / FMath::Tan(HalfHigh);
	const float Distance = FMath::Max(FitAcross, FitUp) / FMath::Max(Fill, 0.1f) + SubjectReach * 0.5f;
	const FRotator View(CameraPitch, 0.f, 0.f);
	Capture->SetRelativeLocationAndRotation(FVector(0.f, 0.f, SubjectHeight) - View.Vector() * Distance, View);
	Capture->FOVAngle = FieldOfView;
	// Turn 0 faces the camera.
	Turntable->SetRelativeRotation(FRotator(0.f, 180.f + Turn, 0.f));

	// The lights keep their places around the model as it grows: farther away and brighter by the square of the distance.
	const float Scale = FMath::Max(FMath::Max(SubjectReach, SubjectHeight) / ReferenceRadius, 0.5f);
	const TPair<UPointLightComponent*, const FStudioLight*> Lights[] = {
		{ KeyLight.Get(), &KeyLightSetup }, { FillLight.Get(), &FillLightSetup }, { RimLight.Get(), &RimLightSetup } };
	for (const TPair<UPointLightComponent*, const FStudioLight*>& Light : Lights)
	{
		Light.Key->SetRelativeLocation(Light.Value->Location * Scale);
		Light.Key->SetAttenuationRadius(LightReach * Scale);
		Light.Key->SetIntensity(Light.Value->Candelas * Scale * Scale);
	}
	SettleTime = SettleSeconds;
}

void ABestiaryStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// A still model needs no new picture every frame: only while it animates, or for a moment after a change.
	const bool bAnimating = Model->GetSingleNodeInstance() && Model->GetSingleNodeInstance()->GetAnimationAsset();
	if (bAnimating || SettleTime > 0.f)
	{
		Capture->CaptureSceneDeferred();
	}
	SettleTime -= DeltaSeconds;
}

bool ABestiaryStage::ProjectToImage(const FVector& WorldLocation, FVector2D& OutUV) const
{
	return StageStudio::ProjectToImage(Capture, ImageWidth, ImageHeight, WorldLocation, OutUV);
}

FVector ABestiaryStage::GetFloorCenter() const
{
	return Turntable->GetComponentLocation();
}

FVector ABestiaryStage::GetTowardCamera() const
{
	const FVector Toward = (Capture->GetComponentLocation() - GetFloorCenter()).GetSafeNormal2D();
	return Toward.IsNearlyZero() ? FVector::BackwardVector : Toward;
}
