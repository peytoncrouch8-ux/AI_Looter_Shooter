// AColdOpenCast's glows and lights: Saint Ada's ember in the Deacon's hand (and its spark as he fires), Abel's lantern,
// the shots' flash light. The rest of the cast is ColdOpenCast.cpp's and ColdOpenCastFigures.cpp's.

#include "Scenes/ColdOpenCast.h"
#include "Scenes/ColdOpenSet.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"

namespace
{
	/** The glows: the game's additive glow on camera-facing quads (R, G, B, strength, shape 1 = round). */
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* QuadPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	constexpr int32 GlowFloats = 5;
	constexpr int32 EmberGlow = 0;
	constexpr int32 LanternGlowIndex = 1;

	/** Saint Ada's ember: a burning orange light in a gloved hand. Abel's lantern: warm, and smaller in the dark. */
	const FLinearColor EmberColor(1.f, 0.42f, 0.08f);
	constexpr float EmberSize = 22.f;
	constexpr float EmberStrength = 32.f;
	constexpr float EmberCandelas = 14.f;
	const FLinearColor LanternColor(1.f, 0.72f, 0.38f);
	constexpr float LanternSize = 30.f;
	constexpr float LanternStrength = 26.f;
	constexpr float LanternCandelas = 22.f;

	/**
	 * The spark (Docs/Story.md, Seeds: the ember flares and dims as he fires): the saint flinches at what she's made to
	 * watch. Up to three times her rest in a blink, down to a third of it while the shot rings, then back to rest slowly,
	 * as if she'd caught her breath.
	 */
	constexpr float FlarePeak = 3.f;
	constexpr float FlareLow = 0.35f;
	constexpr float FlareRise = 0.15f;
	constexpr float FlareFall = 1.2f;
	constexpr float FlareRecover = 2.f;

	/** The hand the ember (and Abel's lantern) is in: the mannequin's left. */
	const FName LeftHand(TEXT("hand_l"));

	/** Where the camera is, for the glows to face: the player's, or a point in front of Fallback. */
	FVector Viewer(const UWorld* World, const FVector& Fallback)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller && Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetCameraLocation()
			: Fallback + FVector(500.0, 0.0, 0.0);
	}

	UPointLightComponent* MakeLight(AActor& Owner, USceneComponent* Parent, const FLinearColor& Color, float Radius)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(&Owner, NAME_None, RF_Transient);
		Light->SetupAttachment(Parent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(0.f);
		Light->SetLightColor(Color);
		Light->SetAttenuationRadius(Radius);
		// Lanterns and flashes light what's near them; at dusk nothing they could shadow reads anyway.
		Light->SetCastShadows(false);
		Light->SetVisibility(false);
		Light->RegisterComponent();
		return Light;
	}
}

void AColdOpenCast::BuildGlows(const AColdOpenSet& Set)
{
	// The ember and the lantern: round glows facing the camera, each with a little light of its own.
	Glows = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Glows"), RF_Transient);
	Glows->SetupAttachment(Root);
	Glows->SetMobility(EComponentMobility::Movable);
	Glows->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glows->SetCastShadow(false);
	Glows->bReceivesDecals = false;
	Glows->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, QuadPath));
	Glows->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, GlowMaterialPath));
	Glows->SetNumCustomDataFloats(GlowFloats);
	Glows->RegisterComponent();
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Glows->AddInstance(FTransform(FQuat::Identity, Set.GetActorLocation(), FVector(0.01)), /*bWorldSpace*/ true);
	}
	EmberLight = MakeLight(*this, Root, EmberColor, 380.f);
	LanternLight = MakeLight(*this, Root, LanternColor, 700.f);
	FlashLight = MakeLight(*this, Root, FLinearColor(1.f, 0.78f, 0.5f), 1600.f);
}

// ---------------------------------------------------------------------------
// The spark
// ---------------------------------------------------------------------------

