#include "Weapons/WeaponMagazineDrop.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Player/FirstPersonHandsFraming.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"

// The thrown magazine's plain rules (MagazineDrop, WeaponMagazineDrop.h): which reloads throw one and when, its arc,
// bounce, pool slot, rest and sink, and the launch a gun gives it as its old magazine clears the well. The pool that flies
// and draws them is WeaponMagazineDrop.cpp.

namespace
{
	/** The support hand flicks the old one off the gun's left as it lets go (cm/s), and a little every way. */
	constexpr float DropFlickMin = 25.f;
	constexpr float DropFlickMax = 55.f;
	constexpr float DropWobble = 15.f;
	/** It tumbles end over end off the gun's side (radians a second about the gun's length), with a wobble. */
	constexpr float DropTumbleMin = 3.f;
	constexpr float DropTumbleMax = 6.f;
	constexpr float DropTumbleWobble = 1.5f;
	/** The slide's speed is read over this much of the choreography before it clears the well. */
	constexpr float DropSlideSample = 0.005f;
}

bool MagazineDrop::Drops(EWeaponReloadPart Part)
{
	switch (Part)
	{
	case EWeaponReloadPart::Magazine:
	case EWeaponReloadPart::Helix:
	case EWeaponReloadPart::Box:
	case EWeaponReloadPart::Bolt:
		return true;
	default:
		// A pump's shells, a cylinder's empties (FWeaponBrass), a rocket fired: nothing to throw.
		return false;
	}
}

float MagazineDrop::DropAt(EWeaponReloadPart Part)
{
	if (!Drops(Part))
	{
		return -1.f;
	}
	// The sniper's box comes out a little later (its bolt is opened first: LooterReload::WellTravel).
	return LooterReload::MagazineClearAt + (Part == EWeaponReloadPart::Bolt ? LooterReload::BoltMagazineLag : 0.f);
}

bool MagazineDrop::Hides(EWeaponReloadPart Part, float Progress)
{
	const float From = DropAt(Part);
	if (From < 0.f || Progress < From)
	{
		return false;
	}
	const float FreshAt = LooterReload::FreshMagazineAt + (Part == EWeaponReloadPart::Bolt ? LooterReload::BoltMagazineLag : 0.f);
	return Progress < FreshAt;
}

FVector MagazineDrop::Arc(const FVector& Start, const FVector& Velocity, float Seconds)
{
	return Start + Velocity * Seconds + FVector(0.0, 0.0, -0.5 * Gravity * Seconds * Seconds);
}

FVector MagazineDrop::Bounce(const FVector& Velocity, const FVector& Normal, bool bSoft)
{
	// A box of rounds: it gives back little of what went into it (a dull knock in dirt, a short clatter off stone) and keeps
	// some of its slide along the face.
	const FVector Face = Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const float Restitution = bSoft ? 0.1f : 0.28f;
	const float Keep = bSoft ? 0.25f : 0.5f;
	const double Into = FVector::DotProduct(Velocity, Face);
	const FVector Across = Velocity - Face * Into;
	return Across * Keep - Face * (FMath::Min(Into, 0.0) * Restitution);
}

int32 MagazineDrop::PickSlot(TConstArrayView<float> Ages)
{
	int32 Oldest = INDEX_NONE;
	for (int32 Slot = 0; Slot < Ages.Num(); ++Slot)
	{
		if (Ages[Slot] < 0.f)
		{
			return Slot;
		}
		if (Oldest == INDEX_NONE || Ages[Slot] > Ages[Oldest])
		{
			Oldest = Slot;
		}
	}
	return Oldest;
}

float MagazineDrop::SinkShare(float RestAge, float Rest)
{
	return FMath::Clamp((RestAge - Rest) / SinkSeconds, 0.f, 1.f);
}

FQuat MagazineDrop::RestRotation(const FQuat& Now, const FVector& Normal, const FVector& Extent)
{
	// Its thinnest axis turned onto the ground's normal (whichever way it faces it now): it lies on its side, as a magazine
	// does, its length along the ground the way it fell.
	const FVector Ground = Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FVector Size = Extent.GetAbs();
	FVector Axis = Size.X <= Size.Y && Size.X <= Size.Z ? Now.GetAxisX() : (Size.Y <= Size.Z ? Now.GetAxisY() : Now.GetAxisZ());
	if (FVector::DotProduct(Axis, Ground) < 0.0)
	{
		Axis = -Axis;
	}
	return (FQuat::FindBetweenNormals(Axis, Ground) * Now).GetNormalized();
}

float MagazineDrop::RestLift(const FVector& Extent)
{
	return static_cast<float>(Extent.GetAbs().GetMin());
}

