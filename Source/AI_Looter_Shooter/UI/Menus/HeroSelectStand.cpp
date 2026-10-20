// UHeroSelectWidget: the live stand in the screen's middle (AHeroSelectStage). Spawning it the first time the screen
// opens, its picture (the loadout's cut-out material on the stand's capture), the chosen hero on it or their painting in
// its place, turning it with a drag or the right stick, a silhouette for a hero in the works, and stopping it as the
// screen goes.

#include "UI/Menus/HeroSelectWidget.h"
#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Menus/HeroSelectParts.h"
#include "UI/Menus/HeroSelectStage.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

using namespace LooterUI;
using namespace HeroSelectParts;

namespace
{
	/** A drag's turn (degrees a pixel) and the stick's (degrees a second at full tilt): slower than the bestiary's, for a figure. */
	constexpr float DragTurnRate = 0.45f;
	constexpr float StickTurnRate = 120.f;
	constexpr float StickDeadZone = 0.2f;
	/** A hero in the works stands dark, the picture's brightness all but off (the bestiary's unmet creatures). */
	constexpr float SilhouetteExposure = 0.03f;
}

AHeroSelectStage* UHeroSelectWidget::GetStand() const
{
	return Stand.Get();
}

UWidget* UHeroSelectWidget::MakeStand()
{
	// The picture takes the drag (the grab hand over it); the stamp over its middle for a hero in the works.
	UOverlay* Holder = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	StandImage = MakeImage(WidgetTree, RectBrush(FLinearColor::Transparent));
	StandImage->SetVisibility(ESlateVisibility::Collapsed);
	StandImage->SetCursor(EMouseCursor::GrabHand);
	StandImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FillOverlaySlot(Holder->AddChildToOverlay(StandImage));
	StandStamp = MakeWorksStamp(WidgetTree, 18);
	StandStamp->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* StampSlot = Holder->AddChildToOverlay(StandStamp);
	StampSlot->SetHorizontalAlignment(HAlign_Center);
	StampSlot->SetVerticalAlignment(VAlign_Center);
	return MakeSized(WidgetTree, Holder, StandSize.X, StandSize.Y);
}

void UHeroSelectWidget::OpenStand()
{
	UWorld* World = GetWorld();
	if (!Stand.IsValid() && World)
	{
		// Spawned the first time, then kept, idle while the screen is closed; it goes with the menu's world.
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Stand = World->SpawnActor<AHeroSelectStage>(Params);
	}
	AHeroSelectStage* StandPtr = Stand.Get();
	if (!StandPtr)
	{
		return;
	}
	// A world that hasn't begun play (a test's) begins nothing it spawns: the stand still needs its picture.
	if (!StandPtr->HasActorBegunPlay())
	{
		StandPtr->DispatchBeginPlay();
	}
	if (!StandMaterial)
	{
		// The loadout's picture material: it cuts the model out of the capture and tone maps it.
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, LoadoutParts::StageMaterialPath))
		{
			StandMaterial = UMaterialInstanceDynamic::Create(Base, this);
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Hero select: %s is missing, so the stand shows the paintings."), LoadoutParts::StageMaterialPath);
		}
	}
	if (StandMaterial && StandImage)
	{
		StandMaterial->SetTextureParameterValue(TEXT("Capture"), StandPtr->GetRenderTarget());
		StandMaterial->SetScalarParameterValue(TEXT("Exposure"), StandPtr->GetExposure());
		FSlateBrush Brush;
		Brush.SetResourceObject(StandMaterial);
		Brush.ImageSize = StandSize;
		StandImage->SetBrush(Brush);
	}
	StandPtr->SetActive(true);
}

void UHeroSelectWidget::ShowStandHero(bool bSwitched)
{
	const UHeroDefinition* Hero = GetHero(Selected);
	AHeroSelectStage* StandPtr = Stand.Get();
	// Their body on the stand when it's in the game; otherwise their painting where the stand would be.
	bStandShown = Hero && StandPtr && StandMaterial && StandPtr->ShowHero(Hero, bSwitched);
	if (StandImage)
	{
		StandImage->SetVisibility(bStandShown ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (StagePicture)
	{
		StagePicture->SetVisibility(bStandShown ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (StandStamp)
	{
		StandStamp->SetVisibility(bStandShown && !CanConfirm(Selected) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bStandShown)
	{
		bDraggingStand = false;
	}
}

void UHeroSelectWidget::TickStand(float DeltaSeconds)
{
	AHeroSelectStage* StandPtr = Stand.Get();
	if (!StandPtr || !bStandShown)
	{
		return;
	}
	if (!FMath::IsNearlyZero(StandTurnInput) && !IsConfirming())
	{
		StandPtr->AddTurn(StandTurnInput * StickTurnRate * DeltaSeconds);
	}
	if (StandMaterial)
	{
		StandMaterial->SetScalarParameterValue(TEXT("Exposure"), CanConfirm(Selected) ? StandPtr->GetExposure() : SilhouetteExposure);
	}
}

void UHeroSelectWidget::HandleTurnStick(float Value)
{
	StandTurnInput = FMath::Abs(Value) > StickDeadZone ? -Value : 0.f;
}

void UHeroSelectWidget::Close()
{
	bDraggingStand = false;
	StandTurnInput = 0.f;
	StopNavHold();
	if (AHeroSelectStage* StandPtr = Stand.Get())
	{
		StandPtr->SetFlare(0.f);
		StandPtr->SetActive(false);
	}
}

void UHeroSelectWidget::NativeDestruct()
{
	if (AHeroSelectStage* StandPtr = Stand.Get())
	{
		StandPtr->Destroy();
	}
	Stand.Reset();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Turning it with the mouse
// ---------------------------------------------------------------------------

FReply UHeroSelectWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bStandShown && StandImage && Stand.IsValid() && !IsConfirming()
		&& StandImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDraggingStand = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UHeroSelectWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDraggingStand)
	{
		// Dragging right turns the hero's front to the right, as a hand would turn a turntable.
		if (AHeroSelectStage* StandPtr = Stand.Get())
		{
			StandPtr->AddTurn(-InMouseEvent.GetCursorDelta().X * DragTurnRate);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UHeroSelectWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDraggingStand && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDraggingStand = false;
		// The menu holding this screen keeps the keyboard.
		if (UUserWidget* Host = GetTypedOuter<UUserWidget>())
		{
			Host->SetKeyboardFocus();
		}
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UHeroSelectWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDraggingStand = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}