float AColdOpenCast::EmberFlareAt(float SinceFlare)
{
	if (SinceFlare < 0.f)
	{
		return 1.f;
	}
	// Up fast (eased out, a flinch), down over the shot's ring, then back to rest eased both ways.
	if (SinceFlare < FlareRise)
	{
		const float Alpha = SinceFlare / FlareRise;
		return FMath::Lerp(1.f, FlarePeak, 1.f - FMath::Square(1.f - Alpha));
	}
	float Since = SinceFlare - FlareRise;
	if (Since < FlareFall)
	{
		return FMath::Lerp(FlarePeak, FlareLow, FMath::SmoothStep(0.f, 1.f, Since / FlareFall));
	}
	Since -= FlareFall;
	if (Since < FlareRecover)
	{
		return FMath::Lerp(FlareLow, 1.f, FMath::SmoothStep(0.f, 1.f, Since / FlareRecover));
	}
	return 1.f;
}

float AColdOpenCast::GetFlareSeconds()
{
	return FlareRise + FlareFall + FlareRecover;
}

void AColdOpenCast::FlareEmber()
{
	FlareAt = GlowClock;
	UpdateGlows();
}

float AColdOpenCast::GetEmberFlare() const
{
	return FlareAt < 0.f ? 1.f : EmberFlareAt(GlowClock - FlareAt);
}

float AColdOpenCast::GetEmberStrength() const
{
	return IsPointShown() ? EmberStrength * GetEmberFlare() : 0.f;
}

// ---------------------------------------------------------------------------
// Every frame
// ---------------------------------------------------------------------------

void AColdOpenCast::UpdateGlows()
{
	if (!Glows || Glows->GetInstanceCount() < 2)
	{
		return;
	}
	// The ember in the Deacon's hand while the Point shows; the lantern in Abel's, then rolling off with its flame dying.
	const bool bEmber = bPointShown && Gang.IsValidIndex(AColdOpenSet::DeaconIndex);
	const FVector Ember = bEmber ? BoneInWorld(Gang[AColdOpenSet::DeaconIndex], LeftHand) + FVector(0.0, 0.0, 6.0) : GetActorLocation();
	const float Flare = GetEmberFlare();
	const bool bLantern = bPointShown && (bAbelShown || bLanternLoose) && (!bLanternLoose || LanternGlow > 0.f);
	const FVector Lantern = bLanternLoose ? LanternAt : (Abel ? BoneInWorld(Abel, LeftHand) - FVector(0.0, 0.0, 10.0) : GetActorLocation());
	const float LanternStrengthNow = bLantern ? (bLanternLoose ? LanternGlow : 1.f) : 0.f;

	const FVector Camera = Viewer(GetWorld(), Ember);
	auto Facing = [&Camera](const FVector& Where, float Size)
	{
		FVector ToCamera = Camera - Where;
		if (!ToCamera.Normalize())
		{
			ToCamera = FVector::UpVector;
		}
		// The engine's quad lies in XY facing +Z, 100 across.
		return FTransform(FRotationMatrix::MakeFromZ(ToCamera).ToQuat(), Where, FVector(Size / 100.0, Size / 100.0, 1.0));
	};
	Glows->UpdateInstanceTransform(EmberGlow, Facing(Ember, bEmber ? EmberSize : 0.01f), /*bWorldSpace*/ true, false, /*bTeleport*/ true);
	Glows->UpdateInstanceTransform(LanternGlowIndex, Facing(Lantern, LanternStrengthNow > 0.f ? LanternSize : 0.01f), true, false, true);
	const float EmberData[] = { EmberColor.R, EmberColor.G, EmberColor.B, bEmber ? EmberStrength * Flare : 0.f, 1.f };
	const float LanternData[] = { LanternColor.R, LanternColor.G, LanternColor.B, LanternStrength * LanternStrengthNow, 1.f };
	Glows->SetCustomData(EmberGlow, MakeArrayView(EmberData), false);
	Glows->SetCustomData(LanternGlowIndex, MakeArrayView(LanternData), false);
	Glows->MarkRenderStateDirty();

	if (EmberLight)
	{
		EmberLight->SetWorldLocation(Ember);
		EmberLight->SetIntensity(bEmber ? EmberCandelas * Flare : 0.f);
		EmberLight->SetVisibility(bEmber);
	}
	if (LanternLight)
	{
		LanternLight->SetWorldLocation(Lantern);
		LanternLight->SetIntensity(LanternCandelas * LanternStrengthNow);
		LanternLight->SetVisibility(LanternStrengthNow > 0.f);
	}
}
