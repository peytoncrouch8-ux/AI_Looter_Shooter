#include "Weapons/WeaponBase.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponModelComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

// AWeaponBase's muzzle flash (moved out of WeaponBaseFiring.cpp by the VFX pass; reworked 2026-10-22, the user: the hip-fire
// flash showed flat yellow diamonds up close that covered the target). The flash is layered light on M_FX_Muzzle (a white-hot
// core, a soft orange bloom, a star of thin tapered spikes, tongues along the barrel; VfxRules::MuzzleQuads by the gun's
// kind), placed again each shot for its view: in first person each layer has its own size (the tongues short: along the
// barrel is toward the crosshair), it shrinks to keep clear of the crosshair and the material fades what still reaches in.
// It lasts about two frames. FWeaponFX::SpawnMuzzleBurst throws the sparks and smoke round it. The point light is as before.

namespace
{
	/** The flash's master (Tools/Unreal/build_vfx_materials.py), then the old glow (the same shape numbers) until it's built. */
	const TCHAR* MuzzleMaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_Muzzle.M_FX_Muzzle");
	const TCHAR* MuzzleGlowFallbackPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* MuzzleEnvironmentGlowPath = TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow");
	/** M_FX_Muzzle's custom data: 0-2 colour, 3 strength, 4 shape, 5 param, 6 seed, 7 the crosshair's clear share (0 none). */
	constexpr int32 FlashFloats = 8;
	constexpr int32 FlashStrengthIndex = 3;
	constexpr float FlashLightCandelas = 24.f;
	/** Each shot's flash this much stronger or weaker at random. */
	constexpr float FlashStrengthLow = 0.85f;
	constexpr float FlashStrengthHigh = 1.1f;
	/** A pause this long (s) between shots starts a new burst (the rifle's climb has the same). */
	constexpr double MuzzleBurstGap = 0.35;

	/** Degrees off the view's middle Point is seen at from Eye looking along Axis. */
	float DegreesOff(const FVector& Eye, const FVector& Axis, const FVector& Point)
	{
		const FVector Ray = (Point - Eye).GetSafeNormal();
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(FVector::DotProduct(Ray, Axis)), -1.f, 1.f)));
	}
}

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

	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, MuzzleMaterialPath);
	if (!Glow)
	{
		Glow = LoadObject<UMaterialInterface>(nullptr, MuzzleGlowFallbackPath);
	}
	if (!Glow)
	{
		Glow = LoadObject<UMaterialInterface>(nullptr, MuzzleEnvironmentGlowPath);
	}
	MuzzleFlash->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	MuzzleFlash->SetMaterial(0, Glow);
	MuzzleFlash->ClearInstances();
	MuzzleFlash->SetNumCustomDataFloats(FlashFloats);

	// The kind's look: its layers, how long it shows.
	const FVfxMuzzleLook Look = VfxRules::MuzzleLook(Definition->Kind);
	FlashBaseDuration = Look.Duration;
	FlashDuration = Look.Duration;
	// A gun drawn bigger than modeled (UWeaponModelComponent::ScaleFor) flashes as much bigger.
	FlashGunScale = Definition->MuzzleFlashScale * (bUsingModel ? Model->GetModelScale() : 1.f);
	FlashQuads.Reset();
	VfxRules::MuzzleQuads(Look, FlashQuads);

	// One instance a quad, placed as seen from behind until the first shot places them for its view.
	const FVfxMuzzleView Behind;
	for (const FVfxFlashQuad& Quad : FlashQuads)
	{
		const int32 Index = MuzzleFlash->AddInstance(VfxRules::MuzzleQuadTransform(Quad, VfxRules::MuzzleLayerScale(Quad.Layer, Behind), FlashGunScale));
		const float Data[FlashFloats] = { Quad.Color.R, Quad.Color.G, Quad.Color.B, Quad.Glow, static_cast<float>(Quad.Shape), Quad.Param, 0.f, 0.f };
		MuzzleFlash->SetCustomData(Index, Data);
	}
	FlashTimeLeft = 0.f;
	MuzzleFlash->SetVisibility(false);
	MuzzleLight->SetVisibility(false);
}

