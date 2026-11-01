// USecretCatalog: a level's secrets for sale, and what its data says for a point in the story.

#include "Economy/SecretCatalog.h"
#include "Session/CampaignRecord.h"

const FSecretOffer* USecretCatalog::FindOffer(FName Id) const
{
	return Id.IsNone() ? nullptr : Secrets.FindByPredicate([Id](const FSecretOffer& Each) { return Each.Id == Id; });
}

bool USecretCatalog::IsOpen(const FCampaignRecord& Record, const UMissionRunner* Runner) const
{
	return OpenWhen.IsEmpty() || OpenWhen.IsMet(Record, Runner);
}

float USecretCatalog::DiscountFor(const FCampaignRecord& Record) const
{
	return !DiscountMission.IsNone() && Record.HasCompleted(DiscountMission) ? FMath::Clamp(Discount, 0.f, 1.f) : 0.f;
}

FText USecretCatalog::GreetingFor(const FCampaignRecord& Record, const UMissionRunner* Runner) const
{
	for (const FSecretGreeting& Greeting : Greetings)
	{
		if (Greeting.When.IsEmpty() || Greeting.When.IsMet(Record, Runner))
		{
			return Greeting.Line;
		}
	}
	return FText::GetEmpty();
}
