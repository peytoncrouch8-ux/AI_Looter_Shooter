#include "UI/Cards/LooterCardBackdrop.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UI/Style/LooterUIStyle.h"

// The "Wide Sweep" card and row (the user's pick, 2026-10-28; mockup Docs/Work/2026-10-28-UICharacter/mockup/wide.png),
// painted as vertices so it is crisp at any size and needs no texture per maker. The glass and the lines are two widgets
// (Part) so the glass alone fades with the UI transparency setting.

using namespace LooterUI;

namespace
{
	/** The sweep's S-curve, from the mockup's path "M0 0 H232 C250 0 256 28 276 28" on a 420 px card. */
	constexpr float CurveStart = 232.f / 420.f;
	constexpr float CurveEnd = 276.f / 420.f;

	FVector2f Bezier(const FVector2f& A, const FVector2f& B, const FVector2f& C, const FVector2f& D, float T)
	{
		const float U = 1.f - T;
		return A * (U * U * U) + B * (3.f * U * U * T) + C * (3.f * U * T * T) + D * (T * T * T);
	}

	/** Draws filled triangles in one call (a card's glass, a sweep's plates). */
	struct FPainter
	{
		FSlateWindowElementList& Elements;
		int32 Layer;
		const FGeometry& Geometry;
		float Alpha;
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;

		FPainter(FSlateWindowElementList& InElements, int32 InLayer, const FGeometry& InGeometry, float InAlpha)
			: Elements(InElements), Layer(InLayer), Geometry(InGeometry), Alpha(InAlpha) {}

