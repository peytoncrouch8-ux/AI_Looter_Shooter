#pragma once

#include "CoreMinimal.h"
#include "Story/CaptionQueue.h"
#include "Subsystems/WorldSubsystem.h"
#include "CaptionSubsystem.generated.h"

/** How new lines meet the ones already being said. */
enum class ECaptionPlay : uint8
{
	/** Now: the line on screen fades out quickly and what waited is dropped (someone the player turned to talk to). */
	Interrupt,
	/** After everything queued (a remark on something that happened). */
	Queue,
};

/**
 * The level's captions: every line said aloud plays through here, one at a time in order (FCaptionQueue's rules), and
 * the caption widget (UHudCaptionWidget) shows the line on screen. Each line is logged as it comes on, so a play's log
 * reads like its script. While a menu covers the game the widget holds the queue, so no line plays on unseen; the game
 * paused stops it too.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCaptionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UCaptionSubsystem* Get(const UObject* WorldContextObject);

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Plays lines as captions. Returns their conversation, to ask after (0: none of them had words). */
	int32 Play(const TArray<FStoryLine>& Lines, ECaptionPlay How = ECaptionPlay::Interrupt);

	/** Everything said stops: the line on screen fades out quickly. */
	void Clear();

	/** Some of the conversation's lines are on screen or still to come. */
	bool IsPlaying(int32 Conversation) const;

	/** The line on screen, or null; and how visible it is (0-1). */
	const FCaptionEntry* GetCurrent() const;
	float GetAlpha() const;

	/** While nobody can read them (a menu over the game), the lines wait where they are. The caption widget sets it. */
	void SetHeld(bool bInHeld);
	bool IsHeld() const { return bHeld; }

	/** Moves the captions on by DeltaSeconds unless they're held: the tick calls it, and so can tests. */
	void Update(float DeltaSeconds);

	const FCaptionQueue& GetQueue() const { return Queue; }

private:
	/** Logs the line on screen the first time it's there. */
	void LogNewLine();

	FCaptionQueue Queue;
	int32 LoggedSerial = 0;
	bool bHeld = false;
};
