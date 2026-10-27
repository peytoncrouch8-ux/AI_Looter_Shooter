#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Economy/VendorTrade.h"
#include "Weapons/AmmoTypes.h"
#include "VendorWidget.generated.h"

class ALooterHUD;
class AVendor;
class UImage;
class ULooterButton;
class UOverlay;
class UPlayerThrowComponent;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWalletSubsystem;
class UWeaponManagerComponent;
class UWidget;
struct FWeaponInstanceData;

/** The vendor's screen's measures, at the kit's sizes (CSS px as Slate sizes, the user's call). */
namespace VendorLayout
{
	inline constexpr float PanelWidth = 1060.f;
	inline constexpr float ColumnWidth = 494.f;
	inline constexpr float ColumnGap = 24.f;
	inline constexpr float RowGap = 6.f;
	inline constexpr float RowHeight = 60.f;
	inline constexpr float OfferHeight = 120.f;
	/** The sell list scrolls past this height, so a full backpack still fits a small screen. */
	inline constexpr float SellListHeight = 420.f;
	inline constexpr float ConfirmWidth = 520.f;
}

/** What the vendor's screen's buttons do (ULooterButton::Action). */
namespace VendorActions
{
	inline const FName Row(TEXT("VendorRow"));
	inline const FName Close(TEXT("VendorClose"));
	inline const FName Accept(TEXT("VendorAccept"));
	inline const FName Back(TEXT("VendorBack"));
}

