// AChapelReliquary: the smashed Reliquary (or its stand-in), being looked at, and the ember Grave Sight sees lift off it.

#include "World/ChapelReliquary.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/GraveSightSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AChapelReliquary::ReliquaryTag(TEXT("Reliquary_Chapel"));
const FName AChapelReliquary::SightEvent(TEXT("GraveSight.Reliquary"));

namespace
{
	/** The stand-in's shapes and looks: the engine's cube (100 cm a side), granite, worn brass. */
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* GranitePath = TEXT("/Game/Art/Materials/MI_RockGranite.MI_RockGranite");
	const TCHAR* BrassPath = TEXT("/Game/Art/Materials/MI_BrassWorn.MI_BrassWorn");

	/** The ember: the game's additive glow on the engine's quad (R, G, B, strength, shape 1 = round), as the cold open draws it. */
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* QuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;

	/** Saint Ada's ember: the cold open's burning orange, its size, strength and light (cm, candelas). */
	const FLinearColor EmberColor(1.f, 0.42f, 0.08f);
	constexpr float EmberSize = 22.f;
	constexpr float EmberStrength = 32.f;
	constexpr float EmberCandelas = 14.f;
	constexpr float EmberLightRadius = 380.f;

	/** It wavers a little as it climbs, as a flame does in still air (cm either way, and how fast). */
	constexpr float EmberWaver = 5.f;
	constexpr float EmberWaverSpeed = 7.f;

	/**
	 * The stand-in: a granite chest about the backlog Reliquary's size on the plinth's 1.56 x 1.02 m top, its lid knocked
	 * askew, a brass band round its middle (cm, its foot at the actor's origin).
	 */
	constexpr double ChestLength = 120.0;
	constexpr double ChestWidth = 74.0;
	constexpr double ChestHeight = 56.0;
	constexpr double LidThickness = 10.0;
	constexpr double BandHeight = 6.0;
	constexpr double BandProud = 2.0;

	/** Over the top of whatever it lifts from, the ember starts this far up (cm). */
	constexpr double EmberAbove = 8.0;

	/** An asset found only once it's in this checkout, so the class works, and its tests run, without it. In a constructor. */
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

AChapelReliquary::AChapelReliquary()
{
	// Ticks only while its flash plays.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// Solid like any prop: the player bumps it, and the Interact key's line finds it (its hulls block Visibility).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// The stand-in, for a checkout without the model: shown, and solid, only while the Mesh has none.
	static UStaticMesh* const Cube = FindIfMade<UStaticMesh>(CubePath);
	static UMaterialInterface* const Granite = FindIfMade<UMaterialInterface>(GranitePath);
	static UMaterialInterface* const Brass = FindIfMade<UMaterialInterface>(BrassPath);
	auto MakePart = [this](const TCHAR* Name, UMaterialInterface* Skin, const FVector& Middle, const FRotator& Turn, const FVector& Size)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(Cube);
		Part->SetRelativeLocationAndRotation(Middle, Turn);
		Part->SetRelativeScale3D(Size / 100.0);
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		if (Skin)
		{
			Part->SetMaterial(0, Skin);
		}
		StandIn.Add(Part);
	};
	MakePart(TEXT("StandInChest"), Granite, FVector(0.0, 0.0, ChestHeight * 0.5), FRotator::ZeroRotator,
		FVector(ChestLength, ChestWidth, ChestHeight));
	// The lid, cracked loose and shoved aside: turned a little, tipped toward its fallen edge.
	MakePart(TEXT("StandInLid"), Granite, FVector(10.0, 7.0, ChestHeight + LidThickness * 0.5 + 2.0), FRotator(0.0, 9.0, 4.0),
		FVector(ChestLength + 8.0, ChestWidth + 6.0, LidThickness));
	MakePart(TEXT("StandInBand"), Brass, FVector(0.0, 0.0, ChestHeight * 0.7), FRotator::ZeroRotator,
		FVector(ChestLength + BandProud, ChestWidth + BandProud, BandHeight));

