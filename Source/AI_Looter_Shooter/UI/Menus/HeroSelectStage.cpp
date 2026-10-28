// AHeroSelectStage: the hero select's live stand. The hero on the plinth, the studio round them, the slow sway and the
// player's turns, the turn-in and flares, and the picture rendered for the screen.

#include "UI/Menus/HeroSelectStage.h"
#include "Embers/EmberDefinition.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "UI/Inventory/StageStudio.h"
#include "UI/Style/LooterUIStyle.h"
#include "Animation/AnimInstance.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"
#include "Retargeter/IKRetargeter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The player's mannequin and its unarmed Anim Blueprint (the stance layer's weary idle rides on it), as on the player. */
	const TCHAR* const MannequinPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	const TCHAR* const UnarmedAnimPath = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed");
	/** The engine's 1 m cylinder (radius 50, height 100, pivot in its middle; always cooked) and the game's lit surface. */
	const TCHAR* const CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* const SurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");

	/** The plinth's parts as the cylinder scaled: radius (cm), height (cm), the middle's height under the plinth's top. */
	struct FPlinthPart
	{
		float Radius;
		float Height;
		float Middle;
	};
	constexpr FPlinthPart TopPart{ 60.f, 7.f, -3.5f };
	constexpr FPlinthPart BandPart{ 61.5f, 2.f, -8.f };
	constexpr FPlinthPart FootPart{ 70.f, 10.f, -14.f };
	/** How brightly the band glows (M_StylizedSurface's emissive multiplier of its colour). */
	constexpr float BandGlow = 6.f;

	/** The studio round the hero, from the stand's spot (the camera looks along +X from in front). */
	const FVector KeySpot(-330.f, -280.f, 340.f);
	const FVector KeyTarget(0.f, 0.f, 105.f);
	const FVector FillSpot(-300.f, 320.f, 130.f);
	const FVector RimSpot(230.f, 170.f, 250.f);
	const FVector KickerSpot(200.f, -230.f, 160.f);

	/**
	 * The lights' strengths (candelas) and the picture's brightness, tunable live while the screen shows (each read every
	 * frame; a change applies at once). The sky's light is off in the stand's picture: only the studio lights it.
	 */
	TAutoConsoleVariable<float> CVarKey(TEXT("Looter.HeroStand.Key"), 160.f, TEXT("The hero select stand's warm key spot (candelas)."));
	TAutoConsoleVariable<float> CVarFill(TEXT("Looter.HeroStand.Fill"), 20.f, TEXT("The hero select stand's cool fill (candelas)."));
	TAutoConsoleVariable<float> CVarRim(TEXT("Looter.HeroStand.Rim"), 420.f, TEXT("The hero select stand's rim in the hero's colour (candelas)."));
	TAutoConsoleVariable<float> CVarKicker(TEXT("Looter.HeroStand.Kicker"), 200.f,
		TEXT("The hero select stand's kicker in the hero's first ember power's colour (candelas)."));
	TAutoConsoleVariable<float> CVarExposure(TEXT("Looter.HeroStand.Exposure"), 1.f, TEXT("The hero select stand picture's brightness."));
	/** For measuring what the stand costs: 0 holds its picture (no capture, no animation) while the screen stays open. */
	TAutoConsoleVariable<int32> CVarRun(TEXT("Looter.HeroStand.Run"), 1,
		TEXT("1: the hero select's stand animates and renders every frame while the screen is open. 0: it holds still (to measure its cost)."));
	/** A warm key (lamplight, not daylight) and a cool fill. */
	const FLinearColor KeyColour(1.f, 0.84f, 0.66f);
	const FLinearColor FillColour(0.7f, 0.82f, 1.f);

	/** A new hero turns in from the side over this long, and the rim flares as they do. */
	constexpr float TurnInSeconds = 0.6f;
	constexpr float TurnInDegrees = 70.f;
	constexpr float SwitchFlareSeconds = 0.7f;
	/** After the player turns the hero, the sway rests this long, then comes back over SwayReturnSeconds. */
	constexpr float SwayRestSeconds = 2.5f;
	constexpr float SwayReturnSeconds = 1.5f;

	/** A colour at full strength (its brightest channel 1), so a dark hero colour still lights: the light's own strength sets how bright. */
	FLinearColor AsLight(const FLinearColor& Colour)
	{
		const float Max = FMath::Max3(Colour.R, Colour.G, Colour.B);
		return Max > UE_KINDA_SMALL_NUMBER ? FLinearColor(Colour.R / Max, Colour.G / Max, Colour.B / Max, 1.f) : FLinearColor::White;
	}

	/**
	 * The rim lights at most this luminance (of its colour at full strength): a pale hero colour is nearly white at full
	 * strength and lit far hotter than a saturated one at the same candelas. Crane's sage (0.84) blew his pale grey-green
	 * face out in green-white patches on the stand (2026-10-20); Pike's teal (0.70) is held too, Ellis's blue (0.55),
	 * Gauge's amber and Odessa's crimson are under it.
	 */
	constexpr float MaxRimLuminance = 0.6f;
}

