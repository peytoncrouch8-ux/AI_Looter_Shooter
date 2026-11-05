#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

/**
 * The boarding stage's numbers and rules as plain math (Main 6 "The Pilot Light", MAINS_2_7.md section 10; lane MAIN67's
 * W3), so the tests check them without a level: the barge's own frame (cm; X along it, its bow +X; Y across, its berth side
 * -Y; Z up from its keel's pivot, as SM_DredgeBarge is built), her decks as zones, the berth and the deck's pieces. The
 * furnace hall below the deck, its hatch, stair and side room, the grate ring, the vents' clock, the dying and the fall are
 * BargeHallRules.h (included at the end). ABargeBoarding builds and plays them; ABargeFurnace plays the grate ring.
 *
 * Since 2026-11-04 the hull is the user's Tripo SM_DredgeBarge jumboed (Docs/Work/2026-11-04-Art/CONTRACT.md "Round 3": "Use
 * the Tripo model and adjust it"): a well deck between the house and the scoop's raised floor (the forecastle), walks past
 * the house to the cross walk between the house and the casing, where a hatch opens over the hall's hollow under the
 * casing; all with the hull's own collision. The kit adds no structure on her: the hall below, the cover, vents, horns,
 * the falling third stack and the drum's gantry. Numbers marked "the hull's" are the shared contract with that model,
 * measured on the built mesh in BARGE_LAYOUT.md (round 9): change both together.
 */
namespace BargeRules
{
	// --- Main 6's names and steps (from 0), the same as lane MAIN67's contract and W2's PilotLight constants
	// (World/PilotSortieRules.h); kept here too so the barge needs nothing of the sortie's to build ---

	AI_LOOTER_SHOOTER_API FName Mission();          // "Main6"
	AI_LOOTER_SHOOTER_API FName DrumJammedFlag();   // "Pilot.DrumJammed"
	AI_LOOTER_SHOOTER_API FName BargeFellFlag();    // "Pilot.BargeFell"
	inline constexpr int32 GrappleStep = 2;
	inline constexpr int32 DeckStep = 3;
	inline constexpr int32 ManifestStep = 4;
	inline constexpr int32 FurnaceStep = 5;
	inline constexpr int32 CageStep = 6;
	inline constexpr int32 EscapeStep = 7;
	inline constexpr int32 ReadyStep = 8;

	/** A straight stair: its top's and its foot's middles (the barge's frame, Z from the keel). */
	struct FStairRun
	{
		FVector From;
		FVector To;
	};

	// --- The hull (the hull's: BARGE_LAYOUT.md's bounds; the pivot at the bottom, where the hose drum hangs; bow +X) ---

	inline constexpr float HullAft = -3750.f;
	inline constexpr float HullFore = 5590.f;
	inline constexpr float HullHalfBeam = 1237.5f;
	inline constexpr float HullTop = 4809.f;

	// --- Her decks as zones (the hull's, BARGE_LAYOUT.md): where the kit's spots and pieces may stand, each a flat box at
	// its own height, kept to what the layout measured clear (the walks and the well deck are wider than their boxes) ---

