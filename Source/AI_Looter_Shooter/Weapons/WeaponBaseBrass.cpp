#include "Weapons/WeaponBase.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponBrass.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Engine/World.h"

// AWeaponBase's spent brass (FWeaponBrass draws and lands it): a rifle's case flicked out of the port each shot, a
// shotgun's hull as it's pumped, a revolver's empties tipped out on a reload's eject.

namespace
{
	/**
	 * Where a gun's port is when no part marks it with SOCKET_Eject, in its own space (cm: X along the bore from the back
	 * of the receiver, Y to its right, Z up): the bullpup's, just behind the grip on its right; the pump's, on the
	 * receiver's right. A revolver has none.
	 */
	const FVector BrassRiflePort(5.f, 2.6f, 0.4f);
	const FVector BrassShotgunPort(10.f, 1.6f, 0.2f);

	/** Which way the cases are flung, in the gun's space: out to its right, up, a little back. */
	const FVector BrassRifleOut(-0.2f, 1.f, 0.5f);
	const FVector BrassHullOut(-0.1f, 1.f, 0.65f);

	/** How hard (cm/s) and how fast they tumble (radians a second). */
	constexpr float BrassRifleMinSpeed = 260.f;
	constexpr float BrassRifleMaxSpeed = 360.f;
	constexpr float BrassHullMinSpeed = 200.f;
	constexpr float BrassHullMaxSpeed = 280.f;
	constexpr float BrassRifleMinSpin = 22.f;
	constexpr float BrassRifleMaxSpin = 38.f;
	constexpr float BrassHullMinSpin = 10.f;
	constexpr float BrassHullMaxSpin = 18.f;

	/** The pump's stroke after a shot: this share of the time between shots, within these bounds (seconds). */
	constexpr float BrassHullShare = 0.35f;
	constexpr float BrassHullSoonest = 0.15f;
	constexpr float BrassHullLatest = 0.35f;

	/** Empties sliding out of the chambers' backs (cm/s), tumbling lazily. */
	constexpr float BrassEmptyMinSpeed = 60.f;
	constexpr float BrassEmptyMaxSpeed = 140.f;

	FVector BrassHolderVelocity(const AActor* Holder)
	{
		return Holder ? Holder->GetVelocity() : FVector::ZeroVector;
	}

	UBulletSubsystem* BrassSubsystem(const UWorld* World)
	{
		return World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	}
}

bool AWeaponBase::GetEjectPort(FVector& OutLocation, FVector& OutDirection) const
{
	if (!bUsingModel || !Instance.Definition)
	{
		return false;
	}
	const EWeaponKind Kind = Instance.Definition->Kind;
	const ECasingKind Casing = FWeaponBrass::KindFor(Kind);
	if (Casing == ECasingKind::Pistol)
	{
		return false;
	}
	FVector Port;
	if (!Model->GetEject(Port))
	{
		Port = Casing == ECasingKind::Hull ? BrassShotgunPort : BrassRiflePort;
	}
	// The visible gun's port (in first person the gun is drawn with its own field of view): the cases come out of the gun
	// the player sees.
	const FTransform& Gun = Model->GetComponentTransform();
	OutLocation = GetVisibleLocation(Gun.TransformPosition(Port));
	OutDirection = Gun.TransformVectorNoScale(Casing == ECasingKind::Hull ? BrassHullOut : BrassRifleOut).GetSafeNormal();
	return true;
}

float AWeaponBase::GetHullDelay() const
{
	return FMath::Clamp(Instance.Stats.GetSecondsBetweenShots() * BrassHullShare, BrassHullSoonest, BrassHullLatest);
}