UStaticMeshComponent* MagazineDrop::FindReloadPart(const UWeaponModelComponent& Model, const UWeaponDefinition& Definition)
{
	if (Definition.ReloadPart == EWeaponReloadPart::None || Definition.ReloadSlot.IsNone())
	{
		return nullptr;
	}
	const FWeaponPartSlot* Slot = Definition.Parts.FindByPredicate([&Definition](const FWeaponPartSlot& Each) { return Each.Name == Definition.ReloadSlot; });
	// The reload part rests on its slot's socket (UWeaponModelComponent::SetReloadTravel moves it off along its way); the
	// model hangs it from the latest earlier part that has that socket, and no other slot uses it.
	if (!Slot || Slot->Socket.IsNone())
	{
		return nullptr;
	}
	const TArray<TObjectPtr<UStaticMeshComponent>>& Parts = Model.GetParts();
	for (int32 Index = Parts.Num() - 1; Index >= 0; --Index)
	{
		UStaticMeshComponent* Part = Parts[Index];
		if (Part && Part->GetAttachSocketName() == Slot->Socket && Part->GetAttachParent() != &Model)
		{
			return Part->GetStaticMesh() ? Part : nullptr;
		}
	}
	return nullptr;
}

bool MagazineDrop::Throws(const AWeaponBase& Gun, const UWeaponModelComponent& Model)
{
	const UWeaponDefinition* Definition = Gun.GetInstance().Definition;
	const UWorld* World = Gun.GetWorld();
	return ReloadTuning::DropMagOn() && Definition && World && Drops(Model.GetReloadPart()) && World->GetSubsystem<UWeaponMagazineDrop>()
		&& FindReloadPart(Model, *Definition);
}

FMagazineDropLaunch MagazineDrop::LaunchFrom(const AWeaponBase& Gun, const UStaticMeshComponent& Part, bool bFirstPerson, float MotionSeconds)
{
	FMagazineDropLaunch Launch;
	Launch.Mesh = Part.GetStaticMesh();
	for (int32 Index = 0; Index < Part.GetNumMaterials(); ++Index)
	{
		Launch.Materials.Add(Part.GetMaterial(Index));
	}
	Launch.CustomData = Part.GetCustomPrimitiveData().Data;
	const FTransform World = Part.GetComponentTransform();
	Launch.Transform = World;
	Launch.WorldScale = World.GetScale3D();
	const UWeaponDefinition* Definition = Gun.GetInstance().Definition;
	if (bFirstPerson)
	{
		// It leaves the gun the player sees: where the part shows (drawn with the view model's field of view), as big as it
		// shows there (the renderer widens a first-person part across the view by the field of view's correction and pulls
		// it in by the first-person scale), and then its own size (the first-person gun is drawn FPHandsFraming::GunScale bigger).
		Launch.Transform.SetLocation(Gun.GetVisibleLocation(World.GetLocation()));
		float Shown = 1.f;
		if (const UCameraComponent* Camera = UPlayerViewComponent::FindFirstPersonCamera(Gun.GetOwner()))
		{
			FMinimalViewInfo View;
			View.FOV = Camera->FieldOfView;
			View.FirstPersonFOV = Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : Camera->FieldOfView;
			Shown = (Camera->bEnableFirstPersonFieldOfView ? View.CalculateFirstPersonFOVCorrectionFactor() : 1.f)
				* (Camera->bEnableFirstPersonScale ? Camera->FirstPersonScale : 1.f);
		}
		Launch.Transform.SetScale3D(World.GetScale3D() * Shown);
		Launch.WorldScale = World.GetScale3D() / FPHandsFraming::GunScale(Definition ? Definition->Kind : EWeaponKind::None);
	}

	// Moving as its slide out of the well had it as it cleared (the choreography's travel over the reload's seconds), out
	// along its way (the part's -Z), carried on by the holder's run, flicked off the gun's left by the letting hand.
	const EWeaponReloadPart ReloadPart = Gun.GetReloadPart();
	const FReloadPlan& Plan = Gun.GetReloadPlan();
	const float At = DropAt(ReloadPart);
	bool bShown = true;
	const float Slid = LooterReload::WellTravel(ReloadPart, At, bShown) - LooterReload::WellTravel(ReloadPart, At - DropSlideSample, bShown);
	const float RawSlid = LooterReload::RawProgressOf(ReloadPart, Plan, At) - LooterReload::RawProgressOf(ReloadPart, Plan, At - DropSlideSample);
	const float Speed = MotionSeconds > 0.f && RawSlid > 0.f ? Slid / (RawSlid * MotionSeconds) : 0.f;
	const FVector Out = -World.GetUnitAxis(EAxis::Z);
	const FVector Left = -World.GetUnitAxis(EAxis::Y);
	const AActor* Holder = Gun.GetOwner();
	Launch.Velocity = Out * Speed + Left * FMath::FRandRange(DropFlickMin, DropFlickMax) + FMath::VRand() * DropWobble
		+ (Holder ? Holder->GetVelocity() : FVector::ZeroVector);
	Launch.Spin = World.GetUnitAxis(EAxis::X) * FMath::FRandRange(DropTumbleMin, DropTumbleMax) + FMath::VRand() * DropTumbleWobble;
	Launch.Channel = Gun.TraceChannel;
	Launch.Holder = Holder;
	Launch.Gun = &Gun;
	return Launch;
}
