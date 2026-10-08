#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/WeaponTypes.h"
#include "BenchStage.generated.h"

class UPointLightComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UWeaponModelComponent;

/**
 * The bench screen's stand: a copy of the chosen gun's model (its parts, paint and glow, as UWeaponModelComponent builds
 * it) floating over a ring, turnable, framed to fit, under the studio lights the inventory's stands share (StageStudio).
 * Like the loadout's and the bestiary's stands it stands far outside the level, is seen only by its own camera and renders
 * into the picture the screen shows: only while the screen is open, and only for a moment after something changes (a gun
 * doesn't animate).
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ABenchStage : public AActor
{
	GENERATED_BODY()

public:
	ABenchStage();

	/** Puts the gun on the stand (only a changed gun is rebuilt); bResetTurn turns it back to the opening view. */
	void ShowGun(const FWeaponInstanceData& Gun, bool bResetTurn);

	/** Takes the gun off the stand. */
	void ClearGun();

	bool HasGun() const { return bHasGun; }

	/**
	 * The corners of the box round the part in a slot of the gun on the stand (the slot's index in its definition), in the
	 * world, for marking it on the picture. False when the slot is empty.
	 */
	bool GetSlotCorners(int32 SlotIndex, TArray<FVector>& OutCorners) const;

	/** Renders while active (for a moment after each change); costs nothing while inactive. */
	void SetActive(bool bInActive);

	/** Turns the gun on the spot (degrees, positive turns it to its left as seen from the camera). */
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
	static constexpr int32 ImageWidth = 940;
	static constexpr int32 ImageHeight = 600;

	/** Horizontal field of view (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float FieldOfView = 22.f;

	/** How far the camera looks down on the gun (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float CameraPitch = -18.f;

	/** How much of the picture the gun and its ring fill, across or up and down, whichever they fill first. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float Fill = 0.86f;

	/** How the gun is turned when it's put on the stand: its side to the camera, muzzle to the right, a little toward it. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float DefaultTurn = -72.f;

	/** How high the gun floats over its ring (cm). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float Lift = 16.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float Exposure = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Places the camera and lights for the gun's size, and turns the turntable. */
	void PlaceCamera();

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Root;

	/** Turns with the gun (drag to turn); the camera and lights stay put. */
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

	/** Each of its definition's slots' part on the model, null for an empty slot. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> SlotParts;

	bool bHasGun = false;

	/** Half the framed size: how far the gun reaches from its middle, and up from the floor to the framing's middle. */
	float SubjectReach = 40.f;
	float SubjectHeight = 25.f;
	float RingRadius = 30.f;
	float Turn = 0.f;
	/** Seconds left of rendering every frame after a change, while textures stream in. */
	float SettleTime = 0.f;
	bool bActive = false;
};
