#pragma once

// What ALamplighterShip's pieces share (LamplighterShipParts.cpp builds them, LamplighterShipGangway.cpp moves them): her
// models' paths, her deck's pieces and blocks fitted to the Tripo hull, her rail line and its tags, where her pieces stand,
// her crew's stations, and how each kind of box collides. Not for anyone else.

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Story/WardenCrewMember.h"

namespace LamplighterShipKit
{
	// The user's Tripo models (TRIPO_MODELS.md 10, 11; lanes/IMPORT.md). Stand-ins were SM_Skiff_A_Packet and SM_KeepersLantern.
	inline const TCHAR* HullMeshPath = TEXT("/Game/Art/Tripo/SM_Lamplighter.SM_Lamplighter");
	inline const TCHAR* LanternMeshPath = TEXT("/Game/Art/Tripo/SM_GreatLantern.SM_GreatLantern");
	inline const TCHAR* GangwayMeshPath = TEXT("/Game/Art/Vehicles/SM_Skiff_Gangplank.SM_Skiff_Gangplank");
	inline const TCHAR* EngineMeshPath = TEXT("/Game/Art/Props/SM_Barrel_A.SM_Barrel_A");
	inline const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	inline const TCHAR* IronPath = TEXT("/Game/Art/Materials/MI_LanternIron.MI_LanternIron");
	inline const TCHAR* WoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	// --- Her deck, in her frame (cm; the keel's middle at 0, the bow along +X) ---
	// Measured on the Tripo hull (Docs/Work/2026-11-04-Art/COLLISION.md: rays down every 10 cm over her deck): the main deck
	// is crowned, 10 cm over DeckTop on her centreline falling 4.6 degrees to -18 at the bulwarks; aft of x -250 it rises
	// to +50 at the wheel, the crown flattening; the forecastle is 1 m up, rising 6 degrees to the Lantern, its aft edge an
	// arc (x 1050 on the centreline, 960 at the sides) with a notch on the centreline down to the main deck.

	/** A floor piece: its rectangle in plan (her frame) and its top, the plane z = DeckTop + Rise + SlopeX x + SlopeY y; the
	 *  box reaches Depth under its top. Each was fitted to the hull's deck (within 3 cm on 86-100% of the walk area). */
	struct FDeckPiece
	{
		const TCHAR* Name;
		float X0, X1, Y0, Y1;
		float Rise, SlopeX, SlopeY;
		float Depth;
	};
	/** The pieces overlap their neighbours by this much in plan, so no seam opens between two. */
	inline constexpr float PieceOverlap = 5.f;
	/** Deck (the crown strip, the floor the player rides), SternDeck (at the wheel) and ProwDeck (the forecastle, 1 m up: a
	 *  solid block from the main deck, not climbed; Finch stands on it) are the named ones; the forecastle's arc is stepped
	 *  round in four more. */
	inline const FDeckPiece DeckPieces[] = {
		{ TEXT("Deck"), -250.f, 820.f, -40.f, 40.f, 10.51f, -0.00270f, 0.f, 30.f },
		{ TEXT("SternDeck"), -1163.f, -1000.f, -250.f, 250.f, -40.56f, -0.08093f, 0.01020f, 40.f },
		{ TEXT("ProwDeck"), 1050.f, 1300.f, -240.f, 240.f, -11.37f, 0.10849f, 0.f, 125.f },
		{ TEXT("DeckMainPort"), -250.f, 820.f, -360.f, -40.f, 15.52f, -0.00672f, 0.08032f, 30.f },
		{ TEXT("DeckMainStarboard"), -250.f, 820.f, 40.f, 360.f, 16.19f, -0.00752f, -0.08269f, 30.f },
		{ TEXT("DeckFore"), 820.f, 1060.f, -40.f, 40.f, 2.05f, 0.00792f, 0.f, 30.f },
		{ TEXT("DeckForePort"), 820.f, 1060.f, -340.f, -40.f, 5.34f, 0.00671f, 0.05729f, 30.f },
		{ TEXT("DeckForeStarboard"), 820.f, 1060.f, 40.f, 340.f, 4.40f, 0.00837f, -0.06084f, 30.f },
		{ TEXT("ProwArcPort"), 1000.f, 1050.f, -260.f, -140.f, -11.37f, 0.10849f, 0.f, 115.f },
		{ TEXT("ProwArcStarboard"), 1000.f, 1050.f, 140.f, 260.f, -11.37f, 0.10849f, 0.f, 115.f },
		{ TEXT("ProwArcPortAft"), 960.f, 1000.f, -260.f, -200.f, -11.37f, 0.10849f, 0.f, 110.f },
		{ TEXT("ProwArcStarboardAft"), 960.f, 1000.f, 200.f, 260.f, -11.37f, 0.10849f, 0.f, 110.f },
		{ TEXT("DeckAftPort"), -500.f, -250.f, -360.f, 0.f, 8.61f, -0.02372f, 0.05934f, 30.f },
		{ TEXT("DeckAftStarboard"), -500.f, -250.f, 0.f, 360.f, 9.59f, -0.02327f, -0.06401f, 30.f },
		{ TEXT("DeckWaistPort"), -750.f, -500.f, -340.f, 0.f, -0.09f, -0.03737f, 0.04705f, 30.f },
		{ TEXT("DeckWaistStarboard"), -750.f, -500.f, 0.f, 340.f, -1.06f, -0.04021f, -0.04982f, 30.f },
		{ TEXT("DeckQuarterPort"), -1000.f, -750.f, -300.f, 0.f, -29.92f, -0.07150f, 0.01835f, 30.f },
		{ TEXT("DeckQuarterStarboard"), -1000.f, -750.f, 0.f, 300.f, -32.39f, -0.07451f, -0.02051f, 30.f },
	};