		SlateIndex Add(const FVector2f& Point, const FLinearColor& Tint)
		{
			FLinearColor Faded = Tint;
			Faded.A *= Alpha;
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geometry.GetAccumulatedRenderTransform(), Point,
				FVector2f(0.5f, 0.5f), Faded.ToFColor(true)));
			return static_cast<SlateIndex>(Verts.Num() - 1);
		}

		void Triangle(SlateIndex A, SlateIndex B, SlateIndex C)
		{
			Indices.Add(A);
			Indices.Add(B);
			Indices.Add(C);
		}

		/** A convex polygon, each point coloured by its height between Top (y 0) and Bottom (y Height). */
		void Convex(const TArray<FVector2f>& Points, const FLinearColor& Top, const FLinearColor& Bottom, float Height)
		{
			if (Points.Num() < 3)
			{
				return;
			}
			auto At = [&](const FVector2f& Point)
			{
				return FMath::Lerp(Top, Bottom, FMath::Clamp(Point.Y / FMath::Max(Height, 1.f), 0.f, 1.f));
			};
			const SlateIndex First = Add(Points[0], At(Points[0]));
			SlateIndex Last = Add(Points[1], At(Points[1]));
			for (int32 Index = 2; Index < Points.Num(); ++Index)
			{
				const SlateIndex Next = Add(Points[Index], At(Points[Index]));
				Triangle(First, Last, Next);
				Last = Next;
			}
		}

		/** A quad with its own colour at each corner (top-left, top-right, bottom-right, bottom-left). */
		void Quad(const FVector2f& TL, const FVector2f& TR, const FVector2f& BR, const FVector2f& BL, const FLinearColor& CTL,
			const FLinearColor& CTR, const FLinearColor& CBR, const FLinearColor& CBL)
		{
			const SlateIndex A = Add(TL, CTL);
			const SlateIndex B = Add(TR, CTR);
			const SlateIndex C = Add(BR, CBR);
			const SlateIndex D = Add(BL, CBL);
			Triangle(A, B, C);
			Triangle(A, C, D);
		}

		/** A small round dot (a rivet, a bolt head): an octagon. */
		void Dot(const FVector2f& Center, float Radius, const FLinearColor& Tint)
		{
			TArray<FVector2f> Points;
			for (int32 Step = 0; Step < 8; ++Step)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Step) / 8.f;
				Points.Add(Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
			}
			Convex(Points, Tint, Tint, 1.f);
		}

		void Flush()
		{
			if (Verts.Num() == 0 || !FSlateApplication::IsInitialized())
			{
				return;
			}
			const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
			const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White);
			FSlateDrawElement::MakeCustomVerts(Elements, Layer, Handle, Verts, Indices, nullptr, 0, 0);
			Verts.Reset();
			Indices.Reset();
		}
	};

	void Lines(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, TArray<FVector2f> Points,
		const FLinearColor& Tint, float Alpha, float Width)
	{
		if (Points.Num() < 2)
		{
			return;
		}
		FLinearColor Faded = Tint;
		Faded.A *= Alpha;
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Faded, true, Width);
	}

	/** The sweep's top edge at X on a card Width wide (0 on the straight part, down the curve after it). */
	float SweepTopAt(float X, const TArray<FVector2f>& Outline)
	{
		// Outline runs (0,0) -> straight -> curve points -> (Length, H) -> (0, H). Find the curve segment holding X.
		for (int32 Index = 1; Index + 2 < Outline.Num(); ++Index)
		{
			const FVector2f& A = Outline[Index];
			const FVector2f& B = Outline[Index + 1];
			if (X >= A.X && X <= B.X && B.X > A.X)
			{
				return FMath::Lerp(A.Y, B.Y, (X - A.X) / (B.X - A.X));
			}
		}
		return X <= Outline[1].X ? 0.f : LooterBackdrop::SweepHeight;
	}

	FLinearColor PlateAt(const GunCardStyle::FStyle& Style, float Share)
	{
		const TArray<FLinearColor>& Plates = Style.Plates;
		if (Plates.Num() == 0)
		{
			return Color::MakerPlain();
		}
		if (Style.PlateStops.Num() == 0)
		{
			return Plates.Num() == 1 ? Plates[0] : FMath::Lerp(Plates[0], Plates.Last(), FMath::Clamp(Share, 0.f, 1.f));
		}
		int32 Plate = 0;
		while (Plate < Style.PlateStops.Num() && Share >= Style.PlateStops[Plate])
		{
			++Plate;
		}
		return Plates[FMath::Min(Plate, Plates.Num() - 1)];
	}

	/** The sweep's plates, as columns under its top edge (hard edges at a patchwork's stops). */
	void PaintSweep(FPainter& Painter, const GunCardStyle::FStyle& Style, float Width)
	{
		const TArray<FVector2f> Outline = LooterBackdrop::SweepOutline(Width);
		const float Length = LooterBackdrop::SweepLength(Width);
		const float H = LooterBackdrop::SweepHeight;
		TArray<float> Xs;
		for (int32 Index = 0; Index + 2 < Outline.Num(); ++Index)
		{
			Xs.Add(Outline[Index].X);
		}
		Xs.Add(Length);
		for (float Stop : Style.PlateStops)
		{
			Xs.Add(Stop * Length);
		}
		Xs.Sort();
		for (int32 Index = 0; Index + 1 < Xs.Num(); ++Index)
		{
			const float X0 = Xs[Index];
			const float X1 = Xs[Index + 1];
			if (X1 - X0 < 0.01f)
			{
				continue;
			}
			// A patchwork's plate is one colour across its column; a gradient blends from column edge to edge.
			const float Mid = (X0 + X1) * 0.5f / Length;
			const bool bPlates = Style.PlateStops.Num() > 0;
			const FLinearColor C0 = bPlates ? PlateAt(Style, Mid) : PlateAt(Style, X0 / Length);
			const FLinearColor C1 = bPlates ? PlateAt(Style, Mid) : PlateAt(Style, X1 / Length);
			Painter.Quad(FVector2f(X0, SweepTopAt(X0, Outline)), FVector2f(X1, SweepTopAt(X1, Outline)), FVector2f(X1, H), FVector2f(X0, H),
				C0, C1, C1, C0);
		}
	}

	/** The sweep's deco: what tells the makers apart besides colour. */
	void PaintDeco(FPainter& Painter, TArray<TPair<TArray<FVector2f>, FLinearColor>>& OutLines, const GunCardStyle::FStyle& Style, float Width)
	{
		const float Length = LooterBackdrop::SweepLength(Width);
		const float Straight = Width * CurveStart;
		switch (Style.Deco)
		{
		case GunCardStyle::EDeco::Rivets:
			for (const float X : { 8.f, Straight - 12.f })
			{
				Painter.Dot(FVector2f(X, 8.f), 2.4f, Style.Trim);
				Painter.Dot(FVector2f(X, 20.f), 2.4f, Style.Trim);
			}
			break;
		case GunCardStyle::EDeco::Gilt:
		{
			const float X = Straight - 36.f;
			OutLines.Emplace(TArray<FVector2f>{ FVector2f(X, 8.f), FVector2f(X + 5.f, 14.f), FVector2f(X, 20.f), FVector2f(X - 5.f, 14.f), FVector2f(X, 8.f) }, Style.Trim);
			OutLines.Emplace(TArray<FVector2f>{ FVector2f(X - 22.f, 14.f), FVector2f(X - 9.f, 14.f) }, Style.Trim);
			OutLines.Emplace(TArray<FVector2f>{ FVector2f(X + 9.f, 14.f), FVector2f(X + 22.f, 14.f) }, Style.Trim);
			break;
		}
		case GunCardStyle::EDeco::Patch:
		{
			// A strip of tape slapped on near the curve, and two bolt heads.
			const FVector2f Center(Straight - 24.f, 3.f);
			const FVector2f Along = FVector2f(FMath::Cos(-0.14f), FMath::Sin(-0.14f)) * 15.f;
			const FVector2f Across = FVector2f(-Along.Y, Along.X).GetSafeNormal() * 5.f;
			const FLinearColor Tape = Color::MakerWindfallTape();
			Painter.Quad(Center - Along - Across, Center + Along - Across, Center + Along + Across, Center - Along + Across, Tape, Tape, Tape, Tape);
			Painter.Dot(FVector2f(6.f, 6.f), 2.8f, Color::MakerWindfallBolt());
			Painter.Dot(FVector2f(Straight - 6.f, 21.f), 2.8f, Color::MakerWindfallBolt());
			break;
		}
		case GunCardStyle::EDeco::Ticks:
			// A surveyor's ruler along the foot: every fifth tick long, in the brass trim.
			for (int32 Tick = 0; 4.f + Tick * 8.f < Length - 2.f; ++Tick)
			{
				const float X = 4.f + Tick * 8.f;
				const bool bLong = Tick % 5 == 0;
				FLinearColor Tint = bLong ? Style.Trim : Style.Ink;
				Tint.A *= 0.75f;
				OutLines.Emplace(TArray<FVector2f>{ FVector2f(X, bLong ? 19.f : 22.f), FVector2f(X, LooterBackdrop::SweepHeight - 1.f) }, Tint);
			}
			break;
		default:
			break;
		}
	}
}

