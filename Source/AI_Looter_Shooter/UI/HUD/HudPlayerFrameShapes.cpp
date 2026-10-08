#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/HudPortraitWidget.h"

using namespace LooterUI;

HudFrameShapes::FPoints HudFrameShapes::FrameDiamond(const FVector2D& Centre, float Radius)
{
	return { Centre + FVector2D(0.f, -Radius), Centre + FVector2D(Radius, 0.f), Centre + FVector2D(0.f, Radius),
		Centre + FVector2D(-Radius, 0.f) };
}

HudFrameShapes::FPoints HudFrameShapes::ClosedLine(FPoints Points)
{
	if (Points.Num() > 0)
	{
		// A copy first: adding an element of the array to itself could read it after the array has moved.
		const FVector2D First = Points[0];
		Points.Add(First);
	}
	return Points;
}

HudFrameShapes::FPoints HudFrameShapes::Leaned(const FPoints& Points, float Mid, const FVector2D& Shift)
{
	FPoints Result;
	Result.Reserve(Points.Num());
	for (const FVector2D& Point : Points)
	{
		Result.Add(FVector2D(Point.X + (Mid - Point.Y) * LeanPerPixel, Point.Y) + Shift);
	}
	return Result;
}

HudFrameShapes::FPoints HudFrameShapes::LeanBox(float Left, float Top, float Right, float Bottom, float Mid, const FVector2D& Shift)
{
	return Leaned({ FVector2D(Left, Top), FVector2D(Right, Top), FVector2D(Right, Bottom), FVector2D(Left, Bottom) }, Mid, Shift);
}

HudFrameShapes::FPoints HudFrameShapes::EndWedge(float Top, float Bottom, float CutBottom, float Overlap, const FVector2D& Shift)
{
	auto CutAt = [CutBottom](float Y) { return (CutBottom - Y) * LeanPerPixel; };
	return { FVector2D(-Overlap, Top) + Shift, FVector2D(CutAt(Top), Top) + Shift, FVector2D(CutAt(Bottom), Bottom) + Shift,
		FVector2D(-Overlap, Bottom) + Shift };
}

HudFrameShapes::FPoints HudFrameShapes::ClipToConvex(const FPoints& Subject, const FPoints& Clip)
{
	FPoints Result = Subject;
	for (int32 Side = 0; Side < Clip.Num() && Result.Num() > 0; ++Side)
	{
		const FVector2D A = Clip[Side];
		const FVector2D Edge = Clip[(Side + 1) % Clip.Num()] - A;
		// Clockwise on screen (y down), the inside lies where this cross product is positive.
		auto Inside = [&A, &Edge](const FVector2D& P) { return FVector2D::CrossProduct(Edge, P - A); };
		const FPoints Input = MoveTemp(Result);
		Result.Reset();
		for (int32 Index = 0; Index < Input.Num(); ++Index)
		{
			const FVector2D& P = Input[Index];
			const FVector2D& Q = Input[(Index + 1) % Input.Num()];
			const double InP = Inside(P);
			const double InQ = Inside(Q);
			if (InP >= 0.0)
			{
				Result.Add(P);
			}
			if ((InP >= 0.0) != (InQ >= 0.0))
			{
				Result.Add(P + (Q - P) * (InP / (InP - InQ)));
			}
		}
	}
	return Result;
}

FPaintLayer HudFrameShapes::PaintFill(TArray<FPoints> Shapes, const FLinearColor& Color, float Opacity)
{
	FPaintLayer Layer;
	Layer.Fills = MoveTemp(Shapes);
	Layer.Color = Color;
	Layer.Opacity = Opacity;
	return Layer;
}

FPaintLayer HudFrameShapes::PaintStroke(TArray<FPoints> Lines, float Width, const FLinearColor& Color, float Opacity)
{
	FPaintLayer Layer;
	Layer.Strokes = MoveTemp(Lines);
	Layer.StrokeWidth = Width;
	Layer.Color = Color;
	Layer.Opacity = Opacity;
	return Layer;
}

FPaintLayer HudFrameShapes::PaintGraded(FPaintLayer Layer, const FLinearColor& To, const FVector2D& Start, const FVector2D& End)
{
	Layer.GradientTo = To;
	Layer.GradientStart = Start;
	Layer.GradientEnd = End;
	return Layer;
}

void HudFrameShapes::AddSoftShadow(FPaintedIcon& Picture, TFunctionRef<FPoints(float)> Outline, const TArray<FPoints>& Holes,
	float Spread)
{
	static constexpr int32 Steps = 3;
	static constexpr float Grows[Steps] = { 0.2f, 0.6f, 1.f };
	static constexpr float Opacities[Steps] = { 0.24f, 0.14f, 0.07f };
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		FPaintLayer Layer = PaintFill({ Outline(Grows[Index] * Spread) }, Color::Ink(), Opacities[Index]);
		Layer.Fills.Append(Holes);
		Picture.Layers.Add(MoveTemp(Layer));
	}
}

void HudFrameShapes::KeepOutOfWindow(FPaintedIcon& Picture, const FVector2D& Shift)
{
	// Every part of the bar near the medallion lies above and right of its centre, where the window's edge is the line
	// x - y = const in the bar's pixels: keeping the side past that line (half a pixel out) is enough, and the bezel's
	// ring covers the cut. Holes stay holes: cutting the shapes and their holes alike cuts their difference.
	const float WindowLine = (UHudPlayerFrameWidget::MedallionX - UHudPlayerFrameWidget::HealthBarX)
		- (UHudPlayerFrameWidget::MedallionY - UHudPlayerFrameWidget::HealthBarY) + UHudPortraitWidget::WindowSize * 0.5f + 0.5f;
	const float Line = WindowLine + Shift.X - Shift.Y;
	const FPoints PastWindow = { FVector2D(Line - 1000.f, -1000.f), FVector2D(5000.f, -1000.f), FVector2D(5000.f, 1000.f),
		FVector2D(Line + 1000.f, 1000.f) };
	for (FPaintLayer& Layer : Picture.Layers)
	{
		TArray<FPoints> Kept;
		for (const FPoints& Shape : Layer.Fills)
		{
			FPoints Cut = ClipToConvex(Shape, PastWindow);
			if (Cut.Num() >= 3)
			{
				Kept.Add(MoveTemp(Cut));
			}
		}
		Layer.Fills = MoveTemp(Kept);
	}
}

float HudFrameShapes::SectionStart(int32 Section)
{
	return SectionInset + Section * (SectionLength + XPSectionGap);
}

TArray<HudFrameShapes::FPoints> HudFrameShapes::XPSections(float Top, float Bottom, const FVector2D& Shift)
{
	TArray<FPoints> Result;
	for (int32 Section = 0; Section < SectionCount; ++Section)
	{
		Result.Add(LeanBox(SectionStart(Section), Top, SectionStart(Section) + SectionLength, Bottom, XPMid, Shift));
	}
	return Result;
}
