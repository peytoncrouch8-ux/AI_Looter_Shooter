#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

/**
 * The boarding stage's numbers and rules as plain math (Main 6 "The Pilot Light", MAINS_2_7.md section 10; lane MAIN67's
 * W3), so the tests check them without a level: the barge's own frame (cm; X along it, its bow +X; Y across, its berth side
 * -Y; Z up from its keel's pivot, as SM_DredgeBarge is built), the walkable kit's sizes, the furnace hall that fits inside
 * the hull, the floor grate ring's turning pattern, the deck vents' clock, the dying list and drift, and the fall's pose.
 * ABargeBoarding builds and plays them; ABargeFurnace plays the grate ring.
 *
 * Since 2026-11-04 (round 4) the hull is the user's Tripo SM_DredgeBarge reworked for the kit (Tools/Blender/
 * looter_tripo_wardens.py's rework step; Docs/Work/2026-11-04-Art/CONTRACT.md "the Tripo hull REWORKED"): one flat open deck,
 * the bulwark with its gap, the aft house raised and widened to the full beam with a real door, the stacks and the tower on
 * its roof, all with their own collision. The kit adds what the hull hasn't got: the vestibule and the office inside the
 * door, the furnace hall at deck level in the house, the cover, vents, horns, the falling third stack and the drum's gantry.
 * The numbers marked "the hull's" are the shared contract with that model: change both together.
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

	// --- The hull (the hull's numbers: 75 m long, 24.75 m in the beam, pivot under its middle at the bottom where the hose
	// drum hangs, bow +X) and the walkable deck on it ---

	inline constexpr float HullLength = 7500.f;
	inline constexpr float HullHalfBeam = 1237.5f;
	/** The open deck's walking surface over the keel (the hull's). */
	inline constexpr float DeckTop = 2250.f;
	/** The arena on the open deck the kit's spots stand in: from just fore of the house to the bow, inside the bulwark's
	 *  waterway (the hull's open deck runs x -400..3350, walkable |y| <= 1100). */
	inline constexpr float DeckAft = -300.f;
	inline constexpr float DeckFore = 3300.f;
	inline constexpr float DeckHalfBeam = 1100.f;
	/** The kit's floor plates' thickness; the bulwark (the hull's: 110 high, 15 thick, its outer face on the beam); the unseen
	 *  rail walls' height over the deck (pawns only, as the Lamplighter's: nobody climbs the bulwark and falls). */
	inline constexpr float PlateThickness = 30.f;
	/** The kit's floors inside the house stand this hair over the deck's height: over any surface the hull keeps there, so the
	 *  two never fight (a 2 cm step at the door is nothing). */
	inline constexpr float InteriorFloorLift = 2.f;
	inline constexpr float RailHeight = 110.f;
	inline constexpr float BulwarkHalfBeam = HullHalfBeam;
	inline constexpr float BulwarkThick = 15.f;
	inline constexpr float RailWallHeight = 320.f;
	/** The bulwark gap's sill over the deck at the berth (the hull's; the plank must pass over it). */
	inline constexpr float SillUp = 15.f;
	/** The kit draws its own low rail round the deck only over the stand-in box (the hull has its bulwark); true draws it
	 *  over the hull too. */
	inline constexpr bool bDrawRails = false;

	// --- The house (the hull's aft block, reworked): hollow, the full beam, from the deck to its roof; the furnace hall stands
	// in it at deck level, through its door ---

	inline constexpr float HouseAft = -3300.f;
	inline constexpr float HouseFore = -400.f;
	inline constexpr float HouseHalfBeam = 1237.5f;
	inline constexpr float HouseRoof = 3300.f;
	/** Its door: 240 wide, 300 tall, at y 0 in the fore face, its threshold on the deck (the hull's). */
	inline constexpr float DoorHalfWidth = 120.f;
	inline constexpr float DoorHeight = 300.f;
	/** The house's walls (the hull's: 20 thick, so its inside is 1217.5 to a side): the kit's lining stands just inside. */
	inline constexpr float HouseWallThick = 20.f;
	/**
	 * The vestibule just inside the door, at deck level: a small lobby the kit floors, walls, lines and roofs (the house is
	 * empty inside), its aft wall the office's fore wall with a doorway as big as the house's. From the house's fore wall's
	 * lining back to VestibuleAft, this wide, this high, its walls this thick.
	 */
	inline constexpr float VestibuleAft = -580.f;
	inline constexpr float VestibuleHalfWidth = 300.f;
	inline constexpr float VestibuleHeight = 360.f;
	inline constexpr float VestibuleWall = 20.f;
	/** The vestibule's room inside its walls and lining (flat, the barge's frame). */
	AI_LOOTER_SHOOTER_API FBox2D VestibuleArea();

	/**
	 * The kit's floor plates (the barge's frame). One on the hull's own deck (bOnHullDeck) is laid only over the stand-in box
	 * (the hull's deck has its collision); the rest are always laid, drawn iron, InteriorFloorLift over the deck.
	 */
	struct FDeckPlate
	{
		float XMin;
		float XMax;
		float YMin;
		float YMax;
		bool bOnHullDeck;
	};
	inline constexpr FDeckPlate DeckPlates[] = {
		// The open deck, to the bulwark's inner face (the stand-in's only).
		{ HouseFore, DeckFore, -(BulwarkHalfBeam - BulwarkThick), BulwarkHalfBeam - BulwarkThick, true },
		// The vestibule's floor under its walls (the kit floors the house's inside).
		{ VestibuleAft - VestibuleWall, HouseFore, -(VestibuleHalfWidth + VestibuleWall), VestibuleHalfWidth + VestibuleWall, false },
	};

	/**
	 * Superstructure footprints on the deck the kit walls off with unseen boxes (pawns and shots stopped, nothing drawn), Up
	 * tall over the deck: for a hull without its own collision. The reworked hull has it: none.
	 */
	struct FDeckBlock
	{
		float XMin;
		float XMax;
		float YMin;
		float YMax;
		float Up;
	};
	inline TConstArrayView<FDeckBlock> DeckBlocks() { return {}; }

	/** True when a flat point (the barge's frame) is in the deck's arena and out of every DeckBlock: where a spot on the deck
	 *  may stand. */
	AI_LOOTER_SHOOTER_API bool IsOpenDeck(const FVector2D& At);

	/** One of the unseen pawn-only rail walls (the barge's frame, flat; RailWallHeight tall over the deck). */
	struct FRailWall
	{
		const TCHAR* Name;
		FBox2D Footprint;
	};
	/** The rail walls the kit stands, by name: on the bulwark's line fore and aft of its gap and along the far side, and
	 *  across the bow (the house is the full beam: no side walks beside it to close). */
	AI_LOOTER_SHOOTER_API TArray<FRailWall> RailWallLayout();
	/** The bow's rail wall's thickness (it doesn't stand on the bulwark): a hand's. */
	inline constexpr float RailWallThick = 30.f;

	// --- The berth: where the Lamplighter lies grappled (W2's ship flies to it), her starboard to the barge's -Y side ---

	/** The berth side (-1: the barge's port, -Y) and the berth's place along the barge: the bulwark's gap (the hull's), forward,
	 *  so the deck fight runs aft to the house. */
	inline constexpr float BerthSide = -1.f;
	inline constexpr float BerthX = 2000.f;
	/** Her keel's line out from the barge's middle: the barge's half beam, her bag's (SM_Lamplighter 10.54 m), and a gap. */
	inline constexpr float BerthOut = 1950.f;
	/** Her deck under the barge's (negative: over it, so the plank runs down over the bulwark gap's sill onto the deck), her
	 *  keel 4.22 m under her deck (ALamplighterShip::DeckTop), her rail 3.75 m out (::RailOut), her bag 5.27 m out from her
	 *  keel line. */
	inline constexpr float BerthUnderDeck = -100.f;
	inline constexpr float ShipDeckOverKeel = 422.f;
	inline constexpr float ShipRailOut = 375.f;
	inline constexpr float ShipBagHalfBeam = 527.f;
	/** The plank lands this far in from the arena's edge; its floor lies this far under its line. */
	inline constexpr float BoardingInset = 150.f;
	inline constexpr float PlankUnderside = 10.f;
	/** A player put aboard by the plank stands this far in from the arena's edge (a step past the landing); anyone put aboard
	 *  stands this far over the floor (a capsule's half height and a hand: never sunk into it). */
	inline constexpr float PlankStartIn = 200.f;
	inline constexpr float StandUp = 100.f;
	/** Step 7: she hangs under the broken gangway this far down, this far out from the barge's middle: far enough that her
	 *  bag clears the bulwark's top when the barge has listed her full 12 degrees toward her. */
	inline constexpr float JumpUnderDeck = 350.f;
	inline constexpr float JumpOut = 2050.f;
	/** The grapples bite the bulwark's top fore and aft of the berth by this much (the ship's own bites are at +-600). */
	inline constexpr float GrappleSpread = 600.f;
	/** The bulwark's gap where the gangplank lands (and the jump is), along the barge either side of the berth (the hull's
	 *  300 wide gap). */
	inline constexpr float GapHalf = 150.f;
	/** The plank's floor over the gap's sill where it crosses the bulwark's inner face (cm; above 0 it clears). */
	AI_LOOTER_SHOOTER_API float PlankSillClearance();
	/** Her bag's inner edge outside the bulwark's outer face at the berth (cm; above 0 it clears). */
	AI_LOOTER_SHOOTER_API float BerthBagClearance();
	/** At the jump berth with the barge listed ListDegrees toward her: how far her bag's inner edge stays out of the bulwark's
	 *  top (cm, in her level frame; above 0 it clears). */
	AI_LOOTER_SHOOTER_API float JumpBagClearance();

	// --- The deck's pieces: the arena (x DeckAft..DeckFore, |y| <= DeckHalfBeam), the house its aft wall, the berth's gap on
	// its port side; the deck fight runs aft from the berth to the house's door ---

	/** The hose drum under the keel (the hull's SOCKET_HoseDrum), and its gantry with the jam lever on the deck, port side by
	 *  the house (the drum is under the house, which has no deck to stand a gantry on: a drive shaft runs between). */
	inline const FVector DrumUnderKeel(-889.0, 323.0, 0.0);
	inline const FVector DrumOnDeck(-100.0, -750.0, DeckTop);
	/** The gantry's housing on the deck, half its size (ABargeHoseDrum's 4 x 2.6 m housing: change both together). */
	inline const FVector2D DrumGantryHalf(200.0, 130.0);
	/** The kit's own third stack, just fore of the house's fore face on the starboard side (10 cm of air to the wall's
	 *  collision at x -400): it breaks off in step 7 and falls about its foot across the after deck toward the port lane, a
	 *  little forward (degrees off -Y toward -X: negative, the bow), lying to (352, -469). */
	inline const FVector FallingStackFoot(-240.0, 800.0, DeckTop);
	inline constexpr float StackRadius = 150.f;
	inline constexpr float StackHeight = 1400.f;
	inline constexpr float StackFallAft = -25.f;
	/** The work lamp over the house's door (the reworked hull has no crane to hang a second on: dropped, round 5);
	 *  shadowless, warm, this bright (candelas), this far. */
	inline const FVector DeckLampSpots[] = { FVector(HouseFore + 60.0, 0.0, HouseRoof - 120.0) };
	inline constexpr float DeckLampCandelas = 35.f;
	inline constexpr float DeckLampReach = 2600.f;
	inline const FLinearColor DeckLampColor(1.f, 0.72f, 0.42f);
	/** The furnace vents in the arena, each a 2 x 2 m grille glowing through its slots: two by the berth, two each in the
	 *  middle lanes, one right before the house's door. */
	inline const FVector VentSpots[] = { FVector(2300.0, 0.0, DeckTop), FVector(1950.0, 400.0, DeckTop), FVector(1400.0, -550.0, DeckTop),
		FVector(1400.0, 450.0, DeckTop), FVector(850.0, 0.0, DeckTop), FVector(750.0, -700.0, DeckTop), FVector(250.0, 0.0, DeckTop),
		FVector(2900.0, 450.0, DeckTop) };
	inline constexpr int32 NumVents = UE_ARRAY_COUNT(VentSpots);
	/** A grille's plate this thick, its middle this far over the deck: sunk to a centimetre proud (flush underfoot, nothing to
	 *  trip on or to fight the deck; its glow and fire stand on it the same). */
	inline constexpr float VentGrilleThick = 10.f;
	inline constexpr float VentGrilleUp = 1.f - VentGrilleThick * 0.5f;
	inline constexpr float VentHalf = 100.f;
	/** Riflemen's cover: crates and plate stacks in five rows from the berth to the house (centre, half size). */
	inline const FVector CoverSpots[] = { FVector(2500.0, 350.0, DeckTop), FVector(2400.0, -350.0, DeckTop), FVector(1700.0, -300.0, DeckTop),
		FVector(1700.0, 750.0, DeckTop), FVector(1150.0, 200.0, DeckTop), FVector(1100.0, -750.0, DeckTop), FVector(550.0, 700.0, DeckTop),
		FVector(500.0, -250.0, DeckTop), FVector(100.0, 350.0, DeckTop), FVector(-50.0, -350.0, DeckTop) };
	inline const FVector CoverHalf(110.0, 80.0, 65.0);
	/** The four horn clusters (ACompanyLoudspeaker posts the build script attaches here), at the walkable deck's sides (|y|
	 *  980, inside its 1100; the bulwark's inner face is at 1222.5). */
	inline const FVector HornSpots[] = { FVector(2800.0, -980.0, DeckTop), FVector(1900.0, 980.0, DeckTop), FVector(800.0, -980.0, DeckTop),
		FVector(300.0, 980.0, DeckTop) };
	/** Her back half's fires as she dies, each on the surface it burns from: the after deck by the house, the house's roof
	 *  (clear of its stacks and of the tower over x -2000..-1200). */
	inline const FVector SternFireSpots[] = { FVector(-200.0, -450.0, DeckTop), FVector(150.0, 750.0, DeckTop), FVector(-2900.0, -600.0, HouseRoof),
		FVector(-2400.0, 700.0, HouseRoof), FVector(-600.0, -800.0, HouseRoof), FVector(-900.0, 500.0, HouseRoof) };
	inline constexpr int32 NumSternFires = UE_ARRAY_COUNT(SternFireSpots);
	/** Where her smoke trails from in the fall: over the house's roof. */
	inline const FVector FallSmokeSpot(-2600.0, 0.0, HouseRoof + 150.0);
	/** The deck fight's nine spots (BargeDeck), each just aft of a crate: aft of the berth, so the fight runs to the house. The
	 *  build script and the spawner's groups count on nine. */
	inline const FVector DeckFightSpots[] = { FVector(2350.0, 350.0, DeckTop), FVector(2250.0, -350.0, DeckTop), FVector(1550.0, -300.0, DeckTop),
		FVector(1550.0, 750.0, DeckTop), FVector(1000.0, 200.0, DeckTop), FVector(950.0, -750.0, DeckTop), FVector(400.0, 700.0, DeckTop),
		FVector(350.0, -250.0, DeckTop), FVector(-50.0, 350.0, DeckTop) };
	/** Every spawn spot stands this far over its floor (a hand: the spawner's probes find the floor under it). */
	inline constexpr float SpotUp = 20.f;

	// --- The furnace hall at deck level, in the house ---

	/** The design asks 26 m across; the house is 24.35 m inside, so the hall is the widest round room that keeps a hull
	 *  wall's 85 cm all round inside it: 22 m (HallRadius). Its middle in the house's aft part, the office and vestibule
	 *  between its rim and the door. */
	inline constexpr float DesignHallRadius = 1300.f;
	inline constexpr float HullWall = 85.f;
	/** The half-beam the hall keeps inside: the house's walls' inner faces. */
	inline constexpr float HallSkinHalfBeam = HouseHalfBeam - HouseWallThick;
	inline constexpr float HallRadius = 1100.f;
	inline const FVector HallCenter(-2050.0, 0.0, 0.0);
	/** The hall's floor this far under the deck: none, it's on the deck's level, walked into from the office through the
	 *  house's door (its 9 m walls and its ceiling fit under the house's roof). */
	inline constexpr float HallUnderDeck = 0.f;
	inline constexpr float HallFloor = DeckTop - HallUnderDeck;
	inline constexpr float HallHeight = 900.f;
	inline constexpr float WallThickness = 30.f;
	inline constexpr int32 WallPanels = 24;
	/** The floor's box (what's stood on) runs this far past the hall's wall along the barge (under the alcoves and the
	 *  stairs' feet), across it to the skin less SkinGap; this thick. */
	inline constexpr float HallFloorOut = 140.f;
	inline constexpr float HallFloorThick = 30.f;
	/** The hall's ceiling on its walls' top, this thick, under the house's roof: the house is empty inside, so without it the
	 *  hall would look up into the shell. */
	inline constexpr float CeilingThickness = 30.f;
	/** The catwalk ring 4 m up, its inner edge, its middle (where the floor stairs run), and the stokehold doors (degrees
	 *  round the hall from its +X). */
	inline constexpr float CatwalkUp = 400.f;
	inline constexpr float CatwalkInner = 880.f;
	inline constexpr float CatwalkMiddle = (CatwalkInner + HallRadius) * 0.5f;
	inline constexpr float StokeholdAngles[] = { 90.f, 270.f };
	/** Kit pieces past the hall's wall keep this far inside the hull's skin (so its curve never shows them through it). */
	inline constexpr float SkinGap = 20.f;
	/** A stokehold's alcove: as deep past the hall's wall's inner face as the skin allows (the design's 1.2 m at most), this
	 *  wide, its walls this thick and this high (under the catwalk; a lintel closes the hall's wall above it). */
	inline constexpr float AlcoveMostDepth = 120.f;
	inline constexpr float AlcoveWall = 20.f;
	inline constexpr float AlcoveDepth = HallSkinHalfBeam - SkinGap - AlcoveWall - HallRadius < AlcoveMostDepth
		? HallSkinHalfBeam - SkinGap - AlcoveWall - HallRadius : AlcoveMostDepth;
	inline constexpr float AlcoveHalfWidth = 190.f;
	inline constexpr float AlcoveUp = CatwalkUp - 20.f;
	/**
	 * The stoker's office off the hall at 0 degrees (toward the bow), at deck level: from the hall's rim to the vestibule's
	 * aft wall (x -950..-600), this wide, as high as the vestibule and roofed, its walls this thick. Its aft wall closes it but
	 * for the doorway onto the hall's floor, as wide as the hall wall's gap there and as tall as the house's door (a lintel
	 * in the hall's wall over it).
	 */
	inline constexpr float OfficeAngle = 0.f;
	inline constexpr float OfficeDepth = 350.f;
	inline constexpr float OfficeHalfWidth = 500.f;
	inline constexpr float OfficeHeight = VestibuleHeight;
	inline constexpr float OfficeWall = 20.f;
	inline constexpr float OfficeDoorHalf = 190.f;
	inline constexpr float OfficeDoorUp = DoorHeight;
	inline constexpr float OfficeAftWall = 30.f;
	/** The office's floor (flat, the barge's frame; the office stands at 0 degrees, so it's square to the barge). */
	AI_LOOTER_SHOOTER_API FBox2D OfficeArea();
	/** The stoker's desk in the office (out from the hall's wall, across toward the port end, out of the way between the two
	 *  doorways), its top over the floor; the radio on its corner; the safe at the starboard end, this far out. */
	inline constexpr float DeskOut = 220.f;
	inline constexpr float DeskAcross = -300.f;
	inline constexpr float DeskTopUp = 82.f;
	inline const FVector RadioOnDesk(30.0, -50.0, 15.0);
	inline constexpr float SafeAcross = 400.f;
	inline constexpr float SafeOut = 120.f;
	/** The desk's top's middle (the barge's frame): where the manifest lies. */
	AI_LOOTER_SHOOTER_API FVector DeskTop();
	/** The soul furnace in the middle: 6 m across, 6 m tall; its cage's window faces the hall's +X (toward the office). */
	inline constexpr float FurnaceRadius = 300.f;
	inline constexpr float FurnaceHeight = 600.f;
	inline constexpr float CageUp = 250.f;
	/** Its three pipes rise from this far under its crown into the hall's ceiling (they used to run on up through the deck). */
	inline constexpr float PipeUnderCrown = 25.f;
	/** The floor grate ring: twelve wedges between these radii; the safe floor inside it, by the furnace, and outside it. */
	inline constexpr int32 Grates = 12;
	inline constexpr float GrateInner = 420.f;
	inline constexpr float GrateOuter = 860.f;
	/** The three draft valves on their pipe runs at the hall's wall (degrees). */
	inline constexpr float ValveAngles[] = { 60.f, 180.f, 300.f };
	inline constexpr int32 NumValves = UE_ARRAY_COUNT(ValveAngles);
	inline constexpr float ValveInset = 160.f;
	/** The hall fight's eight spots (FurnaceHall): four at each stokehold door. */
	inline constexpr int32 HallFightSpots = 8;
	/** Each door's four in two rows: this far either side of its middle, the first row this far in from the hall's wall, the
	 *  second this much further in. */
	inline constexpr float HallSpotAcross = 120.f;
	inline constexpr float HallSpotIn = 150.f;
	inline constexpr float HallSpotRow = 180.f;

	/** True when a round hall of Radius fits the hull's beam (HallSkinHalfBeam) with a hull wall's thickness all round. */
	AI_LOOTER_SHOOTER_API bool HallFits(float Radius);

	/** The flat rectangle the hall's ceiling covers: the hall to its wall's outer face (the office has its own roof). */
	AI_LOOTER_SHOOTER_API FBox2D HallCeilingArea();

	// --- The hall's stairs and the hatch (the barge's frame) ---

	/**
	 * The two stairs from the hall's floor up to the catwalk (Dredgers on it shoot down; the player can climb to them), mirror
	 * images either side of the hall's middle line: each a chord of the ring under the catwalk's middle, its top on a span's
	 * edge (degrees round the hall), running this far round the ring the way given (+1: the angles grow toward its foot) down
	 * to the floor. The catwalk is open over every stair (its spans whose middles lie over a stair aren't laid): it rises up
	 * through the ring and lands on the span past its top; an unseen wall closes the ring's end past its foot.
	 */
	inline constexpr float FloorStairAngles[] = { 220.f, 140.f };
	inline constexpr float FloorStairWays[] = { 1.f, -1.f };
	inline constexpr int32 NumFloorStairs = UE_ARRAY_COUNT(FloorStairAngles);
	inline constexpr float FloorStairSpan = 35.f;
	/** The catwalk ring in this many straight spans, each this much longer than its share (they overlap at the joints). */
	inline constexpr int32 CatwalkSpans = 18;
	inline constexpr float CatwalkSpanStretch = 1.06f;
	/** Span Index's middle (degrees round the hall), and whether it's laid (not over a stair). */
	AI_LOOTER_SHOOTER_API float CatwalkSpanAngle(int32 Index);
	AI_LOOTER_SHOOTER_API bool IsCatwalkSpanLaid(int32 Index);
	/** Where the ring ends past each stair's foot (degrees round the hall, one per stair, in FloorStairAngles' order): the near
	 *  edge of the first laid span there, where the unseen end wall stands. */
	AI_LOOTER_SHOOTER_API float CatwalkEndAngle(int32 Stair);
	/** No stair may be steeper (the player walks up to 44.8 degrees, creatures 50): the tests hold every stair to it. */
	inline constexpr float StairMostSlope = 40.f;
	inline constexpr float StairWidth = 180.f;
	inline constexpr float StairThick = 24.f;
	/** Put back aboard after the manifest: this far out on the deck from the house's door, facing it. */
	inline constexpr float HatchStartOut = 150.f;

	/** A straight stair: its top's and its foot's middles (the barge's frame, Z from the keel). */
	struct FStairRun
	{
		FVector From;
		FVector To;
	};
	/** A stair from the catwalk down to the hall's floor (Index into FloorStairAngles). */
	AI_LOOTER_SHOOTER_API FStairRun FloorStair(int32 Index);
	/** A stair's slope (degrees over the flat). */
	AI_LOOTER_SHOOTER_API float StairSlope(const FStairRun& Stair);
	/** The hatch, as the story knows it: the house's door's threshold on the deck (the Reach marker and the getter). */
	AI_LOOTER_SHOOTER_API FVector HatchSpot();

	/** A point in the hall at Angle (degrees from the hall's +X), Radius out, Up over its floor (barge frame). */
	AI_LOOTER_SHOOTER_API FVector HallPoint(float AngleDegrees, float Radius, float Up = 0.f);

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

	/** The wedge (0-11) under a point in the hall's frame (flat), or INDEX_NONE off the ring. */
	AI_LOOTER_SHOOTER_API int32 GrateAt(const FVector2D& FromHallMiddle);

	/** A burning wedge hurts this share of the player's health a second; a vent's column this. */
	inline constexpr float GrateBurnShare = 0.14f;
	inline constexpr float VentBurnShare = 0.22f;
	/** Hurt is dealt in pulses this far apart (no stream of tiny numbers). */
	inline constexpr float HurtEvery = 0.25f;

	// --- The deck vents ---

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
	/** Aboard: anywhere on her, from the house's stern to the bow, out to her bulwark, from under the deck to this far over
	 *  the house's roof (the barge's frame). */
	inline constexpr float AboardOverRoof = 300.f;
	AI_LOOTER_SHOOTER_API bool IsAboard(const FVector& BargeLocal);
}
