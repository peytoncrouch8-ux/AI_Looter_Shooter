// UHeroDefinition: whether a hero is playable yet, their assets loaded only when they're in the game, and the roster.

#include "Embers/HeroDefinition.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
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

FSoftObjectPath UHeroDefinition::GetFirstPersonArmsPath() const
{
	if (!FirstPersonArms.IsNull())
	{
		return FirstPersonArms.ToSoftObjectPath();
	}
	// Ellis's arms came first and have no suffix.
	return Id == EllisId ? FSoftObjectPath(TEXT("/Game/Art/Characters/SK_FP_Arms.SK_FP_Arms"))
		: NamedPath(TEXT("/Game/Art/Characters"), TEXT("SK_FP_Arms_"), Id);
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

USkeletalMesh* UHeroDefinition::LoadFirstPersonArms() const
{
	return LoadIfThere<USkeletalMesh>(GetFirstPersonArmsPath());
}

UTexture2D* UHeroDefinition::LoadPortrait() const
{
	return LoadIfThere<UTexture2D>(GetPortraitPath());
}

FHeroMeleeTool UHeroDefinition::MeleeToolFor(FName HeroId)
{
	// The user's picks (2026-10-27) with the sheets' swings and feel tags. Art/Models/Weapons/Melee/<Tool>.py imports to
	// /Game/Art/Melee (models.ps1: the folder names the category).
	struct FPick
	{
		const TCHAR* Hero;
		const TCHAR* Tool;
		EMeleeGripKind Grip;
		EMeleeSwingStyle Swing;
		EMeleeHeft Heft;
		/** Its sounds' and effects' key (FHeroMeleeTool::Kind). */
		const TCHAR* Kind;
		/** The handle's radius under the fist (cm, MeleeTools.py): the spade's ash 1.55, the gaff's pole 1.75 (POLE_R), the
		 * pick's hickory 1.45, the fan's pivot stack about 1.2. */
		float HandleRadius;
		/** Round all the way under the fist (FHeroMeleeTool::bRoundHandle): the gaff's pole. */
		bool bRoundHandle = false;
	};
	static const FPick Picks[] = {
		{ TEXT("Ellis"), TEXT("GraveSpade"), EMeleeGripKind::Held, EMeleeSwingStyle::Swat, EMeleeHeft::Steady, TEXT("Spade"), 1.55f },
		{ TEXT("Odessa"), TEXT("RazorFan"), EMeleeGripKind::Held, EMeleeSwingStyle::Flick, EMeleeHeft::Quick, TEXT("Fan"), 1.2f },
		{ TEXT("Crane"), TEXT("FerryGaff"), EMeleeGripKind::Held, EMeleeSwingStyle::Gaff, EMeleeHeft::Heavy, TEXT("Gaff"), 1.75f, true },
		{ TEXT("Gauge"), TEXT("PistonFist"), EMeleeGripKind::Forearm, EMeleeSwingStyle::Jab, EMeleeHeft::Quick, TEXT("Piston"), 1.6f },
		{ TEXT("Pike"), TEXT("RockPick"), EMeleeGripKind::Held, EMeleeSwingStyle::Peck, EMeleeHeft::Quick, TEXT("Pick"), 1.45f },
	};
	FHeroMeleeTool Tool;
	for (const FPick& Pick : Picks)
	{
		if (HeroId == FName(Pick.Hero))
		{
			Tool.Mesh = TSoftObjectPtr<UStaticMesh>(NamedPath(TEXT("/Game/Art/Melee"), TEXT("SM_Melee_"), FName(Pick.Tool)));
			Tool.Grip = Pick.Grip;
			Tool.Swing = Pick.Swing;
			Tool.Heft = Pick.Heft;
			Tool.Kind = FName(Pick.Kind);
			Tool.HandleRadius = Pick.HandleRadius;
			Tool.bRoundHandle = Pick.bRoundHandle;
			break;
		}
	}
	return Tool;
}

FHeroMeleeTool UHeroDefinition::GetMeleeTool() const
{
	if (MeleeTool.Mesh.IsNull())
	{
		return MeleeToolFor(Id);
	}
	// A tool set on the data keeps the picks' sounds and effects unless it names its own.
	FHeroMeleeTool Tool = MeleeTool;
	if (Tool.Kind.IsNone())
	{
		Tool.Kind = MeleeToolFor(Id).Kind;
	}
	return Tool;
}

FSoftObjectPath UHeroDefinition::PlaceholderMeleeToolPath()
{
	return FSoftObjectPath(TEXT("/Game/Art/Melee/SM_Melee_Placeholder.SM_Melee_Placeholder"));
}

UStaticMesh* UHeroDefinition::LoadMeleeTool() const
{
	// Modeled after the user's picks (TOOLS, 2026-10-27): every hero swings the one placeholder till theirs is imported.
	if (UStaticMesh* Own = LoadIfThere<UStaticMesh>(GetMeleeTool().Mesh.ToSoftObjectPath()))
	{
		return Own;
	}
	return LoadIfThere<UStaticMesh>(PlaceholderMeleeToolPath());
}

void UHeroDefinition::SetMeleeTool(const FString& MeshPath, EMeleeGripKind Grip, EMeleeSwingStyle Swing, EMeleeHeft Heft, float Scale)
{
	MeleeTool.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath));
	MeleeTool.Grip = Grip;
	MeleeTool.Swing = Swing;
	MeleeTool.Heft = Heft;
	MeleeTool.Scale = FMath::Clamp(Scale, 0.25f, 3.f);
	MarkPackageDirty();
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

void UHeroDefinition::SetFirstPersonArms(const FString& ArmsPath, float ArmsScale)
{
	FirstPersonArms = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(ArmsPath));
	FirstPersonArmsScale = FMath::Clamp(ArmsScale, 0.f, 1.3f);
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