	enum class EDeckZone : uint8 { WellDeck, Forecastle, Walk, CrossWalk, Stern };
	struct FDeckZone
	{
		const TCHAR* Name;
		EDeckZone Kind;
		float XMin;
		float XMax;
		float YMin;
		float YMax;
		float Z;
		FBox2D Box() const { return FBox2D(FVector2D(XMin, YMin), FVector2D(XMax, YMax)); }
	};
	/** The well deck: the arena from the house's fore face (its low hull to x 2238) to the forecastle stairs' foot, the
	 *  bulwark's inner face's nearest station either side (18.8 x 13.3 m: the hull's own beam), the berth's gap in its port
	 *  rail. */
	inline constexpr float WellDeckAft = 2245.f;
	inline constexpr float WellDeckFore = 4031.f;
	inline constexpr float WellDeckHalfBeam = 665.f;
	/**
	 * The decks' heights. The level's drops in play find some decks a few cm under the hull's own collision tops (PIE,
	 * 2026-11-04, the round-3 hull: the forecastle 2370-2371 over a 2376.8 top, the cross walk 2252.6 over 2260.2; the
	 * editor's traces find the tops), so each zone's height sits between the two, within the hand (5 cm) every floor check
	 * holds to. Round 4's tops (BARGE_LAYOUT.md "Deck zones"): the well deck 2254 (its look 2249..2252), the forecastle
	 * 2373, the walks 2257 / 2258, the cross walk 2260, the stern 2252.
	 */
	inline constexpr float WellDeckZ = 2250.f;
	/** The forecastle: the scoop's floor, the high ground over the well deck; the stairs top out at its aft edge. The scoop's
	 *  frame (round 4: 17 tube hulls, x 4320..4518) stands across it between its two clear parts: aft, from the stairs'
	 *  heads to the frame (the scoop's walls at |y| 508 / 566), and fore of it, narrowing between the walls (traced clear to
	 *  2 m over the floor on a 20 cm grid, 2026-11-04). */
	inline constexpr float ForecastleAft = 4130.f;
	inline constexpr float ForecastleAftFore = 4320.f;
	inline constexpr float ForecastleAftPort = -490.f;
	inline constexpr float ForecastleAftStarboard = 540.f;
	inline constexpr float ForecastleFrontAft = 4530.f;
	inline constexpr float ForecastleFore = 5015.f;
	inline constexpr float ForecastleFrontPort = -360.f;
	inline constexpr float ForecastleFrontStarboard = 420.f;
	inline constexpr float ForecastleZ = 2371.f;
	/** The house walks (a flank each, well deck to the cross walk): the strip clear at every station (the wall's foot's
	 *  outermost to the bulwark's innermost; the walks are 2.4 m at their median); a walk's mouth its fore end this deep. */
	inline constexpr float WalkAft = -294.f;
	inline constexpr float WalkFore = 2000.f;
	inline constexpr float WalkPortOuter = -790.f;
	inline constexpr float WalkPortInner = -725.f;
	inline constexpr float WalkStarboardInner = 745.f;
	inline constexpr float WalkStarboardOuter = 815.f;
	inline constexpr float WalkZ = 2257.f;
	inline constexpr float WalkMouth = 300.f;
	/** The cross walk between the casing's fore face and the house's aft face (its wall's hull from x -315), where the hatch
	 *  is; the hatch's stair starts at its height. */
	inline constexpr float CrossWalkAft = -665.f;
	inline constexpr float CrossWalkFore = -320.f;
	inline constexpr float CrossWalkHalfBeam = 935.f;
	inline constexpr float CrossWalkZ = 2256.f;
	/** The stern platform aft of the casing: clear inside its corner bulwarks and aft of the casing's aft wall (its hull x
	 *  -3411..-3361; the walks' floors start at -3360); its floor is wider (the aft rail wall spans that). */
	inline constexpr float SternAft = -3650.f;
	inline constexpr float SternFore = -3420.f;
	inline constexpr float SternHalfBeam = 500.f;
	inline constexpr float SternFloorHalfBeam = 700.f;
	inline constexpr float SternZ = 2250.f;
	inline constexpr FDeckZone DeckZones[] = {
		{ TEXT("WellDeck"), EDeckZone::WellDeck, WellDeckAft, WellDeckFore, -WellDeckHalfBeam, WellDeckHalfBeam, WellDeckZ },
		{ TEXT("ForecastleAft"), EDeckZone::Forecastle, ForecastleAft, ForecastleAftFore, ForecastleAftPort, ForecastleAftStarboard, ForecastleZ },
		{ TEXT("Forecastle"), EDeckZone::Forecastle, ForecastleFrontAft, ForecastleFore, ForecastleFrontPort, ForecastleFrontStarboard, ForecastleZ },
		{ TEXT("WalkPort"), EDeckZone::Walk, WalkAft, WalkFore, WalkPortOuter, WalkPortInner, WalkZ },
		{ TEXT("WalkStarboard"), EDeckZone::Walk, WalkAft, WalkFore, WalkStarboardInner, WalkStarboardOuter, WalkZ },
		{ TEXT("CrossWalk"), EDeckZone::CrossWalk, CrossWalkAft, CrossWalkFore, -CrossWalkHalfBeam, CrossWalkHalfBeam, CrossWalkZ },
		{ TEXT("Stern"), EDeckZone::Stern, SternAft, SternFore, -SternHalfBeam, SternHalfBeam, SternZ },
	};
	inline constexpr int32 NumDeckZones = UE_ARRAY_COUNT(DeckZones);
	/** The zone a flat point stands in (the first that holds it), or null. */
	AI_LOOTER_SHOOTER_API const FDeckZone* DeckZoneAt(const FVector2D& At);
	/** The point on her decks at its zone's height, Up over it (off every zone: the well deck's height). */
	AI_LOOTER_SHOOTER_API FVector OnDeck(const FVector2D& At, float Up = 0.f);

