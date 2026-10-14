#include "World/HouseLights.h"
#include "World/LightFlickerSubsystem.h"
#include "World/LightingStateSubsystem.h"
#include "World/TownLifeRules.h"
#include "World/TownLifeSubsystem.h"
#include "World/WindowLifeSubsystem.h"
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
	// A paraffin flame: it flickers near the player (ULightFlickerSubsystem; BeginPlay adds its windows as its glass).
	Lamp->ComponentTags.Add(ULightFlickerSubsystem::FlickerTag);
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
	FindLitWindows();
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
	// The lamp breathes near the player, its windows with it; with windows somebody carries it about now and then. Its
	// own intensity and glow are what ApplyState just set, and whatever a later state sets.
	if (ULightFlickerSubsystem* Flicker = ULightFlickerSubsystem::Get(this))
	{
		if (bFlickers)
		{
			Flicker->AddLight(Lamp, Windows, GlowParameter, bCarried && Windows != nullptr);
		}
		else
		{
			Flicker->RemoveLight(Lamp);
		}
	}
	// Somebody at home at the lamplit windows now and then.
	if (bWindowLife && NumLitWindows() > 0)
	{
		if (UWindowLifeSubsystem* Life = UWindowLifeSubsystem::Get(this))
		{
			Life->AddHouse(this);
		}
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
	if (ULightFlickerSubsystem* Flicker = ULightFlickerSubsystem::Get(this))
	{
		Flicker->RemoveLight(Lamp);
	}
	if (UWindowLifeSubsystem* Life = UWindowLifeSubsystem::Get(this))
	{
		Life->RemoveHouse(this);
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
	bDuskNow = bDusk;
	Lamp->SetIntensity(bDusk ? DuskLamp : DayLamp);
	if (Windows)
	{
		Windows->SetScalarParameterValue(GlowParameter, bDusk ? DuskGlow : DayGlow);
	}
}

// ---------------------------------------------------------------------------
// The lamplit windows
// ---------------------------------------------------------------------------

FName AHouseLights::WindowSocketName(int32 Number)
{
	return FName(*FString::Printf(TEXT("Window_%d"), Number));
}

void AHouseLights::FindLitWindows()
{
	LitWindows.Reset();
	if (!House)
	{
		return;
	}
	// The house's mesh with the first window socket (the one with the glowing panes); the rest follow it in order.
	const FName First = WindowSocketName(1);
	for (const UStaticMeshComponent* Mesh : TInlineComponentArray<UStaticMeshComponent*>(House))
	{
		if (Mesh && Mesh->DoesSocketExist(First))
		{
			for (int32 Number = 1; Number <= MaxLitWindows; ++Number)
			{
				const FName Socket = WindowSocketName(Number);
				if (!Mesh->DoesSocketExist(Socket))
				{
					break;
				}
				LitWindows.Add(Mesh->GetSocketTransform(Socket, RTS_World));
			}
			return;
		}
	}
}

bool AHouseLights::GetLitWindow(int32 Index, FTransform& Out) const
{
	if (!LitWindows.IsValidIndex(Index))
	{
		return false;
	}
	Out = LitWindows[Index];
	return true;
}
