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
 *
 * The lamp is a flame (tagged Flicker; ULightFlickerSubsystem): near the player it breathes, gutters now and then, and,
 * with windows, is carried about the house now and again, the windows' glow following it. A house whose model has
 * SOCKET_Window_<n> on its lamplit panes (Art/Models/Buildings/Homestead.py's lived-in homes) has somebody at home at
 * those windows now and then (UWindowLifeSubsystem: a shadow crossing, a curtain twitching).
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

	/**
	 * Who lives here, for the town's life (TownLifeRules: Ransom, Bright, Pruitt, CottageNorth, CottageSouth...): the house
	 * is heard through its walls now and then as the player passes. None: from the house's model (Delia's farmhouse,
	 * Tilly's shop, Pruitt's store, a settler's cottage), or nobody for any other.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	FName Household;

	/** The lamp burns like a flame near the player (ULightFlickerSubsystem); off: steady. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	bool bFlickers = true;

	/**
	 * With windows, the lamp is carried about the house now and then: it and the windows dim slowly and come back
	 * (somebody moving about inside). Off for a lamp nobody carries (one hung under a canopy).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	bool bCarried = true;

	/** Somebody shows at the lamplit windows now and then (UWindowLifeSubsystem), where the house's model has them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	bool bWindowLife = true;

	/** The household it's heard as now: Household, or the house's model's. */
	FName GetHousehold() const;

	/** Lamp and windows as the named lighting state has them. */
	void ApplyState(FName State);

	/** Whether the lamp and windows are lit for dusk now (the last state applied). */
	bool IsDusk() const { return bDuskNow; }

	/** The windows' instance on the house (null without a house or a GlowSlot): set up as play begins. */
	UMaterialInstanceDynamic* GetWindows() const { return Windows; }

	/**
	 * The house's lamplit windows: its model's SOCKET_Window_1, _2... (each at a lit pane's outer face, its middle, X out
	 * through the glass, Z up), found in the world as play begins (a house never moves). Out: window Index (from 0).
	 */
	int32 NumLitWindows() const { return LitWindows.Num(); }
	bool GetLitWindow(int32 Index, FTransform& Out) const;

	/** The socket names its lit windows are found by: Window_1, Window_2... */
	static FName WindowSocketName(int32 Number);

	/** The most lit windows a house is looked for (a socket past it is never found). */
	static constexpr int32 MaxLitWindows = 16;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnLightingChanged(const FLightingStateChange& Change);

	/** The house's window sockets in the world, from Window_1 on as long as each is there (on its mesh that has the first). */
	void FindLitWindows();

	/** The house's windows, made a dynamic instance of their own so only this house changes. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Windows;

	TArray<FTransform> LitWindows;
	bool bDuskNow = false;

	FDelegateHandle Listening;
};