float AHeroSelectStage::RimShareFor(const FLinearColor& HeroColour)
{
	const float Luminance = AsLight(HeroColour).GetLuminance();
	return Luminance > MaxRimLuminance ? MaxRimLuminance / Luminance : 1.f;
}

namespace
{

	float EaseOutCubic(float Alpha)
	{
		const float Inverse = 1.f - FMath::Clamp(Alpha, 0.f, 1.f);
		return 1.f - Inverse * Inverse * Inverse;
	}

	UStaticMeshComponent* MakePlinthPart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Cylinder, const FPlinthPart& Part)
	{
		UStaticMeshComponent* Mesh = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Parent);
		Mesh->SetStaticMesh(Cylinder);
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, Part.Middle));
		Mesh->SetRelativeScale3D(FVector(Part.Radius / 50.f, Part.Radius / 50.f, Part.Height / 100.f));
		StageStudio::SetupPrimitive(Mesh);
		// Lit by the key and fill only (channel 2): the rims grazing its top made it a disc of the hero's colour.
		Mesh->LightingChannels.bChannel1 = false;
		Mesh->LightingChannels.bChannel2 = true;
		return Mesh;
	}

	UPointLightComponent* MakeAbsoluteLight(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, const FLinearColor& Colour, float Candelas)
	{
		UPointLightComponent* Light = StageStudio::MakeLight(Owner, Parent, Name, FVector::ZeroVector, Colour, Candelas, false);
		// The actor turns; the studio doesn't.
		Light->SetUsingAbsoluteLocation(true);
		Light->SetUsingAbsoluteRotation(true);
		return Light;
	}
}

