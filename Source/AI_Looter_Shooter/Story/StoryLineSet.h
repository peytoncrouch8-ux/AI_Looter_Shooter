#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Story/StoryLine.h"
#include "StoryLineSet.generated.h"

/**
 * Lines said together, as a data asset in /Game/Data/Story (DA_Lines_<Name>): what a speaker point says, in order. Lines
 * that name no speaker take the speaker point's. Only data: who plays them and when is the speaker point's (or a scene's).
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UStoryLineSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Played in this order, one after another. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lines")
	TArray<FStoryLine> Lines;
};
