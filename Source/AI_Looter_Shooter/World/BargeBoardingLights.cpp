// ABargeBoarding: her lights by where the player is (the perf lane's finding, 2026-11-04: her point lights cost 1.65 ms at the
// deck): the deck's (the house's work lamp and the work lamps) while the player is on her deck or near, the hall's lanterns,
// the stoker's and the cage's only while the player is below her deck. Dying puts them all out (BeginDying); a reset shows
// the player's view again.

#include "World/BargeBoarding.h"
#include "World/BargeFurnace.h"
#include "World/BargeRules.h"
#include "Components/PointLightComponent.h"

int32 ABargeBoarding::LightViewAt(const FVector& OnHer)
{
	// Below: in her footprint, from under the hall's floor up to a hand under the cross walk's deck (the hatch's stair, the
	// hall); else on her deck or near it; else far.
	const bool bInHer = OnHer.X > BargeRules::HullAft && OnHer.X < BargeRules::HullFore && FMath::Abs(OnHer.Y) < BargeRules::HullHalfBeam;
	if (bInHer && OnHer.Z > BargeRules::HallFloor - 100.0 && OnHer.Z < BargeRules::HatchDeckZ() - BargeRules::BelowDeckUnder)
	{
		return 1;
	}
	return FVector::Dist(OnHer, BargeRules::DeckMiddle()) < BargeRules::LampsNear ? 0 : 2;
}

void ABargeBoarding::ShowLightsFor(int32 View)
{
	LightView = View;
	for (UPointLightComponent* Lamp : DeckLamps)
	{
		if (Lamp)
		{
			Lamp->SetVisibility(View == 0);
		}
	}
	for (UPointLightComponent* Lantern : Lanterns)
	{
		if (Lantern)
		{
			Lantern->SetVisibility(View == 1);
		}
	}
	if (Furnace)
	{
		Furnace->SetLightShown(View == 1);
	}
}
