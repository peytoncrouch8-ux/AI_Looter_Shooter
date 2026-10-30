#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Elements/ElementStatusIcons.h"
#include "Elements/LooterElement.h"
#include "CreatureHealthBarWidget.generated.h"

class UCanvasPanel;
class UHorizontalBox;
class UImage;
class ULooterProgressRing;
class USizeBox;
class UTextBlock;
class UWidget;

/**
 * The tag over a creature in combat, in the gameplay HUD's style: no backing plate, just floating outlined text ("LV 1
 * Brown Spider") over a slim slanted health bar with a dark rim and a lit top edge. A ranked creature's word comes before
 * its name in its rank's color ("LV 2 Restless Brown Spider", the word blue), and a rank with no word (a boss) colors the
 * name instead. The bar is the same over every creature whatever its name or health: solid dark lines cut it into
 * quarters, so a glance says how far gone it is without a tough creature's bar turning into noise. A hit leaves a pale
 * chip of the health it took that drains away a moment later. When a ranked creature turns on the player (its rank sting
 * sounds) its rank word pops: bigger and in the accent color, settling back to its own over RankFlashSeconds.
 *
 * Its tier (SetTier, SetTraitLine; UCreatureTierLookComponent::DressTag, the enemy tiers of 2026-10-20): Gravebound and
 * up name their traits in a small dim line under the name ("FLEET · WARDED"); Hallowed and Ethereal frame the bar in
 * their colour, a thin line round it with a clamp biting each end, and their sting (the notice) pops the word bigger and
 * longer (TopTierFlashSeconds) in a white-hot of its own colour while the frame flares bright and swells a little.
 *
 * A Warded creature's ward (SetWard; the traits' soul ward, UCreatureTraitComponent::GetWardFraction): a slim bar over the
 * health bar in the ward's element's colour (typed wards, 2026-10-29: ElementRules::UiColor of GetWardElement), leaning with
 * it, while the ward stands. A hit snaps its fill down and leaves a paler chip draining after it, as the health's; breaking,
 * it flashes white-hot of its colour, swells a little and goes.
 *
 * The element statuses on it (SetStatuses; UElementStatusComponent::GetStatusIcons): a row of small icons under the bar, one
 * per element in the elements' own order (so an icon never jumps as another runs out): the element's glyph on a dark disc
 * inside a ring of its colour that drains as the status runs down, a stack count beside it past one, popping in as it
 * starts. CreatureHealthBarWidgetStatus.cpp has the row.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Cheap to call every frame: the texts only change when one of these does. InRankWord is empty for Basic; InRankColor
	 * colors the word, or the name when there's no word (Basic's is the plain text color).
	 */
	void SetCreature(const FText& InName, int32 InLevel, const FText& InRankWord, const FLinearColor& InRankColor);

	/** Cheap to call every frame: the bar only moves when the share of health left changes. */
	void SetHealth(float Health, float MaxHealth);

	/**
	 * Cheap to call every frame: seconds since the creature's rank sting sounded (UCreaturePackComponent::GetRankStingAge;
	 * a large or negative number for never). While it is under RankFlashSeconds the rank word (the name, for a rank with no
	 * word) pops; the flash plays out from that age, so a tag shown late joins it part-way.
	 */
	void SetRankStingAge(float Seconds);

	/** How long the rank word's pop takes to settle. */
	static constexpr float RankFlashSeconds = 0.6f;

	/** The word's size at the start of the pop (1 = its own). */
	static constexpr float RankFlashPeakScale = 1.25f;

	/** The rank word's scale Seconds into the pop: the peak at once, easing down to 1 (1 before and after it). */
	static float RankFlashScaleAt(float Seconds);

	/** The rank word's color Seconds into the pop: the accent color at once, easing to Normal (Normal before and after it). */
	static FLinearColor RankFlashColorAt(float Seconds, const FLinearColor& Normal);

	// --- Its tier ---

	/**
	 * Cheap to call every frame (CreatureTierLook::TagFor): bThickFrame frames the bar in FrameColor with clamps at its
	 * ends (Hallowed, Ethereal); bTopTierFlash makes its sting the top tiers' bigger, longer pop with the frame's flare.
	 */
	void SetTier(bool bInThickFrame, const FLinearColor& InFrameColor, bool bInTopTierFlash);

	/** Cheap to call every frame: the small dim line of its traits under its name (empty: none, and the line collapses). */
	void SetTraitLine(const FText& InLine);

	bool HasThickFrame() const { return bThickFrame; }
	const FText& GetTraitLine() const { return TraitLine; }

	/** A top tier's sting: the word's pop lasts longer and starts bigger. */
	static constexpr float TopTierFlashSeconds = 0.9f;
	static constexpr float TopTierFlashPeakScale = 1.45f;

	/** The pop's length for a top tier's sting or anyone else's. */
	static float FlashSecondsFor(bool bTopTier) { return bTopTier ? TopTierFlashSeconds : RankFlashSeconds; }

	/** The word's scale and color Seconds into the pop: a top tier's (white-hot of its own colour), else the rank sting's. */
	static float TierFlashScaleAt(float Seconds, bool bTopTier);
	static FLinearColor TierFlashColorAt(float Seconds, const FLinearColor& Normal, bool bTopTier);

	/** How far a top tier's frame is flared Seconds into its sting: 1 at once, easing to 0 by TopTierFlashSeconds. */
	static float FrameFlareAt(float Seconds);

	/** The frame round the bar: its pad (px a side), its tier line's width, a clamp's width, its size at the flare's peak. */
	static constexpr float FramePad = 3.f;
	static constexpr float FrameLine = 1.5f;
	static constexpr float ClampWidth = 4.f;
	static constexpr float FrameFlareScale = 1.12f;

	/** The trait line's type: under the level's (10), dim and spaced as a label. */
	static constexpr int32 TraitTextSize = 9;

	// --- A Warded creature's ward ---

	/**
	 * Cheap to call every frame: whether it's Warded, the share of its ward left (0 to 1) and the ward's colour (its
	 * element's UiColor). Shown while the ward stands (a ward whole again, a new life, shows full at once); a lower share
	 * snaps the fill down and leaves the chip; 0 while shown breaks it (the flash, then gone). Not Warded: no ward bar, at
	 * once and with no flash. The colour is taken whenever it changes, so a break flashes in the ward's own colour.
	 */
	void SetWard(bool bInWarded, float InFraction, const FLinearColor& InColor);

	/** The same in the kit's cyan (DefaultWardColor): a ward with no element. */
	void SetWard(bool bInWarded, float InFraction) { SetWard(bInWarded, InFraction, DefaultWardColor()); }

	/** Moves the ward bar's chip and its break's flash on by DeltaSeconds (its tick calls it; the tests call it). */
	void AdvanceWard(float DeltaSeconds);

	bool IsWardShown() const { return bWardShown; }
	bool IsWardBreaking() const { return WardBreakClock < WardBreakSeconds; }
	/** The ward bar's fill and its chip's end, as shares of the bar. */
	float GetWardFill() const { return WardFraction; }
	float GetWardChip() const { return WardGhost; }

	/** The ward's fill colour as last set, and its chip's (a paler of it). */
	const FLinearColor& GetWardFillColor() const { return WardColor; }
	FLinearColor GetWardChipColor() const { return WardChipColorFor(WardColor); }

	/** The break's flash Seconds after it: 1 at once, easing to 0 by WardBreakSeconds (0 before and after). */
	static float WardBreakFlashAt(float Seconds);

	/** A ward with no element's fill (the kit's pale cyan), and the chip's colour for a fill (paler, a little see-through). */
	static FLinearColor DefaultWardColor();
	static FLinearColor WardChipColorFor(const FLinearColor& Fill);

	/** The ward bar: its height (px; the health bar's is 6), the gap over the health's frame, the break's length and swell. */
	static constexpr float WardHeight = 4.f;
	static constexpr float WardGap = 2.f;
	static constexpr float WardBreakSeconds = 0.35f;
	static constexpr float WardBreakSwell = 1.15f;
	/** Its chip holds this long after a hit, then drains at this many bar lengths a second. */
	static constexpr float WardChipHold = 0.25f;
	static constexpr float WardChipDrainRate = 1.2f;

	// --- Its element statuses ---

	/**
	 * Cheap to call every frame (UElementStatusComponent::GetStatusIcons): the statuses on it now. The row only rebuilds an
	 * icon when the elements shown change; each frame moves the rings and the stack counts. Empty: the row collapses.
	 */
	void SetStatuses(TConstArrayView<FCreatureStatusIcon> Icons);

	/** Moves the icons' pop-in on by DeltaSeconds (its tick calls it; the tests call it). */
	void AdvanceStatuses(float DeltaSeconds);

	/** The icons as shown, in order (ArrangeStatuses'), and how far each one's pop has come (its scale now). */
	const TArray<FCreatureStatusIcon>& GetStatuses() const { return Statuses; }
	int32 GetStatusCount() const { return Statuses.Num(); }
	float GetStatusScale(int32 Index) const;

	/**
	 * The icons to show for In: one per element (None and spent ones left out; the same element twice keeps the most time
	 * and stacks), at most MaxStatusIcons, in the elements' own order so their places hold.
	 */
	static void ArrangeStatuses(TConstArrayView<FCreatureStatusIcon> In, TArray<FCreatureStatusIcon>& Out);

	/** The count beside an icon: empty for one stack, "2", "3"... past it. */
	static FText StacksLabel(int32 Stacks);

	/** An icon's scale Seconds after it showed: StatusPopScale at once, settling to 1 by StatusPopSeconds. */
	static float StatusPopScaleAt(float Seconds);

	/** One icon per element there is. */
	static constexpr int32 MaxStatusIcons = 5;
	/** An icon's box (px), its ring's line, its glyph's size inside, the gap between icons and the row's gap under the bar. */
	static constexpr float StatusIconSize = 20.f;
	static constexpr float StatusRingThickness = 2.f;
	static constexpr float StatusGlyphSize = 10.f;
	static constexpr float StatusIconGap = 3.f;
	static constexpr float StatusRowGap = 3.f;
	/** The stack count's type. */
	static constexpr int32 StatusStackTextSize = 9;
	/** A new icon's pop. */
	static constexpr float StatusPopScale = 1.45f;
	static constexpr float StatusPopSeconds = 0.18f;

	/** The bar's length on screen, whatever the creature's health. */
	static constexpr float BarWidth = 120.f;

	/** How many parts the dividers cut the bar into, at any health (4 = a line at every quarter). */
	static constexpr int32 Parts = 4;

	/** Where the dividers go across the bar (0-1): at every 1 / Parts of it, the same for every creature. */
	static TArray<float> DividerPositions();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyLabel();
	void ApplyBar();
	/** The rank word's scale and color for the pop's clock now (its own once the pop is over), and a top tier's frame flare. */
	void ApplyRankFlash();
	/** The frame and the trait line as the tier asks (shown, coloured), and the frame's lean and flare. */
	void ApplyTier();
	void ApplyFrameFlare(float Flare);
	float FlashSeconds() const { return FlashSecondsFor(bTopTierFlash); }
	/** The ward bar as it stands: shown or not, its fill and chip, the break's flash and swell. */
	void ApplyWard();
	/** The ward's fill and chip images in its colour. */
	void ApplyWardColor();

	// CreatureHealthBarWidgetStatus.cpp
	/** The status row's icons (MaxStatusIcons of them, collapsed), for RebuildWidget to place under the bar. */
	UWidget* BuildStatusRow();
	/** Every icon as Statuses stands: its glyph and colour when its element changed, its ring, its count, its scale. */
	void ApplyStatuses(bool bElementsChanged);
	void ApplyStatusScale(int32 Index);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	/** The rank's word ("Restless"), hidden for Basic. */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RankText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	/** Its traits' line under the name (Gravebound and up). */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TraitText;

	/** The bar in its frame (sheared as one), the frame's dark edge, its tier line and the clamps at its ends. */
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> FrameBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameEdge;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FrameTierLine;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ClampLeft;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ClampRight;

	/** The lit part of the bar, and the chip of lost health trailing it. */
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> HealthFill;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ChipFill;

	/** The ward bar over the health (sheared as the frame is), its fill, its chip and the break's white-hot flash. */
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> WardBox;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> WardFillBox;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> WardChipBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> WardFlashImage;

	/** The ward's fill and chip themselves, white brushes tinted with the ward's colour. */
	UPROPERTY(Transient)
	TObjectPtr<UImage> WardFillImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> WardChipImage;

	/** The status row under the bar, and each icon's pieces: its box (scaled for the pop), ring, glyph and stack count. */
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> StatusRow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USizeBox>> StatusBoxes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULooterProgressRing>> StatusRings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> StatusGlyphs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> StatusStacks;

	FText CreatureName;
	int32 CreatureLevel = 1;
	FText RankWord;
	FLinearColor RankColor = FLinearColor::White;
	float Fraction = 1.f;
	/** The chip's end: snaps up with healing, holds a moment after a hit, then drains down to the health. */
	float GhostFraction = 1.f;
	float GhostHold = 0.f;

	/** Seconds into the rank word's pop; it is playing while bRankFlashing. */
	float RankFlashClock = RankFlashSeconds;
	bool bRankFlashing = false;

	/** Its tier as last set (SetTier, SetTraitLine). */
	bool bThickFrame = false;
	FLinearColor FrameColor = FLinearColor::White;
	bool bTopTierFlash = false;
	FText TraitLine;

	/** Its ward as last set (SetWard): shown while it stands; the fill, the chip trailing it, the break's clock. */
	bool bWardShown = false;
	float WardFraction = 0.f;
	float WardGhost = 0.f;
	float WardGhostHold = 0.f;
	float WardBreakClock = WardBreakSeconds;
	/** Its colour as last set (the kit's cyan until SetWard names one). */
	FLinearColor WardColor = DefaultWardColor();

	/** Its statuses as last set (ArrangeStatuses'), each icon's element as built, and each icon's seconds since it showed. */
	TArray<FCreatureStatusIcon> Statuses;
	ELooterElement IconElements[MaxStatusIcons] = {};
	float IconAges[MaxStatusIcons] = {};
};