AHeroSelectStage::AHeroSelectStage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// After the body has animated, so the picture shows this frame's pose.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetCanBeDamaged(false);
	// A pawn only so the anim instance has an owner standing still (the weary idle); nobody ever possesses it.
	AutoPossessAI = EAutoPossessAI::Disabled;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	AIControllerClass = nullptr;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mannequin(MannequinPath);
	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnim(UnarmedAnimPath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(CylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(SurfacePath);
	SurfaceBase = Surface.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	// The mannequin faces +Y in its own space; the actor's +X is the way the hero faces.
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	// Only the stand's camera sees what it drives, so it animates whether or not anything renders it.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->PrimaryComponentTick.bStartWithTickEnabled = false;
	// AndUpdate, not the bare setter: the editor checks every skinned mesh was told its asset changed.
	Body->SetSkinnedAssetAndUpdate(Mannequin.Object, false);
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->AnimClass = UnarmedAnim.Class;
	StageStudio::SetupPrimitive(Body);

	Figure = CreateDefaultSubobject<UBodyFigureComponent>(TEXT("Figure"));
	Figure->SetupAttachment(Body);
	Figure->PrimaryComponentTick.bStartWithTickEnabled = false;
	StageStudio::SetupPrimitive(Figure);

	PlinthTop = MakePlinthPart(this, Root, TEXT("PlinthTop"), Cylinder.Object, TopPart);
	PlinthBand = MakePlinthPart(this, Root, TEXT("PlinthBand"), Cylinder.Object, BandPart);
	PlinthBand->SetCastShadow(false);
	PlinthFoot = MakePlinthPart(this, Root, TEXT("PlinthFoot"), Cylinder.Object, FootPart);

	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Capture"));
	Capture->SetupAttachment(Root);
	Capture->SetUsingAbsoluteLocation(true);
	Capture->SetUsingAbsoluteRotation(true);
	StageStudio::SetupCapture(Capture);

	// The key is a spot (one shadow map, not a point light's six): the hero's one shadow, onto the plinth.
	KeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(Root);
	KeyLight->SetUsingAbsoluteLocation(true);
	KeyLight->SetUsingAbsoluteRotation(true);
	KeyLight->SetIntensityUnits(ELightUnits::Candelas);
	KeyLight->SetIntensity(CVarKey.GetValueOnAnyThread());
	KeyLight->SetLightColor(KeyColour);
	KeyLight->SetAttenuationRadius(1500.f);
	KeyLight->SetInnerConeAngle(16.f);
	KeyLight->SetOuterConeAngle(30.f);
	KeyLight->SetCastShadows(true);
	KeyLight->LightingChannels.bChannel0 = false;
	KeyLight->LightingChannels.bChannel1 = true;
	// The plinth too (channel 2), where the hero's shadow falls.
	KeyLight->LightingChannels.bChannel2 = true;
	KeyLight->SetVisibility(false);
	FillLight = MakeAbsoluteLight(this, Root, TEXT("FillLight"), FillColour, CVarFill.GetValueOnAnyThread());
	FillLight->LightingChannels.bChannel2 = true;
	RimLight = MakeAbsoluteLight(this, Root, TEXT("RimLight"), FLinearColor::White, CVarRim.GetValueOnAnyThread());
	KickerLight = MakeAbsoluteLight(this, Root, TEXT("KickerLight"), FLinearColor::White, CVarKicker.GetValueOnAnyThread());
	// Only the studio lights the hero: the level's sky light (every lighting channel) would flatten them into its blue.
	Capture->ShowFlags.SetSkyLighting(false);
	// What a lone figure on a plinth doesn't need, every frame the screen is open (measured on Medium, 2026-10-20: the
	// stand cost 1.5 ms of GPU and 1.9 of the render thread before these): no screen-space occlusion or contact
	// shadows, no translucency (the heroes' hair cards and glows are masked and opaque), no exposure or lens passes.
	Capture->ShowFlags.SetAmbientOcclusion(false);
	Capture->ShowFlags.SetContactShadows(false);
	Capture->ShowFlags.SetCapsuleShadows(false);
	Capture->ShowFlags.SetTranslucency(false);
	Capture->ShowFlags.SetEyeAdaptation(false);
	Capture->ShowFlags.SetLensFlares(false);
}

void AHeroSelectStage::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(StageStudio::Location(StandIndex));
	RenderTarget = StageStudio::MakeRenderTarget(this, TEXT("HeroSelectStageImage"), ImageWidth, ImageHeight);
	Capture->TextureTarget = RenderTarget;
	Capture->ShowOnlyActors.Add(this);

	// The plinth's looks: dark gunmetal like the HUD's bezels' shade, a near-black foot, the band in the hero's colour (ShowHero).
	PlinthMaterials.Reset();
	if (SurfaceBase)
	{
		const TPair<UStaticMeshComponent*, FLinearColor> Parts[] = {
			{ PlinthTop.Get(), LooterUI::Color::MetalDeep() }, { PlinthFoot.Get(), LooterUI::Color::Ink() }, { PlinthBand.Get(), LooterUI::Color::Hairline() } };
		for (const TPair<UStaticMeshComponent*, FLinearColor>& Part : Parts)
		{
			UMaterialInstanceDynamic* Look = UMaterialInstanceDynamic::Create(SurfaceBase, this);
			Look->SetVectorParameterValue(TEXT("Color"), Part.Value);
			Look->SetScalarParameterValue(TEXT("Glow"), Part.Key == PlinthBand ? BandGlow : 0.f);
			Look->SetScalarParameterValue(TEXT("Variation"), 0.f);
			Part.Key->SetMaterial(0, Look);
			PlinthMaterials.Add(Look);
		}
	}
	PlaceCamera();
}

