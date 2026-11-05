#pragma once

// The second half of BargeRules (World/BargeRules.h includes it at its end; include that): the furnace hall below the deck,
// the hatch and its stair down into it, the stoker's corner on its catwalk, the grate ring, the vents' clock, the hose, her
// dying and fall, and keeping the player aboard. Its math is BargeHallRules.cpp.

#include "World/BargeRules.h"

namespace BargeRules
{
	// --- The furnace hall below the deck, in the hull's hollow under the casing (the hull's: BARGE_LAYOUT.md's cavity) ---

	/** The user's call (2026-11-04): "Below deck, smaller", about 14 m round; the contract asks at least this radius. */
	inline constexpr float HallLeastRadius = 650.f;
	/** The gap the hall's wall keeps from her skin's inside, and her nearest skin from the hall's axis over its height (the
	 *  hull's: the layout measures to the skin's inner face). */
	inline constexpr float HullWall = 20.f;
	inline constexpr float HallSkinHalfBeam = 827.f;
	inline constexpr float HallRadius = 750.f;
	/** Its middle along her (on her middle line, under the casing), its floor (the kit's, on the hollow's bottom) and the
	 *  hollow's top, where the kit's ceiling hangs (Tripo made no deck inside the casing: the ceiling closes it). */
	inline constexpr float HallCenterX = -1185.f;
	inline const FVector HallCenter(HallCenterX, 0.0, 0.0);
	inline constexpr float HallFloor = 1300.f;
	inline constexpr float HallCeiling = 2220.f;
	inline constexpr float CeilingThickness = 20.f;
	/** Its walls from the floor to the ceiling's underside, at least the contract's 9 m (room for the catwalk and the
	 *  hatch's stair over it), this thick; the floor's box runs this far past the wall along her (under the alcoves and the
	 *  stairs' feet), this thick; the floor's riveted disc (a look) this thick. */
	inline constexpr float HallHeight = HallCeiling - CeilingThickness - HallFloor;
	inline constexpr float HallLeastHeight = 900.f;
	inline constexpr float WallThickness = 30.f;
	inline constexpr float HallFloorOut = 140.f;
	inline constexpr float HallFloorThick = 30.f;
	inline constexpr float HallFloorLookThick = 10.f;
	/** The hall's round wall in this many panels, each this much wider than its share (they overlap at the joints). A door
	 *  is one panel's width either side of its middle, so it's HallDoorHalf() wide either side at the wall's inner face. */
	inline constexpr int32 HallWallPanels = 36;
	inline constexpr float HallWallPanelStretch = 1.04f;
	inline constexpr float DoorGapDegrees = 360.f / HallWallPanels;
	AI_LOOTER_SHOOTER_API float HallDoorHalf();
	/** A look or a wall that meets a floor's top stops this far under it, so no two faces share a plane (no flicker). */
	inline constexpr float LookDrop = 1.f;
	/** Kit pieces past the hall's wall keep this far inside her skin (so its curve never shows them through it). */
	inline constexpr float SkinGap = 20.f;
	/** True when a round hall of Radius fits her beam (HallSkinHalfBeam) with the wall's gap all round. */
	AI_LOOTER_SHOOTER_API bool HallFits(float Radius);
	/** The flat square the hall's ceiling covers: the hall to its wall's outer face. */
	AI_LOOTER_SHOOTER_API FBox2D HallCeilingArea();

	// --- The hatch (the hull's: its coaming's hole in the cross walk, over the hall's fore rim) ---

