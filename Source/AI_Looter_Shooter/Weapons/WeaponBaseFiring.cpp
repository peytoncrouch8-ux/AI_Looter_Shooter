#include "Weapons/WeaponBase.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponCurseEffects.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/PartEffectComponent.h"
#include "Weapons/WeaponSounds.h"
#include "Weapons/WeaponTrigger.h"
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

	/** How big a misfire's spit of sparks and smoke is beside a shot's flash in the open (FWeaponFX::SpawnFlash). */
	constexpr float MisfirePuffScale = 0.25f;
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------

void AWeaponBase::StartFire()
{
	const UWorld* World = GetWorld();
	StartFireAt(World ? World->GetTimeSeconds() : 0.0);
}

void AWeaponBase::StartFireAt(double Now)
{
	bWantsToFire = true;

	// Mid-strike the trigger is only remembered: auto fire takes up again as the swing ends (EndMeleeSwing).
	if (bReloading || bMeleeSwinging || !Instance.Definition)
	{
		return;
	}
	// An LMG starts to spin (heard from cold).
	StartKindTrigger();

	// Respect the fire rate however the trigger is worked. A press while the gun is still cycling is remembered only if it
	// comes just before the gun is ready, and only once (WeaponTrigger::Decide): clicks made earlier are lost, so spamming
	// never stacks shots that go off after the clicking has stopped.
	FTimerManager& Timers = GetWorldTimerManager();
	// A part effect's say over the trigger (Phase 12.3, WeaponTrigger::PlanFor): a Burst part's three rounds and pause, an
	// Overcharge part's wind-up at half the rate; any other gun's plan is its definition's.
	const WeaponTrigger::FTriggerPlan Trigger = WeaponTrigger::PlanFor(Instance);
	// In the world's seconds: a shooter living faster than the world (Sundown) cycles the gun in fewer of them. An LMG
	// still spinning up fires slower (GetShotInterval).
	const double Interval = GetShotInterval() * Trigger.PullIntervalScale / GetOwnerTimeScale();
	const double SecondsUntilReady = Interval - (Now - LastFireTime);
	const WeaponTrigger::EPress Press = WeaponTrigger::Decide(Trigger.FireMode, SecondsUntilReady, Timers.IsTimerActive(FireTimer));
	// An Overcharge pull winds up from the press itself: one made before the gun is ready is lost, never a charge that
	// starts late on its own.
	if (Press == WeaponTrigger::EPress::Ignore || (Trigger.ChargeSeconds > 0.f && Press != WeaponTrigger::EPress::FireNow))
	{
		return;
	}

	if (Trigger.FireMode == EWeaponFireMode::Burst)
	{
		BurstShotsRemaining = Trigger.BurstCount;
	}

	if (Press == WeaponTrigger::EPress::FireNow && Trigger.ChargeSeconds > 0.f)
	{
		// Held, it fires charged (x2) when the wind-up is done; let go sooner, it fires what it has (StopFire).
		UPartEffectComponent::For(*this)->BeginCharge();
		Timers.SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, Trigger.ChargeSeconds / GetOwnerTimeScale(), false);
	}
	else if (Press == WeaponTrigger::EPress::FireNow)
	{
		HandleFiring();
	}
	else
	{
		Timers.SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, static_cast<float>(SecondsUntilReady), false);
	}
}

bool AWeaponBase::HasShotPending() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(FireTimer);
}

void AWeaponBase::StopFire()
{
	bWantsToFire = false;
	StopKindTrigger();

	// Bursts finish on their own and semi-auto keeps its one remembered press (made just before the gun was ready), so only
	// auto stops immediately.
	// Without a world (its holder ending play as the world is collected) there's no timer left to clear: asking for the
	// timer manager then crashed the editor between test batches.
	UWorld* World = GetWorld();
	if (!World || !Instance.Definition)
	{
		return;
	}
	// An Overcharge gun let go mid wind-up fires at the charge it reached (UPartEffectComponent::TakeChargeScale).
	UPartEffectComponent* Parts = UPartEffectComponent::Find(this);
	if (Parts && Parts->IsCharging() && World->GetTimerManager().IsTimerActive(FireTimer))
	{
		World->GetTimerManager().ClearTimer(FireTimer);
		HandleFiring();
		return;
	}
	if (WeaponTrigger::PlanFor(Instance).FireMode == EWeaponFireMode::FullAuto)
	{
		World->GetTimerManager().ClearTimer(FireTimer);
	}
}