TArray<FVector2f> LooterBackdrop::CardOutline(const FVector2f& Size)
{
	const float Cut = FMath::Min(CardCut, FMath::Min(Size.X, Size.Y) * 0.5f);
	return { FVector2f(0.f, 0.f), FVector2f(Size.X - Cut, 0.f), FVector2f(Size.X, Cut), FVector2f(Size.X, Size.Y), FVector2f(Cut, Size.Y),
		FVector2f(0.f, Size.Y - Cut) };
}

TArray<FVector2f> LooterBackdrop::RowOutline(const FVector2f& Size)
{
	const float Cut = FMath::Min(RowCut, FMath::Min(Size.X, Size.Y) * 0.5f);
	return { FVector2f(0.f, 0.f), FVector2f(Size.X - Cut, 0.f), FVector2f(Size.X, Cut), FVector2f(Size.X, Size.Y), FVector2f(0.f, Size.Y) };
}

float LooterBackdrop::SweepLength(float Width)
{
	return Width * CurveEnd;
}

TArray<FVector2f> LooterBackdrop::SweepOutline(float Width)
{
	// The mockup's cubic: from the straight top's end, its handles 18 px on and 20 px back at 420 wide, scaled with the width.
	const float Scale = Width / 420.f;
	const FVector2f A(Width * CurveStart, 0.f);
	const FVector2f D(Width * CurveEnd, SweepHeight);
	const FVector2f B(A.X + 18.f * Scale, 0.f);
	const FVector2f C(D.X - 20.f * Scale, SweepHeight);
	TArray<FVector2f> Points = { FVector2f(0.f, 0.f), A };
	constexpr int32 Steps = 12;
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		Points.Add(Bezier(A, B, C, D, static_cast<float>(Step) / Steps));
	}
	Points.Add(FVector2f(0.f, SweepHeight));
	return Points;
}

