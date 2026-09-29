#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/FontFace.h"
#include "Fonts/CompositeFont.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** A generated 9-slice shape: white fill/outline textures that get tinted per use. */
	struct FShapeTextures
	{
		UTexture2D* Fill = nullptr;
		UTexture2D* Line = nullptr;
		FVector2D Size = FVector2D::ZeroVector;
		FMargin Margin;
	};

	UTexture2D* MakeTexture(int32 Width, int32 Height, TFunctionRef<float(float, float)> Alpha, const TCHAR* Name)
	{
		TArray<uint8> Pixels;
		Pixels.SetNumUninitialized(Width * Height * 4);
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 Index = (Y * Width + X) * 4;
				Pixels[Index + 0] = 255;
				Pixels[Index + 1] = 255;
				Pixels[Index + 2] = 255;
				Pixels[Index + 3] = static_cast<uint8>(FMath::Clamp(Alpha(X + 0.5f, Y + 0.5f), 0.f, 1.f) * 255.f + 0.5f);
			}
		}

		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, FName(Name), Pixels);
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->NeverStream = true;
		Texture->UpdateResource();
		Texture->AddToRoot(); // Shared for the whole session, like engine style assets.
		return Texture;
	}

	/** Signed distance to a convex polygon given clockwise (screen space) vertices; negative inside. */
	float ConvexDistance(const TArray<FVector2D>& Points, float X, float Y)
	{
		float Distance = -TNumericLimits<float>::Max();
		for (int32 I = 0; I < Points.Num(); ++I)
		{
			const FVector2D A = Points[I];
			const FVector2D B = Points[(I + 1) % Points.Num()];
			const FVector2D Edge = (B - A).GetSafeNormal();
			const FVector2D Outward(Edge.Y, -Edge.X);
			Distance = FMath::Max(Distance, FVector2D::DotProduct(FVector2D(X, Y) - A, Outward));
		}
		return Distance;
	}

	FShapeTextures MakePolygonShape(const TArray<FVector2D>& Points, int32 Width, int32 Height, float LineWidth, const FMargin& Margin, const TCHAR* Name)
	{
		FShapeTextures Shape;
		Shape.Size = FVector2D(Width, Height);
		Shape.Margin = Margin;
		Shape.Fill = MakeTexture(Width, Height, [&Points](float X, float Y)
		{
			return 0.5f - ConvexDistance(Points, X, Y);
		}, *FString::Printf(TEXT("UI_%s_Fill"), Name));
		Shape.Line = MakeTexture(Width, Height, [&Points, LineWidth](float X, float Y)
		{
			const float D = ConvexDistance(Points, X, Y);
			return FMath::Clamp(0.5f - D, 0.f, 1.f) * FMath::Clamp(D + LineWidth + 0.5f, 0.f, 1.f);
		}, *FString::Printf(TEXT("UI_%s_Line"), Name));
		return Shape;
	}

	const FShapeTextures& GetShape(LooterUI::EShape Shape)
	{
		static TMap<uint8, FShapeTextures> Cache;
		if (const FShapeTextures* Found = Cache.Find(static_cast<uint8>(Shape)))
		{
			return *Found;
		}

		FShapeTextures Made;
		switch (Shape)
		{
		case LooterUI::EShape::Panel:
		{
			const float W = 64.f, C = 14.f;
			Made = MakePolygonShape({ {C, 0}, {W - C, 0}, {W, C}, {W, W - C}, {W - C, W}, {C, W}, {0, W - C}, {0, C} },
				64, 64, 2.f, FMargin(18.f / 64.f), TEXT("Panel"));
			break;
		}
		case LooterUI::EShape::Control:
		{
			const float W = 32.f, C = 7.f;
			Made = MakePolygonShape({ {C, 0}, {W, 0}, {W, W - C}, {W - C, W}, {0, W}, {0, C} },
				32, 32, 1.f, FMargin(10.f / 32.f), TEXT("Control"));
			break;
		}
		case LooterUI::EShape::Tab:
		{
			const float W = 64.f, H = 32.f, S = 12.f;
			Made = MakePolygonShape({ {S, 0}, {W - S, 0}, {W, H}, {0, H} },
				64, 32, 1.5f, FMargin(16.f / 64.f, 0.f, 16.f / 64.f, 0.f), TEXT("Tab"));
			break;
		}
		case LooterUI::EShape::Bracket:
		{
			// Top stroke + left stroke: the little orange corner brackets on panels.
			Made.Size = FVector2D(32, 12);
			Made.Fill = MakeTexture(32, 12, [](float X, float Y)
			{
				return (Y < 2.5f || X < 2.5f) ? 1.f : 0.f;
			}, TEXT("UI_Bracket"));
			Made.Line = Made.Fill;
			break;
		}
		}
		return Cache.Add(static_cast<uint8>(Shape), Made);
	}

	UTexture2D* GetScanlines()
	{
		static UTexture2D* Scanlines = nullptr;
		if (!Scanlines)
		{
			Scanlines = MakeTexture(4, 4, [](float X, float Y) { return Y < 1.f ? 0.05f : 0.f; }, TEXT("UI_Scanlines"));
		}
		return Scanlines;
	}

	UImage* MakeImage(UWidgetTree* Tree, const FSlateBrush& Brush)
	{
		UImage* Image = Tree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	void FillOverlaySlot(UOverlaySlot* Slot, const FMargin& Padding = FMargin(0.f))
	{
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
		Slot->SetPadding(Padding);
	}

	UWidget* MakeSizedRect(UWidgetTree* Tree, float Width, float Height, const FSlateBrush& Brush)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(Width);
		Size->SetHeightOverride(Height);
		Size->SetContent(MakeImage(Tree, Brush));
		return Size;
	}
}

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------

