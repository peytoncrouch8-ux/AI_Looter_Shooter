#pragma once

#include "CoreMinimal.h"

/** What the settings menu's live previews share: how long they stay, how they fade, and the plain math of one. */
namespace SettingsMenu
{
	/** A slider's preview stays up this long after the slider's last move or let-go (a held key's repeats keep it up). */
	inline constexpr float PreviewLingerSeconds = 0.8f;
	/** How fast a preview fades in and out, in opacity per second. */
	inline constexpr float PreviewFadeSpeed = 6.f;
	/** While the view is being previewed the panel ghosts to this much of its opacity, and the dimming over the game goes. */
	inline constexpr float ViewPreviewPanelOpacity = 0.45f;

	/**
	 * A slider whose setting shows live while it moves (the minimap's size and zoom, the view's field of view): up while
	 * the slider is held, by the mouse or a controller, and for a moment after each move and after the let-go (so a key
	 * held down on a focused slider, which only repeats moves, counts as holding it too). Plain math, no widgets.
	 */
	struct FSliderPreview
	{
		/** The slider is taken; Value is what it showed, to go back to if the drag is cancelled. */
		void Grab(float Value)
		{
			bHeld = true;
			Linger = PreviewLingerSeconds;
			GrabbedValue = Value;
		}

		/** The value moved: the preview stays up. */
		void Nudge()
		{
			Linger = PreviewLingerSeconds;
		}

		void Release()
		{
			bHeld = false;
			Linger = PreviewLingerSeconds;
		}

		void Reset()
		{
			*this = FSliderPreview();
		}

		bool IsHeld() const { return bHeld; }
		bool IsActive() const { return bHeld || Linger > 0.f; }
		float GetGrabbedValue() const { return GrabbedValue; }
		float GetOpacity() const { return Opacity; }

		/** One frame; bPointedAt keeps it up while the pointer is over the slider. Returns the opacity, 0 to 1. */
		float Tick(float DeltaSeconds, bool bPointedAt = false)
		{
			Linger = (bHeld || bPointedAt) ? PreviewLingerSeconds : FMath::Max(Linger - DeltaSeconds, 0.f);
			Opacity = FMath::FInterpConstantTo(Opacity, Linger > 0.f ? 1.f : 0.f, DeltaSeconds, PreviewFadeSpeed);
			return Opacity;
		}

	private:
		bool bHeld = false;
		float Linger = 0.f;
		float Opacity = 0.f;
		float GrabbedValue = 0.f;
	};
}
