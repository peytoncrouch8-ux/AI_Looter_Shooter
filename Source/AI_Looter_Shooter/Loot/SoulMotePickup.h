#pragma once

#include "CoreMinimal.h"
#include "Combat/RecoverySettings.h"
#include "GameFramework/Actor.h"
#include "SoulMotePickup.generated.h"

class APawn;
class UHealthComponent;
class USceneComponent;
class UStaticMeshComponent;

/** The sound cues a soul-mote plays (Art/Sounds/cues.json gives each its sounds; with none a cue plays nothing). */
namespace SoulMoteCue
{
	/** A kill lets go of its mote or motes: a soft rising chime at the body (3D). */
	inline constexpr const TCHAR* Drop = TEXT("Loot.SoulMote.Drop");
	/** A mote taken into the player: a warm little swell, heard where it was (3D). */
	inline constexpr const TCHAR* Pickup = TEXT("Loot.SoulMote.Pickup");
}

/**
 * A soul-mote: a small pale-green wisp some kills leave behind (Docs/Polish/BorderlandsComparison.md, item 3;
 * FRecoverySettings has every number). It rises out of the body, floats at chest height with a soft bob and a faint trail,
 * and heals 15% of the most health of a hurt player who comes near: within a few meters it drifts toward them, faster
 * the closer it gets, and a short run past takes it. It ignores a player at full health (it is no use to them) and waits
 * for them, about 30 seconds, flickering out through the last few. Green is the HUD's heal color (LooterUI::Color::Heal),
 * the one color on a drop that doesn't mean rarity; no light of its own and no beam, as the ammo drops (an emissive
 * surface, which also draws on the Nanite fallback). Drawn as opaque emissive spheres from the stylized surface material,
 * so it fades by shrinking and flickering rather than by translucency.
 *
 * Its motion is Advance, which the tick calls with the nearest player; a test world, which doesn't tick, calls it directly.
 */
UCLASS(NotBlueprintable)
class AI_LOOTER_SHOOTER_API ASoulMotePickup : public AActor
{
	GENERATED_BODY()

public:
	ASoulMotePickup();

	/** Spawns one mote at Where, launched with Velocity (cm/s); it settles up to hover over the ground below. */
	static ASoulMotePickup* SpawnMote(UWorld* World, const FVector& Where, const FVector& Velocity);

	/**
	 * Spawns Count motes at Where (a kill's body), each rising on its own heading round the ring so they fan out, with the
	 * drop's chime once. The recovery numbers in force in World (UPlayerVitalsSubsystem::SettingsFor) are the motes'.
	 */
	static TArray<ASoulMotePickup*> SpawnMotes(UWorld* World, const FVector& Where, int32 Count);

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Moves it on DeltaSeconds with Player (the nearest player's pawn, or null): it ages, floats, drifts toward a player who
	 * needs healing and is near, and heals them on reaching them. True when it healed Player (and is gone). Past its life
	 * it destroys itself and returns false.
	 */
	bool Advance(float DeltaSeconds, APawn* Player);

	/** Whether Health could use a mote: alive and short of the most by more than half a point. */
	static bool NeedsHealing(const UHealthComponent& Health);

	/** How far Point is from the nearest part of Player's body (cm): its capsule's surface for a character. */
	static float DistanceToBody(const FVector& Point, const APawn& Player);

	float GetAge() const { return Age; }

	/** How much of it is left to see (1 to 0 through its last seconds). */
	float GetVisible() const { return Settings.MoteVisible(Age); }

	const FRecoverySettings& GetSettings() const { return Settings; }
	void SetSettings(const FRecoverySettings& NewSettings) { Settings = NewSettings; }

	/** The glowing core, and the three faint spheres that lag behind it. */
	UStaticMeshComponent* GetCore() const { return Core; }
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetTrail() const { return Trail; }

	/** The core's size across at rest (cm). */
	static constexpr float CoreDiameter = 17.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	/** Starts it moving from From: Velocity out of the body, and the hover height found over the ground below. */
	void Launch(const FVector& From, const FVector& Velocity);

	/** The nearest local player's pawn, or null. */
	APawn* FindPlayerPawn() const;

	/** Heals Player and goes. */
	void Collect(APawn& Player, UHealthComponent& Health);

	/** Floats on: drift fades out, the height settles on the hover plus a bob. */
	void Float(float DeltaSeconds);

	/** Sizes the core and the trail for the age and the pulse, and trails the spheres after it. */
	void UpdateVisuals(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Trail;

	/** Copied from the world's recovery numbers as it is spawned. */
	UPROPERTY(Transient)
	FRecoverySettings Settings;

	FVector Velocity = FVector::ZeroVector;
	/** The height (world Z) it floats at, before the bob. */
	double HoverZ = 0.0;
	float Age = 0.f;
	float BobPhase = 0.f;
	/** Where each trailing sphere is now (world), each easing after the one before it. */
	TArray<FVector> TrailAt;
};
