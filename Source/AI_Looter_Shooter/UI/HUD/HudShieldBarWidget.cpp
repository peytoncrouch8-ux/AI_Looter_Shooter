// UHudShieldBarWidget: the ward bar's tree, what it shows of the pawn's ward each frame, and its slanted stretches (the
// frame's way: a picture cropped short of the level and an end piece along the lean). The pictures are
// HudShieldBarWidgetPictures.cpp.

#include "UI/HUD/HudShieldBarWidget.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldTuning.h"
#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

using namespace LooterUI;

namespace
{
	using W = UHudShieldBarWidget;

	/** A stretch's drawn end: nothing, the whole bar, or not drawn yet. */
	constexpr float CutHidden = -1000.f;
	constexpr float CutFull = 100000.f;
	constexpr float CutUnpainted = -2000.f;
	/** Half the lean across the track: a stretch's end is cut this far left of its level. */
	constexpr float HalfLean = W::TrackHeight * 0.5f * HudFrameShapes::LeanPerPixel;
	/** A stretch shorter than this into the track shows as a little more: its end piece would poke out past the start. */
	constexpr float MinReach = HalfLean * 2.f + 1.f;
	constexpr float FullAt = 0.9995f;

	/** The broken ward's pulse: the empty track's cyan breathing this deep, this fast, while it waits to fill. */
	constexpr float PulseLow = 0.06f;
	constexpr float PulseHigh = 0.22f;
	constexpr float PulseSeconds = 1.1f;
	/** How bright each flash starts (a break's is whitest), and the chip's colour (the kit's cyan, paled). */
	constexpr float BreakFlash = 1.f;
	constexpr float FullFlash = 0.6f;
	constexpr float GraceFlash = 0.9f;
	/** The slivers: how many, how long they fly, how fast (px/s) and how hard they fall. */
	constexpr int32 ShardCount = 7;
	constexpr float ShardSeconds = 0.55f;
	constexpr float ShardSpeed = 120.f;
	constexpr float ShardFall = 260.f;

	float BarLevel(float Amount)
	{
		return FMath::Max(W::TrackLeft + Amount * (W::TrackRight - W::TrackLeft), W::TrackLeft + MinReach);
	}

	FLinearColor ChipColor()
	{
		return FMath::Lerp(Color::CyanText(), FLinearColor::White, 0.6f).CopyWithNewOpacity(0.85f);
	}
}

TSharedRef<SWidget> UHudShieldBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		Canvas = Root;
		auto AddPicture = [this, Root](EHudShieldPicture Picture)
		{
			UImage* Image = MakeImage(WidgetTree, PictureBrush(Picture));
			UCanvasPanelSlot* ImageSlot = Root->AddChildToCanvas(Image);
			ImageSlot->SetAutoSize(true);
			ImageSlot->SetPosition(PicturePosition(Picture));
			return Image;
		};

		// Back to front: the metal, the track (a background), the chip, the fill, the shimmer, the flash, the slivers.
		AddPicture(EHudShieldPicture::Back);
		MarkBackground(AddPicture(EHudShieldPicture::Track));
		Chip = AddStretch(Root, EHudShieldPicture::WhiteBody, EHudShieldPicture::WhiteCap, ChipColor());
		Fill = AddStretch(Root, EHudShieldPicture::FillBody, EHudShieldPicture::FillCap, FLinearColor::White);
		ShimmerBrush = GlowBrush(FVector2D(ShimmerLength, TrackHeight), FMath::Lerp(Color::CyanText(), FLinearColor::White, 0.7f).CopyWithNewOpacity(0.85f));
		Shimmer = MakeImage(WidgetTree, ShimmerBrush);
		Shimmer->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot* ShimmerSlot = Root->AddChildToCanvas(Shimmer);
		ShimmerSlot->SetAutoSize(true);
		ShimmerSlot->SetPosition(FVector2D(-BoxLeft, TrackTop - BoxTop));
		bShimmerShown = false;
		Flash = AddStretch(Root, EHudShieldPicture::WhiteBody, EHudShieldPicture::WhiteCap, FLinearColor::Transparent);

		Shards.Reset();
		ShardMotion.SetNum(ShardCount);
		for (int32 Index = 0; Index < ShardCount; ++Index)
		{
			UImage* Sliver = AddPicture(EHudShieldPicture::Shard);
			Sliver->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Sliver->SetVisibility(ESlateVisibility::Hidden);
			Shards.Add(Sliver);
		}

		// The charge, after the bar's end on its middle (the frame's numbers' type, in the kit's cyan).
		Number = MakeFloatingText(WidgetTree, NumberSize, Color::CyanText());
		UCanvasPanelSlot* NumberSlot = Root->AddChildToCanvas(Number);
		NumberSlot->SetAutoSize(true);
		NumberSlot->SetAlignment(FVector2D(0.f, 0.5f));
		NumberSlot->SetPosition(FVector2D(Length + NumberGap - BoxLeft, Mid - BoxTop));

		USizeBox* Sized = MakeSized(WidgetTree, Root, Width, Height);
		Sized->SetVisibility(ESlateVisibility::Hidden);
		WidgetTree->RootWidget = Sized;
		bShown = false;
		ShownPoints = INDEX_NONE;
		SeenWard = 0;
	}
	return Super::RebuildWidget();
}