	/** The coaming's hole (flat, the barge's frame) and the way its stair runs down it (along Y; the stair's top is the
	 *  hole's edge behind it, flush with the deck). */
	inline constexpr float HatchHoleXMin = -611.f;
	inline constexpr float HatchHoleXMax = -436.f;
	inline constexpr float HatchHoleYMin = -220.f;
	inline constexpr float HatchHoleYMax = 225.f;
	inline const FVector2D HatchWay(0.0, -1.0);
	AI_LOOTER_SHOOTER_API FBox2D HatchHole();
	/** The coaming round it (the hull's: a riveted rim this thick, its top this high over the cross walk; open on the stair's
	 *  top side): the hole and its rim, kept clear of the kit's pieces. */
	inline constexpr float HatchCoamingThick = 20.f;
	inline constexpr float HatchCoamingUp = 50.f;
	AI_LOOTER_SHOOTER_API FBox2D HatchCoaming();
	/** The deck the hatch is cut in (its zone's height). */
	AI_LOOTER_SHOOTER_API float HatchDeckZ();
	/** The hatch, as the story knows it: the stair's top on the deck (the Reach marker and the getter). A player put back
	 *  after the manifest stands this far behind it, facing down the stair. */
	AI_LOOTER_SHOOTER_API FVector HatchSpot();
	inline constexpr float HatchStartOut = 150.f;

	/** The hall's front: 0 (the hatch fore of its middle) or 180 (aft). The hall's angles (degrees) run from it, so
	 *  everything in the hall turns with the hatch. */
	inline constexpr float OfficeAngle = (HatchHoleXMin + HatchHoleXMax) * 0.5f >= HallCenterX ? 0.f : 180.f;
	/** A point in the hall at Angle (from OfficeAngle), Radius out, Up over its floor (the barge's frame). */
	AI_LOOTER_SHOOTER_API FVector HallPoint(float AngleDegrees, float Radius, float Up = 0.f);

	// --- In the hall ---