	/** What stands on her deck, solid to walk round and to shots (the hull itself has no collision): the console and its
	 *  engine, the starboard locker, the port chest, the wheel house aft. Plan rectangle and top over DeckTop (cm). */
	struct FDeckBlock
	{
		const TCHAR* Name;
		float X0, X1, Y0, Y1, Top;
	};
	inline const FDeckBlock DeckBlocks[] = {
		{ TEXT("Console"), -240.f, 205.f, -132.f, 137.f, 135.f },
		{ TEXT("ConsoleEngine"), -185.f, 25.f, -115.f, 45.f, 300.f },
		// The starboard locker bench, whole: Main 6's plank comes aboard fore of it (PlankHingeX).
		{ TEXT("Locker"), -175.f, 215.f, 288.f, 380.f, 112.f },
		{ TEXT("Chest"), -525.f, -215.f, -281.f, -227.f, 65.f },
		{ TEXT("WheelHouse"), -1055.f, -870.f, 5.f, 180.f, 203.f },
	};

	/** The rail walls: this tall above the deck (more than a jump), this thick. */
	inline constexpr float WallHeight = 320.f;
	inline constexpr float WallHalfThick = 15.f;
	/** The side rails in three lengths each (aft, the gangway's middle, fore), from X to X; the gangway's opens. */
	inline const FVector2D SideSpans[] = { FVector2D(-1163.0, -250.0), FVector2D(-250.0, 150.0), FVector2D(150.0, 1055.0) };
	/** Her bulwark's inner face, out from her centreline (both sides alike), measured 50 cm over the deck: each side's rail
	 *  is a wall along this line, so the player walks to the bulwark she sees (no further: the bow and stern curve in). */
	inline const FVector2D BulwarkLine[] = {
		FVector2D(-1163.0, 215.0), FVector2D(-1100.0, 227.0), FVector2D(-1000.0, 255.0), FVector2D(-900.0, 280.0),
		FVector2D(-800.0, 301.0), FVector2D(-700.0, 321.0), FVector2D(-600.0, 337.0), FVector2D(-500.0, 354.0),
		FVector2D(-400.0, 366.0), FVector2D(-250.0, 379.0), FVector2D(-150.0, 386.0), FVector2D(0.0, 392.0),
		FVector2D(150.0, 388.0), FVector2D(300.0, 381.0), FVector2D(450.0, 367.0), FVector2D(550.0, 352.0),
		FVector2D(650.0, 335.0), FVector2D(750.0, 312.0), FVector2D(850.0, 286.0), FVector2D(950.0, 233.0),
		FVector2D(1000.0, 184.0) };
	/** A side rail's tag: which side and which of SideSpans it stands in (ShowGangway opens the gangway's). */
	inline FName SideRailTag(float SideSign, int32 Span)
	{
		return FName(*FString::Printf(TEXT("SideRail_%s_%d"), SideSign < 0.f ? TEXT("Port") : TEXT("Starboard"), Span));
	}
	/** Where Main 6's plank to the barge comes aboard, her starboard rail opens this far either side of its hinge
	 *  (PlankHingeX): the plank's 55 cm half width (PilotSortieFight.cpp PlankHalfWidth), the player's capsule and a hand.
	 *  Every stretch of her bulwark line reaching into that is tagged so (BuildRails): x 150..450 round the hinge at 300. */
	inline constexpr double PlankOpenHalf = 110.0;
	inline FName PlankRailTag()
	{
		static const FName Name(TEXT("SideRail_Plank"));
		return Name;
	}

