#include "World/LightingStateSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/LightingStates.h"
#include "World/LightingTargets.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

const FName ULightingStateSubsystem::BackdropTintParameter(TEXT("BackdropTint"));
const FName ULightingStateSubsystem::CloudTintParameter(TEXT("CloudTint"));
const FName ULightingStateSubsystem::FogInscatteringParameter(TEXT("FogInscattering"));
const FName ULightingStateSubsystem::FogDirectionalParameter(TEXT("FogDirectional"));

namespace
{
	/**
	 * Frames the screen stays black once the new light has landed: the sky light's capture renders as the first ends,
	 * and the second is drawn with it before the screen clears.
	 */
	constexpr int32 BlackFrames = 2;

	const FLinearColor SwitchFadeColor(0.f, 0.f, 0.f);

	const TCHAR* DescribeSwitch(ELightingSwitch How)
	{
		switch (How)
		{
		case ELightingSwitch::Fade:
			return TEXT("behind a fade");
		case ELightingSwitch::Instant:
			return TEXT("at once");
		case ELightingSwitch::Blend:
			return TEXT("blended");
		}
		return TEXT("");
	}

	FString JoinNames(const TArray<FName>& Names)
	{
		return FString::JoinBy(Names, TEXT(", "), [](const FName& Each) { return Each.ToString(); });
	}

	/** Every local player's camera, which a switch fades. */
	TArray<APlayerCameraManager*> LocalCameras(const UWorld* World)
	{
		TArray<APlayerCameraManager*> Cameras;
		if (!World)
		{
			return Cameras;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* Controller = It->Get();
			if (Controller && Controller->IsLocalController() && Controller->PlayerCameraManager)
			{
				Cameras.Add(Controller->PlayerCameraManager.Get());
			}
		}
		return Cameras;
	}
}

ULightingStateSubsystem* ULightingStateSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<ULightingStateSubsystem>() : nullptr;
}

bool ULightingStateSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULightingStateSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// The level is placed in its initial state, but the collection's values in this world start at the asset's
	// defaults: set them to match before anything is drawn with them.
	if (const ALightingStates* States = FindLevelStates())
	{
		WriteCollection(Applied);
		UE_LOG(LogLooter, Log, TEXT("Lighting: %s starts in %s (its states: %s)."), *InWorld.GetMapName(), *Current.ToString(),
			*JoinNames(States->GetStateNames()));
	}
}

TStatId ULightingStateSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULightingStateSubsystem, STATGROUP_Tickables);
}

void ULightingStateSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Update(DeltaTime);
}

bool ULightingStateSubsystem::IsTickable() const
{
	// Nothing to do between switches.
	return Phase != EPhase::Idle;
}

ALightingStates* ULightingStateSubsystem::FindLevelStates()
{
	if (ALightingStates* Known = LevelStates.Get())
	{
		return Known;
	}
	ALightingStates* Found = ALightingStates::Find(GetWorld());
	LevelStates = Found;
	if (Found && !bKnowsLevel)
	{
		// What's in place: the initial state, as the lights actually show it (a blend sets out from there).
		bKnowsLevel = true;
		const FLightingState* Initial = Found->GetInitialState();
		Applied = Initial ? *Initial : FLightingState();
		FLightingTargets::Find(nullptr, Found).Read(Applied);
		Current = Applied.Name;
	}
	return Found;
}

FLightingTargets ULightingStateSubsystem::FindTargets()
{
	return FLightingTargets::Find(nullptr, FindLevelStates());
}

