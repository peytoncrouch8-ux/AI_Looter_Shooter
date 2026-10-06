#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Text/TextLayout.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UImage;
class UOverlaySlot;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UWidget;
class UWidgetTree;

/**
 * The game's shared UI style kit ("Tactical Slim", concept C): thin gunmetal chamfered frames,
 * dark glass screens with scanlines, orange clamps and accents, cyan outlines.
 * Every menu and HUD element is built from these helpers so the whole game stays consistent.
 * The chamfer shapes are generated as textures at runtime, so there are no UI art assets to manage.
 */
namespace LooterUI
{
	/** A color from sRGB bytes, as written in a design mockup, converted to linear. */
	FLinearColor Hex(uint8 R, uint8 G, uint8 B, uint8 A = 255);

	/** Palette. Change a color here and every UI updates. */
	namespace Color
	{
		FLinearColor FrameEdge();
		FLinearColor FrameFill();
		FLinearColor ScreenBg();
		FLinearColor ScreenLine();
		FLinearColor Plate();
		FLinearColor Title();
		FLinearColor Accent();
		FLinearColor AccentDark();
		FLinearColor Text();
		FLinearColor TextDim();
		FLinearColor Row();
		FLinearColor RowLine();
		FLinearColor Tile();
		FLinearColor TileLine();
		FLinearColor SegmentOn();
		FLinearColor SegmentOff();
		FLinearColor Better();
		FLinearColor Worse();
		FLinearColor Danger();
		FLinearColor Health();
		FLinearColor Backdrop();
		/** The thin dark edge on text and shapes drawn with no panel behind them. */
		FLinearColor Outline();
		/** The weapon and ammo icons (FInkedIcon): the ink line, the light top and the shaded underside. */
		FLinearColor IconInk();
		FLinearColor IconLight();
		FLinearColor IconShade();
		/** The cloud bank's white: the transition screen between places, which the title card rises through. */
		FLinearColor Cloud();
	}

	enum class EShape : uint8
	{
		Panel,    // all four corners cut (big frames)
		Control,  // top-left and bottom-right cut (rows, buttons, plates)
		Tab,      // trapezoid title tab
		Bracket   // orange corner bracket (top + left strokes)
	};

	enum class EButtonKind : uint8
	{
		Normal,
		Primary,  // orange call to action (Resume, Pick up)
		Danger,   // destructive (Quit Game, Drop)
		Key,      // key-binding value
		Mini,     // low-emphasis (Default, small toggles)
		Tab,      // category tab; highlighted when selected
		Bare      // no look of its own: the content draws it (loadout cards)
	};

	FSlateFontInfo Font(int32 Size, bool bBold = true, int32 LetterSpacing = 0);

	/** Bold, with the thin dark outline of text drawn with no panel behind it (StyleFloatingText's font, for Slate widgets). */
	FSlateFontInfo FloatingFont(int32 Size, int32 LetterSpacing = 0);

	/**
	 * The kit's display type, for title cards (REVENANT rising through the white of the cloud bank): Chakra Petch Bold set
	 * big and widely spaced, outlined like floating text, with no panel behind it. LetterSpacing is in thousandths of an em.
	 */
	FSlateFontInfo DisplayFont(int32 Size = 72, int32 LetterSpacing = 560);

	/** One of the generated chamfer shapes (fill or 1-2px outline), tinted. Draws as a 9-slice box. */
	FSlateBrush ShapeBrush(EShape Shape, bool bOutline, const FLinearColor& Tint);

	/** Plain rectangle with optional outline (no texture). */
	FSlateBrush RectBrush(const FLinearColor& Fill, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f);

	/** A circle filling its widget (any size), with an optional outline ring. With a texture, the texture is clipped to the circle. */
	FSlateBrush CircleBrush(const FLinearColor& Fill, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f,
		UObject* Texture = nullptr);

	/** Map markers: generated white shapes with a dark edge baked in, tinted per use. */
	enum class EMarker : uint8
	{
		Arrow,    // the player, pointing up
		Diamond,  // hostiles
		Dot       // loot
	};
	FSlateBrush MarkerBrush(EMarker Marker, const FLinearColor& Tint);

	/**
	 * A small vector drawing (weapon silhouettes, ammo glyphs) in its own view box, like an SVG: filled convex polygons and
	 * stroked polylines. Points run clockwise on screen (y down); repeat a stroke's first point at its end to close it.
	 */
	struct FVectorIcon
	{
		FVector2D ViewBox = FVector2D(24.f, 24.f);
		TArray<TArray<FVector2D>> Fills;
		TArray<TArray<FVector2D>> Strokes;
		float StrokeWidth = 1.5f;
	};

	/**
	 * The icon drawn white (anti-aliased) into a texture, made once per name and resolution and then shared, as a brush of
	 * the given size tinted with Tint. PixelsPerUnit sets the texture's resolution: aim for about twice the drawn size.
	 */
	FSlateBrush IconBrush(FName Name, const FVectorIcon& Icon, float PixelsPerUnit, const FVector2D& Size, const FLinearColor& Tint);

