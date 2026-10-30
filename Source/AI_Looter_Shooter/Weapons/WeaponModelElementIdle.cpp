#include "Weapons/WeaponModelComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/ElementVfxTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// The held gun giving off its element ("Elements matter" lane F, 2026-10-29; the user's pick: "the held gun giving off its
// element (smoke, sparks, drips) idle"): in the player's own view only, a few pieces a second (FElementVfxTuning::IdleRate x
// Looter.Vfx.Element.Idle) off its glowing parts and its element part: Cinder embers and a curl of smoulder, Thunder a crackle,
// Rot a drip, Wardbreak frost and a glint, Hallow a mote. Cheap: the component ticks only while a first-person element gun
// shows, and the pieces are the bullets' pooled FWeaponFX particles, spawned where the eye sees the gun (it is drawn with its
// own field of view) and carried at its holder's speed.

namespace
{
	/** A piece comes off this share of a part's bounds round its middle (not off the bounds' empty corners). */
	constexpr float IdleSpread = 0.6f;
	/** The longest frame counted (a hitch would owe a burst of pieces at once). */
	constexpr float IdleMostStep = 0.1f;
}

void UWeaponModelComponent::StartElementIdle(ELooterElement Element, FName RarityGlowSlot)
{
	ModelElement = Element;
	IdleSpots.Reset();
	IdleOwed = 0.f;
	if (Element != ELooterElement::None)
	{
		// Its element part and every part that glows in the element's colour (vents, barrel slots, gauges), but never the sight:
		// what comes off it would hang over the crosshair when aimed.
		for (UStaticMeshComponent* Part : Parts)
		{
			if (!Part || Part == SightPart)
			{
				continue;
			}
			bool bGlows = Part == ElementPart;
			for (const FName Slot : Part->GetMaterialSlotNames())
			{
				bGlows = bGlows || IsElementGlowSlot(Slot, RarityGlowSlot);
			}
			if (bGlows)
			{
				IdleSpots.Add(Part);
			}
		}
		if (IdleSpots.IsEmpty() && ElementPart && ElementPart != SightPart)
		{
			IdleSpots.Add(ElementPart);
		}
	}
	RefreshElementIdleTick();
}

void UWeaponModelComponent::RefreshElementIdleTick()
{
	// The cvar is read as it ticks (not here), so turning it back on in play needs no rebuild.
	const bool bWanted = ModelElement != ELooterElement::None && !IdleSpots.IsEmpty() && FirstPersonType == EFirstPersonPrimitiveType::FirstPerson;
	if (IsComponentTickEnabled() != bWanted)
	{
		SetComponentTickEnabled(bWanted);
	}
}

void UWeaponModelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float Rate = FElementVfxTuning::IdleRate(ModelElement) * FElementVfxTuning::Live().Idle;
	const AWeaponBase* Weapon = Cast<AWeaponBase>(GetOwner());
	// Only a gun in the hand as the player sees it, shown (a holstered one is hidden), with pieces to give.
	if (Rate <= 0.f || IdleSpots.IsEmpty() || FirstPersonType != EFirstPersonPrimitiveType::FirstPerson || !Weapon || Weapon->IsHidden() || !IsVisible())
	{
		IdleOwed = 0.f;
		return;
	}
	IdleOwed += Rate * FMath::Clamp(DeltaTime, 0.f, IdleMostStep);
	if (IdleOwed < 1.f)
	{
		return;
	}
	UWorld* World = GetWorld();
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		IdleOwed = 0.f;
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	const AActor* Holder = Weapon->GetOwner();
	const FVector Carry = Holder ? Holder->GetVelocity() : FVector::ZeroVector;
	while (IdleOwed >= 1.f)
	{
		IdleOwed -= 1.f;
		const UStaticMeshComponent* Spot = IdleSpots[FMath::RandRange(0, IdleSpots.Num() - 1)];
		if (!Spot || !Spot->IsVisible())
		{
			continue;
		}
		const FBoxSphereBounds& SpotBounds = Spot->Bounds;
		const FVector Point = SpotBounds.Origin + FVector(FMath::FRandRange(-1.f, 1.f) * SpotBounds.BoxExtent.X, FMath::FRandRange(-1.f, 1.f) * SpotBounds.BoxExtent.Y,
			FMath::FRandRange(-1.f, 1.f) * SpotBounds.BoxExtent.Z) * IdleSpread;
		Effects.SpawnElementIdle(ModelElement, Weapon->GetVisibleLocation(Point), Carry, ModelScale);
	}
}
