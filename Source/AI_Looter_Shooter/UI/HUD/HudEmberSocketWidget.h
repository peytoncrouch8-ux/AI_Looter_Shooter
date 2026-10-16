#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Embers/EmberTypes.h"
#include "Styling/SlateBrush.h"
#include "HudEmberSocketWidget.generated.h"

class UEmberComponent;
class UImage;
class USizeBox;
class UTextBlock;
class UWidget;

/** The pictures the socket's ring is drawn from (HudEmberSocketWidgetPictures.cpp). */
enum class EHudEmberPicture : uint8
{
	Disc,
	Rim,
	Metal,
	Hairline,
	Accent,
};

/**
 * The equipped ember power's socket, in the weapon column's Arms row left of the weapon slots (bottom-right;
 * PlayerHUDWidget places it). A gunmetal ring like a weapon slot's with the power's Inked icon inside (tinted white), the
 * Ember key's tab under it and the power's name in small type under that. No backing panel: only the inner disc and the
 * tab's glass fade with the UI transparency setting.
 *  - ready: an orange ring round it and a slow glow in the power's colour (Color::EmberDustDevil, ...); it pops, the ring
 *    flashing white, the moment it comes off cooldown (NotifyState)
 *  - running: an arc in the power's colour runs down round the ring as its duration runs out
 *  - cooling down: a dark pie over the icon drains clockwise from 12 o'clock, with the seconds left in white
 *  - a denied press (Flash): a shake for Cooldown and Active (the press meant "not yet"), a soft dim for the rest; the
 *    component plays the Ember.Denied click itself, so the socket makes no sound
 * Collapsed with no power equipped (no component, no hero). The ring, disc and tab are vector pictures drawn once into
 * shared textures (the weapon slots' measures); the pie, the arc and the seconds are painted each frame (NativePaint),
 * so they follow the cluster's fade.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudEmberSocketWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Reads Embers (null: hidden) with KeyName on the tab; call every frame (it repaints only what changed, and runs the motion). */
	void Update(const UEmberComponent* Embers, const FString& KeyName, float DeltaTime);

	/** A press of the key did nothing: Cooldown and Active shake it, anything else dims it a moment. None does nothing. */
	void Flash(EEmberDenied Why);

	/** The equipped power's state changed (UEmberComponent::OnStateChanged): coming back to Ready pops it. */
	void NotifyState(EEmberState NewState);

	/** What the pie says with Left seconds of cooldown: whole seconds, rounded up ("3" until it reaches 2), never "0". */
	static FString SecondsText(float Left);

	/** The tab's width for a key's name: the slots' 22 px for one letter, wider for a longer name (12 px type). */
	static float TabWidthFor(const FString& KeyName);

	/** The shake's sideways offset (px) Elapsed seconds into it, dying away by ShakeSeconds; the pop's scale, likewise. */
	static float ShakeOffset(float Elapsed);
	static float PopScale(float Elapsed);

	/** Whether it shows now, the power it shows (its id) and the state as last painted, for the tests. */
	bool IsShowing() const { return bShowing; }
	FName GetShownId() const { return ShownId; }
	EEmberState GetShownState() const { return ShownState; }
	bool IsPopping() const { return PopLeft > 0.f; }
	bool IsShaking() const { return ShakeLeft > 0.f; }

	/** The ring (the weapon slots' circle), the tab's height (theirs), and the seconds' and the name's type sizes. */
	static constexpr float RingDiameter = 64.f;
	static constexpr float TabHeight = 16.f;
	/** The gunmetal ring's radii (the running arc lies between them), and how far the ready ring's picture reaches past the ring. */
	static constexpr float MetalOuter = 30.f;
	static constexpr float MetalInner = 27.5f;
	static constexpr float AccentMargin = 4.f;
	static constexpr int32 SecondsSize = 20;
	static constexpr int32 NameSize = 10;
	/** How long a shake, a dim and a pop run. */
	static constexpr float ShakeSeconds = 0.4f;
	static constexpr float DimSeconds = 0.35f;
	static constexpr float PopSeconds = 0.35f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	/** Paints the parts that change with the state (the ring, the glow, the icon's strength). */
	void PaintState();
	/** Sets the tab's plate to the key's width and writes the key. */
	void PaintKey(const FString& KeyName);
	/** The shake, the dim and the pop, frame by frame. */
	void RunMotion(float DeltaTime);

	/** A ring picture as a brush, its texture made once and shared; the key tab's plate or edge Width wide (HudEmberSocketWidgetPictures.cpp). */
	static FSlateBrush PictureBrush(EHudEmberPicture Picture);
	static FSlateBrush TabBrush(float Width, bool bEdge);

	UPROPERTY(Transient) TObjectPtr<UImage> Glow;
	UPROPERTY(Transient) TObjectPtr<UImage> Disc;
	UPROPERTY(Transient) TObjectPtr<UImage> Icon;
	UPROPERTY(Transient) TObjectPtr<UImage> AccentRing;
	UPROPERTY(Transient) TObjectPtr<USizeBox> TabBox;
	UPROPERTY(Transient) TObjectPtr<UImage> TabGlass;
	UPROPERTY(Transient) TObjectPtr<UImage> TabEdge;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;

	bool bShowing = false;
	FName ShownId;
	EEmberState ShownState = EEmberState::Ready;
	FString ShownKey;
	/** The power's colour, as shown (the glow, the arc). */
	FLinearColor PowerColor = FLinearColor::White;

	/** What NativePaint draws: the pie's share (0 none, 1 whole), the arc's share, and the seconds' words. */
	float PieShare = 0.f;
	float ArcShare = 0.f;
	FString PieWords;

	/** The glow's slow beat while ready (seconds), and the motion left to run. */
	float GlowClock = 0.f;
	float ShakeLeft = 0.f;
	float DimLeft = 0.f;
	float PopLeft = 0.f;
};