bool ULightingStateSubsystem::SetState(FName StateName, ELightingSwitch How, float BlendSeconds)
{
	const ALightingStates* States = FindLevelStates();
	const FLightingState* Wanted = States ? States->FindState(StateName) : nullptr;
	const FString MapName = GetWorld() ? GetWorld()->GetMapName() : FString();
	if (!Wanted)
	{
		UE_LOG(LogLooter, Warning, TEXT("Lighting: %s has no state %s (%s)."), *MapName, *StateName.ToString(),
			States ? *FString::Printf(TEXT("its states: %s"), *JoinNames(States->GetStateNames())) : TEXT("it has no lighting states"));
		return false;
	}
	if (Phase == EPhase::Idle && Wanted->Name == Current)
	{
		UE_LOG(LogLooter, Display, TEXT("Lighting: %s is in place already in %s."), *Current.ToString(), *MapName);
		return true;
	}

	const FLightingTargets Targets = FindTargets();
	const FString Missing = Targets.DescribeMissing();
	if (!Missing.IsEmpty())
	{
		UE_LOG(LogLooter, Warning, TEXT("Lighting: %s has no %s; that much stays as it is."), *MapName, *Missing);
	}
	const UDirectionalLightComponent* SunLight = Targets.Sun ? Targets.Sun->GetComponent() : nullptr;
	if (SunLight && SunLight->Mobility != EComponentMobility::Movable)
	{
		UE_LOG(LogLooter, Warning, TEXT("Lighting: %s's sun %s isn't movable, so the game can't turn it (the build scripts place it movable)."),
			*MapName, *Targets.Sun->GetName());
	}

	Target = *Wanted;
	SwitchHow = How;
	Clock = 0.f;
	if (Phase == EPhase::FadingOut || Phase == EPhase::Holding)
	{
		// A fade is underway: the new state takes the old one's place behind it, and the screen still comes back.
		if (Phase == EPhase::Holding || How == ELightingSwitch::Instant)
		{
			HoldBlack();
			Phase = EPhase::Holding;
			HeldFrames = 0;
			Apply(Target, true);
			Land();
		}
		return true;
	}
	if (How == ELightingSwitch::Blend && BlendSeconds > 0.f)
	{
		// From wherever the lights are now, a blend underway included.
		BlendFrom = Applied;
		BlendDuration = BlendSeconds;
		SinceRecapture = 0.f;
		Phase = EPhase::Blending;
		return true;
	}
	if (How == ELightingSwitch::Fade && FadeOut())
	{
		Phase = EPhase::FadingOut;
		return true;
	}
	// At once: asked for, a blend of no time, or a fade with nobody to fade for (no local player).
	Phase = EPhase::Idle;
	SwitchHow = ELightingSwitch::Instant;
	Apply(Target, true);
	Land();
	return true;
}

FName ULightingStateSubsystem::GetState() const
{
	if (bKnowsLevel)
	{
		return Current;
	}
	const ALightingStates* States = ALightingStates::Find(GetWorld());
	const FLightingState* Initial = States ? States->GetInitialState() : nullptr;
	return Initial ? Initial->Name : NAME_None;
}

FName ULightingStateSubsystem::GetPendingState() const
{
	return Phase == EPhase::FadingOut || Phase == EPhase::Blending ? Target.Name : NAME_None;
}

TArray<FName> ULightingStateSubsystem::GetStateNames() const
{
	const ALightingStates* States = LevelStates.IsValid() ? LevelStates.Get() : ALightingStates::Find(GetWorld());
	return States ? States->GetStateNames() : TArray<FName>();
}

void ULightingStateSubsystem::Update(float DeltaSeconds)
{
	switch (Phase)
	{
	case EPhase::Idle:
		break;

	case EPhase::FadingOut:
		Clock += DeltaSeconds;
		if (Clock >= FadeOutSeconds)
		{
			// Black whatever else touched the fade meanwhile (a hurt flash), so the change itself is never seen.
			HoldBlack();
			Phase = EPhase::Holding;
			HeldFrames = 0;
			Apply(Target, true);
			Land();
		}
		break;

	case EPhase::Holding:
		if (++HeldFrames >= BlackFrames)
		{
			FadeIn();
			Phase = EPhase::Idle;
		}
		break;

	case EPhase::Blending:
	{
		Clock += DeltaSeconds;
		const float Alpha = BlendDuration > 0.f ? FMath::Clamp(Clock / BlendDuration, 0.f, 1.f) : 1.f;
		if (Alpha >= 1.f)
		{
			Phase = EPhase::Idle;
			Apply(Target, true);
			Land();
			break;
		}
		// The sky light lags the sky between captures; a capture every so often keeps each step too small to notice.
		SinceRecapture += DeltaSeconds;
		const bool bRecapture = SinceRecapture >= BlendRecaptureSeconds;
		if (bRecapture)
		{
			SinceRecapture = 0.f;
		}
		Apply(FLightingState::Blend(BlendFrom, Target, FMath::SmoothStep(0.f, 1.f, Alpha)), bRecapture);
		break;
	}
	}
}

