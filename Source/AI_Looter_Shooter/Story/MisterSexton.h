#pragma once

#include "CoreMinimal.h"
#include "Story/StoryCharacter.h"
#include "MisterSexton.generated.h"

class UStaticMeshComponent;

/**
 * Mister Sexton on the lookout's railing (Docs/Areas/RansomsRest.md, Main 2 "Shall We Talk Business?"; Docs/Story.md,
 * "Antagonist: Mister Sexton"): the user's Gentleman, SM_MisterSexton seated on the rail with SM_SextonLedger on his knee
 * (his SOCKET_Ledger), a posed model with no rig (Art/Models/Characters/MisterSexton.py). The build script puts the actor
 * on the Lookout's SOCKET_Sit, facing into the deck (Tools/Unreal/build_area_story.py); his model's pivot is that seat.
 *
 * A story character, not a creature: shown by his story condition (during Main 2), talked to at his chin
 * (SOCKET_Speaker), where his captions come from; his topics hold the deal. A posed idle only: he never turns to whoever
 * talks to him and never breathes. His hull stops the player's body and finds the Interact key's line, but shots and
 * pellets pass through him: he can't be hurt, and a missed spider never ricochets off him.
 *
 * When the story is done with him he doesn't vanish under the player's eyes: while he's still talking or in view he
 * stays, and he's gone the next time nobody looks (he attends every death; he doesn't linger after one). In a checkout
 * without his model the story character's placeholder stands in.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AMisterSexton : public AStoryCharacter
{
	GENERATED_BODY()

public:
	AMisterSexton();

	/** The story's condition: shown, or hidden, but only once nobody is looking at him or listening to him. */
	virtual void RefreshShown() override;

	virtual void Tick(float DeltaSeconds) override;

	/** The story is done with him, and he waits for the player to look away (and his lines to end) to go. */
	bool IsLeaving() const { return bLeaving; }

	/** His ledger, on his right knee. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Ledger;

	/** Drawn on a screen this lately (seconds), he still counts as seen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sexton", meta = (ClampMin = "0"))
	float SeenSeconds = 0.5f;

	/** His model and its sockets (Art/Models/Characters/MisterSexton.py, Buildings/Lookout.py). */
	static const TCHAR* const ModelPath;
	static const TCHAR* const LedgerPath;
	static const FName LedgerSocket;
	static const FName SpeakerSocket;

protected:
	virtual void BeginPlay() override;

private:
	/** Someone could see him or is listening to him: drawn on a screen lately, or his lines still on screen or to come. */
	bool IsWatched() const;

	bool bLeaving = false;
	/** His play has begun: as the level begins he's simply where the story has him, watched or not. */
	bool bBegun = false;
};
