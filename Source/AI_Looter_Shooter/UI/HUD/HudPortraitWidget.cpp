#include "UI/HUD/HudPortraitWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/HudPortraitData.inl"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

using namespace LooterUI;

namespace
{
	/** The SVG at the spec's 0.64 px a unit: its 200-unit diamond fills the 128 px window exactly. */
	constexpr float PortraitScale = static_cast<float>(UHudPortraitWidget::WindowSize / HudPortraitData::WindowUnits);
	/** The pictures' textures are drawn at twice the HUD size, so they stay crisp (and land 1:1 at 4K). */
	constexpr float PortraitPixelsPerUnit = PortraitScale * 2.f;
	/** The window's clip: a square turned 45 degrees, whose corners are the diamond's tips. */
	constexpr float ClipSide = UHudPortraitWidget::WindowSize / UE_SQRT_2;

	// Calm (the mockup's timings).
	constexpr float BlinkPeriod = 5.2f;
	/** Shares of the blink's period: the eyes close from the first to the second, and are open again by the third. */
	constexpr float BlinkStart = 0.95f;
	constexpr float BlinkShut = 0.965f;
	constexpr float BlinkEnd = 0.98f;
	/** How tall the eyes are when shut, as a share of open. */
	constexpr float BlinkSquash = 0.08f;
	constexpr float BreathPeriod = 4.2f;
	/** How far the bust rises at the top of a breath (px). */
	constexpr float BreathRise = 1.4f;
	/** A breath repaints only once it has moved this far (px), so the slow bob doesn't redraw the bust every frame. */
	constexpr float BreathStep = 0.05f;

	// A hit.
	constexpr float ShakeSeconds = 0.32f;
	/** The shake's keys (px), evenly spaced over its time: the mockup's flinch, scaled to reach the spec's 4 px. */
	constexpr float ShakeKeys[][2] = { { 0.f, 0.f }, { -4.f, 1.6f }, { 3.2f, -1.6f }, { -2.4f, 0.8f }, { 1.6f, 0.f }, { 0.f, 0.f } };
	constexpr int32 ShakeKeyCount = static_cast<int32>(UE_ARRAY_COUNT(ShakeKeys));
	constexpr float FlashSeconds = 0.45f;
	constexpr float FlashStart = 0.6f;
	constexpr float SquintSeconds = 0.6f;
	/** The squint holds for this share of its time; over the rest it fades back into the calm eyes. */
	constexpr float SquintHold = 0.7f;

	// Low health.
	constexpr float PulsePeriod = 0.9f;
	constexpr float PulseLow = 0.1f;
	constexpr float PulseHigh = 0.34f;

	// A level-up.
	constexpr float FlareSeconds = 1.8f;
	/** The flare holds for this share of its time, then fades. */
	constexpr float FlareHold = 0.6f;

	/** Fast, then settling (close to CSS's ease-out, which the mockup uses). */
	float PortraitEaseOut(float T)
	{
		return 1.f - FMath::Square(1.f - FMath::Clamp(T, 0.f, 1.f));
	}

	/** A smooth swing from 0 up to 1 and back over one period (the breath, the low-health beat). */
	float PortraitSwing(float Clock, float Period)
	{
		return 0.5f - 0.5f * FMath::Cos(UE_TWO_PI * Clock / Period);
	}

	/** Moves a reaction's clock on, and stops it once it has run its length. */
	void AdvanceReaction(float& Time, float DeltaTime, float Length)
	{
		if (Time >= 0.f)
		{
			Time += DeltaTime;
			if (Time >= Length)
			{
				Time = -1.f;
			}
		}
	}

	/** A reaction that holds full for a share of its time, then fades out (the squint, the flare). */
	float HoldThenFade(float Time, float Length, float Hold)
	{
		if (Time < 0.f)
		{
			return 0.f;
		}
		const float Share = Time / Length;
		return Share < Hold ? 1.f : 1.f - PortraitEaseOut((Share - Hold) / (1.f - Hold));
	}

