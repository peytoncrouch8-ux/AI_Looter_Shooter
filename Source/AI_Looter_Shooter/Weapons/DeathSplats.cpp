#include "Weapons/DeathSplats.h"
#include "AI_Looter_Shooter.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// FDeathSplats: the ground splats under kills and blasts (the VFX pass), a pool of decals in M_FX_SplatDecal.

const TCHAR* FDeathSplats::MaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_SplatDecal.M_FX_SplatDecal");

namespace
{
	/** M_FX_SplatDecal's parameters (build_vfx_materials.py): how far it has spread (0..1), its shape's seed, its wetness. */
	const FName SplatSpreadParam(TEXT("Spread"));
	const FName SplatSeedParam(TEXT("Seed"));
	const FName SplatWetParam(TEXT("Wet"));
	/** How deep the decal reaches into the ground and over it (cm, half): enough for a slope, never up a wall. */
	constexpr float SplatDepth = 30.f;
	/** A splat smaller on screen than this fades away (the bullet marks' rule). */
	constexpr float SplatFadeScreenSize = 0.004f;
	/** A splat starts at this share of its size (the first gout), then spreads. */
	constexpr float SplatFirst = 0.35f;
}

FVfxSplatLook FDeathSplats::LookFor(EVfxSplat Kind, const FLinearColor& Tint)
{
	FVfxSplatLook Look;
	switch (Kind)
	{
	case EVfxSplat::Ichor:
		Look.Color = FLinearColor(0.1f, 0.14f, 0.015f);
		Look.Opacity = 0.85f;
		Look.Wet = 0.85f;
		Look.Radius = 70.f;
		Look.SpreadSeconds = 1.4f;
		break;
	case EVfxSplat::DarkIchor:
		Look.Color = FLinearColor(0.06f, 0.07f, 0.03f);
		Look.Opacity = 0.9f;
		Look.Wet = 0.9f;
		Look.Radius = 85.f;
		Look.SpreadSeconds = 1.8f;
		break;
	case EVfxSplat::Gel:
		// The slime's own colour, a little deeper on the ground, glossy.
		Look.Color = Tint * 0.6f;
		Look.Opacity = 0.75f;
		Look.Wet = 1.f;
		Look.Radius = 80.f;
		Look.SpreadSeconds = 0.5f;
		break;
	case EVfxSplat::Ash:
		Look.Color = FLinearColor(0.26f, 0.25f, 0.24f);
		Look.Opacity = 0.7f;
		Look.Wet = 0.f;
		Look.Radius = 65.f;
		Look.SpreadSeconds = 1.2f;
		break;
	case EVfxSplat::Scorch:
	default:
		Look.Color = FLinearColor(0.03f, 0.025f, 0.02f);
		Look.Opacity = 0.85f;
		Look.Wet = 0.f;
		Look.Radius = 120.f;
		Look.SpreadSeconds = 0.08f;
		break;
	}
	return Look;
}

float FDeathSplats::SpreadAt(float Seconds, float SpreadSeconds)
{
	if (SpreadSeconds <= 0.f)
	{
		return 1.f;
	}
	const float U = 1.f - FMath::Clamp(Seconds / SpreadSeconds, 0.f, 1.f);
	return FMath::Lerp(SplatFirst, 1.f, 1.f - U * U);
}

bool FDeathSplats::Initialize(UWorld& World)
{
	if (!Material.IsValid())
	{
		if (bMaterialMissing)
		{
			return false;
		}
		Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Material.IsValid())
		{
			bMaterialMissing = true;
			UE_LOG(LogLooter, Log, TEXT("No ground splats: %s isn't made yet (Tools/Unreal/build_vfx_materials.py)."), MaterialPath);
			return false;
		}
	}
	if (Owner.IsValid())
	{
		return true;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World.SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Actor)
	{
		return false;
	}
