#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Weapons/BulletSubsystem.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponRollLibrary.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "UI/WeaponLabelWidget.h"
#include "Weapons/WeaponManagerComponent.h"
#include "Weapons/WeaponModelBuilder.h"
#include "Environment/StylizedSurface.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/DynamicMeshComponent.h"
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

namespace
{
	// Built-in muzzle flash (M_FX_Glow reads color, strength and shape from each quad's custom data;
	// shapes: 0 = streak, 1 = round, 2 = six-pointed star).
	constexpr float FlashDuration = 0.055f;
	constexpr float FlashCoreSize = 11.f;
	constexpr float FlashStarSize = 34.f;
	constexpr float FlashTongueLength = 26.f;
	constexpr float FlashTongueWidth = 9.f;
	constexpr float FlashCoreGlow = 26.f;
	constexpr float FlashStarGlow = 16.f;
	constexpr float FlashTongueGlow = 14.f;
	constexpr float FlashLightCandelas = 24.f;
	const FLinearColor FlashCoreColor(1.f, 0.85f, 0.6f);
	const FLinearColor FlashStarColor(1.f, 0.62f, 0.25f);
	const FLinearColor FlashTongueColor(1.f, 0.58f, 0.22f);
}

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

	ModelMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("ModelMesh"));
	ModelMesh->SetupAttachment(Collision);
	ModelMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	ModelMesh->SetVisibility(false);

	// Built in the model's own space, so at rest it sits exactly in place and a reload only offsets it.
	ModelPartMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("ModelPartMesh"));
	ModelPartMesh->SetupAttachment(ModelMesh);
	ModelPartMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	ModelPartMesh->SetVisibility(false);

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
	// Always rebuild stats from the seed so saved instances can't drift from their definition.
	Instance.Stats = UWeaponRollLibrary::ComputeStats(Instance.Definition, Instance.Rarity, Instance.Level, Instance.Seed);

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
	return Instance.Definition ? Instance.Definition->DisplayName : FText::FromString(GetName());
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------

void AWeaponBase::StartFire()
{
	bWantsToFire = true;

	if (bReloading || !Instance.Definition)
	{
		return;
	}

	FTimerManager& Timers = GetWorldTimerManager();
	if (Timers.IsTimerActive(FireTimer))
	{
		// Already cycling (auto fire, a burst in progress, or a queued shot).
		return;
	}

	if (Instance.Definition->FireMode == EWeaponFireMode::Burst)
	{
		BurstShotsRemaining = Instance.Definition->BurstCount;
	}

	const double Elapsed = GetWorld()->GetTimeSeconds() - LastFireTime;
	const float Interval = Instance.Stats.GetSecondsBetweenShots();
	if (Elapsed >= Interval)
	{
		HandleFiring();
	}
	else
	{
		// Respect fire rate even when the trigger is spammed.
		Timers.SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, static_cast<float>(Interval - Elapsed), false);
	}
}

void AWeaponBase::StopFire()
{
	bWantsToFire = false;

	// Bursts finish on their own and semi-auto keeps a queued shot, so only auto stops immediately.
	if (Instance.Definition && Instance.Definition->FireMode == EWeaponFireMode::FullAuto)
	{
		GetWorldTimerManager().ClearTimer(FireTimer);
	}
}

void AWeaponBase::HandleFiring()
{
	if (bReloading || !Instance.Definition)
	{
		return;
	}

	if (CurrentMagazine <= 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Instance.Definition->DryFireSound, GetMuzzleLocation());
		BurstShotsRemaining = 0;
		Reload();
		return;
	}

	FireShot();
	--CurrentMagazine;
	LastFireTime = GetWorld()->GetTimeSeconds();

	BroadcastAmmo();
	OnFired.Broadcast();
	K2_OnShotFired();

	if (CurrentMagazine <= 0)
	{
		BurstShotsRemaining = 0;
		Reload();
		return;
	}

	bool bContinue = false;
	switch (Instance.Definition->FireMode)
	{
	case EWeaponFireMode::FullAuto:
		bContinue = bWantsToFire;
		break;
	case EWeaponFireMode::Burst:
		bContinue = --BurstShotsRemaining > 0;
		break;
	default:
		break;
	}

	if (bContinue)
	{
		GetWorldTimerManager().SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, Instance.Stats.GetSecondsBetweenShots(), false);
	}
}

