#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Scenes/TransitionScreen.h"
#include "Styling/SlateBrush.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TransitionScreenSubsystem.generated.h"

class SBox;
class SImage;
class SOverlay;
class STextBlock;
class SVerticalBox;
class UGameViewportClient;

/**
 * The white over the whole game window between places (Docs/Story.md: the cloud bank's white, and REVENANT rising through
 * it). The game instance owns it, so it holds through a level load and covers the next level's first frames: it is drawn
 * by Slate straight into the game viewport, over every widget and menu, and the engine takes a level's UMG widgets away
 * with the level but leaves this.
 *
 * A scene brings the white up (SetWhite, FadeToWhite) and holds it for the trip (HoldWhite); the level arrived at reveals
 * it (Reveal): the title, if any, rises through the white, then the white thins and is gone over a couple of seconds.
 * Nothing keeps the screen white for good: a white nothing reveals thins by itself after 20 s, with a warning in the log.
 * FTransitionScreen holds the timing; the title is set in the LooterUI kit's display type.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UTransitionScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UTransitionScreenSubsystem* Get(const UObject* WorldContextObject);

	virtual void Deinitialize() override;

	/** Full white at once, held through level loads. */
	void HoldWhite();

	bool IsHeld() const;

	/** On arrival: the title, if any, rises through the white, then the white thins. */
	void Reveal(const FText& Title = FText::GetEmpty());

	/** A scene's own white (0-1), now. Not held: HoldWhite, Reveal or Clear follow (left alone, it thins after 20 s). */
	void SetWhite(float White);

	/** Up to full white over Seconds, then held as HoldWhite holds it. */
	void FadeToWhite(float Seconds);

	/** The white gone at once. */
	void Clear();

	/** How white the screen is now (0-1), and whether anything shows. */
	float GetWhite() const { return Screen.GetWhite(); }
	bool IsShowing() const { return Screen.IsShowing(); }

	const FTransitionScreen& GetScreen() const { return Screen; }

	/** The game's title, as the first cast-off's title card sets it: REVENANT. */
	static FText GameTitle();

	/** Over everything in the game viewport: the HUD, the inventory and the pause menu are all lower. */
	static constexpr int32 ViewportZOrder = 1000;

private:
	/** Moves the timing on and redraws; false (the ticker lets go) once the screen is clear. */
	bool HandleTick(float DeltaSeconds);

	/** Puts the overlay in the game viewport (once) and keeps the timing ticking while anything shows. */
	void Show();

	/** Takes the overlay out of the viewport. */
	void RemoveOverlay();

	void BuildOverlay();

	/** The overlay as the timing has it now. */
	void Refresh();

	FTransitionScreen Screen;

	/** The title the reveal raises (empty: none). */
	FText CardTitle;

	FSlateBrush WhiteBrush;
	FSlateBrush LineBrush;
	TSharedPtr<SOverlay> Overlay;
	TSharedPtr<SImage> WhiteImage;
	TSharedPtr<SVerticalBox> TitleBox;
	TSharedPtr<STextBlock> TitleText;
	TSharedPtr<SBox> LineBox;

	/** The viewport the overlay is in, if it is in one. */
	TWeakObjectPtr<UGameViewportClient> ShownOn;

	FTSTicker::FDelegateHandle TickHandle;
};
