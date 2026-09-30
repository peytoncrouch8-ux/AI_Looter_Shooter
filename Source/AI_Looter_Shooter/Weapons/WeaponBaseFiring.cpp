#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "UI/World/WeaponLabelWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponModelComponent.h"
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
			UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->MuzzleFlashFX, Model, NAME_None,
				Model->GetMuzzle(), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
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
	const UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(GetOwner());
	if (!Camera || !IsDrawnFirstPerson())
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
	USceneComponent* Mesh = GetActiveMesh();
	if (!Definition || !Mesh)
	{
		return;
	}

	// Sit on the muzzle of whichever model shows, pointing down the barrel (+X).
	const bool bSocket = !bUsingModel && Mesh->DoesSocketExist(Definition->MuzzleSocket);
	const FName Socket = bSocket ? Definition->MuzzleSocket : NAME_None;
	const FVector Offset = bUsingModel ? Model->GetMuzzle() : FVector::ZeroVector;
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
