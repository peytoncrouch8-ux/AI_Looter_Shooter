#include "World/HouseLights.h"
#include "World/LightingStateSubsystem.h"
#include "World/TownLifeRules.h"
#include "World/TownLifeSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	/** The hall lamp's reach (cm): the hall, and not the porch past the front wall about 1.8 m from it. */
	constexpr float LampReach = 175.f;

	/** A soft bulb, so the end wall behind it gets no hot spot. */
	constexpr float LampSourceRadius = 14.f;

	/** A paraffin lamp's warmth (kelvin). */
	constexpr float LampTemperature = 2400.f;
}

AHouseLights::AHouseLights()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	Lamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lamp"));
	SetRootComponent(Lamp);
	// Movable, as the states change it in the game; unshadowed and short, so it costs one light's pass.
	Lamp->SetMobility(EComponentMobility::Movable);
	Lamp->CastShadows = false;
	Lamp->IntensityUnits = ELightUnits::Candelas;
	Lamp->Intensity = DayLamp;
	Lamp->AttenuationRadius = LampReach;
	Lamp->SourceRadius = LampSourceRadius;
	Lamp->bUseTemperature = true;
	Lamp->Temperature = LampTemperature;
}

void AHouseLights::BeginPlay()
{
	Super::BeginPlay();
	if (House)
	{
		for (UStaticMeshComponent* Mesh : TInlineComponentArray<UStaticMeshComponent*>(House))
		{
			const int32 Slot = Mesh ? Mesh->GetMaterialIndex(GlowSlot) : INDEX_NONE;
			if (Slot != INDEX_NONE)
			{
				Windows = Mesh->CreateDynamicMaterialInstance(Slot);
				break;
			}
		}
	}
	ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this);
	if (Lighting)
	{
		Listening = Lighting->OnChanged.AddUObject(this, &AHouseLights::OnLightingChanged);
	}
	// As the level begins, lit as the state it starts in (a level without states is day).
	ApplyState(Lighting ? Lighting->GetState() : NAME_None);
	// A lived-in house is heard now and then as the player passes: its household's sounds come from its lamp's spot.
	const FName Home = GetHousehold();
	UTownLifeSubsystem* TownLife = Home.IsNone() ? nullptr : UTownLifeSubsystem::Get(this);
	if (TownLife)
	{
		TownLife->AddSource(this, Home);
	}
}

void AHouseLights::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this))
	{
		Lighting->OnChanged.Remove(Listening);
	}
	Listening.Reset();
	if (UTownLifeSubsystem* TownLife = UTownLifeSubsystem::Get(this))
	{
		TownLife->RemoveSource(this);
	}
	Super::EndPlay(EndPlayReason);
}

FName AHouseLights::GetHousehold() const
{
	return Household.IsNone() ? TownLifeRules::HouseholdForHouse(House) : Household;
}

void AHouseLights::OnLightingChanged(const FLightingStateChange& Change)
{
	ApplyState(Change.To);
}

void AHouseLights::ApplyState(FName State)
{
	// FName equality ignores case, as the console's state names do.
	const bool bDusk = !State.IsNone() && State == DuskState;
	Lamp->SetIntensity(bDusk ? DuskLamp : DayLamp);
	if (Windows)
	{
		Windows->SetScalarParameterValue(GlowParameter, bDusk ? DuskGlow : DayGlow);
	}
}
