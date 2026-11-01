#pragma once

#include "CoreMinimal.h"

/**
 * The boarding stage's numbers and rules as plain math (Main 6 "The Pilot Light", MAINS_2_7.md section 10; lane MAIN67's
 * W3), so the tests check them without a level: the barge's own frame (cm; X along it, its bow +X; Y across, its berth side
 * -Y; Z up from its keel's pivot, as SM_DredgeBarge is built), the walkable kit's sizes, the furnace hall that fits inside
 * the hull, the floor grate ring's turning pattern, the deck vents' clock, the dying list and drift, and the fall's pose.
 * ABargeBoarding builds and plays them; ABargeFurnace plays the grate ring.
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

	// --- The hull (the user's Tripo SM_DredgeBarge, lanes/IMPORT.md: 75 x 24.75 x 48.5 m with its stacks, pivot under its
	// middle at the bottom, bow +X) and the walkable kit laid over it ---

	inline constexpr float HullLength = 7500.f;
	inline constexpr float HullHalfBeam = 1237.5f;
	/** The top deck's walking surface over the keel. Measured by eye off the Tripo model's proportions (its hull is a third of
	 *  its 48.5 m height with the stacks): the build script logs it; tune DeckTop on the placed barge if it floats or sinks. */
	inline constexpr float DeckTop = 1400.f;
	/** The kit's deck: from the stern's rail to the bow's (the hull's ends taper), and half its width inside the rails. */
	inline constexpr float DeckAft = -3450.f;
	inline constexpr float DeckFore = 3350.f;
	inline constexpr float DeckHalfBeam = 1150.f;
	/** The deck plate's thickness, the visible rail's height, and the unseen rail walls' (pawns only, as the Lamplighter's). */
	inline constexpr float PlateThickness = 30.f;
	inline constexpr float RailHeight = 110.f;
	inline constexpr float RailWallHeight = 320.f;

	// --- The berth: where the Lamplighter lies grappled (W2's ship flies to it), her starboard to the barge's -Y side ---

	/** The berth side (-1: the barge's port, -Y) and the berth's place along the barge: forward, so the deck fight runs aft. */
	inline constexpr float BerthSide = -1.f;
	inline constexpr float BerthX = 2000.f;
	/** Her keel's line out from the barge's middle: its half beam, hers (the Lamplighter's bag 10.5 m), and a gap. */
	inline constexpr float BerthOut = 1950.f;
	/** Her deck sits this far under the barge's (the gangplank climbs to it), her keel 4.22 m under her deck (DeckTop 422). */
	inline constexpr float BerthUnderDeck = 150.f;
	inline constexpr float ShipDeckOverKeel = 422.f;
	/** Step 7: she hangs under the broken gangway this far down, this far out from the barge's rail (cm). */
	inline constexpr float JumpUnderDeck = 350.f;
	inline constexpr float JumpOut = 1650.f;
	/** The grapples bite the barge's rail fore and aft of the berth by this much (the ship's own bites are at +-600). */
	inline constexpr float GrappleSpread = 600.f;
	/** The rail's gap where the gangplank lands (and the jump is), along the barge either side of the berth. */
	inline constexpr float GapHalf = 220.f;

	// --- The deck's pieces (the barge's frame, Z from the keel) ---

	/** The furnace hatch, aft: a 2.2 m square hole in the deck, the stair down to the hall's catwalk under it. */
	inline constexpr float HatchX = -2700.f;
	inline constexpr float HatchHalf = 110.f;
	/** The hose drum's gantry over the drum under the keel (SM_DredgeBarge's HoseDrum socket: (-889, 323, 0)). */
	inline const FVector DrumOnDeck(-889.0, 323.0, DeckTop);
	inline const FVector DrumUnderKeel(-889.0, 323.0, 0.0);
	/** The three stacks: the Tripo model's two (Stack_1, Stack_2) and the kit's own third, the one that falls in step 7. */
	inline const FVector StackSpots[] = { FVector(-1107.0, 21.0, DeckTop), FVector(-2373.0, 19.0, DeckTop), FVector(-300.0, 380.0, DeckTop) };
	inline constexpr float StackRadius = 150.f;
	inline constexpr float StackHeight = 1400.f;
	/** The falling stack's way down: about its foot, toward the port lane, aft a little (degrees off -Y toward -X). */
	inline constexpr float StackFallAft = 25.f;
	/** The jib crane: its mast and the jib's reach. */
	inline const FVector CraneSpot(900.0, 700.0, DeckTop);
	inline constexpr float CraneHeight = 1200.f;
	inline constexpr float CraneReach = 900.f;
	/** The furnace vents along the deck, each a 2 x 2 m grille glowing through its slots. */
	inline const FVector VentSpots[] = { FVector(1100.0, -480.0, DeckTop), FVector(1100.0, 520.0, DeckTop), FVector(0.0, -520.0, DeckTop),
		FVector(0.0, 480.0, DeckTop), FVector(-1100.0, -500.0, DeckTop), FVector(-1700.0, 560.0, DeckTop), FVector(-2200.0, -520.0, DeckTop),
		FVector(2700.0, 480.0, DeckTop) };
	inline constexpr int32 NumVents = UE_ARRAY_COUNT(VentSpots);
	inline constexpr float VentHalf = 100.f;
	/** Riflemen's cover: crates and plate stacks about the deck (centre, half size). */
	inline const FVector CoverSpots[] = { FVector(2500.0, 300.0, DeckTop), FVector(1700.0, -200.0, DeckTop), FVector(1500.0, 800.0, DeckTop),
		FVector(500.0, -150.0, DeckTop), FVector(-500.0, -700.0, DeckTop), FVector(-700.0, 750.0, DeckTop), FVector(-1500.0, -200.0, DeckTop),
		FVector(-2000.0, 850.0, DeckTop), FVector(-3000.0, -600.0, DeckTop), FVector(-3100.0, 400.0, DeckTop) };
	inline const FVector CoverHalf(110.0, 80.0, 65.0);
	/** The four horn clusters (ACompanyLoudspeaker posts the build script attaches here). */
	inline const FVector HornSpots[] = { FVector(2650.0, -950.0, DeckTop), FVector(600.0, 980.0, DeckTop), FVector(-1500.0, -980.0, DeckTop),
		FVector(-2950.0, 900.0, DeckTop) };
	/** The deck fight's nine spots (BargeDeck): behind cover, aft of the berth, so the fight runs the deck's length. */
	inline const FVector DeckFightSpots[] = { FVector(1400.0, 900.0, DeckTop), FVector(400.0, 250.0, DeckTop), FVector(-300.0, -900.0, DeckTop),
		FVector(-800.0, 950.0, DeckTop), FVector(-1300.0, -450.0, DeckTop), FVector(-1900.0, 300.0, DeckTop), FVector(-2400.0, 900.0, DeckTop),
		FVector(-2900.0, -350.0, DeckTop), FVector(-3200.0, 700.0, DeckTop) };

	// --- The furnace hall below deck ---

	/** The design asks 26 m across; the hull is 24.75 m in the beam, so the hall is the widest round room that keeps a
	 *  hull wall's 85 cm all round inside it: 22 m (HallRadius). Its middle under the deck's aft half, the hatch over its rim. */
	inline constexpr float DesignHallRadius = 1300.f;
	inline constexpr float HullWall = 85.f;
	inline constexpr float HallRadius = 1100.f;
	inline const FVector HallCenter(-1800.0, 0.0, 0.0);
	inline constexpr float HallFloor = 400.f;
	inline constexpr float HallHeight = 900.f;
	inline constexpr float WallThickness = 30.f;
	inline constexpr int32 WallPanels = 24;
	/** The catwalk ring 4 m up, its inner edge, and the stokehold doors (degrees round the hall from its +X). */
	inline constexpr float CatwalkUp = 400.f;
	inline constexpr float CatwalkInner = 880.f;
	inline constexpr float StokeholdAngles[] = { 90.f, 270.f };
	/** The stoker's office off the catwalk at 0 degrees: a 4 m room past the hall's wall. */
	inline constexpr float OfficeAngle = 0.f;
	inline constexpr float OfficeDepth = 400.f;
	inline constexpr float OfficeHalfWidth = 200.f;
	/** The stoker's desk in the office (out from the hall's wall), its top over the catwalk; the radio on its corner. */
	inline constexpr float DeskOut = 220.f;
	inline constexpr float DeskTopUp = 82.f;
	inline const FVector RadioOnDesk(30.0, -110.0, 25.0);
	/** The desk's top's middle (the barge's frame): where the manifest lies. */
	AI_LOOTER_SHOOTER_API FVector DeskTop();
	/** The soul furnace in the middle: 6 m across, 6 m tall; its cage's window faces the hall's +X (toward the office). */
	inline constexpr float FurnaceRadius = 300.f;
	inline constexpr float FurnaceHeight = 600.f;
	inline constexpr float CageUp = 250.f;
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

	/** True when a round hall of Radius fits the hull's beam with a hull wall's thickness all round. */
	AI_LOOTER_SHOOTER_API bool HallFits(float Radius);

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
	/** Aboard: inside the deck's rails from the hall's floor to well over the deck (the barge's frame). */
	AI_LOOTER_SHOOTER_API bool IsAboard(const FVector& BargeLocal);
}
