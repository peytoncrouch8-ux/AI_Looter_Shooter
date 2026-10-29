#include "Weapons/WeaponBase.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponBrass.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponModelComponent.h"
#include "Engine/World.h"

// AWeaponBase's spent brass (FWeaponBrass draws and lands it): a rifle's case flicked out of the port each shot, a
// shotgun's hull as it's pumped (its rack: WeaponBasePump.cpp), a revolver's empties tipped out on a reload's eject.
// Gun identity (Looter.Gunfeel.Identity 1; FWeaponBrass::KindFor): the SMG throws a small fast case, the sniper a fat one
// lazily as its bolt is drawn, the LMG a belt case down and to the right with a link falling after it.

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
	/** The LMG's cases and links drop out of the belt feed: down and to the right. */
	const FVector BrassBeltOut(-0.1f, 1.f, -0.5f);

	/** How hard (cm/s) and how fast they tumble (radians a second). */
	constexpr float BrassRifleMinSpeed = 260.f;
	constexpr float BrassRifleMaxSpeed = 360.f;
	constexpr float BrassHullMinSpeed = 200.f;
	constexpr float BrassHullMaxSpeed = 280.f;
	constexpr float BrassRifleMinSpin = 22.f;
	constexpr float BrassRifleMaxSpin = 38.f;
	constexpr float BrassHullMinSpin = 10.f;
	constexpr float BrassHullMaxSpin = 18.f;

	/** The identity kinds: the SMG's small case is flung hard, the sniper's magnum drawn out lazily, the LMG's belt case and link dropped. */
	constexpr float BrassSmallMinSpeed = 300.f;
	constexpr float BrassSmallMaxSpeed = 420.f;
	constexpr float BrassSmallMinSpin = 30.f;
	constexpr float BrassSmallMaxSpin = 48.f;
	constexpr float BrassMagnumMinSpeed = 110.f;
	constexpr float BrassMagnumMaxSpeed = 170.f;
	constexpr float BrassMagnumMinSpin = 8.f;
	constexpr float BrassMagnumMaxSpin = 14.f;
	constexpr float BrassBeltMinSpeed = 160.f;
	constexpr float BrassBeltMaxSpeed = 240.f;
	constexpr float BrassBeltMinSpin = 14.f;
	constexpr float BrassBeltMaxSpin = 26.f;
	constexpr float BrassLinkMinSpeed = 80.f;
	constexpr float BrassLinkMaxSpeed = 140.f;
	constexpr float BrassLinkMinSpin = 20.f;
	constexpr float BrassLinkMaxSpin = 40.f;

	/** How hard (cm/s) and how fast (radians a second) a case of Kind is thrown. */
	struct FBrassThrow
	{
		float MinSpeed;
		float MaxSpeed;
		float MinSpin;
		float MaxSpin;
	};

	FBrassThrow BrassThrowOf(ECasingKind Kind)
	{
		switch (Kind)
		{
		case ECasingKind::Hull:
			return { BrassHullMinSpeed, BrassHullMaxSpeed, BrassHullMinSpin, BrassHullMaxSpin };
		case ECasingKind::Small:
			return { BrassSmallMinSpeed, BrassSmallMaxSpeed, BrassSmallMinSpin, BrassSmallMaxSpin };
		case ECasingKind::Magnum:
			return { BrassMagnumMinSpeed, BrassMagnumMaxSpeed, BrassMagnumMinSpin, BrassMagnumMaxSpin };
		case ECasingKind::Belt:
			return { BrassBeltMinSpeed, BrassBeltMaxSpeed, BrassBeltMinSpin, BrassBeltMaxSpin };
		case ECasingKind::Link:
			return { BrassLinkMinSpeed, BrassLinkMaxSpeed, BrassLinkMinSpin, BrassLinkMaxSpin };
		default:
			return { BrassRifleMinSpeed, BrassRifleMaxSpeed, BrassRifleMinSpin, BrassRifleMaxSpin };
		}
	}

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
		// The fallbacks are the modeled guns' ports: at the size the gun is drawn (UWeaponModelComponent::ScaleFor).
		Port = (Casing == ECasingKind::Hull ? BrassShotgunPort : BrassRiflePort) * Model->GetModelScale();
	}
	// The visible gun's port (in first person the gun is drawn with its own field of view): the cases come out of the gun
	// the player sees.
	const FTransform& Gun = Model->GetComponentTransform();
	OutLocation = GetVisibleLocation(Gun.TransformPosition(Port));
	const FVector& Out = Casing == ECasingKind::Hull ? BrassHullOut : (Casing == ECasingKind::Belt ? BrassBeltOut : BrassRifleOut);
	OutDirection = Gun.TransformVectorNoScale(Out).GetSafeNormal();
	return true;
}