	/** The shake's offset this far into it, straight between its keys. */
	FVector2D ShakeAt(float Time)
	{
		if (Time < 0.f)
		{
			return FVector2D::ZeroVector;
		}
		const float Position = FMath::Clamp(Time / ShakeSeconds, 0.f, 1.f) * (ShakeKeyCount - 1);
		const int32 Key = FMath::Min(FMath::FloorToInt32(Position), ShakeKeyCount - 2);
		const float Alpha = Position - Key;
		return FVector2D(FMath::Lerp(ShakeKeys[Key][0], ShakeKeys[Key + 1][0], Alpha), FMath::Lerp(ShakeKeys[Key][1], ShakeKeys[Key + 1][1], Alpha));
	}

	/** How tall the open eyes are at this point of the blink's loop (1 open, BlinkSquash shut). */
	float BlinkAt(float Clock)
	{
		const float Phase = Clock / BlinkPeriod;
		if (Phase < BlinkStart || Phase >= BlinkEnd)
		{
			return 1.f;
		}
		return Phase < BlinkShut
			? FMath::Lerp(1.f, BlinkSquash, (Phase - BlinkStart) / (BlinkShut - BlinkStart))
			: FMath::Lerp(BlinkSquash, 1.f, (Phase - BlinkShut) / (BlinkEnd - BlinkShut));
	}

	/** Sets a group's opacity when it changed, hiding it at nothing so it costs no drawing. */
	void ShowOpacity(UWidget* Widget, float Opacity, float& Shown)
	{
		if (!Widget || Opacity == Shown)
		{
			return;
		}
		Shown = Opacity;
		Widget->SetRenderOpacity(Opacity);
		Widget->SetVisibility(Opacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	void PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size)
	{
		UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetSize(Size);
	}

	/** A layer of the window's content that moves or fades as one, covering the whole window square. */
	UCanvasPanel* AddPortraitGroup(UWidgetTree* Tree, UCanvasPanel* Parent)
	{
		UCanvasPanel* Group = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		Group->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlaceOnCanvas(Parent, Group, FVector2D::ZeroVector, FVector2D(UHudPortraitWidget::WindowSize));
		return Group;
	}

	/** One of the portrait's pictures, where the SVG puts it. */
	UImage* AddPortraitPicture(UWidgetTree* Tree, UCanvasPanel* Parent, FName Name, const HudPortraitData::FPicture& Picture)
	{
		const FVector2D Size = Picture.Icon.ViewBox * PortraitScale;
		UImage* Image = MakeImage(Tree, PaintedIconBrush(Name, Picture.Icon, PortraitPixelsPerUnit, Size));
		PlaceOnCanvas(Parent, Image, Picture.Origin * PortraitScale, Size);
		return Image;
	}

	/** An eye group's soft glows, behind its picture. */
	void AddPortraitGlows(UWidgetTree* Tree, UCanvasPanel* Parent, const TArray<HudPortraitData::FGlow>& Glows)
	{
		for (const HudPortraitData::FGlow& Glow : Glows)
		{
			const FVector2D Size = Glow.Size * PortraitScale;
			UImage* Image = MakeImage(Tree, GlowBrush(Size, Glow.Color.CopyWithNewOpacity(Glow.Opacity)));
			PlaceOnCanvas(Parent, Image, Glow.Center * PortraitScale - Size * 0.5, Size);
		}
	}
}

TSharedRef<SWidget> UHudPortraitWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Window = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USizeBox* Root = MakeSized(WidgetTree, Window, WindowSize, WindowSize);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		// The diamond clip: a square turned 45 degrees that clips to its bounds. Slate clips a turned box with the
		// stencil buffer (a clipping zone that isn't axis-aligned), so the clip is the diamond itself; the frame's ink
		// edge covers the clip's hard edge. Inside, the content is turned back upright round the same middle.
		UCanvasPanel* Turned = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		USizeBox* ClipBox = MakeSized(WidgetTree, Turned, ClipSide, ClipSide);
		ClipBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		ClipBox->SetRenderTransformAngle(45.f);
		ClipBox->SetClipping(EWidgetClipping::ClipToBounds);
		UOverlaySlot* ClipSlot = Window->AddChildToOverlay(ClipBox);
		ClipSlot->SetHorizontalAlignment(HAlign_Center);
		ClipSlot->SetVerticalAlignment(VAlign_Center);

		UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		Content->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Content->SetRenderTransformAngle(-45.f);
		UCanvasPanelSlot* ContentSlot = Turned->AddChildToCanvas(Content);
		ContentSlot->SetAutoSize(false);
		ContentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ContentSlot->SetPosition(FVector2D(ClipSide * 0.5f));
		ContentSlot->SetSize(FVector2D(WindowSize));

		// The glass is the window's background: it fades with the UI transparency setting. It reaches past the window
		// far enough that the shake never shows its edge.
		UCanvasPanel* FaceGroup = AddPortraitGroup(WidgetTree, Content);
		Face = FaceGroup;
		MarkBackground(AddPortraitPicture(WidgetTree, FaceGroup, TEXT("HudPortraitGlass"), HudPortraitData::Glass()));

		UCanvasPanel* BodyGroup = AddPortraitGroup(WidgetTree, FaceGroup);
		Body = BodyGroup;
		AddPortraitPicture(WidgetTree, BodyGroup, TEXT("HudPortraitBust"), HudPortraitData::Bust());

		// The calm eyes: the eyes and their glows blink (squashing toward their middle), the brows stay put.
		UCanvasPanel* CalmGroup = AddPortraitGroup(WidgetTree, BodyGroup);
		CalmEyes = CalmGroup;
		UCanvasPanel* BlinkGroup = AddPortraitGroup(WidgetTree, CalmGroup);
		BlinkGroup->SetRenderTransformPivot(FVector2D(0.5f, static_cast<float>(HudPortraitData::BlinkCenterY / HudPortraitData::WindowUnits)));
		BlinkingEyes = BlinkGroup;
		AddPortraitGlows(WidgetTree, BlinkGroup, HudPortraitData::EyesOpenGlows());
		AddPortraitPicture(WidgetTree, BlinkGroup, TEXT("HudPortraitEyesOpen"), HudPortraitData::EyesOpen());
		AddPortraitPicture(WidgetTree, CalmGroup, TEXT("HudPortraitBrowsCalm"), HudPortraitData::BrowsCalm());

		// The squint and the flare, hidden until a reaction shows them; the flare draws over whichever eyes show.
		UCanvasPanel* HurtGroup = AddPortraitGroup(WidgetTree, BodyGroup);
		HurtEyes = HurtGroup;
		AddPortraitGlows(WidgetTree, HurtGroup, HudPortraitData::EyesHurtGlows());
		AddPortraitPicture(WidgetTree, HurtGroup, TEXT("HudPortraitEyesHurt"), HudPortraitData::EyesHurt());
		UCanvasPanel* FlareGroup = AddPortraitGroup(WidgetTree, BodyGroup);
		FlareEyes = FlareGroup;
		AddPortraitGlows(WidgetTree, FlareGroup, HudPortraitData::EyesFlareGlows());
		AddPortraitPicture(WidgetTree, FlareGroup, TEXT("HudPortraitEyesFlare"), HudPortraitData::EyesFlare());

		// The window's red, over the content but not shaking with it (a plain square: the clip makes it the diamond).
		Flash = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
		Flash->SetColorAndOpacity(Color::Hurt().CopyWithNewOpacity(0.f));
		Flash->SetVisibility(ESlateVisibility::Hidden);
		PlaceOnCanvas(Content, Flash, FVector2D::ZeroVector, FVector2D(WindowSize));