FLinearColor LooterUI::Hex(uint8 R, uint8 G, uint8 B, uint8 A)
{
	return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
}

namespace LooterUI::Color
{
	FLinearColor FrameEdge()  { return Hex(14, 17, 22); }
	FLinearColor FrameFill()  { return Hex(38, 43, 51); }
	FLinearColor ScreenBg()   { return Hex(7, 26, 40, 245); }
	FLinearColor ScreenLine() { return Hex(90, 200, 255, 140); }
	FLinearColor Plate()      { return Hex(14, 44, 66); }
	FLinearColor Title()      { return Hex(191, 234, 255); }
	FLinearColor Accent()     { return Hex(255, 159, 28); }
	FLinearColor AccentDark() { return Hex(35, 20, 0); }
	FLinearColor Text()       { return Hex(220, 239, 255); }
	FLinearColor TextDim()    { return Hex(143, 179, 204); }
	FLinearColor Row()        { return Hex(18, 64, 94, 140); }
	FLinearColor RowLine()    { return Hex(90, 200, 255, 72); }
	FLinearColor Tile()       { return Hex(18, 64, 94); }
	FLinearColor TileLine()   { return Hex(90, 200, 255, 180); }
	FLinearColor SegmentOn()  { return Hex(92, 202, 255); }
	FLinearColor SegmentOff() { return Hex(90, 200, 255, 31); }
	FLinearColor Better()     { return Hex(109, 255, 122); }
	FLinearColor Worse()      { return Hex(255, 122, 107); }
	FLinearColor Danger()     { return Hex(122, 34, 25); }
	FLinearColor Health()     { return Hex(255, 91, 74); }
	FLinearColor Backdrop()   { return Hex(2, 8, 14, 190); }
}

// ---------------------------------------------------------------------------
// Fonts, brushes, button styles
// ---------------------------------------------------------------------------

namespace
{
	/** Chakra Petch (OFL) font faces in /Game/UI/Fonts. Null (Roboto fallback) if they're missing. */
	TSharedPtr<const FCompositeFont> GetUIFont()
	{
		static TSharedPtr<const FCompositeFont> CompositeFont = []() -> TSharedPtr<const FCompositeFont>
		{
			UFontFace* Regular = LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Fonts/ChakraPetch-Regular.ChakraPetch-Regular"));
			UFontFace* Bold = LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Fonts/ChakraPetch-Bold.ChakraPetch-Bold"));
			if (!Regular || !Bold)
			{
				UE_LOG(LogTemp, Warning, TEXT("Chakra Petch font faces not found in /Game/UI/Fonts, falling back to Roboto."));
				return nullptr;
			}
			// Slate holds raw pointers to the faces, so keep them loaded for the life of the game.
			Regular->AddToRoot();
			Bold->AddToRoot();
			TSharedRef<FCompositeFont> Font = MakeShared<FCompositeFont>();
			FTypefaceEntry& RegularEntry = Font->DefaultTypeface.Fonts.Add_GetRef(FTypefaceEntry(TEXT("Regular")));
			RegularEntry.Font = FFontData(Regular);
			FTypefaceEntry& BoldEntry = Font->DefaultTypeface.Fonts.Add_GetRef(FTypefaceEntry(TEXT("Bold")));
			BoldEntry.Font = FFontData(Bold);
			return Font;
		}();
		return CompositeFont;
	}
}

