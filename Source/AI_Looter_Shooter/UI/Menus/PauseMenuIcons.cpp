// PauseMenuIcons: the mockup's line icons (common.js ICON: play, gear, door, power, health, xp, mission; e.js: monitor,
// speaker, layers, keys) as painted pictures. SVG arcs are worked out here as circles: each arc's centre and angles are
// from its path's end points and radius (degrees from three o'clock, clockwise, y down).

#include "UI/Menus/PauseMenuIcons.h"
#include "UI/Menus/PauseMenuArtInk.h"

using namespace LooterUI;
using namespace PauseMenuArtInk;

namespace
{
	const FName IconIds[] = {
		TEXT("Resume"), TEXT("Settings"), TEXT("SaveQuit"), TEXT("Quit"),
		TEXT("Graphics"), TEXT("Audio"), TEXT("Interface"), TEXT("Controls"),
		TEXT("Health"), TEXT("Experience"), TEXT("Mission"),
	};

	FVector2D P(float X, float Y)
	{
		return FVector2D(X, Y);
	}

	/** A box's outline as a closed line. */
	TArray<FVector2D> Box(float X, float Y, float W, float H)
	{
		return { P(X, Y), P(X + W, Y), P(X + W, Y + H), P(X, Y + H), P(X, Y) };
	}

	FPaintedIcon Make(std::initializer_list<FPaintLayer> Layers)
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(24.f, 24.f);
		Icon.Layers = Layers;
		return Icon;
	}

	/** White, to be tinted (made here, not copied from FLinearColor::White: no order between files' statics). */
	const FLinearColor White(1.f, 1.f, 1.f, 1.f);
}

TConstArrayView<FName> PauseMenuIcons::Ids()
{
	return MakeArrayView(IconIds);
}

