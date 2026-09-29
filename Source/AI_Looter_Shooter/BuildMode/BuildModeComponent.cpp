#include "BuildMode/BuildModeComponent.h"
#include "BuildMode/BuildCameraPawn.h"
#include "BuildMode/BuildModeWidget.h"
#include "Environment/EnvironmentLayout.h"
#include "Environment/EnvironmentPalette.h"
#include "Environment/StylizedProp.h"
#include "World/MinimapSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

namespace
{
	const TCHAR* DefaultPalettePath = TEXT("/Game/Environment/DA_EnvironmentPalette.DA_EnvironmentPalette");
	constexpr float RotateStep = 15.f;
	constexpr float ScaleStep = 1.1f;
}

UBuildModeComponent::UBuildModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

APlayerController* UBuildModeComponent::GetPC() const
{
	return Cast<APlayerController>(GetOwner());
}

bool UBuildModeComponent::IsKeyPressed(const FKey& Key) const
{
	const APlayerController* PC = GetPC();
	return PC && PC->WasInputKeyJustPressed(Key);
}

bool UBuildModeComponent::IsKeyDown(const FKey& Key) const
{
	const APlayerController* PC = GetPC();
	return PC && PC->IsInputKeyDown(Key);
}

bool UBuildModeComponent::IsCtrlDown() const
{
	return IsKeyDown(EKeys::LeftControl) || IsKeyDown(EKeys::RightControl);
}

bool UBuildModeComponent::IsShiftDown() const
{
	return IsKeyDown(EKeys::LeftShift) || IsKeyDown(EKeys::RightShift);
}

void UBuildModeComponent::SetStatus(const FString& Message)
{
	Status = Message;
}

// ---------------------------------------------------------------------------
// Mode switching
// ---------------------------------------------------------------------------

void UBuildModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* PC = GetPC();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	// Toggling is driven by the rebindable Build Mode hotkey in ALooterHUD.
	if (!bBuildMode)
	{
		return;
	}

	UpdateCursorMode();
	UpdateCamera(DeltaTime);
	UpdateCursor();
	HandleHotkeys();
	HandleMouse(DeltaTime);
	UpdatePreview();
	DrawOverlays();

	AutosaveTimer += DeltaTime;
	if (AutosaveTimer >= AutosaveInterval)
	{
		SaveIfDirty(TEXT("Autosaved"));
	}
}

void UBuildModeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Stopping the session while building must not lose work: ExitBuildMode saves pending changes.
	if (bBuildMode)
	{
		ExitBuildMode();
	}
	Super::EndPlay(EndPlayReason);
}

void UBuildModeComponent::MarkDirty()
{
	bLayoutDirty = true;
}

void UBuildModeComponent::SaveIfDirty(const TCHAR* Reason)
{
	AutosaveTimer = 0.f;
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (!bLayoutDirty || !CurrentLayout)
	{
		return;
	}

	FString Message;
	if (CurrentLayout->SaveLayout(Message))
	{
		bLayoutDirty = false;
		SetStatus(FString::Printf(TEXT("%s: %s"), Reason, *Message));
	}
	else
	{
		SetStatus(FString::Printf(TEXT("Save failed: %s"), *Message));
	}
	UE_LOG(LogLooter, Log, TEXT("Build Mode %s: %s"), Reason, *Message);
}

// ---------------------------------------------------------------------------
// Mouse: look by default, hold Alt for a cursor
// ---------------------------------------------------------------------------

void UBuildModeComponent::UpdateCursorMode()
{
	const bool bWantCursor = IsKeyDown(EKeys::LeftAlt) || IsKeyDown(EKeys::RightAlt);
	if (bWantCursor != bCursorMode)
	{
		bCursorMode = bWantCursor;
		ApplyInputMode();
	}
}