FSlateFontInfo LooterUI::Font(int32 Size, bool bBold, int32 LetterSpacing)
{
	const FName Typeface = bBold ? TEXT("Bold") : TEXT("Regular");
	const TSharedPtr<const FCompositeFont> UIFont = GetUIFont();
	FSlateFontInfo FontInfo = UIFont.IsValid() ? FSlateFontInfo(UIFont, Size, Typeface) : FCoreStyle::GetDefaultFontStyle(Typeface, Size);
	FontInfo.LetterSpacing = LetterSpacing;
	return FontInfo;
}

void LooterUI::StyleText(UTextBlock* Text, int32 Size, const FLinearColor& TextColor, bool bBold, int32 LetterSpacing)
{
	FSlateFontInfo FontInfo = Font(Size, bBold, LetterSpacing);
	// A thin dark outline keeps world-space text (damage numbers, loot labels) readable anywhere.
	FontInfo.OutlineSettings.OutlineSize = FMath::Max(1, Size / 14);
	FontInfo.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.85f);
	Text->SetFont(FontInfo);
	Text->SetColorAndOpacity(FSlateColor(TextColor));
	Text->SetJustification(ETextJustify::Center);
}

FSlateBrush LooterUI::ShapeBrush(EShape Shape, bool bOutline, const FLinearColor& Tint)
{
	const FShapeTextures& Textures = GetShape(Shape);
	FSlateBrush Brush;
	Brush.SetResourceObject(bOutline ? Textures.Line : Textures.Fill);
	Brush.ImageSize = Textures.Size;
	Brush.DrawAs = Shape == EShape::Bracket ? ESlateBrushDrawType::Image : ESlateBrushDrawType::Box;
	Brush.Margin = Textures.Margin;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}

FSlateBrush LooterUI::RectBrush(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth)
{
	return FSlateRoundedBoxBrush(Fill, 0.f, Outline, OutlineWidth);
}

FSlateBrush LooterUI::CircleBrush(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth, UObject* Texture)
{
	// A rounded box whose corner radius is half its height is a circle at any size; Slate clips a texture to it too.
	FSlateBrush Brush = FSlateRoundedBoxBrush(Fill, 0.f, Outline, OutlineWidth);
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	if (Texture)
	{
		Brush.SetResourceObject(Texture);
	}
	return Brush;
}

namespace
{
	/**
	 * A marker shape from its signed distance (negative inside): white inside, a dark edge band, transparent outside,
	 * so a single tint colors the fill and the edge keeps it readable on any background.
	 */
	UTexture2D* MakeMarkerTexture(int32 Size, TFunctionRef<float(float, float)> Distance, const TCHAR* Name)
	{
		constexpr float EdgeWidth = 1.3f;
		TArray<uint8> Pixels;
		Pixels.SetNumUninitialized(Size * Size * 4);
		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				const float D = Distance(X + 0.5f, Y + 0.5f);
				const uint8 Shade = static_cast<uint8>(FMath::Clamp(-EdgeWidth - D + 0.5f, 0.f, 1.f) * 255.f + 0.5f);
				const int32 Index = (Y * Size + X) * 4;
				Pixels[Index + 0] = Shade;
				Pixels[Index + 1] = Shade;
				Pixels[Index + 2] = Shade;
				Pixels[Index + 3] = static_cast<uint8>(FMath::Clamp(0.5f - D, 0.f, 1.f) * 255.f + 0.5f);
			}
		}
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8, FName(Name), Pixels);
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->NeverStream = true;
		Texture->UpdateResource();
		Texture->AddToRoot(); // Shared for the whole session, like the other generated UI textures.
		return Texture;
	}

	UTexture2D* GetMarkerTexture(LooterUI::EMarker Marker)
	{
		static UTexture2D* Textures[3] = {};
		UTexture2D*& Texture = Textures[static_cast<int32>(Marker)];
		if (Texture)
		{
			return Texture;
		}
		switch (Marker)
		{
		case LooterUI::EMarker::Arrow:
		{
			// An arrowhead with a notched tail: two triangles sharing the spine.
			const TArray<FVector2D> Right = { {16, 2}, {30, 30}, {16, 24} };
			const TArray<FVector2D> Left = { {16, 2}, {16, 24}, {2, 30} };
			Texture = MakeMarkerTexture(32, [&Right, &Left](float X, float Y)
			{
				return FMath::Min(ConvexDistance(Right, X, Y), ConvexDistance(Left, X, Y));
			}, TEXT("UI_Marker_Arrow"));
			break;
		}
		case LooterUI::EMarker::Diamond:
		{
			const TArray<FVector2D> Points = { {16, 2}, {30, 16}, {16, 30}, {2, 16} };
			Texture = MakeMarkerTexture(32, [&Points](float X, float Y) { return ConvexDistance(Points, X, Y); }, TEXT("UI_Marker_Diamond"));
			break;
		}
		default:
			Texture = MakeMarkerTexture(32, [](float X, float Y) { return FVector2D(X - 16.f, Y - 16.f).Size() - 13.f; }, TEXT("UI_Marker_Dot"));
			break;
		}
		return Texture;
	}
}

