#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Math/RandomStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/WindowLifeRules.h"
#include "WindowLifeSubsystem.generated.h"

class AHouseLights;
class AWindowLifeCard;
class AWindowShutter;

/**
 * Somebody's home, never a face (WindowLifeRules has the when and how): the lived-in houses' lights (AHouseLights) join
 * with their lamplit windows (the house model's SOCKET_Window_<n>), and the level's shutters are found as they stand.
 * Twice a second, while any house has joined, it looks where the player looks (plain distances and angles; one line
 * trace only when an event is about to start): at a window seen from across the street, now and then, a shadow crosses
 * behind the glass or stops there a moment, a curtain twitches, or a shutter left open swings shut. One event at a time,
 * a house at most every 30-90 s; by day curtains and shutters only. The shadow and the curtain play on one card
 * (AWindowLifeCard, spawned once and moved to each window), with a quiet sound at the window (World.Window.Steps,
 * World.Window.Curtain); a shutter slams by itself (AWindowShutter::Slam, its own bang).
 *
 * Looter.WindowLife 0/1 turns it off and on; Looter.WindowLife.Show [Shadow|Curtain|Shutter] plays one now at the window
 * the player looks at. Played worlds, and the editor preview worlds the automated tests build (they call Check).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UWindowLifeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UWindowLifeSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	/** A lived-in house's lights join with their lit windows (AHouseLights::NumLitWindows), or leave. */
	void AddHouse(AHouseLights* House);
	void RemoveHouse(AHouseLights* House);
	int32 NumHouses() const;

	/** Finds the level's shutters again (they're found on the first look, and every half minute after). */
	void RefreshShutters();
	int32 NumShutters() const;

	/**
	 * One look from Eyes along ViewDirection at world time Now (the timer's, from the player's camera; tests call it):
	 * maybe an event starts. Returns its kind, or None.
	 */
	EWindowLifeKind Check(const FVector& Eyes, const FVector& ViewDirection, double Now);

	/** Plays Kind at House's lit window number Window (from 0) at Now, whatever the clock says (the console, tests). */
	bool PlayAt(AHouseLights* House, int32 Window, EWindowLifeKind Kind, double Now);

	/** Slams Shutter as a window event at Now, whatever the clock says (the console, tests). */
	bool PlayShutter(AWindowShutter* Shutter, double Now);

	/** The last event: its kind, where (the house's lights, or the shutter), which window, when; and how many so far. */
	EWindowLifeKind GetLastKind() const { return LastKind; }
	const AActor* GetLastPlace() const { return LastPlace.Get(); }
	int32 GetLastWindow() const { return LastWindow; }
	double GetLastTime() const { return LastTime; }
	int32 NumEvents() const { return Events; }

	const FWindowLifeClock& GetClock() const { return Clock; }

	/** The card the shadows and curtains play on (spawned on first need), or null. */
	AWindowLifeCard* GetCard() const { return Card.Get(); }

	/** The dice, fixed for a test. */
	void SetRandomSeed(int32 Seed) { Random.Initialize(Seed); }

	/** Whether a line of sight is traced before an event (on in the game; a test can turn it off). */
	void SetSightChecks(bool bOn) { bSightChecks = bOn; }

	/** Whether window life plays at all (Looter.WindowLife). */
	static bool IsEnabled();

	/** The window under the player's look, for the console: the nearest lit window they see, or none. */
	bool FindSeenWindow(const FVector& Eyes, const FVector& ViewDirection, AHouseLights*& OutHouse, int32& OutWindow) const;

	/** The open shutter under the player's look, for the console, or null. */
	AWindowShutter* FindSeenShutter(const FVector& Eyes, const FVector& ViewDirection) const;

private:
	void UpdateTimer();
	void Look();
	AWindowLifeCard* EnsureCard();
	/** The key a shutter rests by: the lit house it's on (the nearest lights within a few metres), or itself. */
	FObjectKey HouseKeyOf(const AWindowShutter* Shutter) const;
	bool IsDuskNow() const;
	bool HasSightLine(const FVector& Eyes, const FVector& Target) const;
	void Remember(EWindowLifeKind Kind, AActor* Place, int32 Window, double Now);

	TArray<TWeakObjectPtr<AHouseLights>> Houses;
	TArray<TWeakObjectPtr<AWindowShutter>> Shutters;
	double NextShutterLook = 0.0;
	bool bShuttersFound = false;

	TWeakObjectPtr<AWindowLifeCard> Card;
	FWindowLifeClock Clock;
	FRandomStream Random = FRandomStream(20261014);
	FTimerHandle Timer;
	bool bSightChecks = true;

	EWindowLifeKind LastKind = EWindowLifeKind::None;
	TWeakObjectPtr<AActor> LastPlace;
	int32 LastWindow = INDEX_NONE;
	double LastTime = -1.0;
	int32 Events = 0;
};