	// The ember: hidden until a flash, with nothing to bump into or shadow.
	static UStaticMesh* const Quad = FindIfMade<UStaticMesh>(QuadPath);
	static UMaterialInterface* const Glow = FindIfMade<UMaterialInterface>(GlowMaterialPath);
	EmberGlow = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EmberGlow"));
	EmberGlow->SetupAttachment(Root);
	EmberGlow->SetMobility(EComponentMobility::Movable);
	EmberGlow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EmberGlow->SetCastShadow(false);
	EmberGlow->bReceivesDecals = false;
	EmberGlow->SetStaticMesh(Quad);
	if (Glow)
	{
		EmberGlow->SetMaterial(0, Glow);
	}
	EmberGlow->SetVisibility(false);

	// A little light of its own on the apse's walls; lights a few metres, shadows nothing.
	EmberLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmberLight"));
	EmberLight->SetupAttachment(Root);
	EmberLight->SetMobility(EComponentMobility::Movable);
	EmberLight->SetIntensityUnits(ELightUnits::Candelas);
	EmberLight->SetIntensity(0.f);
	EmberLight->SetLightColor(EmberColor);
	EmberLight->SetAttenuationRadius(EmberLightRadius);
	EmberLight->SetCastShadows(false);
	EmberLight->SetVisibility(false);

	Prompt = NSLOCTEXT("LooterChapel", "LookPrompt", "Look at the Reliquary");
	Tags.Add(ReliquaryTag);
}

void AChapelReliquary::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Placed or edited: the stand-in only while it has no model, so the editor shows what play will.
	RefreshStandIn();
}