FSlateBrush LooterUI::MarkerBrush(EMarker Marker, const FLinearColor& Tint)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(GetMarkerTexture(Marker));
	Brush.ImageSize = FVector2D(32.f, 32.f);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}

namespace
{
	float SegmentDistance(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double T = FMath::Clamp(FVector2D::DotProduct(Point - A, AB) / FMath::Max(AB.SizeSquared(), UE_SMALL_NUMBER), 0.0, 1.0);
		return FVector2D::Distance(Point, A + AB * T);
	}
}

FSlateBrush LooterUI::IconBrush(FName Name, const FVectorIcon& Icon, float PixelsPerUnit, const FVector2D& Size, const FLinearColor& Tint)
{
	static TMap<TPair<FName, int32>, UTexture2D*> Cache;
	const int32 Resolution = FMath::RoundToInt(PixelsPerUnit * 100.f);
	UTexture2D*& Texture = Cache.FindOrAdd({ Name, Resolution });
	if (!Texture)
	{
		// Draw in texture pixels: scale the shapes once, then every pixel takes its coverage from the nearest edge.
		auto Scaled = [PixelsPerUnit](const TArray<TArray<FVector2D>>& Shapes)
		{
			TArray<TArray<FVector2D>> Result = Shapes;
			for (TArray<FVector2D>& Shape : Result)
			{
				for (FVector2D& Point : Shape)
				{
					Point *= PixelsPerUnit;
				}
			}
			return Result;
		};
		const TArray<TArray<FVector2D>> Fills = Scaled(Icon.Fills);
		const TArray<TArray<FVector2D>> Strokes = Scaled(Icon.Strokes);
		const float HalfStroke = Icon.StrokeWidth * PixelsPerUnit * 0.5f;
		Texture = MakeTexture(FMath::CeilToInt(Icon.ViewBox.X * PixelsPerUnit), FMath::CeilToInt(Icon.ViewBox.Y * PixelsPerUnit),
			[&Fills, &Strokes, HalfStroke](float X, float Y)
		{
			float Alpha = 0.f;
			for (const TArray<FVector2D>& Fill : Fills)
			{
				Alpha = FMath::Max(Alpha, 0.5f - ConvexDistance(Fill, X, Y));
			}
			const FVector2D Point(X, Y);
			for (const TArray<FVector2D>& Stroke : Strokes)
			{
				for (int32 Index = 0; Index + 1 < Stroke.Num(); ++Index)
				{
					Alpha = FMath::Max(Alpha, HalfStroke + 0.5f - SegmentDistance(Point, Stroke[Index], Stroke[Index + 1]));
				}
			}
			return Alpha;
		}, *FString::Printf(TEXT("UI_Icon_%s_%d"), *Name.ToString(), Resolution));
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = Size;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Tint);
	return Brush;
}

namespace
{
	struct FButtonColors
	{
		FLinearColor Normal, Hovered, Pressed, Text, Line;
	};

