// UHudFlaskWidget: the soul flask's row under the player frame (the key tab, the flask's Inked icon, three pips) and its
// flashes; its plain rules and its icon are HudFlaskWidgetRules.cpp's. PlayerHUDWidget places it
// (PlayerHUDWidgetLayout.cpp) and drives it (PlayerHUDWidgetFlask.cpp).

#include "UI/HUD/HudFlaskWidget.h"
#include "Player/PlayerFlaskComponent.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** The row: the key tab, the icon, the pips, then "Health full", each this far after the last. */
	constexpr float TabGap = 8.f;
	constexpr float IconGap = 6.f;
	constexpr float WordsGap = 12.f;
	/** The icon's glow: this much bigger than the icon, this strong while the heal flows, and at a charge's pop. */
	constexpr float GlowScale = 1.5f;
	constexpr float GlowHealing = 0.55f;
	constexpr float GlowFilled = 0.8f;
	/** Its rise and fall (FInterpTo speeds): in quickly with the gulp, out slower, like light settling. */
	constexpr float GlowInSpeed = 12.f;
	constexpr float GlowOutSpeed = 4.f;
	/** A pip's ink edge, and its fill's inset inside it. */
	constexpr float PipEdge = 1.5f;
	constexpr float PipInset = 1.5f;
	/**
	 * An empty pip reads as an empty glass slot, not a dark smudge: a pale hairline just inside its ink edge, this strong
	 * (the fill covers it as it rises).
	 */
	constexpr float GlassLineOpacity = 0.6f;
	constexpr float GlassLineWidth = 1.f;
	/** The soul's surface on a fill: a bright line this tall, the soul-light this far toward white. */
	constexpr float SurfaceHeight = 2.f;
	constexpr float SurfaceWhite = 0.55f;
	/** A pip's partial fill (soul still coming) is this see-through, so a full charge stands out. */
	constexpr float PartialOpacity = 0.75f;

	/** How far each flash pops the row (a filled charge's pip pops on its own, this much more). */
	constexpr float DrunkDip = 0.06f;
	constexpr float FilledPop = 0.18f;
	constexpr float FilledPipPop = 0.4f;
	/** A respawn's top-up: every pip's white flash at this strength. */
	constexpr float RefilledFlash = 0.55f;
	/** "Health full": in this fast, out over this long at its end. */
	constexpr float WordsIn = 0.1f;
	constexpr float WordsOut = 0.4f;

	/** Full strength this long after something happens; otherwise, with a full flask, the HUD's idle opacity. */
	constexpr float ActivityHold = 3.f;
	constexpr float IdleOpacity = 0.6f;
	constexpr float OpacitySpeed = 5.f;

	/** None left: the icon dimmed (grey dims an Inked icon). */
	const FLinearColor EmptyIcon(0.5f, 0.5f, 0.5f, 0.8f);

	FLinearColor WithAlpha(FLinearColor Tone, float Alpha)
	{
		Tone.A = Alpha;
		return Tone;
	}

	float FlashLength(EHudFlaskFlash Kind)
	{
		switch (Kind)
		{
		case EHudFlaskFlash::Drunk: return UHudFlaskWidget::DrunkSeconds;
		case EHudFlaskFlash::Filled: return UHudFlaskWidget::FilledSeconds;
		case EHudFlaskFlash::Refilled: return UHudFlaskWidget::RefilledSeconds;
		case EHudFlaskFlash::DeniedEmpty: return UHudFlaskWidget::DeniedSeconds;
		default: return UHudFlaskWidget::WordsSeconds;
		}
	}

	/** The key tab: a small plate with its top-left and bottom-right corners cut (the grenade counter's and the slots'). */
	TArray<FVector2D> TabOutline()
	{
		const float W = UHudFlaskWidget::TabWidth;
		const float H = UHudFlaskWidget::TabHeight;
		return { FVector2D(3.f, 0.f), FVector2D(W, 0.f), FVector2D(W, H - 3.f), FVector2D(W - 3.f, H), FVector2D(0.f, H), FVector2D(0.f, 3.f) };
	}

	const FVectorIcon& TabPlateIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Plate;
			Plate.ViewBox = FVector2D(UHudFlaskWidget::TabWidth, UHudFlaskWidget::TabHeight);
			Plate.Fills.Add(TabOutline());
			return Plate;
		}();
		return Icon;
	}

	const FVectorIcon& TabEdgeIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Edge;
			Edge.ViewBox = FVector2D(UHudFlaskWidget::TabWidth, UHudFlaskWidget::TabHeight);
			Edge.StrokeWidth = 1.f;
			TArray<FVector2D> Line = TabOutline();
			// Pulled in half the line's width, so the stroke stays inside the picture.
			for (FVector2D& Point : Line)
			{
				Point = FVector2D(FMath::Clamp(Point.X, 0.5f, UHudFlaskWidget::TabWidth - 0.5f), FMath::Clamp(Point.Y, 0.5f, UHudFlaskWidget::TabHeight - 0.5f));
			}
			const FVector2D First = Line[0];
			Line.Add(First);
			Edge.Strokes.Add(Line);
			return Edge;
		}();
		return Icon;
	}

	UImage* AddLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush, bool bFill = false, float Inset = 0.f)
	{
		UImage* Image = MakeImage(Tree, Brush);
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(bFill ? HAlign_Fill : HAlign_Center);
		LayerSlot->SetVerticalAlignment(bFill ? VAlign_Fill : VAlign_Center);
		LayerSlot->SetPadding(FMargin(Inset));
		return Image;
	}
}