	/** A flat footprint on her decks (the barge's frame). */
	struct FDeckFootprint
	{
		float XMin;
		float XMax;
		float YMin;
		float YMax;
	};
	/** Her own fittings over the decks the kit keeps its pieces off (the hull's): the scoop's lattice frame over the
	 *  forecastle (its tubes from 27 cm over the floor). */
	inline constexpr FDeckFootprint DeckFittingList[] = { { 4320.f, 4518.f, -395.f, 558.f } };
	inline TConstArrayView<FDeckFootprint> DeckFittings() { return DeckFittingList; }
	/** True when a flat point is on a deck zone and clear of her fittings, the hatch's coaming and the forecastle's stairs:
	 *  where a spot or a piece of the kit may stand. */
	AI_LOOTER_SHOOTER_API bool IsOpenDeck(const FVector2D& At);

	/**
	 * Her rail line (the hull's, from BARGE_LAYOUT.md's bulwark hulls: each run its stretch along her, its inner face (signed:
	 * + starboard, - port) and its deck's height), stern to bow on each side. The bulwark curves and is low in places (a few
	 * runs stand 7 cm over the deck), so the kit stands an unseen pawn-only wall on each run, just outside its inner face,
	 * so nobody climbs or steps over it and falls. The well deck's port run has no collision of the hull's (its rail is
	 * Tripo's railing): its inner face mirrors the starboard's, and it's cut at the berth's gap.
	 */
	struct FRailRun
	{
		float XMin;
		float XMax;
		float YInner;
		float Z;
	};
	inline constexpr FRailRun RailLine[] = {
		{ -3585.f, -3396.f, 518.f, SternZ }, { -3396.f, -3087.f, 705.f, SternZ }, { -3087.f, -2784.f, 882.f, WalkZ },
		{ -2784.f, -2494.f, 920.f, WalkZ }, { -2494.f, -2193.f, 930.f, WalkZ }, { -2193.f, -1898.f, 937.f, WalkZ },
		{ -1898.f, -1592.f, 938.f, WalkZ }, { -1592.f, -1270.f, 947.f, WalkZ }, { -1270.f, -999.f, 951.f, WalkZ },
		{ -999.f, -680.f, 960.f, WalkZ }, { -680.f, -398.f, 979.f, CrossWalkZ }, { -398.f, -82.f, 979.f, WalkZ },
		{ -82.f, 228.f, 971.f, WalkZ }, { 228.f, 512.f, 966.f, WalkZ }, { 512.f, 814.f, 960.f, WalkZ },
		{ 814.f, 1110.f, 956.f, WalkZ }, { 1110.f, 1400.f, 921.f, WalkZ }, { 1400.f, 1745.f, 885.f, WalkZ },
		{ 1745.f, 2002.f, 851.f, WalkZ }, { 2002.f, 2321.f, 783.f, WellDeckZ }, { 2321.f, 4096.f, 757.f, WellDeckZ },
		{ -3553.f, -3392.f, -526.f, SternZ }, { -3392.f, -3080.f, -674.f, SternZ }, { -3080.f, -2790.f, -844.f, WalkZ },
		{ -2790.f, -2486.f, -890.f, WalkZ }, { -2486.f, -2194.f, -894.f, WalkZ }, { -2194.f, -1898.f, -901.f, WalkZ },
		{ -1898.f, -1597.f, -905.f, WalkZ }, { -1597.f, -1296.f, -912.f, WalkZ }, { -1296.f, -998.f, -914.f, WalkZ },
		{ -998.f, -695.f, -920.f, WalkZ }, { -695.f, -391.f, -941.f, CrossWalkZ }, { -391.f, -89.f, -940.f, WalkZ },
		{ -89.f, 201.f, -934.f, WalkZ }, { 201.f, 505.f, -930.f, WalkZ }, { 505.f, 802.f, -924.f, WalkZ },
		{ 802.f, 1108.f, -909.f, WalkZ }, { 1108.f, 1400.f, -879.f, WalkZ }, { 1400.f, 1706.f, -851.f, WalkZ },
		{ 1706.f, 2003.f, -820.f, WalkZ }, { 2003.f, 2287.f, -777.f, WellDeckZ }, { 2287.f, 4096.f, -757.f, WellDeckZ },
	};
	/** One of the unseen pawn-only rail walls (flat, the barge's frame), RailWallHeight tall over its Z. */
	struct FRailWall
	{
		FName Name;
		FBox2D Footprint;
		float Z;
	};
	/** The rail walls, by name: one on each rail run (the well deck's port one cut at the berth's gap), and one across the
	 *  stern platform's aft edge (the hull has none there). */
	AI_LOOTER_SHOOTER_API TArray<FRailWall> RailWallLayout();
	inline constexpr float RailWallThick = 30.f;
	inline constexpr float RailWallHeight = 320.f;
	/** The kit draws its own low rail along the rail walls only over the stand-in box (the hull has Tripo's); true draws it
	 *  over the hull too. */
	inline constexpr bool bDrawRails = false;
	/** The kit's floor plates over the stand-in box (one per zone, cut round the hatch's hole), this thick. */
	inline constexpr float PlateThickness = 30.f;
	/** A flat piece cut round a hole: the up to four pieces of it outside the hole (the piece whole if they don't meet). */
	AI_LOOTER_SHOOTER_API TArray<FBox2D> CutAround(const FBox2D& Piece, const FBox2D& Hole);

