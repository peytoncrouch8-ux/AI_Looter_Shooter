#include "UI/PlayerHUDWidget.h"
#include "UI/LooterUIStyle.h"
#include "UI/WeaponText.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Game/MinimapSubsystem.h"
#include "Loot/AmmoPickup.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	constexpr int32 NumCompareStats = 5;
	constexpr int32 HealthSegmentCount = 20;
	constexpr int32 AmmoSegmentCount = 24;
	constexpr int32 MaxSlotPips = 4;

	/** Opacity of a corner cluster when nothing is happening. */
	constexpr float IdleOpacity = 0.6f;
	/** Seconds a cluster stays fully visible after something happens in it. */
	constexpr float ActivityHold = 3.f;
	/** Parallelogram slant of the bars, in degrees (mirrored left/right, like the reference). */
	constexpr float BarSlant = 16.f;

	// Minimap: how much of the world it shows (radius, cm, at any size), and how many things it can mark at once. Its size on
	// screen is UPlayerHUDWidget::MinimapDiameter, scaled by the player's setting.
	constexpr float MinimapRange = 3500.f;
	constexpr int32 MaxMinimapMarkers = 32;
	constexpr float MinimapArrowSize = 22.f;

	const FLinearColor Outline(0.f, 0.02f, 0.04f, 0.85f);

	UCanvasPanelSlot* PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(true);
		return CanvasSlot;
	}

	/** HUD text: no panel behind it, so it carries its own dark outline to read against sky, snow or grass. */
	UTextBlock* HudText(UWidgetTree* Tree, int32 Size, const FLinearColor& TextColor, int32 Spacing = 0, ETextJustify::Type Justify = ETextJustify::Left)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo FontInfo = Font(Size, true, Spacing);
		FontInfo.OutlineSettings.OutlineSize = FMath::Max(1, Size / 14);
		FontInfo.OutlineSettings.OutlineColor = Outline;
		Text->SetFont(FontInfo);
		Text->SetColorAndOpacity(FSlateColor(TextColor));
		Text->SetJustification(Justify);
		return Text;
	}

	/** A white, dark-edged rectangle tinted per use (segments, ticks, pips). */
	UImage* TintedRect(UWidgetTree* Tree)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(RectBrush(FLinearColor::White));
		return Image;
	}

	/** Slim segmented bar, sheared into a parallelogram, on a faint dark backing. */
	UWidget* MakeSlantBar(UWidgetTree* Tree, int32 Count, float Width, float Height, float Shear, TArray<TObjectPtr<UImage>>& OutSegments)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(Width);
		Size->SetHeightOverride(Height);
		UBorder* Backing = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Backing->SetBrush(RectBrush(FLinearColor(0.f, 0.02f, 0.04f, 0.45f)));
		MarkBackground(Backing);
		Backing->SetPadding(FMargin(2.f));
		UHorizontalBox* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UImage* Segment = TintedRect(Tree);
			Segment->SetColorAndOpacity(Color::SegmentOff());
			UHorizontalBoxSlot* SegmentSlot = Bar->AddChildToHorizontalBox(Segment);
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Count ? 2.f : 0.f, 0.f));
			OutSegments.Add(Segment);
		}
		Backing->SetContent(Bar);
		Size->SetContent(Backing);
		Size->SetRenderShear(FVector2D(Shear, 0.f));
		return Size;
	}

	/** Four thin ticks around a gap (crosshair, and rotated 45 degrees for the hit marker). */
	UOverlay* MakeTicks(UWidgetTree* Tree, float Length, float Thickness, TArray<TObjectPtr<UImage>>* OutTicks = nullptr)
	{
		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		struct FTick { EHorizontalAlignment H; EVerticalAlignment V; bool bVertical; };
		const FTick Ticks[] = {
			{ HAlign_Center, VAlign_Top, true }, { HAlign_Center, VAlign_Bottom, true },
			{ HAlign_Left, VAlign_Center, false }, { HAlign_Right, VAlign_Center, false } };
		for (const FTick& Tick : Ticks)
		{
			USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Box->SetWidthOverride(Tick.bVertical ? Thickness : Length);
			Box->SetHeightOverride(Tick.bVertical ? Length : Thickness);
			UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass());
			Image->SetBrush(RectBrush(FLinearColor::White, Outline, 1.f));
			Image->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
			Box->SetContent(Image);
			UOverlaySlot* TickSlot = Overlay->AddChildToOverlay(Box);
			TickSlot->SetHorizontalAlignment(Tick.H);
			TickSlot->SetVerticalAlignment(Tick.V);
			if (OutTicks)
			{
				OutTicks->Add(Image);
			}
		}
		return Overlay;
	}

	FString FormatNumber(float Value, int32 Decimals)
	{
		FNumberFormattingOptions Options;
		Options.UseGrouping = false;
		Options.MinimumFractionalDigits = Decimals;
		Options.MaximumFractionalDigits = Decimals;
		return FText::AsNumber(Value, &Options).ToString();
	}

	/** "DAMAGE  19.2  (+1.3)" with green/red depending on whether the new value is an upgrade. */
	/** "LABEL  value (+delta)", colored better/worse against the weapon in hand. ShownValue replaces the formatted number when set. */
	void SetCompareLine(UTextBlock* Text, const TCHAR* Label, float NewValue, float OldValue, bool bHigherIsBetter,
		int32 Decimals, const TCHAR* Prefix, const TCHAR* Suffix, bool bHasCurrent, const FString& ShownValue = FString())
	{
		FString Line = FString::Printf(TEXT("%-10s "), Label) + Prefix + (ShownValue.IsEmpty() ? FormatNumber(NewValue, Decimals) : ShownValue) + Suffix;
		FLinearColor LineColor = Color::Text();
		if (bHasCurrent && !FMath::IsNearlyEqual(NewValue, OldValue, 0.01f))
		{
			const float Delta = NewValue - OldValue;
			Line += FString::Printf(TEXT("   (%s%s)"), Delta > 0.f ? TEXT("+") : TEXT(""), *FormatNumber(Delta, FMath::Max(Decimals, 1)));
			LineColor = ((Delta > 0.f) == bHigherIsBetter) ? Color::Better() : Color::Worse();
		}
		Text->SetText(FText::FromString(Line));
		Text->SetColorAndOpacity(FSlateColor(LineColor));
	}

	void SetTextIfChanged(UTextBlock* Text, const FString& Value)
	{
		if (!Text->GetText().ToString().Equals(Value))
		{
			Text->SetText(FText::FromString(Value));
		}
	}
}

