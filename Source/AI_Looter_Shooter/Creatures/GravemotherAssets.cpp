// The Gravemother's Barrow fight's asset paths, their loading by path (null until imported), the stand-ins' shapes and
// looks, and the ground decals every piece of the fight lays (GravemotherAssets.h).

#include "Creatures/GravemotherAssets.h"
#include "AI_Looter_Shooter.h"
#include "Components/DecalComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	/** A missing asset is looked for again after this long (s): an import while the editor runs is picked up. */
	constexpr double MissingRecheckSeconds = 5.0;

	const TCHAR* const StonePaths[] = {
		TEXT("/Game/Art/Creatures/SM_GravemotherStone_0.SM_GravemotherStone_0"),
		TEXT("/Game/Art/Creatures/SM_GravemotherStone_1.SM_GravemotherStone_1"),
		TEXT("/Game/Art/Creatures/SM_GravemotherStone_2.SM_GravemotherStone_2"),
	};
	const TCHAR* const ShardPaths[] = {
		TEXT("/Game/Art/Creatures/SM_GravemotherStoneShard_1.SM_GravemotherStoneShard_1"),
		TEXT("/Game/Art/Creatures/SM_GravemotherStoneShard_2.SM_GravemotherStoneShard_2"),
		TEXT("/Game/Art/Creatures/SM_GravemotherStoneShard_3.SM_GravemotherStoneShard_3"),
		TEXT("/Game/Art/Creatures/SM_GravemotherStoneShard_4.SM_GravemotherStoneShard_4"),
		TEXT("/Game/Art/Creatures/SM_GravemotherStoneShard_5.SM_GravemotherStoneShard_5"),
	};
	const TCHAR* const ChipPaths[] = {
		TEXT("/Game/Art/Creatures/SM_StoneChip_1.SM_StoneChip_1"),
		TEXT("/Game/Art/Creatures/SM_StoneChip_2.SM_StoneChip_2"),
		TEXT("/Game/Art/Creatures/SM_StoneChip_3.SM_StoneChip_3"),
	};
	const TCHAR* const DebrisPaths[] = {
		TEXT("/Game/Art/Props/SM_GraveDebris_1.SM_GraveDebris_1"),
		TEXT("/Game/Art/Props/SM_GraveDebris_2.SM_GraveDebris_2"),
		TEXT("/Game/Art/Props/SM_GraveDebris_3.SM_GraveDebris_3"),
		TEXT("/Game/Art/Props/SM_GraveDebris_4.SM_GraveDebris_4"),
		TEXT("/Game/Art/Props/SM_GraveDebris_5.SM_GraveDebris_5"),
	};

	template <int32 Count>
	const TCHAR* PickPath(const TCHAR* const (&Paths)[Count], int32 Index)
	{
		return Paths[FMath::Clamp(Index, 0, Count - 1)];
	}

	/** Paths found missing, and when (so they're looked for again a while later, not on every spawn). */
	TMap<FString, double>& MissingSince()
	{
		static TMap<FString, double> Missing;
		return Missing;
	}

	const FName OpacityParameter(TEXT("Opacity"));
	const FName RevealParameter(TEXT("Reveal"));
	/** M_GraveDecal's (U repeats, V repeats, U offset, V offset). */
	const FName TilesParameter(TEXT("Tiles"));
}

namespace GravemotherAssets
{
	const TCHAR* StonePath(int32 State) { return PickPath(StonePaths, State); }
	const TCHAR* ShardPath(int32 Index) { return PickPath(ShardPaths, Index - 1); }
	const TCHAR* ChipPath(int32 Index) { return PickPath(ChipPaths, Index - 1); }
	const TCHAR* DebrisPath(int32 Index) { return PickPath(DebrisPaths, Index - 1); }

	TArray<FString> AllPaths()
	{
		TArray<FString> Paths = { Body, AbdomenBare, ClodSmall, ClodMedium, ClodLarge, Rubble, Berm, Mound, MoundBurst, DecalHeaveCrack,
			DecalHeaveCrackEnd, DecalFurrow, DecalImpactScar, DecalLandingShadow, DecalMoundCracks, CardGraveDust, CardFloorDust,
			CardDustJet, CardStoneSpark, CardGraveFlash };
		for (const TCHAR* Path : StonePaths) { Paths.Add(Path); }
		for (const TCHAR* Path : ShardPaths) { Paths.Add(Path); }
		for (const TCHAR* Path : ChipPaths) { Paths.Add(Path); }
		for (const TCHAR* Path : DebrisPaths) { Paths.Add(Path); }
		return Paths;
	}

