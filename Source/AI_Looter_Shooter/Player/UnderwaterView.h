#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnderwaterView.generated.h"

class ASwimWater;
class SWidget;
class UAudioComponent;
class UGameViewportClient;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPlayerLocomotionComponent;
class UPostProcessComponent;
class USoundMix;

/** The swim's and the underwater view's sound cues (Art/Sounds/recipes/swim.py makes them; with none a cue plays nothing). */
namespace SwimCue
{
	/** A body plunging into deep water (3D at the surface): the whump, the cavity's bloop, the crown falling back. */
	inline constexpr const TCHAR* SplashIn = TEXT("Player.Swim.In");
	/** Climbing or jumping out: water pouring off the coat, a slosh (3D at the surface). */
	inline constexpr const TCHAR* SplashOut = TEXT("Player.Swim.Out");
	/** A stroke at the surface: the hands' sweep and its slosh (3D at the hands). */
	inline constexpr const TCHAR* Stroke = TEXT("Player.Swim.Stroke");
	/** A stroke under water: water rushing past the ears (2D). */
	inline constexpr const TCHAR* StrokeUnder = TEXT("Player.Swim.StrokeUnder");
	/** Surfacing short of breath: a breath dragged in (2D). */
	inline constexpr const TCHAR* Gasp = TEXT("Player.Swim.Gasp");
	/** Under water: the pressed, dark hum heard there, a loop (2D). */
	inline constexpr const TCHAR* UnderLoop = TEXT("Player.Underwater.Loop");
	/** The head going under (a gulp of water closing over) and coming up (the air back, water out of the ears) (2D). */
	inline constexpr const TCHAR* UnderEnter = TEXT("Player.Underwater.Enter");
	inline constexpr const TCHAR* UnderExit = TEXT("Player.Underwater.Exit");
	/** A breath let out under water: bubbles bubbling up past the ears (2D). */
	inline constexpr const TCHAR* Bubbles = TEXT("Player.Underwater.Bubbles");
}

/**
 * What the player sees and hears with the view under water (round 4; note 30's "no underwater effects"): made by
 * UPlayerLocomotionComponent as play begins, it watches the view (the camera, first or third person) against the level's
 * swim waters (USwimWaterSubsystem).
 *
 *  - Under: M_UnderwaterPost (Tools/Unreal/build_underwater_assets.py) on an unbound post-process component of its own:
 *    the water's teal fog by distance (deeper, darker), a caustic shimmer on what lies under the surface near it, a soft
 *    dark edge. Off (no pass at all) above water. Its surface height goes to the material (Surface).
 *  - Sound: SMX_Underwater pushed while under (the world muffled through a low-pass; the menus' sounds stay clear), the
 *    hum looped, a gulp going under and the air coming back on surfacing.
 *  - Bubbles (UnderwaterViewBubbles.cpp): a breath let out every few seconds (a burst rising past the view from the mouth)
 *    and a few off the hands at each stroke's push; rings on M_FX_Shape (the effects' own master), rising and wobbling, gone
 *    at the surface. At most MaxBubbles; nothing drawn when none are up.
 *  - The breath ring (UnderwaterViewBreath.cpp): the kit's progress ring under the crosshair while the breath is short (the
 *    head under, or filling up again after), segmented, cyan, red when nearly out.
 *
 * Ticks after the camera moves; above water it only asks the waters once a frame (a few box checks).
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UUnderwaterViewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUnderwaterViewComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** The view is under water now, and how far its look has come in (0..1). */
	bool IsUnderwater() const { return bUnder; }
	float GetBlend() const { return Blend; }

	/** Bubbles up now (tests). */
	int32 NumBubbles() const { return Live.Num(); }

	/** Lets a burst of Count bubbles go at At (world), Spread cm about it (the breath's, a stroke's). */
	void EmitBubbles(const FVector& At, int32 Count, float Spread);

	/** The post-process material and the sound mix (made by Tools/Unreal/build_underwater_assets.py). */
	static const TCHAR* PostMaterialPath;
	static const TCHAR* MixPath;
	/** The bubbles' master: the effects' shape master (a ring), and the plane it's drawn on. */
	static const TCHAR* BubbleMaterialPath;
	static constexpr int32 MaxBubbles = 40;
	/** The breath ring's place among the viewport's widgets (over the HUD's base, under menus). */
	static constexpr int32 BreathZOrder = 6;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Where the view is (the player's camera), and false when there's none (a test, a server). */
	bool FindView(FVector& OutLocation, FRotator& OutRotation) const;
	void SetUnder(bool bNowUnder);
	void UpdatePost(float DeltaTime);
	bool MakePost();

	// UnderwaterViewBubbles.cpp
	void UpdateBubbles(float DeltaTime, const FVector& ViewLocation, const FRotator& ViewRotation);
	void DrawBubbles(const FVector& ViewLocation);
	bool MakeBubbles();

	// UnderwaterViewBreath.cpp
	void UpdateBreathRing(float DeltaTime);
	void ShowBreathRing(bool bShow);

	struct FBubble
	{
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Size = 1.f;
		float Age = 0.f;
		float Life = 1.f;
		float Seed = 0.f;
	};

	TWeakObjectPtr<UPlayerLocomotionComponent> Locomotion;
	TWeakObjectPtr<ASwimWater> Water;
	float SurfaceZ = 0.f;
	bool bUnder = false;
	float Blend = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> Post;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PostMaterial;
	bool bTriedPost = false;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> Mix;
	bool bMixPushed = false;
	TWeakObjectPtr<UAudioComponent> Loop;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> BubbleInstances;
	bool bTriedBubbles = false;
	TArray<FBubble> Live;
	FRandomStream Random{ 0x5b1b };
	/** Seconds to the next breath let out, and the last stroke the hands' bubbles followed. */
	float NextBreath = 2.f;
	int32 SeenStrokes = 0;

	/** The breath ring on the viewport, how much it shows (0..1), and the breath it shows. */
	TSharedPtr<SWidget> BreathOverlay;
	TWeakObjectPtr<UGameViewportClient> BreathViewport;
	float BreathShow = 0.f;
	float BreathShown = 1.f;
	/** Seconds the breath has been full in the air (long since, as play begins: no ring at the start). */
	float BreathFullFor = 100.f;
};