TSharedRef<SWidget> UPlayerHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		const FAnchors Center(0.5f, 0.5f);

		// Crosshair: four thin ticks with a center dot; the gap follows the weapon's spread.
		{
			// 4px ticks with a 1px dark edge leave a 2px white core that reads on any background.
			UOverlay* Ticks = MakeTicks(WidgetTree, 9.f, 4.f);
			USizeBox* Dot = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Dot->SetWidthOverride(3.f);
			Dot->SetHeightOverride(3.f);
			UImage* DotImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			DotImage->SetBrush(RectBrush(FLinearColor(1.f, 1.f, 1.f, 0.8f)));
			Dot->SetContent(DotImage);
			UOverlaySlot* DotSlot = Ticks->AddChildToOverlay(Dot);
			DotSlot->SetHorizontalAlignment(HAlign_Center);
			DotSlot->SetVerticalAlignment(VAlign_Center);

			CrosshairBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			CrosshairBox->SetWidthOverride(20.f);
			CrosshairBox->SetHeightOverride(20.f);
			CrosshairBox->SetContent(Ticks);
			PlaceOnCanvas(Root, CrosshairBox, Center, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
		}

		// Hit marker: the same ticks turned 45 degrees into an X.
		{
			USizeBox* MarkerBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			MarkerBox->SetWidthOverride(34.f);
			MarkerBox->SetHeightOverride(34.f);
			MarkerBox->SetContent(MakeTicks(WidgetTree, 11.f, 4.f, &HitMarkerTicks));
			MarkerBox->SetRenderTransformAngle(45.f);
			MarkerBox->SetVisibility(ESlateVisibility::Hidden);
			HitMarker = MarkerBox;
			PlaceOnCanvas(Root, MarkerBox, Center, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
		}

		// Message plate under the crosshair.
		MessageText = MakeText(WidgetTree, TEXT(""), 15, Color::Accent(), true, 150);
		MessagePlate = MakePlate(WidgetTree, MessageText, FMargin(18.f, 8.f));
		MessagePlate->SetVisibility(ESlateVisibility::Hidden);
		PlaceOnCanvas(Root, MessagePlate, FAnchors(0.5f, 0.72f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);

		// Bottom-left: health icon + number, slim slanted bar underneath.
		{
			UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			IconSize->SetWidthOverride(22.f);
			IconSize->SetHeightOverride(18.f);
			UBorder* Icon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Icon->SetBrush(RectBrush(Color::Health(), Outline, 1.f));
			Icon->SetHorizontalAlignment(HAlign_Center);
			Icon->SetVerticalAlignment(VAlign_Center);
			Icon->SetPadding(FMargin(0.f));
			UTextBlock* Cross = HudText(WidgetTree, 15, FLinearColor(0.06f, 0.01f, 0.01f), 0, ETextJustify::Center);
			Cross->SetText(FText::FromString(TEXT("+")));
			Icon->SetContent(Cross);
			IconSize->SetContent(Icon);
			IconSize->SetRenderShear(FVector2D(-BarSlant, 0.f));
			Row->AddChildToHorizontalBox(IconSize)->SetVerticalAlignment(VAlign_Center);

			HealthValue = HudText(WidgetTree, 24, Color::Text());
			UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(HealthValue);
			ValueSlot->SetVerticalAlignment(VAlign_Bottom);
			ValueSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			HealthMax = HudText(WidgetTree, 12, Color::TextDim());
			UHorizontalBoxSlot* MaxSlot = Row->AddChildToHorizontalBox(HealthMax);
			MaxSlot->SetVerticalAlignment(VAlign_Bottom);
			MaxSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 4.f));
			Box->AddChildToVerticalBox(Row);

			UVerticalBoxSlot* BarSlot = Box->AddChildToVerticalBox(MakeSlantBar(WidgetTree, HealthSegmentCount, 260.f, 12.f, -BarSlant, HealthSegments));
			BarSlot->SetPadding(FMargin(4.f, 4.f, 0.f, 0.f));

			VitalsCluster = Box;
			PlaceOnCanvas(Root, Box, FAnchors(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(44.f, -38.f));
		}

		// Bottom-right: ammo count, magazine bar, and the weapon's name underneath.
		{
			UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			auto AddRight = [Box](UWidget* Child, float Top)
			{
				UVerticalBoxSlot* ChildSlot = Box->AddChildToVerticalBox(Child);
				ChildSlot->SetHorizontalAlignment(HAlign_Right);
				ChildSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
			};

			UHorizontalBox* AmmoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			StatusText = HudText(WidgetTree, 12, Color::Accent(), 200, ETextJustify::Right);
			UHorizontalBoxSlot* StatusSlot = AmmoRow->AddChildToHorizontalBox(StatusText);
			StatusSlot->SetVerticalAlignment(VAlign_Center);
			StatusSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			AmmoText = HudText(WidgetTree, 34, Color::Text(), 0, ETextJustify::Right);
			AmmoRow->AddChildToHorizontalBox(AmmoText)->SetVerticalAlignment(VAlign_Bottom);
			ReserveText = HudText(WidgetTree, 15, Color::TextDim(), 0, ETextJustify::Left);
			UHorizontalBoxSlot* ReserveSlot = AmmoRow->AddChildToHorizontalBox(ReserveText);
			ReserveSlot->SetVerticalAlignment(VAlign_Bottom);
			ReserveSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 6.f));
			AddRight(AmmoRow, 0.f);

			AddRight(MakeSlantBar(WidgetTree, AmmoSegmentCount, 260.f, 10.f, BarSlant, AmmoSegments), 2.f);

			UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			WeaponName = HudText(WidgetTree, 13, Color::Text(), 120, ETextJustify::Right);
			NameRow->AddChildToHorizontalBox(WeaponName)->SetVerticalAlignment(VAlign_Center);
			UHorizontalBox* Pips = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			for (int32 Index = 0; Index < MaxSlotPips; ++Index)
			{
				USizeBox* PipSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
				PipSize->SetWidthOverride(12.f);
				PipSize->SetHeightOverride(6.f);
				UImage* Pip = TintedRect(WidgetTree);
				PipSize->SetContent(Pip);
				Pips->AddChildToHorizontalBox(PipSize)->SetPadding(FMargin(3.f, 0.f, 0.f, 0.f));
				SlotPips.Add(Pip);
			}
			Pips->SetRenderShear(FVector2D(BarSlant, 0.f));
			UHorizontalBoxSlot* PipsSlot = NameRow->AddChildToHorizontalBox(Pips);
			PipsSlot->SetVerticalAlignment(VAlign_Center);
			PipsSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			AddRight(NameRow, 5.f);

			WeaponCluster = Box;
			PlaceOnCanvas(Root, Box, FAnchors(1.f, 1.f), FVector2D(1.f, 1.f), FVector2D(-44.f, -34.f));
		}

		// Right-middle: comparison card for the loot you're looking at.
		{
			UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			Box->AddChildToVerticalBox(MakeSection(WidgetTree, TEXT("Loot")));
			PickupName = MakeText(WidgetTree, TEXT(""), 17, Color::Text(), false, 80);
			Box->AddChildToVerticalBox(PickupName)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			PickupLevel = MakeText(WidgetTree, TEXT(""), 11, Color::TextDim(), false, 120);
			Box->AddChildToVerticalBox(PickupLevel)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			for (int32 Stat = 0; Stat < NumCompareStats; ++Stat)
			{
				UTextBlock* StatText = MakeText(WidgetTree, TEXT(""), 13, Color::Text());
				PickupStatTexts.Add(StatText);
				Box->AddChildToVerticalBox(StatText)->SetPadding(FMargin(0.f, 1.f));
			}
			PickupHint = MakeText(WidgetTree, TEXT(""), 13, Color::Accent(), false, 150);
			Box->AddChildToVerticalBox(PickupHint)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

			USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			CardSize->SetMinDesiredWidth(320.f);
			CardSize->SetContent(Box);
			PickupCard = MakePlate(WidgetTree, CardSize, FMargin(16.f, 12.f));
			PickupCard->SetVisibility(ESlateVisibility::Collapsed);
			PlaceOnCanvas(Root, PickupCard, FAnchors(1.f, 0.5f), FVector2D(1.f, 0.5f), FVector2D(-32.f, 0.f));
		}

		// Top-right: round minimap that turns with the view. Layers, bottom to top: dark glass disc, the map (clipped
		// to the circle), loot/hostile markers and the player arrow, the rim, then the facing notch and the N.
		{
			const float Radius = MinimapDiameter * 0.5f;
			USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Size->SetWidthOverride(MinimapDiameter);
			Size->SetHeightOverride(MinimapDiameter);
			UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			Size->SetContent(Stack);
			auto AddLayer = [Stack](UWidget* Layer)
			{
				UOverlaySlot* LayerSlot = Stack->AddChildToOverlay(Layer);
				LayerSlot->SetHorizontalAlignment(HAlign_Fill);
				LayerSlot->SetVerticalAlignment(VAlign_Fill);
			};
			auto MakeImageWith = [this](const FSlateBrush& Brush)
			{
				UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				Image->SetBrush(Brush);
				return Image;
			};
			auto AddToCanvas = [](UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& WidgetSize)
			{
				UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetPosition(Position);
				CanvasSlot->SetAutoSize(WidgetSize.IsZero());
				if (!WidgetSize.IsZero())
				{
					CanvasSlot->SetSize(WidgetSize);
				}
			};

			const FLinearColor Glass = Color::ScreenBg();
			UImage* GlassDisc = MakeImageWith(CircleBrush(FLinearColor(Glass.R, Glass.G, Glass.B, 0.62f)));
			MarkBackground(GlassDisc);
			AddLayer(GlassDisc);

			MinimapBrush = CircleBrush(FLinearColor::White);
			MinimapMap = MakeImageWith(MinimapBrush);
			MinimapMap->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			MinimapMap->SetVisibility(ESlateVisibility::Hidden);
			AddLayer(MinimapMap);

			UCanvasPanel* Markers = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
			AddLayer(Markers);
			for (int32 Index = 0; Index < MaxMinimapMarkers; ++Index)
			{
				UImage* Marker = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
				Marker->SetVisibility(ESlateVisibility::Hidden);
				AddToCanvas(Markers, Marker, FVector2D(Radius), FVector2D(10.f));
				MinimapMarkers.Add(Marker);
				MinimapMarkerKeys.Add(0);
			}
			// You: an arrow in the middle that always points up (the map turns under it).
			MinimapArrow = MakeImageWith(MarkerBrush(EMarker::Arrow, FLinearColor::White));
			AddToCanvas(Markers, MinimapArrow, FVector2D(Radius), FVector2D(MinimapArrowSize));

			AddLayer(MakeImageWith(CircleBrush(FLinearColor::Transparent, Color::ScreenLine(), 2.f)));

			UCanvasPanel* Rim = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
			AddLayer(Rim);
			MinimapNotch = MakeImageWith(RectBrush(Color::Accent()));
			AddToCanvas(Rim, MinimapNotch, FVector2D(Radius, 5.f), FVector2D(3.f, 10.f));
			MinimapNorth = HudText(WidgetTree, 12, Color::Accent(), 0, ETextJustify::Center);
			MinimapNorth->SetText(FText::FromString(TEXT("N")));
			AddToCanvas(Rim, MinimapNorth, FVector2D(Radius, 12.f), FVector2D::ZeroVector);

			MinimapSize = Size;
			MinimapCluster = Size;
			PlaceOnCanvas(Root, Size, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-MinimapMargin, MinimapMargin));
		}
	}
	return Super::RebuildWidget();
}

