#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudFrameRateWidget.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/HUD/HudXPBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Combat/HealthComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Inventory/WeaponManagerComponent.h"
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
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

using namespace LooterUI;

namespace
{
	constexpr int32 NumCompareStats = 5;
	constexpr int32 HealthSegmentCount = 20;
	constexpr int32 AmmoSegmentCount = 24;
	constexpr int32 MaxSlotPips = 4;
	constexpr int32 PickupHoldSegmentCount = 16;

	/** Opacity of a corner cluster when nothing is happening. */
	constexpr float IdleOpacity = 0.6f;
	/** Seconds a cluster stays fully visible after something happens in it. */
	constexpr float ActivityHold = 3.f;
	/** Parallelogram slant of the bars, in degrees (mirrored left/right, like the reference). */
	constexpr float BarSlant = 16.f;

	UCanvasPanelSlot* PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(Anchors);
		CanvasSlot->SetAlignment(Alignment);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetAutoSize(true);
		return CanvasSlot;
	}

	/** A white rectangle, tinted per use (segments, pips). */
	UImage* TintableRect(UWidgetTree* Tree)
	{
		return MakeImage(Tree, RectBrush(FLinearColor::White));
	}

	/** Slim segmented bar, sheared into a parallelogram, on a faint dark backing. */
	UWidget* MakeSlantBar(UWidgetTree* Tree, int32 Count, float Width, float Height, float Shear, TArray<TObjectPtr<UImage>>& OutSegments)
	{
		UBorder* Backing = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Backing->SetBrush(RectBrush(FLinearColor(0.f, 0.02f, 0.04f, 0.45f)));
		MarkBackground(Backing);
		Backing->SetPadding(FMargin(2.f));
		UHorizontalBox* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UImage* Segment = TintableRect(Tree);
			Segment->SetColorAndOpacity(Color::SegmentOff());
			UHorizontalBoxSlot* SegmentSlot = Bar->AddChildToHorizontalBox(Segment);
			SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Count ? 2.f : 0.f, 0.f));
			OutSegments.Add(Segment);
		}
		Backing->SetContent(Bar);
		USizeBox* Size = MakeSized(Tree, Backing, Width, Height);
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
			UImage* Image = MakeImage(Tree, RectBrush(FLinearColor::White, Color::Outline(), 1.f));
			Image->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
			UWidget* Box = MakeSized(Tree, Image, Tick.bVertical ? Thickness : Length, Tick.bVertical ? Length : Thickness);
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
			UWidget* Dot = MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(FLinearColor(1.f, 1.f, 1.f, 0.8f))), 3.f, 3.f);
			UOverlaySlot* DotSlot = Ticks->AddChildToOverlay(Dot);
			DotSlot->SetHorizontalAlignment(HAlign_Center);
			DotSlot->SetVerticalAlignment(VAlign_Center);

			CrosshairBox = MakeSized(WidgetTree, Ticks, 20.f, 20.f);
			PlaceOnCanvas(Root, CrosshairBox, Center, FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
		}

		// Hit marker: the same ticks turned 45 degrees into an X.
		{
			USizeBox* MarkerBox = MakeSized(WidgetTree, MakeTicks(WidgetTree, 11.f, 4.f, &HitMarkerTicks), 34.f, 34.f);
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
			UBorder* Icon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Icon->SetBrush(RectBrush(Color::Health(), Color::Outline(), 1.f));
			Icon->SetHorizontalAlignment(HAlign_Center);
			Icon->SetVerticalAlignment(VAlign_Center);
			Icon->SetPadding(FMargin(0.f));
			UTextBlock* Cross = MakeFloatingText(WidgetTree, 15, FLinearColor(0.06f, 0.01f, 0.01f), 0, ETextJustify::Center);
			Cross->SetText(FText::FromString(TEXT("+")));
			Icon->SetContent(Cross);
			USizeBox* IconSize = MakeSized(WidgetTree, Icon, 22.f, 18.f);
			IconSize->SetRenderShear(FVector2D(-BarSlant, 0.f));
			Row->AddChildToHorizontalBox(IconSize)->SetVerticalAlignment(VAlign_Center);

			HealthValue = MakeFloatingText(WidgetTree, 24, Color::Text());
			UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(HealthValue);
			ValueSlot->SetVerticalAlignment(VAlign_Bottom);
			ValueSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			HealthMax = MakeFloatingText(WidgetTree, 12, Color::TextDim());
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
			StatusText = MakeFloatingText(WidgetTree, 12, Color::Accent(), 200, ETextJustify::Right);
			UHorizontalBoxSlot* StatusSlot = AmmoRow->AddChildToHorizontalBox(StatusText);
			StatusSlot->SetVerticalAlignment(VAlign_Center);
			StatusSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
			AmmoText = MakeFloatingText(WidgetTree, 34, Color::Text(), 0, ETextJustify::Right);
			AmmoRow->AddChildToHorizontalBox(AmmoText)->SetVerticalAlignment(VAlign_Bottom);
			ReserveText = MakeFloatingText(WidgetTree, 15, Color::TextDim(), 0, ETextJustify::Left);
			UHorizontalBoxSlot* ReserveSlot = AmmoRow->AddChildToHorizontalBox(ReserveText);
			ReserveSlot->SetVerticalAlignment(VAlign_Bottom);
			ReserveSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 6.f));
			AddRight(AmmoRow, 0.f);

			AddRight(MakeSlantBar(WidgetTree, AmmoSegmentCount, 260.f, 10.f, BarSlant, AmmoSegments), 2.f);

			UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			WeaponName = MakeFloatingText(WidgetTree, 13, Color::Text(), 120, ETextJustify::Right);
			NameRow->AddChildToHorizontalBox(WeaponName)->SetVerticalAlignment(VAlign_Center);
			UHorizontalBox* Pips = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			for (int32 Index = 0; Index < MaxSlotPips; ++Index)
			{
				UImage* Pip = TintableRect(WidgetTree);
				Pips->AddChildToHorizontalBox(MakeSized(WidgetTree, Pip, 12.f, 6.f))->SetPadding(FMargin(3.f, 0.f, 0.f, 0.f));
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
			// Fills while the interact key is held to equip; hidden (keeping its space, so the card doesn't jump) otherwise.
			PickupHoldBar = MakeSlantBar(WidgetTree, PickupHoldSegmentCount, 200.f, 6.f, BarSlant, PickupHoldSegments);
			PickupHoldBar->SetVisibility(ESlateVisibility::Hidden);
			UVerticalBoxSlot* HoldSlot = Box->AddChildToVerticalBox(PickupHoldBar);
			HoldSlot->SetHorizontalAlignment(HAlign_Left);
			HoldSlot->SetPadding(FMargin(4.f, 6.f, 0.f, 0.f));

			USizeBox* CardSize = MakeSized(WidgetTree, Box, 0.f);
			CardSize->SetMinDesiredWidth(320.f);
			PickupCard = MakePlate(WidgetTree, CardSize, FMargin(16.f, 12.f));
			PickupCard->SetVisibility(ESlateVisibility::Collapsed);
			PlaceOnCanvas(Root, PickupCard, FAnchors(1.f, 0.5f), FVector2D(1.f, 0.5f), FVector2D(-32.f, 0.f));
		}

		// Top-right: the minimap.
		UHudMinimapWidget* Minimap = WidgetTree->ConstructWidget<UHudMinimapWidget>(UHudMinimapWidget::StaticClass());
		PlaceOnCanvas(Root, Minimap, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-UHudMinimapWidget::Margin, UHudMinimapWidget::Margin));

		// Top-left: the frame rate, the same distance in from the corner as the minimap.
		UHudFrameRateWidget* FrameRate = WidgetTree->ConstructWidget<UHudFrameRateWidget>(UHudFrameRateWidget::StaticClass());
		PlaceOnCanvas(Root, FrameRate, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(UHudMinimapWidget::Margin, UHudMinimapWidget::Margin));

		// Bottom-center: level and experience, its bar level with the health bar. Level-ups show in the message plate.
		UHudXPBarWidget* XPBar = WidgetTree->ConstructWidget<UHudXPBarWidget>(UHudXPBarWidget::StaticClass());
		XPBar->OnAnnouncement.BindUObject(this, &UPlayerHUDWidget::HandleMessage);
		PlaceOnCanvas(Root, XPBar, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -38.f));
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

	// A tap and a hold only differ when every slot is full: the tap stashes the loot, the hold takes it in hand.
	const FString Key = BoundKeyName(TEXT("Interact"), TEXT("E"));
	const bool bSlotsFull = Manager->GetWeapons().Num() >= Manager->MaxWeapons;
	const bool bBackpackFull = Manager->GetBackpack().Num() >= Manager->BackpackCapacity;
	FString Hint;
	if (!bSlotsFull)
	{
		Hint = FString::Printf(TEXT("[%s] PICK UP"), *Key);
	}
	else if (!bBackpackFull)
	{
		Hint = FString::Printf(TEXT("[%s] SEND TO BACKPACK\nHOLD [%s] EQUIP, WEAPON IN HAND TO BACKPACK"), *Key, *Key);
	}
	else
	{
		Hint = FString::Printf(TEXT("[%s] SWAP WITH WEAPON IN HAND\nBACKPACK FULL: IT DROPS"), *Key);
	}
	SetTextIfChanged(PickupHint, Hint);

	const float Hold = Manager->GetPickupHoldProgress();
	PickupHoldBar->SetVisibility(Hold > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (Hold > 0.f)
	{
		const int32 Lit = FMath::Clamp(FMath::CeilToInt(Hold * PickupHoldSegmentCount), 0, PickupHoldSegmentCount);
		for (int32 Index = 0; Index < PickupHoldSegments.Num(); ++Index)
		{
			PickupHoldSegments[Index]->SetColorAndOpacity(Index < Lit ? Color::Accent() : Color::SegmentOff());
		}
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
