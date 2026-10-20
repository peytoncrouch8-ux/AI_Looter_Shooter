#include "Player/Animation/BodyFigureComponent.h"
#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyRetargetAnimInstance.h"
#include "Session/SessionSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "Retargeter/IKRetargeter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Ellis boots to hat (Art/Models/Characters/Ellis.py), and the retargeter Tools/Unreal/build_hero_retarget.py makes. */
	const TCHAR* const FigurePath = TEXT("/Game/Art/Characters/SK_Ellis.SK_Ellis");
	const TCHAR* const RetargeterPath = TEXT("/Game/Player/Retarget/RTG_MannyToEllis.RTG_MannyToEllis");

	/** In a constructor: the asset at ObjectPath, or null while it isn't imported (no load error then). */
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

const FName UBodyFigureComponent::HolsterGunSlot(TEXT("EllisHolsterGun"));
const TCHAR* const UBodyFigureComponent::HolsterGunSuffix = TEXT("HolsterGun");

USkeletalMesh* UBodyFigureComponent::FindFigureMesh()
{
	return FindIfMade<USkeletalMesh>(FigurePath);
}

UIKRetargeter* UBodyFigureComponent::FindRetargeter()
{
	return FindIfMade<UIKRetargeter>(RetargeterPath);
}

UBodyFigureComponent::UBodyFigureComponent()
{
	// After the mannequin it copies (Wear adds it as a prerequisite), each frame, wherever it's drawn: the shadow in first
	// person needs the pose too.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Looks only: the capsule and the mannequin's own setup do everything else.
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	bReceivesDecals = false;

	static USkeletalMesh* const Figure = FindFigureMesh();
	static UIKRetargeter* const FoundRetargeter = FindRetargeter();
	Retargeter = FoundRetargeter;
	// AndUpdate, not the bare setter: the editor checks every skinned mesh was told its asset changed.
	SetSkinnedAssetAndUpdate(Figure, false);
	// The default animation mode (an anim instance class) runs it.
	AnimClass = UBodyRetargetAnimInstance::StaticClass();
}

USkeletalMeshComponent* UBodyFigureComponent::GetBody() const
{
	return Cast<USkeletalMeshComponent>(GetAttachParent());
}

bool UBodyFigureComponent::IsWorn() const
{
	return HiddenBody.IsValid();
}

void UBodyFigureComponent::BeginPlay()
{
	Super::BeginPlay();
	Wear();
}

void UBodyFigureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USkeletalMeshComponent* Body = HiddenBody.Get())
	{
		Body->SetHiddenInGame(false);
		HiddenBody.Reset();
	}
	Super::EndPlay(EndPlayReason);
}

