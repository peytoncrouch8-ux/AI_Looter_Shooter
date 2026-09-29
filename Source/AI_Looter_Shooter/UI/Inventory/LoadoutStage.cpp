#include "UI/Inventory/LoadoutStage.h"
#include "Player/Animation/LooterCharacterAnimInstance.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponModelBuilder.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Character.h"

namespace
{
	/** Far outside any level, so nothing there shadows or lights it, and the minimap's top-down bake never sees it. */
	const FVector StageLocation(400000.0, 400000.0, 50000.0);

	/** Seen only by the stage's camera, lit only by its studio lights (lighting channel 1), and never in the way of anything. */
	void SetupStagePrimitive(UPrimitiveComponent* Primitive)
	{
		Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Primitive->SetGenerateOverlapEvents(false);
		Primitive->SetCanEverAffectNavigation(false);
		Primitive->bVisibleInSceneCaptureOnly = true;
		Primitive->bReceivesDecals = false;
		Primitive->LightingChannels.bChannel0 = false;
		Primitive->LightingChannels.bChannel1 = true;
	}

	bool IsSameGun(const FWeaponInstanceData& A, const FWeaponInstanceData& B)
	{
		return A.Definition == B.Definition && A.Seed == B.Seed && A.Rarity == B.Rarity && A.Level == B.Level;
	}
}

// ---------------------------------------------------------------------------
// Where guns are carried
// ---------------------------------------------------------------------------

ELoadoutCarry LoadoutCarry::ForSlot(int32 Slot, int32 NumWeapons, int32 ActiveSlot)
{
	if (Slot < 0 || Slot >= NumWeapons)
	{
		return ELoadoutCarry::None;
	}
	if (Slot == ActiveSlot)
	{
		return ELoadoutCarry::InHand;
	}
	int32 Holstered = 0;
	for (int32 Other = 0; Other < Slot; ++Other)
	{
		Holstered += Other != ActiveSlot ? 1 : 0;
	}
	return Holstered % 2 == 0 ? ELoadoutCarry::Back : ELoadoutCarry::Hip;
}

const TCHAR* LoadoutCarry::Label(ELoadoutCarry Carry)
{
	switch (Carry)
	{
	case ELoadoutCarry::InHand: return TEXT("In hand");
	case ELoadoutCarry::Back:   return TEXT("On back");
	case ELoadoutCarry::Hip:    return TEXT("On hip");
	default:                    return TEXT("Empty");
	}
}

// ---------------------------------------------------------------------------
// Stage
// ---------------------------------------------------------------------------

ALoadoutStage::ALoadoutStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// After the stand-in has animated, so the guns land in its hands this frame.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Turntable = CreateDefaultSubobject<USceneComponent>(TEXT("Turntable"));
	Turntable->SetupAttachment(Root);

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Turntable);
	// The mannequin faces +Y in its own space; the turntable's +X is the way the stand-in faces.
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	// Only the stage's camera ever sees it, so it must animate whether or not the main view renders it.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->PrimaryComponentTick.bStartWithTickEnabled = false;
	SetupStagePrimitive(Body);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = false;
	// Scene color, with inverse opacity in alpha so the screen can cut the stand-in out (it tone maps the color itself).
	Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	Capture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	Capture->bExcludeFromSceneTextureExtents = true;
	FEngineShowFlags& Show = Capture->ShowFlags;
	Show.SetAtmosphere(false);
	Show.SetFog(false);
	Show.SetVolumetricFog(false);
	Show.SetCloud(false);
	Show.SetLightShafts(false);
	Show.SetDistanceFieldAO(false);
	Show.SetLumenGlobalIllumination(false);
	Show.SetLumenReflections(false);
	Show.SetScreenSpaceReflections(false);
	Show.SetMotionBlur(false);
	Show.SetBloom(false);
	Show.SetDecals(false);
	Show.SetParticles(false);

	// Studio lighting, fixed around the camera: a warm key from the front left, a cool fill from the right and a cyan rim
	// from behind. Channel 1 only, so they light nothing but the stand-in and its guns.
	auto MakeLight = [this](const TCHAR* Name, const FVector& Location, const FLinearColor& Color, float Candelas, bool bShadows)
	{
		UPointLightComponent* Light = CreateDefaultSubobject<UPointLightComponent>(Name);
		Light->SetupAttachment(Root);
		Light->SetRelativeLocation(Location);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Candelas);
		Light->SetLightColor(Color);
		Light->SetAttenuationRadius(1200.f);
		Light->SetCastShadows(bShadows);
		Light->LightingChannels.bChannel0 = false;
		Light->LightingChannels.bChannel1 = true;
		Light->SetVisibility(false);
		return Light;
	};
	KeyLight = MakeLight(TEXT("KeyLight"), FVector(-300.f, -220.f, 280.f), FLinearColor(1.f, 0.93f, 0.84f), 60.f, true);
	FillLight = MakeLight(TEXT("FillLight"), FVector(-260.f, 260.f, 110.f), FLinearColor(0.72f, 0.84f, 1.f), 14.f, false);
	RimLight = MakeLight(TEXT("RimLight"), FVector(180.f, 120.f, 230.f), FLinearColor(0.45f, 0.8f, 1.f), 90.f, false);
}