void AWeaponBase::FireShot()
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetAimViewPoint(ViewLocation, ViewRotation);
	const FVector AimDirection = ViewRotation.Vector();
	UE_LOG(LogLooter, VeryVerbose, TEXT("%s shot from %s along %s"), *GetName(), *ViewLocation.ToCompactString(), *AimDirection.ToCompactString());

	// Every pellet is a real bullet that flies from the aim point along the aim (the tracer leaves from the muzzle).
	const UWeaponDefinition* Definition = Instance.Definition;
	if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		FBulletShot Shot;
		Shot.Start = ViewLocation;
		Shot.VisualStart = GetVisibleMuzzleLocation();
		Shot.Speed = Definition->BulletSpeed;
		Shot.Range = Instance.Stats.Range;
		Shot.Damage = Instance.Stats.Damage;
		Shot.HitImpulse = Definition->HitImpulse;
		Shot.Channel = TraceChannel;
		Shot.TracerColor = Definition->TracerColor;
		Shot.TracerWidth = Definition->TracerWidth;
		Shot.TracerLength = Definition->TracerLength;
		Shot.ImpactFX = Definition->ImpactFX.Get();
		Shot.bDrawDebug = bDrawDebugTraces;
		Shot.Weapon = this;
		Shot.Shooter = GetOwner();
		Shot.Instigator = GetInstigatorController();

		const float SpreadRadians = FMath::DegreesToRadians(GetEffectiveSpread());
		for (int32 Pellet = 0; Pellet < Instance.Stats.PelletsPerShot; ++Pellet)
		{
			Shot.Direction = SpreadRadians > 0.f ? FMath::VRandCone(AimDirection, SpreadRadians) : AimDirection;
			Bullets->Fire(Shot);
		}
	}

	if (Definition->MuzzleFlashFX)
	{
		if (bUsingModel)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->MuzzleFlashFX, ModelMesh, NAME_None,
				ModelMuzzle, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
		}
		else
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->MuzzleFlashFX, GetActiveMesh(), Definition->MuzzleSocket,
				FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
		}
	}
	else
	{
		PlayMuzzleFlash();
	}
	UGameplayStatics::PlaySoundAtLocation(this, Definition->FireSound, GetMuzzleLocation());
}

void AWeaponBase::NotifyBulletHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	OnHit.Broadcast(Hit, Damage, bCritical);
}

const FWeaponRecoilProfile& AWeaponBase::GetRecoilProfile() const
{
	static const FWeaponRecoilProfile Default;
	return Instance.Definition ? Instance.Definition->Recoil : Default;
}

FVector AWeaponBase::GetVisibleMuzzleLocation() const
{
	const FVector Muzzle = GetMuzzleLocation();
	const UPrimitiveComponent* Mesh = GetActiveMesh();
	const UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(GetOwner());
	if (!Mesh || !Camera || Mesh->FirstPersonPrimitiveType != EFirstPersonPrimitiveType::FirstPerson)
	{
		return Muzzle;
	}
	// First-person guns are drawn with their own field of view and pulled in toward the camera: find where the muzzle
	// actually shows, the way the renderer does.
	FMinimalViewInfo View;
	View.Location = Camera->GetComponentLocation();
	View.Rotation = Camera->GetComponentRotation();
	View.FOV = Camera->FieldOfView;
	View.FirstPersonFOV = Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : Camera->FieldOfView;
	View.FirstPersonScale = Camera->bEnableFirstPersonScale ? Camera->FirstPersonScale : 1.f;
	View.bUseFirstPersonParameters = Camera->bEnableFirstPersonFieldOfView || Camera->bEnableFirstPersonScale;
	return View.bUseFirstPersonParameters ? View.TransformWorldToFirstPerson(Muzzle, false) : Muzzle;
}

// ---------------------------------------------------------------------------
// Muzzle flash
// ---------------------------------------------------------------------------