void UBuildModeComponent::ApplyInputMode()
{
	APlayerController* PC = GetPC();
	if (!PC || !bBuildMode)
	{
		return;
	}

	if (bCursorMode)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		// Captured, hidden mouse: movement turns the camera, just like playing.
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

bool UBuildModeComponent::IsPointerOverUI() const
{
	return bCursorMode && Widget && Widget->IsPointerOverPanel();
}

void UBuildModeComponent::SetupBuildInput()
{
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return;
	}

	if (!LookAction)
	{
		LookAction = NewObject<UInputAction>(this, TEXT("IA_BuildLook"));
		LookAction->ValueType = EInputActionValueType::Axis2D;
		BuildContext = NewObject<UInputMappingContext>(this, TEXT("IMC_BuildMode"));
		BuildContext->MapKey(LookAction, EKeys::Mouse2D);
	}

	PendingLook = FVector2D::ZeroVector;
	BuildInput = NewObject<UEnhancedInputComponent>(PC, TEXT("BuildModeInput"));
	BuildInput->RegisterComponent();
	BuildInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &UBuildModeComponent::HandleLook);
	PC->PushInputComponent(BuildInput);

	if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Input->AddMappingContext(BuildContext, 50);
	}
}

void UBuildModeComponent::TeardownBuildInput()
{
	APlayerController* PC = GetPC();
	if (PC && BuildInput)
	{
		PC->PopInputComponent(BuildInput);
	}
	if (PC && BuildContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Input->RemoveMappingContext(BuildContext);
		}
	}
	if (BuildInput)
	{
		BuildInput->DestroyComponent();
		BuildInput = nullptr;
	}
	PendingLook = FVector2D::ZeroVector;
}

void UBuildModeComponent::HandleLook(const FInputActionValue& Value)
{
	PendingLook += Value.Get<FVector2D>();
}

void UBuildModeComponent::SetBuildMode(bool bEnable)
{
	if (bEnable == bBuildMode)
	{
		return;
	}
	bEnable ? EnterBuildMode() : ExitBuildMode();
}

AEnvironmentLayout* UBuildModeComponent::FindOrCreateLayout()
{
	UWorld* World = GetWorld();
	for (TActorIterator<AEnvironmentLayout> It(World); It; ++It)
	{
		return *It;
	}

	// Levels without a layout actor still get a working (but unsaveable) Build Mode.
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	AEnvironmentLayout* NewLayout = World->SpawnActor<AEnvironmentLayout>(AEnvironmentLayout::StaticClass(), FTransform::Identity, Params);
	if (NewLayout)
	{
		NewLayout->Palette = LoadObject<UEnvironmentPalette>(nullptr, DefaultPalettePath);
		UE_LOG(LogLooter, Warning, TEXT("Build Mode: no EnvironmentLayout actor in this level; placements won't be saved."));
	}
	return NewLayout;
}

