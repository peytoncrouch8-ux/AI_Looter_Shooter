#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "WatchFire.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionRunner;
class UPointLightComponent;
class UStaticMesh;
struct FCampaignRecord;

/**
 * The watch fire on Calder's Post's tower (Docs/Areas/RansomsRest_Expansion.md, "The Last Watch", step 3 "Light the watch
 * fire on the tower"): the deck's iron brazier (the watchtower model's own, its SOCKET_WatchFire at the fire bed), dark
 * until the player holds Interact at it once The Last Watch has them bring the oil, and burning for good after: the story
 * keeps it lit (LitFlag in the campaign, set as it catches; and after its mission, LitWhen), so every later visit finds
 * the posts down the Rim answered.
 *
 * Cheap, as every lantern on Ransom's Rest: a few crossed glow cards (M_FX_Glow, one instanced draw) for the flames and a
 * soft halo that carries a long way, and one small shadowless warm light that's gone past a long walk. It flickers only
 * while the player is near enough to see it (FlickerRange), a few times slower when they're far; dark, it never ticks.
 *  - Lightable (its LightableWhen: The Last Watch from its third step): hold Interact (1.5 s) to light it. It catches with
 *    a flare, the missions hear the held Interact (its objective's actor is tagged WatchFire by the build script).
 *  - Before that, while its mission runs (TriesWhen): a tap finds the brazier dry, and a caption says it wants oil.
 *  - Lit, it's used up (IInteractable::IsUsedUp): a lasting interact objective counts it whenever it looks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AWatchFire : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AWatchFire();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual bool IsUsedUp() const override { return bLit; }

	// --- The rules (apart from the world, for the tests) ---

	/** Any of Conditions holds in Campaign (an empty list: none). */
	static bool AnyMet(const TArray<FStoryCondition>& Conditions, const FCampaignRecord& Campaign, const UMissionRunner* Runner);

	/** The fire's flicker Seconds in: a multiplier about 1 (0.83 to 1.17), three unsteady waves that never fall in step. */
	static float Flicker(float Seconds);

	/** A lighting's flare Since seconds after it caught: 0 before, swelling to 1 in a quarter second, gone by 1.75 s. */
	static float Flare(float Since);

	/** How far the flames have caught Since seconds after the light (0 to 1 over 0.6 s; 1 when lit without a flare). */
	static float Catch(float Since);

	// --- The story ---

	/** The story says it burns: its flag set, or LitWhen holds. */
	bool ShouldBeLit(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const;

	/** It can be lit now: dark, and LightableWhen holds. */
	bool CanLight() const;

	/** The brazier would be found dry now: dark, not lightable, and TriesWhen holds. */
	bool IsDryNow() const;

	/** Reads the story again (the missions changed): lit as it says, without a flare. */
	void RefreshStory();

	// --- Using it ---

	/** Lights it as the player's hold would: it catches with a flare, its flag set and the session saved soon. False when
	 *  it can't be lit now. */
	bool Light(AActor* User);

	/** Lit or dark at once, no flare (the story's state, the console, tests). */
	void SetLit(bool bInLit);

	bool IsLit() const { return bLit; }

	/** Moves the flicker and a flare on by DeltaSeconds: the tick calls it while lit; tests call it. */
	void Advance(float DeltaSeconds);

	/** The light's brightness now (candelas; 0 dark), and the flames' height as a share of their own. */
	float GetLightCandela() const;
	float GetFlameScale() const;

	// --- Components ---

	/** The fire bed's middle (the brazier's SOCKET_WatchFire): the flames stand up from it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Flame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FireLight;

	// --- The story ---

	/** The campaign flag it sets as it catches, and that keeps it lit on every later visit. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Story")
	FName LitFlag = TEXT("WatchFire.Larkspur");

	/** When it can be lit (any one): The Last Watch from its third step (counted from 0: 2), the oil found. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Story")
	TArray<FStoryCondition> LightableWhen;

	/** When the story has it burning without its flag (any one): after The Last Watch, for saves that lost the flag. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Story")
	TArray<FStoryCondition> LitWhen;

	/** When a tap finds it dry (any one): The Last Watch under way, before the oil. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Story")
	TArray<FStoryCondition> TriesWhen;

	// --- Using it ---

	UPROPERTY(EditAnywhere, Category = "WatchFire|Use")
	FText Prompt;

	/** The caption when the brazier's found dry. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Use", meta = (MultiLine = true))
	FText DryLine;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Use", meta = (ClampMin = "0.05"))
	float HoldSeconds = 1.5f;

	/** How far from the player's eyes it can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Use", meta = (ClampMin = "0"))
	float Reach = 0.f;

	/** Raised over the socket to where the player looks to light it (cm): the brazier's rim. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Use")
	float UseHeight = 20.f;

	// --- The look ---

	/** The flames' height and width (cm), how brightly they glow, and their colour. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Look", meta = (ClampMin = "1"))
	float FlameHeight = 130.f;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Look", meta = (ClampMin = "1"))
	float FlameWidth = 60.f;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Look", meta = (ClampMin = "0"))
	float FlameStrength = 12.f;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Look")
	FLinearColor FlameColor = FLinearColor(1.f, 0.6f, 0.25f);

	/** The light: a fire's warm glow on the deck and the tower's timbers, never a shadow, gone past a long walk. */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Look", meta = (ClampMin = "0"))
	float LightCandela = 60.f;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Look", meta = (ClampMin = "0"))
	float LightRadius = 1800.f;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Look", meta = (ClampMin = "0"))
	float LightDrawDistance = 9000.f;

	/** Within this of the player it flickers every frame; past it, a couple of times a second (cm). */
	UPROPERTY(EditAnywhere, Category = "WatchFire|Look", meta = (ClampMin = "0"))
	float FlickerRange = 6000.f;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Art")
	TObjectPtr<UStaticMesh> CardMesh;

	UPROPERTY(EditAnywhere, Category = "WatchFire|Art")
	TObjectPtr<UMaterialInterface> GlowMaterial;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The cards: tongues crossed about the upright, a bright round heart low in them, a faint wide halo seen from afar. */
	void BuildFlame();

	/** The flames and the light as the fire is now (lit or dark, the flicker, a flare). */
	void ApplyFire();

	/** Starts or stops the burning's sound with the fire. */
	void ApplySound();

	void HandleMissionsChanged();

	/** Ticks while lit: every frame near the player or in a flare, a couple of times a second farther off. */
	void RefreshTick();

	/** The player's eyes (or body) are within FlickerRange. */
	bool IsPlayerNear() const;

	bool bLit = false;
	/** Seconds burning (the flicker's clock), and since it caught (negative: no flare). */
	float BurnTime = 0.f;
	float FlareAge = -1.f;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
	TWeakObjectPtr<UAudioComponent> BurnLoop;
};
