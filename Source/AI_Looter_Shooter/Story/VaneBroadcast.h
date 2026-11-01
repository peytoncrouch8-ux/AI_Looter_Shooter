#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"
#include "Subsystems/WorldSubsystem.h"
#include "VaneBroadcast.generated.h"

class UAudioComponent;

/**
 * Cornelius Vane's voice breaking in (the user's pick: a taunting voice on radios and loudspeakers, Borderlands style;
 * captions for now, voices later): on the Lamplighter's horn as the player reaches the prow, over the valley's company
 * loudspeakers in the spyglass reveal, and wherever the story wants him later. Say plays the radio-in crackle, holds a hiss
 * under him while he talks, shows his lines as captions in his name ("Cornelius Vane"; a line that names someone else keeps
 * its speaker), and closes with the radio-out click once the last line is done. One broadcast at a time: a new one cuts the
 * last off. Nothing is heard in a world that plays no audio (the tests), but the lines and the timing run the same.
 *
 * Story lines never overlap (the play check of 2026-11-01: the fly-by's radio broke in over Hob's road line): he waits,
 * on the air but silent, until no line is on screen or waiting (anyone's: Hob's remarks, a talk, a notice), at most
 * QuietWaitSeconds; then the crackle, and his lines queue behind whatever is being said, never cutting it off. A line
 * that comes while he talks waits its turn (Hob's queue), unless it interrupts (the player turned to talk to someone).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UVaneBroadcast : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Vane says Lines over the radio in Context's world (empty: nothing). */
	static void Say(UObject* Context, const TArray<FStoryLine>& Lines);

	/** Cuts him off now (a skipped scene): his captions cleared, the hiss stopped, the click. */
	static void Stop(UObject* Context);

	/** He's on the air: from Say (waiting for quiet included) to the click. */
	static bool IsSpeaking(const UObject* Context);

	/** "Cornelius Vane", as his captions name him. */
	static FText SpeakerName();

	/** The crackle comes this long before his first words. */
	static constexpr float RadioLeadSeconds = 0.45f;

	/** He waits at most this long (s) for the captions to fall quiet before his crackle; his lines then queue behind them. */
	static constexpr float QuietWaitSeconds = 8.f;

	static UVaneBroadcast* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Moves the broadcast on (the tick's work; the tests call it). */
	void Advance(float DeltaSeconds);

	bool IsOnAir() const { return bOnAir; }
	/** On the air but not yet heard: waiting for the captions to fall quiet. */
	bool IsWaitingForQuiet() const { return bOnAir && bWaiting; }

private:
	void Begin(const TArray<FStoryLine>& Lines);
	void End(bool bClearCaptions);
	/** The crackle and the hiss: his lines follow after RadioLeadSeconds. */
	void StartRadio();
	/** No line on screen or waiting. */
	bool AreCaptionsQuiet() const;

	TArray<FStoryLine> Pending;
	bool bWaiting = false;
	float WaitLeft = 0.f;
	float LeadLeft = 0.f;
	int32 Conversation = INDEX_NONE;
	bool bOnAir = false;
	bool bLinesStarted = false;
	TWeakObjectPtr<UAudioComponent> Hiss;
};
