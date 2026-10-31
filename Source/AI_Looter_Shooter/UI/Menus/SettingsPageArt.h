#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Widgets/SLeafWidget.h"
#include "SettingsPageArt.generated.h"

class ULooterButton;
class UOverlay;
class USettingsPageArt;
class USlider;
class UWidgetTree;

/**
 * The full-page settings' painted pieces (E2 "Big Dial", the user's pick 2026-10-30; Docs/Archive/Work/2026-10-30-PauseMenu,
 * mockups/e.js MODE settings), each painted to its widget's size as vertices and lines, so it is crisp at any size and needs
 * no texture. A piece that belongs to a button or a slider reads its state as it paints (chosen, pointed at, focused, its
 * value), so the look follows with nothing to update. The big shapes (the dial, the pennants, the help card, the gems) are
 * PauseMenuArt's pictures; these are the pieces that stretch with the page.
 */
enum class ESettingsPageArt : uint8
{
	/** A banded row: its glass (Back; the darker band with SetOdd) and its salt hairline along the foot (Front). */
	RowBand,
	/** The lit row: a gold wash fading to the right (Back) and its 5 px gold edge at the left (Front). */
	RowLight,
	/** A switch button: a box cut at its top-left and bottom-right; its fill (Back), its line and word (Front). Gold when chosen. */
	Switch,
	/** A slider's ruler: ticks hanging under a salt line, the gold run out to the knob, the end values under the ends. */
	SliderTrack,
	/** The ruler under SETTINGS: a faint salt line with ticks hanging every 10.7 px, every fifth long. */
	TitleRuler,
	/** The long ruler the tabs hang from: the same on a dark edge, its long ticks stronger. */
	TabRuler,
	/** The tick a tab hangs on, from the ruler down to its hook: gold while its page shows. */
	HangTick,
	/** A hollow key cap: rounded, a 3.5 px gold line round faint glass (Back) and its word (Front). */
	KeyCap,
	/** The help card's rule: salt lines either side of a small gold diamond. */
	Ornament,
	/** The page's shade over the game or the main menu (Back only): the backdrop's dark, heaviest at the left and behind the rows,
	 *  thinning toward the right edge (e.js's page shade over its dimmed, blurred world). */
	Shade,
};

/** Which layers of a piece a widget paints: the backgrounds go in a widget of their own, so the UI transparency fades them alone. */
enum class ESettingsPagePart : uint8
{
	All,
	/** Fills only (mark the widget with LooterUI::MarkBackground). */
	Back,
	/** Lines and words only. */
	Front,
};

/** The painter. In UMG it is USettingsPageArt. */
class AI_LOOTER_SHOOTER_API SSettingsPageArt : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSettingsPageArt) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetKind(ESettingsPageArt InKind, ESettingsPagePart InPart);
	/** The word it paints (a switch, a key cap), its size and letter spacing (thousandths of an em). */
	void SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing);
	/** The button whose state it shows (a switch, a tab's tick, a key cap that is a button). */
	void SetButton(ULooterButton* InButton);
	/** The slider a track follows, its end values, and the knob's width (the track runs between the knob's halves). */
	void SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth);
	/** The darker of the two bands (RowBand). */
	void SetOdd(bool bInOdd);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	ESettingsPageArt Kind = ESettingsPageArt::RowBand;
	ESettingsPagePart Part = ESettingsPagePart::All;
	FString Label;
	int32 FontSize = 24;
	int32 LetterSpacing = 0;
	TWeakObjectPtr<ULooterButton> Button;
	TWeakObjectPtr<USlider> Slider;
	FString MinLabel;
	FString MaxLabel;
	float KnobWidth = 36.f;
	bool bOdd = false;
};

/** SSettingsPageArt in UMG. */
UCLASS()
class AI_LOOTER_SHOOTER_API USettingsPageArt : public UWidget
{
	GENERATED_BODY()

public:
	/**
	 * A new piece of Kind in Tree, painting Part of it. It ignores the mouse, but a row band's Front, which takes it for its
	 * whole row (so pointing anywhere on a row marks it). A Back piece is marked as a background here.
	 */
	static USettingsPageArt* Make(UWidgetTree* Tree, ESettingsPageArt Kind, ESettingsPagePart Part = ESettingsPagePart::All);

	void SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing);
	const FString& GetLabel() const { return Label; }
	void SetButton(ULooterButton* InButton);
	void SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth);
	void SetOdd(bool bInOdd);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	void Apply();

	TSharedPtr<SSettingsPageArt> Art;
	ESettingsPageArt Kind = ESettingsPageArt::RowBand;
	ESettingsPagePart Part = ESettingsPagePart::All;
	FString Label;
	int32 FontSize = 24;
	int32 LetterSpacing = 0;
	TWeakObjectPtr<ULooterButton> Button;
	TWeakObjectPtr<USlider> Slider;
	FString MinLabel;
	FString MaxLabel;
	float KnobWidth = 36.f;
	bool bOdd = false;
};