void UBuildModeComponent::EnterBuildMode()
{
	APlayerController* PC = GetPC();
	UWorld* World = GetWorld();

	AEnvironmentLayout* FoundLayout = FindOrCreateLayout();
	Layout = FoundLayout;
	if (FoundLayout && !FoundLayout->Palette)
	{
		FoundLayout->Palette = LoadObject<UEnvironmentPalette>(nullptr, DefaultPalettePath);
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

	PlayerPawn = PC->GetPawn();
	SetPlayerWeaponsHidden(true);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CameraPawn = World->SpawnActor<ABuildCameraPawn>(ABuildCameraPawn::StaticClass(), ViewLocation, FRotator(ViewRotation.Pitch, ViewRotation.Yaw, 0.f), Params);
	PC->Possess(CameraPawn);
	PC->SetControlRotation(FRotator(ViewRotation.Pitch, ViewRotation.Yaw, 0.f));

	Widget = CreateWidget<UBuildModeWidget>(PC, UBuildModeWidget::StaticClass());
	if (Widget)
	{
		Widget->Init(this);
		// Above the HUD/inventory, below the pause menu (40) so the menu dims it.
		Widget->AddToViewport(30);
	}

	bBuildMode = true;
	bCursorMode = false;
	AutosaveTimer = 0.f;
	SetupBuildInput();
	ApplyInputMode();
	PlacementSeed = FMath::Rand();
	SetStatus(TEXT("Mouse looks around. Hold Alt for the cursor, or press 1-9 to pick an item."));
}

void UBuildModeComponent::ExitBuildMode()
{
	APlayerController* PC = GetPC();

	SaveIfDirty(TEXT("Saved on exit"));
	TeardownBuildInput();

	// The island may have changed: the minimap draws it again.
	if (UMinimapSubsystem* Minimap = GetWorld() ? GetWorld()->GetSubsystem<UMinimapSubsystem>() : nullptr)
	{
		Minimap->Invalidate();
	}
	bCursorMode = false;
	DestroyPreview();
	SelectObject(nullptr);
	SelectedEntry = INDEX_NONE;
	bGrabbing = false;

	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}

	if (PC)
	{
		if (APawn* Pawn = PlayerPawn.Get())
		{
			PC->Possess(Pawn);
		}
		SetPlayerWeaponsHidden(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	if (CameraPawn)
	{
		CameraPawn->Destroy();
		CameraPawn = nullptr;
	}

	bBuildMode = false;
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void UBuildModeComponent::UpdateCamera(float DeltaTime)
{
	APlayerController* PC = GetPC();
	if (!CameraPawn || !PC)
	{
		return;
	}

	FRotator Rotation = PC->GetControlRotation();

	// Mouse always looks, except while Alt frees the cursor (then right-drag still looks, like the editor).
	const FVector2D Look = PendingLook;
	PendingLook = FVector2D::ZeroVector;
	if ((!bCursorMode || IsKeyDown(EKeys::RightMouseButton)) && !Look.IsNearlyZero())
	{
		Rotation.Yaw += Look.X * LookSensitivity * 10.f;
		Rotation.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Rotation.Pitch + Look.Y * LookSensitivity * 10.f), -89.f, 89.f);
		Rotation.Roll = 0.f;
		PC->SetControlRotation(Rotation);
	}

	if (IsCtrlDown())
	{
		return; // Ctrl is for shortcuts (Ctrl+S etc.), not movement.
	}

	FVector Input = FVector::ZeroVector;
	if (IsKeyDown(EKeys::W)) Input.X += 1.f;
	if (IsKeyDown(EKeys::S)) Input.X -= 1.f;
	if (IsKeyDown(EKeys::D)) Input.Y += 1.f;
	if (IsKeyDown(EKeys::A)) Input.Y -= 1.f;
	if (IsKeyDown(EKeys::E)) Input.Z += 1.f;
	if (IsKeyDown(EKeys::Q)) Input.Z -= 1.f;

	if (!Input.IsNearlyZero())
	{
		const FRotationMatrix Matrix(Rotation);
		const FVector Direction = Matrix.GetUnitAxis(EAxis::X) * Input.X + Matrix.GetUnitAxis(EAxis::Y) * Input.Y + FVector::UpVector * Input.Z;
		const float Speed = FlySpeed * (IsShiftDown() ? FastMultiplier : 1.f);
		CameraPawn->AddActorWorldOffset(Direction.GetSafeNormal() * Speed * DeltaTime);
	}
}

void UBuildModeComponent::UpdateCursor()
{
	bCursorValid = false;
	CursorActor.Reset();

	APlayerController* PC = GetPC();
	if (!PC || IsPointerOverUI())
	{
		return;
	}

	// Aim with the crosshair normally; with the cursor out, aim where the cursor points.
	FVector Origin;
	FVector Direction;
	if (bCursorMode)
	{
		if (!PC->DeprojectMousePositionToWorld(Origin, Direction))
		{
			return;
		}
	}
	else
	{
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(Origin, ViewRotation);
		Direction = ViewRotation.Vector();
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildModeCursor), /*bTraceComplex*/ true);
	Params.AddIgnoredActor(CameraPawn);
	Params.AddIgnoredActor(Preview);
	if (bGrabbing)
	{
		Params.AddIgnoredActor(SelectedObject);
	}

	// Soft props (grass, flowers, bushes, clouds) only overlap Visibility: placement goes through them to the
	// ground underneath, but they are still reported so they can be clicked and selected.
	TArray<FHitResult> Hits;
	GetWorld()->LineTraceMultiByChannel(Hits, Origin, Origin + Direction * 500000.f, ECC_Visibility, Params);
	const FHitResult* Blocking = Hits.Num() > 0 && Hits.Last().bBlockingHit ? &Hits.Last() : nullptr;
	if (Blocking)
	{
		bCursorValid = true;
		CursorLocation = Blocking->ImpactPoint;
		CursorNormal = Blocking->ImpactNormal;
	}
	if (Hits.Num() > 0)
	{
		CursorActor = Hits[0].GetActor();
	}

	// Sky pieces (islands, clouds) go in mid-air in front of the camera unless a surface is closer.
	const FEnvironmentPaletteEntry* AirEntry = GetSelectedPaletteEntry();
	FPlacedObjectRecord GrabbedRecord;
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (bGrabbing && CurrentLayout && CurrentLayout->GetRecord(SelectedObject, GrabbedRecord))
	{
		AirEntry = CurrentLayout->FindEntry(GrabbedRecord.EntryId);
	}
	if (AirEntry && AirEntry->bPlaceInAir && (!Blocking || Blocking->Distance > AirEntry->AirDistance))
	{
		bCursorValid = true;
		CursorLocation = Origin + Direction * AirEntry->AirDistance;
		CursorNormal = FVector::UpVector;
	}
}

