#include "Story/WardenCrewBody.h"
#include "AI_Looter_Shooter.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Story/WardenCrewAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const WardenCrewBody::MannequinPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");

namespace
{
	/** What a stand-in's material is, by the character master it's made on (Tools/Unreal/build_world_materials.py). */
	enum class ECrewSurface : uint8
	{
		Cloth,
		Skin,
		Hair,
		Glow,
	};

	ECrewSurface SurfaceOf(const UMaterialInterface* Material)
	{
		const UMaterial* Master = Material ? Material->GetMaterial() : nullptr;
		const FString Name = Master ? Master->GetName() : FString();
		if (Name.Contains(TEXT("Glow")))
		{
			return ECrewSurface::Glow;
		}
		if (Name.Contains(TEXT("Skin")))
		{
			return ECrewSurface::Skin;
		}
		if (Name.Contains(TEXT("Hair")))
		{
			return ECrewSurface::Hair;
		}
		return ECrewSurface::Cloth;
	}
}

UBodyFigureComponent* WardenCrewBody::MakeBody(AActor& Owner, USkeletalMeshComponent& Body)
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mannequin(MannequinPath);
	// AndUpdate, not the bare setter: the editor checks every skinned mesh was told its asset changed.
	Body.SetSkinnedAssetAndUpdate(Mannequin.Object, false);
	// The mannequin faces its +Y; the actor's +X is the person's front.
	Body.SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Body.SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body.AnimClass = UWardenCrewAnimInstance::StaticClass();
	// Looks only: the actor's own hull is what the player bumps into and talks to.
	Body.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body.SetGenerateOverlapEvents(false);
	Body.SetCanEverAffectNavigation(false);
	Body.bReceivesDecals = false;
	// Hidden under the stand-in, it still animates: the stand-in copies its pose.
	Body.VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	UBodyFigureComponent* Figure = Owner.CreateDefaultSubobject<UBodyFigureComponent>(TEXT("Figure"));
	Figure->SetupAttachment(&Body);
	Figure->LDMaxDrawDistance = DrawDistance;
	Figure->SetCachedMaxDrawDistance(DrawDistance);
	Figure->bReceivesDecals = false;
	return Figure;
}

bool WardenCrewBody::WearLook(USkeletalMeshComponent& Body, TObjectPtr<UBodyFigureComponent>& Figure, const FWardenLook& Look)
{
	// Their own model, once it's in the game: worn as the animated body itself, when it shares the mannequin's skeleton (the
	// idle and the layer are the mannequin's bones).
	USkeletalMesh* Own = nullptr;
	if (Look.Model.IsValid() && FPackageName::DoesPackageExist(Look.Model.GetLongPackageName()))
	{
		Own = Cast<USkeletalMesh>(Look.Model.TryLoad());
	}
	const USkeletalMesh* Mannequin = Cast<USkeletalMesh>(Body.GetSkinnedAsset());
	if (Own && Mannequin && Own->GetSkeleton() == Mannequin->GetSkeleton())
	{
		Body.SetSkinnedAssetAndUpdate(Own, true);
		Body.SetRelativeScale3D(FVector(Look.ModelScale));
		Body.SetHiddenInGame(false);
		Body.LDMaxDrawDistance = DrawDistance;
		Body.SetCachedMaxDrawDistance(DrawDistance);
		if (Figure)
		{
			Figure->DestroyComponent();
			Figure = nullptr;
		}
		return true;
	}
	UE_CLOG(Own, LogLooter, Warning, TEXT("%s: %s isn't on the mannequin's skeleton, so %s's body stands in (import it onto SK_Mannequin)."),
		*GetNameSafe(Body.GetOwner()), *Look.Model.ToString(), *Look.StandInHero.ToString());
	Body.SetRelativeScale3D(FVector(Look.StandInScale));
	if (Figure)
	{
		// Before the figure's own play begins, so it puts on this hero's body the first time.
		Figure->SetShownHero(UHeroDefinition::Load(Look.StandInHero));
	}
	return false;
}

