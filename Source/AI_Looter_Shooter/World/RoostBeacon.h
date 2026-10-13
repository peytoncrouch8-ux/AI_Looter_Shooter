#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "World/GraveMarkRules.h"
#include "RoostBeacon.generated.h"

/**
 * A Hob's roost's beacon, the light seen from across the map (the user's call, 2026-10-13): dark until the roost is found,
 * then a flame in the mast's fire basket under a tall, wide, soft column of light in the roost's one colour (Hob's Ember,
 * URoostSettings::BeaconColor, its hot core FlameCoreColor): far taller and wider than a loot beam, so the form tells them
 * apart as well as the colour. Cheap, and right on Medium's Nanite fallback: no light at all, only the gunfire's additive
 * glow (M_FX_Glow) on the engine's plane, which isn't Nanite, as one instanced draw that is never culled by distance:
 *  - the core: three crossed upright cards of the glow's streak, its bright head at the basket, fading to nothing at the top;
 *  - the haze: three wider, fainter crossed cards about it, so the column reads soft rather than as a line;
 *  - the flame: three crossed round cards in the basket, and a faint halo round them;
 *  - a pulse: a round glow that climbs the column when the roost is found, made the save point again, or the crows bring
 *    someone home.
 * As the roost is found the column shoots up out of the basket (RoostRules::BeamRise) while the flame flares and a pulse
 * climbs. Near the camera the column fades away (NearGone to NearFull), so a player standing under it is never blinded.
 *
 * It ticks only while its show runs or the camera is near enough to fade it (the roost's poll says when: WatchView); at
 * rest far off it costs nothing but its one draw.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API URoostBeaconComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	URoostBeaconComponent(const FObjectInitializer& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Lays its cards out for the settings' look (the roost calls it once it stands; calling it again changes nothing). */
	void BuildCards();

	/** Lit (the roost found) or dark, at once, with no show: a level beginning with the roost found, a mark taken back. */
	void SetLit(bool bInLit);
	bool IsLit() const { return bLit; }

	/** Found: lit, the column shoots up out of the post, the flame flares and a pulse climbs it. */
	void Ignite();

	/** A pulse climbs the column and the flame lifts (made the save point again; someone brought home). Nothing while dark. */
	void Pulse();

	/** Moves its show on by DeltaSeconds (the tick; a test level never ticks, so the tests call it). */
	void Advance(float DeltaSeconds);

	/** Fades the column for a camera Distance (cm) from the beacon (the tick, from the player's camera; tests). */
	void SetViewDistance(float Distance);

	/** Near enough that the camera fades it: it ticks to follow the camera. Off, it is whole again and stops. */
	void WatchView(bool bWatch);

	/** Its show (the rise, a flare, a pulse) is running. */
	bool IsShowing() const;

	// --- What it shows now (the tests) ---

	/** The core's height over the post (cm); 0 while dark. */
	float GetBeamLength() const;
	/** The core's and the flame's strength (the glow's), and the share left by the camera's nearness. */
	float GetBeamStrength() const { return BeamShown; }
	float GetFlameStrength() const { return FlameShown; }
	float GetNearFade() const { return NearFade; }
	/** Where the pulse is (0 the post, 1 the top), or below zero with none. */
	float GetPulseHeight() const { return PulseShown; }

	/** The cards' order in the instances: the core's three, the haze's three, the flame's three, the halo's two, the pulse's two. */
	static constexpr int32 CoreFirst = 0;
	static constexpr int32 HazeFirst = 3;
	static constexpr int32 FlameFirst = 6;
	static constexpr int32 HaloFirst = 9;
	static constexpr int32 PulseFirst = 11;
	static constexpr int32 CardCount = 13;

private:
	/** Every card where the show has it now, and how bright. */
	void Apply();

	bool bLit = false;
	bool bWatching = false;
	/** Seconds into the column's rise, a flare (and its kind) and a pulse; below zero: none running. */
	float RiseAge = -1.f;
	float FlareAge = -1.f;
	EGraveMark FlareKind = EGraveMark::None;
	float PulseAge = -1.f;
	/** The share of the column the camera's nearness leaves (1 far off). */
	float NearFade = 1.f;

	/** What it showed last (the tests read them; Apply skips work when nothing has moved). */
	float BeamShown = 0.f;
	float FlameShown = 0.f;
	float PulseShown = -1.f;
	float LengthShown = 0.f;
};
