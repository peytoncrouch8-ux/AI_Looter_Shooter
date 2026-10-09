#include "UI/Inventory/MapPlaces.h"

namespace
{
	// Ransom's Rest: Art/Levels/RansomsRest/layout.json, preview.labels ([X, Y] world cm). The places past the ring of
	// ridges (Larkspur Ridge, Gravewind Canyon, Mill Gorge, the Hogback, East Ridge) are named too: they're what the
	// player sees from inside.
	const FMapPlace RansomsRestPlaces[] =
	{
		{ TEXT("Ransom Farm"),         FVector2D(-3600.0, -6600.0), true },
		{ TEXT("Ransom's Point"),      FVector2D(-7900.0, -9300.0), true },
		{ TEXT("Main Street"),         FVector2D(-500.0, 2700.0), true },
		{ TEXT("Boot Hill"),           FVector2D(3300.0, -800.0), true },
		{ TEXT("Chapel of Saint Ada"), FVector2D(6800.0, -1900.0), true },
		{ TEXT("The Sink"),            FVector2D(4600.0, 6500.0), true },
		{ TEXT("Whitlock Fields"),     FVector2D(-6000.0, 6600.0), true },
		{ TEXT("Undertaker's Yard"),   FVector2D(-1300.0, 9200.0), true },
		{ TEXT("Depot"),               FVector2D(1500.0, 11400.0), true },
		{ TEXT("Gravewind Point"),     FVector2D(1700.0, -15000.0), true },
		{ TEXT("Widow's Pines"),       FVector2D(6400.0, -8000.0), true },
		{ TEXT("Family plot"),         FVector2D(-3200.0, -9100.0), false },
		{ TEXT("Mooring Ledge"),       FVector2D(-10700.0, -9700.0), false },
		{ TEXT("Stage Gap"),           FVector2D(300.0, 13700.0), false },
		{ TEXT("Gravewind Canyon"),    FVector2D(0.0, -17200.0), false },
		{ TEXT("Dry Wash"),            FVector2D(1500.0, -7200.0), false },
		{ TEXT("Coffin Rock"),         FVector2D(-6000.0, -1500.0), false },
		{ TEXT("Mill Falls"),          FVector2D(-8600.0, 7500.0), false },
		{ TEXT("Mill Gorge"),          FVector2D(-11600.0, 8600.0), false },
		{ TEXT("The Nose"),            FVector2D(-8800.0, 1000.0), false },
		{ TEXT("Larkspur Ridge"),      FVector2D(11800.0, -1000.0), false },
		{ TEXT("East Ridge"),          FVector2D(-3200.0, 14200.0), false },
		{ TEXT("The Hogback"),         FVector2D(-11600.0, -2500.0), false },
		{ TEXT("Old Quarry"),          FVector2D(9700.0, 2000.0), false },
		{ TEXT("Stone Teeth"),         FVector2D(-4200.0, 11000.0), false },
		{ TEXT("Hearse Rock"),         FVector2D(3300.0, 11300.0), false },
		{ TEXT("Three Widows"),        FVector2D(500.0, -7700.0), false },
		{ TEXT("Keeper's Gate"),       FVector2D(1200.0, -12400.0), false },
		{ TEXT("The Tumble"),          FVector2D(-8500.0, -5000.0), false },
		{ TEXT("Spoil Bank"),          FVector2D(6600.0, 2900.0), false },
		{ TEXT("Webwood"),             FVector2D(7400.0, 8200.0), false },
		{ TEXT("Rimrock"),             FVector2D(-3500.0, -10000.0), false },
		{ TEXT("Keeper's grave"),      FVector2D(2300.0, -11000.0), false },
	};

	// Skyreach (the tutorial island): Art/Levels/TutorialIsland/layout.json's zones and labels, by the names the game
	// gives them now (Crossroads Town, Web Hollow, the Wallow, Range Practice, the lookout's posting).
	const FMapPlace SkyreachPlaces[] =
	{
		{ TEXT("Crossroads Town"), FVector2D(-200.0, 0.0), true },
		{ TEXT("The Farm"),        FVector2D(-5600.0, -4800.0), true },
		{ TEXT("Web Hollow"),      FVector2D(5800.0, 5600.0), true },
		{ TEXT("The Wallow"),      FVector2D(2400.0, -5600.0), true },
		{ TEXT("The Range"),       FVector2D(5000.0, -1400.0), true },
		{ TEXT("The Lookout"),     FVector2D(-6200.0, 6000.0), true },
		{ TEXT("Orchard"),         FVector2D(-6600.0, -7000.0), false },
		{ TEXT("Pond"),            FVector2D(1800.0, 5600.0), false },
		{ TEXT("Windmill"),        FVector2D(6600.0, -3800.0), false },
		{ TEXT("Pasture"),         FVector2D(-5900.0, -100.0), false },
		{ TEXT("The Plateau"),     FVector2D(-4000.0, 4200.0), false },
	};
}

namespace MapPlaces
{
	TConstArrayView<FMapPlace> For(const FString& LevelName)
	{
		if (LevelName.Equals(TEXT("Lvl_RansomsRest"), ESearchCase::IgnoreCase))
		{
			return MakeArrayView(RansomsRestPlaces);
		}
		if (LevelName.Equals(TEXT("Lvl_TutorialIsland"), ESearchCase::IgnoreCase))
		{
			return MakeArrayView(SkyreachPlaces);
		}
		return TConstArrayView<FMapPlace>();
	}
}
