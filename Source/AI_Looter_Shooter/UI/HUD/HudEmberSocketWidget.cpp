// UHudEmberSocketWidget: the equipped ember power's socket in the weapon column. The ring and the tab are pictures drawn
// once (the weapon slots' measures, HudWeaponSlotsWidgetPictures.cpp); the cooldown's pie, the running power's arc and
// the seconds are painted each frame in NativePaint, over the icon.

#include "UI/HUD/HudEmberSocketWidget.h"
#include "AI_Looter_Shooter.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDefinition.h"
#include "UI/Style/LooterProgressRing.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/WidgetStyle.h"

using namespace LooterUI;

namespace
{
	constexpr float RingDiameter = UHudEmberSocketWidget::RingDiameter;
	constexpr float RingRadius = RingDiameter * 0.5f;
	/** The ready ring's picture reaches past the ring by this much on every side (it's scaled up to match). */
	constexpr float AccentMargin = UHudEmberSocketWidget::AccentMargin;
	/** The glow behind it, across: the slot in hand's. */
	constexpr float GlowSize = 96.f;

	/** The power's icon fits this box inside the disc. */
	constexpr float IconBox = 36.f;

	/** The pie reaches the hairline; its line is drawn as wide as its radius (less a hair at the middle, so it never folds). */
	constexpr float PieRadius = 27.6f;
	constexpr float PieHole = 1.5f;
	/** The running arc lies on the gunmetal ring, over a dark edge this much wider. */
	constexpr float ArcRadius = (UHudEmberSocketWidget::MetalOuter + UHudEmberSocketWidget::MetalInner) * 0.5f;
	constexpr float ArcWidth = 3.2f;
	constexpr float ArcEdge = 2.f;
	/** Chakra Petch's digits sit low in their line: the seconds are lifted to the ring's middle. */
	constexpr float SecondsLift = 1.5f;

	/** Under the ring: the tab, then the name, this far apart. */
	constexpr float TabGap = 6.f;
	constexpr float NameGap = 2.f;

	/** The glow's slow beat while ready: a period this long, between these strengths; steady while it runs. */
	constexpr float GlowPeriod = 2.4f;
	constexpr float GlowLow = 0.18f;
	constexpr float GlowHigh = 0.6f;
	constexpr float GlowActive = 0.7f;

	/** A shake's reach (px) and a dim's depth. */
	constexpr float ShakeReach = 4.f;
	constexpr float DimDepth = 0.45f;
	constexpr float PopGrow = 0.18f;

	/** Cooling down: the icon dimmed (grey dims an Inked icon). */
	const FLinearColor CoolingIcon(0.6f, 0.6f, 0.6f, 0.9f);

	FLinearColor WithAlpha(FLinearColor Tone, float Alpha)
	{
		Tone.A = Alpha;
		return Tone;
	}

	UImage* AddLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush)
	{
		UImage* Image = MakeImage(Tree, Brush);
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(HAlign_Center);
		LayerSlot->SetVerticalAlignment(VAlign_Center);
		return Image;
	}

	/** Strokes an arc of the ring (degrees clockwise from 12 o'clock, as the kit's progress ring counts). */
	void StrokeArc(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, const FVector2f& Center, float Radius,
		float From, float To, const FLinearColor& Tint, float Width)
	{
		if (To - From < 0.05f || Tint.A <= 0.f)
		{
			return;
		}
		TArray<FVector2f> Points;
		LooterRing::AppendArcPoints(Points, Center, Radius, From, To, 3.f);
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Width);
	}
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

FString UHudEmberSocketWidget::SecondsText(float Left)
{
	return Left > 0.f ? FString::FromInt(FMath::Max(1, FMath::CeilToInt(Left))) : FString();
}

float UHudEmberSocketWidget::TabWidthFor(const FString& KeyName)
{
	// One capital in the 12 px type is about 8 px: the plate grows by that per letter past the first, within reason.
	const int32 Letters = FMath::Max(KeyName.Len(), 1);
	return FMath::Min(22.f + 8.f * (Letters - 1), 80.f);
}

float UHudEmberSocketWidget::ShakeOffset(float Elapsed)
{
	if (Elapsed < 0.f || Elapsed >= ShakeSeconds)
	{
		return 0.f;
	}
	const float Alpha = 1.f - Elapsed / ShakeSeconds;
	return FMath::Sin(Elapsed * 55.f) * ShakeReach * Alpha;
}