	/**
	 * An icon in the Inked style, the game's weapon and ammo icons (the user's pick): a light top and a shaded underside
	 * inside a thick dark ink line, so it reads over any background. Made in Art/Icons/InkedIcons.py, which generates them
	 * as C++ (UI/Style/InkedIconData.inl) and whose docstring defines these fields; coordinates are x right, y down.
	 */
	struct FInkedIcon
	{
		FVector2D ViewBox = FVector2D(24.f, 24.f);
		/** Filled outlines, any simple polygon either way round; overlapping ones fill their union. */
		TArray<TArray<FVector2D>> Shapes;
		/** Polygons cut through the shapes. */
		TArray<TArray<FVector2D>> Holes;
		/** Open polylines drawn as part of the shape, StrokeWidth wide. */
		TArray<TArray<FVector2D>> Strokes;
		/** Detail lines drawn in ink across the fill, CutWidth wide. */
		TArray<TArray<FVector2D>> Cuts;
		float StrokeWidth = 0.6f;
		float CutWidth = 0.35f;
		/** The fill is light above this line and shaded below it. */
		float ShadeY = 0.f;
		/** The ink line round the shape; drawn small it grows to stay readable, up to MaxOutlineWidth. */
		float OutlineWidth = 1.f;
		float MaxOutlineWidth = 1.f;
	};

	/**
	 * The icon drawn in its own colors into a texture (made once per name and resolution, then shared), as a brush of the
	 * given size: keep the tint white, grey to dim it, alpha to fade it. PixelsPerUnit sets the texture's resolution: aim
	 * for about twice the drawn size.
	 */
	FSlateBrush InkedIconBrush(FName Name, const FInkedIcon& Icon, float PixelsPerUnit, const FVector2D& Size,
		const FLinearColor& Tint = FLinearColor::White);

	/** The icon as large as fits inside Box (keeping its shape), drawn at twice that size. */
	FSlateBrush InkedIconBrush(FName Name, const FInkedIcon& Icon, const FVector2D& Box, const FLinearColor& Tint = FLinearColor::White);

	FButtonStyle ButtonStyle(EButtonKind Kind, bool bHighlighted);
	FLinearColor ButtonTextColor(EButtonKind Kind, bool bHighlighted);
	FLinearColor ButtonLineColor(EButtonKind Kind, bool bHighlighted);

	// --- UI transparency (the settings menu's slider) ---

	/**
	 * Sets how solid panel backgrounds are drawn, for every UI at once and live: 1 = as designed, 0 = fully see-through.
	 * Only backgrounds registered with MarkBackground fade (glass, frames, rows, plates, screen dimming); text, outlines,
	 * bars, icons and buttons stay as they are, so every UI stays readable at any setting.
	 */
	void SetBackgroundOpacity(float Opacity);

	/**
	 * Makes a widget a background that follows the transparency setting from now on: an image fades as a whole; a border
	 * fades only its own brush, never its content. The kit's builders mark their backgrounds themselves.
	 */
	void MarkBackground(UWidget* Widget);

	// --- Widget builders (construct inside the given widget tree) ---

	/** Text in the kit style. bUpper uppercases the string (labels, headers). */
	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& Color, bool bUpper = false, int32 LetterSpacing = 0);

	/**
	 * Styles text drawn with no panel behind it (the gameplay HUD, damage numbers, loot labels): bold, with a thin dark
	 * outline so it reads against sky, snow or grass. Call it again to restyle (a label growing when focused).
	 */
	void StyleFloatingText(UTextBlock* Text, int32 Size, const FLinearColor& Color, int32 LetterSpacing = 0,
		ETextJustify::Type Justify = ETextJustify::Center);

	/** A new text block styled by StyleFloatingText. */
	UTextBlock* MakeFloatingText(UWidgetTree* Tree, int32 Size, const FLinearColor& Color, int32 LetterSpacing = 0,
		ETextJustify::Type Justify = ETextJustify::Left);

	/** An image of Brush that ignores the mouse. */
	UImage* MakeImage(UWidgetTree* Tree, const FSlateBrush& Brush);

	/** Content in a size box. A Width or Height of 0 leaves that side to the content. */
	USizeBox* MakeSized(UWidgetTree* Tree, UWidget* Content, float Width, float Height = 0.f);

	/** Makes an overlay child fill the whole overlay, inset by Padding. */
	void FillOverlaySlot(UOverlaySlot* Slot, const FMargin& Padding = FMargin(0.f));

	/** A chamfered box: shape fill + outline, with Content padded inside. */
	UWidget* MakeShapeBox(UWidgetTree* Tree, EShape Shape, const FLinearColor& Fill, const FLinearColor& Line, UWidget* Content, const FMargin& Padding);

	/** Full framed window: gunmetal frame, title tab, clamps, corner brackets, scanline glass. */
	UWidget* MakePanel(UWidgetTree* Tree, const FString& Title, UWidget* Content, UWidget* CornerButton = nullptr);

	/** Orange, letter-spaced section header ("MOVEMENT"). */
	UWidget* MakeSection(UWidgetTree* Tree, const FString& Label);

	/** Row container used for list entries. */
	UWidget* MakeRow(UWidgetTree* Tree, UWidget* Content);

	/** Segmented bar (health, stats). */
	UWidget* MakeSegmentBar(UWidgetTree* Tree, int32 Segments, float Fraction, const FLinearColor& OnColor, float Height = 10.f);

	/** Small HUD plate (control-shaped, translucent glass). */
	UWidget* MakePlate(UWidgetTree* Tree, UWidget* Content, const FMargin& Padding = FMargin(12.f, 8.f));

	/** Thin cyan scrollbar that matches the kit. */
	void StyleScrollBox(UScrollBox* ScrollBox);
}