	FButtonColors ColorsFor(LooterUI::EButtonKind Kind, bool bHighlighted)
	{
		using namespace LooterUI;
		if (Kind == EButtonKind::Bare)
		{
			return { FLinearColor::Transparent, FLinearColor::Transparent, FLinearColor::Transparent, Color::Text(), FLinearColor::Transparent };
		}
		if (bHighlighted || Kind == EButtonKind::Primary)
		{
			return { Color::Accent(), Hex(255, 181, 77), Hex(224, 133, 15), Color::AccentDark(), Hex(255, 201, 119) };
		}
		switch (Kind)
		{
		case EButtonKind::Danger:
			return { Color::Danger(), Hex(150, 48, 34), Hex(92, 25, 17), Hex(255, 225, 220), Hex(255, 122, 107, 180) };
		case EButtonKind::Mini:
		case EButtonKind::Tab:
			return { Hex(11, 34, 51, 170), Hex(22, 77, 114), Hex(14, 44, 66), Color::TextDim(), Color::RowLine() };
		default:
			return { Hex(22, 77, 114), Hex(31, 99, 148), Hex(16, 56, 79), Color::Text(), Hex(120, 215, 255, 230) };
		}
	}
}

FButtonStyle LooterUI::ButtonStyle(EButtonKind Kind, bool bHighlighted)
{
	const FButtonColors Colors = ColorsFor(Kind, bHighlighted);
	FButtonStyle Style;
	Style.SetNormal(ShapeBrush(EShape::Control, false, Colors.Normal));
	Style.SetHovered(ShapeBrush(EShape::Control, false, Colors.Hovered));
	Style.SetPressed(ShapeBrush(EShape::Control, false, Colors.Pressed));
	Style.SetDisabled(ShapeBrush(EShape::Control, false, Colors.Normal * FLinearColor(1.f, 1.f, 1.f, 0.5f)));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	return Style;
}

FLinearColor LooterUI::ButtonTextColor(EButtonKind Kind, bool bHighlighted)
{
	return ColorsFor(Kind, bHighlighted).Text;
}

FLinearColor LooterUI::ButtonLineColor(EButtonKind Kind, bool bHighlighted)
{
	return ColorsFor(Kind, bHighlighted).Line;
}

// ---------------------------------------------------------------------------
// UI transparency
// ---------------------------------------------------------------------------

namespace
{
	float GBackgroundOpacity = 1.f;

	/** Every live background widget, so a change of the setting reaches UIs that are already built. */
	TArray<TWeakObjectPtr<UWidget>>& Backgrounds()
	{
		static TArray<TWeakObjectPtr<UWidget>> List;
		return List;
	}

	void ApplyBackgroundOpacity(UWidget* Widget, float Opacity)
	{
		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			// A border's brush color tints only its own background, not what it holds.
			Border->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, Opacity));
		}
		else if (Widget)
		{
			Widget->SetRenderOpacity(Opacity);
		}
	}
}

float LooterUI::GetBackgroundOpacity()
{
	return GBackgroundOpacity;
}

void LooterUI::SetBackgroundOpacity(float Opacity)
{
	GBackgroundOpacity = FMath::Clamp(Opacity, 0.f, 1.f);
	TArray<TWeakObjectPtr<UWidget>>& List = Backgrounds();
	List.RemoveAll([](const TWeakObjectPtr<UWidget>& Widget) { return !Widget.IsValid(); });
	for (const TWeakObjectPtr<UWidget>& Widget : List)
	{
		ApplyBackgroundOpacity(Widget.Get(), GBackgroundOpacity);
	}
}

void LooterUI::MarkBackground(UWidget* Widget)
{
	if (!Widget)
	{
		return;
	}
	// UIs rebuild parts of themselves as they go (inventory tiles): drop the dead entries whenever the list doubles.
	static int32 NextPrune = 256;
	TArray<TWeakObjectPtr<UWidget>>& List = Backgrounds();
	if (List.Num() >= NextPrune)
	{
		List.RemoveAll([](const TWeakObjectPtr<UWidget>& Entry) { return !Entry.IsValid(); });
		NextPrune = FMath::Max(256, List.Num() * 2);
	}
	List.Add(Widget);
	ApplyBackgroundOpacity(Widget, GBackgroundOpacity);
}

FLinearColor LooterUI::BackgroundColor(const FLinearColor& Color)
{
	return FLinearColor(Color.R, Color.G, Color.B, Color.A * GBackgroundOpacity);
}

// ---------------------------------------------------------------------------
// Widget builders
// ---------------------------------------------------------------------------