FString UPlayerHUDWidget::BoundKeyName(FName BindingId, const TCHAR* Fallback) const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	return Bindings ? Bindings->GetKey(BindingId).GetDisplayName().ToString().ToUpper() : FString(Fallback);
}

void UPlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	PulseTime += InDeltaTime;

	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;

	BindToPawn(Manager);
	UpdateWeaponCluster(Manager, InDeltaTime);
	UpdateVitals(Health, InDeltaTime);
	UpdatePickupCard(Manager);
	UpdateMinimap(Pawn);

	if (HitMarkerTime > 0.f)
	{
		HitMarkerTime -= InDeltaTime;
		const float Alpha = FMath::Clamp(HitMarkerTime / 0.18f, 0.f, 1.f);
		HitMarker->SetRenderOpacity(Alpha);
		// Pops in slightly large and settles.
		HitMarker->SetRenderScale(FVector2D(1.f + 0.3f * Alpha * Alpha));
		if (HitMarkerTime <= 0.f)
		{
			HitMarker->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (MessageTime > 0.f)
	{
		MessageTime -= InDeltaTime;
		MessagePlate->SetRenderOpacity(FMath::Clamp(MessageTime / 0.5f, 0.f, 1.f));
		if (MessageTime <= 0.f)
		{
			MessagePlate->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UPlayerHUDWidget::BindToPawn(UWeaponManagerComponent* Manager)
{
	if (BoundManager.Get() != Manager)
	{
		if (UWeaponManagerComponent* Old = BoundManager.Get())
		{
			Old->OnMessage.RemoveDynamic(this, &UPlayerHUDWidget::HandleMessage);
		}
		if (Manager)
		{
			Manager->OnMessage.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleMessage);
		}
		BoundManager = Manager;
	}

	// Hit markers and reload progress come from whichever weapon is in hand.
	AWeaponBase* Active = Manager ? Manager->GetActiveWeapon() : nullptr;
	if (BoundWeapon.Get() != Active)
	{
		if (AWeaponBase* Old = BoundWeapon.Get())
		{
			Old->OnHit.RemoveDynamic(this, &UPlayerHUDWidget::HandleHit);
			Old->OnReloadStarted.RemoveDynamic(this, &UPlayerHUDWidget::HandleReloadStarted);
		}
		if (Active)
		{
			Active->OnHit.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleHit);
			Active->OnReloadStarted.AddUniqueDynamic(this, &UPlayerHUDWidget::HandleReloadStarted);
		}
		BoundWeapon = Active;
		// A weapon switch is worth showing.
		WeaponActivity = ActivityHold;
		LastMagazine = INDEX_NONE;
		ReloadDuration = 0.f;
	}
}

void UPlayerHUDWidget::UpdateWeaponCluster(UWeaponManagerComponent* Manager, float DeltaTime)
{
	const AWeaponBase* Active = Manager ? Manager->GetActiveWeapon() : nullptr;
	WeaponCluster->SetVisibility(Active ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	UpdateCrosshair(Active, DeltaTime);
	if (!Active)
	{
		return;
	}

	const int32 Magazine = Active->GetCurrentMagazine();
	const int32 Reserve = Active->GetReserveAmmo();
	const int32 MagazineSize = FMath::Max(1, Active->GetStats().MagazineSize);
	if (Magazine != LastMagazine || Reserve != LastReserve)
	{
		WeaponActivity = ActivityHold;
		LastMagazine = Magazine;
		LastReserve = Reserve;
	}
	const bool bReloading = Active->IsReloading();
	if (bReloading)
	{
		WeaponActivity = ActivityHold;
		ReloadElapsed += DeltaTime;
	}
	WeaponActivity = FMath::Max(0.f, WeaponActivity - DeltaTime);

	const float MagazineFraction = static_cast<float>(Magazine) / MagazineSize;
	const bool bLow = MagazineFraction <= 0.25f;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(PulseTime * 8.f);

	// Numbers: magazine turns orange when running low, red when empty.
	SetTextIfChanged(AmmoText, FString::FromInt(Magazine));
	AmmoText->SetColorAndOpacity(FSlateColor(Magazine == 0 ? Color::Worse() : (bLow ? Color::Accent() : Color::Text())));
	SetTextIfChanged(ReserveText, FString::Printf(TEXT("/ %d"), Reserve));
	ReserveText->SetColorAndOpacity(FSlateColor(Reserve == 0 ? Color::Worse() : Color::TextDim()));

	// The magazine bar doubles as reload progress.
	if (bReloading)
	{
		const float Progress = ReloadDuration > 0.f ? FMath::Clamp(ReloadElapsed / ReloadDuration, 0.f, 1.f) : 0.f;
		const int32 Filled = FMath::FloorToInt(Progress * AmmoSegmentCount);
		for (int32 Index = 0; Index < AmmoSegments.Num(); ++Index)
		{
			AmmoSegments[Index]->SetColorAndOpacity(Index < Filled ? Color::Accent() : Color::SegmentOff());
		}
	}
	else
	{
		const int32 Lit = FMath::Clamp(FMath::CeilToInt(MagazineFraction * AmmoSegmentCount), 0, AmmoSegmentCount);
		const FLinearColor On = bLow ? Color::Accent() : Color::SegmentOn();
		for (int32 Index = 0; Index < AmmoSegments.Num(); ++Index)
		{
			AmmoSegments[Index]->SetColorAndOpacity(Index < Lit ? On : Color::SegmentOff());
		}
	}

	// Status beside the count: reloading, a prompt when dry, or nothing.
	if (bReloading)
	{
		SetTextIfChanged(StatusText, TEXT("RELOADING"));
		StatusText->SetColorAndOpacity(FSlateColor(Color::Accent()));
	}
	else if (Magazine == 0 && Reserve > 0)
	{
		SetTextIfChanged(StatusText, FString::Printf(TEXT("[%s] RELOAD"), *BoundKeyName(TEXT("Reload"), TEXT("R"))));
		StatusText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Accent(), Color::Worse(), Pulse)));
	}
	else if (Magazine == 0)
	{
		SetTextIfChanged(StatusText, TEXT("NO AMMO"));
		StatusText->SetColorAndOpacity(FSlateColor(Color::Worse()));
	}
	else
	{
		SetTextIfChanged(StatusText, TEXT(""));
	}

	SetTextIfChanged(WeaponName, LooterWeaponText::Name(Active->GetInstance()).ToUpper());
	WeaponName->SetColorAndOpacity(FSlateColor(LooterWeaponText::Color(Active->GetInstance())));

	// Slot pips: filled for each carried weapon, orange for the one in hand.
	const TArray<AWeaponBase*> Weapons = Manager->GetWeapons();
	for (int32 Index = 0; Index < SlotPips.Num(); ++Index)
	{
		UImage* Pip = SlotPips[Index];
		if (Index >= Manager->MaxWeapons)
		{
			Pip->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}
		Pip->SetVisibility(ESlateVisibility::HitTestInvisible);
		const bool bHasWeapon = Weapons.IsValidIndex(Index) && Weapons[Index];
		const bool bInHand = bHasWeapon && Weapons[Index] == Active;
		Pip->SetColorAndOpacity(bInHand ? Color::Accent() : (bHasWeapon ? Color::Text() : Color::SegmentOff()));
	}

	// Fade back when idle; stay up while there's something to act on.
	const bool bNeedsAttention = WeaponActivity > 0.f || bLow || Reserve == 0;
	const float Target = bNeedsAttention ? 1.f : IdleOpacity;
	WeaponCluster->SetRenderOpacity(FMath::FInterpTo(WeaponCluster->GetRenderOpacity(), Target, DeltaTime, 5.f));
}

