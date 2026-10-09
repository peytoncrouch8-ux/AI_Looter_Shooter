#include "UI/Bestiary/BestiaryWidget.h"
#include "UI/Bestiary/BestiaryStage.h"
#include "UI/HUD/LooterHUD.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Components/Image.h"
#include "Engine/LocalPlayer.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"

using namespace LooterUI;
using namespace LoadoutParts;

// ---------------------------------------------------------------------------
// Choosing
// ---------------------------------------------------------------------------

void UBestiaryWidget::HandleCardClicked(ULooterButton* Button)
{
	HandleCardHovered(Button);
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the page's keys.
	SetKeyboardFocus();
}

void UBestiaryWidget::HandleCardHovered(ULooterButton* Button)
{
	if (Button)
	{
		Select(Button->Index, false);
	}
}

void UBestiaryWidget::HandleTabClicked(ULooterButton* Button)
{
	ALooterHUD* HUD = OwningHUD.Get();
	if (HUD && Button && Button->Index != static_cast<int32>(EInventoryPage::Bestiary))
	{
		HUD->ShowInventoryPage(static_cast<EInventoryPage>(Button->Index));
	}
	else
	{
		SetKeyboardFocus();
	}
}

FReply UBestiaryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || (Bindings ? Bindings->IsInventoryKey(Key) : (Key == EKeys::Tab || Key == EKeys::I)))
	{
		Close();
		return FReply::Handled();
	}
	if (Key == EKeys::One || Key == EKeys::Gamepad_LeftShoulder)
	{
		GoToLoadout();
		return FReply::Handled();
	}
	if (Key == EKeys::Three || Key == EKeys::Gamepad_RightShoulder)
	{
		if (ALooterHUD* HUD = OwningHUD.Get())
		{
			HUD->ShowInventoryPage(EInventoryPage::Missions);
		}
		return FReply::Handled();
	}
	if (Key == EKeys::Four)
	{
		if (ALooterHUD* HUD = OwningHUD.Get())
		{
			HUD->ShowInventoryPage(EInventoryPage::Map);
		}
		return FReply::Handled();
	}
	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
	{
		Select(Selected - 1, true);
		return FReply::Handled();
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
	{
		Select(Selected + 1, true);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ---------------------------------------------------------------------------
// Turning the model
// ---------------------------------------------------------------------------

FReply UBestiaryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && StageImage && Stage.IsValid()
		&& StageImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDragging = true;
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UBestiaryWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		// Dragging right turns the model's front to the right.
		if (ABestiaryStage* StagePtr = Stage.Get())
		{
			StagePtr->AddTurn(-InMouseEvent.GetCursorDelta().X * DragTurnRate);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UBestiaryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		SetKeyboardFocus();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UBestiaryWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bDragging = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

FReply UBestiaryWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent)
{
	if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightX)
	{
		const float Value = InAnalogEvent.GetAnalogValue();
		TurnInput = FMath::Abs(Value) > 0.2f ? -Value : 0.f;
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

// ---------------------------------------------------------------------------
// The stand's ring
// ---------------------------------------------------------------------------

bool UBestiaryWidget::ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const
{
	const ABestiaryStage* StagePtr = Stage.Get();
	FVector2D UV;
	if (!StagePtr || !StageImage || !StagePtr->ProjectToImage(WorldLocation, UV))
	{
		return false;
	}
	OutPoint = FVector2f(BestiaryLayout::StageTopLeft + UV * BestiaryLayout::StageSize);
	return true;
}

namespace
{
	/** Points around a ring on the floor under the model, from one angle to another (degrees, 0 = toward the camera). */
	template <typename FProject>
	TArray<FVector2f> RingArc(const ABestiaryStage& Stage, float Radius, float From, float To, FProject&& Project)
	{
		const FVector Center = Stage.GetFloorCenter();
		const FVector Toward = Stage.GetTowardCamera();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Toward);
		TArray<FVector2f> Points;
		constexpr int32 Steps = 48;
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(From, To, static_cast<float>(Step) / Steps));
			FVector2f Point;
			if (Project(Center + (Toward * FMath::Cos(Angle) + Side * FMath::Sin(Angle)) * Radius, Point))
			{
				Points.Add(Point);
			}
		}
		return Points;
	}

	/** The inner ring sits this far in from the outer one, as on the loadout's stand. */
	constexpr float InnerRingShare = 32.f / 46.f;
}

void UBestiaryWidget::PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ABestiaryStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial || !StagePtr->HasModel())
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };
	const float Radius = StagePtr->GetRingRadius();

	// A soft glow over the whole ring, then the far halves of the rings, behind the model.
	const TArray<FVector2f> Circle = RingArc(*StagePtr, Radius, 0.f, 360.f, Project);
	if (Circle.Num() > 2)
	{
		const FBox2f Bounds(Circle);
		DrawBox(Elements, LayerId, Geometry, Bounds.Min, Bounds.GetSize(), &DiscBrush, Colors::RingGlow());
	}
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, Radius, 90.f, 270.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, Radius * InnerRingShare, 90.f, 270.f, Project), Colors::InnerRing(), 1.5f);
}

void UBestiaryWidget::PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	const ABestiaryStage* StagePtr = Stage.Get();
	if (!StagePtr || !StageMaterial || !StagePtr->HasModel())
	{
		return;
	}
	auto Project = [this](const FVector& World, FVector2f& Out) { return ProjectToPage(World, Out); };
	const float Radius = StagePtr->GetRingRadius();
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, Radius, -90.f, 90.f, Project), Colors::Ring(), 2.f);
	DrawLines(Elements, LayerId, Geometry, RingArc(*StagePtr, Radius * InnerRingShare, -90.f, 90.f, Project), Colors::InnerRing(), 1.5f);
}
