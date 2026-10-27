#include "Player/UnderwaterView.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Player/PlayerLocomotionComponent.h"
#include "World/SwimWater.h"
#include "World/SwimWaterSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundMix.h"

const TCHAR* UUnderwaterViewComponent::PostMaterialPath = TEXT("/Game/Art/Materials/Masters/M_UnderwaterPost.M_UnderwaterPost");
const TCHAR* UUnderwaterViewComponent::MixPath = TEXT("/Game/Audio/Mix/SMX_Underwater.SMX_Underwater");
const TCHAR* UUnderwaterViewComponent::BubbleMaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_Shape.M_FX_Shape");

namespace
{
	TAutoConsoleVariable<float> CVarFadeIn(TEXT("Looter.Underwater.FadeIn"), 0.06f,
		TEXT("Seconds the underwater look takes to come in as the view goes under (short: the surface is a hard line)."));
	TAutoConsoleVariable<float> CVarFadeOut(TEXT("Looter.Underwater.FadeOut"), 0.12f,
		TEXT("Seconds it takes to go as the view comes up (the water running off the eyes)."));
	TAutoConsoleVariable<bool> CVarMuffle(TEXT("Looter.Underwater.Muffle"), true,
		TEXT("Muffle the world's sound under water (SMX_Underwater's low-pass)."));
	TAutoConsoleVariable<float> CVarLoopVolume(TEXT("Looter.Underwater.LoopVolume"), 1.f,
		TEXT("The underwater hum's volume (its cue's own mix times this)."));

	/** The view goes under this far below the surface and comes up this far over it (cm): no flicker on the line. */
	constexpr float UnderBelow = 2.f;
	constexpr float UpAbove = 2.f;
	/** The post-process pass's priority over the level's own volumes (it wins where they disagree). */
	constexpr float PostPriority = 50.f;

	UMaterialInterface* LoadQuietly(const TCHAR* Path)
	{
		return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

UUnderwaterViewComponent::UUnderwaterViewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// After the camera has moved this frame.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UUnderwaterViewComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	Locomotion = Owner ? Owner->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
}

void UUnderwaterViewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Let go quietly (no surfacing sound): the hum, the muffle.
	if (bUnder)
	{
		bUnder = false;
		LooterSound::Stop(Loop.Get());
		Loop.Reset();
		UWorld* World = GetWorld();
		if (bMixPushed && World && Mix)
		{
			UGameplayStatics::PopSoundMixModifier(World, Mix);
		}
		bMixPushed = false;
	}
	ShowBreathRing(false);
	Live.Reset();
	Super::EndPlay(EndPlayReason);
}

bool UUnderwaterViewComponent::FindView(FVector& OutLocation, FRotator& OutRotation) const
{
	// The local player's own camera, first person or third (whatever is on screen).
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* Player = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!Player || !Player->IsLocalController() || !Player->PlayerCameraManager)
	{
		return false;
	}
	OutLocation = Player->PlayerCameraManager->GetCameraLocation();
	OutRotation = Player->PlayerCameraManager->GetCameraRotation();
	return true;
}

void UUnderwaterViewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float Dt = FMath::Clamp(DeltaTime, 0.f, 0.1f);
	FVector View;
	FRotator Look;
	const bool bView = FindView(View, Look);
	const USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(this);
	bool bNowUnder = false;
	if (bView && Waters && Waters->NumWaters() > 0)
	{
		// The view's water: under its surface (past a hair either way, so the line never flickers).
		ASwimWater* Found = Waters->FindAt(View, bUnder ? UpAbove : -UnderBelow, USwimWaterSubsystem::DeepReachBelow);
		if (Found)
		{
			Water = Found;
			SurfaceZ = Found->GetSurfaceZ();
			bNowUnder = true;
		}
	}
	if (bNowUnder != bUnder)
	{
		SetUnder(bNowUnder);
	}
	UpdatePost(Dt);
	if (bView)
	{
		UpdateBubbles(Dt, View, Look);
	}
	UpdateBreathRing(Dt);
}

void UUnderwaterViewComponent::SetUnder(bool bNowUnder)
{
	bUnder = bNowUnder;
	UWorld* World = GetWorld();
	const bool bCanHear = World && ULooterSoundSubsystem::CanPlayIn(World);
	if (bUnder)
	{
		LooterSound::Play2D(this, SwimCue::UnderEnter);
		LooterSound::Stop(Loop.Get());
		Loop = LooterSound::Start(this, SwimCue::UnderLoop, nullptr, NAME_None, FMath::Max(CVarLoopVolume.GetValueOnGameThread(), 0.f));
		if (bCanHear && CVarMuffle.GetValueOnGameThread() && !bMixPushed)
		{
			if (!Mix)
			{
				Mix = LoadObject<USoundMix>(nullptr, MixPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
			}
			if (Mix)
			{
				UGameplayStatics::PushSoundMixModifier(World, Mix);
				bMixPushed = true;
			}
		}
		// The first breath let out comes a moment after going under.
		NextBreath = static_cast<float>(Random.FRandRange(1.2f, 2.2f));
		if (const UPlayerLocomotionComponent* Moves = Locomotion.Get())
		{
			SeenStrokes = Moves->GetSwimStrokeCount();
		}
		return;
	}
	LooterSound::Stop(Loop.Get(), 0.25f);
	Loop.Reset();
	if (bMixPushed)
	{
		if (World && Mix)
		{
			UGameplayStatics::PopSoundMixModifier(World, Mix);
		}
		bMixPushed = false;
	}
	LooterSound::Play2D(this, SwimCue::UnderExit);
}

bool UUnderwaterViewComponent::MakePost()
{
	bTriedPost = true;
	AActor* Owner = GetOwner();
	UMaterialInterface* Base = LoadQuietly(PostMaterialPath);
	if (!Owner || !Base)
	{
		UE_LOG(LogLooter, Warning, TEXT("Underwater view: no %s yet (Tools/Unreal/build_underwater_assets.py makes it): no underwater look."), PostMaterialPath);
		return false;
	}
	PostMaterial = UMaterialInstanceDynamic::Create(Base, this);
	Post = NewObject<UPostProcessComponent>(Owner, TEXT("UnderwaterPost"), RF_Transient);
	// Everywhere at once (unbound), over the level's own volumes; off (no pass at all) until the view is under.
	Post->bUnbound = true;
	Post->Priority = PostPriority;
	Post->BlendWeight = 0.f;
	Post->bEnabled = false;
	Post->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, PostMaterial));
	Post->RegisterComponent();
	return true;
}

void UUnderwaterViewComponent::UpdatePost(float DeltaTime)
{
	const float Seconds = FMath::Max(bUnder ? CVarFadeIn.GetValueOnGameThread() : CVarFadeOut.GetValueOnGameThread(), 0.01f);
	Blend = FMath::FInterpConstantTo(Blend, bUnder ? 1.f : 0.f, DeltaTime, 1.f / Seconds);
	if (Blend <= 0.f && !Post)
	{
		return;
	}
	if (!Post && (bTriedPost || !MakePost()))
	{
		return;
	}
	const bool bOn = Blend > 0.f;
	if (Post->bEnabled != bOn)
	{
		Post->bEnabled = bOn;
	}
	Post->BlendWeight = Blend;
	if (bOn && PostMaterial)
	{
		PostMaterial->SetScalarParameterValue(TEXT("Surface"), SurfaceZ);
	}
}