void AWeaponBase::PlayMuzzleFlash()
{
	if (MuzzleFlash->GetInstanceCount() == 0 || FlashQuads.Num() != MuzzleFlash->GetInstanceCount())
	{
		return;
	}
	const FVfxTuning Vfx = FVfxTuning::Live();
	const bool bFirstPerson = IsDrawnFirstPerson();
	const AActor* Holder = GetOwner();
	const UCameraComponent* Eye = bFirstPerson ? UPlayerViewComponent::FindFirstPersonCamera(Holder) : nullptr;
	const UPlayerViewComponent* Sights = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const float Aim = bFirstPerson && Sights ? Sights->GetAimAlpha() : 0.f;
	const FVfxMuzzleView View = VfxRules::MuzzleView(bFirstPerson, Aim, Vfx);

	// Every flash a little different: turned, sized and bright at random, about two frames long.
	FlashDuration = FlashBaseDuration * Vfx.MuzzleTime;
	FlashTimeLeft = FlashDuration;
	FlashFrame = GFrameCounter;
	FlashStrength = FMath::FRandRange(FlashStrengthLow, FlashStrengthHigh) * View.Glow * Vfx.MuzzleGlow;
	MuzzleFlash->SetRelativeRotation(FRotator(0.f, 0.f, FMath::FRandRange(0.f, 360.f)));
	// Bigger in the player's own view (the first-person gun is drawn small and close); each layer then as its view has it.
	float Size = (1.f + FMath::FRandRange(-Vfx.MuzzleJitter, Vfx.MuzzleJitter)) * View.Size * (bFirstPerson ? GunfeelKick::Muzzle1PScale() : 1.f);
	MuzzleFlash->SetRelativeScale3D(FVector(FMath::Max(Size, 0.01f)));
	TArray<FTransform> Placed;
	Placed.Reserve(FlashQuads.Num());
	for (const FVfxFlashQuad& Quad : FlashQuads)
	{
		Placed.Add(VfxRules::MuzzleQuadTransform(Quad, VfxRules::MuzzleLayerScale(Quad.Layer, View), FlashGunScale));
	}
	MuzzleFlash->BatchUpdateInstancesTransforms(0, Placed, /*bWorldSpace*/ false, /*bMarkRenderStateDirty*/ true, /*bTeleport*/ true);

	// In the player's own view, keep clear of the crosshair: shrink the flash till its nearest edge (the star's or bloom's
	// toward the middle, the tongues' tips along the barrel) stays Clear degrees out, as the renderer shows the first-person
	// gun; the material fades whatever still reaches in. The way out from the middle sends the sparks and smoke off too.
	float ClearShare = 0.f;
	FVector Away = FVector::ZeroVector;
	if (Eye)
	{
		const float Clear = FMath::Lerp(Vfx.MuzzleClear, Vfx.MuzzleAimClear, Aim);
		const FVector EyeAt = Eye->GetComponentLocation();
		const FVector Axis = Eye->GetForwardVector();
		const FVector Muzzle = GetMuzzleLocation();
		const FVector Seen = GetVisibleLocation(Muzzle);
		const FVector Ray = (Seen - EyeAt).GetSafeNormal();
		FVector Toward = Axis - Ray * FVector::DotProduct(Axis, Ray);
		if (Toward.Normalize())
		{
			Away = -Toward;
			FVfxMuzzleView Unit = View;
			Unit.Size = 1.f;
			const float World = static_cast<float>(MuzzleFlash->GetComponentScale().X) * FlashGunScale;
			const float Across = VfxRules::MuzzleReach(FlashQuads, Unit, false) * World;
			const float Along = VfxRules::MuzzleReach(FlashQuads, Unit, true) * World;
			const float MuzzleDegrees = DegreesOff(EyeAt, Axis, Seen);
			const float Nearest = FMath::Min(DegreesOff(EyeAt, Axis, GetVisibleLocation(Muzzle + Toward * Across)),
				DegreesOff(EyeAt, Axis, GetVisibleLocation(Muzzle + MuzzleFlash->GetForwardVector() * Along)));
			const float Fit = VfxRules::MuzzleClearScale(MuzzleDegrees, Nearest, Clear, Vfx.MuzzleClearMin);
			if (Fit < 1.f)
			{
				Size *= Fit;
				MuzzleFlash->SetRelativeScale3D(FVector(FMath::Max(Size, 0.01f)));
			}
		}
		ClearShare = VfxRules::MuzzleClearShare(Clear, Eye->FieldOfView);
	}
	// Each shot's own: the star's turn and uneven spikes (its seed), the tongues' lengths, the crosshair's hole.
	for (int32 Index = 0; Index < FlashQuads.Num(); ++Index)
	{
		const FVfxFlashQuad& Quad = FlashQuads[Index];
		const float Param = Quad.Shape == EVfxFlashShape::Streak ? VfxRules::MuzzleStreakShare(FMath::FRand()) : Quad.Param;
		MuzzleFlash->SetCustomDataValue(Index, 5, Param);
		MuzzleFlash->SetCustomDataValue(Index, 6, FMath::FRand());
		MuzzleFlash->SetCustomDataValue(Index, 7, ClearShare);
	}
	MuzzleFlash->SetVisibility(true);
	MuzzleLight->SetVisibility(Vfx.MuzzleLight > 0.f);
	UpdateMuzzleFlash(0.f);
	RefreshTick();

	// What the shot throws round its flash in the world: sparks, smoke, a heavy gun's blast ring (FWeaponFX).
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets || !Instance.Definition)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	MuzzleShotsInBurst = Now - LastMuzzleTime <= MuzzleBurstGap ? MuzzleShotsInBurst + 1 : 0;
	LastMuzzleTime = Now;
	FVfxMuzzle Muzzle;
	Muzzle.Location = GetVisibleMuzzleLocation();
	// The flash is turned at random about its X each shot: its forward stays the barrel's.
	Muzzle.Forward = MuzzleFlash->GetForwardVector();
	Muzzle.Up = MuzzleFlash->GetUpVector();
	Muzzle.Kind = Instance.Definition->Kind;
	Muzzle.Scale = FlashGunScale;
	Muzzle.bFirstPerson = bFirstPerson;
	Muzzle.InBurst = MuzzleShotsInBurst;
	Muzzle.Away = Away;
	if (UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this))
	{
		Muzzle.DustLight = Reacts->GetDustLight();
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	Effects.SpawnMuzzleBurst(Muzzle);
}

