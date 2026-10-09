#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tutorial/ControlHintRules.h"
#include "HudControlHintWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * The contextual control hint (UControlHintSubsystem decides which, FControlHintRules when): one line in the left column,
 * above the mission tracker, in the tracker's own key-hint style: the bound key as a keycap, then what it does, floating
 * outlined text with no panel (only the keycap's plate is a background, which the UI transparency setting fades). It
 * slides in; when the player uses the control its words turn cyan and the keycap pops before it fades, and an unheeded
 * one simply fades. It steps aside under every menu, under a scene that puts the HUD away, and while paused, picking up
 * where it was after. ALooterHUD makes it as a viewport widget of its own. It does nothing per frame but compare one value
 * unless something is moving.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudControlHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Just over the gameplay HUD (0), under the mission tracker (22), the inventory's pages and every menu. */
	static constexpr int32 ViewportZOrder = 1;

	/** Its line's left end and middle, in 1080p pixels from the screen's top-left: the tracker's column, 36 px above its medal. */
	static constexpr float Left = 36.f;
	static constexpr float Middle = 250.f;

	/** The slide in, the done flash and the fade out. */
	static constexpr float SlideSeconds = 0.25f;
	static constexpr float DoneSeconds = 0.45f;
	static constexpr float FadeSeconds = 0.4f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** What it's doing: nothing, a hint on show, a hint whose control was just used, a hint fading out. */
	enum class EPhase : uint8
	{
		Idle,
		Showing,
		Done,
		Fading,
	};

	void BuildTree();

	/** Shows Hint: its key as the player bound it and its words, sliding in. */
	void Present(EControlHint Hint);

	/** The hint on show went: with its control used (bDone) it flashes first. */
	void BeginLeave(bool bDone);

	/** A menu covers the game, a scene puts the HUD away, or the game is paused. */
	bool IsCovered() const;

	/** The line's offset, opacity, colours and the keycap's pop for where the phase is. */
	void Animate(float DeltaTime);

	UPROPERTY(Transient) TObjectPtr<UWidget> Block;
	UPROPERTY(Transient) TObjectPtr<UWidget> Keycap;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WordsText;

	EPhase Phase = EPhase::Idle;
	EControlHint ShownHint = EControlHint::Count;
	float Age = 0.f;
	/** Eased toward hidden while covered, so it doesn't blink under a menu that opens for a moment. */
	float CoverOpacity = 1.f;
};