#if WITH_EDITOR
	Actor->SetActorLabel(TEXT("DeathSplats"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Owner = Actor;
	Splats.Reset();
	return true;
}

bool FDeathSplats::Add(UWorld& World, const FVector& Ground, const FVector& Normal, EVfxSplat Kind, const FLinearColor& Tint, float Size, float Hold,
	float Fade, int32 Max, float Pick)
{
	if (Max <= 0 || !Initialize(World))
	{
		return false;
	}
	const double Now = World.GetTimeSeconds();
	// An ended one's slot first, else a new one while there's room, else the oldest (it's nearly gone anyway).
	int32 Slot = INDEX_NONE;
	int32 Oldest = INDEX_NONE;
	for (int32 Index = 0; Index < Splats.Num(); ++Index)
	{
		if (!Splats[Index].Decal.IsValid() || Splats[Index].Ends <= Now)
		{
			Slot = Index;
			break;
		}
		if (Oldest == INDEX_NONE || Splats[Index].Born < Splats[Oldest].Born)
		{
			Oldest = Index;
		}
	}
	if (Slot == INDEX_NONE)
	{
		Slot = Splats.Num() < Max ? Splats.AddDefaulted() : Oldest;
	}
	if (Slot == INDEX_NONE)
	{
		return false;
	}
	FSplat& Splat = Splats[Slot];
	UDecalComponent* Decal = Splat.Decal.Get();
	if (!Decal)
	{
		AActor* Actor = Owner.Get();
		Decal = NewObject<UDecalComponent>(Actor, NAME_None, RF_Transient);
		Decal->SetupAttachment(Actor->GetRootComponent());
		Decal->SetMobility(EComponentMobility::Movable);
		Decal->SetUsingAbsoluteLocation(true);
		Decal->SetUsingAbsoluteRotation(true);
		Decal->SetUsingAbsoluteScale(true);
		UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(Material.Get(), Actor);
		Decal->SetDecalMaterial(Look ? static_cast<UMaterialInterface*>(Look) : Material.Get());
		Decal->FadeScreenSize = SplatFadeScreenSize;
		// The pool keeps its decals: never let the engine's fade destroy one (or the pool's actor).
		Decal->bDestroyOwnerAfterFade = false;
		Decal->SetVisibility(false);
		Decal->RegisterComponent();
		Splat.Decal = Decal;
		Splat.Look = Look;
	}

	const FVfxSplatLook Look = LookFor(Kind, Tint);
	const float Radius = Look.Radius * FMath::Clamp(Size / 100.f, 0.3f, 4.f) * FMath::Lerp(0.85f, 1.15f, FMath::Frac(Pick * 7.31f));
	const FVector Up = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	// The decal projects along its X: down into the ground, turned at random about it.
	FVector Turn = FVector::CrossProduct(Up, FVector::ForwardVector);
	if (!Turn.Normalize())
	{
		Turn = FVector::RightVector;
	}
	Turn = Turn.RotateAngleAxis(360.f * Pick, Up);
	Decal->SetWorldLocationAndRotation(Ground, FRotationMatrix::MakeFromXZ(-Up, Turn).Rotator());
	Decal->DecalSize = FVector(SplatDepth, Radius, Radius);
	Decal->DecalColor = FLinearColor(Look.Color.R, Look.Color.G, Look.Color.B, Look.Opacity);
	if (UMaterialInstanceDynamic* Dynamic = Splat.Look.Get())
	{
		Dynamic->SetScalarParameterValue(SplatSeedParam, Pick);
		Dynamic->SetScalarParameterValue(SplatWetParam, Look.Wet);
		Dynamic->SetScalarParameterValue(SplatSpreadParam, SpreadAt(0.f, Look.SpreadSeconds));
	}
	// Set straight on the decal, never through SetFadeOut (which would destroy it as it ends): its proxy, rebuilt below,
	// starts the fade's clock now (the bullet marks' way).
	Decal->FadeStartDelay = FMath::Max(Hold, 0.f);
	Decal->FadeDuration = FMath::Max(Fade, 0.1f);
	Decal->FadeInStartDelay = 0.f;
	Decal->FadeInDuration = 0.f;
	Decal->SetVisibility(true);
	Decal->MarkRenderStateDirty();
	Splat.Born = Now;
	Splat.Ends = Now + FMath::Max(Hold, 0.f) + FMath::Max(Fade, 0.1f);
	Splat.SpreadSeconds = Look.SpreadSeconds;
	Splat.bSpreading = Look.SpreadSeconds > 0.f;
	return true;
}

bool FDeathSplats::Advance(double Now)
{
	bool bAnySpreading = false;
	for (FSplat& Splat : Splats)
	{
		UDecalComponent* Decal = Splat.Decal.Get();
		if (!Decal)
		{
			continue;
		}
		if (Splat.Ends <= Now)
		{
			if (Decal->IsVisible())
			{
				Decal->SetVisibility(false);
			}
			Splat.bSpreading = false;
			continue;
		}
		if (!Splat.bSpreading)
		{
			continue;
		}
		const float Since = static_cast<float>(Now - Splat.Born);
		if (UMaterialInstanceDynamic* Dynamic = Splat.Look.Get())
		{
			Dynamic->SetScalarParameterValue(SplatSpreadParam, SpreadAt(Since, Splat.SpreadSeconds));
		}
		Splat.bSpreading = Since < Splat.SpreadSeconds;
		bAnySpreading |= Splat.bSpreading;
	}
	return bAnySpreading;
}

int32 FDeathSplats::NumLive(double Now) const
{
	int32 Count = 0;
	for (const FSplat& Splat : Splats)
	{
		Count += Splat.Decal.IsValid() && Splat.Ends > Now ? 1 : 0;
	}
	return Count;
}

void FDeathSplats::Shutdown()
{
	if (AActor* Actor = Owner.Get())
	{
		Actor->Destroy();
	}
	Owner.Reset();
	Splats.Reset();
}
