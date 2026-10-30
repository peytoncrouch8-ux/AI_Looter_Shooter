#include "Weapons/LegendaryRules.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponTypes.h"

namespace
{
	/** "1", "0.5", "1.5": a number for a card line without trailing zeros. */
	FString Short(float Value)
	{
		return FString::SanitizeFloat(FMath::RoundToFloat(Value * 10.f) / 10.f, 0);
	}

	int32 Percent(float Share)
	{
		return FMath::RoundToInt(Share * 100.f);
	}
}

ELegendarySignature LegendaryRules::SignatureOf(const FWeaponInstanceData& Gun)
{
	const UNamedWeaponDefinition* Named = Gun.Named;
	return Named ? Named->Signature : ELegendarySignature::None;
}

FString LegendaryRules::Description(ELegendarySignature Signature, const FLegendaryTuning& Tuning)
{
	// Borderlands' red text says it in a breath; these say what it does with the numbers it does it with.
	switch (Signature)
	{
	case ELegendarySignature::Gravesilk:
		return FString::Printf(TEXT("Hits lay silk: %d stacks root the target for %s s (never a boss)"), Tuning.GravesilkStacks,
			*Short(Tuning.GravesilkRootSeconds));
	case ELegendarySignature::CompanyScrip:
		return FString::Printf(TEXT("Each kill in one burst: +%d%% fire rate (up to %d). A reload pays back %d%% of the magazine"),
			Percent(Tuning.CompanyScripFireRatePerStack), Tuning.CompanyScripMaxStacks, Percent(Tuning.CompanyScripRefundShare));
	case ELegendarySignature::Lamplighter:
		return FString::Printf(TEXT("Scoped kills leave a lantern: %d%% of your health a second within %s m for %s s"),
			Percent(Tuning.LamplighterHealShare), *Short(Tuning.LamplighterRadius / 100.f), *Short(Tuning.LamplighterSeconds));
	default:
		return FString();
	}
}

float LegendaryRules::DropChance(ELegendarySignature Signature, const FLegendaryTuning& Tuning)
{
	switch (Signature)
	{
	case ELegendarySignature::Gravesilk:
		return Tuning.GravesilkDropChance;
	case ELegendarySignature::CompanyScrip:
		return Tuning.CompanyScripDropChance;
	case ELegendarySignature::Lamplighter:
		return Tuning.LamplighterDropChance;
	default:
		return 0.f;
	}
}

LegendaryRules::ESilk LegendaryRules::LaySilk(FSilk& Silk, double Now, bool bBoss, const FLegendaryTuning& Tuning)
{
	// A boss never: nothing new holds up a boss's moves. Rooted, and the gap after, nothing takes either: steady fire roots
	// a creature now and then, never for good.
	if (bBoss || Now < Silk.RootEnds + Tuning.GravesilkRootGap)
	{
		return ESilk::None;
	}
	Silk.Stacks = StacksAt(Silk, Now, Tuning) + 1;
	Silk.LastLaid = Now;
	if (Silk.Stacks < FMath::Max(Tuning.GravesilkStacks, 1))
	{
		return ESilk::Laid;
	}
	Silk.Stacks = 0;
	Silk.RootEnds = Now + Tuning.GravesilkRootSeconds;
	return ESilk::Rooted;
}

int32 LegendaryRules::StacksAt(const FSilk& Silk, double Now, const FLegendaryTuning& Tuning)
{
	return Now - Silk.LastLaid <= Tuning.GravesilkSilkSeconds ? Silk.Stacks : 0;
}

float LegendaryRules::ScripFireRateScale(int32 Stacks, const FLegendaryTuning& Tuning)
{
	return 1.f + Tuning.CompanyScripFireRatePerStack * FMath::Clamp(Stacks, 0, Tuning.CompanyScripMaxStacks);
}

int32 LegendaryRules::ScripAddKill(int32 Stacks, const FLegendaryTuning& Tuning)
{
	return FMath::Clamp(Stacks + 1, 0, Tuning.CompanyScripMaxStacks);
}

int32 LegendaryRules::ScripRefund(int32 RoundsTaken, int32 MagazineSize, const FLegendaryTuning& Tuning)
{
	// Back to the pool, never more than the reload took: a reload with an empty pool pays nothing, so it's never free ammo.
	const int32 Share = FMath::RoundToInt(FMath::Max(MagazineSize, 0) * Tuning.CompanyScripRefundShare);
	return FMath::Clamp(Share, 0, FMath::Max(RoundsTaken, 0));
}

float LegendaryRules::LanternHeal(float MaxHealth, float DeltaSeconds, const FLegendaryTuning& Tuning)
{
	return FMath::Max(MaxHealth, 0.f) * Tuning.LamplighterHealShare * FMath::Max(DeltaSeconds, 0.f);
}

bool LegendaryRules::InLanternReach(const FVector& Lantern, const FVector& Body, float Radius)
{
	return FVector::DistSquared(Lantern, Body) <= FMath::Square(static_cast<double>(FMath::Max(Radius, 0.f)));
}

float LegendaryRules::LanternGlow(float Age, float Life)
{
	if (Age < 0.f || Age >= Life)
	{
		return 0.f;
	}
	const float In = FMath::Clamp(Age / 0.2f, 0.f, 1.f);
	const float Out = FMath::Clamp((Life - Age) / 0.5f, 0.f, 1.f);
	return FMath::Min(In, Out);
}
