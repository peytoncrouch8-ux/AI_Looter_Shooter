#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Weapons/WeaponParts.h"
#include "World/LightBeam.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Loot/LootTossComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

AWeaponBase::AWeaponBase()
{
	// Ticks only while reloading (to animate the magazine or pump) or flashing.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(15.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Collision);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(Collision);
	SkeletalMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(Collision);
	StaticMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	Model = CreateDefaultSubobject<UWeaponModelComponent>(TEXT("Model"));
	Model->SetupAttachment(Collision);

	LootBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LootBeam"));
	LootBeam->SetupAttachment(Collision);
	LootBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LootBeam->SetCastShadow(false);
	LootBeam->SetVisibility(false);

	MuzzleFlash = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MuzzleFlash"));
	MuzzleFlash->SetupAttachment(Collision);
	MuzzleFlash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MuzzleFlash->SetCastShadow(false);
	MuzzleFlash->SetCanEverAffectNavigation(false);
	MuzzleFlash->bReceivesDecals = false;
	MuzzleFlash->SetVisibility(false);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Collision);
	MuzzleLight->SetIntensityUnits(ELightUnits::Candelas);
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetAttenuationRadius(320.f);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.7f, 0.42f));
	MuzzleLight->SetCastShadows(false);
	MuzzleLight->SetVisibility(false);

	RarityLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RarityLight"));
	RarityLight->SetupAttachment(Collision);
	RarityLight->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	RarityLight->SetIntensityUnits(ELightUnits::Candelas);
	RarityLight->SetIntensity(6.f);
	RarityLight->SetAttenuationRadius(180.f);
	RarityLight->SetCastShadows(false);
	RarityLight->SetVisibility(false);

	Label = CreateDefaultSubobject<UWidgetComponent>(TEXT("Label"));
	Label->SetupAttachment(Collision);
	Label->SetUsingAbsoluteRotation(true);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Label->SetWidgetSpace(EWidgetSpace::Screen);
	Label->SetDrawAtDesiredSize(true);
	Label->SetPivot(FVector2D(0.5f, 1.f));
	Label->SetWidgetClass(UWeaponLabelWidget::StaticClass());
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetVisibility(false);

	TossMovement = CreateDefaultSubobject<ULootTossComponent>(TEXT("TossMovement"));
	TossMovement->SetUpdatedComponent(Collision);
	TossMovement->Bounciness = 0.25f;

	SpinMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("SpinMovement"));
	SpinMovement->bAutoActivate = false;
	SpinMovement->RotationRate = FRotator(0.f, 90.f, 0.f);
}

void AWeaponBase::InitializeFromInstance(const FWeaponInstanceData& InInstance)
{
	Instance = InInstance;
	Instance.Level = FMath::Max(Instance.Level, 1);
	// A gun saved before parts were kept gets the parts its seed picks now (a named gun its own), and keeps them from now on.
	if (Instance.Parts.IsEmpty() && Instance.Definition && !Instance.Definition->Parts.IsEmpty())
	{
		Instance.Parts = Instance.Named ? Instance.Named->GetPartKeys()
			: WeaponParts::PartKeys(WeaponParts::Pick(*Instance.Definition, Instance.Seed, Instance.Rarity));
	}
	// Always rebuild stats from the seed and parts (a named gun's at its fixed quality) so saved instances can't drift
	// from their definition.
	Instance.Stats = UWeaponRollLibrary::ComputeInstanceStats(Instance);

	CurrentMagazine = Instance.SavedMagazine >= 0 ? FMath::Min(Instance.SavedMagazine, Instance.Stats.MagazineSize) : Instance.Stats.MagazineSize;
	bAmmoInitialized = true;

	ApplyDefinitionVisuals();
	BroadcastAmmo();
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Weapons placed directly in a level get initialized from the Instance set in the Details panel.
	if (!bAmmoInitialized && Instance.Definition)
	{
		InitializeFromInstance(Instance);
	}
	else
	{
		// The label widget only exists once components are registered, so refresh it now.
		ApplyDefinitionVisuals();
	}

	TossMovement->OnProjectileStop.AddDynamic(this, &AWeaponBase::HandleTossStopped);

	if (GetOwner() == nullptr)
	{
		// Unowned weapons are loot: drop them onto whatever is below.
		Toss(FVector::ZeroVector);
	}
}

void AWeaponBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateReloadPart();
	UpdateMuzzleFlash(DeltaSeconds);
	RefreshTick();
}

void AWeaponBase::RefreshTick()
{
	const bool bAnimatingReload = bReloading && GetReloadPart() != EWeaponReloadPart::None;
	const bool bWanted = bAnimatingReload || FlashTimeLeft > 0.f;
	if (IsActorTickEnabled() != bWanted)
	{
		SetActorTickEnabled(bWanted);
	}
}

void AWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	Super::EndPlay(EndPlayReason);
}

FWeaponInstanceData AWeaponBase::GetInstanceForStorage() const
{
	FWeaponInstanceData Stored = Instance;
	Stored.SavedMagazine = CurrentMagazine;
	return Stored;
}