void AWeaponBase::UpdateMuzzleFlash(float DeltaSeconds)
{
	if (FlashTimeLeft <= 0.f)
	{
		return;
	}
	// Not aged on the frame it was fired (that frame's tick may come after the shot): every flash is seen once at full.
	if (DeltaSeconds > 0.f && GFrameCounter != FlashFrame)
	{
		FlashTimeLeft -= DeltaSeconds;
	}
	if (FlashTimeLeft <= 0.f)
	{
		FlashTimeLeft = 0.f;
		MuzzleFlash->SetVisibility(false);
		MuzzleLight->SetVisibility(false);
		return;
	}
	// Full at once, then gone fast ((1 - share) ^ Looter.Vfx.Muzzle.FadePower).
	const FVfxTuning Vfx = FVfxTuning::Live();
	const float Fade = VfxRules::MuzzleFade(FlashDuration - FlashTimeLeft, FlashDuration, Vfx.MuzzleFadePower) * FlashStrength;
	for (int32 Index = 0; Index < FlashQuads.Num() && Index < MuzzleFlash->GetInstanceCount(); ++Index)
	{
		MuzzleFlash->SetCustomDataValue(Index, FlashStrengthIndex, FlashQuads[Index].Glow * Fade);
	}
	MuzzleFlash->MarkRenderStateDirty();
	MuzzleLight->SetIntensity(FlashLightCandelas * Fade * Vfx.MuzzleLight * (Instance.Definition ? Instance.Definition->MuzzleFlashScale : 1.f));
}