void AWeaponBase::SetupMuzzleFlash()
{
	const UWeaponDefinition* Definition = Instance.Definition;
	UPrimitiveComponent* Mesh = GetActiveMesh();
	if (!Definition || !Mesh)
	{
		return;
	}

	// Sit on the muzzle of whichever model shows, pointing down the barrel (+X).
	const bool bSocket = !bUsingModel && Mesh->DoesSocketExist(Definition->MuzzleSocket);
	const FName Socket = bSocket ? Definition->MuzzleSocket : NAME_None;
	const FVector Offset = bUsingModel ? ModelMuzzle : FVector::ZeroVector;
	MuzzleFlash->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	MuzzleFlash->SetRelativeLocation(Offset);
	MuzzleLight->AttachToComponent(Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	MuzzleLight->SetRelativeLocation(Offset + FVector(10.f, 0.f, 0.f));

	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow"));
	if (!Glow)
	{
		Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow"));
	}
	MuzzleFlash->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	MuzzleFlash->SetMaterial(0, Glow);
	MuzzleFlash->ClearInstances();
	MuzzleFlash->SetNumCustomDataFloats(5);

	// The engine plane is 100 x 100 in XY, facing +Z.
	const float Scale = Definition->MuzzleFlashScale;
	FlashGlow.Reset();
	auto AddQuad = [this](const FTransform& Transform, const FLinearColor& Color, float Glow, float Shape)
	{
		const float Data[] = { Color.R, Color.G, Color.B, Glow, Shape };
		MuzzleFlash->SetCustomData(MuzzleFlash->AddInstance(Transform), Data);
		FlashGlow.Add(Glow);
	};
	// A hot round core and a spiky star just past the muzzle, facing back down the barrel at the shooter...
	const FRotator FacingBack(-90.f, 0.f, 0.f);
	AddQuad(FTransform(FacingBack, FVector(1.f, 0.f, 0.f), FVector(FlashCoreSize * Scale / 100.f, FlashCoreSize * Scale / 100.f, 1.f)), FlashCoreColor, FlashCoreGlow, 1.f);
	AddQuad(FTransform(FacingBack, FVector(3.f, 0.f, 0.f), FVector(FlashStarSize * Scale / 100.f, FlashStarSize * Scale / 100.f, 1.f)), FlashStarColor, FlashStarGlow, 2.f);
	// ...and three flame tongues along the barrel, 60 degrees apart, for side views (turned so the streak's bright
	// head is at the muzzle end).
	const float Length = FlashTongueLength * Scale;
	for (int32 Tongue = 0; Tongue < 3; ++Tongue)
	{
		AddQuad(FTransform(FRotator(0.f, 180.f, Tongue * 60.f), FVector(Length * 0.5f, 0.f, 0.f), FVector(Length / 100.f, FlashTongueWidth * Scale / 100.f, 1.f)),
			FlashTongueColor, FlashTongueGlow, 0.f);
	}
	FlashTimeLeft = 0.f;
	MuzzleFlash->SetVisibility(false);
	MuzzleLight->SetVisibility(false);
}

void AWeaponBase::PlayMuzzleFlash()
{
	if (MuzzleFlash->GetInstanceCount() == 0)
	{
		return;
	}
	// Every flash a little different: turned, sized and bright at random.
	FlashTimeLeft = FlashDuration;
	FlashStrength = FMath::FRandRange(0.8f, 1.15f);
	MuzzleFlash->SetRelativeRotation(FRotator(0.f, 0.f, FMath::FRandRange(0.f, 360.f)));
	MuzzleFlash->SetRelativeScale3D(FVector(FMath::FRandRange(0.85f, 1.2f)));
	MuzzleFlash->SetVisibility(true);
	MuzzleLight->SetVisibility(true);
	UpdateMuzzleFlash(0.f);
	RefreshTick();
}

void AWeaponBase::UpdateMuzzleFlash(float DeltaSeconds)
{
	if (FlashTimeLeft <= 0.f)
	{
		return;
	}
	FlashTimeLeft -= DeltaSeconds;
	if (FlashTimeLeft <= 0.f)
	{
		FlashTimeLeft = 0.f;
		MuzzleFlash->SetVisibility(false);
		MuzzleLight->SetVisibility(false);
		return;
	}
	// Full for the first part of the flash, then fading out.
	const float Fade = FMath::Clamp(FlashTimeLeft / FlashDuration * 1.6f, 0.f, 1.f) * FlashStrength;
	for (int32 Index = 0; Index < FlashGlow.Num() && Index < MuzzleFlash->GetInstanceCount(); ++Index)
	{
		MuzzleFlash->SetCustomDataValue(Index, 3, FlashGlow[Index] * Fade);
	}
	MuzzleFlash->MarkRenderStateDirty();
	MuzzleLight->SetIntensity(FlashLightCandelas * Fade * (Instance.Definition ? Instance.Definition->MuzzleFlashScale : 1.f));
}

float AWeaponBase::GetEffectiveSpread() const
{
	const AActor* Holder = GetOwner();
	const UPlayerLocomotionComponent* Locomotion = Holder ? Holder->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	return Instance.Stats.Spread * (Locomotion ? Locomotion->GetSpreadMultiplier() : 1.f);
}

// ---------------------------------------------------------------------------
// Ammo
// ---------------------------------------------------------------------------

float AWeaponBase::GetReloadProgress() const
{
	if (!bReloading)
	{
		return -1.f;
	}
	const FTimerManager& Timers = GetWorldTimerManager();
	const float Rate = Timers.GetTimerRate(ReloadTimer);
	return Rate > 0.f ? FMath::Clamp(Timers.GetTimerElapsed(ReloadTimer) / Rate, 0.f, 1.f) : 0.f;
}

float AWeaponBase::GetReloadBlend() const
{
	const float Progress = GetReloadProgress();
	if (Progress < 0.f)
	{
		return 0.f;
	}
	// Ease in over the first ~12% of the reload and out over the last ~12%.
	return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(FMath::Min(Progress, 1.f - Progress) * 8.f, 0.f, 1.f));
}

