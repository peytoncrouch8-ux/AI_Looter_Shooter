#include "Shields/ShieldText.h"
#include "Elements/ElementRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldTuning.h"
#include "UI/Style/LooterUIStyle.h"

FString ShieldText::BuildName(EShieldBuild Build)
{
	switch (Build)
	{
	case EShieldBuild::Bulwark: return TEXT("Bulwark");
	case EShieldBuild::Quickset: return TEXT("Quickset");
	default: return TEXT("Ward Box");
	}
}

FString ShieldText::TraitName(EShieldTrait Trait)
{
	switch (Trait)
	{
	case EShieldTrait::LastBreath: return TEXT("Last Breath");
	case EShieldTrait::Barbwire: return TEXT("Barbwire");
	case EShieldTrait::Lodestone: return TEXT("Lodestone");
	case EShieldTrait::Harvest: return TEXT("Harvest");
	case EShieldTrait::SaintsGrace: return TEXT("Saint's Grace");
	default: return FString();
	}
}

FString ShieldText::TraitWord(EShieldTrait Trait)
{
	switch (Trait)
	{
	case EShieldTrait::LastBreath: return TEXT("Gasping");
	case EShieldTrait::Barbwire: return TEXT("Barbed");
	case EShieldTrait::Lodestone: return TEXT("Lodestone");
	case EShieldTrait::Harvest: return TEXT("Reaping");
	case EShieldTrait::SaintsGrace: return TEXT("Graced");
	default: return FString();
	}
}

FString ShieldText::Name(const FShieldInstanceData& Ward)
{
	const FString Word = TraitWord(Ward.Trait);
	const FString Body = BuildName(Ward.Build);
	return Word.IsEmpty() ? Body : FString::Printf(TEXT("%s %s"), *Word, *Body);
}

FString ShieldText::TraitLine(const FShieldInstanceData& Ward)
{
	// The shipped numbers (a default-made tuning), so a card reads the same whatever is being tried in PIE.
	const FShieldTuning Tuning;
	const float Strength = ShieldRules::TraitStrength(Ward.Definition, Ward.Rarity);
	switch (Ward.Trait)
	{
	case EShieldTrait::LastBreath:
	{
		const float LevelScale = UPlayerProgressionSubsystem::GetLevelRules().EnemyScale(Ward.Level);
		return FString::Printf(TEXT("Last Breath: breaking looses %s round you for %d (once a full charge)."),
			*ElementRules::Word(Ward.Element), FMath::RoundToInt32(ShieldRules::NovaDamageAt(0.f, Strength, LevelScale, Tuning)));
	}
	case EShieldTrait::Barbwire:
		return FString::Printf(TEXT("Barbwire: a bite or claw on the ward cuts the biter for %d."),
			FMath::RoundToInt32(ShieldRules::BarbDamage(Ward.Stats.Capacity, Strength, Tuning)));
	case EShieldTrait::Lodestone:
		return FString::Printf(TEXT("Lodestone: %d%% of hits on the ward give rounds for the gun in hand."),
			FMath::RoundToInt32(ShieldRules::LodestoneOdds(Strength, Tuning) * 100.f));
	case EShieldTrait::Harvest:
		return FString::Printf(TEXT("Harvest: a kill puts back %d and starts the ward filling."),
			FMath::RoundToInt32(ShieldRules::HarvestRestore(Ward.Stats.Capacity, Strength, Tuning)));
	case EShieldTrait::SaintsGrace:
		return FString::Printf(TEXT("Saint's Grace: broken, with your health under a third, the ward flares back to %d (once a full charge)."),
			FMath::RoundToInt32(ShieldRules::GraceCharge(Ward.Stats.Capacity, Strength, Tuning)));
	default:
		return FString();
	}
}

FLinearColor ShieldText::Color(const FShieldInstanceData& Ward)
{
	return Ward.Definition ? Ward.Definition->GetRarityInfo(Ward.Rarity).Color : FLinearColor::White;
}

FLinearColor ShieldText::TraitColor(const FShieldInstanceData& Ward)
{
	return Ward.Trait == EShieldTrait::LastBreath && Ward.Element != ELooterElement::None ? ElementRules::UiColor(Ward.Element)
		: LooterUI::Color::CyanText();
}

FString ShieldText::CapacityString(const FShieldStats& Stats)
{
	return FString::FromInt(FMath::RoundToInt32(Stats.Capacity));
}

FString ShieldText::DelayString(const FShieldStats& Stats)
{
	return FString::Printf(TEXT("%.1f s"), Stats.RechargeDelay);
}

FString ShieldText::RateString(const FShieldStats& Stats)
{
	return FString::Printf(TEXT("%d / s"), FMath::RoundToInt32(Stats.RechargeRate));
}

FString ShieldText::KindLine(const FShieldInstanceData& Ward)
{
	const FString Kind = Ward.Definition ? Ward.Definition->DisplayName.ToString() : FString(TEXT("Ward Box"));
	return FString::Printf(TEXT("%s · Lv %d"), *Kind, Ward.Level);
}
