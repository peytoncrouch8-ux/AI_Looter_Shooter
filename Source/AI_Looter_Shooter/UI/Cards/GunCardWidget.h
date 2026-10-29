#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Cards/GunCardRules.h"
#include "Weapons/WeaponTypes.h"
#include "GunCardWidget.generated.h"

class UHorizontalBox;
class ULooterCardBackdrop;
class ULooterHollowKey;
class ULooterSegmentBar;
class USizeBox;
class UTextBlock;
class UWidget;

/**
 * The gun card (the user's pick, 2026-10-28: "Wide Sweep"; Docs/Work/2026-10-28-UICharacter/CONTRACT.md): the maker's
 * sweep across its top, the gun's name in its rarity's colour, its kind and level, the damage big, three segmented bars
 * (fire rate, magazine, reload) with the compared gun's tick and arrows, one special line, and on a world card the take
 * keys. One card for the gun on the ground (World), the gun in hand (InHand) and the inventory (Loadout); simple on purpose,
 * every other stat waits behind the inventory's Inspect.
 *
 * Its intro (PlayIntro) unfolds it, fills the bars one by one, counts the damage up, sweeps a shine across an Epic's or a
 * Legendary's band, and plays the card's open sound with its maker's. It runs from NativeTick, or AdvanceIntro (tests).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGunCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Shows Gun, compared with Compare (null: nothing to compare; the arrows, ticks and verdict hide). True when it's a
	 * different gun from the one shown (by its seed, definition, level and rarity), so the caller can PlayIntro.
	 */
	bool SetGun(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Compare, EGunCardMode InMode);

	/** A world card's keys (the row hides on the other modes). Cheap to call every frame. */
	void SetKeys(const FGunCardKeys& Keys);

	/** Starts the intro over; with sound, its open sound and its maker's play (2D). */
	void PlayIntro(bool bWithSound);

	/** Shows it whole now (no intro). */
	void SkipIntro();

	/** Runs the intro on by DeltaSeconds (NativeTick calls it). */
	void AdvanceIntro(float DeltaSeconds);

	bool IsIntroPlaying() const { return IntroAge < GunCardRules::IntroSeconds; }

	/** The card's width for a mode (World 420, InHand 360, Loadout 420); its height follows its content. */
	static float CardWidth(EGunCardMode InMode);

	/** Where the card unfolds from (0-1 of its height): its foot for a world card (it rises out of the gun), its top else. */
	static float UnfoldPivotY(EGunCardMode InMode);

	// --- What it shows (tests) ---
	EGunCardMode GetMode() const { return Mode; }
	FText GetNameText() const;
	FText GetMakerText() const;
	/** "UPGRADE", or empty. */
	FText GetVerdictText() const;
	/** The special line's word ("EMBER"), or empty. */
	FText GetSpecialText() const;
	/** The damage number as drawn now (counting up during the intro). */
	FText GetDamageText() const;
	int32 GetStatCount() const { return Bars.Num(); }
	const ULooterSegmentBar* GetBar(int32 Index) const { return Bars.IsValidIndex(Index) ? Bars[Index].Get() : nullptr; }
	/** The value written after bar Index ("45 RPM"). */
	FText GetStatValueText(int32 Index) const;
	/** The keys row's caps ("E", "HOLD E"), in order; empty when it hides. */
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
	/** Writes the gun's words and bars into the tree. */
	void ApplyGun();
	/** The intro's look at its age: scale, opacity, bars shown, the counting number, the shine. */
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
	TObjectPtr<UTextBlock> MakerText;
	UPROPERTY(Transient)
	TObjectPtr<class UImage> MakerMark;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CornerText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> KindText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DamageText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PelletText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DamageLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DamageDelta;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> StatLabels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULooterSegmentBar>> Bars;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> Values;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> Arrows;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> SpecialRow;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpecialWord;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpecialWords;
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
	/** The keys shown last (SetKeys rebuilds the row only when they change). */
	TArray<GunCardRules::FKeyHint> ShownKeys;
	float ShownHold = 0.f;
	/** Seconds since the intro started; IntroSeconds and past: whole. */
	float IntroAge = GunCardRules::IntroSeconds;
	/** The damage it counts up to. */
	float DamageValue = 0.f;
	FString PelletWords;
};