// ---------------------------------------------------------------------------
// The tree
// ---------------------------------------------------------------------------

TSharedRef<SWidget> UHudFlaskWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Line;
		Row = Line;

		// The key tab: dark glass (a background), its edge, and the bound key.
		UOverlay* Tab = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		const FVector2D TabSize(TabWidth, TabHeight);
		MarkBackground(AddLayer(WidgetTree, Tab, IconBrush(TEXT("HudFlaskTabPlate"), TabPlateIcon(), 2.f, TabSize, WithAlpha(Color::ScreenBg(), 0.85f))));
		AddLayer(WidgetTree, Tab, IconBrush(TEXT("HudFlaskTabEdge"), TabEdgeIcon(), 2.f, TabSize, WithAlpha(Color::Hairline(), 0.7f)));
		KeyText = MakeFloatingText(WidgetTree, KeySize, Color::Text(), 0, ETextJustify::Center);
		UOverlaySlot* KeySlot = Tab->AddChildToOverlay(KeyText);
		KeySlot->SetHorizontalAlignment(HAlign_Center);
		KeySlot->SetVerticalAlignment(VAlign_Center);
		// Chakra Petch's capitals sit low in its line: lifted to the plate's middle, as the grenade counter's key is.
		KeyText->SetRenderTranslation(FVector2D(0.f, -3.f));
		UHorizontalBoxSlot* TabSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Tab, TabWidth, TabHeight));
		TabSlot->SetVerticalAlignment(VAlign_Center);
		TabSlot->SetPadding(FMargin(0.f, 0.f, TabGap, 0.f));

		// The flask's icon over its soft green glow (feedback, not a background: it shows only while the heal flows or a
		// charge fills). The overlay is the icon's size; the glow spills past it.
		UOverlay* IconStack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		const FVector2D GlowSize(IconSize * GlowScale, IconSize * GlowScale);
		IconGlow = AddLayer(WidgetTree, IconStack, GlowBrush(GlowSize, Color::Heal()));
		IconGlow->SetRenderOpacity(0.f);
		IconGlow->SetVisibility(ESlateVisibility::Collapsed);
		IconImage = AddLayer(WidgetTree, IconStack, InkedIconBrush(TEXT("SoulFlask"), Icon(), FVector2D(IconSize, IconSize)));
		// It tips about its foot, as a flask raised to the mouth does.
		IconImage->SetRenderTransformPivot(FVector2D(0.5f, 0.85f));
		UHorizontalBoxSlot* IconSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, IconStack, IconSize, IconSize));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, IconGap, 0.f));

		// The pips: each a bar leaning with the HUD's others, its empty track (a background) with a pale glass line inside
		// its edge, its fill rising from its foot with the soul's bright surface on it, its white flash, and its ink edge
		// over all.
		UHorizontalBox* PipRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PipBoxes.Reset();
		PipTracks.Reset();
		PipFills.Reset();
		PipSurfaces.Reset();
		PipFlashes.Reset();
		for (int32 Index = 0; Index < MaxPips; ++Index)
		{
			UOverlay* Pip = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			UImage* Track = AddLayer(WidgetTree, Pip, RectBrush(FLinearColor::White), true);
			Track->SetColorAndOpacity(WithAlpha(Color::Track(), 0.85f));
			MarkBackground(Track);
			UImage* Glass = AddLayer(WidgetTree, Pip, RectBrush(FLinearColor::Transparent, Color::SaltLine(), GlassLineWidth), true, PipInset);
			Glass->SetColorAndOpacity(WithAlpha(FLinearColor::White, GlassLineOpacity));
			UImage* Fill = AddLayer(WidgetTree, Pip, RectBrush(FLinearColor::White), true, PipInset);
			Fill->SetColorAndOpacity(Color::Heal());
			Fill->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
			Fill->SetRenderScale(FVector2D(1.f, 0.01f));
			Fill->SetVisibility(ESlateVisibility::Collapsed);
			// The surface: a thin bright line at the pip's foot, moved up to the fill's top (PaintPips).
			USizeBox* Surface = MakeSized(WidgetTree,
				MakeImage(WidgetTree, RectBrush(FMath::Lerp(Color::Heal(), FLinearColor::White, SurfaceWhite))), 0.f, SurfaceHeight);
			UOverlaySlot* SurfaceSlot = Pip->AddChildToOverlay(Surface);
			SurfaceSlot->SetHorizontalAlignment(HAlign_Fill);
			SurfaceSlot->SetVerticalAlignment(VAlign_Bottom);
			SurfaceSlot->SetPadding(FMargin(PipInset));
			Surface->SetVisibility(ESlateVisibility::Collapsed);
			UImage* FlashImage = AddLayer(WidgetTree, Pip, RectBrush(FLinearColor::White), true, PipInset);
			FlashImage->SetRenderOpacity(0.f);
			AddLayer(WidgetTree, Pip, RectBrush(FLinearColor::Transparent, Color::Ink(), PipEdge), true);

			USizeBox* PipBox = MakeSized(WidgetTree, Pip, PipWidth, PipHeight);
			PipBox->SetRenderShear(FVector2D(-PipSlant, 0.f));
			PipBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			UHorizontalBoxSlot* PipSlot = PipRow->AddChildToHorizontalBox(PipBox);
			PipSlot->SetVerticalAlignment(VAlign_Center);
			PipSlot->SetPadding(FMargin(Index > 0 ? PipGap : 0.f, 0.f, 0.f, 0.f));

			PipBoxes.Add(PipBox);
			PipTracks.Add(Track);
			PipFills.Add(Fill);
			PipSurfaces.Add(Surface);
			PipFlashes.Add(FlashImage);
		}
		Line->AddChildToHorizontalBox(PipRow)->SetVerticalAlignment(VAlign_Center);

		// "Health full", after the pips: a press refused at full health says so here, once.
		WordsText = MakeFloatingText(WidgetTree, WordsSize, Color::Text(), 100, ETextJustify::Left);
		WordsText->SetText(NSLOCTEXT("HudFlask", "HealthFull", "Health full"));
		WordsText->SetVisibility(ESlateVisibility::Hidden);
		UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(WordsText);
		WordsSlot->SetVerticalAlignment(VAlign_Center);
		WordsSlot->SetPadding(FMargin(WordsGap, 0.f, 0.f, 0.f));

		Line->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		Line->SetVisibility(ESlateVisibility::Collapsed);
		ShownLevels.Init(0.f, MaxPips);
		// Nothing painted yet: the first paint sets everything.
		PaintedLevels.Init(-1.f, MaxPips);
		PaintedWhite.Init(-1.f, MaxPips);
		PaintedPop.Init(-1.f, MaxPips);
		PaintedRed = -1.f;
		PaintedAngle = -1000.f;
		PaintedGlow = -1.f;
		bPaintedEmpty = false;
		ShownCharges = INDEX_NONE;
		ShownKey.Reset();
		bShown = false;
		bSnapNext = true;
		FlashLeft = 0.f;
		FlashPip = INDEX_NONE;
		WordsLeft = 0.f;
		GlowShown = 0.f;
		Activity = 0.f;
		OpacityShown = 1.f;
	}
	return Super::RebuildWidget();
}