void AWeaponBase::HandleFiring()
{
	if (bReloading || bMeleeSwinging || !Instance.Definition)
	{
		return;
	}

	if (CurrentMagazine <= 0)
	{
		PlayCue(WeaponSounds::DryFire(Instance.Definition->Kind), Instance.Definition->DryFireSound);
		BurstShotsRemaining = 0;
		Reload();
		return;
	}

	// A part effect's plan for this pull (WeaponTrigger::PlanFor: a Burst part's rounds and their gap; StartFireAt winds an
	// Overcharge's charge, which the bullets take as the shot goes: UPartEffectSubsystem).
	const WeaponTrigger::FTriggerPlan Trigger = WeaponTrigger::PlanFor(Instance);
	// A cursed iron's drawback acts on the trigger: Greedy spends two rounds a shot (or the last one alone), and Unlucky
	// now and then drops the hammer on a dud. Either way the shot takes its time and its rounds.
	const WeaponCurseEffects::FShotPlan Plan = WeaponCurseEffects::PlanShot(Instance, CurrentMagazine, ShotRolls);
	if (Plan.bMisfire)
	{
		Misfire();
	}
	else
	{
		FireShot();
	}
	// The wind-up is spent with the shot (a bullet took its charge; a dud or a rocket lets it go).
	if (UPartEffectComponent* Parts = UPartEffectComponent::Find(this))
	{
		Parts->CancelCharge();
	}
	CurrentMagazine = FMath::Max(CurrentMagazine - Plan.Rounds, 0);
	// A six-gun keeps its empties (a dud's too) until a reload tips them out.
	SpentInCylinder = FMath::Min(SpentInCylinder + Plan.Rounds, Instance.Stats.MagazineSize);
	LastFireTime = GetWorld()->GetTimeSeconds();
	// The frontier-tech kinds' after-shot: the sniper's bolt cycles (a dud's too), the LMG heats, the launcher's mouth empties.
	NoteKindShot(Plan.bMisfire);
	// A revolver's cylinder turns the next round under the hammer (a dud's too), as the gun settles from the kick.
	if (bUsingModel && Model->GetCylinderChambers() > 0)
	{
		Model->TurnCylinder(Plan.Rounds);
		RefreshTick();
	}

	BroadcastAmmo();
	if (!Plan.bMisfire)
	{
		// A dud doesn't kick, and the crosshair doesn't bloom for it.
		OnFired.Broadcast();
		// The click of the magazine's last rounds (the last one is the reload's own, so none then).
		GunshotSound::PlayLowAmmo(*this, CurrentMagazine, Instance.Stats.MagazineSize);
	}

	if (CurrentMagazine <= 0)
	{
		BurstShotsRemaining = 0;
		Reload();
		return;
	}

	bool bContinue = false;
	switch (Trigger.FireMode)
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
		const float Gap = Trigger.FireMode == EWeaponFireMode::Burst ? Trigger.BurstIntervalScale : 1.f;
		GetWorldTimerManager().SetTimer(FireTimer, this, &AWeaponBase::HandleFiring, GetShotInterval() * Gap / GetOwnerTimeScale(), false);
	}
}

void AWeaponBase::FireShot()
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetAimViewPoint(ViewLocation, ViewRotation);
	const FVector AimDirection = ViewRotation.Vector();
	UE_LOG(LogLooter, VeryVerbose, TEXT("%s shot from %s along %s"), *GetName(), *ViewLocation.ToCompactString(), *AimDirection.ToCompactString());

	// Every pellet is a real bullet that flies from the aim point along the aim (the tracer leaves from the muzzle). A
	// launcher's shot is a rocket instead (AWeaponRocket), off the same aim, its cone the gun's spread as a pellet's.
	const UWeaponDefinition* Definition = Instance.Definition;
	if (Definition->Kind == EWeaponKind::Launcher)
	{
		const float SpreadRadians = FMath::DegreesToRadians(GetEffectiveSpread());
		FireRocket(ViewLocation, SpreadRadians > 0.f ? FMath::VRandCone(AimDirection, SpreadRadians) : AimDirection);
	}
	else if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		FBulletShot Shot;
		Shot.Start = ViewLocation;
		Shot.VisualStart = GetVisibleMuzzleLocation();
		Shot.Speed = Definition->BulletSpeed;
		Shot.Range = Instance.Stats.Range * FWeaponStats::MaxRangeFactor;
		Shot.FalloffStats = Instance.Stats;
		Shot.Damage = Instance.Stats.Damage;
		// A cursed iron's critical hits (Unlucky's triple, Cold's extra) when larger than the kind's own: the revolver's
		// 1.75x, the sniper's 2x, the game's 1.5x for the rest.
		Shot.CritMultiplier = GunfeelKick::CritMultiplier(Definition->Kind, WeaponCurses::CritMultiplier(Instance));
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
	// The next shot's cone is wider (a rifle's bloom).
	AddShotBloom();

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
	GunshotSound::PlayShot(*this);
	// What a shot leaves: its brass, and heat in the barrel that smokes once firing stops.
	EjectAfterShot();
	NoteShotHeat();
}

