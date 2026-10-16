#pragma once

#include "CoreMinimal.h"
#include "EmberTypes.generated.h"

/** Where the equipped ember power stands (UEmberComponent::GetState). */
UENUM()
enum class EEmberState : uint8
{
	/** Can be used now. */
	Ready,
	/** Running (a power with a duration); Ready or Cooldown after. */
	Active,
	/** Used; waiting out its cooldown. */
	Cooldown,
};

/** Why a press of the ember key did nothing (UEmberComponent::TryUse; the HUD's socket shakes on the ones worth showing). */
UENUM()
enum class EEmberDenied : uint8
{
	None,
	/** No power equipped (no hero, or the hero owns none). */
	NoPower,
	Cooldown,
	/** Already running. */
	Active,
	Dead,
	/** A scene, a menu, a mantle or vault, a swap: the body is spoken for. */
	Busy,
	/** The power's own say: nothing to dash into, no room, and so on. */
	Refused,
};

/** What the session keeps of the ember powers (ULooterSessionSave::Embers): the one equipped, by its id, and its cooldown left. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FEmberSave
{
	GENERATED_BODY()

	/** UEmberDefinition::Id of the equipped power; None: the hero's first. */
	UPROPERTY()
	FName Equipped;

	/** Seconds of cooldown left when saved (0: ready), so a Save & Quit doesn't reset a power mid-cooldown. */
	UPROPERTY()
	float CooldownLeft = 0.f;
};

/** The equipped power changed: the new one's id (None: nothing equipped). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEmberEquipped, FName /*Id*/);
/** The equipped power's state changed (Ready, Active, Cooldown). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEmberStateChanged, EEmberState /*State*/);
/** The equipped power was used: its id. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEmberUsed, FName /*Id*/);
/** The key was pressed and nothing happened: why. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEmberDenied, EEmberDenied /*Why*/);