// ---------------------------------------------------------------------------
// Frame by frame
// ---------------------------------------------------------------------------

void UHudFlaskWidget::Update(const UPlayerFlaskComponent* Flask, const FString& KeyName, float DeltaTime)
{
	if (!Flask)
	{
		UpdateState(false, 0, 0.f, 0.f, false, KeyName, DeltaTime);
		return;
	}
	UpdateState(true, Flask->GetCharges(), Flask->GetSoulShare(), Flask->IsDrinking() ? Flask->GetDrinkAlpha() : 0.f,
		Flask->IsHealing(), KeyName, DeltaTime);
}

void UHudFlaskWidget::UpdateState(bool bHasFlask, int32 Charges, float SoulShare, float DrinkAlpha, bool bHealing,
	const FString& KeyName, float DeltaTime)
{
	if (!Row || !KeyText || !IconImage || !IconGlow || !WordsText || PipFills.Num() != MaxPips || PipSurfaces.Num() != MaxPips)
	{
		return;
	}
	if (bHasFlask != bShown)
	{
		bShown = bHasFlask;
		Row->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// Coming back (a new pawn): shown as it is, nothing poured or filled on the way.
		bSnapNext = true;
	}
	if (!bShown)
	{
		return;
	}
	if (KeyName != ShownKey)
	{
		ShownKey = KeyName;
		KeyText->SetText(FText::FromString(KeyName));
	}

	const int32 Clamped = FMath::Clamp(Charges, 0, MaxPips);
	if (Clamped != ShownCharges)
	{
		ShownCharges = Clamped;
		Activity = ActivityHold;
	}
	// Each pip's level, moved toward where the flask says it is (snapped on first sight or a load).
	for (int32 Index = 0; Index < MaxPips; ++Index)
	{
		const float Target = PipTarget(Index, Clamped, SoulShare);
		ShownLevels[Index] = bSnapNext ? Target : StepLevel(ShownLevels[Index], Target, DeltaTime);
	}
	bSnapNext = false;
	if (DrinkAlpha > 0.f)
	{
		// Drinking: worth the row's full strength (soul coming in shows anyway: a flask not full stays up).
		Activity = ActivityHold;
	}

	RunFlash(DeltaTime);
	PaintPips();
	PaintIcon(DrinkAlpha, bHealing, DeltaTime);

	// Steps back with a full flask and nothing happening, as the player frame does; a flask not full stays up.
	Activity = FMath::Max(Activity - DeltaTime, 0.f);
	const bool bAttention = Activity > 0.f || FlashLeft > 0.f || WordsLeft > 0.f || Clamped < MaxPips;
	const float OpacityTarget = bAttention ? 1.f : IdleOpacity;
	OpacityShown = FMath::FInterpTo(OpacityShown, OpacityTarget, DeltaTime, OpacitySpeed);
	Row->SetRenderOpacity(OpacityShown);
}

