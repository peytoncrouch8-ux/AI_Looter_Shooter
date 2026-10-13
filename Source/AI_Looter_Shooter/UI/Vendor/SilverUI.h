#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

class UHorizontalBox;
class UImage;
class UTextBlock;
class UWidgetTree;

/**
 * How silver looks wherever it shows (the HUD's readout, the vendor's screen, the inventory's ammo line): a silver coin in
 * the Inked style, the weapon and ammo icons' light top, shaded underside and thick ink line, drawn by the kit
 * (LooterUI::InkedIconBrush, no texture asset), beside the amount ("1,240"). The coin's metal is the icons' own light and
 * shade, never a rarity's colour.
 */
namespace SilverUI
{
	/** The coin: a disc with a raised rim and a star struck in its middle. */
	const LooterUI::FInkedIcon& CoinIcon();

	/** The name its textures are kept under. */
	FName CoinIconName();

	/** The coin as an image Size px across. Keep Tint white; grey dims it. */
	UImage* MakeCoin(UWidgetTree* Tree, float Size, const FLinearColor& Tint = FLinearColor::White);

	/**
	 * The coin and an amount on one line, the coin a little taller than the type. bFloating: the HUD's outlined floating
	 * type (no panel behind it); else the kit's panel type. OutText: the amount's text, to change later.
	 */
	UHorizontalBox* MakeAmount(UWidgetTree* Tree, int32 Silver, int32 FontSize, const FLinearColor& TextColor, bool bFloating,
		UTextBlock** OutText = nullptr);
}