UTextBlock* LooterUI::MakeText(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& TextColor, bool bUpper, int32 LetterSpacing)
{
	UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Block->SetFont(Font(Size, true, LetterSpacing));
	Block->SetColorAndOpacity(FSlateColor(TextColor));
	Block->SetShadowOffset(FVector2D(1.f, 1.f));
	Block->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	Block->SetJustification(ETextJustify::Left);
	Block->SetText(FText::FromString(bUpper ? Text.ToUpper() : Text));
	return Block;
}

UWidget* LooterUI::MakeShapeBox(UWidgetTree* Tree, EShape Shape, const FLinearColor& Fill, const FLinearColor& Line, UWidget* Content, const FMargin& Padding)
{
	UOverlay* Box = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// The fill is background (it follows the UI transparency setting); the outline always shows.
	UImage* FillImage = MakeImage(Tree, ShapeBrush(Shape, false, Fill));
	MarkBackground(FillImage);
	FillOverlaySlot(Box->AddChildToOverlay(FillImage));
	FillOverlaySlot(Box->AddChildToOverlay(MakeImage(Tree, ShapeBrush(Shape, true, Line))));
	if (Content)
	{
		FillOverlaySlot(Box->AddChildToOverlay(Content), Padding);
	}
	return Box;
}

UWidget* LooterUI::MakePanel(UWidgetTree* Tree, const FString& Title, UWidget* Content, UWidget* CornerButton)
{
	constexpr float TabOverlap = 18.f;
	UOverlay* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	// Dark glass screen with scanlines; content sits below the title tab. The glass and scanlines are background (they
	// follow the UI transparency setting); the screen's outline always shows.
	UOverlay* Screen = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	UImage* Glass = MakeImage(Tree, RectBrush(Color::ScreenBg()));
	MarkBackground(Glass);
	FillOverlaySlot(Screen->AddChildToOverlay(Glass));
	FSlateBrush ScanBrush;
	ScanBrush.SetResourceObject(GetScanlines());
	ScanBrush.ImageSize = FVector2D(4.f, 4.f);
	ScanBrush.DrawAs = ESlateBrushDrawType::Image;
	ScanBrush.Tiling = ESlateBrushTileType::Both;
	UImage* Scanlines = MakeImage(Tree, ScanBrush);
	MarkBackground(Scanlines);
	FillOverlaySlot(Screen->AddChildToOverlay(Scanlines), FMargin(1.5f));
	FillOverlaySlot(Screen->AddChildToOverlay(MakeImage(Tree, RectBrush(FLinearColor::Transparent, Color::ScreenLine(), 1.5f))));
	if (Content)
	{
		FillOverlaySlot(Screen->AddChildToOverlay(Content), FMargin(16.f, 30.f, 16.f, 16.f));
	}

	// Thin gunmetal frame around the glass.
	UWidget* Frame = MakeShapeBox(Tree, EShape::Panel, Color::FrameFill(), Color::FrameEdge(), Screen, FMargin(8.f));
	FillOverlaySlot(Root->AddChildToOverlay(Frame), FMargin(0.f, TabOverlap, 0.f, 0.f));

	// Orange clamps on both sides.
	for (const EHorizontalAlignment Side : { HAlign_Left, HAlign_Right })
	{
		UVerticalBox* Clamp = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		for (int32 Tick = 0; Tick < 3; ++Tick)
		{
			UVerticalBoxSlot* TickSlot = Clamp->AddChildToVerticalBox(MakeSizedRect(Tree, 8.f, 6.f, RectBrush(Color::Accent(), Hex(122, 74, 0), 1.f)));
			TickSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 3.f));
		}
		UOverlaySlot* ClampSlot = Root->AddChildToOverlay(Clamp);
		ClampSlot->SetHorizontalAlignment(Side);
		ClampSlot->SetVerticalAlignment(VAlign_Center);
		ClampSlot->SetPadding(FMargin(Side == HAlign_Left ? 0.f : 0.f, TabOverlap, 0.f, 0.f));
	}

	// Corner brackets on the top edge (the right one is mirrored).
	for (const EHorizontalAlignment Side : { HAlign_Left, HAlign_Right })
	{
		UImage* Bracket = MakeImage(Tree, ShapeBrush(EShape::Bracket, false, Color::Accent()));
		if (Side == HAlign_Right)
		{
			Bracket->SetRenderScale(FVector2D(-1.f, 1.f));
		}
		UOverlaySlot* BracketSlot = Root->AddChildToOverlay(Bracket);
		BracketSlot->SetHorizontalAlignment(Side);
		BracketSlot->SetVerticalAlignment(VAlign_Top);
		BracketSlot->SetPadding(Side == HAlign_Left ? FMargin(30.f, TabOverlap - 8.f, 0.f, 0.f) : FMargin(0.f, TabOverlap - 8.f, 30.f, 0.f));
	}

	// Title tab straddling the top edge.
	UBorder* TitlePlate = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
	TitlePlate->SetBrush(RectBrush(Color::Plate(), Color::ScreenLine(), 1.f));
	TitlePlate->SetPadding(FMargin(26.f, 4.f));
	TitlePlate->SetContent(MakeText(Tree, Title, 15, Color::Title(), true, 350));
	UWidget* Tab = MakeShapeBox(Tree, EShape::Tab, Color::FrameFill(), Color::FrameEdge(), TitlePlate, FMargin(18.f, 5.f, 18.f, 3.f));
	UOverlaySlot* TabSlot = Root->AddChildToOverlay(Tab);
	TabSlot->SetHorizontalAlignment(HAlign_Center);
	TabSlot->SetVerticalAlignment(VAlign_Top);

	if (CornerButton)
	{
		UOverlaySlot* CornerSlot = Root->AddChildToOverlay(CornerButton);
		CornerSlot->SetHorizontalAlignment(HAlign_Right);
		CornerSlot->SetVerticalAlignment(VAlign_Top);
		CornerSlot->SetPadding(FMargin(0.f, TabOverlap - 12.f, -6.f, 0.f));
	}
	return Root;
}