	/** The forecastle's two stairs up from the well deck (the hull's: the model has them, two flights at |y| 150..450; the
	 *  kit lays its own only over the stand-in box), port (0) and starboard (1): from the forecastle's aft edge down this far
	 *  to the well deck, this far either side of the middle, this wide either side of that. */
	inline constexpr float ForecastleStairAcross = 300.f;
	inline constexpr float ForecastleStairRun = 180.f;
	inline constexpr float ForecastleStairHalfWidth = 150.f;
	inline constexpr int32 NumForecastleStairs = 2;
	inline constexpr bool bKitForecastleStairs = false;
	AI_LOOTER_SHOOTER_API FStairRun ForecastleStair(int32 Index);

	// --- The berth: where the Lamplighter lies grappled (W2's ship flies to it), her starboard to the barge's -Y side ---

	/** The berth side (-1: the barge's port, -Y) and its place along her: the gap in the well deck's port rail (the hull's:
	 *  x 3005..3300, its Gangway socket at 3150), so the plank lands mid-deck and the fight is all round it. */
	inline constexpr float BerthSide = -1.f;
	inline constexpr float BerthX = 3150.f;
	/** Her rail at the berth (the hull's: its rim this far over the well deck, from its inner face to her side outboard this
	 *  thick, her side this far out), its gap's sill over the deck (the plank must pass over it), the gap this wide either
	 *  side of the berth. */
	inline constexpr float RailHeight = 90.f;
	inline constexpr float BulwarkThick = 70.f;
	inline constexpr float BulwarkHalfBeam = 827.f;
	inline constexpr float SillUp = 12.f;
	inline constexpr float GapHalf = 145.f;
	/** Her keel's line out from the barge's middle: her side's, her bag's (SM_Lamplighter 10.54 m) and a gap. */
	inline constexpr float BerthOut = 1520.f;
	/** Her deck under the barge's (negative: over it, so the plank runs down over the gap's sill onto the deck), her keel
	 *  4.22 m under her deck (ALamplighterShip::DeckTop), her rail 3.75 m out (::RailOut), her bag 5.27 m out from her keel. */
	inline constexpr float BerthUnderDeck = -100.f;
	inline constexpr float ShipDeckOverKeel = 422.f;
	inline constexpr float ShipRailOut = 375.f;
	inline constexpr float ShipBagHalfBeam = 527.f;
	/** Her plank's hinge this far fore of her keel's middle (ALamplighterShip::PlankHingeX: fore of her locker bench): the
	 *  berths lie her keel this far aft of the gap, so the plank comes square across it. */
	inline constexpr float ShipPlankHingeX = 300.f;
	/** The plank lands this far in from the well deck's edge; its floor lies this far under its line. */
	inline constexpr float BoardingInset = 150.f;
	inline constexpr float PlankUnderside = 10.f;
	/** A player put aboard by the plank stands this far in from the deck's edge (a step past the landing); anyone put aboard
	 *  stands this far over the floor (a capsule's half height and a hand: never sunk into it). */
	inline constexpr float PlankStartIn = 200.f;
	inline constexpr float StandUp = 100.f;
	/** Step 7: she hangs under the broken gangway this far down, this far out from the barge's middle: far enough that her
	 *  bag clears the rail's top when the barge has listed her full 12 degrees toward her. */
	inline constexpr float JumpUnderDeck = 350.f;
	inline constexpr float JumpOut = 1630.f;
	/** The grapples bite the rail's top fore and aft of the berth by this much (the ship's own bites are at +-600). */
	inline constexpr float GrappleSpread = 600.f;
	/** The plank's floor over the gap's sill where it crosses the rail's inner face (cm; above 0 it clears). */
	AI_LOOTER_SHOOTER_API float PlankSillClearance();
	/** Her bag's inner edge outside her side at the berth (cm; above 0 it clears). */
	AI_LOOTER_SHOOTER_API float BerthBagClearance();
	/** At the jump berth with the barge listed ListDegrees toward her: how far her bag's inner edge stays out of the rail's
	 *  top (cm, in her level frame; above 0 it clears). */
	AI_LOOTER_SHOOTER_API float JumpBagClearance();