EAmmoType AWeaponBase::GetAmmoType() const
{
	return Instance.Definition ? Instance.Definition->AmmoType : EAmmoType::AssaultRifle;
}

UWeaponManagerComponent* AWeaponBase::GetHolderInventory() const
{
	const AActor* Holder = GetOwner();
	return Holder ? Holder->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
}

int32 AWeaponBase::GetReserveAmmo() const
{
	const UWeaponManagerComponent* Inventory = GetHolderInventory();
	return Inventory ? Inventory->GetAmmo(GetAmmoType()) : 0;
}

FText AWeaponBase::GetDisplayName() const
{
	// A named gun goes by its own name ("Heirloom sent to backpack").
	if (Instance.Named && !Instance.Named->DisplayName.IsEmpty())
	{
		return Instance.Named->DisplayName;
	}
	return Instance.Definition ? Instance.Definition->DisplayName : FText::FromString(GetName());
}

// ---------------------------------------------------------------------------
// Equip / world state
// ---------------------------------------------------------------------------

void AWeaponBase::OnEquipped(APawn* NewOwner, USceneComponent* AttachTo, FName Socket, const FTransform& AttachOffset)
{
	SetOwner(NewOwner);
	SetInstigator(NewOwner);
	SetPickupState(false);

	// Undo any loot spin so the gun points forward in hand.
	SkeletalMesh->SetRelativeRotation(FRotator::ZeroRotator);
	StaticMesh->SetRelativeRotation(FRotator::ZeroRotator);
	Model->SetRelativeRotation(FRotator::ZeroRotator);

	AttachToHolder(AttachTo, Socket, AttachOffset);
	SetActorHiddenInGame(false);
	BroadcastAmmo();

	// Drawn: it fires once it's up, quicker the better its handling (a held trigger fires as soon as it's ready).
	if (const UWorld* World = GetWorld())
	{
		DrawnTime = World->GetTimeSeconds();
		ReadySeconds = BaseReadySeconds / FMath::Max(Instance.Stats.Handling, 0.1f);
		LastFireTime = FMath::Max(LastFireTime, DrawnTime + ReadySeconds - Instance.Stats.GetSecondsBetweenShots());
	}
}

void AWeaponBase::AttachToHolder(USceneComponent* AttachTo, FName Socket, const FTransform& AttachOffset)
{
	if (!AttachTo)
	{
		return;
	}
	AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	SetActorRelativeTransform(AttachOffset);

	// Render like the holder: first-person arms, or the first-person camera, get first-person rendering (no clipping,
	// the view model's own FOV, so the player's field of view setting doesn't grow or shrink the gun); a third-person
	// body gets normal world rendering.
	const UPrimitiveComponent* ParentPrimitive = Cast<UPrimitiveComponent>(AttachTo);
	const bool bFirstPerson = (ParentPrimitive && ParentPrimitive->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson)
		|| AttachTo->IsA<UCameraComponent>();
	const EFirstPersonPrimitiveType Type = bFirstPerson ? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::None;
	SkeletalMesh->SetFirstPersonPrimitiveType(Type);
	StaticMesh->SetFirstPersonPrimitiveType(Type);
	Model->SetFirstPersonPrimitiveType(Type);
	MuzzleFlash->SetFirstPersonPrimitiveType(Type);
}

void AWeaponBase::OnHolstered()
{
	StopFire();
	GetWorldTimerManager().ClearTimer(FireTimer);
	CancelReload();
	FlashTimeLeft = 0.f;
	MuzzleFlash->SetVisibility(false);
	MuzzleLight->SetVisibility(false);
	RefreshTick();
	SetActorHiddenInGame(true);
}

void AWeaponBase::OnDropped()
{
	OnHolstered();

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
	SetInstigator(nullptr);

	SkeletalMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	StaticMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	Model->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	MuzzleFlash->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);

	SetActorHiddenInGame(false);
	SetPickupState(true);
}

void AWeaponBase::Toss(const FVector& Velocity)
{
	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	SetPickupState(true);
	TossMovement->Throw(Velocity);
}

void AWeaponBase::HandleTossStopped(const FHitResult& ImpactResult)
{
	// Settle flat so the spin looks clean.
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
}

void AWeaponBase::SetPickupState(bool bPickup)
{
	bIsPickup = bPickup;

	if (bPickup)
	{
		// Only collide with world geometry so the toss can bounce and settle; ignore pawns, bullets and other loot.
		Collision->SetCollisionObjectType(ECC_WorldDynamic);
		Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SpinMovement->Activate(true);
	}
	else
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TossMovement->StopMovementImmediately();
		TossMovement->Deactivate();
		SpinMovement->Deactivate();
		SetLabelState(false, false);
	}

	// Guns built from parts have their origin at the back of the receiver; spin loot around the middle of the gun.
	const FVector ModelCenter(bUsingModel ? Model->GetMuzzle().X * 0.45f : 0.f, 0.f, 0.f);
	SpinMovement->PivotTranslation = bPickup ? ModelCenter : FVector::ZeroVector;
	Model->SetRelativeLocationAndRotation(bPickup ? -ModelCenter : FVector::ZeroVector, FRotator::ZeroRotator);

	RarityLight->SetVisibility(bPickup);
	RefreshLootBeam();
}

