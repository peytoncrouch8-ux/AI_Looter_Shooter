#include "Weapons/WeaponModelComponent.h"
#include "AI_Looter_Shooter.h"
#include "Elements/ElementRules.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// The frontier-tech guns' moving and glowing parts on the gun's model (Phase 12.3): the sniper's bolt, the launcher's
// next rocket, the SMG helix's and the LMG barrel's glows the game drives, and the launcher's exhaust.

const FName UWeaponModelComponent::AmmoGlowSlot(TEXT("GunAmmoGlow"));
const FName UWeaponModelComponent::HeatGlowSlot(TEXT("GunHeatGlow"));

namespace
{
	/** The gauge master (Tools/Unreal/build_gun_gauge_material.py): lit along UV0.x up to its Fill. */
	const TCHAR* const GaugeMasterPath = TEXT("/Game/Art/Materials/Masters/M_GunGauge.M_GunGauge");

	/** Without the gauge master the whole slot glows: its strength never falls quite to nothing until it's empty. */
	constexpr float FallbackLeastShare = 0.2f;

	void SetGauge(UMaterialInstanceDynamic* Gauge, float Fill, float Strength)
	{
		if (!Gauge)
		{
			return;
		}
		const float Lit = FMath::Clamp(Fill, 0.f, 1.f);
		Gauge->SetScalarParameterValue(TEXT("Fill"), Lit);
		// The gauge master cuts the length itself; an ordinary glow can only dim, so it dims with what's left.
		const UMaterialInterface* Master = UWeaponModelComponent::FindGaugeMaster();
		const bool bMaster = Master && Gauge->Parent.Get() == Master;
		const float Share = Lit <= 0.f ? 0.f : FMath::Lerp(FallbackLeastShare, 1.f, Lit);
		Gauge->SetScalarParameterValue(TEXT("Glow"), FMath::Max(Strength, 0.f) * (bMaster ? 1.f : Share));
	}
}

UMaterialInterface* UWeaponModelComponent::FindGaugeMaster()
{
	// Looked up once: the master is an asset of the game's own and doesn't come and go while it runs.
	static TWeakObjectPtr<UMaterialInterface> Found;
	static bool bLooked = false;
	if (!bLooked || (!Found.IsValid() && GIsEditor))
	{
		bLooked = true;
		Found = LoadObject<UMaterialInterface>(nullptr, GaugeMasterPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Found.Get();
}

UMaterialInstanceDynamic* UWeaponModelComponent::MakeGauge(FName Slot, UMaterialInterface* Source, ELooterElement Element)
{
	UMaterialInterface* Master = FindGaugeMaster();
	UMaterialInstanceDynamic* Gauge = UMaterialInstanceDynamic::Create(Master ? Master : Source, this);
	if (Master && Source)
	{
		// The art's color for the slot (its stylized material's Color), so the ember orange stays the modeler's.
		FLinearColor Color;
		if (Source->GetVectorParameterValue(FMaterialParameterInfo(FName(TEXT("Color"))), Color))
		{
			Gauge->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
	// An element helix glows in its element's hue: the art's ember orange drowned the Soul-light part's gold-white and the
	// salt's cyan. The heat rings take it too while every glow does (the user's call 2026-10-27, "Glow parts take element").
	if (Element != ELooterElement::None && (Slot == AmmoGlowSlot || ElementGlowsOn()))
	{
		Gauge->SetVectorParameterValue(TEXT("Color"), ElementRules::FxColor(Element));
	}
	Paints.Add(Gauge);
	if (Slot == AmmoGlowSlot)
	{
		AmmoGauge = Gauge;
	}
	else
	{
		HeatGauge = Gauge;
	}
	// A fresh gun shows full and cold until its owner lights it (AWeaponBase::RefreshGauges).
	SetGauge(Gauge, Slot == AmmoGlowSlot ? 1.f : 0.f, Slot == AmmoGlowSlot ? 1.f : 0.f);
	return Gauge;
}

void UWeaponModelComponent::MakeRocketLoad(const UWeaponDefinition& Definition, UStaticMeshComponent& Loaded)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Definition.ProjectileMesh)
	{
		return;
	}
	// Hung where the loaded rocket is (the barrel's Rocket socket): its origin is where it seats, so it slides in along -X.
	UStaticMeshComponent* Rocket = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
	Rocket->SetStaticMesh(Definition.ProjectileMesh);
	Rocket->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rocket->SetGenerateOverlapEvents(false);
	Rocket->SetCanEverAffectNavigation(false);
	Rocket->SetCastShadow(false);
	Rocket->SetFirstPersonPrimitiveType(FirstPersonType);
	Rocket->SetupAttachment(Loaded.GetAttachParent(), Loaded.GetAttachSocketName());
	Rocket->SetVisibility(false);
	Rocket->RegisterComponent();
	RocketLoadMesh = Rocket;
}

void UWeaponModelComponent::FindBackblast()
{
	static const FName Socket(TEXT("Backblast"));
	bHasBackblast = false;
	for (int32 Index = Parts.Num() - 1; Index >= 0; --Index)
	{
		const UStaticMeshComponent* Part = Parts[Index];
		if (Part && Part->DoesSocketExist(Socket))
		{
			const FTransform World = Part->GetSocketTransform(Socket, RTS_World);
			const FTransform& Gun = GetComponentTransform();
			Backblast = Gun.InverseTransformPosition(World.GetLocation());
			BackblastWay = Gun.InverseTransformVectorNoScale(World.GetUnitAxis(EAxis::X)).GetSafeNormal(UE_SMALL_NUMBER, -FVector::ForwardVector);
			bHasBackblast = true;
			return;
		}
	}
}

void UWeaponModelComponent::SetBoltOpen(float Open)
{
	if (!BoltMesh)
	{
		return;
	}
	float Lift = 0.f;
	float Back = 0.f;
	LooterReload::BoltPose(Open, Lift, Back);
	// Turned about the bore (its own X: a positive turn carries its right side, where the handle is, up), then drawn back.
	BoltMesh->SetRelativeLocationAndRotation(FVector(-Back, 0.f, 0.f), FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Lift)));
}

void UWeaponModelComponent::SetRocketLoad(float Travel, bool bIncomingShown, bool bLoaded)
{
	if (ReloadPart != EWeaponReloadPart::Rocket || !ReloadPartMesh)
	{
		return;
	}
	ReloadPartMesh->SetVisibility(bLoaded);
	if (RocketLoadMesh)
	{
		RocketLoadMesh->SetRelativeLocation(FVector(FMath::Max(Travel, 0.f), 0.f, 0.f));
		RocketLoadMesh->SetVisibility(bIncomingShown && !bLoaded);
	}
}

void UWeaponModelComponent::SetAmmoGauge(float Fill, float Strength)
{
	SetGauge(AmmoGauge, Fill, Strength);
}

void UWeaponModelComponent::SetHeatGauge(float Fill, float Strength)
{
	SetGauge(HeatGauge, Fill, Strength);
}

FLinearColor UWeaponModelComponent::GetAmmoGaugeColor() const
{
	FLinearColor Color = FLinearColor::Black;
	if (AmmoGauge)
	{
		AmmoGauge->GetVectorParameterValue(FMaterialParameterInfo(FName(TEXT("Color"))), Color);
	}
	return Color;
}
