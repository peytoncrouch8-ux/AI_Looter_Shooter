#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudShieldBarWidget.generated.h"

class UCanvasPanel;
class UImage;
class UPlayerShieldComponent;
class UTextBlock;

/** The ward bar's pictures, each drawn once into a shared texture (HudShieldBarWidgetPictures.cpp). */
enum class EHudShieldPicture : uint8
{
	Back,       // the ink rim and gunmetal bezel round the track (its track cut out)
	Track,      // the dark track (a background)
	FillBody,   // the fill: two cyan bands, the lit top line, the light leading edge at the far end
	FillCap,    // the fill's slanted leading end
	WhiteBody,  // the track's shape in white, tinted: the chip, the flashes, the broken ward's pulse
	WhiteCap,
	Shard,      // a sliver of the ward flying off as it breaks
	Count
};

/**
 * The player's ward as a slim slanted bar over the health bar (SHIELD, 2026-10-22; Docs/Work/2026-10-22-Next/SHIELD.md),
 * in the kit's cyan, built like the frame's bars (pictures cropped to a level, an end piece along the HUD's 16 degree lean,
 * so the leading edge stays smooth). Its left end tucks behind the medallion as the health bar's does, and the charge's
 * number floats after its right end. It reads the pawn's UPlayerShieldComponent itself each frame:
 *  - a hit: the fill drops at once, what it took lingers pale (the chip), then drains
 *  - the break: a white flash along the whole bar and slivers flying off it; then the empty track pulses faintly in cyan
 *    until it starts to fill
 *  - refilling: the fill rises with the charge, a shimmer sweeping along it
 *  - full again: a quick bright flash; Saint's Grace a warm one
 *  - no ward worn: the bar isn't there
 * The player frame (UHudPlayerFrameWidget) holds it in its canvas behind the medallion and wakes from its idle fade when
 * the ward changes (ConsumeActivity) or isn't full (IsShowingLoss). Tools\hudshots.ps1 photographs the frame with it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudShieldBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Where it sits: 1080p pixels, the bar's box top-left before the lean (as the frame's HealthBarX/Y) ---
	/** Over the health bar (its box at 130, 933): 5 px above it, its left end behind the medallion's bezel. */
	static constexpr float BarX = 128.f;
	static constexpr float BarY = 915.f;

	// --- Its measures (pixels at 1080p from the bar's box top-left before the lean) ---
	static constexpr float Length = 540.f;
	static constexpr float Thickness = 13.f;
	static constexpr float Mid = Thickness * 0.5f;
	static constexpr float Rim = 1.3f;
	static constexpr float Inset = 1.4f;
	/** The track starts this far in: where the medallion's bezel crosses the bar's middle (an empty bar still starts there, as the health bar does). */
	static constexpr float TrackLeft = Rim + 4.3f;
	static constexpr float TrackRight = Length - Rim - Inset;
	static constexpr float TrackTop = Rim + Inset;
	static constexpr float TrackBottom = Thickness - TrackTop;
	static constexpr float TrackHeight = TrackBottom - TrackTop;
	/** The pictures' box round the bar (room for the lean and the rim); an end piece's box round its cut. */
	static constexpr float BoxLeft = -8.f;
	static constexpr float BoxTop = -6.f;
	static constexpr float BoxWidth = Length + 20.f;
	static constexpr float BoxHeight = Thickness + 12.f;
	static constexpr float CapLeft = -4.f;
	static constexpr float CapWidth = 10.f;
	static constexpr float CapOverlap = 1.f;
	/** The shimmer: about a fifth of the bar, as tall as the track. */
	static constexpr float ShimmerLength = 110.f;
	/** The charge's number after the bar's end. */
	static constexpr float NumberGap = 9.f;
	static constexpr int32 NumberSize = 12;
	/** The widget's own box: the pictures' and the number's room after them. */
	static constexpr float Width = BoxWidth + 48.f;
	static constexpr float Height = BoxHeight;

	/** Something happened since the last call (a hit, a break, a refill, a grace): the frame wakes from its idle fade. */
	bool ConsumeActivity();
	/** A ward is worn and isn't full: the frame stays at full strength. */
	bool IsShowingLoss() const { return bShown && FillShown < 0.999f; }
	/** What it shows now (tests): shown at all, the fill and the chip (0-1 of the bar). */
	bool IsShown() const { return bShown; }
	float GetFillShown() const { return FillShown; }
	float GetChipShown() const { return ChipShown; }

	/**
	 * Follows the owning pawn's ward (UPlayerShieldComponent) this frame: the player frame calls it from its tick, so the bar
	 * keeps watching while it is hidden (no ward worn) and Slate wouldn't tick it.
	 */
	void Update(float DeltaTime);

	/** Shows Shield as it is now (Update calls it with the pawn's; tests call it with theirs). Null: no ward. */
	void ShowShield(const UPlayerShieldComponent* Shield, float DeltaTime);

	/** A picture as a brush its own size, and where its top-left sits in the widget (HudShieldBarWidgetPictures.cpp). */
	static FSlateBrush PictureBrush(EHudShieldPicture Picture);
	static FVector2D PicturePosition(EHudShieldPicture Picture);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** A body cropped to a level and its end piece moved along to it. */
	struct FStretch
	{
		UImage* Body = nullptr;
		UImage* Cap = nullptr;
		FSlateBrush Whole;
		/** Where its end is drawn (bar pixels), or a marker for hidden / whole / not drawn yet. */
		float Shown = -2000.f;
	};

	FStretch AddStretch(UCanvasPanel* Root, EHudShieldPicture BodyPicture, EHudShieldPicture CapPicture, const FLinearColor& Tint);
	/** Shows a stretch from the bar's start up to Amount (0-1); repaints only when its end moves a pixel. */
	void PaintStretch(FStretch& Stretch, float Amount);
	static void TintStretch(const FStretch& Stretch, const FLinearColor& Tint);
	/** The shimmer Progress (0-1) of the way along a fill of Amount, cut off at the fill's ends. */
	void PaintShimmer(float Progress, float Amount);
	/** The break's slivers: thrown off along the fill as it was (From, 0-1 of the bar), each tick moved and faded. */
	void ThrowShards(float From);
	void TickShards(float DeltaTime);
	/** Snaps to Share with nothing lingering (first sight, a new ward). */
	void Snap(float Share);

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TObjectPtr<UImage> Shimmer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Number;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Shards;

	FStretch Chip;
	FStretch Fill;
	FStretch Flash;
	FSlateBrush ShimmerBrush;
	bool bShimmerShown = false;

	TWeakObjectPtr<const UPlayerShieldComponent> Watched;
	bool bShown = false;
	bool bActivity = false;
	float FillShown = 0.f;
	float ChipShown = 0.f;
	float ChipFrom = 0.f;
	float ChipHold = 0.f;
	float ChipDrain = 0.f;
	/** Seconds into the bar's flash (break, full, grace), its colour, and the shimmer's clock. */
	float FlashAge = 1000.f;
	FLinearColor FlashColor = FLinearColor::White;
	float ShimmerClock = 0.f;
	/** The ward's counts as last seen (UPlayerShieldComponent::GetHitCount and the rest), and the identity of the ward. */
	uint32 SeenHits = 0;
	uint32 SeenBreaks = 0;
	uint32 SeenFills = 0;
	uint32 SeenGraces = 0;
	uint32 SeenWard = 0;
	int32 ShownPoints = INDEX_NONE;
	bool bShownBroken = false;

	struct FShard
	{
		FVector2D Position = FVector2D::ZeroVector;
		FVector2D Velocity = FVector2D::ZeroVector;
		float Spin = 0.f;
		float Angle = 0.f;
		float Age = 1000.f;
	};
	TArray<FShard> ShardMotion;
};