void UHudShieldBarWidget::Update(float InDeltaTime)
{
	const UPlayerShieldComponent* Shield = Watched.Get();
	const APawn* Pawn = GetOwningPlayerPawn();
	if (!Shield || Shield->GetOwner() != Pawn)
	{
		// The pawn changed (a respawn's new body, a possession) or the ward came later: look again.
		Shield = UPlayerShieldComponent::Find(Pawn);
		Watched = Shield;
	}
	ShowShield(Shield, InDeltaTime);
}

bool UHudShieldBarWidget::ConsumeActivity()
{
	const bool bWas = bActivity;
	bActivity = false;
	return bWas;
}

void UHudShieldBarWidget::Snap(float Share)
{
	FillShown = ChipShown = ChipFrom = Share;
	ChipHold = 0.f;
	ChipDrain = 1000.f;
	FlashAge = 1000.f;
	for (FShard& Shard : ShardMotion)
	{
		Shard.Age = 1000.f;
	}
	for (UImage* Sliver : Shards)
	{
		if (Sliver)
		{
			Sliver->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UHudShieldBarWidget::ShowShield(const UPlayerShieldComponent* Shield, float DeltaTime)
{
	UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr;
	if (!Root || !Fill.Body)
	{
		return;
	}
	const bool bWard = Shield && Shield->HasWard();
	if (!bWard)
	{
		if (bShown)
		{
			Root->SetVisibility(ESlateVisibility::Hidden);
			bShown = false;
			bActivity = true;
		}
		return;
	}
	const FShieldTuning Tuning = FShieldTuning::Live();
	const float Share = Shield->GetChargeShare();
	const uint32 WardIdentity = ShieldRules::Identity(Shield->GetWard());
	if (!bShown || WardIdentity != SeenWard)
	{
		// First sight, or another ward put on: shown as it is.
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		bShown = true;
		bActivity = true;
		SeenWard = WardIdentity;
		SeenHits = Shield->GetHitCount();
		SeenBreaks = Shield->GetBreakCount();
		SeenFills = Shield->GetFillCount();
		SeenGraces = Shield->GetGraceCount();
		Snap(Share);
	}

	// A hit: the fill drops at once, what it took lingers as the chip, then drains.
	if (Shield->GetHitCount() != SeenHits)
	{
		SeenHits = Shield->GetHitCount();
		if (Share < FillShown)
		{
			ChipFrom = FMath::Max(ChipShown, FillShown);
			ChipHold = Tuning.HudChipHold;
			ChipDrain = 0.f;
		}
		bActivity = true;
	}
	if (Shield->GetBreakCount() != SeenBreaks)
	{
		SeenBreaks = Shield->GetBreakCount();
		// The break: the whole bar flashes white and slivers fly off it from where the fill was.
		FlashAge = 0.f;
		FlashColor = FLinearColor::White.CopyWithNewOpacity(BreakFlash);
		ThrowShards(ChipFrom);
		bActivity = true;
	}
	if (Shield->GetFillCount() != SeenFills)
	{
		SeenFills = Shield->GetFillCount();
		FlashAge = 0.f;
		FlashColor = FMath::Lerp(Color::GemLight(), FLinearColor::White, 0.5f).CopyWithNewOpacity(FullFlash);
		bActivity = true;
	}
	if (Shield->GetGraceCount() != SeenGraces)
	{
		SeenGraces = Shield->GetGraceCount();
		// Saint's Grace: a warm flash, the saints' gold.
		FlashAge = 0.f;
		FlashColor = Color::ElementSoulLight().CopyWithNewOpacity(GraceFlash);
		bActivity = true;
	}
	// The fill follows the charge: down at once, up as it refills (the charge itself rises smoothly).
	FillShown = Share;

	if (ChipHold > 0.f)
	{
		ChipHold -= DeltaTime;
		ChipShown = ChipFrom;
	}
	else if (ChipShown > FillShown)
	{
		ChipDrain += DeltaTime;
		ChipShown = FMath::Lerp(ChipFrom, FillShown, FMath::InterpEaseIn(0.f, 1.f, FMath::Min(ChipDrain / Tuning.HudChipDrain, 1.f), 2.f));
	}
	ChipShown = FMath::Max(ChipShown, FillShown);
	PaintStretch(Chip, ChipShown);
	PaintStretch(Fill, FillShown);

	// Refilling: a shimmer sweeps along the fill, again and again.
	if (Shield->IsRefilling())
	{
		ShimmerClock += DeltaTime;
		PaintShimmer(FMath::Fmod(ShimmerClock, Tuning.HudShimmerSeconds) / Tuning.HudShimmerSeconds, FillShown);
	}
	else
	{
		ShimmerClock = 0.f;
		PaintShimmer(1.f, FillShown);
	}

	// The flash over the whole bar fades; with none, a broken ward's empty track pulses faintly while it waits.
	const bool bBroken = Shield->IsBroken();
	if (FlashAge < Tuning.HudFlashSeconds)
	{
		FlashAge += DeltaTime;
		const float Left = 1.f - FMath::Clamp(FlashAge / Tuning.HudFlashSeconds, 0.f, 1.f);
		PaintStretch(Flash, 1.f);
		TintStretch(Flash, FlashColor * FLinearColor(1.f, 1.f, 1.f, Left * Left));
	}
	else if (bBroken)
	{
		const UWorld* World = GetWorld();
		const double Time = World ? World->GetTimeSeconds() : 0.0;
		const float Beat = static_cast<float>(0.5 - 0.5 * FMath::Cos(2.0 * UE_DOUBLE_PI * FMath::Fmod(Time, static_cast<double>(PulseSeconds)) / PulseSeconds));
		PaintStretch(Flash, 1.f);
		TintStretch(Flash, Color::Hairline().CopyWithNewOpacity(FMath::Lerp(PulseLow, PulseHigh, Beat)));
	}
	else
	{
		PaintStretch(Flash, 0.f);
	}
	TickShards(DeltaTime);

	// The charge's number, rounded up so a sliver never reads 0; dim while the ward is out.
	const int32 Points = FMath::CeilToInt32(Shield->GetCharge() - 0.01f);
	if (Points != ShownPoints || bBroken != bShownBroken)
	{
		ShownPoints = Points;
		bShownBroken = bBroken;
		Number->SetText(FText::AsNumber(FMath::Max(Points, 0)));
		Number->SetColorAndOpacity(FSlateColor(bBroken ? Color::TextDim() : Color::CyanText()));
	}
}

// ---------------------------------------------------------------------------
// Stretches, the shimmer, the slivers
// ---------------------------------------------------------------------------

UHudShieldBarWidget::FStretch UHudShieldBarWidget::AddStretch(UCanvasPanel* Root, EHudShieldPicture BodyPicture, EHudShieldPicture CapPicture,
	const FLinearColor& Tint)
{
	FStretch Stretch;
	auto AddLayer = [this, Root, &Tint](EHudShieldPicture Picture, const FSlateBrush& Brush)
	{
		UImage* Image = MakeImage(WidgetTree, Brush);
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot* ImageSlot = Root->AddChildToCanvas(Image);
		// Sized by its brush, so a cropped body stays anchored at its start.
		ImageSlot->SetAutoSize(true);
		ImageSlot->SetPosition(PicturePosition(Picture));
		return Image;
	};
	Stretch.Whole = PictureBrush(BodyPicture);
	Stretch.Body = AddLayer(BodyPicture, Stretch.Whole);
	Stretch.Cap = AddLayer(CapPicture, PictureBrush(CapPicture));
	Stretch.Shown = CutUnpainted;
	return Stretch;
}

void UHudShieldBarWidget::TintStretch(const FStretch& Stretch, const FLinearColor& Tint)
{
	if (Stretch.Body && Stretch.Cap)
	{
		Stretch.Body->SetColorAndOpacity(Tint);
		Stretch.Cap->SetColorAndOpacity(Tint);
	}
}

void UHudShieldBarWidget::PaintStretch(FStretch& Stretch, float Amount)
{
	if (!Stretch.Body || !Stretch.Cap)
	{
		return;
	}
	float Cut = CutHidden;
	if (Amount >= FullAt)
	{
		Cut = CutFull;
	}
	else if (Amount > 0.f)
	{
		// Just short of the level by half the lean, whole pixels, so the end piece's slant passes through the level.
		Cut = FMath::RoundToFloat(BarLevel(Amount) - HalfLean);
	}
	if (Cut == Stretch.Shown)
	{
		return;
	}
	Stretch.Shown = Cut;
	if (Cut == CutHidden)
	{
		Stretch.Body->SetVisibility(ESlateVisibility::Hidden);
		Stretch.Cap->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	if (Cut == CutFull)
	{
		Stretch.Body->SetBrush(Stretch.Whole);
		Stretch.Body->SetVisibility(ESlateVisibility::HitTestInvisible);
		Stretch.Cap->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	// Cropped, not squeezed: the body shows its picture from its start up to the cut.
	const float WholeLength = static_cast<float>(Stretch.Whole.ImageSize.X);
	const float Shown = FMath::Clamp(Cut - BoxLeft, 1.f, WholeLength);
	FSlateBrush Cropped = Stretch.Whole;
	Cropped.ImageSize = FVector2D(Shown, Stretch.Whole.ImageSize.Y);
	Cropped.SetUVRegion(FBox2f(FVector2f(0.f, 0.f), FVector2f(Shown / WholeLength, 1.f)));
	Stretch.Body->SetBrush(Cropped);
	Stretch.Body->SetVisibility(ESlateVisibility::HitTestInvisible);
	Stretch.Cap->SetRenderTranslation(FVector2D(Cut, 0.f));
	Stretch.Cap->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UHudShieldBarWidget::PaintShimmer(float Progress, float Amount)
{
	if (!Shimmer)
	{
		return;
	}
	const float From = TrackLeft + HalfLean;
	const float To = Amount >= FullAt ? TrackRight + HalfLean : BarLevel(Amount) - HalfLean;
	float Left = 0.f;
	float VisibleLeft = 0.f;
	float VisibleRight = 0.f;
	if (Progress < 1.f && Amount > 0.f && To > From)
	{
		// It runs in from before the fill's start and out past its end, steadily: the charge pouring in.
		Left = FMath::Lerp(From - ShimmerLength, To, Progress);
		VisibleLeft = FMath::Max(Left, From);
		VisibleRight = FMath::Min(Left + ShimmerLength, To);
	}
	if (VisibleRight - VisibleLeft < 0.5f)
	{
		if (bShimmerShown)
		{
			Shimmer->SetVisibility(ESlateVisibility::Hidden);
			bShimmerShown = false;
		}
		return;
	}
	// Cut off at the fill's ends: soft at both ends, so a straight cut doesn't show.
	FSlateBrush Cropped = ShimmerBrush;
	Cropped.ImageSize = FVector2D(VisibleRight - VisibleLeft, ShimmerBrush.ImageSize.Y);
	Cropped.SetUVRegion(FBox2f(FVector2f((VisibleLeft - Left) / ShimmerLength, 0.f), FVector2f((VisibleRight - Left) / ShimmerLength, 1.f)));
	Shimmer->SetBrush(Cropped);
	Shimmer->SetRenderTranslation(FVector2D(VisibleLeft, 0.f));
	if (!bShimmerShown)
	{
		Shimmer->SetVisibility(ESlateVisibility::HitTestInvisible);
		bShimmerShown = true;
	}
}

void UHudShieldBarWidget::ThrowShards(float From)
{
	const float Reach = BarLevel(FMath::Clamp(From, 0.08f, 1.f));
	for (int32 Index = 0; Index < ShardMotion.Num(); ++Index)
	{
		FShard& Shard = ShardMotion[Index];
		// Spread along what was lit, thrown up and out, spinning: the ward coming apart.
		const float Along = FMath::Lerp(TrackLeft + 6.f, Reach, (Index + FMath::FRandRange(0.2f, 0.8f)) / ShardMotion.Num());
		Shard.Position = FVector2D(Along, FMath::FRandRange(-1.5f, 1.5f));
		const float Angle = FMath::DegreesToRadians(FMath::FRandRange(-150.f, -30.f));
		Shard.Velocity = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * ShardSpeed * FMath::FRandRange(0.6f, 1.3f);
		Shard.Spin = FMath::FRandRange(-540.f, 540.f);
		Shard.Angle = FMath::FRandRange(-20.f, 20.f);
		Shard.Age = 0.f;
	}
}

void UHudShieldBarWidget::TickShards(float DeltaTime)
{
	for (int32 Index = 0; Index < ShardMotion.Num() && Index < Shards.Num(); ++Index)
	{
		FShard& Shard = ShardMotion[Index];
		UImage* Sliver = Shards[Index];
		if (!Sliver)
		{
			continue;
		}
		if (Shard.Age >= ShardSeconds)
		{
			if (Sliver->GetVisibility() != ESlateVisibility::Hidden)
			{
				Sliver->SetVisibility(ESlateVisibility::Hidden);
			}
			continue;
		}
		Shard.Age += DeltaTime;
		Shard.Velocity.Y += ShardFall * DeltaTime;
		Shard.Position += Shard.Velocity * DeltaTime;
		Shard.Angle += Shard.Spin * DeltaTime;
		const float Life = FMath::Clamp(Shard.Age / ShardSeconds, 0.f, 1.f);
		Sliver->SetRenderTranslation(Shard.Position);
		Sliver->SetRenderTransformAngle(Shard.Angle);
		Sliver->SetRenderScale(FVector2D(1.f - 0.4f * Life));
		Sliver->SetColorAndOpacity(FMath::Lerp(FLinearColor::White, Color::CyanText(), Life).CopyWithNewOpacity(1.f - Life * Life));
		Sliver->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}