void AWeaponBase::EjectAfterShot()
{
	if (!Instance.Definition)
	{
		return;
	}
	switch (FWeaponBrass::KindFor(Instance.Definition->Kind))
	{
	case ECasingKind::Rifle:
		EjectCasing();
		break;
	case ECasingKind::Hull:
		// The hull waits for the pump (a shot fired before it's out pumps the last one out first).
		if (PendingEjectSeconds >= 0.f)
		{
			EjectCasing();
		}
		PendingEjectSeconds = GetHullDelay();
		RefreshTick();
		break;
	case ECasingKind::Pistol:
	default:
		// A six-gun keeps its empties until a reload.
		break;
	}
}

void AWeaponBase::UpdateBrass(float DeltaSeconds)
{
	if (PendingEjectSeconds < 0.f)
	{
		return;
	}
	PendingEjectSeconds -= DeltaSeconds;
	if (PendingEjectSeconds < 0.f)
	{
		PendingEjectSeconds = -1.f;
		EjectCasing();
	}
}

void AWeaponBase::EjectCasing()
{
	UBulletSubsystem* Bullets = BrassSubsystem(GetWorld());
	FVector Port;
	FVector Out;
	if (!Bullets || !GetEjectPort(Port, Out))
	{
		return;
	}
	const ECasingKind Kind = FWeaponBrass::KindFor(Instance.Definition->Kind);
	const bool bHull = Kind == ECasingKind::Hull;
	const FTransform& Gun = Model->GetComponentTransform();
	const FVector Along = Gun.GetUnitAxis(EAxis::X);
	const FVector Up = Gun.GetUnitAxis(EAxis::Z);

	FCasingLaunch Launch;
	Launch.Kind = Kind;
	Launch.Location = Port;
	// Flung clear, carried on by the holder's own motion (a case thrown while running keeps up for a moment).
	Launch.Velocity = Out * FMath::FRandRange(bHull ? BrassHullMinSpeed : BrassRifleMinSpeed, bHull ? BrassHullMaxSpeed : BrassRifleMaxSpeed)
		+ FMath::VRand() * 20.f + BrassHolderVelocity(GetOwner());
	// It leaves lying along the bore, mouth forward, and tumbles end over end about the gun's up, with a wobble.
	Launch.Rotation = FRotationMatrix::MakeFromZX(Along, Up).ToQuat();
	Launch.Spin = (Up + FMath::VRand() * 0.35f).GetSafeNormal()
		* FMath::FRandRange(bHull ? BrassHullMinSpin : BrassRifleMinSpin, bHull ? BrassHullMaxSpin : BrassRifleMaxSpin);
	Launch.Channel = TraceChannel;
	Launch.Holder = GetOwner();
	Launch.Gun = this;
	Bullets->EjectCasing(Launch);
}

void AWeaponBase::DumpEmpties()
{
	const int32 Count = SpentInCylinder;
	SpentInCylinder = 0;
	UBulletSubsystem* Bullets = BrassSubsystem(GetWorld());
	TArray<FVector> Chambers;
	FVector Back;
	if (Count <= 0 || !Bullets || !bUsingModel || !Model->GetChamberRears(Chambers, Back))
	{
		return;
	}
	// The muzzle is tipped up for the punch: the empties slide out of the chambers' backs and drop, tumbling lazily.
	for (int32 Index = 0; Index < Count && Index < Chambers.Num(); ++Index)
	{
		FCasingLaunch Launch;
		Launch.Kind = ECasingKind::Pistol;
		Launch.Location = GetVisibleLocation(Chambers[Index]);
		Launch.Velocity = Back * FMath::FRandRange(BrassEmptyMinSpeed, BrassEmptyMaxSpeed) + FMath::VRand() * 25.f + BrassHolderVelocity(GetOwner());
		Launch.Rotation = FRotationMatrix::MakeFromZ(-Back).ToQuat();
		Launch.Spin = FMath::VRand() * FMath::FRandRange(3.f, 9.f);
		Launch.Channel = TraceChannel;
		Launch.Holder = GetOwner();
		Launch.Gun = this;
		Bullets->EjectCasing(Launch);
	}
}
