#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudInteractPromptWidget.generated.h"

class AActor;
class UInteractionComponent;
class ULooterProgressRing;
class UTextBlock;
class UWidget;

/**
 * What the Interact key does to the thing in front of the player (a door, the bell, a lantern post; loot has its own
 * card), just under the crosshair as floating outlined text: "[E] OPEN THE DOOR", or "HOLD (E) RING THE BELL" with the key
 * inside a round progress ring that fills clockwise while the key is held. Only a hold wears the ring: a tap is plain
 * text. When a hold completes the ring pops and flashes and the Hold::Complete cue plays. No panel behind it, as the
 * gameplay HUD has none. It fades in and out quickly as things come into and out of reach.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows what Interaction's focused thing offers, Key being the Interact key's name ("E"). Fades out when there's
	 * nothing to say: nothing focused, no words, or no Interaction (the loot card has it). Listens to Interaction for the
	 * holds it completes.
	 */
	void Update(const UInteractionComponent* Interaction, const FString& Key, float DeltaSeconds);

	/** How far under the middle of the screen the prompt's top sits: clear of the widest crosshair and the hit marker. */
	static constexpr float Gap = 52.f;

	/** What the prompt shows now, for the tests. */
	struct FShown
	{
		/** The tap line shows ("[E] OPEN THE DOOR"). */
		bool bTapLine = false;
		/** The hold line shows, and with it the ring. */
		bool bHoldLine = false;
		/** The prompt is on screen at all (not faded out). */
		bool bVisible = false;
		/** How full the ring is (1 while it pops). */
		float RingProgress = 0.f;
		bool bRingPopping = false;
		/** The key's name inside the ring. */
		FString RingKey;
		/** The hold line's words. */
		FString HoldWords;
	};
	FShown GetShown() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** The tap line: the key in brackets in the accent color, then the words. */
	UWidget* MakeTapLine();
	/** The hold line: HOLD, the key inside its progress ring, then the words. */
	UWidget* MakeHoldLine();

	/** Listens to Interaction (once, whichever frame it first shows up) for the holds it completes. */
	void Listen(const UInteractionComponent* Interaction);

	UFUNCTION()
	void HandleInteracted(AActor* Target, bool bHeld);

	UPROPERTY(Transient) TObjectPtr<UWidget> Lines;
	UPROPERTY(Transient) TObjectPtr<UWidget> TapLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TapKey;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TapWords;
	UPROPERTY(Transient) TObjectPtr<UWidget> HoldLine;
	/** The ring round the key, and the key inside it. */
	UPROPERTY(Transient) TObjectPtr<ULooterProgressRing> HoldRing;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HoldKey;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HoldWords;

	/** The interaction component this one hears of completed holds from. */
	TWeakObjectPtr<const UInteractionComponent> Listening;

	/** The hold line is what the prompt is showing (or fading out showing), so a completed hold pops its ring. */
	bool bHoldLineUp = false;

	/** Shown opacity, eased toward 1 while there's something to say. */
	float Opacity = 0.f;
};
