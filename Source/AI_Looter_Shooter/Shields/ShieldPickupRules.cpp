#include "Shields/ShieldPickupRules.h"
#include "Shields/PlayerShieldComponent.h"

ShieldPickupRules::ETap ShieldPickupRules::TapFor(bool bWardWorn, bool bBackpackRoom)
{
	if (!bWardWorn)
	{
		return ETap::Wear;
	}
	return bBackpackRoom ? ETap::ToBackpack : ETap::Swap;
}

ShieldPickupRules::ETap ShieldPickupRules::TapFor(const UPlayerShieldComponent* Shield)
{
	return Shield ? TapFor(Shield->HasWard(), Shield->HasBackpackRoom()) : ETap::Wear;
}
