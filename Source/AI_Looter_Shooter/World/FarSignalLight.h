#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "FarSignalLight.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionRunner;
class UStaticMesh;
struct FCampaignRecord;
struct FLightingStateChange;

/**
 * The far answering light (Docs/Areas/RansomsRest_Expansion.md, "The Last Watch", step 5): "Far down the Rim a light winks
 * on a mesa", a Rangers' post a few kilometres off answering Larkspur's watch fire by heliograph in the day and lamp by
 * night. An unlit flicker card on the nearest backdrop mesa (Tools/Unreal/build_area_lift.py places it), shown by the
 * story (ShownWhen, any of them, or ShownFlag): from The Last Watch's answer on, and after it.
 *
 * It keys its message in Morse (Message, a unit of UnitSeconds: a dot is one on, a dash three, one off between them, three
 * between letters, seven between words, a long pause before it starts again), each wink easing on and off a little so it
 * reads as a light, not a switch. Ruth's code book, read in the mission, turns the winks into words.
 *
 * Cheap: three crossed round glow cards (M_FX_Glow, one instanced draw, never culled by distance), big enough to be a
 * point of light from the watchtower 3 km off. Hidden, it never ticks; shown, it ticks at 30 a second to key the message.
 *
 * Steady (bSteady): no message, only a slow breath (BreathHz, ±BreathDepth), ticking 15 a second: the second light far
 * west over the fog seen from Gravewind Point at dusk (Docs/Story.md, Seeds: "a second light, low and steady"; placed by
 * Tools/Unreal/build_area_seeds.py). ShownState gates it by the level's lighting state as ADuskScenery is gated (set:
 * shown only while ULightingStateSubsystem's state is that one; None: any), on top of the story's ShownWhen/ShownFlag.
 * From Python: set_editor_property('steady', True), ('shown_state', 'Dusk'), ('shown_when', [...]).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFarSignalLight : public AActor
{
	GENERATED_BODY()

public:
	AFarSignalLight();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- The signal as plain math (the tests) ---

	/** Text in Morse, one entry per unit of time: on or off, with the gaps between elements, letters and words, and the
	 *  pause before it starts again. Letters and digits only; anything else is a gap between words. */
	static TArray<bool> KeyUnits(const FString& Text);

	/** How bright the light is (0 to 1) Seconds into keying Keyed at Unit seconds a unit, repeating; each wink eases on and
	 *  off over EdgeShare of a unit. */
	static float KeyAt(const TArray<bool>& Keyed, float Seconds, float Unit, float EdgeShare = 0.18f);

	/** The pause before the message starts again, in units. */
	static constexpr int32 RepeatGapUnits = 14;

	/** A steady light's brightness Seconds in: 1 breathing ±Depth at Hz (a lamp's flame behind glass, not a switch). */
	static float BreathAt(float Seconds, float Hz, float Depth);

	/** A light gated to ShownState may show in the lighting state State (None ShownState: any state, any level). */
	static bool StateAllows(FName InShownState, FName State);

	// --- The story ---

	/** The story shows it in Campaign: ShownFlag set, or any of ShownWhen. */
	bool ShouldShow(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const;

	/** Reads the story again (the missions changed), and shows it if the lighting state allows too. */
	void RefreshStory();

	/** The level's lighting state is now State (its switches call it; tests may): shown again as story and state allow. */
	void ApplyLightingState(FName State);

	/** Shown or not, now (the story's state, the console, tests). */
	void SetShown(bool bInShown);
	bool IsShown() const { return bShown; }

	/** Keys the message on by DeltaSeconds: the tick calls it while shown; tests call it. */
	void Advance(float DeltaSeconds);

	/** How bright it is now (0 to 1 of Strength; a steady light's breath may reach a little over 1). */
	float GetKey() const { return LastKey; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Card;

	/** When it shows (any one): from The Last Watch's answer (its fifth step, from 0: 4) on, and after it. */
	UPROPERTY(EditAnywhere, Category = "Signal|Story")
	TArray<FStoryCondition> ShownWhen;

	/** Or while this campaign flag is set (None: none). */
	UPROPERTY(EditAnywhere, Category = "Signal|Story")
	FName ShownFlag;

	/** Always shown, whatever the story or the light (to judge its look from the watchtower). */
	UPROPERTY(EditAnywhere, Category = "Signal|Story")
	bool bAlwaysShown = false;

	/** The lighting state it's seen in (Dusk), as ADuskScenery's: shown only while the level is in it. None: any. */
	UPROPERTY(EditAnywhere, Category = "Signal|Story")
	FName ShownState;

	/** Burns steady with a slow breath instead of keying Message. */
	UPROPERTY(EditAnywhere, Category = "Signal")
	bool bSteady = false;

	/** A steady light's breath: how fast (Hz) and how deep (a share of Strength either way). */
	UPROPERTY(EditAnywhere, Category = "Signal", meta = (ClampMin = "0.01", EditCondition = "bSteady"))
	float BreathHz = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Signal", meta = (ClampMin = "0", ClampMax = "0.5", EditCondition = "bSteady"))
	float BreathDepth = 0.08f;

	/** What it keys, and the length of a unit (s): a signal lamp's steady hand. */
	UPROPERTY(EditAnywhere, Category = "Signal")
	FString Message = TEXT("LARKSPUR WE SEE YOU");

	UPROPERTY(EditAnywhere, Category = "Signal", meta = (ClampMin = "0.05"))
	float UnitSeconds = 0.22f;

	/** Its size (cm: about 6 pixels at 1080p from 3 km), colour (a heliograph's sunlit white, warm) and glow. */
	UPROPERTY(EditAnywhere, Category = "Signal", meta = (ClampMin = "1"))
	float Size = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Signal")
	FLinearColor Color = FLinearColor(1.f, 0.9f, 0.72f);

	UPROPERTY(EditAnywhere, Category = "Signal", meta = (ClampMin = "0"))
	float Strength = 30.f;

	UPROPERTY(EditAnywhere, Category = "Signal|Art")
	TObjectPtr<UStaticMesh> CardMesh;

	UPROPERTY(EditAnywhere, Category = "Signal|Art")
	TObjectPtr<UMaterialInterface> GlowMaterial;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The three crossed cards. */
	void BuildCards();

	/** The cards' glow at Key (0 to 1): set only when it has changed enough to see. */
	void ApplyKey(float Key, bool bForce);

	void HandleMissionsChanged();
	void HandleLightingChanged(const FLightingStateChange& Change);

	/** Shown as the story (last read) and the lighting state allow, or always. */
	void ApplyGates();

	/** How bright it is Clock seconds in: keyed, or breathing. */
	float BrightnessNow() const;

	bool bShown = false;
	bool bStoryShown = false;
	FName LightingState;
	FDelegateHandle LightingChangedHandle;
	float Clock = 0.f;
	float LastKey = 0.f;
	TArray<bool> Units;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