void ALoadoutStage::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(StageLocation);

	RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("LoadoutStageImage"));
	RenderTarget->RenderTargetFormat = RTF_RGBA16f;
	// Nothing drawn reads as see-through (alpha is inverse opacity).
	RenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
	// Rendered larger than it's shown: mips keep the downsized picture smooth.
	RenderTarget->bAutoGenerateMips = true;
	RenderTarget->Filter = TF_Trilinear;
	RenderTarget->InitAutoFormat(ImageWidth, ImageHeight);
	RenderTarget->UpdateResourceImmediate(true);

	Capture->TextureTarget = RenderTarget;
	Capture->ShowOnlyActors.Add(this);
	ResetTurn();
}

void ALoadoutStage::ShowLoadout(const ACharacter* Character, const UWeaponManagerComponent* Weapons)
{
	const TArray<AWeaponBase*> Equipped = Weapons ? Weapons->GetWeapons() : TArray<AWeaponBase*>();
	const int32 ActiveSlot = Weapons ? Weapons->GetActiveSlot() : INDEX_NONE;
	const bool bArmed = Equipped.IsValidIndex(ActiveSlot) && Equipped[ActiveSlot];
	if (Weapons)
	{
		HoldSocket = Weapons->ThirdPersonAttachSocket;
	}

	// The body: the character's own mesh and materials, with its animation (armed while a gun is in hand).
	if (const USkeletalMeshComponent* Source = Character ? Character->GetMesh() : nullptr)
	{
		if (Body->GetSkeletalMeshAsset() != Source->GetSkeletalMeshAsset())
		{
			Body->SetSkeletalMeshAsset(Source->GetSkeletalMeshAsset());
		}
		for (int32 Index = 0; Index < Source->GetNumMaterials(); ++Index)
		{
			Body->SetMaterial(Index, Source->GetMaterial(Index));
		}
		const UPlayerViewComponent* View = Character->FindComponentByClass<UPlayerViewComponent>();
		const TSubclassOf<UAnimInstance> AnimClass = View ? View->GetBodyAnimClass(bArmed) : Source->AnimClass;
		if (AnimClass && Body->GetAnimClass() != AnimClass)
		{
			Body->SetAnimInstanceClass(AnimClass);
		}
	}

	// A gun per equipped slot; only the ones that changed are rebuilt.
	while (Guns.Num() > Equipped.Num())
	{
		DestroyGun(Guns.Last());
		Guns.Pop();
	}
	Guns.SetNum(Equipped.Num());
	for (int32 Slot = 0; Slot < Equipped.Num(); ++Slot)
	{
		FLoadoutStageGun& Gun = Guns[Slot];
		if (!Equipped[Slot])
		{
			DestroyGun(Gun);
			continue;
		}
		const FWeaponInstanceData& Instance = Equipped[Slot]->GetInstance();
		if (!Gun.Mesh || !IsSameGun(Gun.Instance, Instance))
		{
			DestroyGun(Gun);
			BuildGun(Gun, Instance);
		}
		Gun.Carry = LoadoutCarry::ForSlot(Slot, Equipped.Num(), ActiveSlot);
	}
	UpdateHold();
}

