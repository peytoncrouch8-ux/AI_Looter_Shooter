#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

/**
 * The boarding stage's stand-in looks, one path each (Main 6's barge kit: ABargeBoarding, ABargeFurnace, AFurnaceDraftValve,
 * ABargeHoseDrum, the scenes): the engine's shapes in the barge's own iron and glows, the props it's dressed with, and the
 * PROPS lane's kit models by path, used in place of the shapes once they're imported. Find is for constructors only
 * (ConstructorHelpers); Load for play.
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
	/** The kit's own iron (MATERIAL lane, Tools/Unreal/build_world_materials.py): M_World's detail layer in world metres with
	 *  riveted plates, so a block stretched to a wall or a floor still reads as plate, not a stretched engine cube. Rooms and
	 *  walls on KitIron, floors, stairs, grilles and fittings on KitIronDark; each falls back to Iron / IronBlack. */
	inline constexpr const TCHAR* KitIron = TEXT("/Game/Art/Materials/MI_BargeKitIron.MI_BargeKitIron");
	inline constexpr const TCHAR* KitIronDark = TEXT("/Game/Art/Materials/MI_BargeKitIronDark.MI_BargeKitIronDark");

	// The props the kit dresses her with (REUSE lane's picks, 2026-11-04), each laid at its own size from its bounds; the
	// kit's block stands in where one is missing. Deck cover:
	inline constexpr const TCHAR* Barrel = TEXT("/Game/Art/Props/SM_Barrel_B.SM_Barrel_B");                       // 0.88 m steel drum
	inline constexpr const TCHAR* Crate = TEXT("/Game/Art/Props/SM_Crate_B.SM_Crate_B");                          // 1.0 m plank crate
	inline constexpr const TCHAR* Sandbags = TEXT("/Game/Art/Tripo/SM_CompanySandbags.SM_CompanySandbags");       // 2.0 m wall
	inline constexpr const TCHAR* HoseCart = TEXT("/Game/Art/Tripo/SM_CompanyHoseCart.SM_CompanyHoseCart");       // 2.2 m pump cart
	inline constexpr const TCHAR* Winch = TEXT("/Game/Art/Tripo/SM_AnchorWinch.SM_AnchorWinch");                  // 1.2 m winch
	// Below deck:
	inline constexpr const TCHAR* Safe = TEXT("/Game/Art/Loot/SM_Strongbox.SM_Strongbox");                        // 0.82 m safe
	inline constexpr const TCHAR* Desk = TEXT("/Game/Art/Props/SM_GunsmithBench.SM_GunsmithBench");              // 0.95 m top
	inline constexpr const TCHAR* CoalSack = TEXT("/Game/Art/Props/SM_CoalSack.SM_CoalSack");
	inline constexpr const TCHAR* HallLantern = TEXT("/Game/Art/Mine/SM_RR_Mine_Lantern.SM_RR_Mine_Lantern");
	inline constexpr const TCHAR* OfficeLantern = TEXT("/Game/Art/Props/SM_PorchLantern.SM_PorchLantern");

	// The user's Tripo pieces in the game.
	inline constexpr const TCHAR* TripoSoulCoal = TEXT("/Game/Art/Tripo/SM_SoulCoal.SM_SoulCoal");                // [31]
	inline constexpr const TCHAR* TripoHose = TEXT("/Game/Art/Tripo/SM_DredgeHose.SM_DredgeHose");                // [13]

	// The PROPS lane's kit (Art/Models/Landfall/BargeKit.py; Docs/Work/2026-11-04-Art/BARGE_PROPS.md): each piece made to
	// the rules' numbers, its pivot at its base (no centre offsets), its slots by name. The engine shape stands in until
	// each is imported.
	inline constexpr const TCHAR* Furnace = TEXT("/Game/Art/Landfall/SM_BargeFurnace.SM_BargeFurnace");
	inline constexpr const TCHAR* FurnaceBurst = TEXT("/Game/Art/Landfall/SM_BargeFurnaceBurst.SM_BargeFurnaceBurst");
	inline constexpr const TCHAR* CageShard = TEXT("/Game/Art/Landfall/SM_BargeCageShard.SM_BargeCageShard");
	inline constexpr const TCHAR* FurnaceGrate = TEXT("/Game/Art/Landfall/SM_BargeFurnaceGrate.SM_BargeFurnaceGrate");
	inline constexpr const TCHAR* DraftValve = TEXT("/Game/Art/Landfall/SM_BargeDraftValve.SM_BargeDraftValve");
	inline constexpr const TCHAR* ValveWheel = TEXT("/Game/Art/Landfall/SM_BargeValveWheel.SM_BargeValveWheel");
	inline constexpr const TCHAR* Pipe = TEXT("/Game/Art/Landfall/SM_BargePipe.SM_BargePipe");
	inline constexpr const TCHAR* DeckVent = TEXT("/Game/Art/Landfall/SM_BargeDeckVent.SM_BargeDeckVent");
	inline constexpr const TCHAR* FurnaceVent = TEXT("/Game/Art/Landfall/SM_BargeFurnaceVent.SM_BargeFurnaceVent");
	inline constexpr const TCHAR* FireTongue = TEXT("/Game/Art/Landfall/SM_BargeFireTongue.SM_BargeFireTongue");
	inline constexpr const TCHAR* Stack = TEXT("/Game/Art/Landfall/SM_BargeStack.SM_BargeStack");
	inline constexpr const TCHAR* StackFoot = TEXT("/Game/Art/Landfall/SM_BargeStackFoot.SM_BargeStackFoot");
	inline constexpr const TCHAR* HoseDrum = TEXT("/Game/Art/Landfall/SM_BargeHoseDrum.SM_BargeHoseDrum");
	inline constexpr const TCHAR* DrumGantry = TEXT("/Game/Art/Landfall/SM_BargeDrumGantry.SM_BargeDrumGantry");
	inline constexpr const TCHAR* DrumLever = TEXT("/Game/Art/Landfall/SM_BargeDrumLever.SM_BargeDrumLever");
	inline constexpr const TCHAR* Radio = TEXT("/Game/Art/Landfall/SM_BargeRadio.SM_BargeRadio");

	/** The kit's glow slot (additive CinderGlow; never slot 0 on these meshes): cold, it's overridden with IronBlack; hot,
	 *  left as the model has it. */
	inline const FName GlowSlot(TEXT("CinderGlow"));
	/** The hose drum model's hose slot (set to her hanging hose's rubber, so the two match). */
	inline const FName HoseSlot(TEXT("HoseRubber"));

	/** The sizes the kit's models were built at (BARGE_PROPS.md), which the code scales to the rules' numbers: the furnace's
	 *  drum radius and crown height; the stack's tube radius and height; the gantry's half size; a grate wedge's depth and
	 *  its middle radius (at 12 wedges); a pipe section's diameter and length; the fire tongue's radius and height; the deck
	 *  vent's half size and its fire's foot over its middle; the hall's vent hood's half diagonal; the drum's foot over the
	 *  gantry's floor. */
	inline constexpr float FurnaceModelRadius = 250.f;
	inline constexpr float FurnaceModelHeight = 600.f;
	inline constexpr float StackModelRadius = 150.f;
	inline constexpr float StackModelHeight = 1400.f;
	inline const FVector2D GantryModelHalf(200.0, 130.0);
	inline constexpr float GrateModelDepth = 180.f;
	inline constexpr float GrateModelMiddle = 430.f;
	inline constexpr int32 GrateModelCount = 12;
	inline constexpr float PipeModelWidth = 50.f;
	inline constexpr float PipeModelLength = 300.f;
	inline constexpr float FireModelRadius = 50.f;
	inline constexpr float FireModelHeight = 100.f;
	inline constexpr float DeckVentModelHalf = 100.f;
	inline constexpr float DeckVentFireUp = 8.f;
	inline constexpr float HoodHalfDiagonal = 106.f;
	inline constexpr float DrumOnGantry = 50.f;

	/** Sets Part's named slot to Look (Look null: back to the mesh's own); a slot the mesh hasn't got is left alone. */
	inline void SetSlot(UPrimitiveComponent& Part, FName Slot, UMaterialInterface* Look)
	{
		const int32 Index = Part.GetMaterialIndex(Slot);
		if (Index != INDEX_NONE)
		{
			Part.SetMaterial(Index, Look);
		}
	}

	/** A fire's transform with its foot at Foot, Scale its size in tongue units (FireModelRadius x FireModelHeight): the
	 *  tongue stands on its foot; the engine cone (bTongue false) is centred, so it's raised half its height. */
	inline FTransform FireAt(const FVector& Foot, const FVector& Scale, bool bTongue)
	{
		return FTransform(FQuat::Identity, bTongue ? Foot : Foot + FVector(0.0, 0.0, Scale.Z * FireModelHeight * 0.5), Scale);
	}

	/** A run of pipe from Foot straight up Length long as model sections (about one a section's length, each stretched to
	 *  fit), Width across. */
	inline void AddPipeRun(TArray<FTransform>& Out, const FVector& Foot, float Length, float Width)
	{
		const int32 Sections = FMath::Max(1, FMath::RoundToInt32(Length / PipeModelLength));
		const double Each = Length / Sections;
		for (int32 Index = 0; Index < Sections; ++Index)
		{
			Out.Add(FTransform(FQuat::Identity, Foot + FVector(0.0, 0.0, Each * Index),
				FVector(Width / PipeModelWidth, Width / PipeModelWidth, Each / PipeModelLength)));
		}
	}

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

	/** The asset at ObjectPath, else the one at Fallback (a constructor's). */
	template <typename T>
	T* FindOr(const TCHAR* ObjectPath, const TCHAR* Fallback)
	{
		T* Found = Find<T>(ObjectPath);
		return Found ? Found : Find<T>(Fallback);
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