void SLooterCardBackdrop::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
}

void SLooterCardBackdrop::SetShape(ELooterBackdropShape InShape) { Shape = InShape; Invalidate(EInvalidateWidgetReason::Paint); }
void SLooterCardBackdrop::SetPart(ELooterBackdropPart InPart) { Part = InPart; Invalidate(EInvalidateWidgetReason::Paint); }
void SLooterCardBackdrop::SetSweep(const GunCardStyle::FStyle* InSweep) { Sweep = InSweep; Invalidate(EInvalidateWidgetReason::Paint); }
void SLooterCardBackdrop::SetRowState(ELooterRowState InState) { RowState = InState; Invalidate(EInvalidateWidgetReason::Paint); }
void SLooterCardBackdrop::SetRowTick(const FLinearColor& InTick) { RowTick = InTick; Invalidate(EInvalidateWidgetReason::Paint); }

void SLooterCardBackdrop::SetShine(float InShine)
{
	if (!FMath::IsNearlyEqual(Shine, InShine))
	{
		Shine = InShine;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

int32 SLooterCardBackdrop::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	if (Size.X < 2.f || Size.Y < 2.f)
	{
		return LayerId;
	}
	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A;
	const float W = LooterBackdrop::LineWidth;
	// Lines sit a half-width inside the shape so the cut edges aren't shaved by the box.
	const float In = W * 0.5f;
	int32 Layer = LayerId;

	if (Shape == ELooterBackdropShape::Card)
	{
		if (Part == ELooterBackdropPart::Fill)
		{
			FPainter Painter(OutDrawElements, Layer, AllottedGeometry, Alpha);
			Painter.Convex(LooterBackdrop::CardOutline(Size), Color::CardFillTop(), Color::CardFillBottom(), Size.Y);
			Painter.Flush();
			return Layer;
		}
		const float Cut = FMath::Min(LooterBackdrop::CardCut, FMath::Min(Size.X, Size.Y) * 0.5f);
		const float Start = Sweep ? LooterBackdrop::SweepLength(Size.X) : 0.f;
		if (Sweep)
		{
			FPainter Painter(OutDrawElements, Layer, AllottedGeometry, Alpha);
			PaintSweep(Painter, *Sweep, Size.X);
			TArray<TPair<TArray<FVector2f>, FLinearColor>> DecoLines;
			PaintDeco(Painter, DecoLines, *Sweep, Size.X);
			// The shine: a soft bright band skewed like the cuts, crossing the sweep's straight part once.
			if (Shine >= 0.f && Shine <= 1.f)
			{
				const float Straight = Size.X * CurveStart;
				const float X = FMath::Lerp(-30.f, Straight + 10.f, Shine);
				const FLinearColor Clear = Color::CardShine().CopyWithNewOpacity(0.f);
				const float H = LooterBackdrop::SweepHeight;
				const float Skew = 10.f;
				const float X0 = FMath::Max(0.f, X - 18.f);
				const float X2 = FMath::Min(Straight, X + 18.f);
				if (X2 > X0)
				{
					const float Mid = FMath::Clamp(X, X0, X2);
					Painter.Quad(FVector2f(X0 + Skew, 0.f), FVector2f(Mid + Skew, 0.f), FVector2f(Mid, H), FVector2f(X0, H), Clear, Color::CardShine(), Color::CardShine(), Clear);
					Painter.Quad(FVector2f(Mid + Skew, 0.f), FVector2f(X2 + Skew, 0.f), FVector2f(X2, H), FVector2f(Mid, H), Color::CardShine(), Clear, Clear, Color::CardShine());
				}
			}
			Painter.Flush();
			++Layer;
			for (TPair<TArray<FVector2f>, FLinearColor>& Line : DecoLines)
			{
				Lines(OutDrawElements, Layer, AllottedGeometry, MoveTemp(Line.Key), Line.Value, Alpha, 1.3f);
			}
			// The trim along the sweep's foot, 3 px.
			Lines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(0.f, LooterBackdrop::SweepHeight - 1.5f), FVector2f(Start, LooterBackdrop::SweepHeight - 1.5f) },
				Sweep->Trim, Alpha, 3.f);
		}
		++Layer;
		// The cyan line from the sweep's end along the top and down the cut; the bracket round the bottom-left cut.
		Lines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(Start, In), FVector2f(Size.X - Cut, In), FVector2f(Size.X - In, Cut) },
			Color::CardLine(), Alpha, W);
		Lines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(Cut + 44.f, Size.Y - In), FVector2f(Cut, Size.Y - In), FVector2f(In, Size.Y - Cut) },
			Color::CardBracket(), Alpha, W);
		return Layer;
	}

	// A row.
	const TArray<FVector2f> Outline = LooterBackdrop::RowOutline(Size);
	if (Part == ELooterBackdropPart::Fill)
	{
		FPainter Painter(OutDrawElements, Layer, AllottedGeometry, Alpha);
		if (RowState == ELooterRowState::Lit)
		{
			// The wash runs left to right: a quad per outline span, coloured by x.
			const float Cut = FMath::Min(LooterBackdrop::RowCut, FMath::Min(Size.X, Size.Y) * 0.5f);
			const FLinearColor L = Color::RowLitWash();
			const FLinearColor R = Color::RowLitWashEnd();
			const FLinearColor M = FMath::Lerp(L, R, (Size.X - Cut) / Size.X);
			Painter.Quad(FVector2f(0.f, 0.f), FVector2f(Size.X - Cut, 0.f), FVector2f(Size.X - Cut, Size.Y), FVector2f(0.f, Size.Y), L, M, M, L);
			Painter.Quad(FVector2f(Size.X - Cut, 0.f), FVector2f(Size.X, Cut), FVector2f(Size.X, Size.Y), FVector2f(Size.X - Cut, Size.Y), M, R, R, M);
		}
		else
		{
			const FLinearColor Glass = RowState == ELooterRowState::Empty ? Color::RowEmpty() : Color::RowFill();
			Painter.Convex(Outline, Glass, Glass, Size.Y);
		}
		Painter.Flush();
		return Layer;
	}
	if (RowState == ELooterRowState::Lit)
	{
		TArray<FVector2f> Ring;
		const float Cut = FMath::Min(LooterBackdrop::RowCut, FMath::Min(Size.X, Size.Y) * 0.5f);
		Ring = { FVector2f(In, In), FVector2f(Size.X - Cut, In), FVector2f(Size.X - In, Cut), FVector2f(Size.X - In, Size.Y - In),
			FVector2f(In, Size.Y - In), FVector2f(In, In) };
		Lines(OutDrawElements, Layer, AllottedGeometry, MoveTemp(Ring), Color::RowLitLine(), Alpha, W);
	}
	const FLinearColor Tick = RowState == ELooterRowState::Lit ? Color::Accent() : RowTick;
	if (Tick.A > 0.f)
	{
		// A short skewed tick on the row's left, like the cuts: orange when lit, else the gun's rarity.
		FPainter Painter(OutDrawElements, Layer + 1, AllottedGeometry, Alpha);
		const float Half = FMath::Min(13.f, Size.Y * 0.5f - 6.f);
		const float Mid = Size.Y * 0.5f;
		const float X = 14.f;
		const float Lean = 3.f;
		const float Thick = RowState == ELooterRowState::Lit ? 4.f : 3.f;
		Painter.Quad(FVector2f(X + Lean, Mid - Half), FVector2f(X + Lean + Thick, Mid - Half), FVector2f(X + Thick, Mid + Half), FVector2f(X, Mid + Half),
			Tick, Tick, Tick, Tick);
		Painter.Flush();
		++Layer;
	}
	return Layer;
}

