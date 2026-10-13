#pragma once

#include "CoreMinimal.h"

class UWidget;
class UWidgetTree;
struct FKey;

/**
 * The menus' focus look for the keyboard and a controller, in the kit's style (Concept C): the stop that has the focus
 * gets an orange outline doubled just inside it and a lit edge down its left side, and a row a faint orange wash. It
 * shows only while the menus are driven by keys or a gamepad; under the mouse the buttons keep their own hover look, as
 * they always had.
 *
 * Which of the three drives the menus is the last one used in a menu, kept here for every menu at once, so the main menu
 * hands it to its settings and back, and the prompts at the bottom name the gamepad's buttons or the keyboard's keys.
 */
namespace LooterUI
{
	enum class EMenuDevice : uint8
	{
		Mouse,
		Keyboard,
		Gamepad,
	};

	EMenuDevice GetMenuDevice();
	void SetMenuDevice(EMenuDevice Device);

	/** A key used in a menu: a gamepad's makes it the gamepad, any other the keyboard. True when that changed it. */
	bool NoteMenuKey(const FKey& Key);

	/** The mouse moved, clicked or scrolled in a menu. True when that changed it. */
	bool NoteMenuPointer();

	/** The focus look shows (keys or a gamepad drive the menus), or not (the mouse does). */
	bool IsMenuFocusShown();

	/** The prompts name the gamepad's buttons ("A Select"), or the keyboard's keys ("Enter Select"). */
	bool IsMenuGamepad();

	/**
	 * A focus look on its own, hidden until lit: the outline, its inner double and the lit edge, in Focus (or
	 * FocusOnAccent, bOnAccent, for something orange), and with bWash the faint wash under them. Fills what it's put over.
	 */
	UWidget* MakeFocusFrame(UWidgetTree* Tree, bool bOnAccent, bool bWash);

	/**
	 * Gives a row made by MakeRow or MakeShapeBox (an overlay) its focus look, hidden, under its content; returns the
	 * look to light (SetFocusLit), or null for anything else. Call it while building, before the row is on screen.
	 */
	UWidget* AddFocusFrame(UWidget* Box);

	/** Lights a focus look or puts it out: a look from AddFocusFrame or MakeFocusFrame, or a ULooterButton's own. */
	void SetFocusLit(UWidget* Light, bool bLit);
	bool IsFocusLit(const UWidget* Light);

	/** How bright a lit focus look shows, 0 to 1 (it breathes while it waits: FocusGlowAt). */
	void SetFocusGlow(UWidget* Light, float Glow);

	/** The focus look's brightness this long after it arrived: full at first, then a slow breath that never goes dim. */
	float FocusGlowAt(float Seconds);
}
