// AChapelReliquary: Saint Ada's whole Reliquary (or its stand-in), her guttering ember, being looked at, and its flare.

#include "World/ChapelReliquary.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
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
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AChapelReliquary::ReliquaryTag(TEXT("Reliquary_Chapel"));
const FName AChapelReliquary::SightEvent(TEXT("GraveSight.Reliquary"));

namespace
{
	/** The stand-in's shapes and looks: the engine's cube (100 cm a side), granite, worn brass, the recess's black. */
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* GranitePath = TEXT("/Game/Art/Materials/MI_RockGranite.MI_RockGranite");
	const TCHAR* BrassPath = TEXT("/Game/Art/Materials/MI_BrassWorn.MI_BrassWorn");
	const TCHAR* RecessPath = TEXT("/Game/Art/Materials/MI_PaintBlack.MI_PaintBlack");

	/** The ember: the game's additive glow on the engine's quad (R, G, B, strength, shape 1 = round), as the cold open draws it. */
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* QuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;

	/** Saint Ada's ember: the cold open's burning orange. Guttering: small, dim, a faint light (cm, strength, candelas). */
	const FLinearColor EmberColor(1.f, 0.42f, 0.08f);
	constexpr float GutterSize = 11.f;
	constexpr float GutterStrength = 7.f;
	constexpr float GutterCandelas = 2.5f;
	/** The flare at the flash's height: this much bigger and brighter. */
	constexpr float FlareSize = 26.f;
	constexpr float FlareStrength = 34.f;
	constexpr float FlareCandelas = 16.f;
	/** Lit (her keeper home): big, bright and steady. */
	constexpr float LitSize = 30.f;
	constexpr float LitStrength = 40.f;
	constexpr float LitCandelas = 22.f;
	constexpr float EmberLightRadius = 420.f;
	/** A hose drinking her: the gutter this share of itself. */
	constexpr float DrinkingShare = 0.4f;
	/**
	 * Her waking's blaze (Main 7, SaintAdaWakes; lane MAIN67): white-gold, three times her lit size, a light filling the apse
	 * and the nave (still shadowless). Lit after, her ember keeps a share of that warmth.
	 */
	const FLinearColor BlazeColor(1.f, 0.9f, 0.68f);
	constexpr float BlazeSize = 90.f;
	constexpr float BlazeStrength = 70.f;
	constexpr float BlazeCandelas = 90.f;
	constexpr float BlazeLightRadius = 1600.f;
	constexpr float LitWarmth = 0.45f;

	/** The gutter is drawn and lit only with the player this near (cm); farther, it ticks slowly to notice them coming. */
	constexpr double GutterReach = 4000.0;
	constexpr float FarTickSeconds = 0.5f;

	/**
	 * The stand-in: a granite chest about the backlog Reliquary's size on the plinth's 1.56 x 1.02 m top, its lid shut, a
	 * brass band round its middle, a dark round-ish recess on top for her ember (cm, its foot at the actor's origin).
	 */
	constexpr double ChestLength = 120.0;
	constexpr double ChestWidth = 74.0;
	constexpr double ChestHeight = 56.0;
	constexpr double LidThickness = 10.0;
	constexpr double BandHeight = 6.0;
	constexpr double BandProud = 2.0;
	constexpr double RecessSize = 20.0;
	constexpr double RecessDepth = 1.0;