void UPlayerHUDWidget::UpdateVitals(UHealthComponent* Health, float DeltaTime)
{
	VitalsCluster->SetVisibility(Health ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Health)
	{
		return;
	}

	const float Fraction = FMath::Clamp(Health->GetHealthPercent(), 0.f, 1.f);
	if (ShownHealthFraction < 0.f)
	{
		ShownHealthFraction = Fraction;
		GhostHealthFraction = Fraction;
	}
	if (Fraction < ShownHealthFraction - KINDA_SMALL_NUMBER)
	{
		// Took damage: the lost chunk lingers as a light "chip", then drains away.
		GhostHoldTime = 0.45f;
		VitalsActivity = ActivityHold;
	}
	else if (Fraction > ShownHealthFraction + KINDA_SMALL_NUMBER)
	{
		GhostHealthFraction = Fraction;
		VitalsActivity = ActivityHold;
	}
	ShownHealthFraction = Fraction;
	if (GhostHoldTime > 0.f)
	{
		GhostHoldTime -= DeltaTime;
	}
	else
	{
		GhostHealthFraction = FMath::FInterpConstantTo(GhostHealthFraction, Fraction, DeltaTime, 0.6f);
	}
	GhostHealthFraction = FMath::Max(GhostHealthFraction, Fraction);
	VitalsActivity = FMath::Max(0.f, VitalsActivity - DeltaTime);

	SetTextIfChanged(HealthValue, FString::FromInt(FMath::CeilToInt(Health->GetHealth())));
	SetTextIfChanged(HealthMax, FString::Printf(TEXT("/ %d"), FMath::RoundToInt(Health->GetMaxHealth())));

	const bool bLow = Fraction <= 0.3f;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(PulseTime * 6.f);
	const FLinearColor Bar = bLow ? FMath::Lerp(Color::Health(), FLinearColor(1.f, 0.75f, 0.7f), Pulse * 0.6f) : Color::Health();
	const FLinearColor Chip(1.f, 0.82f, 0.72f, 0.9f);
	const int32 Lit = Fraction <= 0.f ? 0 : FMath::Clamp(FMath::CeilToInt(Fraction * HealthSegmentCount), 1, HealthSegmentCount);
	const int32 Ghost = FMath::Clamp(FMath::CeilToInt(GhostHealthFraction * HealthSegmentCount), Lit, HealthSegmentCount);
	for (int32 Index = 0; Index < HealthSegments.Num(); ++Index)
	{
		HealthSegments[Index]->SetColorAndOpacity(Index < Lit ? Bar : (Index < Ghost ? Chip : Color::SegmentOff()));
	}
	HealthValue->SetColorAndOpacity(FSlateColor(bLow ? FMath::Lerp(Color::Health(), Color::Text(), Pulse) : Color::Text()));

	// Full health and nothing happening: step back. Hurt, low, or just hit: full strength.
	const bool bNeedsAttention = VitalsActivity > 0.f || Fraction < 0.999f;
	const float Target = bNeedsAttention ? 1.f : IdleOpacity;
	VitalsCluster->SetRenderOpacity(FMath::FInterpTo(VitalsCluster->GetRenderOpacity(), Target, DeltaTime, 5.f));
}

