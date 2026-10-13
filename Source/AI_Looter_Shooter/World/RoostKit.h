#pragma once

// What Hob's roost's pieces share (ARespawnMarker's RespawnMarker*.cpp, URoostBeaconComponent, ARoostCrows): the models and
// materials they look for, the model's sockets, and the stand-in's (its greybox, until the art's SM_HobRoost is imported).
// Not for anyone else.

#include "CoreMinimal.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace RoostKit
{
	/**
	 * The roost, the user's pick (2026-10-13, look sheet 41, design B): the Crow's Mast, a mast with its yard cockbilled,
	 * crosstrees, a stork's nest round the topmast and a fire basket on top, on a cairn, the crow's-foot mark burnt in a board
	 * at eye height (the map's icon). Its pivot at the mast's foot; a slim hull on the mast and a low one over the cairn.
	 * Sockets (Art/README.md: SOCKET_<Name>, imported without the prefix): Beacon (the fire basket's middle), Bell (where the
	 * bell hangs), Perch_1 to Perch_6 (the crows' feet, facing out), Wake (on the ground, where the player stands on waking,
	 * facing away from the mast) and Speaker (Hob's line).
	 */
	inline const TCHAR* const RoostPath = TEXT("/Game/Art/Props/SM_HobRoost.SM_HobRoost");

	/** Its bell, a piece of its own so it swings when rung: hung on the Bell socket, its pivot the top of its loop. */
	inline const TCHAR* const BellPath = TEXT("/Game/Art/Props/SM_HobRoostBell.SM_HobRoostBell");

	/** The stand-in's pieces: the engine's cube in the lantern post's weathered wood and iron, on the stone wall's stone. */
	inline const TCHAR* const CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	inline const TCHAR* const WoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");
	inline const TCHAR* const IronPath = TEXT("/Game/Art/Materials/MI_LanternIron.MI_LanternIron");
	inline const TCHAR* const StonePath = TEXT("/Game/Art/Materials/MI_StoneWall.MI_StoneWall");

	/** The beacon: the gunfire's additive glow on the engine's plane (R, G, B, strength, shape: 0 a streak, 1 round). */
	inline const TCHAR* const PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	inline const TCHAR* const GlowPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	inline constexpr int32 GlowFloats = 5;
	inline constexpr int32 StrengthFloat = 3;
	inline constexpr float StreakShape = 0.f;
	inline constexpr float RoundShape = 1.f;

	/** Hob's crows: the ambient fauna's crow (Art/Models/Creatures/AmbientFauna.py), as build_area_fauna.py gives its flocks. */
	inline const TCHAR* const CrowPerchedPath = TEXT("/Game/Art/Creatures/SM_Crow_Perched.SM_Crow_Perched");
	inline const TCHAR* const CrowHeadPath = TEXT("/Game/Art/Creatures/SM_Crow_Head.SM_Crow_Head");
	inline const TCHAR* const CrowFlyingPath = TEXT("/Game/Art/Creatures/SM_Crow_Flying.SM_Crow_Flying");
	inline const TCHAR* const CrowWingLPath = TEXT("/Game/Art/Creatures/SM_Crow_WingInnerL.SM_Crow_WingInnerL");
	inline const TCHAR* const CrowWingRPath = TEXT("/Game/Art/Creatures/SM_Crow_WingInnerR.SM_Crow_WingInnerR");
	inline const TCHAR* const CrowOuterLPath = TEXT("/Game/Art/Creatures/SM_Crow_WingOuterL.SM_Crow_WingOuterL");
	inline const TCHAR* const CrowOuterRPath = TEXT("/Game/Art/Creatures/SM_Crow_WingOuterR.SM_Crow_WingOuterR");

	// --- The model's sockets ---

	inline FName BeaconSocket() { return FName(TEXT("Beacon")); }
	inline FName BellSocket() { return FName(TEXT("Bell")); }
	inline FName WakeSocket() { return FName(TEXT("Wake")); }
	inline FName SpeakerSocket() { return FName(TEXT("Speaker")); }

	/** The crows' perches, Perch_1 to Perch_6. */
	inline constexpr int32 PerchCount = 6;
	inline FName PerchSocket(int32 Index) { return FName(*FString::Printf(TEXT("Perch_%d"), Index + 1)); }

	// --- The stand-in's sockets: the model's own (cm, its frame: the mast's foot, +X its front, +Z up), so the greybox
	//     stands, wakes and perches as the model will ---

	/**
	 * Where the player wakes: 1.7 m out in front of the mast on the ground, facing away from it, out over the land. The
	 * build script keeps the same numbers for a level built before the model is in (build_area_roosts.py's FALLBACK_WAKE).
	 */
	inline FTransform FallbackWake()
	{
		return FTransform(FRotator::ZeroRotator, FVector(170.0, 0.0, 0.0));
	}
	inline FVector FallbackBeacon() { return FVector(-21.0, -5.0, 476.0); }
	inline FVector FallbackBell() { return FVector(5.0, -82.0, 208.0); }
	inline FVector FallbackSpeaker() { return FVector(38.0, -4.0, 364.0); }

	/** A perch of the stand-in, the model's, facing out from the mast. */
	inline FTransform FallbackPerch(int32 Index)
	{
		static const FVector Spots[PerchCount] = { FVector(25.0, 65.0, 271.0), FVector(8.0, -55.0, 235.0), FVector(-123.0, 20.0, 320.0),
			FVector(88.0, -3.0, 320.0), FVector(10.0, -55.0, 363.0), FVector(20.0, -23.0, 413.0) };
		const FVector& Spot = Spots[FMath::Clamp(Index, 0, PerchCount - 1)];
		return FTransform(FRotator(0.0, FMath::RadiansToDegrees(FMath::Atan2(Spot.Y, Spot.X)), 0.0), Spot);
	}

	/** One box of the stand-in: its middle and size (cm), in the roost's frame. */
	struct FGreyboxPart
	{
		FVector Center;
		FVector Size;
	};

	/** The stand-in's weathered wood: the mast, the yard, the crosstrees, the nest, the topmast, the bell's arm. */
	inline TArray<FGreyboxPart> GreyboxWood()
	{
		return { { FVector(0.0, 0.0, 225.0), FVector(24.0, 24.0, 450.0) }, { FVector(16.0, 5.0, 253.0), FVector(14.0, 160.0, 14.0) },
			{ FVector(-17.0, 8.0, 314.0), FVector(230.0, 14.0, 12.0) }, { FVector(5.0, -15.0, 395.0), FVector(120.0, 120.0, 30.0) },
			{ FVector(-10.0, -5.0, 438.0), FVector(12.0, 12.0, 60.0) }, { FVector(2.0, -41.0, 212.0), FVector(10.0, 82.0, 8.0) } };
	}

	/** Its iron: the fire basket on the topmast. (The bell is a piece of its own: the stand-in hangs a box for it.) */
	inline TArray<FGreyboxPart> GreyboxIron()
	{
		return { { FVector(-21.0, -5.0, 476.0), FVector(44.0, 44.0, 40.0) } };
	}

	/** Its stone: the cairn the mast is set in. */
	inline TArray<FGreyboxPart> GreyboxStone()
	{
		return { { FVector(0.0, 0.0, 25.0), FVector(130.0, 130.0, 50.0) } };
	}

	/** The stand-in's bell: a box this big, hung its height under the Bell socket (the cube's pivot is its middle). */
	inline FVector StandInBellSize() { return FVector(28.0, 28.0, 34.0); }

	/** An asset found only once it's in this checkout, so the classes work, and their tests run, without it. In a constructor. */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}
}