void AWeaponBase::RefreshLootBeam()
{
	// Commons stay subtle; every tier above throws a taller pillar of its color into the sky.
	static const float Heights[] = { 0.f, 350.f, 650.f, 1000.f, 1600.f };
	const float Height = Heights[FMath::Clamp(static_cast<int32>(Instance.Rarity), 0, 4)];
	const bool bShow = bIsPickup && Height > 0.f && Instance.Definition;
	LootBeam->SetVisibility(bShow);
	if (bShow)
	{
		LightBeams::Setup(LootBeam, UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity),
			Instance.Rarity >= EWeaponRarity::Epic ? 3.f : 2.f, Height, Instance.Rarity >= EWeaponRarity::Epic ? 9.f : 6.f);
	}
}

void AWeaponBase::SetLabelState(bool bVisible, bool bFocused)
{
	Label->SetVisibility(bVisible && bIsPickup);
	if (UWeaponLabelWidget* LabelWidget = Cast<UWeaponLabelWidget>(Label->GetUserWidgetObject()))
	{
		LabelWidget->SetFocused(bFocused);
	}
}

void AWeaponBase::ApplyDefinitionVisuals()
{
	const UWeaponDefinition* Definition = Instance.Definition;
	bUsingModel = Model->Assemble(Instance);
	const bool bUseSkeletal = !bUsingModel && Definition && Definition->SkeletalMesh;
	const bool bUseStatic = !bUsingModel && !bUseSkeletal;

	SkeletalMesh->SetSkeletalMesh(bUseSkeletal ? Definition->SkeletalMesh.Get() : nullptr);
	SkeletalMesh->SetVisibility(bUseSkeletal);

	StaticMesh->SetStaticMesh(bUseStatic && Definition ? Definition->StaticMesh.Get() : nullptr);
	StaticMesh->SetVisibility(bUseStatic);

	UpdateReloadPart();
	SetupMuzzleFlash();
	RefreshLootBeam();

	// Spin just the mesh, not the root, so the toss physics and label stay steady.
	SpinMovement->SetUpdatedComponent(GetActiveMesh());

	RarityLight->SetLightColor(UWeaponRollLibrary::GetRarityColor(Definition, Instance.Rarity));

	if (UWeaponLabelWidget* LabelWidget = Cast<UWeaponLabelWidget>(Label->GetUserWidgetObject()))
	{
		LabelWidget->SetWeapon(this);
	}
}

USceneComponent* AWeaponBase::GetActiveMesh() const
{
	if (bUsingModel)
	{
		return Model;
	}
	if (SkeletalMesh->GetSkeletalMeshAsset())
	{
		return SkeletalMesh;
	}
	return StaticMesh;
}

bool AWeaponBase::IsDrawnFirstPerson() const
{
	const EFirstPersonPrimitiveType Type = bUsingModel ? Model->GetFirstPersonPrimitiveType()
		: CastChecked<UPrimitiveComponent>(GetActiveMesh())->FirstPersonPrimitiveType;
	return Type == EFirstPersonPrimitiveType::FirstPerson;
}

FVector AWeaponBase::GetGripPoint() const
{
	return bUsingModel ? Model->GetGrip() : FVector::ZeroVector;
}

FVector AWeaponBase::GetForegripPoint() const
{
	return bUsingModel ? Model->GetForegrip() : FVector(30.f, 0.f, -3.f);
}

EWeaponReloadPart AWeaponBase::GetReloadPart() const
{
	return bUsingModel ? Model->GetReloadPart() : EWeaponReloadPart::None;
}

FVector AWeaponBase::GetMuzzleLocation() const
{
	if (bUsingModel)
	{
		return Model->GetComponentTransform().TransformPosition(Model->GetMuzzle());
	}
	const USceneComponent* Mesh = GetActiveMesh();
	const FName Socket = Instance.Definition ? Instance.Definition->MuzzleSocket : NAME_None;
	if (Mesh && Mesh->DoesSocketExist(Socket))
	{
		return Mesh->GetSocketLocation(Socket);
	}
	return GetActorLocation();
}

void AWeaponBase::GetAimViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (const UPlayerViewComponent* View = Pawn ? Pawn->FindComponentByClass<UPlayerViewComponent>() : nullptr)
	{
		// Players: the view decides (crosshair in first/third person, eyes in the front view).
		View->GetAimViewPoint(OutLocation, OutRotation);
	}
	else if (Pawn && Pawn->GetController())
	{
		// Camera for players, eyes for AI.
		Pawn->GetController()->GetPlayerViewPoint(OutLocation, OutRotation);
	}
	else if (Pawn)
	{
		Pawn->GetActorEyesViewPoint(OutLocation, OutRotation);
	}
	else
	{
		OutLocation = GetMuzzleLocation();
		OutRotation = GetActorRotation();
	}
}