void UPlayerHUDWidget::UpdateCrosshair(const AWeaponBase* Active, float DeltaTime)
{
	// The gap tracks the weapon's accuracy: tight for rifles, wide for shotguns, tighter still when crouched.
	const float Spread = Active ? Active->GetEffectiveSpread() : 1.f;
	const float Wanted = FMath::Clamp(22.f + Spread * 7.f, 24.f, 80.f);
	if (!FMath::IsNearlyEqual(Wanted, CrosshairSize, 0.25f))
	{
		CrosshairSize = CrosshairSize <= 0.f ? Wanted : FMath::FInterpTo(CrosshairSize, Wanted, DeltaTime, 10.f);
		CrosshairBox->SetWidthOverride(CrosshairSize);
		CrosshairBox->SetHeightOverride(CrosshairSize);
	}

	// No aiming while the gun is down in the sprint pose, and no crosshair when the camera faces the character.
	const AActor* Holder = Active ? Active->GetOwner() : nullptr;
	const UPlayerLocomotionComponent* Locomotion = Holder ? Holder->FindComponentByClass<UPlayerLocomotionComponent>() : nullptr;
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const bool bFacingCamera = View && View->GetViewMode() == EPlayerViewMode::ThirdPersonFront;
	const float Opacity = bFacingCamera ? 0.f : 1.f - (Locomotion ? Locomotion->GetSprintAlpha() : 0.f);
	if (!FMath::IsNearlyEqual(CrosshairBox->GetRenderOpacity(), Opacity, 0.01f))
	{
		CrosshairBox->SetRenderOpacity(Opacity);
	}
}

