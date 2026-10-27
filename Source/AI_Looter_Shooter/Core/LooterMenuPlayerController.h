#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LooterMenuPlayerController.generated.h"

class ACameraActor;

/** The menu's backdrop scene, Lvl_MenuTeropa (Tools/Unreal/build_menu_teropa.py places and tags it). */
namespace MenuScene
{
	/** The authored view: the camera the menu looks through, framed so the planet keeps clear of the menu's column. */
	inline const FName ViewTag(TEXT("MenuView"));
	/** Teropa: a proxy sphere wearing MI_Planet_Teropa, whose PlanetRadius parameter is the planet's own radius. */
	inline const FName PlanetTag(TEXT("MenuPlanet"));
	inline const FName MoonTag(TEXT("MenuMoon"));
	inline const FName StarsTag(TEXT("MenuStars"));
	/** M_Planet's radius parameter: the sphere it draws (its proxy mesh is a little bigger). */
	inline const FName RadiusParameter(TEXT("PlanetRadius"));
	/**
	 * Where the menu's column ends, as a screen x (NDC, -1 left .. 1 right): the menu and its title own the left 40% of the
	 * screen, the planet the right 60% (the round 3 contract's split). Horizontal field of view is kept across aspect ratios,
	 * so this holds on any screen.
	 */
	inline constexpr float ColumnEdgeNdc = -0.2f;
}

/**
 * The main menu's player: no character, and a view of the backdrop. In the menu's own level (Lvl_MenuTeropa) it looks
 * through the authored camera (tag MenuView) at Teropa from orbit, with a slow drift that never loops visibly, a slight
 * turn toward the cursor and an ease-in as the menu opens. In any other level (the editor's Play over an open level) it
 * circles that level's island instead (the actors tagged Ground, as the minimap finds it), set right of centre.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALooterMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALooterMenuPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- The authored view (Lvl_MenuTeropa) ---

	/** The drift's reach, degrees each way (pitch, yaw, roll): the planet breathes on screen without ever nearing the menu. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera|Planet")
	FRotator DriftReach = FRotator(0.4f, 0.7f, 0.3f);

	/** How far the view turns toward the cursor at the screen's edge, degrees (yaw for its x, pitch for its y). */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera|Planet")
	FVector2D ParallaxReach = FVector2D(0.6f, 0.35f);

	/** How quickly the view follows the cursor (interp speed): slow enough to feel like weight, not lag. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera|Planet")
	float ParallaxSpeed = 1.6f;

	/** The ease-in as the menu opens: the view starts this far right (yaw, degrees) and settles over IntroSeconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera|Planet")
	float IntroYaw = 2.5f;

	// --- The island orbit (any other level) ---

	/** How fast the view circles the island: a full turn takes 360 / this seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float DegreesPerSecond = 1.5f;

	/** Where the circle starts, in degrees around the island (0 north, 90 east): the waterfall's side. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float StartAngle = 50.f;

	/** How far out it circles, in island radii from its center. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float Distance = 2.2f;

	/** How high over the island's ground it flies, in island radii. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float Height = 0.3f;

	/** How far under the ground's top the view aims, in island radii, so its cliffs and underside show. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float AimDrop = 0.2f;

	/** Degrees the view turns left of the island, which then sits right of the menu. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float AimOffset = 14.f;

	UPROPERTY(EditDefaultsOnly, Category = "Menu Camera")
	float FieldOfView = 60.f;

	/** The drift's turn at a moment (Reach each way, scaled; 0 is still): three slow waves of unrelated periods. */
	static FRotator DriftAt(float Seconds, const FRotator& Reach, float Scale);

	/** The turn toward a cursor at CursorNdc (-1..1 each way, +y up): right and up follow it. */
	static FRotator ParallaxFor(const FVector2D& CursorNdc, const FVector2D& Reach, float Scale);

	/** The ease-in's yaw left at a moment: Yaw at 0 s, easing out to none at Seconds' end (none when Seconds is 0). */
	static float IntroYawAt(float Elapsed, float Seconds, float Yaw);

	/**
	 * The leftmost screen x (NDC) of a sphere seen from View (X forward, Y right, Z up) at a horizontal field of view:
	 * its exact outline, sampled. -inf when the eye is inside it or it reaches behind the eye.
	 */
	static float LeftmostNdcX(const FTransform& View, float FovDegrees, const FVector& Centre, float Radius);

	/** The radius M_Planet draws on Planet (its PlanetRadius), or 0 when it wears something else. */
	static float PlanetRadiusOf(const AActor* Planet);

private:
	/** Looks for the authored view and starts on it; false in a level without one. */
	bool StartAuthoredView();

	/** The authored view this frame: its base turned by the ease-in, the drift and the cursor. */
	void PlaceAuthoredView(float DeltaSeconds);

	/** The island orbit: puts the camera at Angle on its circle, looking at the island. */
	void PlaceCamera();

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> OrbitCamera;

	/** True in the menu's own level: the camera follows the authored view, not the island orbit. */
	bool bAuthoredView = false;
	FTransform BaseView;
	FVector2D Parallax = FVector2D::ZeroVector;
	float Clock = 0.f;

	FVector Center = FVector::ZeroVector;
	float Radius = 10000.f;
	float Top = 0.f;
	float Angle = 0.f;
};