void WardenCrewBody::DressStandIn(UBodyFigureComponent* Figure, const FWardenLook& Look)
{
	if (!Figure || !Figure->IsWorn())
	{
		return;
	}
	// Each material its own copy, its colour multiplied toward the person (the character masters' Tint), glows dimmed: a
	// living crew has no burning eyes.
	for (int32 Index = 0; Index < Figure->GetNumMaterials(); ++Index)
	{
		UMaterialInterface* Material = Figure->GetMaterial(Index);
		if (!Material)
		{
			continue;
		}
		UMaterialInstanceDynamic* Dressed = Figure->CreateDynamicMaterialInstance(Index, Material);
		if (!Dressed)
		{
			continue;
		}
		switch (SurfaceOf(Material))
		{
		case ECrewSurface::Glow:
			Dressed->SetScalarParameterValue(TEXT("Glow"), Look.Glow);
			break;
		case ECrewSurface::Skin:
			Dressed->SetVectorParameterValue(TEXT("Tint"), Look.Skin);
			break;
		case ECrewSurface::Hair:
			Dressed->SetVectorParameterValue(TEXT("Tint"), Look.Hair);
			break;
		default:
			Dressed->SetVectorParameterValue(TEXT("Tint"), Look.Cloth);
			break;
		}
	}
	// The hero's holstered revolver isn't theirs (a stand-in carries no gun at the hip).
	for (const FName& Slot : Figure->GetMaterialSlotNames())
	{
		const int32 SlotIndex = UBodyFigureComponent::IsHolsterGunSlot(Slot) ? Figure->GetMaterialIndex(Slot) : INDEX_NONE;
		for (int32 LOD = 0; SlotIndex != INDEX_NONE && LOD < Figure->GetNumLODs(); ++LOD)
		{
			Figure->ShowMaterialSection(SlotIndex, INDEX_NONE, false, LOD);
		}
	}
}

EWardenDetail WardenCrewBody::DetailFor(float Distance)
{
	if (Distance < 0.f || Distance <= FullDetailRange)
	{
		return EWardenDetail::Full;
	}
	return Distance <= ReducedDetailRange ? EWardenDetail::Reduced : EWardenDetail::Held;
}

void WardenCrewBody::ApplyDetail(USkeletalMeshComponent& Body, UBodyFigureComponent* Figure, EWardenDetail Detail, EWardenDetail& Current)
{
	if (Detail == Current)
	{
		return;
	}
	Current = Detail;
	const bool bTicks = Detail != EWardenDetail::Held;
	const float Interval = Detail == EWardenDetail::Reduced ? ReducedInterval : 0.f;
	Body.SetComponentTickEnabled(bTicks);
	Body.SetComponentTickInterval(Interval);
	if (Figure && Figure->IsWorn())
	{
		Figure->SetComponentTickEnabled(bTicks);
		Figure->SetComponentTickInterval(Interval);
	}
	// What's drawn: the stand-in, or their own model on the body.
	USkeletalMeshComponent* Shown = Figure && Figure->IsWorn() ? static_cast<USkeletalMeshComponent*>(Figure) : &Body;
	Shown->SetCastShadow(bTicks);
}

float WardenCrewBody::PlayerDistance(const AActor& Context, const FVector& Where)
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(&Context, 0);
	return Player ? static_cast<float>(FVector::Dist(Player->GetActorLocation(), Where)) : -1.f;
}

bool WardenCrewBody::PlayerEyes(const AActor& Context, FVector& OutEyes)
{
	const APlayerController* Controller = UGameplayStatics::GetPlayerController(&Context, 0);
	if (!Controller || !Controller->GetPawn())
	{
		return false;
	}
	FRotator Unused;
	Controller->GetPlayerViewPoint(OutEyes, Unused);
	return true;
}

FVector WardenCrewBody::EyesOf(const AActor& Someone)
{
	const APawn* Pawn = Cast<APawn>(&Someone);
	const APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (Controller)
	{
		FVector Eyes;
		FRotator Unused;
		Controller->GetPlayerViewPoint(Eyes, Unused);
		return Eyes;
	}
	return Someone.GetActorLocation() + FVector(0.0, 0.0, 60.0);
}

float WardenCrewBody::YawTo(const AActor& From, const FVector& Where)
{
	const FVector To = Where - From.GetActorLocation();
	if (To.SizeSquared2D() < 1.0)
	{
		return 0.f;
	}
	return static_cast<float>(FRotator::NormalizeAxis(To.Rotation().Yaw - From.GetActorRotation().Yaw));
}