	/** Whether Wall is one of the side rails' stretches (BuildRails tags each with its side and span). */
	inline bool IsSideRail(const UBoxComponent& Wall)
	{
		for (int32 Span = 0; Span < UE_ARRAY_COUNT(SideSpans); ++Span)
		{
			if (Wall.ComponentHasTag(SideRailTag(-1.f, Span)) || Wall.ComponentHasTag(SideRailTag(1.f, Span)))
			{
				return true;
			}
		}
		return false;
	}

	/** Where the pieces stand (her frame, Z from the deck's top). */
	/** On the hull's own lantern pedestal at the prow (SM_Lamplighter's socket Lantern: 2.22 m over the main deck). */
	inline const FVector LanternSpot(1329.0, 0.0, 0.0);
	inline constexpr float LanternPostHeight = 222.f;
	/** SM_GreatLantern is imported at its 1.5 m; the flame keeps the size it had on the stand-in (a 35 cm lantern x 4.2). */
	inline constexpr float LanternScale = 1.f;
	inline constexpr float LanternFlameScale = 4.2f;
	inline constexpr float LanternCandela = 40.f;
	inline constexpr float LanternRadius = 1400.f;
	inline const FLinearColor LanternWarm(1.f, 0.66f, 0.3f);
	/** The captain's wheel on the stern deck (the hull's own big wheel stands forward of it, its hub 2.8 m up: out of reach). */
	inline const FVector WheelSpot(-1050.0, 0.0, 142.0);
	/** The engine works amidships, forward of the hull's console and clear of the bag's struts (on the crowned deck, 8 cm up). */
	inline const FVector EngineSpots[] = { FVector(450.0, -55.0, 8.0), FVector(450.0, 55.0, 8.0) };
	/** Her crown under the arrival spot is 10 cm over DeckTop. */
	inline const FVector LandingLocal(300.0, 0.0, 10.0);
	inline const FVector ProwMarkerLocal(990.0, 0.0, 90.0);
	/** On the bag's top (15 m over the deck). */
	inline const FVector HobPerchLocal(250.0, 0.0, 1512.0);
	/** The gangway plank's own length (SM_Skiff_Gangplank, 3 m) and its scale. */
	inline constexpr float PlankLength = 300.f;
	inline constexpr float GangwayScale = 1.3f;

	/** The crew's stations: Holloway at the wheel, Finch by the Great Lantern, Junie at the engine (her frame, yaw; Z the deck
	 *  measured under each: the stern deck 50 cm up at the wheel, the forecastle 120). */
	struct FStation
	{
		EWardenCrew Crew;
		FVector Spot;
		float Yaw;
	};
	inline const FStation Stations[] = {
		{ EWardenCrew::Captain, FVector(-1120.0, 0.0, 50.0), 0.f },
		{ EWardenCrew::Keeper, FVector(1210.0, 105.0, 120.0), -25.f },
		{ EWardenCrew::Engineer, FVector(450.0, -165.0, 0.0), 90.f },
	};

	/** A floor the player walks and the boarders stand on: blocks everything, world-static for the creatures' ground traces. */
	inline void MakeFloor(UBoxComponent& Box)
	{
		Box.SetMobility(EComponentMobility::Movable);
		Box.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Box.SetCollisionObjectType(ECC_WorldStatic);
		Box.SetCanEverAffectNavigation(false);
		Box.SetGenerateOverlapEvents(false);
		Box.SetHiddenInGame(true);
	}

	/** Something standing on her deck: solid to walk round and to shots, but not a floor the creatures' ground traces take. */
	inline void MakeBlock(UBoxComponent& Box)
	{
		Box.SetMobility(EComponentMobility::Movable);
		Box.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Box.SetCollisionObjectType(ECC_WorldDynamic);
		Box.SetCanEverAffectNavigation(false);
		Box.SetGenerateOverlapEvents(false);
		Box.SetHiddenInGame(true);
	}

	/** A rail wall: stops pawns only, so shots, the camera and the creatures' ground traces pass it. */
	inline void MakeWall(UBoxComponent& Box)
	{
		Box.SetMobility(EComponentMobility::Movable);
		Box.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box.SetCollisionObjectType(ECC_WorldDynamic);
		Box.SetCollisionResponseToAllChannels(ECR_Ignore);
		Box.SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Box.SetCanEverAffectNavigation(false);
		Box.SetGenerateOverlapEvents(false);
		Box.SetHiddenInGame(true);
	}

	/** A deck piece's part: no collision of its own (the deck and the walls are hers). */
	inline void MakeDressing(UStaticMeshComponent& Mesh)
	{
		Mesh.SetMobility(EComponentMobility::Movable);
		Mesh.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh.SetCanEverAffectNavigation(false);
	}
}
