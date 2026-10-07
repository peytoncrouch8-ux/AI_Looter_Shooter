#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "AreaTravelSubsystem.generated.h"

class AActor;
class ATrain;
class UAreaDefinition;
class UTransitionScreenSubsystem;
class UWorld;
struct FCampaignRecord;
struct FStationBoardLine;

/**
 * Trips from the station boards (Areas/StationBoard.h) and how the player arrives, one per level:
 *  - the first cast-off, off Skyreach: its jetty plays the skiff's ride (ASkiffJetty::CastOff), and behind the white the
 *    ride ends in, CompleteFirstCastOff records it and travels to the story's first arrival: the family plot on Ransom's
 *    Rest (Landing_FamilyPlot). Without a jetty or a ride it goes at once, behind the white (LeaveForFirstArrival).
 *  - every later trip (practice to Skyreach and back, every station board): a plain fade to black, then the session's
 *    travel to the destination's station (its first landing). No cutscene: a default the user can change. Except by
 *    train: a trip to an opened area of the story from a station whose train stands at its platform plays the train's
 *    departure (Scenes/TrainShots.h: it pulls out, then black), and arrives by train (it backs in, then the player's
 *    view), as step 9 chose. Practice trips stay plain fades both ways.
 *  - arriving: the story's first area opens on the cold open while it's due (UColdOpenSubsystem takes the held white and
 *    reveals it with REVENANT on the gang's skiff, then Ellis claws out of the grave). Otherwise a white held through the
 *    level load (UTransitionScreenSubsystem) is revealed, with the title REVENANT on the story's first arrival and none
 *    otherwise; after a plain trip the screen fades in from black.
 * Saves wait while a trip fades out (the trip's own save has everything).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UAreaTravelSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Seconds the screen takes to go black before a plain trip, and to come back on its arrival. */
	static constexpr float FadeOutSeconds = 0.6f;
	static constexpr float FadeInSeconds = 0.8f;

	static UAreaTravelSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/**
	 * Takes a station board's line from From (the jetty or the station whose board it is; may be null): the first cast-off
	 * (the jetty's ride, else straight on behind the white) or a plain trip. False, logged, when nobody goes.
	 */
	bool Depart(const FStationBoardLine& Line, AActor* From);

	/** A plain trip to Area's Landing: the screen fades to black, then the session travels. False when it can't go. */
	bool FadeTo(UAreaDefinition& Area, FName Landing);

	/**
	 * A trip to Area's Landing by Train: its departure shot, then the trip in the black, arriving by train. False when the
	 * shot can't play (scenes off, a trip or a scene under way, no level to go to): the caller fades instead.
	 */
	bool DepartByTrain(ATrain& Train, UAreaDefinition& Area, FName Landing);

	/**
	 * The train Line goes by from From: an opened area of the story (not practice, not the first cast-off) from a station
	 * (ATrainStation) whose train stands at its platform. Null: a plain trip.
	 */
	static ATrain* FindTrainFor(const FStationBoardLine& Line, const AActor* From);

	/** Leaves for the story's first arrival at once, behind the white: no ride (no jetty, scenes off, the dev commands). */
	bool LeaveForFirstArrival();

	/**
	 * The first cast-off, behind the white: records it (the story has begun, its first area is open, the tutorial is behind
	 * the player), then travels there, to the first arrival; Skyreach's world and the player's guns go into the trip's save.
	 * False, with the record put back, when nobody can go (the white stays for the caller to reveal).
	 */
	bool CompleteFirstCastOff();

	/** A trip is fading out or leaving: no second one. */
	bool IsDeparting() const { return bDeparting; }

	/** The landing the player arrived at as this level began (None: its start, or a saved spot). */
	FName GetArrivalLanding() const { return ArrivalLanding; }

	/**
	 * The board's lines in World as its story stands: every area asset (put in OutAreas, which the caller holds while it
	 * shows the lines), the level's missions and the area the level is.
	 */
	static TArray<FStationBoardLine> GatherBoardLines(const UWorld* World, TArray<UAreaDefinition*>& OutAreas);

	/** The story being played in World: the mission runner's record (the session's, or kept in memory without one). */
	static FCampaignRecord* FindCampaign(const UWorld* World);

	/** The title an arrival at Landing reveals the held white with: REVENANT on the story's first arrival, else none. */
	static FText ArrivalTitle(FName Landing);

private:
	/** Every actor has begun play and the session has put the player on their landing: the white or the fade comes off. */
	void HandleLevelBegun();

	/** The fade-out is black: the trip goes, or the screen comes back if it can't. */
	void FinishFade();

	/** The departure shot is black: the trip goes by train, or the train comes back and the player with it. */
	void FinishTrainDeparture();

	/** The player arrived by train at a landing with the train at its platform: the arrival shot. False when it can't play. */
	bool PlayTrainArrival();

	UTransitionScreenSubsystem* GetTransition() const;

	/** Where the plain trip fading out now goes. */
	UPROPERTY(Transient)
	TObjectPtr<UAreaDefinition> FadeArea;

	FName FadeLanding;
	FTimerHandle FadeTimer;
	FName ArrivalLanding;
	bool bArrivedByTrain = false;
	bool bDeparting = false;
};
