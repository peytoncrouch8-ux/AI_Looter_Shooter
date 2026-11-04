#include "Weapons/ImpactFlashSubsystem.h"
#include "Weapons/VfxTuning.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// UImpactFlashSubsystem: four pooled, unshadowed point lights for the flash a round throws on stone, metal and wood. The same
// build as the Unpaid shriek's pooled lights (UUnpaidPackSubsystem): one transient actor, lights made once, hidden while unused.

UImpactFlashSubsystem* UImpactFlashSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UImpactFlashSubsystem>() : nullptr;
}

bool UImpactFlashSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Played worlds, and the editor preview worlds the automated tests build (Flash does nothing in those).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

UPointLightComponent* UImpactFlashSubsystem::LightAt(int32 Index)
{
	if (Index < 0 || Index >= FImpactFlashPool::MaxSlots)
	{
		return nullptr;
	}
	if (UPointLightComponent* Made = Lights[Index].Get())
	{
		return Made;
	}
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return nullptr;
	}
	AActor* Owner = LightOwner.Get();
	if (!Owner)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Owner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
		if (!Owner)
		{
			return nullptr;
		}
#if WITH_EDITOR
		Owner->SetActorLabel(TEXT("ImpactFlashLights"));
#endif
		USceneComponent* Root = NewObject<USceneComponent>(Owner, TEXT("Root"), RF_Transient);
		Root->SetMobility(EComponentMobility::Movable);
		Owner->SetRootComponent(Root);
		Root->RegisterComponent();
		LightOwner = Owner;
	}
	UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner, NAME_None, RF_Transient);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Owner->GetRootComponent());
	// A flash, not a lamp: never a shadow (a shadow pass a light), out of the indirect light (Lumen's cost, for 60 ms).
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetCastShadows(false);
	Light->SetIndirectLightingIntensity(0.f);
	Light->SetVisibility(false);
	Light->RegisterComponent();
	Lights[Index] = Light;
	return Light;
}

bool UImpactFlashSubsystem::Flash(const UObject* WorldContextObject, const FVfxTuning& Tuning, const FVector& Point, const FVector& Normal,
	EImpactMaterial Material, float Weight, const FVector* Eye)
{
	// The cheapest refusals first: this runs for every hit of a stream of fire.
	if (Tuning.ImpactLight <= 0.f || Tuning.ImpactLightMax <= 0 || VfxRules::ImpactLightSurface(Material) <= 0.f)
	{
		return false;
	}
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	UImpactFlashSubsystem* Self = World && World->IsGameWorld() ? World->GetSubsystem<UImpactFlashSubsystem>() : nullptr;
	if (!Self)
	{
		return false;
	}
	// Hung off the surface along its facing, and judged by where the light is, not where the bullet struck.
	const FVector Place = Point + Normal.GetSafeNormal() * Tuning.ImpactLightLift;
	const float EyeDistance = Eye ? static_cast<float>(FVector::Dist(*Eye, Place)) : -1.f;
	const FVfxLightShape Shape = VfxRules::ImpactLightShape(Material, Weight, EyeDistance, Tuning);
	if (Shape.Candelas <= 0.f)
	{
		return false;
	}
	const int32 Slot = Self->Pool.Start(Place, Shape, Tuning.ImpactLightSeconds, Tuning.ImpactLightMax, Tuning.ImpactLightMerge, GFrameCounter);
	if (Slot == INDEX_NONE)
	{
		return false;
	}
	UPointLightComponent* Light = Self->LightAt(Slot);
	if (!Light)
	{
		// No light to drive (the actor could not be made): the slot goes back.
		Self->Pool.Slots[Slot].bLive = false;
		return false;
	}
	const FImpactFlashSlot& Lit = Self->Pool.Slots[Slot];
	Light->SetWorldLocation(Lit.Place);
	Light->SetLightColor(Lit.Color);
	Light->SetAttenuationRadius(Lit.RadiusCm);
	Light->SetIntensity(Self->Pool.StrengthOf(Slot));
	Light->SetVisibility(true);
	return true;
}

void UImpactFlashSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const uint32 Ended = Pool.Advance(DeltaTime, GFrameCounter);
	for (int32 Index = 0; Index < FImpactFlashPool::MaxSlots; ++Index)
	{
		UPointLightComponent* Light = Lights[Index].Get();
		if (!Light)
		{
			continue;
		}
		if (Ended & (1u << Index))
		{
			Light->SetVisibility(false);
			Light->SetIntensity(0.f);
		}
		else if (Pool.Slots[Index].bLive)
		{
			Light->SetIntensity(Pool.StrengthOf(Index));
		}
	}
}

bool UImpactFlashSubsystem::IsTickable() const
{
	// Asleep with no flash lit: a hit wakes it, the last flash's end lets it sleep.
	return Pool.NumLive() > 0;
}

TStatId UImpactFlashSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UImpactFlashSubsystem, STATGROUP_Tickables);
}

void UImpactFlashSubsystem::Deinitialize()
{
	if (AActor* Owner = LightOwner.Get())
	{
		Owner->Destroy();
	}
	LightOwner.Reset();
	Pool = FImpactFlashPool();
	Super::Deinitialize();
}