void ULightingStateSubsystem::Apply(const FLightingState& State, bool bRecapture)
{
	const FLightingTargets Targets = FindTargets();
	Targets.Write(State);
	WriteCollection(State);
	USkyLightComponent* Sky = Targets.SkyLight ? Targets.SkyLight->GetLightComponent() : nullptr;
	if (bRecapture && Sky)
	{
		// The sky light was captured once as the level loaded, and that sky is gone now. The capture runs as this frame
		// ends, after everything set above has reached the renderer, so it sees the new sun, fog and tints.
		Sky->RecaptureSky();
	}
	Applied = State;
}

void ULightingStateSubsystem::WriteCollection(const FLightingState& State)
{
	UWorld* World = GetWorld();
	if (!ParameterCollection && !bCollectionLooked)
	{
		// Loaded once. A states actor that names no collection writes none.
		bCollectionLooked = true;
		const ALightingStates* States = FindLevelStates();
		if (States && !States->Collection.IsNull())
		{
			ParameterCollection = States->Collection.LoadSynchronous();
			if (!ParameterCollection)
			{
				UE_LOG(LogLooter, Warning, TEXT("Lighting: no material parameter collection at %s, so the backdrop and clouds keep their tints (Tools/Unreal/build_world_materials.py M_Backdrop makes it)."),
					*States->Collection.ToString());
			}
		}
	}
	UMaterialParameterCollectionInstance* Instance = World && ParameterCollection ? World->GetParameterCollectionInstance(ParameterCollection) : nullptr;
	if (!Instance)
	{
		return;
	}
	const TPair<FName, FLinearColor> Values[] = {
		{ BackdropTintParameter, State.BackdropTint },
		{ CloudTintParameter, State.CloudTint },
		{ FogInscatteringParameter, State.FogInscattering },
		{ FogDirectionalParameter, State.FogDirectionalInscattering },
	};
	for (const TPair<FName, FLinearColor>& Value : Values)
	{
		if (!Instance->SetVectorParameterValue(Value.Key, Value.Value) && !bCollectionWarned)
		{
			bCollectionWarned = true;
			UE_LOG(LogLooter, Warning, TEXT("Lighting: %s has no %s parameter (Tools/Unreal/build_world_materials.py M_Backdrop adds it)."),
				*ParameterCollection->GetName(), *Value.Key.ToString());
		}
	}
}

void ULightingStateSubsystem::Land()
{
	// Called with the phase already moved on, so a listener that switches again finds the subsystem as it now is.
	const FLightingStateChange Change{ Current, Target.Name, SwitchHow };
	Current = Target.Name;
	UE_LOG(LogLooter, Display, TEXT("Lighting: %s in place in %s (%s, from %s): %s."), *Current.ToString(),
		GetWorld() ? *GetWorld()->GetMapName() : TEXT("no world"), DescribeSwitch(SwitchHow), *Change.From.ToString(), *Target.Describe());
	OnChanged.Broadcast(Change);
}

bool ULightingStateSubsystem::FadeOut()
{
	const TArray<APlayerCameraManager*> Cameras = LocalCameras(GetWorld());
	for (APlayerCameraManager* Camera : Cameras)
	{
		// From wherever another fade left the screen, at the same pace.
		const float Start = Camera->bEnableFading ? FMath::Clamp(Camera->FadeAmount, 0.f, 1.f) : 0.f;
		Camera->StartCameraFade(Start, 1.f, FadeOutSeconds * (1.f - Start), SwitchFadeColor, false, /*bHoldWhenFinished*/ true);
	}
	return !Cameras.IsEmpty();
}

void ULightingStateSubsystem::HoldBlack()
{
	for (APlayerCameraManager* Camera : LocalCameras(GetWorld()))
	{
		Camera->SetManualCameraFade(1.f, SwitchFadeColor, false);
	}
}

void ULightingStateSubsystem::FadeIn()
{
	for (APlayerCameraManager* Camera : LocalCameras(GetWorld()))
	{
		Camera->StartCameraFade(1.f, 0.f, FadeInSeconds, SwitchFadeColor, false, false);
	}
}
