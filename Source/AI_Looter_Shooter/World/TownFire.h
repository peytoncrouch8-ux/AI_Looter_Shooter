#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "World/LightingStates.h"
#include "TownFire.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionRunner;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FCampaignRecord;
struct FLightingStateChange;

/**
 * A fire in a town (the towns plan, Docs/Work/2026-11-04-Towns/PLAN.md 4.1): the Company's barrel fire at the gate's
 * sandbags, the smithy's forge and the bathhouse boiler's fire door. The watch fire's recipe (AWatchFire) without its
 * lighting by hand: a few crossed glow cards for the flames (M_FX_Glow, one instanced draw) and a faint wide halo, one
 * shadowless 2400 K point light a few candelas strong that reaches 4.5 m, a faint smoke wisp (SM_SmokePlume, small) and
 * the burning sound, all of them the fire's and nothing else's: it has no interaction and never ticks unless it burns.
 *
 * It burns while the story has it there (ShownWhen, HiddenWhen, read again whenever the missions change: the barrel
 * fire is the Company's, from Main 1 until Main 3 breaks the checkpoint) and the light of the moment lights it
 * (LightingStates::Kind: a forge by day, all of them in the smog's Gloom and at Dusk). The light follows the town's
 * light rules: no shadow, no light function, no translucency lighting, gone beyond 45 m (AHouseLights::LampDrawDistance),
 * hidden when the fire is out. At most two in any view (the plan's budget).
 *
 * The flames flicker on the fire's own clock, near the player every frame and a couple of times a second farther off.
 * The light is not tagged Flicker (ULightFlickerSubsystem): the flames and the light share the one clock, and no moth
 * is drawn to a fire (UAmbientAirSubsystem takes Flicker lights for lamps).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ATownFire : public AActor
{
	GENERATED_BODY()

public:
	ATownFire();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- The rules (apart from the world, for the tests) ---

	/** The fire burns in Kind of light, if the story has it there: its bBurnsByDay, bBurnsInGloom, bBurnsAtDusk. */
	bool BurnsIn(ELightKind Kind) const;

	/** The story has it there in Campaign (Runner adds the missions running in the level): ShownWhen holds (empty: always), HiddenWhen doesn't. */
	bool ShouldShow(const FCampaignRecord* Campaign, const UMissionRunner* Runner) const;

	// --- The state ---

	/** Reads the story again (the missions changed). */
	void RefreshStory();

	/** The story has it there or not (the story's state, the console, tests). */
	void SetShown(bool bInShown);

	/** The fire as the named lighting state has it (the lighting subsystem's changes call it). */
	void ApplyState(FName State);

	bool IsShown() const { return bShown; }

	/** Burning now: shown by the story and lit by the light of the moment. */
	bool IsBurning() const { return bBurning; }

	ELightKind GetKind() const { return KindNow; }

	/** Moves the flicker on by DeltaSeconds: the tick calls it while burning; tests call it. */
	void Advance(float DeltaSeconds);

	/** The light's brightness now (candelas; 0 out), and the flames' height as a share of their own. */
	float GetLightCandela() const;
	float GetFlameScale() const;

	/**
	 * How many town lights are lit within the draw distance of Eyes: house lamps, street lamps and porch lanterns
	 * (AHouseLights::IsLampLit) and fires burning. The plan's cap is 26 in any view; Looter.Lights.Count asks it of the camera.
	 */
	static int32 CountLitInView(UWorld* World, const FVector& Eyes);

	// --- Components ---

	/** The fire bed's middle (a BarrelFire's SOCKET_Fire, the forge's, the boiler's fire door): the flames stand up from it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Flame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FireLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Smoke;

	// --- The story and the light ---

	/** It's there while any of these holds (empty: always) and none of HiddenWhen does. The barrel fire: after Main 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Story")
	TArray<FStoryCondition> ShownWhen;

	/** It's out while any of these holds, whatever ShownWhen says. The barrel fire: after Main 3 (the checkpoint broken). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Story")
	TArray<FStoryCondition> HiddenWhen;

	/** Burns by day (a forge, a boiler: somebody works there), in the smog (Smog, SmogThin) and at dusk (Dusk, DuskSmog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Story")
	bool bBurnsByDay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Story")
	bool bBurnsInGloom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Story")
	bool bBurnsAtDusk = true;

	// --- The look ---

	/** The flames' height and width (cm), how brightly they glow, and their colour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "1"))
	float FlameHeight = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "1"))
	float FlameWidth = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "0"))
	float FlameStrength = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look")
	FLinearColor FlameColor = FLinearColor(1.f, 0.6f, 0.25f);

	/** The light: candelas (3 to 5), how far it reaches (cm), and its warmth (kelvin). Never a shadow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "0"))
	float LightCandela = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "0"))
	float LightRadius = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "1700"))
	float LightTemperature = 2400.f;

	/** Within this of the player it flickers every frame; past it, a couple of times a second (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "0"))
	float FlickerRange = 4000.f;

	/** The smoke wisp: SM_SmokePlume is 8 m tall, so this makes it a faint column of about 2.5 m; it leans downwind (yaw, degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "0"))
	float SmokeScale = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look")
	float SmokeYaw = 19.f;

	/** The burning loop (the watch fire's, smaller): its volume and pitch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "0"))
	float SoundVolume = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Look", meta = (ClampMin = "0.1"))
	float SoundPitch = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Art")
	TObjectPtr<UStaticMesh> CardMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Art")
	TObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Town Fire|Art")
	TObjectPtr<UStaticMesh> SmokeMesh;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The cards: tongues crossed about the upright, a bright round heart low in them, a faint wide halo. */
	void BuildFlame();

	/** The flames, the light, the smoke and the sound as the fire is now (burning or out, the flicker, a catch). */
	void ApplyFire();

	void ApplySound();
	void HandleMissionsChanged();
	void HandleLightingChanged(const FLightingStateChange& Change);

	/** Ticks while burning: every frame near the player, a couple of times a second farther off. */
	void RefreshTick();

	bool IsPlayerNear() const;

	bool bShown = true;
	bool bBurning = false;
	ELightKind KindNow = ELightKind::Day;
	bool bBegun = false;

	/** Seconds burning (the flicker's clock), and since it caught (negative: no catch under way). */
	float BurnTime = 0.f;
	float CatchAge = -1.f;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
	FDelegateHandle LightingHandle;
	TWeakObjectPtr<UAudioComponent> BurnLoop;
};
