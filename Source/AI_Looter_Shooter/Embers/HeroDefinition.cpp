// UHeroDefinition: whether a hero is playable yet, their assets loaded only when they're in the game, and the roster.

#include "Embers/HeroDefinition.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Retargeter/IKRetargeter.h"

namespace
{
	const FName EllisId(TEXT("Ellis"));

#if !UE_BUILD_SHIPPING
	/** The developers' key to the heroes in the works: picked, they play with Ellis's body and no powers. */
	TAutoConsoleVariable<int32> CVarHeroUnlock(TEXT("Looter.Hero.Unlock"), 0,
		TEXT("1: the hero select lets heroes in the works be picked (they play with Ellis's body and no powers until their model, "
			"retargeter and powers are in the game). 0: only playable heroes."));
#endif

	/**
	 * Path's asset when its package is in the game, else null without a load warning: a hero's model lands after their
	 * definition, and the select screen asks about all five every time it opens.
	 */
	template <typename T>
	T* LoadIfThere(const FSoftObjectPath& Path)
	{
		if (Path.IsNull())
		{
			return nullptr;
		}
		if (UObject* Loaded = Path.ResolveObject())
		{
			return Cast<T>(Loaded);
		}
		const FString Package = Path.GetLongPackageName();
		if (Package.IsEmpty() || !FPackageName::DoesPackageExist(Package))
		{
			return nullptr;
		}
		return Cast<T>(Path.TryLoad());
	}

	/** The naming rule's path: Folder/<Prefix><Id>.<Prefix><Id>. */
	FSoftObjectPath NamedPath(const TCHAR* Folder, const TCHAR* Prefix, FName Id)
	{
		const FString Name = FString::Printf(TEXT("%s%s"), Prefix, *Id.ToString());
		return FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, *Name, *Name));
	}

	/** Soft's path, or the naming rule's while it's empty. */
	template <typename T>
	FSoftObjectPath PathOr(const TSoftObjectPtr<T>& Soft, const TCHAR* Folder, const TCHAR* Prefix, FName Id)
	{
		return Soft.IsNull() ? NamedPath(Folder, Prefix, Id) : Soft.ToSoftObjectPath();
	}
}

FSoftObjectPath UHeroDefinition::GetPortraitPath() const
{
	return PathOr(Portrait, TEXT("/Game/UI/Portraits"), TEXT("T_Portrait_"), Id);
}

FSoftObjectPath UHeroDefinition::GetFigureMeshPath() const
{
	return PathOr(FigureMesh, TEXT("/Game/Art/Characters"), TEXT("SK_"), Id);
}

FSoftObjectPath UHeroDefinition::GetRetargeterPath() const
{
	return PathOr(Retargeter, TEXT("/Game/Player/Retarget"), TEXT("RTG_MannyTo"), Id);
}

bool UHeroDefinition::HasAllPowerCode() const
{
	if (Powers.IsEmpty())
	{
		return false;
	}
	for (const UEmberDefinition* Power : Powers)
	{
		const UClass* PowerClass = Power ? Power->PowerClass.Get() : nullptr;
		if (!PowerClass || PowerClass->HasAnyClassFlags(CLASS_Abstract))
		{
			return false;
		}
	}
	return true;
}

FText UHeroDefinition::GetShortName() const
{
	return ShortName.IsEmpty() ? FText::FromName(Id) : ShortName;
}

bool UHeroDefinition::IsPlayable() const
{
	// The cheap test first: the twelve new powers' code lands after the models.
	return HasAllPowerCode() && LoadFigureMesh() && LoadRetargeter();
}

bool UHeroDefinition::CanBePicked() const
{
	return IsPlayable() || AreLockedHeroesUnlocked();
}

USkeletalMesh* UHeroDefinition::LoadFigureMesh() const
{
	return LoadIfThere<USkeletalMesh>(GetFigureMeshPath());
}

UIKRetargeter* UHeroDefinition::LoadRetargeter() const
{
	return LoadIfThere<UIKRetargeter>(GetRetargeterPath());
}

UTexture2D* UHeroDefinition::LoadPortrait() const
{
	return LoadIfThere<UTexture2D>(GetPortraitPath());
}

FName UHeroDefinition::DefaultId()
{
	return EllisId;
}

const TArray<FName>& UHeroDefinition::RosterIds()
{
	// The contract's order (2026-10-19): Ellis first, the story's own; the four the user picked portraits for after.
	static const TArray<FName> Ids = { EllisId, FName(TEXT("Odessa")), FName(TEXT("Crane")), FName(TEXT("Gauge")), FName(TEXT("Pike")) };
	return Ids;
}

FString UHeroDefinition::AssetPath(FName Id)
{
	const FString Name = FString::Printf(TEXT("DA_Hero_%s"), *Id.ToString());
	return FString::Printf(TEXT("/Game/Data/Embers/%s.%s"), *Name, *Name);
}

UHeroDefinition* UHeroDefinition::Load(FName Id)
{
	if (Id.IsNone())
	{
		return nullptr;
	}
	const FString Path = AssetPath(Id);
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
	{
		return nullptr;
	}
	return LoadObject<UHeroDefinition>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

void UHeroDefinition::SetLookPaths(const FString& PortraitPath, const FString& FigureMeshPath, const FString& RetargeterPath)
{
	// An empty path leaves the field empty (the naming rule stands in: GetFigureMeshPath and the rest).
	Portrait = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(PortraitPath));
	FigureMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(FigureMeshPath));
	Retargeter = TSoftObjectPtr<UIKRetargeter>(FSoftObjectPath(RetargeterPath));
	MarkPackageDirty();
}

bool UHeroDefinition::AreLockedHeroesUnlocked()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return CVarHeroUnlock.GetValueOnGameThread() != 0;
#endif
}
