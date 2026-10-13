#pragma once

#include "CoreMinimal.h"
#include "Story/DeadFriend.h"
#include "Story/HiramPoses.h"
#include "HiramCoyle.generated.h"

class USpeakerPointComponent;

/**
 * Hiram Coyle, the ore lift's dead engineman (Docs/Areas/RansomsRest_Expansion.md, Dead Lift: "ran its winch for twenty years
 * and died at his brake. He hasn't left."): a friendly dead man in the winch house at the Old Quarry, on his stool by the
 * cold boiler, a giver (Speaker_Hiram: Dead Lift's offer and his lines through the story). The build script stands him at
 * his stool (his origin at the stool's foot, facing out from the boiler) and says which side the speaking tube is
 * (TubeSide). There from Main 5's finish, when Dead Lift opens (ShownWhen).
 *
 * He sits (his idle: SK_Hiram's rest pose, on a 56 cm stool), now and then leaning to the speaking tube to listen for the
 * lift head (every half a minute or so, a few seconds; HiramPoses::AtTube, to the side it's on), and leans to it while
 * anyone talks through the tube at the lift head (the speaker point tagged Speaker_LiftTube: Dead Lift's turn-in comes up it
 * in his voice). Talked to, he turns his head to the listener. His body is SK_Hiram (the dead miners' costume set), else
 * SK_Unpaid in a miner's tint until it's imported; his cap SM_HiramCap on the hat bone (else the dead miners' hat).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AHiramCoyle : public ADeadFriend
{
	GENERATED_BODY()

public:
	/** The tag the missions and the console find him by; the lift head's tube he answers through. */
	static const FName SpeakerTag;
	static const FName TubeTag;

	static const TCHAR* const ModelPath;
	static const TCHAR* const CapPath;

	AHiramCoyle();

	/** Leaning to the tube now (listening, or answering it). */
	bool IsAtTube() const { return bAtTube; }

	/** Leans to the tube for Seconds (the console, the tests), whatever his idle says. */
	void LeanToTube(float Seconds);

	/** Which side of him the speaking tube is: +1 his right, -1 his left (the build script sets it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hiram")
	float TubeSide = 1.f;

	/** He leans to the tube to listen every this many seconds or so, and stays this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hiram", meta = (ClampMin = "5", Units = "s"))
	float ListenEvery = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hiram", meta = (ClampMin = "1", Units = "s"))
	float ListenSeconds = 5.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual FDeadFriendPose WantedPose(float Time) const override;
	virtual void TickFriend(float DeltaSeconds) override;

private:
	void HandleTubeTalked(USpeakerPointComponent& Point, AActor* InListener);

	bool bAtTube = false;
	float TubeLeft = 0.f;
	float NextListen = 0.f;
	TWeakObjectPtr<USpeakerPointComponent> Tube;
};
