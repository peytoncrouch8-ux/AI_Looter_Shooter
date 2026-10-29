#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Cards/GunCardRules.h"
#include "Weapons/WeaponTypes.h"
#include "GunCardWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class ULooterCardBackdrop;
class ULooterHollowKey;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

/**
 * The gun card: the tag card (the user's pick, 2026-10-28 evening; Docs/Work/2026-10-28-UICharacter/CONTRACT.md "Round 2"),
 * laid out like Borderlands' item card in our own look. A tag in Slate & Salt with the level's gem in its notch; the name in
 * its rarity's colour, "Maker · Kind" in italics, the stats as a list (icon, label, value, arrow against the compared gun),
 * an ornament, bullet lines (element, part effect, maker's trait, curse, notches), a named gun's flavour line, the maker's
 * badge and the element at its foot. On a world card the take keys float under it. One card for the gun on the ground
 * (World), the gun in hand (InHand) and the inventory (Loadout).
 *
 * Its intro (PlayIntro) unfolds it, brings the rows in one by one, counts the damage up and pops the gem, and plays the
 * card's open sound with its maker's. It runs from NativeTick, or AdvanceIntro (tests).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGunCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows Gun, compared with Compare (null: nothing to compare; the arrows hide). True when it's a different gun from the
	 * one shown (by its seed, definition, level and rarity), so the caller can PlayIntro.
	 */
	bool SetGun(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Compare, EGunCardMode InMode);

	/** A world card's keys (they hide on the other modes). Cheap to call every frame. */
	void SetKeys(const FGunCardKeys& Keys);

	/** Starts the intro over; with sound, its open sound and its maker's play (2D). */
	void PlayIntro(bool bWithSound);

	/** Shows it whole now (no intro). */
	void SkipIntro();

	/** Runs the intro on by DeltaSeconds (NativeTick calls it). */
	void AdvanceIntro(float DeltaSeconds);

	bool IsIntroPlaying() const { return IntroAge < GunCardRules::IntroSeconds; }

	/** The card's width for a mode (World and Loadout 470, InHand 430); its height follows its content. */
	static float CardWidth(EGunCardMode InMode);

	/** Where the card unfolds from (0-1 of its height): its foot for a world card (it rises out of the gun), its top else. */
	static float UnfoldPivotY(EGunCardMode InMode);

	// --- What it shows (tests) ---
	EGunCardMode GetMode() const { return Mode; }
	FText GetNameText() const;
	/** "ROUNDHOUSE": the maker's badge (the rarity's word for a gun with no maker). */
	FText GetMakerText() const;
	/** "UPGRADE" when it beats the compared gun, else empty (not drawn; the inventory reads it). */
	FText GetVerdictText() const;
	/** The damage as drawn now (counting up during the intro). */
	FText GetDamageText() const;
	int32 GetStatCount() const { return ShownStats.Num(); }
	/** Row Index's value ("540 rpm") and arrow ("▲", "▼" or empty). */
	FText GetStatValueText(int32 Index) const;
	FText GetStatArrowText(int32 Index) const;
	/** It has a gun to compare with (its arrows show). */
	bool HasCompare() const { return bHasCompare; }
	/** The bullet lines shown. */
	TArray<FString> GetLines() const;
	/** The keys' caps ("E", "HOLD E"), in order; empty when they hide. */
	TArray<FString> GetKeyCaps() const;
	/** The hold cap's fill (0 without a hold cap). */
	float GetHoldFill() const;
	/** The cue its open plays with its maker's (none without a maker). */
	FName GetMakerCue() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** Builds the tree once (RebuildWidget, or SetGun before it). */
	void EnsureBuilt();
	/** Writes the gun's words, rows and lines into the tree. */
	void ApplyGun();
	/** One stat row (made once per row the card can show, reused). */
	void EnsureRows(int32 Count);
	/** The intro's look at its age: scale, opacity, rows, the counting number, the gem. */
	void ApplyIntro();

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Root;
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> Box;
	UPROPERTY(Transient)
	TObjectPtr<ULooterCardBackdrop> Fill;
	UPROPERTY(Transient)
	TObjectPtr<ULooterCardBackdrop> Lines;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GemText;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> Gem;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> KindText;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> StatsBox;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> RowWidgets;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> RowIcons;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RowLabels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RowValues;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> RowArrows;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> Ornament;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> LinesBox;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> LineTexts;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FlavorText;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> MakerBadge;
	UPROPERTY(Transient)
	TObjectPtr<UImage> MakerMark;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MakerText;
	UPROPERTY(Transient)
	TObjectPtr<UImage> ElementIcon;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> KeysRow;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULooterHollowKey>> KeyCaps;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> KeyWords;

	FWeaponInstanceData Shown;
	FWeaponInstanceData Compared;
	bool bHasGun = false;
	bool bHasCompare = false;
	EGunCardMode Mode = EGunCardMode::Loadout;
	/** The stats on its rows now (CardStats of the gun shown). */
	TArray<LoadoutRules::EStat> ShownStats;
	/** The keys shown last (SetKeys rebuilds them only when they change). */
	TArray<GunCardRules::FKeyHint> ShownKeys;
	float ShownHold = 0.f;
	/** Seconds since the intro started; IntroSeconds and past: whole. */
	float IntroAge = GunCardRules::IntroSeconds;
};