		// Start at rest: calm eyes, nothing flashing.
		ShownShake = FVector2D::ZeroVector;
		ShownBreath = 0.f;
		ShownBlink = 1.f;
		ShownCalm = 1.f;
		ShownHurt = -1.f;
		ShownFlare = -1.f;
		ShownFlash = 0.f;
		ShowOpacity(HurtEyes, 0.f, ShownHurt);
		ShowOpacity(FlareEyes, 0.f, ShownFlare);
	}
	return Super::RebuildWidget();
}

void UHudPortraitWidget::PlayHit()
{
	ShakeTime = 0.f;
	FlashTime = 0.f;
	SquintTime = 0.f;
}

void UHudPortraitWidget::SetLowHealth(bool bLow)
{
	if (bLow == bLowHealth)
	{
		return;
	}
	bLowHealth = bLow;
	if (bLow)
	{
		// The beat starts at its faintest, so the pulse eases in.
		PulseClock = 0.f;
	}
	else
	{
		// Out of danger: the held squint lets go as a hit's does, over the end of its time.
		SquintTime = SquintSeconds * SquintHold;
	}
}

void UHudPortraitWidget::PlayLevelUp()
{
	FlareTime = 0.f;
}

void UHudPortraitWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Face || !Body || !CalmEyes || !HurtEyes || !FlareEyes || !BlinkingEyes || !Flash)
	{
		return;
	}

	BlinkClock = FMath::Fmod(BlinkClock + InDeltaTime, BlinkPeriod);
	BreathClock = FMath::Fmod(BreathClock + InDeltaTime, BreathPeriod);
	PulseClock = bLowHealth ? FMath::Fmod(PulseClock + InDeltaTime, PulsePeriod) : 0.f;
	AdvanceReaction(ShakeTime, InDeltaTime, ShakeSeconds);
	AdvanceReaction(FlashTime, InDeltaTime, FlashSeconds);
	AdvanceReaction(SquintTime, InDeltaTime, SquintSeconds);
	AdvanceReaction(FlareTime, InDeltaTime, FlareSeconds);

	// The shake moves everything in the window; the breath lifts the bust and eyes (up is negative on screen).
	const FVector2D Shake = ShakeAt(ShakeTime);
	if (Shake != ShownShake)
	{
		ShownShake = Shake;
		Face->SetRenderTranslation(Shake);
	}
	const float Breath = -BreathRise * PortraitSwing(BreathClock, BreathPeriod);
	if (FMath::Abs(Breath - ShownBreath) >= BreathStep)
	{
		ShownBreath = Breath;
		Body->SetRenderTranslation(FVector2D(0.f, Breath));
	}

	const float Blink = BlinkAt(BlinkClock);
	if (Blink != ShownBlink)
	{
		ShownBlink = Blink;
		BlinkingEyes->SetRenderScale(FVector2D(1.f, Blink));
	}

	// The squint shows instead of the calm eyes (held at low health); the flare shows over either.
	const float Squint = bLowHealth ? 1.f : HoldThenFade(SquintTime, SquintSeconds, SquintHold);
	ShowOpacity(HurtEyes, Squint, ShownHurt);
	ShowOpacity(CalmEyes, 1.f - Squint, ShownCalm);
	ShowOpacity(FlareEyes, HoldThenFade(FlareTime, FlareSeconds, FlareHold), ShownFlare);

	// The window's red: a hit's flash and the low-health pulse, the one over the other.
	const float HitRed = FlashTime >= 0.f ? FlashStart * (1.f - PortraitEaseOut(FlashTime / FlashSeconds)) : 0.f;
	const float LowRed = bLowHealth ? FMath::Lerp(PulseLow, PulseHigh, PortraitSwing(PulseClock, PulsePeriod)) : 0.f;
	const float Red = 1.f - (1.f - HitRed) * (1.f - LowRed);
	if (Red != ShownFlash)
	{
		ShownFlash = Red;
		Flash->SetColorAndOpacity(Color::Hurt().CopyWithNewOpacity(Red));
		const ESlateVisibility Wanted = Red > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
		if (Flash->GetVisibility() != Wanted)
		{
			Flash->SetVisibility(Wanted);
		}
	}
}
