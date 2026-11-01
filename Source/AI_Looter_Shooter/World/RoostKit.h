#pragma once

// What the save points' pieces share (ARespawnMarker's RespawnMarker*.cpp, URoostBeaconComponent, ARoostCrows): the models and
// materials they look for, the model's sockets, and the stand-in's (its greybox, while no post model is in this checkout).
// The Wardens' lantern posts since 2026-10-31 (Hob's roosts before; the code keeps the roost's and the grave's names).
// Not for anyone else.

#include "CoreMinimal.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

namespace RoostKit
{
	/**
	 * The Wardens' lantern post (the user's call, 2026-10-31: "lantern posts instead of graves"; the Wardens light them as
	 * they explore). THE ONE PATH for its model: a STAND-IN until the user's Tripo "Lantern post" is imported, Art/Models/
	 * Props/BurialDeck.py's KeeperLanternPost (a 3.2 m post on a cross foot with a braced arm toward +X and an iron lantern
	 * hung from its end, LanternGlow glass, SOCKET_Light at the flame; hulls on the post and the foot). Its pivot at the
	 * post's foot. The Tripo model may carry the roost's sockets (below: Beacon at the lamp, Bell, Perch_1 to Perch_6, Wake,
	 * Speaker; else the stand-in's numbers stand) and should keep the arm toward +X, the glass slot LanternGlow and a Light
	 * socket. Swap it here (and nowhere else: build_area_roosts.py reads the class's default).
	 */
	// The user's Tripo lantern post since lane IMPORT (lanes/IMPORT.md: every socket above, LanternGlow glass); the
	// stand-in was /Game/Art/Props/SM_KeeperLanternPost.SM_KeeperLanternPost.
	inline const TCHAR* const PostPath = TEXT("/Game/Art/Tripo/SM_LanternPost.SM_LanternPost");

	/**
	 * The Crow's Mast the posts replaced (the user's pick, 2026-10-13, look sheet 41): SM_HobRoost. A level saved with it on
	 * a roost (built before 2026-10-31) shows the post instead (ARespawnMarker::FitRoost), so no rebuild is needed for it.
	 */
	inline const TCHAR* const RoostPath = TEXT("/Game/Art/Props/SM_HobRoost.SM_HobRoost");

	/** Its bell, a piece of its own so it swings when rung: hung on the Bell socket (the post's arm), its pivot the top of its loop. */
	inline const TCHAR* const BellPath = TEXT("/Game/Art/Props/SM_HobRoostBell.SM_HobRoostBell");

	/**
	 * The post's lamp dark until it's lit: its glass slot (BurialDeck.py's LanternGlow) shows the house trim's window glass,
	 * as the keeper's lantern posts' do when Abel puts them out (AKeeperLanternPost; their glass UVs sit on that glass).
	 */
	inline const TCHAR* const DarkGlassPath = TEXT("/Game/Art/Materials/MI_HouseTrim.MI_HouseTrim");
	inline FName GlassSlot() { return FName(TEXT("LanternGlow")); }

	/** The post model's flame (BurialDeck.py's SOCKET_Light): the beacon and its light stand there when it has no Beacon socket. */
	inline FName LightSocket() { return FName(TEXT("Light")); }

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

	// --- The post's sockets where the model has none: the stand-in's own numbers (cm, its frame: the post's foot, +X the
	//     arm and its lantern, +Z up; BurialDeck.py's KeeperLanternPost: 3.2 m tall, the arm's top at 3.13 m reaching 0.95 m,
	//     the lantern's glass at (90.5, 0, 236) as AKeeperLanternPost's FallbackGlass), so the post and its greybox stand,
	//     wake, ring and perch alike ---

	/**
	 * Where the player wakes: 1.7 m out in front of the post on the ground (past the lantern), facing away from it, out over
	 * the land. The build script keeps the same numbers (build_area_roosts.py's FALLBACK_WAKE: change both together).
	 */
	inline FTransform FallbackWake()
	{
		return FTransform(FRotator::ZeroRotator, FVector(170.0, 0.0, 0.0));
	}
	/** The beacon at the lamp: the lantern's glass, where its light burns and its column rises from. */
	inline FVector FallbackBeacon() { return FVector(90.5, 0.0, 236.0); }
	/** The bell hung under the arm between the brace's top and the lantern (its loop's top at the arm's underside). */
	inline FVector FallbackBell() { return FVector(64.0, 0.0, 302.0); }
	/** Hob's word from the post's top. */
	inline FVector FallbackSpeaker() { return FVector(0.0, 0.0, 330.0); }

	/** A perch of the stand-in: on the post's top, along the arm, on the lantern's cap and the brace, looking out or across. */
	inline FTransform FallbackPerch(int32 Index)
	{
		struct FPerch
		{
			FVector Spot;
			double Yaw;
		};
		static const FPerch Perches[PerchCount] = { { FVector(0.0, 0.0, 322.0), 0.0 }, { FVector(24.0, 0.0, 316.0), 90.0 },
			{ FVector(46.0, 0.0, 316.0), -90.0 }, { FVector(80.0, 0.0, 316.0), 90.0 }, { FVector(90.5, 0.0, 284.0), 0.0 },
			{ FVector(29.0, 0.0, 276.0), -90.0 } };
		const FPerch& Perch = Perches[FMath::Clamp(Index, 0, PerchCount - 1)];
		return FTransform(FRotator(0.0, Perch.Yaw, 0.0), Perch.Spot);
	}

	/** One box of the stand-in: its middle and size (cm), in the roost's frame. */
	struct FGreyboxPart
	{
		FVector Center;
		FVector Size;
	};

	/** The stand-in post's weathered wood: the post and its arm. */
	inline TArray<FGreyboxPart> GreyboxWood()
	{
		return { { FVector(0.0, 0.0, 160.0), FVector(18.0, 18.0, 320.0) }, { FVector(43.0, 0.0, 308.0), FVector(104.0, 13.0, 10.0) } };
	}

	/** Its iron: the lantern under the arm's end. (The bell is a piece of its own: the stand-in hangs a box for it.) */
	inline TArray<FGreyboxPart> GreyboxIron()
	{
		return { { FVector(90.5, 0.0, 240.0), FVector(26.0, 26.0, 40.0) } };
	}

	/** Its stone: the footing the post stands in. */
	inline TArray<FGreyboxPart> GreyboxStone()
	{
		return { { FVector(0.0, 0.0, 8.0), FVector(90.0, 90.0, 16.0) } };
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
