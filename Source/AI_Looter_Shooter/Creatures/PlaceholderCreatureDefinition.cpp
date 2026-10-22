#include "Creatures/PlaceholderCreatureDefinition.h"
#include "Creatures/PlaceholderRules.h"

FText UPlaceholderCreatureDefinition::GetTagName() const
{
	return PlaceholderRules::TagName(DisplayName, PlainType);
}

bool UPlaceholderCreatureDefinition::IsSound(TArray<FString>& OutProblems) const
{
	const int32 Before = OutProblems.Num();
	if (DisplayName.IsEmpty())
	{
		OutProblems.Add(TEXT("no name"));
	}
	if (PlainType.IsEmpty())
	{
		OutProblems.Add(TEXT("no plain type (the tag shows it in brackets)"));
	}
	if (MaxHealth <= 0.f || AttackDamage <= 0.f)
	{
		OutProblems.Add(TEXT("no health or no attack"));
	}
	if (ChaseSpeed < WalkSpeed)
	{
		OutProblems.Add(TEXT("chases slower than it walks"));
	}
	if (HeadSize > FMath::Max(Width, Height))
	{
		OutProblems.Add(TEXT("its head is bigger than its body"));
	}
	if (Role == EPlaceholderRole::Ranged && !(BackOffWithin < CloseInTo && CloseInTo <= ShotRange))
	{
		OutProblems.Add(TEXT("a ranged kind needs BackOffWithin < CloseInTo <= ShotRange"));
	}
	if (Role == EPlaceholderRole::Flyer && HoverHeight <= 0.f)
	{
		OutProblems.Add(TEXT("a flyer with no hover height"));
	}
	return OutProblems.Num() == Before;
}
