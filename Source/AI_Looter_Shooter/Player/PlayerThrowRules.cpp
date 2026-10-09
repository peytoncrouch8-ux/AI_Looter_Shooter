#include "Player/PlayerThrowRules.h"

int32 FThrowRules::Add(int32 Count, int32 Amount, int32* OutTaken)
{
	const int32 Before = FMath::Clamp(Count, 0, MaxGrenades);
	const int32 After = FMath::Clamp(Before + Amount, 0, MaxGrenades);
	if (OutTaken)
	{
		*OutTaken = After - Before;
	}
	return After;
}

FVector FThrowRules::LaunchVelocity(const FVector& Look, const FVector& ThrowerVelocity)
{
	FVector Way = Look.GetSafeNormal();
	if (Way.IsNearlyZero())
	{
		Way = FVector::ForwardVector;
	}
	// Lifted about the look's own right-hand axis, so the lob rises the same whichever way the player faces. Looking
	// straight up or down there is no right-hand axis: the look is thrown as it is.
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Way).GetSafeNormal();
	if (!Right.IsNearlyZero())
	{
		Way = Way.RotateAngleAxis(-LiftDegrees, Right).GetSafeNormal();
	}
	return Way * ThrowSpeed + ThrowerVelocity * InheritShare;
}

FVector FThrowRules::ReleasePoint(const FVector& Eye, const FVector& Look)
{
	const FVector Forward = Look.GetSafeNormal();
	FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	if (Right.IsNearlyZero())
	{
		Right = FVector::RightVector;
	}
	const FVector Up = FVector::CrossProduct(Forward, Right).GetSafeNormal();
	return Eye + Forward * ReleaseForward + Right * ReleaseRight + Up * ReleaseUp;
}

float FThrowRules::DropChance(ECreatureRank Rank, int32 Carried)
{
	if (Carried >= MaxGrenades)
	{
		return 0.f;
	}
	// Tougher kills pay better; a boss always leaves one. Out of grenades, the next one comes twice as soon: the panic
	// button should be there for the next panic.
	float Chance = 0.06f;
	switch (Rank)
	{
	case ECreatureRank::Rare: Chance = 0.15f; break;
	case ECreatureRank::Epic: Chance = 0.3f; break;
	case ECreatureRank::Legendary: Chance = 0.6f; break;
	case ECreatureRank::Boss: Chance = 1.f; break;
	default: break;
	}
	return FMath::Min(Carried <= 0 ? Chance * 2.f : Chance, 1.f);
}

float FThrowRules::ChestChance(bool bStrongbox, int32 Carried)
{
	if (Carried >= MaxGrenades)
	{
		return 0.f;
	}
	return bStrongbox || Carried <= 0 ? 1.f : 0.5f;
}

EThrowBlock FThrowRules::WhyBlocked(const FThrowGateInput& In)
{
	if (!In.bAlive)
	{
		return EThrowBlock::NoPlayer;
	}
	if (In.bInScene)
	{
		return EThrowBlock::Scene;
	}
	if (In.bMenuOpen)
	{
		return EThrowBlock::Menu;
	}
	// The hands are on the ledge.
	if (In.bTraversing)
	{
		return EThrowBlock::Traversing;
	}
	// The stock is swinging: one thing at a time, so the gun's pose and its held-back reload stay the strike's.
	if (In.bMeleeing)
	{
		return EThrowBlock::Busy;
	}
	if (In.bThrowing || In.Now - In.LastThrow < CooldownSeconds)
	{
		return EThrowBlock::Cooldown;
	}
	if (In.Grenades <= 0)
	{
		return EThrowBlock::Empty;
	}
	return EThrowBlock::None;
}