void AWeaponBase::EjectAfterShot()
{
	if (!Instance.Definition)
	{
		return;
	}
	// A bolt gun's case comes out as its bolt is drawn back (UpdateBoltCycle); a rocket leaves no case.
	const EWeaponKind Kind = Instance.Definition->Kind;
	if (Kind == EWeaponKind::Sniper || Kind == EWeaponKind::Launcher)
	{
		return;
	}
	switch (FWeaponBrass::KindFor(Kind))
	{
	case ECasingKind::Rifle:
	case ECasingKind::Small:
	case ECasingKind::Belt:
	case ECasingKind::Magnum:
		EjectCasing();
		break;
	case ECasingKind::Hull:
		// The hull waits for the pump: it flies as the rack bottoms (WeaponBasePump.cpp).
		StartPumpCycle();
		break;
	case ECasingKind::Pistol:
	default:
		// A six-gun keeps its empties until a reload.
		break;
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
	const FBrassThrow Throw = BrassThrowOf(Kind);
	const FTransform& Gun = Model->GetComponentTransform();
	const FVector Along = Gun.GetUnitAxis(EAxis::X);
	const FVector Up = Gun.GetUnitAxis(EAxis::Z);

	FCasingLaunch Launch;
	Launch.Kind = Kind;
	Launch.Location = Port;
	// Flung clear, carried on by the holder's own motion (a case thrown while running keeps up for a moment).
	Launch.Velocity = Out * FMath::FRandRange(Throw.MinSpeed, Throw.MaxSpeed) + FMath::VRand() * 20.f + BrassHolderVelocity(GetOwner());
	// It leaves lying along the bore, mouth forward, and tumbles end over end about the gun's up, with a wobble.
	Launch.Rotation = FRotationMatrix::MakeFromZX(Along, Up).ToQuat();
	Launch.Spin = (Up + FMath::VRand() * 0.35f).GetSafeNormal() * FMath::FRandRange(Throw.MinSpin, Throw.MaxSpin);
	Launch.Channel = TraceChannel;
	Launch.Holder = GetOwner();
	Launch.Gun = this;
	Bullets->EjectCasing(Launch);

	// The LMG's belt link falls with every round's case: a tab of dark brass dropped from the feed, tumbling every way.
	if (Kind == ECasingKind::Belt)
	{
		const FBrassThrow LinkThrow = BrassThrowOf(ECasingKind::Link);
		FCasingLaunch Link = Launch;
		Link.Kind = ECasingKind::Link;
		Link.Velocity = Out * FMath::FRandRange(LinkThrow.MinSpeed, LinkThrow.MaxSpeed) + FMath::VRand() * 30.f
			+ BrassHolderVelocity(GetOwner());
		Link.Rotation = FRotationMatrix::MakeFromZ(FMath::VRand()).ToQuat();
		Link.Spin = FMath::VRand() * FMath::FRandRange(LinkThrow.MinSpin, LinkThrow.MaxSpin);
		Bullets->EjectCasing(Link);
	}
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