void UBuildModeComponent::HandleHotkeys()
{
	const bool bCtrl = IsCtrlDown();

	if (bCtrl && IsKeyPressed(EKeys::S)) { SaveLayout(); return; }
	if (bCtrl && IsKeyPressed(EKeys::Z)) { Undo(); return; }
	if (bCtrl && IsKeyPressed(EKeys::D)) { DuplicateSelected(); return; }

	if (IsKeyPressed(EKeys::X))
	{
		// Drop whatever tool/selection is active.
		if (bGrabbing) { bGrabbing = false; }
		else if (SelectedEntry != INDEX_NONE) { SelectEntry(SelectedEntry); }
		else { SelectObject(nullptr); }
	}

	if (IsKeyPressed(EKeys::Delete) || IsKeyPressed(EKeys::BackSpace)) { DeleteSelected(); }
	if (IsKeyPressed(EKeys::G) && SelectedObject) { bGrabbing = !bGrabbing; }
	if (IsKeyPressed(EKeys::B)) { ToggleBrush(); }

	if (IsKeyPressed(EKeys::R)) { RotateTarget(IsShiftDown() ? -RotateStep : RotateStep); }
	if (IsKeyPressed(EKeys::Equals) || IsKeyPressed(EKeys::Add)) { ScaleTarget(ScaleStep); }
	if (IsKeyPressed(EKeys::Hyphen) || IsKeyPressed(EKeys::Subtract)) { ScaleTarget(1.f / ScaleStep); }

	if (IsKeyPressed(EKeys::LeftBracket)) { BrushRadius = FMath::Clamp(BrushRadius * 0.8f, 200.f, 20000.f); }
	if (IsKeyPressed(EKeys::RightBracket)) { BrushRadius = FMath::Clamp(BrushRadius * 1.25f, 200.f, 20000.f); }

	// 1-9 pick from the palette tab that's showing, so building never needs the cursor.
	static const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	if (Widget)
	{
		const TArray<int32> Visible = Widget->GetVisibleEntries();
		for (int32 Number = 0; Number < UE_ARRAY_COUNT(NumberKeys); ++Number)
		{
			if (IsKeyPressed(NumberKeys[Number]) && Visible.IsValidIndex(Number))
			{
				SelectEntry(Visible[Number]);
			}
		}
	}

	// Mouse wheel: fly speed while looking, scale with Shift, otherwise rotate.
	const int32 Wheel = (IsKeyPressed(EKeys::MouseScrollUp) ? 1 : 0) - (IsKeyPressed(EKeys::MouseScrollDown) ? 1 : 0);
	if (Wheel != 0 && !IsPointerOverUI())
	{
		if (IsKeyDown(EKeys::RightMouseButton))
		{
			FlySpeed = FMath::Clamp(FlySpeed * (Wheel > 0 ? 1.25f : 0.8f), 100.f, 50000.f);
			SetStatus(FString::Printf(TEXT("Fly speed %.0f"), FlySpeed));
		}
		else if (IsShiftDown())
		{
			ScaleTarget(Wheel > 0 ? ScaleStep : 1.f / ScaleStep);
		}
		else
		{
			RotateTarget(Wheel * RotateStep);
		}
	}
}