UWidget* LooterUI::MakeSection(UWidgetTree* Tree, const FString& Label)
{
	return MakeText(Tree, Label, 12, Color::Accent(), true, 300);
}

UWidget* LooterUI::MakeRow(UWidgetTree* Tree, UWidget* Content)
{
	return MakeShapeBox(Tree, EShape::Control, Color::Row(), Color::RowLine(), Content, FMargin(12.f, 5.f, 6.f, 5.f));
}

UWidget* LooterUI::MakeSegmentBar(UWidgetTree* Tree, int32 Segments, float Fraction, const FLinearColor& OnColor, float Height)
{
	UHorizontalBox* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const int32 Lit = FMath::Clamp(FMath::RoundToInt(Fraction * Segments), 0, Segments);
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		USizeBox* Segment = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Segment->SetHeightOverride(Height);
		Segment->SetContent(MakeImage(Tree, RectBrush(Index < Lit ? OnColor : Color::SegmentOff())));
		UHorizontalBoxSlot* SegmentSlot = Bar->AddChildToHorizontalBox(Segment);
		SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Segments ? 3.f : 0.f, 0.f));
	}
	return Bar;
}

UWidget* LooterUI::MakePlate(UWidgetTree* Tree, UWidget* Content, const FMargin& Padding)
{
	return MakeShapeBox(Tree, EShape::Control, Hex(7, 26, 40, 215), Color::ScreenLine(), Content, Padding);
}

void LooterUI::StyleScrollBox(UScrollBox* ScrollBox)
{
	FScrollBarStyle Bar;
	Bar.SetVerticalBackgroundImage(RectBrush(Color::SegmentOff()));
	Bar.SetHorizontalBackgroundImage(RectBrush(Color::SegmentOff()));
	Bar.SetVerticalTopSlotImage(RectBrush(FLinearColor::Transparent));
	Bar.SetVerticalBottomSlotImage(RectBrush(FLinearColor::Transparent));
	Bar.SetNormalThumbImage(RectBrush(Color::Tile(), Color::TileLine(), 1.f));
	Bar.SetHoveredThumbImage(RectBrush(Hex(31, 99, 148), Color::TileLine(), 1.f));
	Bar.SetDraggedThumbImage(RectBrush(Color::Accent()));
	Bar.SetThickness(6.f);
	ScrollBox->SetWidgetBarStyle(Bar);
	ScrollBox->SetScrollbarThickness(FVector2D(6.f, 6.f));
	ScrollBox->SetScrollbarPadding(FMargin(8.f, 0.f, 0.f, 0.f));
}
