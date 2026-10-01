#include "UI/Inventory/StageStudio.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"

FVector StageStudio::Location(int32 StandIndex)
{
	// Each stand gets its own spot, a few light radii apart, so one's lights never reach another's model.
	return FVector(400000.0 + StandIndex * 20000.0, 400000.0, 50000.0);
}

void StageStudio::SetupPrimitive(UPrimitiveComponent* Primitive)
{
	Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Primitive->SetGenerateOverlapEvents(false);
	Primitive->SetCanEverAffectNavigation(false);
	Primitive->bVisibleInSceneCaptureOnly = true;
	Primitive->bReceivesDecals = false;
	Primitive->LightingChannels.bChannel0 = false;
	Primitive->LightingChannels.bChannel1 = true;
}

void StageStudio::SetupCapture(USceneCaptureComponent2D* Capture)
{
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->bAlwaysPersistRenderingState = false;
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
}

UPointLightComponent* StageStudio::MakeLight(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, const FVector& Location,
	const FLinearColor& Color, float Candelas, bool bShadows)
{
	UPointLightComponent* Light = Owner->CreateDefaultSubobject<UPointLightComponent>(Name);
	Light->SetupAttachment(Parent);
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
}

UTextureRenderTarget2D* StageStudio::MakeRenderTarget(UObject* Outer, FName Name, int32 Width, int32 Height)
{
	UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>(Outer, Name);
	Target->RenderTargetFormat = RTF_RGBA16f;
	// Nothing drawn reads as see-through (alpha is inverse opacity).
	Target->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
	Target->bAutoGenerateMips = true;
	Target->Filter = TF_Trilinear;
	Target->InitAutoFormat(Width, Height);
	Target->UpdateResourceImmediate(true);
	return Target;
}

bool StageStudio::ProjectToImage(const USceneCaptureComponent2D* Capture, int32 Width, int32 Height, const FVector& WorldLocation,
	FVector2D& OutUV)
{
	// View space: X ahead, Y right, Z up.
	const FVector Local = Capture->GetComponentTransform().InverseTransformPositionNoScale(WorldLocation);
	if (Local.X <= 1.0)
	{
		return false;
	}
	// Scene captures spread the field of view across the width; the height follows from the aspect ratio.
	const double TanHalf = FMath::Tan(FMath::DegreesToRadians(Capture->FOVAngle * 0.5));
	const double Aspect = static_cast<double>(Width) / Height;
	const double X = Local.Y / (Local.X * TanHalf);
	const double Y = Local.Z * Aspect / (Local.X * TanHalf);
	OutUV = FVector2D(0.5 + 0.5 * X, 0.5 - 0.5 * Y);
	return true;
}
