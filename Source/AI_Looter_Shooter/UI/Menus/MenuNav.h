#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/**
 * The menus' keys and controller as plain rules, no widgets, so the main menu and the settings menu answer alike (and
 * the tests can read them): which key moves the focus which way, which picks, which backs out, which turns the sections;
 * a held key repeating, faster the longer a slider is held; the left stick read as a D-pad; the prompts' key names.
 *
 *  - Move: the arrows, W A S D, the D-pad and the left stick.
 *  - Select: Enter, Space, E, the gamepad's A.        - Back: Esc, the gamepad's B (Start closes a menu too).
 *  - Sections: Page Up / Page Down, the bumpers (LB / RB).
 */
namespace MenuNav
{
	enum class EMove : uint8
	{
		None,
		Up,
		Down,
		Left,
		Right,
	};

	/** The arrows, W A S D and the D-pad (the left stick comes in through its axes: StickMove). */
	EMove MoveOf(const FKey& Key);
	/** -1 up, 1 down, 0 for left, right or none. */
	int32 VerticalStep(EMove Move);
	/** -1 left, 1 right, 0 for up, down or none. */
	int32 HorizontalStep(EMove Move);

	/** Enter, Space, E and the gamepad's A. */
	bool IsAccept(const FKey& Key);
	/** Esc and the gamepad's B: back out one step. */
	bool IsBack(const FKey& Key);
	/** The gamepad's Start: closes the pause menu like Esc. */
	bool IsMenuButton(const FKey& Key);
	/** -1 the previous section (Page Up, LB), 1 the next (Page Down, RB), 0 any other key. */
	int32 SectionStepOf(const FKey& Key);

	/**
	 * The left stick's own direction keys, which some platforms send as well as its axes. The menus read the axes
	 * (StickMove), so these are taken and ignored, never counted twice.
	 */
	bool IsStickKey(const FKey& Key);

	/** The stick takes a direction past this far along it, and lets go below StickRelease (no flicker at the edge). */
	inline constexpr float StickPress = 0.5f;
	inline constexpr float StickRelease = 0.3f;

	/** The direction the left stick points as a D-pad would (its larger axis wins; up is +Y), held while it stays past StickRelease. */
	EMove StickMove(const FVector2D& Stick, EMove Held);

	/** The key a stick direction stands for (its direction keys), so a held stick repeats and lets go like a key. */
	FKey StickKey(EMove Move);

	// --- Held keys ---

	/** The first repeat comes this long after the press... */
	inline constexpr float RepeatDelay = 0.38f;
	/** ...then one every RepeatInterval; a held slider's speed up to one every FastestInterval over AccelerateSeconds. */
	inline constexpr float RepeatInterval = 0.11f;
	inline constexpr float FastestInterval = 0.025f;
	inline constexpr float AccelerateSeconds = 1.6f;

	/**
	 * A held direction's repeats, on the menu's own clock (the platform's key repeat differs from keyboard to pad and is
	 * ignored): the press is the caller's first step, Tick says how many more are due. One direction at a time; a new
	 * press takes over.
	 */
	struct FRepeat
	{
		void Press(const FKey& InKey, EMove InMove, bool bInAccelerate);
		/** True when InKey was the one held (it stops). */
		bool Release(const FKey& InKey);
		void Stop();
		/** The repeats due after DeltaSeconds more (a few at most, on a long frame). */
		int32 Tick(float DeltaSeconds);

		bool IsHeld() const { return Move != EMove::None; }
		EMove GetMove() const { return Move; }
		const FKey& GetKey() const { return Key; }
		float GetHeldSeconds() const { return Held; }

		/** The time to the next repeat, this far into a hold. */
		static float IntervalAt(float HeldSeconds, bool bAccelerate);

	private:
		FKey Key;
		EMove Move = EMove::None;
		bool bAccelerate = false;
		float Held = 0.f;
		float NextAt = 0.f;
	};

	// --- The prompts at the bottom ---

	enum class EPromptKey : uint8
	{
		Select,
		Back,
		Sections,
		/** Left and right: a slider or a choice. */
		Adjust,
		Move,
	};

	/** The key a prompt names: the gamepad's ("A", "B", "LB / RB", "D-pad") or the keyboard's ("Enter", "Esc", ...). */
	FString PromptKeyName(EPromptKey Key, bool bGamepad);

	/** A prompt: its key cap and what it does. The first of a bar is the main one (its cap lit). */
	struct FPrompt
	{
		FString Key;
		FString Text;
	};

	FPrompt MakePrompt(EPromptKey Key, bool bGamepad, const FString& Text);

	/** "A Select  B Back": a bar as one line, for the tests. */
	FString PromptsText(const TArray<FPrompt>& Prompts);
}
