#pragma once

#include "CoreMinimal.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

/**
 * The boarding stage's stand-in looks, one path each (Main 6's barge kit: ABargeBoarding, ABargeFurnace, AFurnaceDraftValve,
 * ABargeHoseDrum, the scenes): the engine's shapes in the barge's own iron and glows, and the user's Tripo kit pieces
 * (TRIPO_MODELS.md "Mains 6-7", 30-34 and 37) by soft path, used in place of the shapes once they're imported. Find is
 * for constructors only (ConstructorHelpers); Load for play.
 */
namespace BargeLooks
{
	inline constexpr const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	inline constexpr const TCHAR* Cylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	inline constexpr const TCHAR* Sphere = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	inline constexpr const TCHAR* Cone = TEXT("/Engine/BasicShapes/Cone.Cone");
	inline constexpr const TCHAR* Iron = TEXT("/Game/Art/Landfall/Materials/MI_Landfall_BargeIron.MI_Landfall_BargeIron");
	inline constexpr const TCHAR* IronBlack = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");
	inline constexpr const TCHAR* Brass = TEXT("/Game/Art/Materials/MI_BrassWorn.MI_BrassWorn");
	inline constexpr const TCHAR* Glow = TEXT("/Game/Art/Materials/MI_CinderGlow.MI_CinderGlow");
	inline constexpr const TCHAR* Soul = TEXT("/Game/Art/Landfall/Materials/MI_Landfall_AbelTrail.MI_Landfall_AbelTrail");
	inline constexpr const TCHAR* Smog = TEXT("/Game/Art/Landfall/Materials/MI_Landfall_SoulSmog.MI_Landfall_SoulSmog");
	inline constexpr const TCHAR* Rubber = TEXT("/Game/Art/Landfall/Materials/MI_Landfall_HoseRubber.MI_Landfall_HoseRubber");
	inline constexpr const TCHAR* Wood = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	// The user's Tripo kit (soft: the shapes stand in until each lands).
	inline constexpr const TCHAR* TripoSoulCoal = TEXT("/Game/Art/Tripo/SM_SoulCoal.SM_SoulCoal");                // [31]
	inline constexpr const TCHAR* TripoStack = TEXT("/Game/Art/Tripo/SM_BargeStack.SM_BargeStack");               // [32]
	inline constexpr const TCHAR* TripoHoseDrum = TEXT("/Game/Art/Tripo/SM_BargeHoseDrum.SM_BargeHoseDrum");      // [32]
	inline constexpr const TCHAR* TripoVent = TEXT("/Game/Art/Tripo/SM_FurnaceVent.SM_FurnaceVent");              // [32]
	inline constexpr const TCHAR* TripoFurnace = TEXT("/Game/Art/Tripo/SM_SoulFurnace.SM_SoulFurnace");           // [33]
	inline constexpr const TCHAR* TripoFurnaceBurst = TEXT("/Game/Art/Tripo/SM_SoulFurnaceBurst.SM_SoulFurnaceBurst"); // [33]
	inline constexpr const TCHAR* TripoGrate = TEXT("/Game/Art/Tripo/SM_FurnaceGrate.SM_FurnaceGrate");           // [34]
	inline constexpr const TCHAR* TripoValve = TEXT("/Game/Art/Tripo/SM_DraftValve.SM_DraftValve");               // [34]
	inline constexpr const TCHAR* TripoHose = TEXT("/Game/Art/Tripo/SM_DredgeHose.SM_DredgeHose");                // [13]

	/** The asset at ObjectPath if it's in the project (a constructor's: ConstructorHelpers), else null. */
	template <typename T>
	T* Find(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	/** The same in play (a scene's props, a late look). */
	template <typename T>
	T* Load(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		return LoadObject<T>(nullptr, ObjectPath);
	}
}