void ALoadoutStage::BuildGun(FLoadoutStageGun& Gun, const FWeaponInstanceData& Instance)
{
	Gun = FLoadoutStageGun();
	Gun.Instance = Instance;

	UDynamicMeshComponent* Mesh = NewObject<UDynamicMeshComponent>(this);
	UDynamicMeshComponent* Part = NewObject<UDynamicMeshComponent>(this);
	SetupStagePrimitive(Mesh);
	SetupStagePrimitive(Part);
	WeaponModels::FPoints Points;
	if (!WeaponModels::BuildInto(Instance, Mesh, Part, Points))
	{
		// Only code-built guns can be copied; a gun with its own mesh asset isn't shown on the stand-in.
		Mesh->MarkAsGarbage();
		Part->MarkAsGarbage();
		return;
	}
	Mesh->SetupAttachment(Root);
	Part->SetupAttachment(Mesh);
	Mesh->RegisterComponent();
	Part->RegisterComponent();

	Gun.Mesh = Mesh;
	Gun.Part = Part;
	Gun.Grip = Points.Grip;
	Gun.Foregrip = Points.Foregrip;
	Gun.Muzzle = Points.Muzzle;
	Gun.Center = Mesh->GetLocalBounds().Origin;
}

void ALoadoutStage::DestroyGun(FLoadoutStageGun& Gun)
{
	if (Gun.Part)
	{
		Gun.Part->DestroyComponent();
	}
	if (Gun.Mesh)
	{
		Gun.Mesh->DestroyComponent();
	}
	Gun = FLoadoutStageGun();
}

void ALoadoutStage::SetActive(bool bInActive)
{
	if (bActive == bInActive)
	{
		return;
	}
	bActive = bInActive;
	SetActorTickEnabled(bActive);
	Body->SetComponentTickEnabled(bActive);
	for (UPointLightComponent* Light : { KeyLight.Get(), FillLight.Get(), RimLight.Get() })
	{
		Light->SetVisibility(bActive);
	}
	if (bActive)
	{
		PlaceCamera();
		UpdateHold();
	}
}

void ALoadoutStage::AddTurn(float Degrees)
{
	Turn = FRotator::NormalizeAxis(Turn + Degrees);
	PlaceCamera();
	UpdateHold();
}

void ALoadoutStage::ResetTurn()
{
	Turn = DefaultTurn;
	PlaceCamera();
	UpdateHold();
}

void ALoadoutStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The framing is re-applied every frame so it can be tuned live.
	PlaceCamera();
	PlaceGuns();
	Capture->CaptureSceneDeferred();
}

// ---------------------------------------------------------------------------
// Posing
// ---------------------------------------------------------------------------

FQuat ALoadoutStage::GetFacing() const
{
	return Turntable->GetComponentQuat();
}

const FLoadoutStageGun* ALoadoutStage::GetGunInHand() const
{
	return Guns.FindByPredicate([](const FLoadoutStageGun& Gun) { return Gun.Mesh && Gun.Carry == ELoadoutCarry::InHand; });
}

void ALoadoutStage::UpdateHold()
{
	ULooterCharacterAnimInstance* Anim = Cast<ULooterCharacterAnimInstance>(Body->GetAnimInstance());
	if (!Anim)
	{
		return;
	}
	if (const FLoadoutStageGun* Gun = GetGunInHand())
	{
		Anim->SetStandaloneHold(Gun->Grip, Gun->Foregrip, HoldSocket, GetFacing());
	}
	else
	{
		Anim->ClearStandaloneHold();
	}
}

void ALoadoutStage::PlaceCamera()
{
	Capture->SetRelativeLocationAndRotation(FVector(-CameraDistance, 0.f, CameraHeight), FRotator(CameraPitch, 0.f, 0.f));
	Capture->FOVAngle = FieldOfView;
	// Turn 0 faces the camera.
	Turntable->SetRelativeRotation(FRotator(0.f, 180.f + Turn, 0.f));
}