bool AWeaponBase::CanReload() const
{
	return !bReloading && GetReserveAmmo() > 0 && CurrentMagazine < Instance.Stats.MagazineSize;
}

void AWeaponBase::Reload()
{
	if (!CanReload())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(FireTimer);
	BurstShotsRemaining = 0;
	bReloading = true;
	RefreshTick();

	if (Instance.Definition)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Instance.Definition->ReloadSound, GetActorLocation());
	}

	const float Duration = FMath::Max(Instance.Stats.ReloadTime, 0.01f);
	OnReloadStarted.Broadcast(Duration);
	GetWorldTimerManager().SetTimer(ReloadTimer, this, &AWeaponBase::FinishReload, Duration, false);
}

void AWeaponBase::FinishReload()
{
	// Reloads draw from the holder's shared pool for this ammo class.
	UWeaponManagerComponent* Inventory = GetHolderInventory();
	const int32 Needed = Instance.Stats.MagazineSize - CurrentMagazine;
	CurrentMagazine += Inventory ? Inventory->TakeAmmo(GetAmmoType(), Needed) : 0;
	bReloading = false;
	RefreshTick();
	UpdateReloadPart();

	OnReloadFinished.Broadcast();
	BroadcastAmmo();

	// Holding the trigger through a reload resumes auto fire.
	if (bWantsToFire && Instance.Definition && Instance.Definition->FireMode == EWeaponFireMode::FullAuto)
	{
		StartFire();
	}
}

void AWeaponBase::CancelReload()
{
	GetWorldTimerManager().ClearTimer(ReloadTimer);
	bReloading = false;
	RefreshTick();
	UpdateReloadPart();
}

void AWeaponBase::UpdateReloadPart()
{
	const EWeaponReloadPart Part = GetReloadPart();
	if (Part == EWeaponReloadPart::None)
	{
		return;
	}
	const float Progress = GetReloadProgress();
	bool bVisible = true;
	float Travel = 0.f;
	if (Progress >= 0.f)
	{
		Travel = Part == EWeaponReloadPart::Magazine ? LooterReload::MagazineTravel(Progress, bVisible) : LooterReload::PumpTravel(Progress);
	}
	ModelPartMesh->SetRelativeLocation(ReloadPartAxis * Travel);
	ModelPartMesh->SetVisibility(bVisible);
}

