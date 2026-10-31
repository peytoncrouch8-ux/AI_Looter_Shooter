#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shields/ShieldTypes.h"
#include "UI/Cards/GunCardRules.h"
#include "UI/Cards/WardCardRules.h"
#include "WardCardWidget.generated.h"

class UHorizontalBox;
class UImage;
class ULooterCardBackdrop;
class ULooterHollowKey;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * The ward card (the user, 2026-10-31: "Wards should have a similar UI to the weapons", on the ground and in the loadout):
 * the gun card's tag card (UGunCardWidget) with a ward's numbers. The tag in Slate & Salt with the level's gem in its notch;
 * the ward box's mark and its rarity's word over the name; the name in its rarity's colour; the stat rows (capacity,
 * recharge delay, recharge rate) with arrows against the ward worn, then its build and trait rows; an ornament and the
 * bullet lines (its trait's effect, its build's lean); the ward box's Inked icon in its rarity's colour at its foot. On a
 * world card the take keys float under it. The same intro as the gun card: it unfolds, the rows come in one by one, the
 * capacity counts up, the gem pops, and it opens with the card's sound.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UWardCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows Ward, compared with Worn (null: nothing to compare; the arrows hide). True when it's a different ward from the
	 * one shown (ShieldRules::Identity) or another mode, so the caller can PlayIntro.
	 */
	bool SetWard(const FShieldInstanceData& Ward, const FShieldInstanceData* Worn, EWardCardMode InMode);

	/** A world card's keys (they hide in the loadout). Cheap to call every frame. */
	void SetKeys(const FWardCardKeys& Keys);

	/** Starts the intro over; with sound, the card's open sound (2D). */
	void PlayIntro(bool bWithSound);
	void SkipIntro();
	void AdvanceIntro(float DeltaSeconds);
	bool IsIntroPlaying() const { return IntroAge < GunCardRules::IntroSeconds; }

	/** The gun card's widths: 470 for both modes. */
	static float CardWidth() { return 470.f; }

	// --- What it shows (tests) ---
	EWardCardMode GetMode() const { return Mode; }
	FText GetNameText() const;
	FText GetTopText() const;
	/** "UPGRADE" when it beats the ward worn, else empty (not drawn; the inventory reads it). */
	FText GetVerdictText() const;
	int32 GetStatCount() const { return WardCardRules::CardStats().Num(); }
	/** Stat row Index's value ("52") and arrow ("▲", "▼" or empty). */
	FText GetStatValueText(int32 Index) const;
	FText GetStatArrowText(int32 Index) const;
	bool HasCompare() const { return bHasCompare; }
	TArray<FString> GetLines() const;
	/** The keys' caps ("E", "HOLD E"), in order; empty when they hide. */
	TArray<FString> GetKeyCaps() const;
	/** The keys' words ("TO BACKPACK", "WEAR"), in order (the caps' and the lone "BACKPACK FULL"). */
	TArray<FString> GetKeyWords() const;
	float GetHoldFill() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void EnsureBuilt();
	void EnsureRows(int32 Count);
	void ApplyWard();
	/** The intro's look at its age (WardCardWidgetMotion.cpp). */
	void ApplyIntro();

	UPROPERTY(Transient) TObjectPtr<UWidget> Root;
	UPROPERTY(Transient) TObjectPtr<USizeBox> Box;
	UPROPERTY(Transient) TObjectPtr<ULooterCardBackdrop> Fill;
	UPROPERTY(Transient) TObjectPtr<ULooterCardBackdrop> Lines;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GemText;
	UPROPERTY(Transient) TObjectPtr<UWidget> Gem;
	UPROPERTY(Transient) TObjectPtr<UImage> TopMark;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TopText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> StatsBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> RowWidgets;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> RowIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RowLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RowValues;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RowArrows;
	UPROPERTY(Transient) TObjectPtr<UWidget> Ornament;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> LinesBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> LineTexts;
	/** The ward box's Inked icon in its rarity's colour, on the foot. */
	UPROPERTY(Transient) TObjectPtr<UImage> KindMark;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> KeysRow;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterHollowKey>> KeyCaps;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> KeyWords;

	UPROPERTY(Transient) FShieldInstanceData Shown;
	UPROPERTY(Transient) FShieldInstanceData Compared;
	bool bHasWard = false;
	bool bHasCompare = false;
	EWardCardMode Mode = EWardCardMode::Loadout;
	TArray<GunCardRules::FKeyHint> ShownKeys;
	float ShownHold = 0.f;
	float IntroAge = GunCardRules::IntroSeconds;
};
