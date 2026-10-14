#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

/**
 * The mission tracker's marks (the user's call, 2026-10-14: circles, checked once done, instead of the old straight
 * lines): the objective's, the step row's and the optional objectives'. Drawn once per session through the kit's
 * painted pictures (HudMissionTrackerShapes.cpp).
 */
enum class EHudTrackerMark : uint8
{
	/** The objective to do: an orange ring. */
	Objective,
	/** Done: a cyan disc (the experience bar's two halves) with an ink check, as the level gem has its number in ink. */
	ObjectiveDone,
	/** A step done: a small checked disc. */
	StepDone,
	/** The step being played: an orange ring round an orange dot. */
	StepNow,
	/** A step still to come: a faint ring. */
	StepToCome,
	/** An optional objective: a dashed cyan ring, so it never reads as the job. */
	Optional,
	/** Done: a small checked disc. */
	OptionalDone,
	Count
};

/**
 * The mission tracker's pictures, shared by its layout (UHudMissionTrackerWidget, HudMissionTrackerWidgetLayout.cpp), its
 * painting (HudMissionTrackerWidgetPaint.cpp) and its optional rows (HudMissionTrackerWidgetOptional.cpp); nothing else uses
 * them. Sizes are 1080p pixels, as everything on the HUD.
 */
namespace HudTrackerShapes
{
	/** The medal's box (it's 32 px across, centred in it). The marks' column under it is as wide, so every mark centres under it. */
	constexpr float MedalBox = 34.f;

	/** The keycap: 22 px high and at least as wide (its middle stretches to the key's width). */
	constexpr float KeycapSize = 22.f;

	/** A mark's box on screen: a check's is a little bigger than its ring's, room for its ink edge and its pop. */
	FVector2D MarkSize(EHudTrackerMark Mark);

	/** A mark's picture, drawn once for the session. */
	FSlateBrush MarkBrush(EHudTrackerMark Mark);

	/** The medal: its metal (rings, open where the glass is), its glass (a background) and the ranger's star over them. */
	FSlateBrush MedalMetalBrush();
	FSlateBrush MedalGlassBrush();
	FSlateBrush MedalStarBrush();

	/** The keycap's plate (a background) and its cyan edge, drawn as boxes whose ends keep their shape as the middle stretches. */
	FSlateBrush KeycapPlateBrush();
	FSlateBrush KeycapEdgeBrush();
}