	/** The catwalk ring this far up (where the hatch's stair comes down onto it: high enough that the stair fits inside the
	 *  hall's wall from the hole over its rim), this deep from the wall: its inner edge and its middle (where the floor stairs
	 *  run); its spans this thick under their top; the hand rail along its inner edge (a look) this far out from it, this
	 *  high over the walk, this wide and this deep. */
	inline constexpr float CatwalkUp = 580.f;
	inline constexpr float CatwalkDepth = 200.f;
	inline constexpr float CatwalkInner = HallRadius - CatwalkDepth;
	inline constexpr float CatwalkMiddle = HallRadius - CatwalkDepth * 0.5f;
	inline constexpr float CatwalkThick = 20.f;
	inline constexpr float CatwalkRailIn = 6.f;
	inline constexpr float CatwalkRailUp = 100.f;
	inline constexpr float CatwalkRailWidth = 6.f;
	inline constexpr float CatwalkRailDepth = 8.f;
	/** The hand rail stops pawns (the PIE walk, 2026-11-04: off the inner edge it was a 5.8 m fall): an unseen wall along
	 *  every laid span's inner edge (not over the floor stairs' openings), the rail's height and its look's width; its
	 *  stanchions and a mid rail this high (the kit's iron). The hatch's stair has the same on both its sides, this far in
	 *  from its edges. */
	inline constexpr float CatwalkRailMidUp = 50.f;
	inline constexpr float CatwalkStanchion = 5.f;
	inline constexpr float HatchRailIn = 3.f;
	/** The rails' unseen walls stand higher than their looks (no jumping over them): the ring's to the hall's ceiling (the
	 *  ends' height), the hatch stair's this high over its treads in this many upright pieces (no overhang past its foot). */
	inline constexpr float HatchRailWallUp = 200.f;
	inline constexpr int32 HatchRailPieces = 8;
	/** The stokehold doors (the FurnaceHall pushes come through them), across her. */
	inline constexpr float StokeholdAngles[] = { 90.f, 270.f };
	/** A stokehold's alcove: as deep past the hall's wall as the skin allows (1.2 m at most), as wide as its door
	 *  (AlcoveHalfWidth: its sides on the door's jambs, so what's in it shows), its walls this thick, its door this tall (a
	 *  lintel closes the hall's wall above it), its floor this thick; its iron door swung back against its side, this thick,
	 *  this far off the side and short of the roof. */
	inline constexpr float AlcoveMostDepth = 120.f;
	inline constexpr float AlcoveWall = 20.f;
	inline constexpr float AlcoveDepth = HallSkinHalfBeam - SkinGap - AlcoveWall - HallRadius < AlcoveMostDepth
		? HallSkinHalfBeam - SkinGap - AlcoveWall - HallRadius : AlcoveMostDepth;
	AI_LOOTER_SHOOTER_API float AlcoveHalfWidth();
	inline constexpr float AlcoveUp = 380.f;
	inline constexpr float AlcoveFloorThick = 30.f;
	inline constexpr float AlcoveDoorThick = 12.f;
	inline constexpr float AlcoveDoorGap = 20.f;
	/**
	 * The two stairs from the catwalk down to the hall's floor (the player comes down the hatch onto the catwalk and goes down
	 * one; Dredgers on the ring shoot down), mirror images either side of the hall's middle line: each a chord of the ring
	 * under the catwalk's middle, its top on a span's edge by a stokehold, running this far round the ring the way given (+1:
	 * the angles grow toward its foot) down to the floor between the stokehold and the back valve. The catwalk is open over
	 * each (its spans whose middles lie over it aren't laid): it rises through the ring onto the span past its top; an unseen
	 * wall closes the ring's end past its foot.
	 */
	inline constexpr float FloorStairAngles[] = { 80.f, 280.f };
	inline constexpr float FloorStairWays[] = { 1.f, -1.f };
	inline constexpr int32 NumFloorStairs = UE_ARRAY_COUNT(FloorStairAngles);
	inline constexpr float FloorStairSpan = 70.f;
	/** The catwalk ring in this many straight spans, each this much longer than its share (they overlap at the joints). */
	inline constexpr int32 CatwalkSpans = 18;
	inline constexpr float CatwalkSpanStretch = 1.06f;
	/** Span Index's middle (degrees round the hall), and whether it's laid (not under the hatch's stair nor over a floor
	 *  stair). */
	AI_LOOTER_SHOOTER_API float CatwalkSpanAngle(int32 Index);
	AI_LOOTER_SHOOTER_API bool IsCatwalkSpanLaid(int32 Index);
	/** Where the ring ends past each floor stair's foot (degrees round the hall, one per stair), and past the hatch stair's top
	 *  (its far side, where nothing comes down): the near edge of the first laid span there, where an unseen end wall stands. */
	AI_LOOTER_SHOOTER_API float CatwalkEndAngle(int32 Stair);
	AI_LOOTER_SHOOTER_API float HatchCatwalkEnd();
	/** Every stair: no steeper than this (the player walks up to 44.8 degrees, creatures 50; the tests hold every stair to
	 *  it), this wide, its ramp this thick, its treads (looks only) this high a step. */
	inline constexpr float StairMostSlope = 40.f;
	inline constexpr float StairWidth = 160.f;
	inline constexpr float StairThick = 24.f;
	inline constexpr float StairTreadRise = 20.f;
	/** The looks' sizes: a tread's plate this thick, the stringers either side this thick and this deep, their top edge this
	 *  far over the ramp's line. */
	inline constexpr float StairTreadThick = 5.f;
	inline constexpr float StairStringerThick = 6.f;
	inline constexpr float StairStringerDepth = 30.f;
	inline constexpr float StairStringerProud = 10.f;
	/** A stair from the catwalk down to the hall's floor (Index into FloorStairAngles), and any stair's slope (degrees). */
	AI_LOOTER_SHOOTER_API FStairRun FloorStair(int32 Index);
	AI_LOOTER_SHOOTER_API float StairSlope(const FStairRun& Stair);

	// --- The hatch's stair, and the stoker's corner on the catwalk ---