void UBuildModeComponent::HandleMouse(float DeltaTime)
{
	const bool bOverPanel = IsPointerOverUI();

	if (IsKeyPressed(EKeys::LeftMouseButton) && !bOverPanel)
	{
		bConfirmClear = false;
		StrokePoints.Reset();
		BrushCooldown = 0.f;

		if (bGrabbing)
		{
			bGrabbing = false;
			RefreshPlacement(SelectedObject);
			MarkDirty();
			SetStatus(TEXT("Moved."));
		}
		else if (SelectedEntry != INDEX_NONE && !bBrush)
		{
			if (bCursorValid)
			{
				PlaceAt(CursorLocation, CursorNormal, PlacementYaw, PlacementScale, PlacementSeed);
				RerollPlacement();
			}
		}
		else if (SelectedEntry == INDEX_NONE)
		{
			AActor* Hit = CursorActor.Get();
			AEnvironmentLayout* CurrentLayout = Layout.Get();
			SelectObject(CurrentLayout && CurrentLayout->IsLayoutObject(Hit) ? Hit : nullptr);
		}
	}

	// Scatter brush: keep painting (or erasing, with Shift) while the button is held.
	if (bBrush && SelectedEntry != INDEX_NONE && IsKeyDown(EKeys::LeftMouseButton) && !bOverPanel && bCursorValid)
	{
		BrushCooldown -= DeltaTime;
		if (BrushCooldown <= 0.f)
		{
			if (IsShiftDown())
			{
				EraseBrush();
			}
			else
			{
				PaintBrush();
			}
			BrushCooldown = 0.05f;
		}
	}
	if (!IsKeyDown(EKeys::LeftMouseButton))
	{
		FinishStroke();
	}

	if (bGrabbing && SelectedObject && bCursorValid)
	{
		SelectedObject->SetActorLocation(CursorLocation);
	}
}

void UBuildModeComponent::UpdatePreview()
{
	const bool bShow = SelectedEntry != INDEX_NONE && !bBrush && !bGrabbing && bCursorValid;
	if (!bShow)
	{
		if (Preview)
		{
			Preview->SetActorHiddenInGame(true);
		}
		return;
	}

	const FEnvironmentPaletteEntry* Entry = GetSelectedPaletteEntry();
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (!Entry || !CurrentLayout)
	{
		return;
	}

	const FTransform PlacementTransform = MakePlacementTransform(CursorLocation, CursorNormal, PlacementYaw, PlacementScale, bAlignToSurface || Entry->bAlignToSurface);
	if (!Preview || PreviewEntry != SelectedEntry)
	{
		DestroyPreview();
		FPlacedObjectRecord Record;
		Record.EntryId = Entry->Id;
		Record.Transform = PlacementTransform;
		Record.Seed = PlacementSeed;
		Preview = CurrentLayout->SpawnPreview(Record);
		PreviewEntry = SelectedEntry;
	}

	if (Preview)
	{
		Preview->SetActorHiddenInGame(false);
		Preview->SetActorTransform(PlacementTransform);
	}
}

