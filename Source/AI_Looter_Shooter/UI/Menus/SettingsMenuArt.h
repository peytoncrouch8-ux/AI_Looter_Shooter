#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Widgets/SLeafWidget.h"
#include "SettingsMenuArt.generated.h"

class ULooterButton;
class USlider;
class UWidgetTree;

/**
 * The pieces of the settings menu's look (round 3, the user's mix of "Ledger & Brass" and "Halo Reticle", 2026-10-29),
 * each painted to its widget's size as vertices and lines (no textures). A piece that belongs to a button or a slider
 * reads it as it paints (chosen, pointed at, focused, its value), so the look follows them with nothing to update.
 */
enum class ESettingsArt : uint8
{
	/** The plate's slate glass, chamfered (a background: mark it). */
	PlateFill,
	/** The plate's double trim (a salt line on a dark edge, a fainter one inside it) and a rivet in each corner. */
	PlateLines,
	/** A choice (c3): a hollow salt outline, the chosen one gold with a gold word and a tick above and below. Its word painted. */
	Chip,
	/** A tab's word: gold while its page shows, white pointed at, else the label grey. */
	Tab,
	/** Save & Quit's rubber stamp: a red double outline and its red spaced word (the button is tilted). */
	Stamp,
	/** The stamp's light red wash (a background: mark it). */
	StampWash,
	/** The brass ribbon, a swallowtail pointing right, its tails out past the plate's left edge. */
	Ribbon,
	/** A ledger row's ruled salt hairline along its foot. Hit-testable, so the whole row is pointed at. */
	LedgerLine,
	/** The ledger's ruled margin: a gold double line down the page at MarginX. */
	MarginRule,
	/** A lit row's gold wash from the margin out (a background: mark it). */
	RowWash,
	/** A lit row's gold double rule along its foot, a ledger total's. */
	RowRule,
	/** Resume's soft gold wash between its brackets, stronger pointed at or focused (a background: mark it). */
	ResumeWash,
	/** A slider's ruler track (ticks up from the line, a gold run to the knob) and the end labels under it. */
	SliderRuler,
	/** The stamped plate beside the title: a thin gold double box and its gold spaced words. */
	AreaPlate,
	/** The description column's short vertical ruler. */
	ColumnRuler,
	/** Under the lit tab: the ruler's gold run and a small gold triangle pointing up at the tab. */
	TabMark,
};

/** The painter. In UMG it is USettingsMenuArt. */
class AI_LOOTER_SHOOTER_API SSettingsMenuArt : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSettingsMenuArt) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetKind(ESettingsArt InKind);
	/** The words it paints (a chip, a tab, the stamp, the plate), their size and letter spacing (thousandths of an em). */
	void SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing);
	/** The button whose state the piece shows (a chip, a tab, the stamp, Resume's wash). */
	void SetButton(ULooterButton* InButton);
	/** The slider a ruler track follows, its end labels, and its knob's width (the track runs between the knob's halves). */
	void SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth);
	/** Where the ledger's margin rule runs (MarginRule, RowWash, RowRule). */
	void SetMarginX(float InMarginX);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

private:
	ESettingsArt Kind = ESettingsArt::PlateFill;
	FString Label;
	int32 FontSize = 14;
	int32 LetterSpacing = 0;
	TWeakObjectPtr<ULooterButton> Button;
	TWeakObjectPtr<USlider> Slider;
	FString MinLabel;
	FString MaxLabel;
	float KnobWidth = 24.f;
	float MarginX = 0.f;
};

/** SSettingsMenuArt in UMG. */
UCLASS()
class AI_LOOTER_SHOOTER_API USettingsMenuArt : public UWidget
{
	GENERATED_BODY()

public:
	/** A new piece of Kind in Tree; it ignores the mouse (but a LedgerLine, which takes it for its row). */
	static USettingsMenuArt* Make(UWidgetTree* Tree, ESettingsArt Kind);

	void SetKind(ESettingsArt InKind);
	void SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing);
	void SetButton(ULooterButton* InButton);
	void SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth);
	void SetMarginX(float InMarginX);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	void Apply();

	TSharedPtr<SSettingsMenuArt> Art;
	ESettingsArt Kind = ESettingsArt::PlateFill;
	FString Label;
	int32 FontSize = 14;
	int32 LetterSpacing = 0;
	TWeakObjectPtr<ULooterButton> Button;
	TWeakObjectPtr<USlider> Slider;
	FString MinLabel;
	FString MaxLabel;
	float KnobWidth = 24.f;
	float MarginX = 0.f;
};

/** The settings menu's sizes and pictures that aren't LooterReticleArt's. */
namespace SettingsMenuArt
{
	/** How strong the dark edge under every line is (the mockups' .65 on the kit's outline colour), and how much wider. */
	inline constexpr float EdgeStrength = 0.75f;
	inline constexpr float EdgeExtra = 3.f;
	/** The plate's chamfer, the trim's inset, the rivets' inset from the corners. */
	inline constexpr float PlateCut = 16.f;
	inline constexpr float TrimInset = 10.f;
	inline constexpr float RivetInset = 28.f;
	/** A chip's chamfer and the room either side of its word; a chip is never narrower than ChipMinWidth. */
	inline constexpr float ChipCut = 7.f;
	inline constexpr float ChipPadX = 12.f;
	inline constexpr float ChipHeight = 36.f;
	inline constexpr float ChipMinWidth = 56.f;
	/** The ribbon (c1: 100 x 38; shortened so it ends before the row numbers in the margin). */
	inline constexpr float RibbonWidth = 64.f;
	inline constexpr float RibbonHeight = 38.f;
	/** Where the ribbon's tails start, in the plate's space: out past its left edge. */
	inline constexpr float RibbonLeft = -30.f;
	/** The slider's ring knob. */
	inline constexpr float KnobSize = 26.f;
	/** The lit tab's mark: the gold run under the tab and the triangle under its middle. */
	inline constexpr float TabMarkWidth = 140.f;
	inline constexpr float TabMarkHeight = 18.f;

	/** The halo's two gold arcs (c3: -60 to 0 and 120 to 180 degrees) at Radius, in a square view box round them. */
	LooterUI::FPaintedIcon HaloArcs(float Radius);
	/** The slider's knob: a ring of gold round the gem's dark, a gold dot in the middle. KnobSize square. */
	LooterUI::FPaintedIcon Knob();

	/** "SER. RV-0001-07": the session (0 with none) and the player's level, the plate's serial. */
	FString SerialText(int32 Session, int32 Level);
	/** "NO. 01 · RANSOM'S REST", or the area alone with no session. */
	FString PlateText(int32 Session, const FString& Area);
}