void AChapelReliquary::BeginPlay()
{
	Super::BeginPlay();
	RefreshStandIn();
	// The ember's quad, made now so nothing of it is kept with the level.
	if (EmberGlow)
	{
		EmberGlow->ClearInstances();
		EmberGlow->SetNumCustomDataFloats(GlowFloats);
		EmberGlow->AddInstance(FTransform(FQuat::Identity, GetEmberStart(), FVector(0.01)), /*bWorldSpace*/ true);
	}
	EmberAt = GetEmberStart();
	ShowEmber(false);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AChapelReliquary::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AChapelReliquary::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

bool AChapelReliquary::HasModel() const
{
	return Mesh && Mesh->GetStaticMesh() != nullptr;
}

void AChapelReliquary::RefreshStandIn()
{
	const bool bStandIn = !HasModel();
	for (UStaticMeshComponent* Part : StandIn)
	{
		if (Part)
		{
			Part->SetVisibility(bStandIn);
			Part->SetCollisionEnabled(bStandIn ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
}

// ---------------------------------------------------------------------------
// Being looked at
// ---------------------------------------------------------------------------

FInteractionOptions AChapelReliquary::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// Not before the story lets it be seen, nor while the sight still plays.
	Options.bUsable = CanLook();
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AChapelReliquary::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Look(User.GetOwner());
}

TOptional<FVector> AChapelReliquary::GetInteractionLocation() const
{
	// The model's own anchor at its front when it has one; else about the middle of what shows.
	if (HasModel())
	{
		return TOptional<FVector>(Mesh->DoesSocketExist(InteractSocket) ? Mesh->GetSocketLocation(InteractSocket) : Mesh->Bounds.Origin);
	}
	return TOptional<FVector>(GetActorTransform().TransformPosition(FVector(0.0, 0.0, ChestHeight * 0.5)));
}

bool AChapelReliquary::CanLook() const
{
	if (Flash.IsPlaying())
	{
		return false;
	}
	if (LookWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && LookWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AChapelReliquary::Look(AActor* ByWhom, bool bForce)
{
	if (Flash.IsPlaying() || (!bForce && !CanLook()))
	{
		return false;
	}
	Flash.Start(FlashSeconds);
	ShowEmber(true);
	PlaceEmber();
	SetActorTickEnabled(true);
	// The screen's side of it: the cyan overlay, for as long as the ember is seen.
	if (UGraveSightSubsystem* Sight = UGraveSightSubsystem::Get(this))
	{
		Sight->Flash(FlashSeconds);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: Grave Sight on the Reliquary%s: her ember lifts off the lid."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" (%s)"), *ByWhom->GetName()) : TEXT(""));
	return true;
}

// ---------------------------------------------------------------------------
// The ember
// ---------------------------------------------------------------------------

FVector AChapelReliquary::GetEmberStart() const
{
	// In the lid's empty setting, where the saint's light floated before the gang came.
	if (HasModel())
	{
		if (Mesh->DoesSocketExist(EmberSocket))
		{
			return Mesh->GetSocketLocation(EmberSocket);
		}
		const FBoxSphereBounds& Shape = Mesh->Bounds;
		return FVector(Shape.Origin.X, Shape.Origin.Y, Shape.Origin.Z + Shape.BoxExtent.Z + EmberAbove);
	}
	return GetActorTransform().TransformPosition(FVector(0.0, 0.0, ChestHeight + LidThickness + EmberAbove));
}

void AChapelReliquary::Advance(float DeltaSeconds)
{
	if (!Flash.IsPlaying())
	{
		return;
	}
	const bool bEnded = Flash.Advance(DeltaSeconds);
	PlaceEmber();
	if (bEnded)
	{
		FinishLook();
	}
}

void AChapelReliquary::PlaceEmber()
{
	// Straight up off the lid, wavering a little as it climbs.
	const float Rise = Flash.GetEmberRise();
	const float Time = Flash.GetElapsed();
	const FVector Waver = GetActorRightVector() * (EmberWaver * FMath::Sin(Time * EmberWaverSpeed))
		+ GetActorForwardVector() * (EmberWaver * 0.5f * FMath::Sin(Time * EmberWaverSpeed * 0.6f));
	EmberAt = GetEmberStart() + FVector(0.0, 0.0, EmberRise * Rise) + Waver * Rise;
	const float Strength = Flash.GetEmberGlow();

	if (EmberGlow && EmberGlow->GetInstanceCount() > 0)
	{
		// Facing whoever looks, as the cold open's glows do (along +X without a camera: a test level).
		const UWorld* World = GetWorld();
		const APlayerController* Viewer = World ? World->GetFirstPlayerController() : nullptr;
		const FVector Camera = Viewer && Viewer->PlayerCameraManager ? Viewer->PlayerCameraManager->GetCameraLocation()
			: EmberAt + FVector(500.0, 0.0, 0.0);
		FVector ToCamera = Camera - EmberAt;
		if (!ToCamera.Normalize())
		{
			ToCamera = FVector::UpVector;
		}
		// The engine's quad lies in XY facing +Z, 100 across.
		const double Size = Strength > 0.f ? EmberSize : 0.01;
		EmberGlow->UpdateInstanceTransform(0, FTransform(FRotationMatrix::MakeFromZ(ToCamera).ToQuat(), EmberAt,
			FVector(Size / 100.0, Size / 100.0, 1.0)), /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		const float Data[] = { EmberColor.R, EmberColor.G, EmberColor.B, EmberStrength * Strength, 1.f };
		EmberGlow->SetCustomData(0, MakeArrayView(Data), /*bMarkRenderStateDirty*/ false);
		EmberGlow->MarkRenderStateDirty();
	}
	if (EmberLight)
	{
		EmberLight->SetWorldLocation(EmberAt);
		EmberLight->SetIntensity(EmberCandelas * Strength);
	}
}

void AChapelReliquary::ShowEmber(bool bShown)
{
	if (EmberGlow)
	{
		EmberGlow->SetVisibility(bShown);
	}
	if (EmberLight)
	{
		EmberLight->SetVisibility(bShown);
		if (!bShown)
		{
			EmberLight->SetIntensity(0.f);
		}
	}
}

void AChapelReliquary::FinishLook()
{
	// Gone off over the lid: nothing of her is left here.
	ShowEmber(false);
	SetActorTickEnabled(false);
	++Looks;
	UE_LOG(LogLooter, Log, TEXT("%s: the sight fades; the Reliquary is dark."), *GetActorNameOrLabel());
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(SightEvent, this));
	}
}