	/**
	 * The stair from the hatch down onto the catwalk, inside the hall's wall all the way: a steep ship's stair (under the
	 * stairs' most), this narrow, its line along the hole this far along her (toward the hall's middle, so its outer edge
	 * clears the wall at its foot); a player's head and a hand over his feet (the tests' capsule) clear the ceiling past the
	 * hole's far edge. The catwalk under it isn't laid (it comes down through the ring onto the span past its foot).
	 */
	inline constexpr float HatchStairSlope = 38.f;
	inline constexpr float HatchStairWidth = 100.f;
	inline constexpr float HatchStairX = -558.f;
	inline constexpr float StairHeadroom = 240.f;
	AI_LOOTER_SHOOTER_API FStairRun HatchStair();
	/** The hole's least length along its stair for that headroom (cm). */
	AI_LOOTER_SHOOTER_API float HatchHoleLeast();
	/** The hall angles (degrees from OfficeAngle, -180..180) the hatch stair spans over the ring, from its foot's to its top's. */
	AI_LOOTER_SHOOTER_API void HatchStairArc(float& OutFoot, float& OutTop);
	/**
	 * The stoker's corner on the catwalk between the hatch stair's foot and the port floor stair's top (the way the player
	 * walks): his desk (the gunsmith's bench at its own size; without it a block of this half size) at this angle, its back
	 * (the bench's tool wall) to the hall's wall this far off it, its work top into the hall's walkway (the PIE walk, 2026-11-04:
	 * turned the other way the tool wall faced the aisle and the manifest lay on its top, 2.1 m up), its long side along the
	 * ring; the manifest on the work top (the bench's Gun socket, the clear middle of its top: GunsmithBench.py's TOP 0.95 m,
	 * the socket this far over it), the radio on its back corner (its middle this far in from the top's middle, along the ring,
	 * up: half its height), this big. His safe across the hall on the ring's other side of the
	 * hatch (by the stair's foot the player steps off toward the wall), this far off the wall (without its mesh a block this
	 * big).
	 */
	inline constexpr float DeskAngle = 316.f;
	inline const FVector DeskFallbackHalf(45.0, 90.0, 47.5);
	inline constexpr float DeskGap = 2.f;
	inline constexpr float DeskWorkTop = 95.f;
	inline FName DeskTopSocket() { return FName(TEXT("Gun")); }
	inline constexpr float DeskSocketOverTop = 3.f;
	inline const FVector RadioOnDesk(-20.0, 60.0, 15.0);
	inline const FVector RadioSize(30.0, 45.0, 30.0);
	inline constexpr float SafeAngle = 40.f;
	inline constexpr float SafeGap = 10.f;
	inline const FVector SafeFallbackHalf(41.0, 41.0, 41.0);
	/** The catwalk's walking height (the corner's floor). */
	inline constexpr float CatwalkFloor = HallFloor + CatwalkUp;
	/** The desk's foot's middle for a desk HalfDepth deep (off the wall), its yaw (its local X, its work side, into the hall),
	 *  its work top's middle (the bench reads its own: ABargeBoarding::GetOfficeSpot), the radio's spot on a desk whose top's
	 *  middle is OnDesk and its yaw (dials into the hall), and the safe's foot. */
	AI_LOOTER_SHOOTER_API FVector DeskSpot(float HalfDepth);
	AI_LOOTER_SHOOTER_API float DeskYaw();
	AI_LOOTER_SHOOTER_API FVector DeskTop();
	AI_LOOTER_SHOOTER_API FVector RadioSpot(const FVector& OnDesk);
	AI_LOOTER_SHOOTER_API float RadioYaw();
	AI_LOOTER_SHOOTER_API FVector SafeSpot();

	// --- The furnace, the grates, the valves, the hall's fight, its lights and its coal ---

