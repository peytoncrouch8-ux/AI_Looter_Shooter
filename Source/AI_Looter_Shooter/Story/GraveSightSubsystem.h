#pragma once

#include "CoreMinimal.h"
#include "Story/GraveSightFlash.h"
#include "Subsystems/WorldSubsystem.h"
#include "GraveSightSubsystem.generated.h"

class UHudGraveSightWidget;

/**
 * Grave Sight on the local player's screen (Docs/Story.md: "The cyan lines are Grave Sight: Ellis sees souls, weak points
 * and the Unpaid"). For now that's its flash: a couple of seconds of the HUD's cyan over the world while Ellis sees what a
 * soul left behind (Saint Ada's ember lifting off her Reliquary, Main 4: AChapelReliquary starts it; Looter.Story.GraveSight
 * from the console). One flash plays at a time, and a new one starts over. It shows on UHudGraveSightWidget, made for the
 * local player the first time a flash plays and kept in the viewport, collapsed between flashes; a level without a local
 * player (a test) plays it unseen. The vision mode itself (a post-process) stays a later, measured option.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGraveSightSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGraveSightSubsystem* Get(const UObject* WorldContextObject);

	/** Played worlds, and the editor preview worlds the automated tests build. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Flashes Grave Sight for Seconds; a flash under way starts over. */
	void Flash(float Seconds = FGraveSightFlash::DefaultSeconds);

	/** A flash is under way. */
	bool IsFlashing() const { return Current.IsPlaying(); }

	/** The flash under way, or the last one. */
	const FGraveSightFlash& GetFlash() const { return Current; }

	/** How many flashes have played in this level. */
	int32 GetFlashCount() const { return Flashes; }

	/** Moves the flash on and shows it: the tick does while one plays; a test level never ticks, so tests call it. */
	void Advance(float DeltaSeconds);

private:
	/** The local player's overlay, made the first time it's needed; null without a local player. */
	UHudGraveSightWidget* FindOrMakeOverlay();

	/** Shows the flash as it stands on the overlay (collapsed once it's over). */
	void ShowFlash();

	FGraveSightFlash Current;
	int32 Flashes = 0;

	UPROPERTY(Transient)
	TObjectPtr<UHudGraveSightWidget> Overlay;
};