	// --- The deck's pieces (BARGE.md "Round 9", the lanes "Round 13"; the fight's plan: BargeFightRules.h). The player lands
	// mid-deck on the port side; the way on: a house walk aft to the cross walk, its hatch, the hall under the casing. Lanes
	// 1.5 m wide stay clear from the landing and the deck's middle along both rails to the walks' mouths: the gantry and the
	// stack stand inboard of them ---

	/** The hose drum under the keel (the hull's SOCKET_HoseDrum), and its gantry with the jam lever on the well deck's aft
	 *  port quarter (a drive shaft runs between; its housing is cover too): half its housing's size (ABargeHoseDrum's
	 *  4 x 2.6 m: change both together). */
	inline const FVector DrumUnderKeel(-889.0, 323.0, 0.0);
	inline const FVector DrumOnDeck(2560.0, -280.0, WellDeckZ);
	inline const FVector2D DrumGantryHalf(200.0, 130.0);
	/** The kit's third stack at the well deck's aft starboard corner, against the house's fore face (clear of the starboard
	 *  walk's mouth): in step 7 it breaks off and falls (degrees off -Y toward -X: -90 is toward the bow) along the deck's
	 *  starboard side, clear of the landing, the port way from the walks to the gap, and the forecastle's stairs. Its tube
	 *  this round and tall (13 m); its bolted foot flange this round: it topples about the flange's edge on the fall's side,
	 *  so it lands on its side on the deck, its doubler plate (this half size) staying behind. */
	inline const FVector FallingStackFoot(2440.0, 380.0, WellDeckZ);
	inline constexpr float StackRadius = 150.f;
	inline constexpr float StackHeight = 1300.f;
	inline constexpr float StackFallAft = -90.f;
	inline constexpr float StackFootRadius = 175.f;
	inline constexpr float StackFootHalf = 190.f;
	/** The way it falls (flat, unit), the edge it turns about (the pivot), and its top's end lying on the deck. */
	AI_LOOTER_SHOOTER_API FVector StackFallDir();
	AI_LOOTER_SHOOTER_API FVector StackFallPivot();
	AI_LOOTER_SHOOTER_API FVector StackTip();
	/** The work lamp on the house's fore face over the well deck; shadowless, warm, this bright (candelas), this far. */
	inline const FVector DeckLampSpots[] = { FVector(2300.0, 0.0, 3200.0) };
	inline constexpr float DeckLampCandelas = 35.f;
	inline constexpr float DeckLampReach = 1200.f;
	inline const FLinearColor DeckLampColor(1.f, 0.72f, 0.42f);
	/**
	 * The work lamps (the PIE look check, 2026-11-04: in Main 6's low sun the Lamplighter's shadow lies over the port half
	 * and the walks, the forecastle's stairs and the hatch are near black): the mine lantern (BargeLooks::HallLantern) on a
	 * short iron bracket, each with a shadowless light in DeckLampColor this bright (candelas), this far; dark as she dies
	 * and lit again with her, like the hall's. Each: where it's fixed (her frame), how, and the way it faces (flat; a post's
	 * arm reaches that way): standing on a rim's top on an iron foot plate; on a post up from a wall's top, hung from the
	 * arm's end; or hung from a rod down from a roof's underside. The posts and rods start a hand inside the iron they're
	 * fixed to (the layout gives the tops and undersides by their hulls), so none floats off it.
	 *  - The well deck's bulwarks, on the rim's top (its middle, |y| 792, z 2341), aft and fore of the gantry and the gap:
	 *    a lantern's height over the rail, under the Lamplighter's bag even listed at the jump (79 over the rail), clear of
	 *    the grapples' bites (x 2550 and 3750 on the port rim).
	 *  - The forecastle: posts on the scoop's side walls' tops (z 2564 / 2569) by the stairs' heads, fore of the port
	 *    strip's spots.
	 *  - The forecastle's fore half (the PIE walk, 2026-11-04: "a near-black pit"): posts 2.3 m up from its floor at the
	 *    scoop's walls' feet (clear of the floor's edge by 15 cm), fore of the frame.
	 *  - Each walk's mouth, from the house's overhang (underside z 2701); the cross walk, from the house's aft overhang (x
	 *    -358..-315, z 2700) either side of the hatch, one over the restart.
	 * The PIE walk's "their light is weak and the deck stays dusk-dark": 40 cd each; the perf lane's 1.65 ms for her lights at
	 * the deck (PERF.md): 6.5 m each, local pools, not a flood.
	 */
	enum class ELampMount : uint8 { Rim, Post, Drop };
	struct FWorkLamp
	{
		ELampMount Mount;
		FVector Fixed;
		FVector2D Out;
		/** A post's height over what it stands in, a rod's length under the roof (a rim's lamp: none). */
		float Up;
	};
	inline const FWorkLamp WorkLamps[] = {
		{ ELampMount::Rim, FVector(2400.0, 792.0, 2341.0), FVector2D(0.0, -1.0), 0.f }, { ELampMount::Rim, FVector(3550.0, 792.0, 2341.0), FVector2D(0.0, -1.0), 0.f },
		{ ELampMount::Rim, FVector(2400.0, -792.0, 2341.0), FVector2D(0.0, 1.0), 0.f }, { ELampMount::Rim, FVector(3550.0, -792.0, 2341.0), FVector2D(0.0, 1.0), 0.f },
		{ ELampMount::Post, FVector(4170.0, 590.0, 2564.0), FVector2D(0.0, -1.0), 50.f }, { ELampMount::Post, FVector(4170.0, -530.0, 2569.0), FVector2D(0.0, 1.0), 50.f },
		{ ELampMount::Post, FVector(4870.0, 445.0, ForecastleZ), FVector2D(0.0, -1.0), 230.f },
		{ ELampMount::Post, FVector(4870.0, -385.0, ForecastleZ), FVector2D(0.0, 1.0), 230.f },
		{ ELampMount::Drop, FVector(1950.0, -757.0, 2701.0), FVector2D(1.0, 0.0), 80.f }, { ELampMount::Drop, FVector(1950.0, 780.0, 2701.0), FVector2D(1.0, 0.0), 80.f },
		{ ELampMount::Drop, FVector(-336.0, 300.0, 2700.0), FVector2D(-1.0, 0.0), 80.f }, { ELampMount::Drop, FVector(-336.0, -450.0, 2700.0), FVector2D(-1.0, 0.0), 80.f },
	};
	inline constexpr int32 NumWorkLamps = UE_ARRAY_COUNT(WorkLamps);
	inline constexpr float WorkLampCandelas = 40.f;
	inline constexpr float WorkLampReach = 650.f;
	/**
	 * Her lights by where the player is (the perf lane's finding): the deck's (the house's lamp and the work lamps) on
	 * while the player is on her deck or within LampsNear of its middle; the hall's lanterns, the stoker's and the cage's
	 * only while the player is below her deck (in her footprint, from under her hall's floor to this far under the cross
	 * walk's deck: on the hatch's stair or in the hall).
	 */
	inline constexpr float LampsNear = 9000.f;
	inline constexpr float BelowDeckUnder = 40.f;
	/** The iron (cm): a post's arm this long, all of it this thick, each post and rod sunk this far into what it's fixed
	 *  to; a rim's foot plate this thick. */
	inline constexpr float WorkLampArm = 40.f;
	inline constexpr float WorkLampIron = 5.f;
	inline constexpr float WorkLampSunk = 10.f;
	inline constexpr float WorkLampPlate = 2.f;
	/** Where a work lamp's lantern is seated (her frame): standing on a rim, the plate's top under its bounds' bottom;
	 *  hung, the arm's end or the rod's foot over its bounds' top. */
	AI_LOOTER_SHOOTER_API FVector WorkLampSeat(const FWorkLamp& Lamp);
	inline bool WorkLampHangs(const FWorkLamp& Lamp) { return Lamp.Mount != ELampMount::Rim; }
	/** The furnace vents, each a 2 x 2 m grille glowing through its slots, in the lanes between the cover (one across the
	 *  landing's way inboard). */
	inline const FVector VentSpots[] = { FVector(3000.0, -100.0, WellDeckZ), FVector(3350.0, 350.0, WellDeckZ), FVector(2480.0, 40.0, WellDeckZ),
		FVector(3800.0, -50.0, WellDeckZ) };
	inline constexpr int32 NumVents = UE_ARRAY_COUNT(VentSpots);
	inline constexpr float VentHalf = 100.f;
	/** A grille's plate this thick, its middle this far over the deck: a centimetre proud (nothing to trip on or to fight the
	 *  deck). Its rim and slats (looks only, no collision) stand this high and this wide over it, so it reads as a grille. */
	inline constexpr float VentGrilleThick = 10.f;
	inline constexpr float VentGrilleUp = 1.f - VentGrilleThick * 0.5f;
	inline constexpr float VentRimUp = 6.f;
	inline constexpr float VentRimWidth = 12.f;
	inline constexpr int32 VentSlats = 6;
	inline constexpr float VentSlatUp = 3.f;
	inline constexpr float VentSlatWidth = 7.f;
	/** The cover: props the Dredgers crouch behind (REUSE's picks: BargeLooks' Barrel, Crate, Sandbags, HoseCart, Winch;
	 *  BargeBoardingProps.cpp lays them), each where its rifleman faces the landing past it, mid-deck cover for the player
	 *  and his first off the plank. Each its mesh's own size and collision (a block of the mesh's bounds where it has none);
	 *  without the mesh a block of CoverFallbackHalf. Barrels stand in twos and threes. */
	enum class ECover : uint8 { Barrel, BarrelPair, BarrelTrio, Crate, Sandbags, HoseCart, Winch };
	struct FCoverPiece
	{
		ECover Kind;
		float X;
		float Y;
		float Z;
		float Yaw;
	};
	inline constexpr FCoverPiece CoverPieces[] = {
		{ ECover::Crate, 3616.f, 176.f, WellDeckZ, 0.f }, { ECover::BarrelTrio, 3605.f, -310.f, WellDeckZ, 0.f },
		{ ECover::HoseCart, 2760.f, 60.f, WellDeckZ, 0.f }, { ECover::BarrelTrio, 3000.f, 150.f, WellDeckZ, 30.f },
		{ ECover::Barrel, 3400.f, -250.f, WellDeckZ, 0.f }, { ECover::BarrelPair, 4790.f, 150.f, ForecastleZ, 0.f },
		{ ECover::Winch, 4950.f, -200.f, ForecastleZ, 90.f }, { ECover::BarrelPair, 3500.f, -560.f, WellDeckZ, 15.f },
	};
	inline constexpr int32 NumCoverPieces = UE_ARRAY_COUNT(CoverPieces);
	/** The stand-in blocks' half sizes, in ECover's order (only without the mesh: each mesh is laid at its own size): about
	 *  the props' own, from their makers' notes. */
	inline const FVector CoverFallbackHalves[] = { FVector(30.0, 30.0, 44.0), FVector(30.0, 60.0, 44.0), FVector(55.0, 60.0, 44.0),
		FVector(50.0, 40.0, 40.0), FVector(100.0, 30.0, 50.0), FVector(110.0, 55.0, 90.0), FVector(60.0, 50.0, 45.0) };
	AI_LOOTER_SHOOTER_API FVector CoverFallbackHalf(ECover Kind);
	/** The sandbag wall is stacked two high when one is lower than this (riflemen crouch behind it). Barrels in a cluster
	 *  stand this far apart, each turned this much more than the one before (their bungs and dents don't line up). */
	inline constexpr float SandbagsStackUnder = 90.f;
	inline constexpr float BarrelGap = 2.f;
	inline constexpr float BarrelTurn = 37.f;
	/** The casing's roof (the hull's: its high hulls' tops), where her two stacks stand. */
	inline constexpr float CasingRoofAft = -3290.f;
	inline constexpr float CasingRoofFore = -680.f;
	inline constexpr float CasingRoofHalfBeam = 600.f;
	inline constexpr float CasingRoofZ = 3200.f;
	AI_LOOTER_SHOOTER_API FBox2D CasingRoof();
	/** Her back half's fires as she dies, each on the surface it burns from: the stern platform, the casing's roof (clear of
	 *  its cabins and her two stacks). */
	inline const FVector SternFireSpots[] = { FVector(-3500.0, -400.0, SternZ), FVector(-3500.0, 400.0, SternZ), FVector(-3000.0, -400.0, CasingRoofZ),
		FVector(-2900.0, 450.0, CasingRoofZ), FVector(-1950.0, -470.0, CasingRoofZ), FVector(-1200.0, 450.0, CasingRoofZ) };
	inline constexpr int32 NumSternFires = UE_ARRAY_COUNT(SternFireSpots);
	/** Where her smoke trails from in the fall: over the casing's roof. */
	inline const FVector FallSmokeSpot(-2000.0, 0.0, CasingRoofZ + 150.0);
}

#include "World/BargeHallRules.h"
#include "World/BargeFightRules.h"
