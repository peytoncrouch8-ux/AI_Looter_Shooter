#pragma once

#include "CoreMinimal.h"

class AActor;
class UClass;
class UDecalComponent;
class UMaterialInterface;
class UObject;
class UStaticMesh;

/**
 * Every asset the Gravemother's Barrow fight loads by path, in one place, and how the fight runs before they exist. The art
 * (Art/Models/Creatures/Gravemother.py and GravemotherStone.py, Art/Models/Props/GraveFight.py,
 * Art/Textures/GraveFight + Tools/Unreal/build_grave_fight_materials.py) lands after this code: until an asset is
 * imported, Load gives null and each piece of the fight shows its stand-in (the engine's shapes in a flat colour on
 * M_StylizedSurface, the weapons' code-drawn dust and chips, AGroundCrack's cracks), so the whole fight plays on SK_Spider.
 * /Game/Art/Effects is always cooked for these (Config/DefaultGame.ini); Creatures and Props already were.
 */
namespace GravemotherAssets
{
	/** Her own body: Spider.py's mesh and rig in her mud-grey hide (constructor-time: FindIfMade). */
	inline constexpr const TCHAR* Body = TEXT("/Game/Art/Creatures/SK_Gravemother.SK_Gravemother");
	/** Her abdomen's slot on that body, and its look once the stone is off (all pale bone: the stone's print bare). */
	inline const FName AbdomenSlot(TEXT("GravemotherAbdomen"));
	inline constexpr const TCHAR* AbdomenBare = TEXT("/Game/Art/Materials/MI_GravemotherBare.MI_GravemotherBare");

	/** The headstone by state (0 intact, 1 cracked, 2 shattered), the pieces it breaks into (1-5), the chips shots knock off (1-3). */
	AI_LOOTER_SHOOTER_API const TCHAR* StonePath(int32 State);
	AI_LOOTER_SHOOTER_API const TCHAR* ShardPath(int32 Index);
	AI_LOOTER_SHOOTER_API const TCHAR* ChipPath(int32 Index);

	/** The fight's earth: clods (S, M, L), the rubble pile, a berm segment, a mound and its crater, debris (1-5). */
	inline constexpr const TCHAR* ClodSmall = TEXT("/Game/Art/Props/SM_GraveClod_S.SM_GraveClod_S");
	inline constexpr const TCHAR* ClodMedium = TEXT("/Game/Art/Props/SM_GraveClod_M.SM_GraveClod_M");
	inline constexpr const TCHAR* ClodLarge = TEXT("/Game/Art/Props/SM_GraveClod_L.SM_GraveClod_L");
	inline constexpr const TCHAR* Rubble = TEXT("/Game/Art/Props/SM_GraveRubble.SM_GraveRubble");
	inline constexpr const TCHAR* Berm = TEXT("/Game/Art/Props/SM_GraveBerm.SM_GraveBerm");
	inline constexpr const TCHAR* Mound = TEXT("/Game/Art/Props/SM_GraveMound.SM_GraveMound");
	inline constexpr const TCHAR* MoundBurst = TEXT("/Game/Art/Props/SM_GraveMoundBurst.SM_GraveMoundBurst");
	AI_LOOTER_SHOOTER_API const TCHAR* DebrisPath(int32 Index);

	/** Decals: the heave's crack (a strip that tiles, and its end cap), the ploughed furrow, the impact scar, the clod's landing
	 *  shadow, a mound's cracks. Each takes a scalar Opacity (0-1) from code, and the crack an optional Reveal along its U. */
	inline constexpr const TCHAR* DecalHeaveCrack = TEXT("/Game/Art/Effects/GraveFight/MI_Decal_HeaveCrack.MI_Decal_HeaveCrack");
	inline constexpr const TCHAR* DecalHeaveCrackEnd = TEXT("/Game/Art/Effects/GraveFight/MI_Decal_HeaveCrackEnd.MI_Decal_HeaveCrackEnd");
	inline constexpr const TCHAR* DecalFurrow = TEXT("/Game/Art/Effects/GraveFight/MI_Decal_Furrow.MI_Decal_Furrow");
	inline constexpr const TCHAR* DecalImpactScar = TEXT("/Game/Art/Effects/GraveFight/MI_Decal_ImpactScar.MI_Decal_ImpactScar");
	inline constexpr const TCHAR* DecalLandingShadow = TEXT("/Game/Art/Effects/GraveFight/MI_Decal_LandingShadow.MI_Decal_LandingShadow");
	inline constexpr const TCHAR* DecalMoundCracks = TEXT("/Game/Art/Effects/GraveFight/MI_Decal_MoundCracks.MI_Decal_MoundCracks");