void UBuildModeComponent::DrawOverlays() const
{
	UWorld* World = GetWorld();

	if (SelectedObject)
	{
		FVector Origin;
		FVector Extent;
		SelectedObject->GetActorBounds(false, Origin, Extent);
		DrawDebugBox(World, Origin, Extent, bGrabbing ? FColor::Cyan : FColor::Yellow, false, -1.f, 0, 4.f);
	}

	if (bBrush && SelectedEntry != INDEX_NONE && bCursorValid)
	{
		// Red while Shift is held: the brush erases instead of painting.
		DrawDebugCircle(World, CursorLocation + FVector(0.f, 0.f, 15.f), BrushRadius, 48, IsShiftDown() ? FColor::Red : FColor::Orange, false, -1.f, 0, 6.f,
			FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}
}

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------

const UEnvironmentPalette* UBuildModeComponent::GetPalette() const
{
	const AEnvironmentLayout* CurrentLayout = Layout.Get();
	return CurrentLayout ? CurrentLayout->Palette : nullptr;
}

const FEnvironmentPaletteEntry* UBuildModeComponent::GetSelectedPaletteEntry() const
{
	const UEnvironmentPalette* Palette = GetPalette();
	return Palette && Palette->Entries.IsValidIndex(SelectedEntry) ? &Palette->Entries[SelectedEntry] : nullptr;
}

void UBuildModeComponent::SelectEntry(int32 EntryIndex)
{
	bConfirmClear = false;
	SelectObject(nullptr);
	DestroyPreview();

	// Clicking the active entry again puts the tool away.
	SelectedEntry = (EntryIndex == SelectedEntry) ? INDEX_NONE : EntryIndex;

	if (const FEnvironmentPaletteEntry* Entry = GetSelectedPaletteEntry())
	{
		RerollPlacement();
		SetStatus(FString::Printf(TEXT("Placing %s: click to place, R/wheel rotate, +/- scale, B brush, X stop."), *Entry->DisplayName.ToString()));
	}
	else
	{
		SetStatus(TEXT("Click an object to select it."));
	}
}

FTransform UBuildModeComponent::MakePlacementTransform(const FVector& Location, const FVector& Normal, float Yaw, float Scale, bool bAlign) const
{
	const FQuat Tilt = bAlign ? FQuat::FindBetweenNormals(FVector::UpVector, Normal.GetSafeNormal()) : FQuat::Identity;
	const FQuat Spin(FVector::UpVector, FMath::DegreesToRadians(Yaw));
	return FTransform(Tilt * Spin, Location, FVector(Scale));
}

AActor* UBuildModeComponent::PlaceAt(const FVector& Location, const FVector& Normal, float Yaw, float Scale, int32 Seed, bool bRecordUndo)
{
	const FEnvironmentPaletteEntry* Entry = GetSelectedPaletteEntry();
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (!Entry || !CurrentLayout)
	{
		return nullptr;
	}

	FPlacedObjectRecord Record;
	Record.EntryId = Entry->Id;
	Record.Seed = Seed;
	Record.Transform = MakePlacementTransform(Location, Normal, Yaw, Scale, bAlignToSurface || Entry->bAlignToSurface);

	AActor* Placed = CurrentLayout->PlaceObject(Record);
	if (Placed)
	{
		if (bRecordUndo)
		{
			FUndoStep Step;
			Step.Placed.Add(Placed);
			UndoStack.Add(MoveTemp(Step));
		}
		MarkDirty();
		SetStatus(FString::Printf(TEXT("Placed %s (%d objects, unsaved)"), *Entry->DisplayName.ToString(), CurrentLayout->GetObjectCount()));
	}
	return Placed;
}

void UBuildModeComponent::PaintBrush()
{
	const FEnvironmentPaletteEntry* Entry = GetSelectedPaletteEntry();
	if (!Entry)
	{
		return;
	}

	// Pick a random point in the brush circle and drop it onto the ground.
	const FVector2D Offset = FMath::RandPointInCircle(BrushRadius);
	const FVector Top = CursorLocation + FVector(Offset.X, Offset.Y, 5000.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BuildModeBrush), true);
	Params.AddIgnoredActor(CameraPawn);
	Params.AddIgnoredActor(Preview);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Top, Top - FVector(0.f, 0.f, 10000.f), ECC_Visibility, Params))
	{
		return;
	}

	const float Spacing = Entry->Footprint * 0.9f;
	for (const FVector& Point : StrokePoints)
	{
		if (FVector::DistSquared2D(Point, Hit.ImpactPoint) < Spacing * Spacing)
		{
			return;
		}
	}

	const float Scale = FMath::FRandRange(Entry->MinScale, Entry->MaxScale);
	if (AActor* Placed = PlaceAt(Hit.ImpactPoint, Hit.ImpactNormal, FMath::FRandRange(0.f, 360.f), Scale, FMath::Rand(), /*bRecordUndo*/ false))
	{
		StrokePoints.Add(Hit.ImpactPoint);
		StrokeStep.Placed.Add(Placed);
	}
}

void UBuildModeComponent::RefreshPlacement(AActor* Object)
{
	if (AStylizedProp* Prop = Cast<AStylizedProp>(Object))
	{
		Prop->RefreshAfterMove();
	}
}

void UBuildModeComponent::EraseBrush()
{
	const FEnvironmentPaletteEntry* Entry = GetSelectedPaletteEntry();
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (!Entry || !CurrentLayout)
	{
		return;
	}

	// Only the selected item is erased, so painting grass back out never takes rocks or trees with it.
	TArray<AActor*> InBrush;
	CurrentLayout->FindObjects(Entry->Id, CursorLocation, BrushRadius, InBrush);
	for (AActor* Actor : InBrush)
	{
		if (Actor == SelectedObject)
		{
			SelectObject(nullptr);
		}
		FPlacedObjectRecord Removed;
		const TWeakObjectPtr<AActor> Victim(Actor);
		if (CurrentLayout->RemoveObject(Actor, Removed))
		{
			RecordRemoval(StrokeStep, Victim, Removed);
		}
	}
	if (InBrush.Num() > 0)
	{
		MarkDirty();
		SetStatus(FString::Printf(TEXT("Erased %s (%d objects, unsaved)"), *Entry->DisplayName.ToString(), CurrentLayout->GetObjectCount()));
	}
}

