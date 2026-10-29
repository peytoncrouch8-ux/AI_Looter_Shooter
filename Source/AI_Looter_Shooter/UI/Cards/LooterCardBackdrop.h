#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/Cards/GunCardStyle.h"
#include "Widgets/SLeafWidget.h"
#include "LooterCardBackdrop.generated.h"

/** Which shape a backdrop draws (the "Wide Sweep" look, Docs/Work/2026-10-28-UICharacter/CONTRACT.md). */
enum class ELooterBackdropShape : uint8
{
	/** A card: top-right and bottom-left cut 16 px; the maker's sweep top-left; hairlines along the top and the cuts. */
	Card,
	/** A list row: the top-right cut 12 px; lit, an orange wash with an orange outline. */
	Row,
	/**
	 * The gun card's tag (the user's pick, 2026-10-28 evening): the pennant's notch on top, the toe tag's cut shoulders and
	 * its point down to the gun, a 4 px salt line round it, the level's gem set in the notch. Slate & Salt colours.
	 */
	Tag
};

/** Which part of the shape a backdrop paints: the glass (a background: it fades with the UI transparency) or its lines. */
enum class ELooterBackdropPart : uint8
{
	/** The glass fill only (mark this widget with LooterUI::MarkBackground). */
	Fill,
	/** The lines, the sweep, the ticks and the shine: they stay solid whatever the UI transparency. */
	Lines
};

/** A row's state. */
enum class ELooterRowState : uint8
{
	/** Dark glass. */
	Normal,
	/** Under the cursor or chosen: an orange wash, an orange outline and an orange tick on its left. */
	Lit,
	/** An empty slot: fainter glass. */
	Empty
};

/**
 * The kit's card and row backdrop, painted to any size (no textures): a card's glass, cuts, hairlines and its maker's
 * sweep (GunCardStyle), or a row's. Use two in an overlay, one Fill under the content (marked as a background) and one
 * Lines over or under it. In UMG it is ULooterCardBackdrop.
 */
class AI_LOOTER_SHOOTER_API SLooterCardBackdrop : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLooterCardBackdrop) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetShape(ELooterBackdropShape InShape);
	void SetPart(ELooterBackdropPart InPart);
	/** The card's sweep (null: none, a card with no maker band). The style must outlive the widget (GunCardStyle's do). */
	void SetSweep(const GunCardStyle::FStyle* InSweep);
	/** The sweep's shine (GunCardRules::ShinePosition): drawn while 0-1. */
	void SetShine(float InShine);
	void SetRowState(ELooterRowState InState);
	/** A row's tick on its left when it isn't lit (a gun's rarity); transparent for none. */
	void SetRowTick(const FLinearColor& InTick);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(8.f, 8.f); }

private:
	ELooterBackdropShape Shape = ELooterBackdropShape::Card;
	ELooterBackdropPart Part = ELooterBackdropPart::Fill;
	const GunCardStyle::FStyle* Sweep = nullptr;
	float Shine = -1.f;
	ELooterRowState RowState = ELooterRowState::Normal;
	FLinearColor RowTick = FLinearColor::Transparent;
};

/**
 * The geometry behind SLooterCardBackdrop, apart from the widget so it can be tested: the outlines as points (y down) for a
 * box of a size.
 */
namespace LooterBackdrop
{
	/** The card's cut, in px. */
	inline constexpr float CardCut = 16.f;
	inline constexpr float RowCut = 12.f;
	/** The sweep's height, and its length's share of the card's width (276 of 420). */
	inline constexpr float SweepHeight = 28.f;
	inline constexpr float SweepShare = 0.657f;
	/** The lines' width. */
	inline constexpr float LineWidth = 2.f;

	/** The tag's room above its top edge for the level gem, its shoulders' cut, its point's depth, its notch, its line. */
	inline constexpr float TagTop = 16.f;
	inline constexpr float TagShoulder = 26.f;
	inline constexpr float TagPoint = 28.f;
	inline constexpr float TagNotchOuter = 46.f;
	inline constexpr float TagNotchInner = 34.f;
	inline constexpr float TagNotchDepth = 11.f;
	inline constexpr float TagLine = 4.f;
	/** The gem: a diamond this many px across, centred on the notch, its outline. */
	inline constexpr float TagGem = 46.f;
	inline constexpr float TagGemLine = 3.5f;

	/** The tag's outline for a box of a size (its top edge TagTop down), clockwise from the left shoulder. */
	AI_LOOTER_SHOOTER_API TArray<FVector2f> TagOutline(const FVector2f& Size);
	/** Where the gem's centre sits in that box. */
	AI_LOOTER_SHOOTER_API FVector2f TagGemCenter(const FVector2f& Size);

	/** The card's outline, clockwise from the top-left (six points: the two cuts). */
	AI_LOOTER_SHOOTER_API TArray<FVector2f> CardOutline(const FVector2f& Size);
	/** A row's outline, clockwise from the top-left (five points: the top-right cut). */
	AI_LOOTER_SHOOTER_API TArray<FVector2f> RowOutline(const FVector2f& Size);
	/** The sweep's outline for a card Width wide: straight, then the S-curve down to its foot (enough points to look smooth). */
	AI_LOOTER_SHOOTER_API TArray<FVector2f> SweepOutline(float Width);
	/** Where the sweep's foot ends (its length) on a card Width wide. */
	AI_LOOTER_SHOOTER_API float SweepLength(float Width);
}

/** SLooterCardBackdrop in UMG. It ignores the mouse. */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterCardBackdrop : public UWidget
{
	GENERATED_BODY()

public:
	void SetShape(ELooterBackdropShape InShape);
	void SetPart(ELooterBackdropPart InPart);
	void SetSweep(const GunCardStyle::FStyle* InSweep);
	void SetShine(float InShine);
	void SetRowState(ELooterRowState InState);
	ELooterRowState GetRowState() const { return RowState; }
	void SetRowTick(const FLinearColor& InTick);

	/**
	 * Two backdrops for a card or row, in an overlay that fills: Fill (marked as a background) then Lines, and the Content
	 * over both with Padding. Returns the overlay; OutFill and OutLines are the two (to restyle later).
	 */
	static class UOverlay* MakeLayered(class UWidgetTree* Tree, ELooterBackdropShape Shape, UWidget* Content, const FMargin& Padding,
		ULooterCardBackdrop*& OutFill, ULooterCardBackdrop*& OutLines);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	void Apply();

	TSharedPtr<SLooterCardBackdrop> Backdrop;
	ELooterBackdropShape Shape = ELooterBackdropShape::Card;
	ELooterBackdropPart Part = ELooterBackdropPart::Fill;
	const GunCardStyle::FStyle* Sweep = nullptr;
	float Shine = -1.f;
	ELooterRowState RowState = ELooterRowState::Normal;
	FLinearColor RowTick = FLinearColor::Transparent;
};
