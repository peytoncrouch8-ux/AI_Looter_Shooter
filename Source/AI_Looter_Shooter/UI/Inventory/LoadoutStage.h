#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapons/WeaponTypes.h"
#include "LoadoutStage.generated.h"

class ACharacter;
class AWeaponBase;
class UPointLightComponent;
class UPrimitiveComponent;
class USceneCaptureComponent2D;
class USkeletalMeshComponent;
class UTextureRenderTarget2D;
class UWeaponManagerComponent;
class UWeaponModelComponent;

/** Where an equipped gun is carried on the character. */
enum class ELoadoutCarry : uint8
{
	None,
	InHand,
	Back,
	Hip
};

namespace LoadoutCarry
{
	/**
	 * The gun in hand is held; the others are carried in slot order, the first on the back and the next at the hip (with
	 * more slots they keep alternating). Slots past the last weapon carry nothing. Given each slot's kind (KindsOf), a
	 * holstered Revolver (the Drover) rides on the hip, where a six-gun is drawn from, and the long guns on the back; the
	 * kinds alternate within themselves as above (two revolvers: the hip, then the back).
	 */
	AI_LOOTER_SHOOTER_API ELoadoutCarry ForSlot(int32 Slot, int32 NumWeapons, int32 ActiveSlot, TConstArrayView<EWeaponKind> Kinds = {});

	/** The kind of each equipped gun, in slot order (None for an empty slot), for ForSlot. */
	AI_LOOTER_SHOOTER_API TArray<EWeaponKind> KindsOf(const TArray<AWeaponBase*>& Equipped);

	/** "In hand", "On back", "On hip", "Empty". */
	AI_LOOTER_SHOOTER_API const TCHAR* Label(ELoadoutCarry Carry);
}

/** One gun the stand-in carries: a copy of an equipped weapon's model. */
USTRUCT()
struct FLoadoutStageGun
{
	GENERATED_BODY()

	UPROPERTY()
	FWeaponInstanceData Instance;

	UPROPERTY()
	TObjectPtr<UWeaponModelComponent> Model;

	ELoadoutCarry Carry = ELoadoutCarry::None;
	/** Key points in the gun's own space. */
	FVector Grip = FVector::ZeroVector;
	FVector Foregrip = FVector::ZeroVector;
	FVector Center = FVector::ZeroVector;
};

/**
 * The loadout screen's old stand: a stand-in of the player's character (same mesh, materials and animation) carrying
 * copies of the equipped guns, in hand, on the back and at the hip. It stands far outside the level, is visible only to its
 * own camera and is lit only by its own studio lights, and renders into a picture only while active.
 * Since the 2026-10-08 redesign the loadout shows the chosen gun on its own (ALoadoutGunStage) and spawns this no more;
 * it's kept while the user decides whether the stand-in comes back (LoadoutCarry, below, is still used everywhere).
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ALoadoutStage : public AActor
{
	GENERATED_BODY()

public:
	ALoadoutStage();

	/** Copies the character's look and the guns it carries (only changed guns are rebuilt). Call whenever the inventory changes. */
	void ShowLoadout(const ACharacter* Character, const UWeaponManagerComponent* Weapons);

	/** Animates and renders every frame while active; costs nothing while inactive. */
	void SetActive(bool bInActive);

	/** Turns the stand-in on the spot (degrees, positive turns it to its left as seen from the camera). */
	void AddTurn(float Degrees);

	/** Back to the three-quarter view the screen opens with. */
	void ResetTurn();

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** Where a world point shows in the picture, as 0..1 across and down. False when it's behind the camera. */
	bool ProjectToImage(const FVector& WorldLocation, FVector2D& OutUV) const;

	/** The floor under the stand-in's feet. */
	FVector GetFloorCenter() const;

	/** Level direction from the stand-in toward the camera. */
	FVector GetTowardCamera() const;

	/** Brightness the loadout screen applies when it tone maps the picture. */
	float GetExposure() const { return Exposure; }

	/** The picture's size in pixels; the loadout screen shows it at this aspect ratio. */
	static constexpr int32 ImageWidth = 880;
	static constexpr int32 ImageHeight = 1280;

	// --- Framing (the camera looks at the stand-in from in front, a little from above) ---

	UPROPERTY(EditAnywhere, Category = "Stage|Camera")
	float CameraDistance = 520.f;

	UPROPERTY(EditAnywhere, Category = "Stage|Camera")
	float CameraHeight = 183.f;

	UPROPERTY(EditAnywhere, Category = "Stage|Camera")
	float CameraPitch = -10.f;

	/** Horizontal field of view (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage|Camera")
	float FieldOfView = 16.f;

	/** How far the stand-in is turned from facing the camera when the screen opens (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage|Camera")
	float DefaultTurn = 30.f;

	UPROPERTY(EditAnywhere, Category = "Stage|Camera")
	float Exposure = 1.f;

	// --- Where holstered guns sit, relative to a bone, in the stand-in's frame (X forward, Y right, Z up) ---

	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FName BackBone = TEXT("spine_05");

	/** Where the middle of a gun on the back sits, from the bone. */
	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FVector BackOffset = FVector(-20.f, 0.f, -8.f);

	/** Which way the muzzle of a gun on the back points. */
	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FVector BackDirection = FVector(0.f, 0.55f, 0.83f);

	/** Which way the top of a gun on the back faces. */
	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FVector BackUp = FVector(0.f, -0.83f, 0.55f);

	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FName HipBone = TEXT("thigh_r");

	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FVector HipOffset = FVector(0.f, 13.f, -16.f);

	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FVector HipDirection = FVector(0.25f, 0.f, -0.97f);

	UPROPERTY(EditAnywhere, Category = "Stage|Holsters")
	FVector HipUp = FVector(0.97f, 0.f, 0.25f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BuildGun(FLoadoutStageGun& Gun, const FWeaponInstanceData& Instance);
	void DestroyGun(FLoadoutStageGun& Gun);
	/** Gives the stand-in's animation the gun in hand to hold (or none). */
	void UpdateHold();
	void PlaceCamera();
	/** Puts every gun where it's carried, after the stand-in has animated this frame. */
	void PlaceGuns();
	FQuat GetFacing() const;
	const FLoadoutStageGun* GetGunInHand() const;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Root;

	/** Turns with the stand-in (drag to turn); the camera and lights stay put. */
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Turntable;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> FillLight;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UPointLightComponent> RimLight;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	/** One per equipped slot. */
	UPROPERTY(Transient)
	TArray<FLoadoutStageGun> Guns;

	FName HoldSocket = TEXT("HandGrip_R");
	float Turn = 0.f;
	bool bActive = false;
};
