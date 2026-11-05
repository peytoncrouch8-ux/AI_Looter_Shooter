#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/LightingStates.h"
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
 *
 * Street lamps and porch lanterns are this class too (town.json's lights: day 0). Three levels of light, by what the
 * lighting state gives the town (LightingStates::Kind): Day, Gloom (Smog, SmogThin: the lamps lit dim against the smog) and
 * Dusk (Dusk and DuskSmog). A house that sets no Gloom values is lit in Gloom as by day. The towns plan's rules for a town
 * light (Docs/Work/2026-11-04-Towns/PLAN.md 4.1): never a shadow, no light function, no translucency lighting, drawn out to
 * 45 m only, and hidden at 0 candelas so a lamp that is off by day costs nothing.
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

	/**
	 * The lamp's intensity by day and at dusk (candelas): a glimmer in the hall by day, a warm doorway at dusk. At dusk means
	 * the lighting states Dusk and DuskSmog (LightingStates::Kind). A lamp at 0 is hidden (a street lamp is out by day).
	 */
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
	 * The lamp's candelas and the windows' Glow in Gloom (Smog, SmogThin: the town lights its lamps against the smog, dim:
	 * a street lamp 3, a porch lantern 2, a lived-in house's windows 4). Negative: as by day (a house that sets nothing is
	 * lit in the smog as it always was: the depot, the workshop, Whitlock's barn).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	float GloomLamp = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "House Lights")
	float GloomGlow = -1.f;

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

	/** Lamp and windows as the named lighting state has them (by its kind: Day, Gloom or Dusk). */
	void ApplyState(FName State);

	/** The lamp's candelas and the windows' Glow in a kind of light (Gloom without values of its own: the day's). */
	float GetLampFor(ELightKind Kind) const;
	float GetGlowFor(ELightKind Kind) const;

	/** The kind of light the last state applied gave it (Day before any). */
	ELightKind GetKind() const { return KindNow; }

	/** Whether the lamp and windows are lit for dusk now (Dusk or DuskSmog: the last state applied). */
	bool IsDusk() const { return KindNow == ELightKind::Dusk; }

	/** Whether they are lit dim for the smog now (Smog or SmogThin). */
	bool IsGloom() const { return KindNow == ELightKind::Gloom; }

	/** Whether the lamp is lit now: shown, and at more than 0 candelas. */
	bool IsLampLit() const;

	/** A town lamp is drawn out to this far from the view (cm), fading over the last LampFadeRange of it: the towns plan's 45 m. */
	static constexpr float LampDrawDistance = 4500.f;
	static constexpr float LampFadeRange = 1000.f;

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
	ELightKind KindNow = ELightKind::Day;

	FDelegateHandle Listening;
};