FPaintedIcon PauseMenuIcons::Get(FName Id)
{
	// --- The options ---
	if (Id == TEXT("Resume"))
	{
		// A play mark: back to the game.
		return Make({ FillLayer({ { P(7.f, 4.5f), P(20.f, 12.f), P(7.f, 19.5f) } }, White) });
	}
	if (Id == TEXT("Settings"))
	{
		// A gear: the hub, eight teeth, the rim.
		TArray<TArray<FVector2D>> Teeth = { { P(12.f, 2.5f), P(12.f, 5.5f) }, { P(12.f, 18.5f), P(12.f, 21.5f) }, { P(2.5f, 12.f), P(5.5f, 12.f) },
			{ P(18.5f, 12.f), P(21.5f, 12.f) }, { P(5.3f, 5.3f), P(7.4f, 7.4f) }, { P(16.6f, 16.6f), P(18.7f, 18.7f) }, { P(5.3f, 18.7f), P(7.4f, 16.6f) },
			{ P(16.6f, 7.4f), P(18.7f, 5.3f) } };
		return Make({ StrokeLayer({ CirclePoints(P(12.f, 12.f), 3.4f, true) }, 2.f, White), StrokeLayer(MoveTemp(Teeth), 2.4f, White),
			StrokeLayer({ CirclePoints(P(12.f, 12.f), 6.6f, true) }, 1.8f, White) });
	}
	if (Id == TEXT("SaveQuit"))
	{
		// A door with an arrow walking out of it, its knob.
		return Make({ StrokeLayer({ Box(5.f, 3.f, 10.f, 18.f) }, 1.8f, White),
			StrokeLayer({ { P(13.f, 12.f), P(21.f, 12.f) }, { P(18.f, 8.5f), P(21.5f, 12.f), P(18.f, 15.5f) } }, 2.f, White),
			FillLayer({ CirclePoints(P(11.5f, 12.f), 1.1f, false) }, White) });
	}
	if (Id == TEXT("Quit"))
	{
		// A power mark: a ring open at the top (radius 8 round (12, 12.6), from its top-left round under to its top-right),
		// the stroke down into the gap.
		return Make({ StrokeLayer({ ArcPoints(P(12.f, 12.6f), 8.f, 233.13f, -53.13f) }, 2.2f, White),
			StrokeLayer({ { P(12.f, 2.5f), P(12.f, 11.5f) } }, 2.4f, White) });
	}

	// --- The settings tabs ---
	if (Id == TEXT("Graphics"))
	{
		return Make({ StrokeLayer({ Box(2.5f, 4.f, 19.f, 12.5f) }, 1.9f, White),
			StrokeLayer({ { P(9.f, 20.5f), P(15.f, 20.5f) }, { P(12.f, 16.5f), P(12.f, 20.5f) } }, 1.9f, White) });
	}
	if (Id == TEXT("Audio"))
	{
		// A speaker and two waves (radius 5 and 8.8, each round a centre left of its chord).
		return Make({ FillLayer({ { P(3.5f, 9.f), P(7.5f, 9.f), P(12.5f, 5.f), P(12.5f, 19.f), P(7.5f, 15.f), P(3.5f, 15.f) } }, White),
			StrokeLayer({ ArcPoints(P(11.93f, 12.f), 5.f, -44.4f, 44.4f), ArcPoints(P(11.955f, 12.f), 8.8f, -44.8f, 44.8f) }, 1.9f, White) });
	}
	if (Id == TEXT("Interface"))
	{
		// Three layers stacked.
		return Make({ StrokeLayer({ { P(12.f, 3.f), P(21.5f, 8.f), P(12.f, 13.f), P(2.5f, 8.f), P(12.f, 3.f) },
			{ P(2.5f, 12.5f), P(12.f, 17.5f), P(21.5f, 12.5f) }, { P(2.5f, 16.5f), P(12.f, 21.5f), P(21.5f, 16.5f) } }, 1.9f, White) });
	}
	if (Id == TEXT("Controls"))
	{
		// A keyboard: its case, two rows of keys, the space bar.
		return Make({ StrokeLayer({ Box(2.f, 6.f, 20.f, 12.5f) }, 1.9f, White),
			StrokeLayer({ { P(5.5f, 9.5f), P(7.f, 9.5f) }, { P(9.5f, 9.5f), P(11.f, 9.5f) }, { P(13.5f, 9.5f), P(15.f, 9.5f) }, { P(17.2f, 9.5f), P(18.5f, 9.5f) },
				{ P(5.5f, 12.5f), P(7.f, 12.5f) }, { P(17.2f, 12.5f), P(18.5f, 12.5f) }, { P(8.f, 15.5f), P(16.f, 15.5f) } }, 1.9f, White) });
	}

	// --- The hero card's rows and the mission card's seal ---
	if (Id == TEXT("Health"))
	{
		return Make({ StrokeLayer({ { P(12.f, 2.f), P(22.f, 12.f), P(12.f, 22.f), P(2.f, 12.f), P(12.f, 2.f) } }, 1.8f, White),
			StrokeLayer({ { P(12.f, 7.f), P(12.f, 17.f) }, { P(7.f, 12.f), P(17.f, 12.f) } }, 2.6f, White) });
	}
	if (Id == TEXT("Experience"))
	{
		return Make({ StrokeLayer({ { P(12.f, 2.f), P(14.9f, 8.2f), P(21.7f, 9.f), P(16.7f, 13.6f), P(18.1f, 20.3f), P(12.f, 17.f), P(5.9f, 20.3f),
			P(7.3f, 13.6f), P(2.3f, 9.f), P(9.1f, 8.2f), P(12.f, 2.f) } }, 1.8f, White) });
	}
	if (Id == TEXT("Mission"))
	{
		return Make({ StrokeLayer({ CirclePoints(P(12.f, 12.f), 10.f, true) }, 1.8f, White),
			FillLayer({ { P(12.f, 5.5f), P(13.9f, 9.5f), P(18.3f, 10.f), P(15.f, 13.f), P(15.9f, 17.4f), P(12.f, 15.2f), P(8.1f, 17.4f), P(9.f, 13.f),
				P(5.7f, 10.f), P(10.1f, 9.5f) } }, White) });
	}

	FPaintedIcon Empty;
	Empty.ViewBox = FVector2D(24.f, 24.f);
	return Empty;
}