// ---------------------------------------------------------------------------
// The hero
// ---------------------------------------------------------------------------

bool AHeroSelectStage::ShowHero(const UHeroDefinition* InHero, bool bTurnIn)
{
	Hero = InHero;
	if (!Hero || !Hero->LoadFigureMesh() || !Hero->LoadRetargeter())
	{
		// Not in the game yet: nothing on the stand (the screen shows the painting).
		Figure->SetVisibility(false);
		return false;
	}
	Figure->SetShownHero(Hero);
	Figure->SetVisibility(true);
	// Back to the three-quarter view, the sway from its start.
	UserTurn = 0.f;
	SwayClock = 0.f;
	SwayWeight = 1.f;
	SinceUserTurn = SwayRestSeconds + SwayReturnSeconds;
	TurnInLeft = bTurnIn ? TurnInSeconds : 0.f;
	SwitchFlareLeft = bTurnIn ? SwitchFlareSeconds : 0.f;

	// Their colour behind them (the rim, the band) and their first ember's from the other side (the kicker).
	const FLinearColor HeroColour = AsLight(Hero->Color);
	const UEmberDefinition* FirstPower = Hero->Powers.IsEmpty() ? nullptr : Hero->Powers[0].Get();
	RimLight->SetLightColor(HeroColour);
	RimShare = RimShareFor(Hero->Color);
	KickerLight->SetLightColor(FirstPower ? AsLight(FirstPower->Color) : HeroColour);
	ColourPlinth(Hero->Color);
	ApplyLights();
	PlaceCamera();
	return IsHeroWorn();
}

bool AHeroSelectStage::IsHeroWorn() const
{
	const USkeletalMesh* HeroMesh = Hero ? Hero->LoadFigureMesh() : nullptr;
	return HeroMesh && Figure->IsWorn() && Figure->GetSkeletalMeshAsset() == HeroMesh;
}

void AHeroSelectStage::ColourPlinth(const FLinearColor& HeroColour)
{
	// The band is the last made (BeginPlay: top, foot, band).
	if (PlinthMaterials.Num() == 3 && PlinthMaterials[2])
	{
		PlinthMaterials[2]->SetVectorParameterValue(TEXT("Color"), HeroColour);
	}
}

// ---------------------------------------------------------------------------
// Running
// ---------------------------------------------------------------------------

void AHeroSelectStage::SetActive(bool bInActive)
{
	if (bActive == bInActive)
	{
		return;
	}
	bActive = bInActive;
	SetActorTickEnabled(bActive);
	Body->SetComponentTickEnabled(bActive);
	Figure->SetComponentTickEnabled(bActive && Figure->IsWorn());
	KeyLight->SetVisibility(bActive);
	for (UPointLightComponent* Light : { FillLight.Get(), RimLight.Get(), KickerLight.Get() })
	{
		Light->SetVisibility(bActive);
	}
	if (bActive)
	{
		PlaceCamera();
		ApplyLights();
	}
}

void AHeroSelectStage::AddTurn(float Degrees)
{
	// The sway's share so far becomes the player's own, so taking hold never jumps; it starts over from nothing later.
	UserTurn = FRotator::NormalizeAxis(UserTurn + SwayWeight * SwayDegrees * FMath::Sin(SwayClock / SwaySeconds * UE_TWO_PI) + Degrees);
	SwayClock = 0.f;
	SwayWeight = 0.f;
	SinceUserTurn = 0.f;
	TurnInLeft = 0.f;
	PlaceCamera();
}

void AHeroSelectStage::SetFlare(float Alpha)
{
	ConfirmFlare = FMath::Clamp(Alpha, 0.f, 1.f);
	ApplyLights();
}

void AHeroSelectStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Held for a measurement (Looter.HeroStand.Run 0): the bodies stop animating and nothing is captured.
	const bool bRun = CVarRun.GetValueOnGameThread() != 0;
	if (Body->IsComponentTickEnabled() != bRun)
	{
		Body->SetComponentTickEnabled(bRun);
		Figure->SetComponentTickEnabled(bRun && Figure->IsWorn());
	}
	if (!bRun)
	{
		return;
	}
	SinceUserTurn += DeltaSeconds;
	SwayWeight = FMath::Clamp((SinceUserTurn - SwayRestSeconds) / SwayReturnSeconds, 0.f, 1.f);
	SwayClock = FMath::Fmod(SwayClock + DeltaSeconds * SwayWeight, SwaySeconds);
	TurnInLeft = FMath::Max(TurnInLeft - DeltaSeconds, 0.f);
	SwitchFlareLeft = FMath::Max(SwitchFlareLeft - DeltaSeconds, 0.f);
	// Every frame: the flares fade, and the tuning variables apply live (a light only changes when its value does).
	ApplyLights();
	PlaceCamera();
	Capture->CaptureSceneDeferred();
}

void AHeroSelectStage::PlaceCamera()
{
	// The turn: the rest view, the player's, the sway, and a new hero's turn-in easing out from the side.
	const float Sway = SwayWeight * SwayDegrees * FMath::Sin(SwayClock / SwaySeconds * UE_TWO_PI);
	const float TurnIn = TurnInDegrees * (1.f - EaseOutCubic(1.f - TurnInLeft / TurnInSeconds));
	CurrentTurn = UserTurn + Sway + TurnIn;
	// The hero stands still, facing the rest view (180 degrees: the camera looks along +X), and the camera and its lights
	// orbit them the other way, a photographer's turntable: a positive turn still shows more of the hero's right side. The
	// hero used to spin, and their coat's springs (world space) whipped out on every drag (the user, 2026-10-27).
	SetActorRotation(FRotator(0.f, 180.f + DefaultTurn, 0.f));
	const FRotator Orbit(0.f, -CurrentTurn, 0.f);

	const FVector Origin = GetActorLocation();
	const FVector Eye = Origin + Orbit.RotateVector(FVector(-CameraDistance, 0.f, CameraHeight));
	Capture->SetWorldLocationAndRotation(Eye, (Origin + FVector(0.f, 0.f, LookHeight) - Eye).Rotation());
	Capture->FOVAngle = FieldOfView;
	const FVector Key = Orbit.RotateVector(KeySpot);
	KeyLight->SetWorldLocationAndRotation(Origin + Key, (Orbit.RotateVector(KeyTarget) - Key).Rotation());
	FillLight->SetWorldLocation(Origin + Orbit.RotateVector(FillSpot));
	RimLight->SetWorldLocation(Origin + Orbit.RotateVector(RimSpot));
	KickerLight->SetWorldLocation(Origin + Orbit.RotateVector(KickerSpot));
}

void AHeroSelectStage::ApplyLights()
{
	// A switch flares the rim (fading over SwitchFlareSeconds); the confirm burns the rim and the key brighter.
	const float Switch = SwitchFlareLeft / SwitchFlareSeconds;
	RimLight->SetIntensity(CVarRim.GetValueOnGameThread() * RimShare * (1.f + 1.f * Switch * Switch + 0.9f * ConfirmFlare));
	KickerLight->SetIntensity(CVarKicker.GetValueOnGameThread() * (1.f + 0.6f * Switch + 0.8f * ConfirmFlare));
	KeyLight->SetIntensity(CVarKey.GetValueOnGameThread() * (1.f + 0.35f * ConfirmFlare));
	FillLight->SetIntensity(CVarFill.GetValueOnGameThread());
	Exposure = CVarExposure.GetValueOnGameThread();
	if (PlinthMaterials.Num() == 3 && PlinthMaterials[2])
	{
		PlinthMaterials[2]->SetScalarParameterValue(TEXT("Glow"), BandGlow * (1.f + 1.f * Switch + 1.5f * ConfirmFlare));
	}
}
