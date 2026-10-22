#include "Weapons/PartEffectComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementTuning.h"
#include "Weapons/PartEffectRules.h"
#include "Weapons/PartEffectTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "CoreGlobals.h"
#include "Engine/World.h"

namespace
{
	/** An Overcharge's gathering light at the muzzle with no element: the ember's own orange. */
	const FLinearColor ChargeColor(1.f, 0.55f, 0.2f);
	/** Motes a second drawn in to the muzzle at full charge. */
	constexpr float ChargeMoteRate = 30.f;
}

UPartEffectComponent::UPartEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

UPartEffectComponent* UPartEffectComponent::Find(const AActor* Weapon)
{
	return Weapon ? Weapon->FindComponentByClass<UPartEffectComponent>() : nullptr;
}

UPartEffectComponent* UPartEffectComponent::For(AWeaponBase& Weapon)
{
	if (UPartEffectComponent* Existing = Find(&Weapon))
	{
		return Existing;
	}
	UPartEffectComponent* Added = NewObject<UPartEffectComponent>(&Weapon, TEXT("PartEffects"));
	Added->RegisterComponent();
	return Added;
}

void UPartEffectComponent::Refresh()
{
	const AWeaponBase* Gun = Cast<AWeaponBase>(GetOwner());
	if (!Gun)
	{
		return;
	}
	const FWeaponInstanceData& Instance = Gun->GetInstance();
	uint32 Key = HashCombine(GetTypeHash(Instance.Definition.Get()), HashCombine(static_cast<uint32>(Instance.Rarity), static_cast<uint32>(Instance.Seed)));
	for (const FName& Part : Instance.Parts)
	{
		Key = HashCombine(Key, GetTypeHash(Part));
	}
	if (bRead && Key == ReadKey)
	{
		return;
	}
	bRead = true;
	ReadKey = Key;
	const FWeaponLook Look = WeaponParts::Pick(Instance);
	Effect = WeaponParts::EffectOf(Look);
	Element = WeaponParts::ElementOf(Look);
	ElementChance = Element == ELooterElement::None ? 0.f
		: ElementRules::GunChance(Instance.Definition ? Instance.Definition->Kind : EWeaponKind::None, Instance.Rarity, FElementTuning::Live());
}

double UPartEffectComponent::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UPartEffectComponent::BeginCharge()
{
	ChargeStart = Now();
	GlowOwed = 0.f;
	SetComponentTickEnabled(true);
}

void UPartEffectComponent::CancelCharge()
{
	ChargeStart = -1.0;
	SetComponentTickEnabled(false);
}

float UPartEffectComponent::GetCharge() const
{
	if (!IsCharging())
	{
		return 0.f;
	}
	// On the holder's own clock (Sundown keeps the player at full speed while the world crawls).
	const AActor* Holder = GetOwner() ? GetOwner()->GetOwner() : nullptr;
	const float Dilation = Holder && Holder->CustomTimeDilation > UE_KINDA_SMALL_NUMBER ? Holder->CustomTimeDilation : 1.f;
	return PartEffectRules::ChargeAfter(static_cast<float>(Now() - ChargeStart) * Dilation, FPartEffectTuning::Live());
}

float UPartEffectComponent::TakeChargeScale()
{
	// Everything fired this frame is the same pull's.
	if (GFrameCounter == ChargeTakenFrame)
	{
		return ChargeTakenScale;
	}
	ChargeTakenFrame = GFrameCounter;
	ChargeTakenScale = IsCharging() ? PartEffectRules::ChargeDamageScale(GetCharge(), FPartEffectTuning::Live()) : 1.f;
	CancelCharge();
	return ChargeTakenScale;
}

void UPartEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AWeaponBase* Gun = Cast<AWeaponBase>(GetOwner());
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	// A gun put down or dropped mid wind-up lets it go.
	if (!IsCharging() || !Gun || Gun->IsPickup() || Gun->IsReloading() || !Bullets)
	{
		CancelCharge();
		return;
	}
	FWeaponFX& FX = Bullets->GetEffects();
	FX.Initialize(World);
	const float Charge = GetCharge();
	const FVector Muzzle = Gun->GetVisibleMuzzleLocation();
	const ELooterElement GunElement = GetElement();
	const FLinearColor Color = GunElement == ELooterElement::None ? ChargeColor : ElementRules::FxColor(GunElement);
	// A glow swelling at the muzzle, quickening as it fills, and motes drawn in to it: the round is ready when it peaks.
	const float Pulse = Charge >= 1.f ? 0.85f + 0.15f * FMath::Sin(static_cast<float>(Now()) * 30.f) : 1.f;
	FX.AddGlow(Muzzle, (2.f + 9.f * Charge) * Pulse, FMath::Lerp(Color, FLinearColor::White, 0.3f * Charge), 6.f + 18.f * Charge);
	GlowOwed += ChargeMoteRate * Charge * DeltaTime;
	const int32 Motes = FMath::FloorToInt32(GlowOwed);
	GlowOwed -= Motes;
	if (Motes > 0)
	{
		FX.SpawnElementAura(GunElement == ELooterElement::None ? ELooterElement::SoulLight : GunElement, Muzzle, 6.f, 4.f, Motes, 0.6f + Charge);
	}
}