float UHudEmberSocketWidget::PopScale(float Elapsed)
{
	if (Elapsed < 0.f || Elapsed >= PopSeconds)
	{
		return 1.f;
	}
	return 1.f + PopGrow * FMath::Sin(UE_PI * Elapsed / PopSeconds);
}

// ---------------------------------------------------------------------------
// Building
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UHudEmberSocketWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Column->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Column;
		const FVector2D CircleSize(RingDiameter, RingDiameter);

		// The ring, back to front: the glow, the inner disc (a background), the power's icon, the rim and the gunmetal ring,
		// the cyan hairline, and the ready ring. The pie, the arc and the seconds go over them in NativePaint.
		UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Glow = AddLayer(WidgetTree, Layers, GlowBrush(CircleSize, FLinearColor::White));
		Glow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Glow->SetRenderScale(FVector2D(GlowSize / RingDiameter));
		Glow->SetRenderOpacity(0.f);
		Disc = AddLayer(WidgetTree, Layers, PictureBrush(EHudEmberPicture::Disc));
		Disc->SetColorAndOpacity(WithAlpha(Color::ScreenBg(), 0.82f));
		MarkBackground(Disc);
		FSlateBrush NoIcon;
		NoIcon.DrawAs = ESlateBrushDrawType::NoDrawType;
		NoIcon.ImageSize = FVector2D(IconBox, IconBox);
		Icon = AddLayer(WidgetTree, Layers, NoIcon);
		UImage* Rim = AddLayer(WidgetTree, Layers, PictureBrush(EHudEmberPicture::Rim));
		Rim->SetColorAndOpacity(Color::Ink());
		AddLayer(WidgetTree, Layers, PictureBrush(EHudEmberPicture::Metal));
		AddLayer(WidgetTree, Layers, PictureBrush(EHudEmberPicture::Hairline));
		AccentRing = AddLayer(WidgetTree, Layers, PictureBrush(EHudEmberPicture::Accent));
		AccentRing->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		AccentRing->SetRenderScale(FVector2D((RingDiameter + AccentMargin * 2.f) / RingDiameter));
		AccentRing->SetColorAndOpacity(Color::Accent());
		Column->AddChildToVerticalBox(MakeSized(WidgetTree, Layers, RingDiameter, RingDiameter))->SetHorizontalAlignment(HAlign_Center);

		// The key tab: dark glass (a background), its cyan edge, and the bound key, as the grenade's.
		UOverlay* Tab = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		const float Width = TabWidthFor(TEXT("Q"));
		TabGlass = AddLayer(WidgetTree, Tab, TabBrush(Width, false));
		TabGlass->SetColorAndOpacity(WithAlpha(Color::ScreenBg(), 0.85f));
		MarkBackground(TabGlass);
		TabEdge = AddLayer(WidgetTree, Tab, TabBrush(Width, true));
		TabEdge->SetColorAndOpacity(WithAlpha(Color::Hairline(), 0.7f));
		KeyText = MakeFloatingText(WidgetTree, 12, Color::Text(), 0, ETextJustify::Center);
		UOverlaySlot* KeySlot = Tab->AddChildToOverlay(KeyText);
		KeySlot->SetHorizontalAlignment(HAlign_Center);
		KeySlot->SetVerticalAlignment(VAlign_Center);
		// Chakra Petch's capitals sit low in its line: lifted to the plate's middle, as the slots' numbers are.
		KeyText->SetRenderTranslation(FVector2D(0.f, -3.f));
		TabBox = MakeSized(WidgetTree, Tab, Width, TabHeight);
		UVerticalBoxSlot* TabSlot = Column->AddChildToVerticalBox(TabBox);
		TabSlot->SetHorizontalAlignment(HAlign_Center);
		TabSlot->SetPadding(FMargin(0.f, TabGap, 0.f, 0.f));

		// The power's name in small floating type.
		NameText = MakeFloatingText(WidgetTree, NameSize, Color::Text(), 120, ETextJustify::Center);
		UVerticalBoxSlot* NameSlot = Column->AddChildToVerticalBox(NameText);
		NameSlot->SetHorizontalAlignment(HAlign_Center);
		NameSlot->SetPadding(FMargin(0.f, NameGap, 0.f, 0.f));

		// Hidden until a power shows; the first update paints everything.
		bShowing = false;
		ShownId = NAME_None;
		ShownKey.Reset();
		PieShare = 0.f;
		ArcShare = 0.f;
		PieWords.Reset();
		ShakeLeft = 0.f;
		DimLeft = 0.f;
		PopLeft = 0.f;
	}
	TSharedRef<SWidget> Built = Super::RebuildWidget();
	SetVisibility(bShowing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	return Built;
}

void UHudEmberSocketWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// The pie and the arc move every frame a power cools down or runs (and it's collapsed with none, when that costs nothing).
	ForceVolatile(true);
	// The shake and the pop move it about the ring's middle.
	SetRenderTransformPivot(FVector2D(0.5f, 0.3f));
}

// ---------------------------------------------------------------------------
// Frame by frame
// ---------------------------------------------------------------------------

void UHudEmberSocketWidget::Update(const UEmberComponent* Embers, const FString& KeyName, float DeltaTime)
{
	if (!Icon || !NameText || !KeyText)
	{
		return;
	}
	const UEmberDefinition* Power = Embers ? Embers->GetEquipped() : nullptr;
	const bool bShow = Power != nullptr;
	if (bShow != bShowing)
	{
		bShowing = bShow;
		SetVisibility(bShowing ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!bShowing)
	{
		ShownId = NAME_None;
		PieShare = 0.f;
		ArcShare = 0.f;
		PieWords.Reset();
		return;
	}

	// Another power equipped: its icon, name and colour.
	bool bRepaint = false;
	if (Power->Id != ShownId)
	{
		ShownId = Power->Id;
		PowerColor = Power->Color;
		const FInkedIcon* Inked = FindInkedIcon(Power->IconName);
		if (Inked)
		{
			Icon->SetBrush(InkedIconBrush(Power->IconName, *Inked, FVector2D(IconBox, IconBox)));
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Ember socket: no Inked icon named %s (Art/Icons/InkedIcons.py)."), *Power->IconName.ToString());
		}
		Icon->SetVisibility(Inked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		NameText->SetText(FText::FromString(Power->DisplayName.ToString().ToUpper()));
		Glow->SetColorAndOpacity(WithAlpha(PowerColor, 1.f));
		bRepaint = true;
	}
	if (KeyName != ShownKey)
	{
		PaintKey(KeyName);
	}

	const EEmberState State = Embers->GetState();
	if (State != ShownState || bRepaint)
	{
		ShownState = State;
		PaintState();
	}
	PieShare = State == EEmberState::Cooldown ? FMath::Clamp(Embers->GetCooldownFraction(), 0.f, 1.f) : 0.f;
	PieWords = State == EEmberState::Cooldown ? SecondsText(Embers->GetCooldownLeft()) : FString();
	ArcShare = State == EEmberState::Active ? FMath::Clamp(Embers->GetActiveFraction(), 0.f, 1.f) : 0.f;
	RunMotion(DeltaTime);
}

void UHudEmberSocketWidget::PaintKey(const FString& KeyName)
{
	ShownKey = KeyName;
	KeyText->SetText(FText::FromString(KeyName));
	const float Width = TabWidthFor(KeyName);
	TabGlass->SetBrush(TabBrush(Width, false));
	TabEdge->SetBrush(TabBrush(Width, true));
	TabBox->SetWidthOverride(Width);
}

void UHudEmberSocketWidget::PaintState()
{
	const bool bReady = ShownState == EEmberState::Ready;
	const bool bActive = ShownState == EEmberState::Active;
	AccentRing->SetVisibility(bReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	AccentRing->SetColorAndOpacity(Color::Accent());
	Icon->SetColorAndOpacity(bReady || bActive ? FLinearColor::White : CoolingIcon);
	// The name lights in the power's colour while it runs, dims while it cools.
	NameText->SetColorAndOpacity(FSlateColor(bActive ? PowerColor : (bReady ? Color::Text() : Color::TextDim())));
	Glow->SetRenderOpacity(bActive ? GlowActive : 0.f);
	GlowClock = 0.f;
}

void UHudEmberSocketWidget::RunMotion(float DeltaTime)
{
	// The ready glow breathes slowly, from dim, so it comes up after the pop rather than jumping.
	if (ShownState == EEmberState::Ready)
	{
		GlowClock += DeltaTime;
		const float Beat = 0.5f - 0.5f * FMath::Cos(UE_TWO_PI * GlowClock / GlowPeriod);
		float Strength = FMath::Lerp(GlowLow, GlowHigh, Beat);
		if (PopLeft > 0.f)
		{
			Strength = FMath::Max(Strength, PopLeft / PopSeconds);
		}
		Glow->SetRenderOpacity(Strength);
	}

	const bool bMoving = ShakeLeft > 0.f || PopLeft > 0.f;
	ShakeLeft = FMath::Max(ShakeLeft - DeltaTime, 0.f);
	PopLeft = FMath::Max(PopLeft - DeltaTime, 0.f);
	if (bMoving)
	{
		FWidgetTransform Transform;
		Transform.Translation = FVector2D(ShakeLeft > 0.f ? ShakeOffset(ShakeSeconds - ShakeLeft) : 0.f, 0.f);
		Transform.Scale = FVector2D(PopLeft > 0.f ? PopScale(PopSeconds - PopLeft) : 1.f);
		SetRenderTransform(Transform);
		// The ready ring flashes white as it pops, settling to orange.
		AccentRing->SetColorAndOpacity(FMath::Lerp(Color::Accent(), FLinearColor::White, PopLeft / PopSeconds));
	}

	if (DimLeft > 0.f)
	{
		DimLeft = FMath::Max(DimLeft - DeltaTime, 0.f);
		// Down at once and back up softly.
		SetRenderOpacity(1.f - DimDepth * (DimLeft / DimSeconds));
	}
}

void UHudEmberSocketWidget::Flash(EEmberDenied Why)
{
	switch (Why)
	{
	case EEmberDenied::None:
		return;
	case EEmberDenied::Cooldown:
	case EEmberDenied::Active:
		ShakeLeft = ShakeSeconds;
		break;
	default:
		DimLeft = DimSeconds;
		SetRenderOpacity(1.f - DimDepth);
		break;
	}
}

void UHudEmberSocketWidget::NotifyState(EEmberState NewState)
{
	// Usable again: it pops (the state's own repaint follows in Update).
	if (NewState == EEmberState::Ready && bShowing && ShownState != EEmberState::Ready)
	{
		PopLeft = PopSeconds;
	}
}

int32 UHudEmberSocketWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Under = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!bShowing || (PieShare <= 0.f && ArcShare <= 0.f))
	{
		return Under;
	}
	// The cluster's fade and the socket's own dim arrive in the style, as they do for its images.
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	if (Opacity <= 0.f)
	{
		return Under;
	}
	// The ring stands centred at the top of the socket (its name may be wider than it).
	const FVector2f Size(AllottedGeometry.GetLocalSize());
	const FVector2f Center(Size.X * 0.5f, RingRadius);
	int32 Layer = Under + 1;

	if (PieShare > 0.f)
	{
		// The part still to wait, ahead of the hand going round clockwise from 12 o'clock: a line as wide as the radius.
		const float Width = PieRadius - PieHole;
		const FLinearColor Pie = WithAlpha(Color::Track(), 0.8f * Opacity);
		StrokeArc(OutDrawElements, Layer, AllottedGeometry, Center, PieHole + Width * 0.5f, (1.f - PieShare) * 360.f, 360.f, Pie, Width);
		++Layer;
		if (!PieWords.IsEmpty() && FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
		{
			FSlateFontInfo Font = FloatingFont(SecondsSize);
			Font.OutlineSettings.OutlineSize = 2;
			const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			const FVector2f TextSize(Measure->Measure(PieWords, Font));
			FSlateDrawElement::MakeText(OutDrawElements, Layer,
				AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Center - TextSize * 0.5f - FVector2f(0.f, SecondsLift))),
				PieWords, Font, ESlateDrawEffect::None, WithAlpha(FLinearColor::White, Opacity));
			++Layer;
		}
	}
	if (ArcShare > 0.f)
	{
		// The running power's arc runs down round the ring from 12 o'clock, over a dark edge so it reads on the metal.
		StrokeArc(OutDrawElements, Layer, AllottedGeometry, Center, ArcRadius, 0.f, ArcShare * 360.f,
			WithAlpha(Color::Outline(), Color::Outline().A * Opacity), ArcWidth + ArcEdge);
		StrokeArc(OutDrawElements, Layer + 1, AllottedGeometry, Center, ArcRadius, 0.f, ArcShare * 360.f, WithAlpha(PowerColor, Opacity), ArcWidth);
		Layer += 2;
	}
	return Layer;
}
