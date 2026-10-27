#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "Containers/Ticker.h"
#include "Scenes/SceneTimeline.h"
#include "Story/NewGameIntro.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NewGameIntroSubsystem.generated.h"

class ACameraActor;
class APlayerController;
class UAudioComponent;
class UIntroWidget;
class UUserWidget;
class UWorld;

/** What the menu's part leaves for the level's: whether the planet played for a session, or the whole intro was skipped. */
struct FNewGameIntroHandoff
{
	bool bPlanetShown = false;
	bool bSkippedAll = false;
};

/**
 * The new game's intro across its two worlds (Story/NewGameIntro.h). The game instance keeps it, so what the menu's part
 * did is still known when the level opens.
 *
 * The planet part plays in the menu world right after a new game is saved (USessionSubsystem::StartNewGame calls
 * PlayPlanet, then opens the level from Then): the menu's widgets fade and step aside, the letterbox comes in, a camera
 * leaves the menu's own view for Teropa (NewGameIntro::PlanetView) under the captions and Music.Intro.Planet, and the
 * white comes up; Then runs at full white. Its skip keys are the widget's (no scene holds the menu's keys); a skip
 * brings the white up at once and skips the Rest's part too. It ticks on the core ticker, in real time.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UNewGameIntroSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UNewGameIntroSubsystem* Get(const UObject* WorldContextObject);

	virtual void Deinitialize() override;

	/**
	 * Plays the planet part in World (the menu, when it is the planet scene) for the new game in Session (its slot, 0-2);
	 * Then runs once the screen is white (the level opens there). Session INDEX_NONE with no Then is a preview
	 * (Looter.Intro.Play in the menu): the menu comes back at its end. False (nothing plays; the caller goes on) when
	 * World isn't the planet scene, the intro or scenes are off, or it is playing already.
	 */
	bool PlayPlanet(UWorld& World, int32 Session, TFunction<void()> Then);

	bool IsPlayingPlanet() const { return TickHandle.IsValid(); }

	/** Seconds into the planet part (0 when it isn't playing). */
	float GetPlanetTime() const { return Timeline.GetTime(); }

	/** Skips the planet part (and with it the Rest's): the white at once, then Then. */
	void SkipPlanet();

	/** The level's part asks what the menu's did for Session (once: it is forgotten after). */
	FNewGameIntroHandoff TakeHandoff(int32 Session);

private:
	bool HandleTick(float DeltaSeconds);

	/** Builds the part's captions, sounds and white on Timeline. */
	void BuildTimeline();

	/** The menu's own widgets fading (Alpha 1 shown, 0 gone; collapsed at 0), or put back as they were. */
	void FadeMenus(float Alpha);
	void RestoreMenus();

	/** The white is up (played or skipped): the hand-off kept, the screen and keys let go, Then run. */
	void Finish(bool bSkipped);

	void Stop();

	FSceneTimeline Timeline;
	TWeakObjectPtr<UWorld> PlayWorld;
	TWeakObjectPtr<APlayerController> Controller;
	TWeakObjectPtr<AActor> PreviousViewTarget;
	TWeakObjectPtr<UAudioComponent> Music;
	TArray<TWeakObjectPtr<UUserWidget>> Menus;
	TArray<float> MenuOpacity;
	TArray<ESlateVisibility> MenuVisibility;
	TArray<FStoryLine> Words;
	FIntroView Base;
	FPlanetFrame Planet;
	TFunction<void()> Then;
	int32 PlaySession = INDEX_NONE;
	bool bShowedCursor = true;
	/** A skip is bringing the white up: seconds left until it is full (negative: none). */
	float SkipWhiteLeft = -1.f;
	/** Inside HandleTick: Stop lets the ticker go by HandleTick's return, not by removing it under itself. */
	bool bInTick = false;

	FNewGameIntroHandoff Handoff;
	int32 HandoffSession = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UIntroWidget> Widget;

	FTSTicker::FDelegateHandle TickHandle;
};