// ---------------------------------------------------------------------------
// UMG
// ---------------------------------------------------------------------------

TSharedRef<SWidget> ULooterCardBackdrop::RebuildWidget()
{
	Backdrop = SNew(SLooterCardBackdrop);
	Apply();
	return Backdrop.ToSharedRef();
}

void ULooterCardBackdrop::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Backdrop.Reset();
}

void ULooterCardBackdrop::Apply()
{
	if (Backdrop.IsValid())
	{
		Backdrop->SetShape(Shape);
		Backdrop->SetPart(Part);
		Backdrop->SetSweep(Sweep);
		Backdrop->SetShine(Shine);
		Backdrop->SetRowState(RowState);
		Backdrop->SetRowTick(RowTick);
	}
}

void ULooterCardBackdrop::SetShape(ELooterBackdropShape InShape) { Shape = InShape; Apply(); }
void ULooterCardBackdrop::SetPart(ELooterBackdropPart InPart) { Part = InPart; Apply(); }
void ULooterCardBackdrop::SetSweep(const GunCardStyle::FStyle* InSweep) { Sweep = InSweep; Apply(); }
void ULooterCardBackdrop::SetRowTick(const FLinearColor& InTick) { RowTick = InTick; Apply(); }

void ULooterCardBackdrop::SetShine(float InShine)
{
	Shine = InShine;
	if (Backdrop.IsValid())
	{
		Backdrop->SetShine(Shine);
	}
}