void AWeaponBase::BroadcastAmmo()
{
	OnAmmoChanged.Broadcast(CurrentMagazine, GetReserveAmmo());
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
	ModelMesh->SetRelativeRotation(FRotator::ZeroRotator);

	AttachToHolder(AttachTo, Socket, AttachOffset);
	SetActorHiddenInGame(false);
	BroadcastAmmo();
}

void AWeaponBase::AttachToHolder(USceneComponent* AttachTo, FName Socket, const FTransform& AttachOffset)
{
	if (!AttachTo)
	{
		return;
	}
	AttachToComponent(AttachTo, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	SetActorRelativeTransform(AttachOffset);

	// Render like the holder: first-person arms get first-person rendering (no clipping, own FOV); a camera or a
	// third-person body gets normal world rendering.
	const UPrimitiveComponent* ParentPrimitive = Cast<UPrimitiveComponent>(AttachTo);
	const EFirstPersonPrimitiveType Type = ParentPrimitive && ParentPrimitive->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson
		? EFirstPersonPrimitiveType::FirstPerson : EFirstPersonPrimitiveType::None;
	SkeletalMesh->SetFirstPersonPrimitiveType(Type);
	StaticMesh->SetFirstPersonPrimitiveType(Type);
	ModelMesh->SetFirstPersonPrimitiveType(Type);
	ModelPartMesh->SetFirstPersonPrimitiveType(Type);
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
	ModelMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
	ModelPartMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
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

	// Code-built models have their origin at the back of the receiver; spin loot around the middle of the gun.
	const FVector ModelCenter(bUsingModel ? ModelMuzzle.X * 0.45f : 0.f, 0.f, 0.f);
	SpinMovement->PivotTranslation = bPickup ? ModelCenter : FVector::ZeroVector;
	ModelMesh->SetRelativeLocationAndRotation(bPickup ? -ModelCenter : FVector::ZeroVector, FRotator::ZeroRotator);

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
		StylizedSurfaces::SetupBeam(LootBeam, UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity),
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
	bUsingModel = Definition && Definition->ProceduralModel != EWeaponModel::None;
	const bool bUseSkeletal = !bUsingModel && Definition && Definition->SkeletalMesh;
	const bool bUseStatic = !bUsingModel && !bUseSkeletal;

	SkeletalMesh->SetSkeletalMesh(bUseSkeletal ? Definition->SkeletalMesh.Get() : nullptr);
	SkeletalMesh->SetVisibility(bUseSkeletal);

	StaticMesh->SetStaticMesh(bUseStatic && Definition ? Definition->StaticMesh.Get() : nullptr);
	StaticMesh->SetVisibility(bUseStatic);

	ModelMesh->SetVisibility(bUsingModel);
	ReloadPart = EWeaponReloadPart::None;
	if (bUsingModel)
	{
		WeaponModels::FPoints Points;
		WeaponModels::BuildInto(Instance, ModelMesh, ModelPartMesh, Points);
		ModelMuzzle = Points.Muzzle;
		ModelGrip = Points.Grip;
		ModelForegrip = Points.Foregrip;
		ReloadPart = Points.ReloadPart;
		ReloadPartAxis = Points.ReloadPartAxis;
	}
	ModelPartMesh->SetVisibility(ReloadPart != EWeaponReloadPart::None);
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

UPrimitiveComponent* AWeaponBase::GetActiveMesh() const
{
	if (bUsingModel)
	{
		return ModelMesh;
	}
	if (SkeletalMesh->GetSkeletalMeshAsset())
	{
		return SkeletalMesh;
	}
	return StaticMesh;
}

FVector AWeaponBase::GetMuzzleLocation() const
{
	if (bUsingModel)
	{
		return ModelMesh->GetComponentTransform().TransformPosition(ModelMuzzle);
	}
	const UPrimitiveComponent* Mesh = GetActiveMesh();
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