void UBodyFigureComponent::PickBody(USkeletalMesh*& OutMesh, UIKRetargeter*& OutRetargeter) const
{
	// The hero played (the session's pick, or Looter.Hero's), when both their mesh and their retargeter are in the game.
	if (const UHeroDefinition* Hero = USessionSubsystem::FindPlayedHero(this))
	{
		USkeletalMesh* HeroMesh = Hero->LoadFigureMesh();
		UIKRetargeter* HeroRetargeter = HeroMesh ? Hero->LoadRetargeter() : nullptr;
		if (HeroMesh && HeroRetargeter)
		{
			OutMesh = HeroMesh;
			OutRetargeter = HeroRetargeter;
			return;
		}
		if (Hero->Id != UHeroDefinition::DefaultId())
		{
			UE_LOG(LogLooter, Log, TEXT("%s: %s's model isn't in the game yet (%s); Ellis's body stands in."), *GetName(), *Hero->Id.ToString(),
				!HeroMesh ? TEXT("no mesh") : TEXT("no retargeter"));
		}
	}
	// Ellis's: the class's defaults, or looked up again when they were remade since the class was set up
	// (build_hero_retarget.py replaces the assets, which leaves the defaults' pointers empty).
	const UBodyFigureComponent* Defaults = GetDefault<UBodyFigureComponent>();
	OutMesh = Defaults->GetSkeletalMeshAsset();
	OutRetargeter = Defaults->Retargeter;
	if (!OutRetargeter)
	{
		OutRetargeter = LoadObject<UIKRetargeter>(nullptr, RetargeterPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!OutMesh)
	{
		OutMesh = LoadObject<USkeletalMesh>(nullptr, FigurePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
}

void UBodyFigureComponent::WearHero()
{
	Wear();
}

void UBodyFigureComponent::Wear()
{
	// The body for the hero played; a change (another hero, or Ellis's remade) starts the animation over with it.
	USkeletalMesh* Mesh = nullptr;
	UIKRetargeter* NewRetargeter = nullptr;
	PickBody(Mesh, NewRetargeter);
	if (Mesh != GetSkeletalMeshAsset() || NewRetargeter != Retargeter)
	{
		Retargeter = NewRetargeter;
		SetSkinnedAssetAndUpdate(Mesh, false);
		// Hidden sections are kept by index on the component, not the mesh: Ellis's holstered revolver (slot 5, hidden
		// while a revolver is drawn) hid slot 5 of the next hero worn, Odessa's and Crane's coats. Show every section
		// of the new mesh; the holster is put right again on the next tick.
		for (int32 LOD = 0; LOD < GetNumLODs(); ++LOD)
		{
			ShowAllMaterialSections(LOD);
		}
		bHolsterGunShown = true;
		if (Mesh && Retargeter)
		{
			InitAnim(true);
		}
	}

	USkeletalMeshComponent* Body = GetBody();
	if (!Body || !GetSkeletalMeshAsset() || !Retargeter)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: no hero body is worn (%s); the mannequin shows."), *GetName(),
			!Body ? TEXT("not on a skeletal mesh") : !GetSkeletalMeshAsset() ? TEXT("SK_Ellis not imported") : TEXT("RTG_MannyToEllis not made"));
		if (USkeletalMeshComponent* Hidden = HiddenBody.Get())
		{
			Hidden->SetHiddenInGame(false);
			HiddenBody.Reset();
		}
		SetHiddenInGame(true);
		SetComponentTickEnabled(false);
		return;
	}
	SetComponentTickEnabled(true);
	// The mannequin animates first; the hero's body reads its finished pose the same frame.
	AddTickPrerequisiteComponent(Body);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->SetHiddenInGame(true);
	HiddenBody = Body;
	SetHiddenInGame(false);
	FollowBody();
}

void UBodyFigureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	FollowBody();
	FollowHolster();
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UBodyFigureComponent::FollowHolster()
{
	if (!bLookedForWeapons)
	{
		bLookedForWeapons = true;
		Weapons = GetOwner() ? GetOwner()->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	}
	const AWeaponBase* InHand = Weapons.IsValid() ? Weapons->GetActiveWeapon() : nullptr;
	const UWeaponDefinition* Definition = InHand ? InHand->GetInstance().Definition.Get() : nullptr;
	const bool bShow = !(Definition && Definition->Kind == EWeaponKind::Revolver);
	if (bShow != bHolsterGunShown)
	{
		ShowHolsterGun(bShow);
	}
}

bool UBodyFigureComponent::IsHolsterGunSlot(FName SlotName)
{
	return SlotName.ToString().EndsWith(HolsterGunSuffix);
}

void UBodyFigureComponent::ShowHolsterGun(bool bShow)
{
	bHolsterGunShown = bShow;
	if (!GetSkinnedAsset())
	{
		return;
	}
	// Every hero's holstered gun is its own slot ending in HolsterGun (EllisHolsterGun, ...): by slot on every LOD (no
	// section index: the slot's sections, whichever LOD draws).
	for (const FName& SlotName : GetMaterialSlotNames())
	{
		const int32 Slot = IsHolsterGunSlot(SlotName) ? GetMaterialIndex(SlotName) : INDEX_NONE;
		if (Slot == INDEX_NONE)
		{
			continue;
		}
		for (int32 LOD = 0; LOD < GetNumLODs(); ++LOD)
		{
			ShowMaterialSection(Slot, INDEX_NONE, bShow, LOD);
		}
	}
}

void UBodyFigureComponent::FollowBody()
{
	const USkeletalMeshComponent* Body = HiddenBody.Get();
	if (!Body)
	{
		return;
	}
	if (bOwnerNoSee != Body->bOwnerNoSee)
	{
		SetOwnerNoSee(Body->bOwnerNoSee);
	}
	if (FirstPersonPrimitiveType != Body->FirstPersonPrimitiveType)
	{
		SetFirstPersonPrimitiveType(Body->FirstPersonPrimitiveType);
	}
}
