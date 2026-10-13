#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RoostSettings.generated.h"

/**
 * Hob's roosts, the same on every map (the user's call, 2026-10-13: one universal save, respawn and fast-travel point): the
 * beacon's one colour and look, how it fades near the player, and the life about a roost. Tunable without code: Project
 * Settings > Game > Hob's Roosts, saved to Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.RoostSettings].
 *
 * The beacon's colour is the roost's colour everywhere: the beam over the world and the icon on the map, the minimap and
 * the banner (LooterUI::Color::Roost and RoostCore read it), so one value changes them all. It must stay off the loot
 * colours (white, green, blue, purple, orange and gold): a loot beam and a roost's must never be mistaken for each other.
 * The user's pick (2026-10-13, look sheet 41): Hob's Ember, a deep ember red (the coal in Hob's empty socket), its flame's
 * core a hot ember orange. The loot's beams are columns too, so the roost's differs in form as well: far taller and wider,
 * a flame in the mast's fire basket, crows circling it (ARoostCrows).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Hob's Roosts"))
class AI_LOOTER_SHOOTER_API URoostSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	URoostSettings();

	/** The settings in force (the class's defaults where the config sets nothing). */
	static const URoostSettings& Get();

	// --- The beacon (URoostBeaconComponent) ---

	/** The roost's one colour, as written on a look sheet (sRGB): the beam's haze, the flame's edge, the icons. Hob's Ember. */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon")
	FColor BeaconColor = FColor(0xD8, 0x32, 0x1C);

	/** Its flame's hot core (sRGB): the beam's core, the flame's heart, the climbing pulse and the light on the mast. */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon")
	FColor FlameCoreColor = FColor(0xFF, 0x8A, 0x52);

	/**
	 * How tall the beam stands over the fire basket (cm): far taller than any loot's beam, over the ridges from the valley's
	 * far side.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "500", Units = "cm"))
	float BeamHeight = 9000.f;

	/** The beam's bright core and the soft haze round it (cm across at the basket: each narrows as it fades upward). Wide, unlike loot's. */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "5", Units = "cm"))
	float BeamWidth = 150.f;

	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "5", Units = "cm"))
	float HazeWidth = 560.f;

	/** How bright the core and the haze glow (the additive glow's strength: it multiplies the colour). */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "0"))
	float BeamStrength = 6.f;

	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "0"))
	float HazeStrength = 1.4f;

	/** The flame in the fire basket (cm across) and its strength; a soft halo round it, this many times as wide. */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "5", Units = "cm"))
	float FlameSize = 130.f;

	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "0"))
	float FlameStrength = 5.f;

	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "1"))
	float HaloScale = 2.8f;

	/**
	 * The beam fades as the camera comes near, so it never blinds the player standing under it: gone within NearGone (cm),
	 * whole from NearFull. The flame on the post stays (dimmed a little).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "0", Units = "cm"))
	float NearGone = 700.f;

	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "100", Units = "cm"))
	float NearFull = 3800.f;

	/** The light on the post once it's found: small, shadowless, in the roost's colour (candela, cm). */
	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "0"))
	float LightCandela = 9.f;

	UPROPERTY(Config, EditAnywhere, Category = "Beacon", meta = (ClampMin = "50", Units = "cm"))
	float LightRadius = 650.f;

	// --- The roost's life ---

	/** Hob's crows on a roost's perches (no more than its perches). */
	UPROPERTY(Config, EditAnywhere, Category = "Life", meta = (ClampMin = "0", ClampMax = "6"))
	int32 CrowCount = 4;

	/** Crows circling a found roost's beacon, high over the basket (0: none): the beacon's form from afar. */
	UPROPERTY(Config, EditAnywhere, Category = "Life", meta = (ClampMin = "0", ClampMax = "6"))
	int32 CirclingCrows = 3;

	/** Past this from the camera (cm) the circling crows hide and only keep time (they're specks by then). */
	UPROPERTY(Config, EditAnywhere, Category = "Life", meta = (ClampMin = "1000", Units = "cm"))
	float CirclingCullDistance = 15000.f;

	/** The roost's sound (RoostMark::Ambient) plays while the player is within this (cm), and stops a little past it. */
	UPROPERTY(Config, EditAnywhere, Category = "Life", meta = (ClampMin = "100", Units = "cm"))
	float AmbientRadius = 2600.f;

	/** BeaconColor and FlameCoreColor in linear colour, as the glow and the light take them. */
	FLinearColor GetBeaconColor() const;
	FLinearColor GetFlameCoreColor() const;
};
