#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "LooterProgressRingWidget.generated.h"

class SLooterProgressRing;
class UTextBlock;
class UWidgetTree;

/**
 * SLooterProgressRing in UMG: the kit's progress ring for a widget tree. Set its look (Configure, SetColors) before it is
 * shown or after, move it with SetProgress, and when a hold completes call Pop: it swells about a tenth and flashes for
 * LooterRing::PopSeconds, run by Advance from the owner's own update (so it follows the owner's delta time and a test can
 * step it). It ignores the mouse.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterProgressRing : public UWidget
{
	GENERATED_BODY()

public:
	ULooterProgressRing(const FObjectInitializer& ObjectInitializer);

	/** The box it fills, its line's width, and its parts (0: one smooth arc) SegmentGap pixels apart. */
	void Configure(float InDiameter, float InThickness, int32 InSegments = 0, float InSegmentGap = 3.f);

	/** The filled part's color and the track's. */
	void SetColors(const FLinearColor& InFill, const FLinearColor& InTrack);

	/** How full it is (0-1, clamped). */
	void SetProgress(float InProgress);
	float GetProgress() const { return Progress; }

	/** Starts the pop; calling it during one starts it over. */
	void Pop();

	/** Runs the pop on by DeltaSeconds. True while the look is changing: popping, or the frame it ended. */
	bool Advance(float DeltaSeconds);

	bool IsPopping() const { return PopLeft > 0.f; }

	/** The scale the ring wears now (1 when it isn't popping), and its flash (0-1). */
	float GetPopScale() const;
	float GetFlash() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	/** Shows the pop's scale and flash. */
	void ApplyPop();

	TSharedPtr<SLooterProgressRing> Ring;

	float Diameter = 32.f;
	float Thickness = 3.f;
	int32 Segments = 0;
	float SegmentGap = 3.f;
	FLinearColor FillColor = FLinearColor::White;
	FLinearColor TrackColor = FLinearColor::White;
	float Progress = 0.f;
	/** Seconds of the pop still to run (0: none). */
	float PopLeft = 0.f;
};

namespace LooterUI
{
	/**
	 * The key cap a hold prompt wears: a dim dark disc with the progress ring round it and the key's name inside, in the
	 * accent color. OutRing and OutKey are the pieces to drive (UpdateHoldKeyCap); the returned widget is the whole cap,
	 * a fixed square so the words beside it never shift.
	 */
	UWidget* MakeHoldKeyCap(UWidgetTree* Tree, ULooterProgressRing*& OutRing, UTextBlock*& OutKey);

	/** A key's name as it goes inside the ring: the brackets off, in capitals ("[Esc]" is "ESC"). */
	FString HoldKeyLabel(const FString& Key);

	/** The font size for a label inside the ring: the one letter big, a longer name smaller to fit. */
	int32 HoldKeyTextSize(int32 Characters);

	/** Writes the key's name inside the ring (restyled only when it changes). */
	void SetHoldKeyLabel(UTextBlock* KeyText, const FString& Key);

	/**
	 * Moves a hold key cap on one frame: the ring shows Progress (full while it pops), the pop runs by DeltaSeconds and
	 * the key's name swells and lights with it. Call it every frame the cap is on screen.
	 */
	void UpdateHoldKeyCap(ULooterProgressRing* Ring, UTextBlock* KeyText, float Progress, float DeltaSeconds);
}