void UBuildModeComponent::RecordRemoval(FUndoStep& Step, const TWeakObjectPtr<AActor>& Actor, const FPlacedObjectRecord& Record)
{
	Step.Removed.Add(Record);
	Step.RemovedActors.Add(Actor);
}

void UBuildModeComponent::SetPlayerWeaponsHidden(bool bHidden)
{
	if (bHidden)
	{
		HiddenPlayerActors.Reset();
		if (APawn* Pawn = PlayerPawn.Get())
		{
			// The character too: its first-person arms only make sense from its own eyes.
			TArray<AActor*> Attached;
			Pawn->GetAttachedActors(Attached, true, true);
			Attached.Insert(Pawn, 0);
			for (AActor* Actor : Attached)
			{
				// Only hide what is showing now, so holstered weapons stay hidden when we restore.
				if (!Actor->IsHidden())
				{
					Actor->SetActorHiddenInGame(true);
					HiddenPlayerActors.Add(Actor);
				}
			}
		}
	}
	else
	{
		for (const TWeakObjectPtr<AActor>& Actor : HiddenPlayerActors)
		{
			if (Actor.IsValid())
			{
				Actor->SetActorHiddenInGame(false);
			}
		}
		HiddenPlayerActors.Reset();
	}
}

void UBuildModeComponent::FinishStroke()
{
	if (!StrokeStep.IsEmpty())
	{
		UndoStack.Add(MoveTemp(StrokeStep));
		StrokeStep = FUndoStep();
	}
}

bool UBuildModeComponent::IsActiveInWorld(const UWorld* World)
{
	if (!World)
	{
		return false;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const UBuildModeComponent* BuildMode = PC ? PC->FindComponentByClass<UBuildModeComponent>() : nullptr;
		if (BuildMode && BuildMode->IsBuildMode())
		{
			return true;
		}
	}
	return false;
}

void UBuildModeComponent::RerollPlacement()
{
	const FEnvironmentPaletteEntry* Entry = GetSelectedPaletteEntry();
	PlacementSeed = FMath::Rand();
	if (bRandomRotation)
	{
		PlacementYaw = FMath::FRandRange(0.f, 360.f);
	}
	if (bRandomScale && Entry)
	{
		PlacementScale = FMath::FRandRange(Entry->MinScale, Entry->MaxScale);
	}
	// New seed means a new mesh variation, so rebuild the preview.
	DestroyPreview();
}

void UBuildModeComponent::DestroyPreview()
{
	if (Preview)
	{
		Preview->Destroy();
		Preview = nullptr;
	}
	PreviewEntry = INDEX_NONE;
}

// ---------------------------------------------------------------------------
// Editing placed objects
// ---------------------------------------------------------------------------

void UBuildModeComponent::SelectObject(AActor* Object)
{
	SelectedObject = Object;
	bGrabbing = false;

	FPlacedObjectRecord Record;
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (Object && CurrentLayout && CurrentLayout->GetRecord(Object, Record))
	{
		SetStatus(FString::Printf(TEXT("Selected %s: G move, R/wheel rotate, +/- scale, Del delete, Ctrl+D duplicate."), *Record.EntryId.ToString()));
	}
}

void UBuildModeComponent::DeleteSelected()
{
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	FPlacedObjectRecord Removed;
	const TWeakObjectPtr<AActor> Victim(SelectedObject);
	if (SelectedObject && CurrentLayout && CurrentLayout->RemoveObject(SelectedObject, Removed))
	{
		FUndoStep Step;
		RecordRemoval(Step, Victim, Removed);
		UndoStack.Add(MoveTemp(Step));
		MarkDirty();
		SelectedObject = nullptr;
		bGrabbing = false;
		SetStatus(FString::Printf(TEXT("Deleted (%d objects, unsaved)"), CurrentLayout->GetObjectCount()));
	}
}

