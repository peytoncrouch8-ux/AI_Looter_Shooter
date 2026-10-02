#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"

class FSlateWindowElementList;
class ULooterButton;
class UImage;
class UOverlay;
class UTextBlock;
class UWidget;
class UWidgetTree;
struct FGeometry;

/**
 * The inventory's shared parts: page layout, colors, vector art and card builders. The loadout screen, the bestiary and
 * the missions (the inventory's three pages) use them, so they look alike.
 */
namespace LoadoutParts
{
	inline const FName ActionSlot(TEXT("Slot"));
	inline const FName ActionBackpack(TEXT("Backpack"));
	/** A title tab: its index is the page (EInventoryPage). */
	inline const FName ActionPage(TEXT("Page"));
	inline const TCHAR* const StageMaterialPath = TEXT("/Game/UI/Loadout/M_UI_LoadoutStage.M_UI_LoadoutStage");

	// The page is laid out at 1600 x 900 and scaled to fit the screen.
	inline const FVector2D PageSize(1600.f, 900.f);
	inline const FVector2D StageTopLeft(580.f, 130.f);
	inline const FVector2D StageSize(440.f, 640.f);
	inline constexpr float LeftX = 60.f;
	inline constexpr float LeftWidth = 426.f;
	inline constexpr float RightX = 1110.f;
	inline constexpr float RightWidth = 430.f;
	inline constexpr float ColumnTop = 150.f;
	inline constexpr float SlotCardHeight = 140.f;
	inline constexpr float ListCardHeight = 61.f;
	/** Room above each slot card; the SELECTED chip straddles the card's top edge in it. */
	inline constexpr float SlotGap = 14.f;
	/** The stand's rings on the floor, and how high its pillars rise (cm). */
	inline constexpr float OuterRingRadius = 46.f;
	inline constexpr float InnerRingRadius = 32.f;
	inline constexpr float PillarHeight = 110.f;
	/** The stats card that floats beside the gun under the cursor, how far from its card, and the dragged gun's card. */
	inline constexpr float InspectWidth = 330.f;
	inline constexpr float InspectGap = 16.f;
	inline constexpr float GhostWidth = 250.f;
	/** How far (screen pixels) a pressed gun must move before it's a drag rather than a click. */
	inline constexpr float DragStartDistance = 8.f;
	/** Drag speed (degrees per pixel) and stick speed (degrees per second) for turning the stand-in. */
	inline constexpr float DragTurnRate = 0.45f;
	inline constexpr float StickTurnRate = 160.f;

	namespace Colors
	{
		inline FLinearColor Dim() { return LooterUI::Hex(2, 8, 14, 204); }
		inline FLinearColor CardFill() { return LooterUI::Hex(7, 26, 40, 230); }
		/** Floating cards (the stats card, the dragged gun) sit over the stand-in: nearly solid so they read over it. */
		inline FLinearColor InspectFill() { return LooterUI::Hex(5, 18, 29, 242); }
		/** A slot or row a dragged gun would land in. */
		inline FLinearColor DropFill() { return LooterUI::Hex(255, 159, 28, 46); }
		inline FLinearColor EmptyFill() { return LooterUI::Hex(7, 26, 40, 140); }
		inline FLinearColor InHandFill() { return LooterUI::Hex(46, 30, 8, 235); }
		inline FLinearColor InHandCursor() { return LooterUI::Hex(74, 47, 10, 240); }
		inline FLinearColor PickedFill() { return LooterUI::Hex(255, 159, 28, 72); }
		inline FLinearColor CardLine() { return LooterUI::Hex(90, 200, 255, 89); }
		inline FLinearColor Ring() { return LooterUI::Hex(92, 202, 255, 191); }
		inline FLinearColor InnerRing() { return LooterUI::Hex(92, 202, 255, 115); }
		inline FLinearColor RingGlow() { return LooterUI::Hex(92, 202, 255, 30); }
		inline FLinearColor Pillar() { return LooterUI::Hex(92, 202, 255, 36); }
	}

	/** A gun's side view in the Inked style (UI/Style/InkedIconData.inl), and the name its textures are kept under. */
	const LooterUI::FInkedIcon& GunIcon(EWeaponKind Kind);
	FName GunIconName(EWeaponKind Kind);

	/** An ammo type's cartridges in the Inked style, and the name its textures are kept under. */
	const LooterUI::FInkedIcon& AmmoIcon(EAmmoType Type);
	FName AmmoIconName(EAmmoType Type);

	/** Small solid triangles for upgrade / weaker (10 x 8). */
	const LooterUI::FVectorIcon& ArrowIcon(bool bUp);

	/** A plain disc (stretched to the floor ring's ellipse for its glow). */
	const LooterUI::FVectorIcon& DiscIcon();

	/** Uppercase text in the kit style. */
	UTextBlock* Label(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& TextColor, int32 LetterSpacing = 0);

	/** Uppercase text that ends in "..." rather than spilling out of its space. */
	UTextBlock* FittedLabel(UWidgetTree* Tree, const FString& Text, int32 Size, const FLinearColor& TextColor, int32 LetterSpacing = 0);

	/**
	 * A chamfered card: fill and outline shapes, white and tinted per state, around padded content. Scale grows the corner
	 * cut and the outline with the card (2 = the slot cards' 14 px cut and 2 px line).
	 */
	UOverlay* MakeCard(UWidgetTree* Tree, UWidget* Content, const FMargin& Padding, float Scale, UImage*& OutFill, UImage*& OutLine);

	/**
	 * A gun's Inked icon, as large as fits in Size and centered there. Its colors are its own: keep Tint white, grey it to
	 * dim it. Rarity shows in the name and the card around it, not on the icon.
	 */
	UWidget* MakeGunPicture(UWidgetTree* Tree, const FWeaponInstanceData& Item, const FVector2D& Size,
		const FLinearColor& Tint = FLinearColor::White);

	/** A key cap and what the key does: [E] SWAP. The first (main) action's cap is lit. */
	UWidget* MakeKeyHint(UWidgetTree* Tree, const FString& Key, const FString& Text, bool bPrimary);

	/**
	 * The inventory's title tabs, one per page in EInventoryPage order (Loadout, Bestiary, Missions): the shown page lit as the
	 * title, the others dimmer. Each tab is a button with ActionPage and its page's index; centered at the top of the page.
	 */
	UWidget* MakePageTabs(UWidgetTree* Tree, int32 ShownPage, TArray<ULooterButton*>& OutTabs);
	inline const FVector2D PageTabsPosition(800.f, 36.f);

	/** A stat change, signed, with fewer decimals for big changes. */
	FString FormatDelta(float Delta, int32 Decimals);
	FString AmmoName(const FWeaponInstanceData& Item);

	/** "Assault Rifles": what the swap list calls the chosen slot's kind of gun. */
	FString KindName(const FWeaponInstanceData& Item);

	void DrawLines(FSlateWindowElementList& Elements, int32 LayerId, const FGeometry& Geometry, TArray<FVector2f> Points,
		const FLinearColor& LineColor, float Thickness);
	void DrawBox(FSlateWindowElementList& Elements, int32 LayerId, const FGeometry& Geometry, const FVector2f& TopLeft, const FVector2f& Size,
		const FSlateBrush* Brush, const FLinearColor& Tint);
}
