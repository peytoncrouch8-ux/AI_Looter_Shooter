#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "XpPopupSubsystem.generated.h"

class AActor;
class APlayerController;
class AXpPopupActor;
class UPlayerProgressionSubsystem;

/**
 * Experience shown where it's earned (concept F, "XP popups"): a small "+38 XP" floats up at a kill a beat after its
 * damage number, a little to its right (it hears the player's progression: UPlayerProgressionSubsystem::OnKillXP), and a
 * big one pops in near the player's view on a mission turn-in, with the mission banner's rewards line
 * (UHudMissionCompleteWidget calls ShowMission). A victim whose damage numbers are hidden (a boss between phases) shows
 * none; menus and scenes hide them with the world labels' layer.
 *
 * Pooled: at most MaxPopups actors, made as needed and used again (the oldest showing one is taken when all are busy);
 * nothing ticks while none shows.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UXpPopupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UXpPopupSubsystem* Get(const UObject* WorldContextObject);

	/** A kill's experience at Victim (the small one). Null for none, or a victim whose damage numbers are hidden. */
	AXpPopupActor* ShowKill(int64 Amount, const AActor* Victim);

	/** The same at a spot (the tests; ShowKill works out the spot). */
	AXpPopupActor* ShowKillAt(int64 Amount, const FVector& Where);

	/** A turn-in's experience before Player's view (the big one), Delay seconds from now. Null for none or no player. */
	AXpPopupActor* ShowMission(int64 Amount, const APlayerController* Player, float Delay);

	/** The same at a spot, not following a view (the tests). */
	AXpPopupActor* ShowMissionAt(int64 Amount, const FVector& Where, float Delay);

	/** Popup actors made so far, and those in use. */
	int32 NumPooled() const;
	int32 NumShowing() const;

	/** The most popup actors there are at once. */
	static constexpr int32 MaxPopups = 8;
	/** A kill's comes up this long after the killing blow's number, so it reads as the kill's second beat (s). */
	static constexpr float KillDelay = 0.12f;
	/** A kill's spot: this share of the victim's half height over its middle. */
	static constexpr float KillSpotHeight = 0.6f;
	/** A turn-in's place before the camera (cm): ahead, right, up. */
	static constexpr double MissionAhead = 220.0;
	static constexpr double MissionRight = 0.0;
	static constexpr double MissionUp = -28.0;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** A free popup (made if there's room), else the oldest showing one. */
	AXpPopupActor* Take();

	void HandleKillXP(int64 XP, const AActor* Victim);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AXpPopupActor>> Pool;

	TWeakObjectPtr<UPlayerProgressionSubsystem> Progression;
	FDelegateHandle KillHandle;
};
