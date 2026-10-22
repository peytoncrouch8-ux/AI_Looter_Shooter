#include "Weapons/WeaponBase.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/VfxRules.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponModelComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

// AWeaponBase's muzzle flash (moved out of WeaponBaseFiring.cpp by the VFX pass, 2026-10-22): the flash quads on the gun,
// shaped per gun kind (VfxRules::MuzzleLook: an SMG's quick four-point flicker, a shotgun's wide bloom, a revolver's
// cylinder spit, a sniper's brake jets, an LMG's heavy tongues, a launcher's gout), and what the shot throws round it in
// the world (FWeaponFX::SpawnMuzzleBurst: powder sparks, smoke, the heavy guns' blast ring). The point light is as before.

namespace
{
	// M_FX_Glow reads colour, strength and shape from each quad's custom data; shapes: 0 = streak, 1 = round, 2 = six-pointed star.
	constexpr float FlashCoreGlow = 26.f;
	constexpr float FlashStarGlow = 16.f;
	constexpr float FlashTongueGlow = 14.f;
	constexpr float FlashJetGlow = 16.f;
	constexpr float FlashSpitGlow = 12.f;
	constexpr float FlashLightCandelas = 24.f;
	const FLinearColor FlashCoreColor(1.f, 0.85f, 0.6f);
	const FLinearColor FlashStarColor(1.f, 0.62f, 0.25f);
	const FLinearColor FlashTongueColor(1.f, 0.58f, 0.22f);
	/** A pause this long (s) between shots starts a new burst (the rifle's climb has the same). */
	constexpr double MuzzleBurstGap = 0.35;
	/** A brake's jets sit this far behind the crown (cm at scale 1); a revolver's cylinder gap this far. */
	constexpr float BrakeBack = 3.f;
	constexpr float GapBack = 16.f;
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

	UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow"));
	if (!Glow)
	{
		Glow = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow"));
	}
	MuzzleFlash->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	MuzzleFlash->SetMaterial(0, Glow);
	MuzzleFlash->ClearInstances();
	MuzzleFlash->SetNumCustomDataFloats(5);

	// The kind's look: sizes, tongues, a brake's jets, a cylinder's spit, how long it shows.
	const FVfxMuzzleLook Look = VfxRules::MuzzleLook(Definition->Kind);
	FlashDuration = Look.Duration;

	// The engine plane is 100 x 100 in XY, facing +Z.
	// A gun drawn bigger than modeled (UWeaponModelComponent::ScaleFor) flashes as much bigger.
	const float Scale = Definition->MuzzleFlashScale * (bUsingModel ? Model->GetModelScale() : 1.f);
	FlashGlow.Reset();
	auto AddQuad = [this](const FTransform& Transform, const FLinearColor& Color, float Strength, float Shape)
	{
		const float Data[] = { Color.R, Color.G, Color.B, Strength, Shape };
		MuzzleFlash->SetCustomData(MuzzleFlash->AddInstance(Transform), Data);
		FlashGlow.Add(Strength);
	};
	// A hot round core and a spiky star just past the muzzle, facing back down the barrel at the shooter...
	const FRotator FacingBack(-90.f, 0.f, 0.f);
	AddQuad(FTransform(FacingBack, FVector(1.f, 0.f, 0.f), FVector(Look.CoreSize * Scale / 100.f, Look.CoreSize * Scale / 100.f, 1.f)), FlashCoreColor,
		FlashCoreGlow, 1.f);
	AddQuad(FTransform(FacingBack, FVector(3.f, 0.f, 0.f), FVector(Look.StarSize * Scale / 100.f, Look.StarSize * Scale / 100.f, 1.f)), FlashStarColor,
		FlashStarGlow, 2.f);
	// ...flame tongues along the barrel, spread evenly about it, for side views (turned so the streak's bright head is at
	// the muzzle end)...
	const float Length = Look.TongueLength * Scale;
	const int32 Tongues = FMath::Max(Look.Tongues, 1);
	for (int32 Tongue = 0; Tongue < Tongues; ++Tongue)
	{
		AddQuad(FTransform(FRotator(0.f, 180.f, Tongue * 180.f / Tongues), FVector(Length * 0.5f, 0.f, 0.f),
			FVector(Length / 100.f, Look.TongueWidth * Scale / 100.f, 1.f)), FlashTongueColor, FlashTongueGlow, 0.f);
	}
	// ...a muzzle brake's two jets blown out square to the barrel...
	if (Look.SideJets > 0.f)
	{
		const float Jet = Look.SideJets * Scale;
		for (const float Side : { -1.f, 1.f })
		{
			AddQuad(FTransform(FRotator(0.f, -90.f * Side, 0.f), FVector(-BrakeBack * Scale, Side * Jet * 0.5f, 0.f),
				FVector(Jet / 100.f, 8.f * Scale / 100.f, 1.f)), FlashTongueColor, FlashJetGlow, 0.f);
		}
	}
	// ...and a revolver's spit out of the cylinder gap, either side.
	if (Look.GapSpit > 0.f)
	{
		const float Spit = Look.GapSpit * Scale;
		for (const float Side : { -1.f, 1.f })
		{
			AddQuad(FTransform(FacingBack, FVector(-GapBack * Scale, Side * 2.f * Scale, 0.f), FVector(Spit / 100.f, Spit / 100.f, 1.f)), FlashCoreColor,
				FlashSpitGlow, 1.f);
		}
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
	Muzzle.Scale = Instance.Definition->MuzzleFlashScale * (bUsingModel ? Model->GetModelScale() : 1.f);
	Muzzle.bFirstPerson = IsDrawnFirstPerson();
	Muzzle.InBurst = MuzzleShotsInBurst;
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
	FlashTimeLeft -= DeltaSeconds;
	if (FlashTimeLeft <= 0.f)
	{
		FlashTimeLeft = 0.f;
		MuzzleFlash->SetVisibility(false);
		MuzzleLight->SetVisibility(false);
		return;
	}
	// Full for the first part of the flash, then fading out.
	const float Fade = FMath::Clamp(FlashTimeLeft / FMath::Max(FlashDuration, 0.01f) * 1.6f, 0.f, 1.f) * FlashStrength;
	for (int32 Index = 0; Index < FlashGlow.Num() && Index < MuzzleFlash->GetInstanceCount(); ++Index)
	{
		MuzzleFlash->SetCustomDataValue(Index, 3, FlashGlow[Index] * Fade);
	}
	MuzzleFlash->MarkRenderStateDirty();
	MuzzleLight->SetIntensity(FlashLightCandelas * Fade * (Instance.Definition ? Instance.Definition->MuzzleFlashScale : 1.f));
}