void ALoadoutStage::PlaceGuns()
{
	const FQuat Facing = GetFacing();
	for (FLoadoutStageGun& Gun : Guns)
	{
		if (!Gun.Mesh)
		{
			continue;
		}
		FQuat Rotation = Facing;
		FVector Location = FVector::ZeroVector;
		switch (Gun.Carry)
		{
		case ELoadoutCarry::InHand:
			// Like the third-person hold: grip in the hand, barrel level along the way the stand-in faces.
			Location = Body->GetSocketLocation(HoldSocket) - Rotation.RotateVector(Gun.Grip);
			break;
		case ELoadoutCarry::Back:
		case ELoadoutCarry::Hip:
		{
			const bool bBack = Gun.Carry == ELoadoutCarry::Back;
			Rotation = Facing * FRotationMatrix::MakeFromXZ(bBack ? BackDirection : HipDirection, bBack ? BackUp : HipUp).ToQuat();
			const FVector Holster = Body->GetBoneLocation(bBack ? BackBone : HipBone) + Facing.RotateVector(bBack ? BackOffset : HipOffset);
			Location = Holster - Rotation.RotateVector(Gun.Center);
			break;
		}
		default:
			Gun.Mesh->SetVisibility(false, true);
			continue;
		}
		Gun.Mesh->SetVisibility(true, true);
		Gun.Mesh->SetWorldLocationAndRotation(Location, Rotation);
	}
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

bool ALoadoutStage::ProjectToImage(const FVector& WorldLocation, FVector2D& OutUV) const
{
	// View space: X ahead, Y right, Z up.
	const FVector Local = Capture->GetComponentTransform().InverseTransformPositionNoScale(WorldLocation);
	if (Local.X <= 1.0)
	{
		return false;
	}
	// Scene captures spread the field of view across the width; the height follows from the aspect ratio.
	const double TanHalf = FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle * 0.5));
	const double Aspect = static_cast<double>(ImageWidth) / ImageHeight;
	const double X = Local.Y / (Local.X * TanHalf);
	const double Y = Local.Z * Aspect / (Local.X * TanHalf);
	OutUV = FVector2D(0.5 + 0.5 * X, 0.5 - 0.5 * Y);
	return true;
}

bool ALoadoutStage::GetSlotAnchor(int32 Slot, FVector& OutWorldLocation) const
{
	if (!Guns.IsValidIndex(Slot) || !Guns[Slot].Mesh || Guns[Slot].Carry == ELoadoutCarry::None)
	{
		return false;
	}
	const FLoadoutStageGun& Gun = Guns[Slot];
	const FTransform& GunTransform = Gun.Mesh->GetComponentTransform();
	OutWorldLocation = GunTransform.TransformPosition(Gun.Center);
	FVector2D Spine;
	if (Gun.Carry != ELoadoutCarry::Back || !ProjectToImage(Body->GetBoneLocation(BackBone), Spine))
	{
		return true;
	}

	// A gun on the back only shows past the body at one end, which one depends on how the stand-in is turned: point at the
	// end that sticks out sideways furthest (the muzzle end when it's a tie, as that one shows over the shoulder).
	double Furthest = -1.0;
	const FVector Ends[] = { FMath::Lerp(Gun.Center, Gun.Muzzle, 0.85), FMath::Lerp(Gun.Center, Gun.Center * 2.0 - Gun.Muzzle, 0.85) };
	for (const FVector& End : Ends)
	{
		const FVector World = GunTransform.TransformPosition(End);
		FVector2D Point;
		const double Sideways = ProjectToImage(World, Point) ? FMath::Abs(Point.X - Spine.X) : -1.0;
		if (Sideways > Furthest + 0.01)
		{
			Furthest = Sideways;
			OutWorldLocation = World;
		}
	}
	return true;
}

FVector ALoadoutStage::GetFloorCenter() const
{
	return Turntable->GetComponentLocation();
}

FVector ALoadoutStage::GetTowardCamera() const
{
	const FVector Toward = (Capture->GetComponentLocation() - GetFloorCenter()).GetSafeNormal2D();
	return Toward.IsNearlyZero() ? FVector::BackwardVector : Toward;
}