	/** Over the recess (or the model's top), the ember lies this far up (cm). */
	constexpr double EmberAbove = 3.0;

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
	// Ticks while the player is near (the gutter) or its flash plays; slowly otherwise.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = FarTickSeconds;

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
	static UMaterialInterface* const Recess = FindIfMade<UMaterialInterface>(RecessPath);
	auto MakePart = [this](const TCHAR* Name, UMaterialInterface* Skin, const FVector& Middle, const FVector& Size, bool bSolid)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Root);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetStaticMesh(Cube);
		Part->SetRelativeLocation(Middle);
		Part->SetRelativeScale3D(Size / 100.0);
		Part->SetCollisionProfileName(bSolid ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		if (Skin)
		{
			Part->SetMaterial(0, Skin);
		}
		StandIn.Add(Part);
	};
	MakePart(TEXT("StandInChest"), Granite, FVector(0.0, 0.0, ChestHeight * 0.5), FVector(ChestLength, ChestWidth, ChestHeight), true);
	// Its lid, whole and shut, a little proud of the chest all round.
	MakePart(TEXT("StandInLid"), Granite, FVector(0.0, 0.0, ChestHeight + LidThickness * 0.5),
		FVector(ChestLength + 8.0, ChestWidth + 6.0, LidThickness), true);
	MakePart(TEXT("StandInBand"), Brass, FVector(0.0, 0.0, ChestHeight * 0.7),
		FVector(ChestLength + BandProud, ChestWidth + BandProud, BandHeight), true);
	// The lid's recess, where her ember lies: a dark plate just proud of the lid, a gold rim round it.
	MakePart(TEXT("StandInRim"), Brass, FVector(0.0, 0.0, ChestHeight + LidThickness + RecessDepth * 0.5),
		FVector(RecessSize + 6.0, RecessSize + 6.0, RecessDepth), false);
	MakePart(TEXT("StandInRecess"), Recess, FVector(0.0, 0.0, ChestHeight + LidThickness + RecessDepth * 1.2),
		FVector(RecessSize, RecessSize, RecessDepth), false);

	// The ember: nothing to bump into or shadow.
	static UStaticMesh* const Quad = FindIfMade<UStaticMesh>(QuadPath);
	static UMaterialInterface* const Glow = FindIfMade<UMaterialInterface>(GlowMaterialPath);
	EmberQuad = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("EmberGlow"));
	EmberQuad->SetupAttachment(Root);
	EmberQuad->SetMobility(EComponentMobility::Movable);
	EmberQuad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EmberQuad->SetCastShadow(false);
	EmberQuad->bReceivesDecals = false;
	EmberQuad->SetStaticMesh(Quad);
	if (Glow)
	{
		EmberQuad->SetMaterial(0, Glow);
	}
	EmberQuad->SetVisibility(false);

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
	if (EmberQuad)
	{
		EmberQuad->ClearInstances();
		EmberQuad->SetNumCustomDataFloats(GlowFloats);
		EmberQuad->AddInstance(FTransform(FQuat::Identity, GetEmberStart(), FVector(0.01)), /*bWorldSpace*/ true);
	}
	EmberAt = GetEmberStart();
	GutterClock = FMath::FRandRange(0.f, 10.f);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	PlaceEmber();
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
	// Drawn and lit only with the player near (an apse's light seen from the yard through the windows); a quick tick then.
	const bool bNear = Flash.IsPlaying() || IsPlayerNear(GutterReach);
	SetActorTickInterval(bNear ? 0.f : FarTickSeconds);
	if (EmberQuad)
	{
		EmberQuad->SetVisibility(bNear);
	}
	if (EmberLight)
	{
		EmberLight->SetVisibility(bNear);
	}
	if (bNear)
	{
		Advance(DeltaSeconds);
	}
}

bool AChapelReliquary::HasModel() const
{
	return Mesh && Mesh->GetStaticMesh() != nullptr;
}