void UBuildModeComponent::DuplicateSelected()
{
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	FPlacedObjectRecord Record;
	if (!SelectedObject || !CurrentLayout || !CurrentLayout->GetRecord(SelectedObject, Record))
	{
		return;
	}

	if (AActor* Copy = CurrentLayout->PlaceObject(Record))
	{
		FUndoStep Step;
		Step.Placed.Add(Copy);
		UndoStack.Add(MoveTemp(Step));
		MarkDirty();
		SelectObject(Copy);
		bGrabbing = true; // Copy follows the cursor until you click.
		SetStatus(TEXT("Duplicated: click to drop it."));
	}
}

void UBuildModeComponent::RotateTarget(float Degrees)
{
	if (SelectedObject && SelectedEntry == INDEX_NONE)
	{
		SelectedObject->AddActorWorldRotation(FRotator(0.f, Degrees, 0.f));
		RefreshPlacement(SelectedObject);
		MarkDirty();
	}
	else
	{
		PlacementYaw = FMath::Fmod(PlacementYaw + Degrees + 360.f, 360.f);
	}
}

void UBuildModeComponent::ScaleTarget(float Factor)
{
	if (SelectedObject && SelectedEntry == INDEX_NONE)
	{
		SelectedObject->SetActorScale3D((SelectedObject->GetActorScale3D() * Factor).BoundToCube(100.f));
		RefreshPlacement(SelectedObject);
		MarkDirty();
	}
	else
	{
		PlacementScale = FMath::Clamp(PlacementScale * Factor, 0.05f, 100.f);
	}
}

void UBuildModeComponent::Undo()
{
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (!CurrentLayout || UndoStack.Num() == 0)
	{
		SetStatus(TEXT("Nothing to undo."));
		return;
	}

	const FUndoStep Step = UndoStack.Pop();
	for (const TWeakObjectPtr<AActor>& WeakActor : Step.Placed)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			if (Actor == SelectedObject)
			{
				SelectObject(nullptr);
			}
			FPlacedObjectRecord Ignored;
			CurrentLayout->RemoveObject(Actor, Ignored);
		}
	}
	for (int32 Index = 0; Index < Step.Removed.Num(); ++Index)
	{
		AActor* Restored = CurrentLayout->PlaceObject(Step.Removed[Index]);
		// Older steps that placed the removed object must now point at its restored copy, or undoing them later
		// would silently do nothing.
		if (Restored && Step.RemovedActors.IsValidIndex(Index))
		{
			const TWeakObjectPtr<AActor>& Original = Step.RemovedActors[Index];
			for (FUndoStep& Earlier : UndoStack)
			{
				for (TWeakObjectPtr<AActor>& Placed : Earlier.Placed)
				{
					if (Placed.HasSameIndexAndSerialNumber(Original))
					{
						Placed = Restored;
					}
				}
			}
		}
	}
	MarkDirty();
	SetStatus(FString::Printf(TEXT("Undone (%d objects, unsaved)"), CurrentLayout->GetObjectCount()));
}

void UBuildModeComponent::SaveLayout()
{
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (!CurrentLayout)
	{
		SetStatus(TEXT("No layout to save."));
		return;
	}
	FString Message;
	if (CurrentLayout->SaveLayout(Message))
	{
		bLayoutDirty = false;
		AutosaveTimer = 0.f;
	}
	SetStatus(Message);
}

void UBuildModeComponent::ClearAll()
{
	AEnvironmentLayout* CurrentLayout = Layout.Get();
	if (!CurrentLayout)
	{
		return;
	}

	if (!bConfirmClear)
	{
		bConfirmClear = true;
		SetStatus(FString::Printf(TEXT("Click Clear All again to remove all %d objects."), CurrentLayout->GetObjectCount()));
		return;
	}

	bConfirmClear = false;
	SelectObject(nullptr);
	CurrentLayout->ClearAll();
	MarkDirty();
	UndoStack.Reset();
	StrokeStep = FUndoStep();
	SetStatus(TEXT("Cleared (unsaved; save to make it stick)."));
}

int32 UBuildModeComponent::GetObjectCount() const
{
	const AEnvironmentLayout* CurrentLayout = Layout.Get();
	return CurrentLayout ? CurrentLayout->GetObjectCount() : 0;
}
