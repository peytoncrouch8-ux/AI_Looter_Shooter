#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EmbersWidget.generated.h"

class ALooterHUD;
class UEmberComponent;
class UEmberDefinition;
class UHorizontalBox;
class UImage;
class ULooterButton;
class UTextBlock;
class UWidget;
enum class EInventoryPage : uint8;

/**
 * The ember powers, the inventory's second page (the user's call, 2026-10-15), laid out like the pages beside it:
 *  - top: the title tabs; under them the hero's name and how the powers work (one equipped, the Ember key fires it)
 *  - middle: the hero's three powers as cards side by side, each with its Inked icon in a medallion ringed in its colour,
 *    its name, its nature, what it does, its cooldown and duration; the equipped one wears an orange EQUIPPED tag and a
 *    glow in its colour, and says how it stands (ready, running, cooling with the seconds left)
 *  - bottom: a line for what just happened (equipped, or refused while a power runs), and what the keys do
 * A / D, the arrows or the D-pad choose a power (so does the mouse); E, Enter, A on a gamepad or a click equips it
 * (UEmberComponent::EquipIndex: refused while a power runs, which the page says); 1 to 5 or the shoulders turn to the other
 * pages; Esc, B and the inventory's key close. Sounds: the loadout's equip clack, UI.Denied when refused, UI.Hover as the
 * choice moves. EmbersWidgetInput.cpp has the keys, the mouse and equipping.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmbersWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(ALooterHUD* InHUD);
	void Close();

	/** How a power's clock reads on its card: "12 s", "0.3 s"; a duration of 0 is "Instant". */
	static FString SecondsWords(float Seconds, bool bDuration);

	/** The cards shown and the one chosen, for the tests. */
	int32 NumCards() const { return Cards.Num(); }
	int32 GetSelected() const { return Selected; }

	/** Equips the chosen power (as E or a click does). True if it changed; false when refused or already equipped. */
	bool EquipSelected();

	/** Moves the choice to Index (clamped to the cards; it sounds when it moves). */
	void Select(int32 Index);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	/** A power's card, and the parts that change with the choice and the power's state. */
	struct FCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		UImage* Glow = nullptr;
		UWidget* EquippedTag = nullptr;
		UTextBlock* State = nullptr;
		/** What the state line says now, so it's set only on change. */
		FString StateWords;
	};

	// EmbersWidget.cpp: opening, layout, the cards
	void Gather();
	void RebuildCards();
	FCard MakePowerCard(int32 Index, const UEmberDefinition& Power);
	void Restyle();
	/** Each card's state line (the equipped one's: ready, running or cooling, with the seconds). */
	void RefreshStates();
	void RefreshPrompts();
	/** Says Words in the line under the cards (in Tone), for a few seconds. */
	void ShowMessage(const FString& Words, const FLinearColor& Tone);
	UEmberComponent* GetEmbers() const;

	// EmbersWidgetInput.cpp: the keys, the mouse, equipping
	void ShowPage(EInventoryPage Page);
	void HandleCardClicked(ULooterButton* Button);
	void HandleCardHovered(ULooterButton* Button);
	void HandleTabClicked(ULooterButton* Button);

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<UEmberComponent> Embers;

	/** The hero's powers, in their order (UHeroDefinition::Powers). */
	UPROPERTY(Transient) TArray<TObjectPtr<UEmberDefinition>> Powers;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeroTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HowTo;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CardRow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EmptyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;

	/** One per power, by index into Powers. */
	TArray<FCard> Cards;
	int32 Selected = 0;
	/** The equipped power's index as the cards were last styled (a swap restyles them). */
	int32 ShownEquipped = INDEX_NONE;
	/** Seconds the message line stays up (it fades over the last half second). */
	float MessageTime = 0.f;
};