void AWeaponBase::Misfire()
{
	// No bullet and no flash: the primer pops weakly, spitting a few sparks and a wisp of smoke from the muzzle as the
	// player sees it.
	if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		GetAimViewPoint(ViewLocation, ViewRotation);
		Bullets->GetEffects().Initialize(GetWorld());
		Bullets->GetEffects().SpawnFlash(GetVisibleMuzzleLocation(), ViewRotation.Vector(), MisfirePuffScale);
	}
	PlayCue(LooterSoundCue::Misfire);
	UE_LOG(LogLooter, Verbose, TEXT("%s misfired"), *GetName());
}

void AWeaponBase::PlayCue(FName Cue, USoundBase* Override) const
{
	// A definition that sets a sound of its own keeps it.
	if (Override)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Override, GetMuzzleLocation());
		return;
	}
	// On the gun, so it follows it: the player's own shots stay with the view as it turns.
	LooterSound::PlayAttached(Cue, GetRootComponent());
}

void AWeaponBase::PlayAimIn() const
{
	PlayCue(LooterSoundCue::AimIn);
}

void AWeaponBase::NotifyBulletHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	OnHit.Broadcast(Hit, Damage, bCritical);
}

FVector AWeaponBase::GetVisibleMuzzleLocation() const
{
	return GetVisibleLocation(GetMuzzleLocation());
}

FVector AWeaponBase::GetVisibleLocation(const FVector& WorldLocation) const
{
	const UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(GetOwner());
	if (!Camera || !IsDrawnFirstPerson())
	{
		return WorldLocation;
	}
	// First-person guns are drawn with their own field of view and pulled in toward the camera: find where the point
	// actually shows, the way the renderer does (the muzzle's flash and tracers, the port's brass).
	FMinimalViewInfo View;
	View.Location = Camera->GetComponentLocation();
	View.Rotation = Camera->GetComponentRotation();
	View.FOV = Camera->FieldOfView;
	View.FirstPersonFOV = Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : Camera->FieldOfView;
	View.FirstPersonScale = Camera->bEnableFirstPersonScale ? Camera->FirstPersonScale : 1.f;
	View.bUseFirstPersonParameters = Camera->bEnableFirstPersonFieldOfView || Camera->bEnableFirstPersonScale;
	return View.bUseFirstPersonParameters ? View.TransformWorldToFirstPerson(WorldLocation, false) : WorldLocation;
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
	// A gun drawn bigger than modeled (UWeaponModelComponent::ScaleFor) flashes as much bigger.
	const float Scale = Definition->MuzzleFlashScale * (bUsingModel ? Model->GetModelScale() : 1.f);
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
	// Bigger in the player's own view (the first-person gun is drawn small and close); from behind it is as before.
	MuzzleFlash->SetRelativeScale3D(FVector(FMath::FRandRange(0.85f, 1.2f) * (IsDrawnFirstPerson() ? GunfeelKick::Muzzle1PScale() : 1.f)));
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
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	// The sniper is a scope gun: its spread is its hip spread until the scope is up, then all but none (no halving mercy).
	const bool bSniper = Instance.Definition && Instance.Definition->Kind == EWeaponKind::Sniper;
	const float Aimed = !View ? 1.f : (bSniper ? GunKinds::AimSpreadScale(EWeaponKind::Sniper, View->GetAimAlpha(),
		UPlayerViewComponent::AimSpreadMultiplier, GunKinds::SniperAimedSpread()) : View->GetAimSpreadMultiplier());
	return Instance.Stats.Spread * (Locomotion ? Locomotion->GetSpreadMultiplier() : 1.f) * Aimed * (1.f + GetSpreadBloom());
}

float AWeaponBase::GetOwnerTimeScale() const
{
	const AActor* Carrier = GetOwner();
	return Carrier && Carrier->CustomTimeDilation > UE_KINDA_SMALL_NUMBER ? Carrier->CustomTimeDilation : 1.f;
}

float AWeaponBase::GetReadyAlpha() const
{
	const UWorld* World = GetWorld();
	return World && ReadySeconds > 0.f ? FMath::Clamp(static_cast<float>(World->GetTimeSeconds() - DrawnTime) / ReadySeconds, 0.f, 1.f) : 1.f;
}

FVector AWeaponBase::GetAimPoint() const
{
	return bUsingModel ? Model->GetRelativeTransform().TransformPosition(Model->GetAimPoint()) : FVector(30.f, 0.f, 8.f);
}