void UHudFlaskWidget::Flash(EHudFlaskFlash Kind, int32 Charges)
{
	Activity = ActivityHold;
	if (Kind == EHudFlaskFlash::DeniedFull)
	{
		// "Health full" restarts with each refused press; it doesn't stop another flash.
		WordsLeft = WordsSeconds;
		return;
	}
	FlashKind = Kind;
	FlashLeft = FlashLength(Kind);
	// The flask tells its count after the change; without it, the row's own count is the one before (the flask says so
	// before the row's next update reads it).
	switch (Kind)
	{
	case EHudFlaskFlash::Drunk:
		// The pip just emptied: the one after the charges left.
		FlashPip = FMath::Clamp(Charges != INDEX_NONE ? Charges : ShownCharges - 1, 0, MaxPips - 1);
		break;
	case EHudFlaskFlash::Filled:
		// The new charge's pip: the last lit one.
		FlashPip = FMath::Clamp(Charges != INDEX_NONE ? Charges - 1 : ShownCharges, 0, MaxPips - 1);
		break;
	default:
		FlashPip = INDEX_NONE;
		break;
	}
}

void UHudFlaskWidget::RunFlash(float DeltaTime)
{
	FWidgetTransform Transform;
	const bool bWasMoving = bRowMoved;
	bRowMoved = FlashLeft > 0.f;
	if (FlashLeft > 0.f)
	{
		FlashLeft = FMath::Max(FlashLeft - DeltaTime, 0.f);
		const float Length = FlashLength(FlashKind);
		const float Alpha = FMath::Clamp(FlashLeft / Length, 0.f, 1.f);
		switch (FlashKind)
		{
		case EHudFlaskFlash::Drunk:
			// A small dip, as the weight leaves the hand.
			Transform.Scale = FVector2D(1.f - DrunkDip * FMath::Sin(UE_PI * Alpha));
			break;
		case EHudFlaskFlash::Filled:
			Transform.Scale = FVector2D(1.f + FilledPop * Alpha * Alpha);
			GlowShown = FMath::Max(GlowShown, GlowFilled * Alpha);
			break;
		case EHudFlaskFlash::DeniedEmpty:
			Transform.Translation = FVector2D(ShakeOffset(Length - FlashLeft), 0.f);
			break;
		default:
			break;
		}
	}
	if (bRowMoved || bWasMoving)
	{
		// Moving, or just stopped (back to rest).
		Row->SetRenderTransform(Transform);
	}

	if (WordsLeft > 0.f)
	{
		WordsLeft = FMath::Max(WordsLeft - DeltaTime, 0.f);
		const float Age = WordsSeconds - WordsLeft;
		const float Opacity = FMath::Min(FMath::Clamp(Age / WordsIn, 0.f, 1.f), FMath::Clamp(WordsLeft / WordsOut, 0.f, 1.f));
		WordsText->SetRenderOpacity(Opacity);
		WordsText->SetVisibility(WordsLeft > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UHudFlaskWidget::PaintPips()
{
	const float Length = FlashLength(FlashKind);
	const float Alpha = FlashLeft > 0.f ? FMath::Clamp(FlashLeft / Length, 0.f, 1.f) : 0.f;
	// A refusal with none left: the empty tracks flush red and fade back.
	const float Red = Alpha > 0.f && FlashKind == EHudFlaskFlash::DeniedEmpty ? Alpha : 0.f;
	const bool bRedChanged = Red != PaintedRed;
	PaintedRed = Red;
	for (int32 Index = 0; Index < MaxPips; ++Index)
	{
		// Only what moved is set again: the row sits still most of the time.
		const float Level = ShownLevels[Index];
		if (Level != PaintedLevels[Index])
		{
			PaintedLevels[Index] = Level;
			UImage* Fill = PipFills[Index];
			// Never a zero scale (a flat transform has no inverse): an empty pip's fill is collapsed instead.
			Fill->SetRenderScale(FVector2D(1.f, FMath::Max(Level, 0.01f)));
			Fill->SetVisibility(Level > LevelSnap ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			// A full charge in the soul-light; one still filling a little see-through, so the full ones stand out.
			Fill->SetColorAndOpacity(WithAlpha(Color::Heal(), Level >= 1.f - LevelSnap ? 1.f : PartialOpacity));
			// The soul's surface rides on the fill's top, so the level reads even where the fill is faint.
			UWidget* Surface = PipSurfaces[Index];
			const float Rise = (PipHeight - PipInset * 2.f - SurfaceHeight) * FMath::Clamp(Level, 0.f, 1.f);
			Surface->SetRenderTranslation(FVector2D(0.f, -Rise));
			Surface->SetVisibility(Level > LevelSnap ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (bRedChanged)
		{
			PipTracks[Index]->SetColorAndOpacity(FMath::Lerp(WithAlpha(Color::Track(), 0.85f), WithAlpha(Color::Hurt(), 0.95f), Red));
		}

		float White = 0.f;
		float Pop = 0.f;
		if (Alpha > 0.f)
		{
			switch (FlashKind)
			{
			case EHudFlaskFlash::Filled:
				White = Index == FlashPip ? Alpha : 0.f;
				Pop = Index == FlashPip ? FilledPipPop * Alpha * Alpha : 0.f;
				break;
			case EHudFlaskFlash::Drunk:
				// The pip pouring out glints as it goes.
				White = Index == FlashPip ? 0.6f * Alpha : 0.f;
				break;
			case EHudFlaskFlash::Refilled:
				// A soft sweep from the first pip to the last.
				White = RefilledFlash * FMath::Clamp(Alpha * 1.5f - Index * 0.25f, 0.f, 1.f);
				break;
			default:
				break;
			}
		}
		if (White != PaintedWhite[Index])
		{
			PaintedWhite[Index] = White;
			PipFlashes[Index]->SetRenderOpacity(White);
		}
		if (Pop != PaintedPop[Index])
		{
			PaintedPop[Index] = Pop;
			PipBoxes[Index]->SetRenderScale(FVector2D(1.f + Pop));
		}
	}
}

void UHudFlaskWidget::PaintIcon(float DrinkAlpha, bool bHealing, float DeltaTime)
{
	const bool bEmpty = ShownCharges == 0;
	if (bEmpty != bPaintedEmpty)
	{
		bPaintedEmpty = bEmpty;
		IconImage->SetColorAndOpacity(bEmpty ? EmptyIcon : FLinearColor::White);
		KeyText->SetColorAndOpacity(FSlateColor(bEmpty ? Color::TextDim() : Color::Text()));
	}
	const float Angle = TipAngle(DrinkAlpha);
	if (Angle != PaintedAngle)
	{
		PaintedAngle = Angle;
		IconImage->SetRenderTransformAngle(Angle);
	}

	// The glow swells while the heal flows and settles after (a filled charge's flare is held up by RunFlash as it fades).
	const float Target = bHealing ? GlowHealing : 0.f;
	GlowShown = FMath::FInterpTo(GlowShown, Target, DeltaTime, GlowShown < Target ? GlowInSpeed : GlowOutSpeed);
	const float Glow = GlowShown > 0.01f ? GlowShown : 0.f;
	if (Glow != PaintedGlow)
	{
		PaintedGlow = Glow;
		IconGlow->SetRenderOpacity(Glow);
		IconGlow->SetVisibility(Glow > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