	/** The soul furnace in the middle: 5 m across (the smaller hall), 6 m tall; its cage's window faces the hall's front; its
	 *  three pipes rise from this far under its crown into the hall's ceiling. */
	inline constexpr float FurnaceRadius = 250.f;
	inline constexpr float FurnaceHeight = 600.f;
	inline constexpr float CageUp = 250.f;
	inline constexpr float PipeUnderCrown = 25.f;
	/** The furnace's three pipes this thick; a draft valve's riser this thick, up into the catwalk's underside (the ring runs
	 *  over every valve: the PIE walk saw one through the grating by the port stair). */
	inline constexpr float FurnacePipeWidth = 55.f;
	inline constexpr float ValveRiserWidth = 45.f;
	inline constexpr float ValveRiserTop = CatwalkUp - CatwalkThick;
	/** The floor grate ring: twelve wedges from this far past the furnace (the safe floor by it) to this far inside the wall
	 *  (the safe floor under the catwalk, where the stairs land). */
	inline constexpr int32 Grates = 12;
	inline constexpr float GrateSafeIn = 90.f;
	inline constexpr float GrateSafeOut = 180.f;
	inline constexpr float GrateInner = FurnaceRadius + GrateSafeIn;
	inline constexpr float GrateOuter = HallRadius - GrateSafeOut;
	/** The three draft valves on their pipe runs at the hall's wall (degrees), this far in from it. */
	inline constexpr float ValveAngles[] = { 60.f, 180.f, 300.f };
	inline constexpr int32 NumValves = UE_ARRAY_COUNT(ValveAngles);
	inline constexpr float ValveIn = 30.f;
	/** The hall fight's eight spots (FurnaceHall): four at each stokehold door in two rows, this far either side of its middle,
	 *  the first row this far in from the hall's wall, the second this much further in. */
	inline constexpr int32 HallFightSpots = 8;
	inline constexpr float HallSpotAcross = 120.f;
	inline constexpr float HallSpotIn = 150.f;
	inline constexpr float HallSpotRow = 180.f;
	/** The wall lanterns (shadowless, warm) so the hall isn't dark: four on its wall under the catwalk, one over the stoker's
	 *  desk; this bright (candelas), this far. */
	inline constexpr float HallLanternAngles[] = { 45.f, 135.f, 225.f, 315.f };
	inline constexpr float HallLanternUp = 250.f;
	inline constexpr float OfficeLanternUp = 260.f;
	inline constexpr float LanternCandelas = 8.f;
	inline constexpr float LanternReach = 900.f;
	inline const FLinearColor LanternColor(1.f, 0.7f, 0.42f);
	AI_LOOTER_SHOOTER_API FVector OfficeLanternSpot();
	/** Coal (looks only): sacks against the wall either side of each stokehold's door, this far in from the wall and this far
	 *  past the door's jamb; soul coal on the safe floor by the furnace, round its back; each lump or sack turned this much
	 *  more than the one before (no two lie alike). */
	inline constexpr float CoalSackIn = 60.f;
	inline constexpr float CoalSackBeside = 60.f;
	inline constexpr float SoulCoalAngles[] = { 130.f, 205.f, 235.f };
	inline constexpr float CoalTurn = 30.f;

	/** True when a hall angle (degrees from OfficeAngle) lies within Half of Target, the long way round too. */
	AI_LOOTER_SHOOTER_API bool IsNearAngle(float Angle, float Target, float Half);

	// --- The grate ring: a roulette wheel of heat ---

	/** Each turn lasts this long; for its first TellSeconds the hot wedges only glow, then they burn. */
	inline constexpr float GrateTurnSeconds = 3.f;
	inline constexpr float GrateTellSeconds = 1.f;
	/** Hot wedges with no valve shut (4 of 12), cooling one notch a valve: 4, 3, 2, then none once all three are shut. */
	AI_LOOTER_SHOOTER_API int32 HotGrates(int32 ValvesShut);
	/** The wedges hot Time seconds into the fight (spread evenly round the ring, the whole pattern stepping one wedge each
	 *  turn): a bit per wedge. */
	AI_LOOTER_SHOOTER_API uint32 HotMask(float Time, int32 ValvesShut);
	/** The hot wedges are burning now (past the turn's tell). */
	AI_LOOTER_SHOOTER_API bool IsBurning(float Time);
	/** The wedge (0-11) under a point in the furnace's frame (flat), or INDEX_NONE off the ring. */
	AI_LOOTER_SHOOTER_API int32 GrateAt(const FVector2D& FromHallMiddle);
	/** A burning wedge hurts this share of the player's health a second; a vent's column this. */
	inline constexpr float GrateBurnShare = 0.14f;
	inline constexpr float VentBurnShare = 0.22f;
	/** Hurt is dealt in pulses this far apart (no stream of tiny numbers). */
	inline constexpr float HurtEvery = 0.25f;