	bool IsImported(const TCHAR* ObjectPath)
	{
		return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath)));
	}

	UObject* LoadAny(const TCHAR* ObjectPath, UClass* Class)
	{
		if (!ObjectPath || !Class)
		{
			return nullptr;
		}
		// Loaded already: no look at the disk.
		if (UObject* Found = StaticFindObject(Class, nullptr, ObjectPath))
		{
			return Found;
		}
		const FString Path(ObjectPath);
		const double Now = FPlatformTime::Seconds();
		if (const double* Since = MissingSince().Find(Path); Since && Now - *Since < MissingRecheckSeconds)
		{
			return nullptr;
		}
		if (!IsImported(ObjectPath))
		{
			if (!MissingSince().Contains(Path))
			{
				UE_LOG(LogLooter, Log, TEXT("Gravemother's fight: %s isn't imported yet; its stand-in shows."), *Path);
			}
			MissingSince().Add(Path, Now);
			return nullptr;
		}
		MissingSince().Remove(Path);
		return StaticLoadObject(Class, nullptr, ObjectPath);
	}

	UStaticMesh* StandInMesh(EStandIn Shape)
	{
		return LoadObject<UStaticMesh>(nullptr, Shape == EStandIn::Cube ? TEXT("/Engine/BasicShapes/Cube.Cube") : TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	}

	UMaterialInterface* StandInLook(UObject* Outer, const FLinearColor& Color)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
		UMaterialInstanceDynamic* Look = Base && Outer ? UMaterialInstanceDynamic::Create(Base, Outer) : nullptr;
		if (!Look)
		{
			return Base;
		}
		Look->SetVectorParameterValue(TEXT("Color"), Color);
		Look->SetScalarParameterValue(TEXT("Glow"), 0.f);
		Look->SetScalarParameterValue(TEXT("Variation"), 0.1f);
		return Look;
	}

	// The sheet's key (Saved/Gravemother/CONTRACT.md, "The look"), sRGB to linear.
	FLinearColor EarthColor() { return FLinearColor(FColor(0x6E, 0x5C, 0x49)); }
	FLinearColor BermColor() { return FLinearColor(FColor(0xB3, 0xA0, 0x8A)); }
	FLinearColor GraniteColor() { return FLinearColor(FColor(0x8E, 0x8B, 0x86)); }
	FLinearColor EarthDustColor() { return FLinearColor(FColor(0x80, 0x6B, 0x55)); }
	FLinearColor FloorDustColor() { return FLinearColor(FColor(0xDC, 0xC9, 0xA4)); }
	FLinearColor StoneDustColor() { return FLinearColor(FColor(0xD2, 0xCD, 0xC3)); }

	UDecalComponent* AddGroundDecal(AActor& Owner, const TCHAR* MaterialPath, const FVector& Center, float Yaw, float Length, float Width,
		int32 SortOrder, float Depth)
	{
		UMaterialInterface* Material = Load<UMaterialInterface>(MaterialPath);
		if (!Material)
		{
			return nullptr;
		}
		UDecalComponent* Decal = NewObject<UDecalComponent>(&Owner, NAME_None, RF_Transient);
		Decal->SetupAttachment(Owner.GetRootComponent());
		Decal->SetUsingAbsoluteLocation(true);
		Decal->SetUsingAbsoluteRotation(true);
		Decal->SetUsingAbsoluteScale(true);
		// It projects along its X (down); its Y runs down the picture (the texture's V: across the strip) and its Z across
		// it (U: along the strip), as the wanted posters' decals lie.
		const FVector Along = FRotator(0.f, Yaw, 0.f).Vector();
		Decal->SetWorldLocationAndRotation(Center, FRotationMatrix::MakeFromXZ(FVector::DownVector, Along).Rotator());
		Decal->DecalSize = FVector(Depth * 0.5f, FMath::Max(Width, 1.f) * 0.5f, FMath::Max(Length, 1.f) * 0.5f);
		Decal->SortOrder = SortOrder;
		Decal->SetDecalMaterial(Material);
		if (UMaterialInstanceDynamic* Look = Decal->CreateDynamicMaterialInstance())
		{
			Look->SetScalarParameterValue(OpacityParameter, 0.f);
		}
		// Off the decal pass until its opacity is set above nothing (SetDecalOpacity).
		Decal->SetVisibility(false);
		Decal->RegisterComponent();
		return Decal;
	}

	void SetDecalOpacity(UDecalComponent* Decal, float Opacity)
	{
		if (UMaterialInstanceDynamic* Look = Decal ? Cast<UMaterialInstanceDynamic>(Decal->GetDecalMaterial()) : nullptr)
		{
			Look->SetScalarParameterValue(OpacityParameter, FMath::Clamp(Opacity, 0.f, 1.f));
		}
		if (Decal)
		{
			// Nothing to draw at 0: keep it off the decal pass entirely.
			Decal->SetVisibility(Opacity > 0.005f);
		}
	}

	void SetDecalReveal(UDecalComponent* Decal, float Reveal)
	{
		if (UMaterialInstanceDynamic* Look = Decal ? Cast<UMaterialInstanceDynamic>(Decal->GetDecalMaterial()) : nullptr)
		{
			Look->SetScalarParameterValue(RevealParameter, FMath::Clamp(Reveal, 0.f, 1.f));
		}
	}

	void SetDecalShift(UDecalComponent* Decal, float UOffset)
	{
		if (UMaterialInstanceDynamic* Look = Decal ? Cast<UMaterialInstanceDynamic>(Decal->GetDecalMaterial()) : nullptr)
		{
			Look->SetVectorParameterValue(TilesParameter, FLinearColor(1.f, 1.f, UOffset, 0.f));
		}
	}

	void SetDecalSize(UDecalComponent* Decal, float Length, float Width, float Depth)
	{
		if (Decal)
		{
			Decal->DecalSize = FVector(Depth * 0.5f, FMath::Max(Width, 1.f) * 0.5f, FMath::Max(Length, 1.f) * 0.5f);
			Decal->MarkRenderStateDirty();
		}
	}
}
