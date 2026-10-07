#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HouseLights.generated.h"

class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;
struct FLightingStateChange;

/**
 * A lived-in house's lights, for the lighting states: a warm lamp in the hall, seen through the screen door, and the
 * windows' glow, both brighter at dusk (Grandma Delia's farmhouse, SM_Farmhouse_Ransom). Tools/Unreal/build_area_story.py
 * places it on the house's SOCKET_Light (the hall lamp) and names the house; the windows are the house mesh's WindowGlow
 * slot (M_StylizedSurface's Glow).
 *
 * The lamp is unshadowed and reaches no further than the hall (AttenuationRadius), so it never lights the porch through
 * the front wall: a shadowed point light would cost a cube shadow map on Medium for one lamp. Nothing ticks: it listens
 * for ULightingStateSubsystem's switches and sets the lamp and the glow then.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AHouseLights : public AActor
{
	GENERATED_BODY()

public:
	AHouseLights();

	/** The hall lamp, at the actor's origin. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Lamp;

	/** The house whose windows glow: its first static mesh with a GlowSlot material slot. Empty: only the lamp. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "House Lights")
	TObjectPtr<AActor> House;

	/** The windows' material slot on the house's mesh, and the scalar parameter that brightens them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	FName GlowSlot = FName(TEXT("WindowGlow"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	FName GlowParameter = FName(TEXT("Glow"));

	/** The lighting state the house is lit up for (the lamp and the windows at their dusk values); any other is day. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	FName DuskState = FName(TEXT("Dusk"));

	/** The lamp's intensity by day and at dusk (candelas): a glimmer in the hall by day, a warm doorway at dusk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights", meta = (ClampMin = "0"))
	float DayLamp = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights", meta = (ClampMin = "0"))
	float DuskLamp = 5.5f;

	/** The windows' Glow by day and at dusk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights", meta = (ClampMin = "0"))
	float DayGlow = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights", meta = (ClampMin = "0"))
	float DuskGlow = 14.f;

	/** Lamp and windows as the named lighting state has them. */
	void ApplyState(FName State);

	/** The windows' instance on the house (null without a house or a GlowSlot): set up as play begins. */
	UMaterialInstanceDynamic* GetWindows() const { return Windows; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnLightingChanged(const FLightingStateChange& Change);

	/** The house's windows, made a dynamic instance of their own so only this house changes. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Windows;

	FDelegateHandle Listening;
};
