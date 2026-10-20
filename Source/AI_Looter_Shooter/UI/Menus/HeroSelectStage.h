#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HeroSelectStage.generated.h"

class UBodyFigureComponent;
class UHeroDefinition;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class USpotLightComponent;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 * The hero select's live stand (2026-10-20, the room left for it): the chosen hero's game model on a gunmetal plinth,
 * idling, turning slowly and turned by a drag or the right stick, rendered into the picture the screen shows in its
 * middle. Built like the other off-screen stands (UI/Inventory/StageStudio: far outside the level, seen only by its own
 * camera, lit only by its own lights on channel 1), and like the player's body: a hidden mannequin plays the unarmed
 * idle (ABP_Unarmed with the stance layer's weary idle) and the hero's body copies it through their retargeter
 * (UBodyFigureComponent with an explicit hero, SetShownHero). It is a pawn, never possessed, so the anim instance finds
 * an owner standing still and eases the weary idle in, as for the player standing.
 *
 * The whole actor turns (the turntable is the actor: the stance layer reads which way the body faces from its pawn);
 * the camera and lights stand still in world space. Lighting: a warm key spot with the stand's one shadow, a cool fill,
 * a rim from behind in the hero's colour and a kicker from the other side in their first ember power's colour (the
 * hero on channel 1); the plinth on channel 2, lit by the key and fill only (the rims grazing its top tinted it), with a
 * glowing band in the hero's colour. The level's sky light is off in the picture. A new hero turns in from the side
 * with the rim flaring. Lights and brightness tune live: Looter.HeroStand.Key, .Fill, .Rim, .Kicker, .Exposure.
 *
 * Costs nothing while inactive (no tick, no capture, lights off); active, one capture a frame at ImageWidth x
 * ImageHeight of a single character and plinth.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AHeroSelectStage : public APawn
{
	GENERATED_BODY()

public:
	AHeroSelectStage();

	/** The picture's size in pixels; the screen shows it at this aspect ratio (a little smaller, so it's sharp). */
	static constexpr int32 ImageWidth = 720;
	static constexpr int32 ImageHeight = 900;

	/** StageStudio's spot for this stand (0-3 are the inventory's, the bestiary's and the bench's). */
	static constexpr int32 StandIndex = 4;

	/**
	 * Puts Hero on the stand: their body (FigureMesh and Retargeter), the rim and band in their colour, the kicker in
	 * their first power's. bTurnIn: they turn in from the side as the rim flares (a switch); otherwise they simply stand
	 * there. False (and nothing on the stand) when their body isn't in the game: the screen shows their painting instead.
	 */
	bool ShowHero(const UHeroDefinition* Hero, bool bTurnIn);

	const UHeroDefinition* GetHero() const { return Hero; }

	/** The hero's own body is worn (not the mannequin, not Ellis standing in). */
	bool IsHeroWorn() const;

	UBodyFigureComponent* GetFigure() const { return Figure; }

	/** Animates and renders every frame while active; costs nothing while inactive. */
	void SetActive(bool bInActive);
	bool IsActive() const { return bActive; }

	/** The player turned it (degrees; positive turns the front to its left as seen from the camera). The slow sway rests a while after. */
	void AddTurn(float Degrees);

	/** How far the sway and the player have turned it from the three-quarter view, degrees (for tests). */
	float GetTurn() const { return CurrentTurn; }

	/** The confirm's beat: 0 to 1, the rim and key burning brighter as the ember catches. */
	void SetFlare(float Alpha);

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** Brightness the screen applies when it tone maps the picture. */
	float GetExposure() const { return Exposure; }

	/** The camera, framing (horizontal field of view, degrees) and the turn the hero is shown at. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float CameraDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float CameraHeight = 110.f;

	/** The height the camera looks at (the frame's middle), over the plinth's top. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float LookHeight = 96.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float FieldOfView = 20.5f;

	/** The rest turn: a three-quarter view, the hero's front a little to their right (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float DefaultTurn = 12.f;

	/** The slow sway either side of the rest turn (degrees) and its round (seconds). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float SwayDegrees = 16.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float SwaySeconds = 14.f;

	/** The picture's brightness: Looter.HeroStand.Exposure's, read with the lights' (Looter.HeroStand.Key and the rest). */
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	float Exposure = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** The camera and lights where they stand (world space round the stand's spot) and the actor's turn. */
	void PlaceCamera();
	/** The lights' strengths: the rest, the switch's flare and the confirm's. */
	void ApplyLights();
	/** The plinth's materials (made at BeginPlay), coloured for the hero. */
	void ColourPlinth(const FLinearColor& HeroColour);

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Root;

	/** The animated mannequin: hidden under the hero once their body is worn, still animating (the retargeter's source). */
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USkeletalMeshComponent> Body;

	/** The hero's body over it (UBodyFigureComponent, told which hero). */
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UBodyFigureComponent> Figure;

	/** The plinth: a gunmetal top, a darker foot, and a thin band between that glows in the hero's colour. */
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UStaticMeshComponent> PlinthTop;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UStaticMeshComponent> PlinthFoot;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UStaticMeshComponent> PlinthBand;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USpotLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> FillLight;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> RimLight;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> KickerLight;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	/** The plinth's look (M_StylizedSurface: lit, with an emissive Glow), one per part. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceBase;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PlinthMaterials;

	UPROPERTY(Transient)
	TObjectPtr<const UHeroDefinition> Hero;

	/** The player's turn (drags, the stick), the sway's clock and weight, the turn-in left, the switch's flare left. */
	float UserTurn = 0.f;
	float SwayClock = 0.f;
	float SwayWeight = 1.f;
	float SinceUserTurn = 100.f;
	float TurnInLeft = 0.f;
	float SwitchFlareLeft = 0.f;
	float ConfirmFlare = 0.f;
	float CurrentTurn = 0.f;
	bool bActive = false;
};