	/** Cards (UGravemotherFX): fresh-earth dust (a 2x2 sheet), pale floor dust, the crack's dust jets, a spark's streak, a flash. */
	inline constexpr const TCHAR* CardGraveDust = TEXT("/Game/Art/Effects/GraveFight/MI_FX_GraveDust.MI_FX_GraveDust");
	inline constexpr const TCHAR* CardFloorDust = TEXT("/Game/Art/Effects/GraveFight/MI_FX_FloorDust.MI_FX_FloorDust");
	inline constexpr const TCHAR* CardDustJet = TEXT("/Game/Art/Effects/GraveFight/MI_FX_GraveDustJet.MI_FX_GraveDustJet");
	inline constexpr const TCHAR* CardStoneSpark = TEXT("/Game/Art/Effects/GraveFight/MI_FX_StoneSpark.MI_FX_StoneSpark");
	inline constexpr const TCHAR* CardGraveFlash = TEXT("/Game/Art/Effects/GraveFight/MI_FX_GraveFlash.MI_FX_GraveFlash");

	/** Every path above (the asset test checks them once they're imported). */
	AI_LOOTER_SHOOTER_API TArray<FString> AllPaths();

	/**
	 * The asset at ObjectPath if it has been imported, else null, said once in the log; a missing one is looked for again
	 * a few seconds later, so an import while the editor runs is found. Not for constructors (FindIfMade is).
	 */
	AI_LOOTER_SHOOTER_API UObject* LoadAny(const TCHAR* ObjectPath, UClass* Class);
	template <typename T>
	T* Load(const TCHAR* ObjectPath)
	{
		return Cast<T>(LoadAny(ObjectPath, T::StaticClass()));
	}

	/** Whether the asset at ObjectPath exists now (for the stand-ins' logs and the tests). */
	AI_LOOTER_SHOOTER_API bool IsImported(const TCHAR* ObjectPath);

	/** A stand-in's shape: the engine's cube or sphere (100 cm across, pivot at the middle). */
	enum class EStandIn : uint8 { Cube, Sphere };
	AI_LOOTER_SHOOTER_API UStaticMesh* StandInMesh(EStandIn Shape);

	/** A stand-in's look: Color, flat, on M_StylizedSurface (as AGroundCrack's), made for Outer. */
	AI_LOOTER_SHOOTER_API UMaterialInterface* StandInLook(UObject* Outer, const FLinearColor& Color);

	/** The sheet's colour key, linear: her fresh grave earth, the berms' paler earth, the headstone's granite, the dusts. */
	AI_LOOTER_SHOOTER_API FLinearColor EarthColor();
	AI_LOOTER_SHOOTER_API FLinearColor BermColor();
	AI_LOOTER_SHOOTER_API FLinearColor GraniteColor();
	AI_LOOTER_SHOOTER_API FLinearColor EarthDustColor();
	AI_LOOTER_SHOOTER_API FLinearColor FloorDustColor();
	AI_LOOTER_SHOOTER_API FLinearColor StoneDustColor();

	/**
	 * A decal of MaterialPath laid on the ground at Center (projected straight down, Depth cm deep), Length cm along Yaw by
	 * Width across (the texture's U along its length), drawn over SortOrder's lower ones, wearing its own dynamic instance
	 * (Opacity starts at 0). Null when the material isn't imported: the caller shows its stand-in.
	 */
	AI_LOOTER_SHOOTER_API UDecalComponent* AddGroundDecal(AActor& Owner, const TCHAR* MaterialPath, const FVector& Center, float Yaw,
		float Length, float Width, int32 SortOrder, float Depth = 150.f);

	/** Its Opacity (0-1), and its Reveal (0-1 along its length, for a crack crawling out); null is fine. */
	AI_LOOTER_SHOOTER_API void SetDecalOpacity(UDecalComponent* Decal, float Opacity);
	AI_LOOTER_SHOOTER_API void SetDecalReveal(UDecalComponent* Decal, float Reveal);
	/** Where along its picture it starts (M_GraveDecal's Tiles offset in U): a strip that tiles along U (the crack, the
	 *  furrow) laid with one offset for all its tiles still joins up, and each furrow looks its own. */
	AI_LOOTER_SHOOTER_API void SetDecalShift(UDecalComponent* Decal, float UOffset);

	/** Resizes a ground decal laid by AddGroundDecal (its Center stays): Length along its line, Width across. */
	AI_LOOTER_SHOOTER_API void SetDecalSize(UDecalComponent* Decal, float Length, float Width, float Depth = 150.f);

	/** Tags the fight's shootable pieces carry: the stone's granite, the earth's dirt (melee and any surface reader). */
	inline const FName SurfaceStoneTag(TEXT("Surface.Stone"));
	inline const FName SurfaceDirtTag(TEXT("Surface.Dirt"));
	/** What spawn probes keep spiderlings and phase steps off (EncounterGroundProbe): berms and rubble carry it. */
	inline const FName ObstacleTag(TEXT("Obstacle"));
}
