#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Environment/LevelLayoutData.h"
#include "InputCoreTypes.h"
#include "BuildModeComponent.generated.h"

class ABuildCameraPawn;
class AEnvironmentLayout;
class APawn;
class APlayerController;
class UBuildModeWidget;
class UEnhancedInputComponent;
class UEnvironmentPalette;
class UInputAction;
class UInputMappingContext;
struct FEnvironmentPaletteEntry;
struct FInputActionValue;

/**
 * In-game environment editor on the PlayerController. The Build Mode hotkey (F1 by default, rebindable
 * in the settings menu) switches between Build Mode (free camera + palette) and Play Mode.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UBuildModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuildModeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Build Mode")
	void SetBuildMode(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "Build Mode")
	bool IsBuildMode() const { return bBuildMode; }

	/** True while any local player is in Build Mode (creatures freeze, etc.). */
	static bool IsActiveInWorld(const UWorld* World);

	/** True while Alt is held: the cursor is free for the palette and the camera stops following the mouse. */
	UFUNCTION(BlueprintPure, Category = "Build Mode")
	bool IsCursorMode() const { return bCursorMode; }

	/** Re-applies Build Mode's mouse/cursor setup (e.g. after a menu closes). */
	void ApplyInputMode();

	/** Seconds between automatic saves while there are unsaved changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode")
	float AutosaveInterval = 30.f;

	/** cm/s. Hold Shift for FastMultiplier, scroll while holding right mouse to change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode|Camera")
	float FlySpeed = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode|Camera")
	float FastMultiplier = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Mode|Camera")
	float LookSensitivity = 0.25f;

	// --- Called by the palette widget ---
	const UEnvironmentPalette* GetPalette() const;
	void SelectEntry(int32 EntryIndex);
	int32 GetSelectedEntry() const { return SelectedEntry; }
	void ToggleRandomRotation() { bRandomRotation = !bRandomRotation; }
	void ToggleRandomScale() { bRandomScale = !bRandomScale; }
	void ToggleAlignOverride() { bAlignToSurface = !bAlignToSurface; }
	void ToggleBrush() { bBrush = !bBrush; }
	bool IsRandomRotation() const { return bRandomRotation; }
	bool IsRandomScale() const { return bRandomScale; }
	bool IsAlignToSurface() const { return bAlignToSurface; }
	bool IsBrush() const { return bBrush; }
	float GetBrushRadius() const { return BrushRadius; }
	void SaveLayout();
	bool HasUnsavedChanges() const { return bLayoutDirty; }
	void Undo();
	void ClearAll();
	int32 GetObjectCount() const;
	const FString& GetStatus() const { return Status; }

private:
	APlayerController* GetPC() const;
	void EnterBuildMode();
	void ExitBuildMode();
	AEnvironmentLayout* FindOrCreateLayout();

	void UpdateCamera(float DeltaTime);
	void UpdateCursor();
	void HandleHotkeys();
	void HandleMouse(float DeltaTime);
	void UpdatePreview();
	void DrawOverlays() const;

	const FEnvironmentPaletteEntry* GetSelectedPaletteEntry() const;
	FTransform MakePlacementTransform(const FVector& Location, const FVector& Normal, float Yaw, float Scale, bool bAlign) const;
	AActor* PlaceAt(const FVector& Location, const FVector& Normal, float Yaw, float Scale, int32 Seed, bool bRecordUndo = true);
	void PaintBrush();
	void EraseBrush();
	void RefreshPlacement(AActor* Object);
	void SetPlayerWeaponsHidden(bool bHidden);
	void FinishStroke();
	void RerollPlacement();
	void DestroyPreview();
	void SelectObject(AActor* Object);
	void DeleteSelected();
	void DuplicateSelected();
	void RotateTarget(float Degrees);
	void ScaleTarget(float Factor);
	void SetStatus(const FString& Message);
	void MarkDirty();
	void SaveIfDirty(const TCHAR* Reason);
	void SetupBuildInput();
	void TeardownBuildInput();
	void HandleLook(const FInputActionValue& Value);
	void UpdateCursorMode();
	bool IsPointerOverUI() const;

	bool IsKeyPressed(const FKey& Key) const;
	bool IsKeyDown(const FKey& Key) const;
	bool IsCtrlDown() const;
	bool IsShiftDown() const;

	/** One undoable action. A whole brush stroke is one step, however much it painted or erased. */
	struct FUndoStep
	{
		TArray<TWeakObjectPtr<AActor>> Placed;
		TArray<FPlacedObjectRecord> Removed;
		/** The actors the Removed records came from, so undoing a removal can re-link older steps to the restored actors. */
		TArray<TWeakObjectPtr<AActor>> RemovedActors;
		bool IsEmpty() const { return Placed.Num() == 0 && Removed.Num() == 0; }
	};

	/** The stroke being painted/erased while the mouse button is held. */
	FUndoStep StrokeStep;

	void RecordRemoval(FUndoStep& Step, const TWeakObjectPtr<AActor>& Actor, const FPlacedObjectRecord& Record);

	UPROPERTY(Transient)
	TObjectPtr<ABuildCameraPawn> CameraPawn;

	// Mouse look goes through Enhanced Input (Mouse2D), same as the player's own look.
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> BuildContext;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> BuildInput;

	/** Mouse movement received since the last camera update. */
	FVector2D PendingLook = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UBuildModeWidget> Widget;

	UPROPERTY(Transient)
	TObjectPtr<AActor> Preview;

	UPROPERTY(Transient)
	TObjectPtr<AActor> SelectedObject;

	TWeakObjectPtr<APawn> PlayerPawn;

	/** The player's held weapon(s), hidden while building so they don't float in front of the free camera. */
	TArray<TWeakObjectPtr<AActor>> HiddenPlayerActors;
	TWeakObjectPtr<AEnvironmentLayout> Layout;

	// Visible so tests/tools can read the state.
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Build Mode|State")
	bool bBuildMode = false;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Build Mode|State")
	bool bCursorMode = false;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Build Mode|State")
	bool bLayoutDirty = false;

	float AutosaveTimer = 0.f;
	bool bRandomRotation = true;
	bool bRandomScale = true;
	bool bAlignToSurface = false;
	bool bBrush = false;
	bool bGrabbing = false;
	bool bConfirmClear = false;

	int32 SelectedEntry = INDEX_NONE;
	int32 PreviewEntry = INDEX_NONE;
	int32 PlacementSeed = 1;
	float PlacementYaw = 0.f;
	float PlacementScale = 1.f;
	float BrushRadius = 1000.f;
	float BrushCooldown = 0.f;

	bool bCursorValid = false;
	FVector CursorLocation = FVector::ZeroVector;
	FVector CursorNormal = FVector::UpVector;
	TWeakObjectPtr<AActor> CursorActor;

	/** Points painted during the current brush stroke, to keep spacing even. */
	TArray<FVector> StrokePoints;

	TArray<FUndoStep> UndoStack;
	FString Status;
};