void UPlayerHUDWidget::UpdatePickupCard(UWeaponManagerComponent* Manager)
{
	const AWeaponBase* Pickup = Manager ? Manager->GetFocusedPickup() : nullptr;
	if (!Pickup)
	{
		PickupCard->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	PickupCard->SetVisibility(ESlateVisibility::HitTestInvisible);

	const FWeaponInstanceData& New = Pickup->GetInstance();
	const AWeaponBase* Current = Manager->GetActiveWeapon();
	const FWeaponStats Old = Current ? Current->GetStats() : FWeaponStats();
	const bool bHasCurrent = Current != nullptr;
	const FWeaponStats& S = New.Stats;

	PickupName->SetText(FText::FromString(LooterWeaponText::Name(New).ToUpper()));
	PickupName->SetColorAndOpacity(FSlateColor(LooterWeaponText::Color(New)));
	PickupLevel->SetText(FText::FromString(FString::Printf(TEXT("LV %d  |  %s  |  VS WEAPON IN HAND"), New.Level, *LooterWeaponText::FireModeName(New).ToUpper())));

	// Damage shows the weapon's damage, compared on total per-shot damage so shotguns and rifles line up fairly.
	SetCompareLine(PickupStatTexts[0], TEXT("DAMAGE"), S.Damage * S.PelletsPerShot, Old.Damage * Old.PelletsPerShot, true, 1, TEXT(""), TEXT(""), bHasCurrent,
		LooterWeaponText::DamageString(S));
	SetCompareLine(PickupStatTexts[1], TEXT("FIRE RATE"), S.FireRate, Old.FireRate, true, 0, TEXT(""), TEXT(" RPM"), bHasCurrent);
	SetCompareLine(PickupStatTexts[2], TEXT("MAGAZINE"), S.MagazineSize, Old.MagazineSize, true, 0, TEXT(""), TEXT(""), bHasCurrent);
	SetCompareLine(PickupStatTexts[3], TEXT("RELOAD"), S.ReloadTime, Old.ReloadTime, false, 2, TEXT(""), TEXT("S"), bHasCurrent);
	SetCompareLine(PickupStatTexts[4], TEXT("SPREAD"), S.Spread, Old.Spread, false, 2, TEXT(""), TEXT(" DEG"), bHasCurrent);

	const bool bSlotsFull = Manager->GetWeapons().Num() >= Manager->MaxWeapons;
	const bool bBackpackFull = Manager->GetBackpack().Num() >= Manager->BackpackCapacity;
	const TCHAR* Action = !bSlotsFull ? TEXT("PICK UP") : (!bBackpackFull ? TEXT("SEND TO BACKPACK") : TEXT("SWAP WITH WEAPON IN HAND"));
	PickupHint->SetText(FText::FromString(FString::Printf(TEXT("[%s] %s"), *BoundKeyName(TEXT("Interact"), TEXT("E")), Action)));
}

void UPlayerHUDWidget::UpdateMinimap(const APawn* Pawn)
{
	UWorld* World = GetWorld();
	UMinimapSubsystem* Minimap = World ? World->GetSubsystem<UMinimapSubsystem>() : nullptr;
	if (!MinimapCluster)
	{
		return;
	}
	if (!Minimap || !Pawn)
	{
		MinimapCluster->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	MinimapCluster->SetVisibility(ESlateVisibility::HitTestInvisible);

	// The player's size setting, live (the settings menu changes it while the game is paused).
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
	const float WantedScale = Settings ? Settings->GetMinimapScale() : 1.f;
	if (!FMath::IsNearlyEqual(WantedScale, MinimapScale))
	{
		ApplyMinimapScale(WantedScale);
	}

	const FVector Location = Pawn->GetActorLocation();
	const float Yaw = Pawn->GetControlRotation().Yaw;
	const float Radius = GetMinimapDiameter() * 0.5f;
	const float PixelsPerCm = (Radius - 4.f) / MinimapRange;
	// Markers grow more gently than the map, so a small map stays readable and a big one uncluttered.
	const float MarkerScale = FMath::Sqrt(MinimapScale);

	// The island: the part of the baked picture around you, turned so the way you look points up.
	UTexture2D* Map = Minimap->GetMapTexture();
	if (Map != MinimapTexture)
	{
		MinimapTexture = Map;
		MinimapBrush.SetResourceObject(Map);
		MinimapMap->SetVisibility(Map ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (Map)
	{
		const FVector2D Center = Minimap->WorldToMapUV(Location);
		const double Half = MinimapRange / FMath::Max(Minimap->GetMapBounds().GetSize().X, 1.0);
		MinimapBrush.SetUVRegion(FBox2d(Center - FVector2D(Half), Center + FVector2D(Half)));
		MinimapMap->SetBrush(MinimapBrush);
		MinimapMap->SetRenderTransformAngle(-Yaw);
	}

	// Markers: ammo and loot first, hostiles last so they draw on top. Anything past the rim is left off.
	int32 Used = 0;
	auto Mark = [&](const FVector& Where, EMarker Kind, const FLinearColor& Tint, float BaseSize)
	{
		if (Used >= MinimapMarkers.Num())
		{
			return;
		}
		const float Size = BaseSize * MarkerScale;
		const FVector2D Offset = UMinimapSubsystem::ViewOffset(Where - Location, Yaw, PixelsPerCm);
		if (Offset.Size() > Radius - Size * 0.5f - 3.f)
		{
			return;
		}
		UImage* Marker = MinimapMarkers[Used];
		const uint32 Key = HashCombine(GetTypeHash(static_cast<uint8>(Kind)), GetTypeHash(Tint.ToFColor(true))) | 1u;
		if (MinimapMarkerKeys[Used] != Key)
		{
			Marker->SetBrush(MarkerBrush(Kind, Tint));
			MinimapMarkerKeys[Used] = Key;
		}
		if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
		{
			MarkerSlot->SetSize(FVector2D(Size));
			MarkerSlot->SetPosition(FVector2D(Radius) + Offset);
		}
		Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
		++Used;
	};
	const FLinearColor AmmoColor(FColor(230, 220, 192));
	for (TActorIterator<AAmmoPickup> It(World); It; ++It)
	{
		Mark(It->GetActorLocation(), EMarker::Dot, AmmoColor, 6.f);
	}
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		if (It->IsPickup())
		{
			Mark(It->GetActorLocation(), EMarker::Dot, LooterWeaponText::Color(It->GetInstance()), 9.f);
		}
	}
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		if (!It->IsDead())
		{
			Mark(It->GetActorLocation(), EMarker::Diamond, Color::Health(), 12.f);
		}
	}
	for (int32 Index = Used; Index < MinimapMarkers.Num(); ++Index)
	{
		MinimapMarkers[Index]->SetVisibility(ESlateVisibility::Hidden);
	}

	// North circles the rim as you turn.
	if (UCanvasPanelSlot* NorthSlot = Cast<UCanvasPanelSlot>(MinimapNorth->Slot))
	{
		const FVector2D North = UMinimapSubsystem::ViewOffset(FVector::ForwardVector, Yaw, 1.f).GetSafeNormal() * (Radius - 13.f);
		NorthSlot->SetPosition(FVector2D(Radius) + North);
	}
}

void UPlayerHUDWidget::ApplyMinimapScale(float Scale)
{
	MinimapScale = Scale;
	if (!MinimapSize)
	{
		return;
	}
	// The glass, the map and the rim fill the box, so they follow it; the arrow and the notch are placed by hand.
	const float Diameter = GetMinimapDiameter();
	const float Radius = Diameter * 0.5f;
	MinimapSize->SetWidthOverride(Diameter);
	MinimapSize->SetHeightOverride(Diameter);
	if (UCanvasPanelSlot* ArrowSlot = Cast<UCanvasPanelSlot>(MinimapArrow->Slot))
	{
		ArrowSlot->SetPosition(FVector2D(Radius));
		ArrowSlot->SetSize(FVector2D(MinimapArrowSize * FMath::Sqrt(Scale)));
	}
	if (UCanvasPanelSlot* NotchSlot = Cast<UCanvasPanelSlot>(MinimapNotch->Slot))
	{
		NotchSlot->SetPosition(FVector2D(Radius, 5.f));
	}
}

void UPlayerHUDWidget::HandleHit(const FHitResult& Hit, float Damage, bool bCritical)
{
	// Only confirm hits on things that can actually be hurt, not walls.
	const AActor* HitActor = Hit.GetActor();
	if (!HitActor || !HitActor->FindComponentByClass<UHealthComponent>())
	{
		return;
	}

	HitMarkerTime = 0.18f;
	HitMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (UImage* Tick : HitMarkerTicks)
	{
		Tick->SetColorAndOpacity(bCritical ? Color::Accent() : FLinearColor::White);
	}
}

void UPlayerHUDWidget::HandleReloadStarted(float Duration)
{
	ReloadDuration = Duration;
	ReloadElapsed = 0.f;
}

void UPlayerHUDWidget::HandleMessage(const FText& Message)
{
	MessageText->SetText(FText::FromString(Message.ToString().ToUpper()));
	MessagePlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	MessagePlate->SetRenderOpacity(1.f);
	MessageTime = 2.5f;
}