void AChapelReliquary::RefreshStandIn()
{
	const bool bStandIn = !HasModel();
	for (int32 Index = 0; Index < StandIn.Num(); ++Index)
	{
		UStaticMeshComponent* Part = StandIn[Index];
		if (Part)
		{
			// The chest, its lid and its band are solid; the recess and its rim (the last two) only seen.
			const bool bSolid = Index < 3;
			Part->SetVisibility(bStandIn);
			Part->SetCollisionEnabled(bStandIn && bSolid ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
}

bool AChapelReliquary::IsPlayerNear(double Within) const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	return Player && FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) <= Within * Within;
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

bool AChapelReliquary::IsLit() const
{
	if (bLitByScene)
	{
		return true;
	}
	if (LitWhen.IsEmpty())
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && LitWhen.IsMet(Runner->GetCampaign(), Runner);
}

void AChapelReliquary::SetLitNow(bool bInLit)
{
	bLitByScene = bInLit;
	SetActorTickInterval(0.f);
	PlaceEmber();
}

void AChapelReliquary::SetBlaze(float Share)
{
	Blaze = FMath::Clamp(Share, 0.f, 1.f);
	SetActorTickInterval(0.f);
	PlaceEmber();
}

bool AChapelReliquary::Look(AActor* ByWhom, bool bForce)
{
	if (Flash.IsPlaying() || (!bForce && !CanLook()))
	{
		return false;
	}
	Flash.Start(FlashSeconds);
	SetActorTickInterval(0.f);
	PlaceEmber();
	// The screen's side of it: the cyan overlay, for as long as the flare is seen.
	if (UGraveSightSubsystem* Sight = UGraveSightSubsystem::Get(this))
	{
		Sight->Flash(FlashSeconds);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: Grave Sight on the Reliquary%s: her ember flares under the ash."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" (%s)"), *ByWhom->GetName()) : TEXT(""));
	return true;
}

// ---------------------------------------------------------------------------
// The ember
// ---------------------------------------------------------------------------

float AChapelReliquary::GutterAt(float Time, bool bInDrinking)
{
	// Two slow, uneven breaths over each other and a quick flicker: never out, never steady (0.45 to 1).
	const float Speed = bInDrinking ? 2.2f : 1.f;
	const float T = Time * Speed;
	const float Breath = 0.5f + 0.25f * FMath::Sin(T * 1.7f) + 0.15f * FMath::Sin(T * 2.9f + 1.3f);
	const float Flicker = 0.1f * FMath::Sin(T * 13.f + FMath::Sin(T * 3.1f) * 2.f);
	return FMath::Clamp(Breath + Flicker, 0.45f, 1.f);
}

FVector AChapelReliquary::GetEmberStart() const
{
	// In the lid's recess, where her light lies.
	if (HasModel())
	{
		if (Mesh->DoesSocketExist(EmberSocket))
		{
			return Mesh->GetSocketLocation(EmberSocket) + FVector(0.0, 0.0, EmberAbove);
		}
		const FBoxSphereBounds& Shape = Mesh->Bounds;
		return FVector(Shape.Origin.X, Shape.Origin.Y, Shape.Origin.Z + Shape.BoxExtent.Z + EmberAbove);
	}
	return GetActorTransform().TransformPosition(FVector(0.0, 0.0, ChestHeight + LidThickness + RecessDepth * 2.0 + EmberAbove));
}

void AChapelReliquary::Advance(float DeltaSeconds)
{
	GutterClock += DeltaSeconds;
	bool bEnded = false;
	if (Flash.IsPlaying())
	{
		bEnded = Flash.Advance(DeltaSeconds);
	}
	PlaceEmber();
	if (bEnded)
	{
		FinishLook();
	}
}

void AChapelReliquary::PlaceEmber()
{
	// Her gutter (lower while drunk), the flare over it while the flash plays, or her full light once lit.
	const float Gutter = GutterAt(GutterClock, bDrinking) * (bDrinking ? DrinkingShare : 1.f);
	const float Flare = Flash.GetEmberGlow();
	float Size = GutterSize;
	float Strength = GutterStrength * Gutter;
	float Candelas = GutterCandelas * Gutter;
	if (IsLit())
	{
		Size = LitSize;
		Strength = LitStrength * (0.92f + 0.08f * GutterAt(GutterClock * 0.5f, false));
		Candelas = LitCandelas;
	}
	else if (Flare > 0.f)
	{
		Size = FMath::Lerp(GutterSize, FlareSize, Flare);
		Strength = FMath::Lerp(Strength, FlareStrength, Flare);
		Candelas = FMath::Lerp(Candelas, FlareCandelas, Flare);
	}
	// Her waking (SaintAdaWakes): from her lit gold up to a white-gold blaze filling the apse, then back to her steady light.
	FLinearColor Burning = EmberColor;
	if (Blaze > 0.f)
	{
		Size = FMath::Lerp(Size, BlazeSize, Blaze);
		Strength = FMath::Lerp(Strength, BlazeStrength, Blaze);
		Candelas = FMath::Lerp(Candelas, BlazeCandelas, Blaze);
		Burning = FMath::Lerp(EmberColor, BlazeColor, Blaze);
	}
	else if (IsLit())
	{
		// Lit, her ember warms toward the keeper's gold: a coal that caught, not one guttering under ash.
		Burning = FMath::Lerp(EmberColor, BlazeColor, LitWarmth);
	}
	EmberGlow = Strength;
	EmberAt = GetEmberStart() + FVector(0.0, 0.0, EmberRise * Flash.GetEmberRise() * (Flare > 0.f ? 1.f : 0.f));

	if (EmberQuad && EmberQuad->GetInstanceCount() > 0)
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
		EmberQuad->UpdateInstanceTransform(0, FTransform(FRotationMatrix::MakeFromZ(ToCamera).ToQuat(), EmberAt,
			FVector(Size / 100.0, Size / 100.0, 1.0)), /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		const float Data[] = { Burning.R, Burning.G, Burning.B, Strength, 1.f };
		EmberQuad->SetCustomData(0, MakeArrayView(Data), /*bMarkRenderStateDirty*/ false);
		EmberQuad->MarkRenderStateDirty();
	}
	if (EmberLight)
	{
		EmberLight->SetWorldLocation(EmberAt + FVector(0.0, 0.0, 8.0));
		EmberLight->SetIntensity(Candelas);
		EmberLight->SetLightColor(Burning);
		EmberLight->SetAttenuationRadius(FMath::Lerp(EmberLightRadius, BlazeLightRadius, Blaze));
	}
}

void AChapelReliquary::FinishLook()
{
	// The flare sinks back under the ash: she's still there, and that's all.
	++Looks;
	UE_LOG(LogLooter, Log, TEXT("%s: the sight fades; her ember gutters again."), *GetActorNameOrLabel());
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(SightEvent, this));
	}
}

TOptional<FInteractionBox> AChapelReliquary::GetInteractionBox() const
{
	// The Reliquary as it stands: anywhere on it (without its model, a box where it would be).
	if (HasModel())
	{
		return InteractionFocus::GrownBox(Mesh->Bounds.GetBox(), InteractionFocus::SmallThingHalfWidth, InteractionFocus::SmallThingHalfHeight);
	}
	return InteractionFocus::BoxAround(GetInteractionLocation().Get(GetActorLocation()), FVector(30.0), GetActorQuat());
}