void ULooterCardBackdrop::SetRowState(ELooterRowState InState)
{
	if (RowState != InState)
	{
		RowState = InState;
		Apply();
	}
}

UOverlay* ULooterCardBackdrop::MakeLayered(UWidgetTree* Tree, ELooterBackdropShape Shape, UWidget* Content, const FMargin& Padding,
	ULooterCardBackdrop*& OutFill, ULooterCardBackdrop*& OutLines)
{
	UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	OutFill = Tree->ConstructWidget<ULooterCardBackdrop>(ULooterCardBackdrop::StaticClass());
	OutFill->SetShape(Shape);
	OutFill->SetPart(ELooterBackdropPart::Fill);
	OutFill->SetVisibility(ESlateVisibility::HitTestInvisible);
	LooterUI::FillOverlaySlot(Overlay->AddChildToOverlay(OutFill));
	LooterUI::MarkBackground(OutFill);
	OutLines = Tree->ConstructWidget<ULooterCardBackdrop>(ULooterCardBackdrop::StaticClass());
	OutLines->SetShape(Shape);
	OutLines->SetPart(ELooterBackdropPart::Lines);
	OutLines->SetVisibility(ESlateVisibility::HitTestInvisible);
	LooterUI::FillOverlaySlot(Overlay->AddChildToOverlay(OutLines));
	if (Content)
	{
		LooterUI::FillOverlaySlot(Overlay->AddChildToOverlay(Content), Padding);
	}
	return Overlay;
}