	// --- The deck vents' clock ---

	inline constexpr float VentEvery = 6.f;
	inline constexpr float VentTellSeconds = 1.f;
	inline constexpr float VentBurnSeconds = 1.4f;
	inline constexpr float VentColumnRadius = 130.f;
	inline constexpr float VentColumnHeight = 450.f;
	/** Each vent's clock starts this much after the one before, so the deck flares in a running pattern, never all at once. */
	inline constexpr float VentStagger = 0.75f;
	enum class EVentPhase : uint8 { Quiet, Tell, Burn };
	/** Vent Index's phase Time seconds after the vents woke. */
	AI_LOOTER_SHOOTER_API EVentPhase VentPhase(int32 Index, float Time);

	// --- The hose ---

	/** How far the hose reaches toward the chapel's roof (0-1): hanging short while she's grappled, snaking lower through the
	 *  deck fight and the office, nearly on the roof by the hall (never on it: she's stopped before it gets there). */
	AI_LOOTER_SHOOTER_API float HoseReach(int32 Step, float SecondsInStep);
	inline constexpr float HoseLowerPerSecond = 0.004f;

	// --- Dying (step 7) and the fall (BargeFall) ---

	inline constexpr float ListDegrees = 12.f;
	inline constexpr float ListSeconds = 6.f;
	/** She drifts west on the dusk wind (world -Y: the level's X is north, Y east), easing in, then holding this speed. */
	inline constexpr float DriftSpeed = 150.f;
	inline constexpr float DriftEaseSeconds = 4.f;
	inline constexpr float DriftMost = 6000.f;
	inline constexpr float StackFallsAt = 3.5f;
	inline constexpr float StackFallSeconds = 1.6f;
	/** How far she's drifted DyingSeconds after she began to die (cm, along the drift). */
	AI_LOOTER_SHOOTER_API float DriftAt(float DyingSeconds);
	/** Her list (degrees, her berth side down) DyingSeconds after she began to die. */
	AI_LOOTER_SHOOTER_API float ListAt(float DyingSeconds);
	/** BargeFall: 12 s; the glide west over the Rest, then the plunge into the canyon from PlungeAt; the fireball at ImpactAt. */
	inline constexpr float FallSeconds = 12.f;
	inline constexpr float PlungeAt = 9.f;
	inline constexpr float ImpactAt = 11.2f;
	/** Without a placed target: the fall ends this far on from where it began (world cm). */
	inline const FVector FallFallback(0.0, -30000.0, -9000.0);
	/** Her pose Seconds into the fall, from From to To (world), nosing down and rolling as she goes. */
	AI_LOOTER_SHOOTER_API FTransform FallPose(const FTransform& From, const FVector& To, float Seconds);
	/** Soul coal raining off her in the fall: this many lumps, one every CoalEvery, each seen this long. */
	inline constexpr int32 CoalLumps = 40;
	inline constexpr float CoalEvery = 0.25f;
	inline constexpr float CoalLife = 4.f;

	// --- Keeping the player aboard ---

	/** A player off both the barge and the Lamplighter this long is put back (steps 3-6), or caught onto her deck (step 7). */
	inline constexpr float OffGrace = 0.6f;
	inline constexpr float OffGraceEscape = 1.6f;
	/** Aboard: anywhere in her bounds and this much round them, from under the hall's floor to this far over her top (the
	 *  barge's frame). */
	inline constexpr float AboardMargin = 150.f;
	inline constexpr float AboardOverRoof = 300.f;
	AI_LOOTER_SHOOTER_API bool IsAboard(const FVector& BargeLocal);
}
