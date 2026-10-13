#pragma once

#include "CoreMinimal.h"
#include "Story/DeadFriend.h"
#include "Story/ThaddeusPoses.h"
#include "ThaddeusBright.generated.h"

/** What Thaddeus is at: his work's moments, each played from his pose table. */
enum class EThaddeusActivity : uint8
{
	/** The tape drawn out between his hands, held, let go (Together to Idle and back). */
	Measure,
	/** The tape held at full stretch, a pencil's tick. */
	Mark,
	/** Two raps of his knuckles on the board (KnockUp to KnockDown and back, twice). */
	Knock,
	/** Straightened from his work, his head up to whoever's talking to him. */
	Look
};

/**
 * Thaddeus Bright, Tilly's dead father (Docs/Areas/RansomsRest_Expansion.md, Knocking Below: "at his bench, a man in an
 * undertaker's apron is measuring a coffin that isn't there"): a friendly dead man in the cold room of Bright & Daughter's
 * cellar, home from the Sundown Road to his work, and a friend with lines through the story after (Speaker_Thaddeus). The
 * build script stands him at his bench (his origin about 45 cm back from its edge, facing it). He's there from Main 4's
 * finish, when Knocking Below opens (ShownWhen); the cellar is shut until then.
 *
 * At his bench he measures, over and over (ThaddeusPoses: the tape stretched between his hands, held, a pencil's tick, let
 * go), and every so often raps the board twice with his knuckles: the knocking Tilly hears at night, heard through the
 * cold-room door. Talked to, he straightens from the bench and turns his head to the listener. His body is SK_Thaddeus (its
 * rest pose his idle, stooped at the measure; his apron and tape skinned in, the tape stretching between his hands), else
 * SK_Unpaid in his Sunday black (MI_Ghost_B) until it's imported; his spectacles SM_ThaddeusSpectacles on the hat bone,
 * carrying SOCKET_Speaker.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AThaddeusBright : public ADeadFriend
{
	GENERATED_BODY()

public:
	static const FName SpeakerTag;

	static const TCHAR* const ModelPath;
	/** His spectacles (on the hat bone, with SOCKET_Speaker at his mouth). */
	static const TCHAR* const SpectaclesPath;

	AThaddeusBright();

	/** What he's at now: measuring, marking, knocking, or looking up to talk. */
	EThaddeusActivity GetActivity() const;

	/** Knocks now (the console, the tests): two raps on the board. */
	void KnockNow();

	/** Knocks this life (the tests). */
	int32 GetKnocks() const { return Knocks; }

	/** He knocks every this many seconds or so. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thaddeus", meta = (ClampMin = "3", Units = "s"))
	float KnockEvery = 14.f;

protected:
	virtual void BeginPlay() override;
	virtual FDeadFriendPose WantedPose(float Time) const override;
	virtual void TickFriend(float DeltaSeconds) override;

private:
	/** Seconds into a knock (negative: none under way), and until the next. */
	float KnockClock = -1.f;
	float NextKnock = 0.f;
	int32 Knocks = 0;
	int32 RapsHeard = 0;
	float MeasureClock = 0.f;
};