/** The full page's sizes (the mockup's px, Slate units as they are) and its small pictures. */
namespace SettingsPageArt
{
	// --- The rows (e.js: rh 132, the number 92 wide, 34 px clear of the right end) ---

	inline constexpr float RowHeight = 132.f;
	/** A key list row: the same band, shorter, so a long list reads at a glance. */
	inline constexpr float KeyRowHeight = 80.f;
	inline constexpr float NumberWidth = 92.f;
	inline constexpr float RowPadRight = 34.f;
	/** The lit row's gold edge at its left (the mockup's inset 5 px). */
	inline constexpr float LitEdgeWidth = 5.f;

	// --- A switch button (e.js: 58 tall, its word's width + 46, cut 12 at the top-left and bottom-right, 14 apart) ---

	inline constexpr float SwitchHeight = 58.f;
	inline constexpr float SwitchPadX = 23.f;
	inline constexpr float SwitchCut = 12.f;
	inline constexpr float SwitchGap = 14.f;
	inline constexpr float SwitchMinWidth = 58.f;

	// --- The slider (e.js: a 380 px ruler, its line 20 px down, a 30 px diamond knob, the value 34 px bold) ---

	/** The ruler a step shorter than the mockup's 380, so the longest row name (Field of view (1st person)) fits on one line. */
	inline constexpr float SliderTrackWidth = 320.f;
	/** The end values sit under the ruler's ends, reaching this far past them at most, clear of the value column. */
	inline constexpr float EndLabelOverhang = 6.f;
	inline constexpr float SliderHeight = 60.f;
	inline constexpr float SliderLineY = 20.f;
	/** The knob's picture, a side: the diamond and its dark edge. */
	inline constexpr float KnobSize = 36.f;

	// --- The tabs (e.js: the ruler's line 14 px down the strip, the pennants 300 x 92 from 36 px down) ---

	inline constexpr float TabRulerLineY = 14.f;
	inline constexpr float TabHangBottom = 36.f;
	inline constexpr float PennantWidth = 300.f;
	inline constexpr float PennantHeight = 92.f;

	// --- The title's ruler (e.js lockup: 520 px, a tick every 10.7, every fifth 12 long, the rest 6) ---

	inline constexpr float TitleRulerWidth = 520.f;
	inline constexpr float TitleRulerHeight = 14.f;
	inline constexpr float RulerStep = 10.7f;

	// --- Key caps (common.css .cap: 40 tall, 46 at least, 11 px in from the sides, a 3.5 px line, 9 px round) ---

	inline constexpr float KeyCapHeight = 40.f;
	inline constexpr float KeyCapMinWidth = 46.f;
	inline constexpr float KeyCapPadX = 11.f;
	inline constexpr int32 KeyCapFontSize = 19;
	inline constexpr int32 KeyCapSpacing = 40;

	/** The ornament's height (the card's rule). */
	inline constexpr float OrnamentHeight = 14.f;

	/**
	 * Kind's Back and Front pieces filling a new overlay (the Back one marked as a background), both following Owner's state
	 * when given. The Front piece paints the words: give it the label (OutFront).
	 */
	UOverlay* MakeLayered(UWidgetTree* Tree, ESettingsPageArt Kind, ULooterButton* Owner, USettingsPageArt*& OutBack, USettingsPageArt*& OutFront);

	/** A key cap with Label (a hint's key, PGUP), Owner's when it is part of a button. Returns the cap; OutFront paints the label. */
	UWidget* MakeKeyCap(UWidgetTree* Tree, const FString& Label, ULooterButton* Owner, USettingsPageArt*& OutFront);

	/** The slider's knob: a diamond of glass in a gold line with a gold dot, on its dark edge. KnobSize square. */
	LooterUI::FPaintedIcon Knob();

	/**
	 * Icon with its light layers in Tint (keeping each layer's own alpha) and its dark edge layers as they were: one icon
	 * drawn gold for the lit tab and grey for the rest, whatever colours it was made in.
	 */
	LooterUI::FPaintedIcon Recoloured(const LooterUI::FPaintedIcon& Icon, const FLinearColor& Tint);

	/**
	 * Picture's backgrounds (fill-only layers that are dark or see-through: glass, a gold wash), or with bBack false the rest
	 * (its lines, edges and gems), in the same view box: the two laid over each other are the picture, and the back alone is
	 * marked for the UI transparency.
	 */
	LooterUI::FPaintedIcon PictureLayers(const LooterUI::FPaintedIcon& Picture, bool bBack);
}