/**
 * A vendor's screen (AVendor; ALooterHUD::OpenVendor; Docs/Progression.md, "Economy"), one LooterUI panel over the dimmed
 * world in the notice board's and the bench's way:
 *  - top: the shop's name on the panel's tab, its greeting, and on the right the player's silver (the purse), which counts
 *    up or down and flashes as it changes;
 *  - left, BUY: REFILL ALL AMMO first (every ammo type to its most for one price, the sum of the boxes; asks first), then a box of each ammo the game's guns use (what fits, its price; FULL when carried to the limit), a jar of
 *    grave salt (up to the three carried), and the GUN OF THE DAY's card (its picture, name in its rarity's colour, level,
 *    damage, rate and magazine, its price; SOLD until the next one, saying when);
 *  - right, SELL: every backpack gun with what a vendor pays (a named gun says it isn't for sale; equipped guns aren't
 *    listed), a JUNK mark on the ones marked, and SELL ALL JUNK with its count and total;
 *  - a status line saying what the last trade did, the keys, a Close button.
 * W / S (the arrows, the D-pad) choose within a side, A / D between BUY and SELL; E (Enter, the face button, a click)
 * buys or sells; J (the top face button, a right-click) marks a backpack gun junk; R (the left face button) sells all
 * junk; Esc backs out of a confirm, then closes. Selling a Rare or better gun, all junk with one among it, or buying the
 * gun of the day asks first. Prices and rules are LooterEconomy's and VendorTrade's; every background follows the UI
 * transparency setting (MarkBackground). It holds the game's input but doesn't pause.
 *
 * VendorWidget.cpp: opening, the panel, choosing, the purse's motion. VendorWidgetKeys.cpp: buttons, keys and the mouse.
 * VendorWidgetRows.cpp: the rows and the gun of the day's card. VendorWidgetTrade.cpp: the trades, the confirm, the words
 * and sounds.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UVendorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens on Vendor: the gun of the day refreshed for the player's level, the lists read afresh. */
	void Open(ALooterHUD* InHUD, AVendor* InVendor);

	/** Closes it: the HUD hands the game its input back. */
	void Close();

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** What a row on the screen is. */
	enum class EEntry : uint8 { RefillAmmo, Ammo, Grenade, Offer, Sell, SellJunk };

	/** One row: what it is, which side it's on, and the parts that restyle as it's chosen (the widgets live in the tree). */
	struct FEntry
	{
		EEntry Kind = EEntry::Ammo;
		EAmmoType Ammo = EAmmoType::AssaultRifle;
		int32 BackpackIndex = INDEX_NONE;
		bool bSell = false;
		/** It can be acted on now (affordable or not: a price too high still answers, with the reason). */
		bool bActive = true;
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
	};

	/** What a confirm asks before it's done. */
	enum class EConfirm : uint8 { None, Sell, SellJunk, BuyOffer, RefillAmmo };

	// --- VendorWidget.cpp: the panel, choosing, keys ---

	/** Reads everything again and builds the panel, keeping the chosen row where it can. */
	void Refresh();
	void BuildPanel();
	void Select(int32 Index, bool bSound = false);
	/** Moves the choice by Direction within its side. */
	void Step(int32 Direction);
	/** To the other side (BUY or SELL), its last chosen row. */
	void SwitchSide(bool bToSell);
	void Restyle();
	void SetStatus(const FText& Message, const FLinearColor& TextColor);
	void HandleButton(ULooterButton* Button);
	void HandleHovered(ULooterButton* Button);
	void UpdatePurse(float DeltaTime);

	UWeaponManagerComponent* GetManager() const;
	UPlayerThrowComponent* GetThrow() const;
	UWalletSubsystem* GetWallet() const;
	int32 GetPlayerLevel() const;

	// --- VendorWidgetRows.cpp: the rows ---

	/** A row: the chamfered card with an icon, a name over a detail line, and what's on its right; a button that acts. */
	UWidget* MakeRow(int32 EntryIndex, UWidget* Icon, UWidget* Name, const FString& Detail, const FLinearColor& DetailColor,
		UWidget* Right, float MinHeight);
	/** The price on a row's right: the coin and the amount, red when it's more than the silver carried. */
	UWidget* MakePrice(int32 Silver, bool bAffordable) const;
	/** A word on a row's right instead of a price (FULL, SOLD, NOT FOR SALE). */
	UWidget* MakeWord(const FString& Word, const FLinearColor& WordColor) const;
	/** REFILL ALL AMMO, first in the Buy list: the total for every stocked type to its most (greyed, with why, when full or too dear). */
	UWidget* MakeRefillAmmoRow();
	UWidget* MakeAmmoRow(EAmmoType Type);
	UWidget* MakeGrenadeRow();
	UWidget* MakeOfferRow();
	UWidget* MakeSellRow(int32 BackpackIndex);
	UWidget* MakeSellJunkRow();
	int32 AddEntry(EEntry Kind, bool bSell);

	// --- VendorWidgetTrade.cpp: trading ---

	/** Acts on the row at Index: buys or sells it (asking first where it must). */
	void Act(int32 Index);
	void BuyAmmo(EAmmoType Type);
	/** Refills every ammo type: asks first (it's a big spend) unless it can't happen, which is said at once. */
	void RefillAllAmmo();
	void BuyGrenade();
	void BuyOffer();
	void SellGun(int32 BackpackIndex);
	void SellAllJunk();
	/** Marks the chosen backpack gun junk, or not. */
	void ToggleJunk();
	/** Says and sounds how a trade went; the purse flashes by itself as the silver changes. */
	void Report(const FVendorTradeResult& Result, const FText& DoneWords, const FText& Item, bool bSale = false);
	void OpenConfirm(EConfirm Kind, int32 BackpackIndex, const FString& Title, const FString& Body, const FText& AcceptLabel);
	void CloseConfirm();
	void AcceptConfirm();
	void PlayCue(const TCHAR* Cue, float Volume = 1.f) const;
	static FText GunName(const FWeaponInstanceData& Gun);

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<AVendor> Vendor;

	TArray<FEntry> Entries;
	int32 Selected = INDEX_NONE;
	int32 LastBuy = INDEX_NONE;
	int32 LastSell = INDEX_NONE;

	EConfirm Confirm = EConfirm::None;
	int32 ConfirmIndex = INDEX_NONE;

	/** The purse as shown: it counts to the silver carried, flashing up (cyan) or down (red), and pops. */
	float ShownSilver = 0.f;
	int32 PurseTarget = 0;
	float PurseFlash = 0.f;
	bool bPurseGain = false;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> Page;
	UPROPERTY(Transient) TObjectPtr<UOverlay> PopupLayer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PurseText;
	UPROPERTY(Transient) TObjectPtr<UWidget> PurseLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SellList;

	/** The status line as last set, kept across a rebuild. */
	FText StatusWords;
	FLinearColor StatusColor = FLinearColor::White;
};
