#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BestiaryStage.generated.h"

class UBestiaryEntry;
class UPointLightComponent;
class USceneCaptureComponent2D;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 * The bestiary's stand: the chosen entry's model on a turntable, framed to fit whatever its size, under the same studio
 * lights as the loadout's stand-in (StageStudio). It stands far outside the level, is seen only by its own camera, and
 * renders into the picture the bestiary shows, only while the bestiary is open.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ABestiaryStage : public AActor
{
	GENERATED_BODY()

public:
	ABestiaryStage();

	/** Puts the entry's model on the stand, turned to the opening view (nothing when it has no model). */
	void ShowEntry(const UBestiaryEntry* Entry);

	/** Animates and renders every frame while active; costs nothing while inactive. */
	void SetActive(bool bInActive);

	/** Turns the model on the spot (degrees, positive turns it to its left as seen from the camera). */
	void AddTurn(float Degrees);

	UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget; }

	/** Where a world point shows in the picture, as 0..1 across and down. False when it's behind the camera. */
	bool ProjectToImage(const FVector& WorldLocation, FVector2D& OutUV) const;

	/** The floor under the model. */
	FVector GetFloorCenter() const;

	/** Level direction from the model toward the camera. */
	FVector GetTowardCamera() const;

	/** The ring the model stands on: a little wider than its footprint (cm). */
	float GetRingRadius() const { return RingRadius; }

	/** A model is on the stand. */
	bool HasModel() const;

	float GetExposure() const { return Exposure; }

	/** The picture's size in pixels; the bestiary shows it at this aspect ratio. */
	static constexpr int32 ImageWidth = 960;
	static constexpr int32 ImageHeight = 880;

	/** Horizontal field of view (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float FieldOfView = 24.f;

	/** How far the camera looks down on the model (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float CameraPitch = -14.f;

	/** How much of the picture the model fills, across or up and down, whichever it fills first. */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float Fill = 0.85f;

	/** How far the model is turned from facing the camera when it's put on the stand (degrees). */
	UPROPERTY(EditAnywhere, Category = "Stage")
	float DefaultTurn = 35.f;

	UPROPERTY(EditAnywhere, Category = "Stage")
	float Exposure = 1.f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Places the camera and lights for the model's size, and turns the turntable. */
	void PlaceCamera();

	/** Puts on what the entry's actor wears on its bones besides its body (UBestiaryEntry::GetPreviewParts), and grows
	 * the model's box (its own space, at rest) by each part. */
	void ShowParts(const UBestiaryEntry* Entry, const USkeletalMesh* Mesh, FBox& InOutBox);

	/** Puts the entry's still parts (UBestiaryEntry::PreviewStillParts) on the still model's sockets, and grows the box
	 * (the still model's own space) by each part. */
	void ShowStillParts(const UBestiaryEntry* Entry, const UStaticMesh* Still, FBox& InOutBox);

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USceneComponent> Turntable;

	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<USkeletalMeshComponent> Model;

	/** A still model (UBestiaryEntry::PreviewStaticMesh), shown instead when the entry has no skeletal one. */
	UPROPERTY(VisibleAnywhere, Category = "Stage")
	TObjectPtr<UStaticMeshComponent> StillModel;

	/** What stands on the still model's sockets (Sexton on his rail, his ledger), one component per part, kept. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> StillParts;

	/** What the model wears on its bones (a hat), one component per part, kept for the next entry. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

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

	/** Half the model's size: how far it reaches from its middle across the floor, and up from the floor to its middle. */
	float SubjectReach = 100.f;
	float SubjectHeight = 100.f;
	float RingRadius = 60.f;
	float Turn = 0.f;
	/** Seconds left of rendering every frame after a change (a still model isn't rendered again otherwise). */
	float SettleTime = 0.f;
	bool bActive = false;
};
