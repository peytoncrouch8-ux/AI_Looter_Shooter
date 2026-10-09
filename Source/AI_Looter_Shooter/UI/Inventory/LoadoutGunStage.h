#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/WeaponTypes.h"
#include "LoadoutGunStage.generated.h"

class UPointLightComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UTextureRenderTarget2D;
class UWeaponModelComponent;

/**
 * The loadout screen's showcase: the gun under the cursor, built from its parts (paint, wear, rarity glow and notches, as
 * UWeaponModelComponent builds it), floating over a ring under the studio lights the inventory's stands share
 * (StageStudio). Each new gun swings in to its three-quarter view, then sways gently so its side stays readable; a drag
 * turns it, and the sway waits a moment before it comes back. Like the other stands it stands far outside the level, is
 * seen only by its own camera and renders into the picture the screen shows, and only while the screen is open.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ALoadoutGunStage : public AActor
{
	GENERATED_BODY()

public:
	ALoadoutGunStage();

	/** Puts the gun on the stand; a new gun (not the same one again) is rebuilt and swings in. */
	void ShowGun(const FWeaponInstanceData& Gun);

	/** Takes the gun off the stand. */
	void ClearGun();

	bool HasGun() const { return bHasGun; }

	/** Renders every frame while active; costs nothing while inactive. */
	void SetActive(bool bInActive);

	/** Turns the gun on the spot (degrees, positive turns it to its left as seen from the camera); the sway pauses. */
	void AddTurn(float Degrees);

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** Where a world point shows in the picture, as 0..1 across and down. False when it's behind the camera. */
	bool ProjectToImage(const FVector& WorldLocation, FVector2D& OutUV) const;

	/** The floor under the gun, where its ring lies. */
	FVector GetFloorCenter() const;

	/** Level direction from the gun toward the camera. */
	FVector GetTowardCamera() const;

	/** The ring under the gun (cm). */
	float GetRingRadius() const { return RingRadius; }

	float GetExposure() const { return Exposure; }

	/** The picture's size in pixels; the screen shows it at this aspect ratio (a gun is long and low). */
	static constexpr int32 ImageWidth = 1024;
	static constexpr int32 ImageHeight = 654;

	/** Horizontal field of view (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float FieldOfView = 22.f;

	/** How far the camera looks down on the gun (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float CameraPitch = -14.f;

	/** How much of the picture the gun and its ring fill, across or up and down, whichever they fill first. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float Fill = 0.84f;

	/** The view it settles on: its side to the camera, muzzle to the right, a little toward it. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float DefaultTurn = -72.f;

	/** A new gun swings in from this far round (degrees), over SwingSeconds, slowing as it lands. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float SwingDegrees = 55.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float SwingSeconds = 0.45f;

	/** Then it sways this far either side (degrees), once every SwayPeriod seconds. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float SwayDegrees = 12.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float SwayPeriod = 7.f;

	/** After a drag the sway comes back this many seconds later, easing in. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float SwayResumeSeconds = 3.f;

	/** How high the gun floats over its ring (cm). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float Lift = 16.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float Exposure = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Places the camera and lights for the gun's size. */
	void PlaceCamera();
	/** Turns the turntable to the turn, the swing and the sway. */
	void PlaceTurn();

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Root;

	/** Turns with the gun; the camera and lights stay put. */
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Turntable;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> FillLight;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> RimLight;

	/** The gun's model, made the first time a gun is shown. */
	UPROPERTY(Transient)
	TObjectPtr<UWeaponModelComponent> Model;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	/** The gun on the stand, to tell when it changes. */
	UPROPERTY(Transient)
	FWeaponInstanceData Shown;

	bool bHasGun = false;
	bool bActive = false;

	/** Half the framed size: how far the gun reaches from its middle, and up from the floor to the framing's middle. */
	float SubjectReach = 40.f;
	float SubjectHeight = 25.f;
	float RingRadius = 30.f;

	/** The turn the player has added by dragging (degrees, on top of DefaultTurn). */
	float UserTurn = 0.f;
	/** Seconds since the gun swung in, and since the last drag (the sway waits for it). */
	float SwingAge = 100.f;
	float SinceDrag = 100.f;
	/** The sway's clock: it runs only while the sway shows, so it picks up where it left off. */
	float SwayClock = 0.f;
};
