#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tutorial/ControlHintRules.h"
#include "ControlHintSubsystem.generated.h"

class AActor;
class ACharacter;
class APawn;
class APlayerController;
class AWeaponBase;
class UWeaponManagerComponent;

/**
 * The contextual control hints (Docs/Polish/TutorialRework.md, "Contextual hints"). Ten times a second it looks at what
 * the local player is doing (ControlHintSubsystemSenses.cpp: moving, sprinting, a ledge ahead, a far target under the
 * crosshair, a low magazine, a creature close in front, a second gun, the bench) and hands it to FControlHintRules, which decide the one hint on
 * screen, if any; the HUD's UHudControlHintWidget shows it with the key the player bound.
 *
 * What the player has learned and how often each hint has shown is kept for the profile (ULooterControlHintsSave, slot
 * "ControlHints"), so a control learned once is never taught again, in any session. Settings > Interface > Control hints
 * turns them off (UGraphicsSettingsSubsystem). It runs in every played level (Skyreach, and the areas too, for a player
 * who skipped the island), never behind the main menu. Looter.Hints reset|show <Hint> for testing (reset brings every
 * hint back: PIE and the standalone game share the slot).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UControlHintSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Seconds between looks at the player. */
	static constexpr float LookInterval = 0.1f;

	/** The profile's save slot. */
	static constexpr const TCHAR* SaveSlot = TEXT("ControlHints");

	/** How near a bench counts as at it (cm, from the player to the bench). */
	static constexpr float BenchDistance = 450.f;

	/** How far ahead (cm, past the capsule's front) a ledge counts as just ahead. */
	static constexpr float LedgeReach = 90.f;

	/** A ledge's top, over the player's feet (cm): from knee height to chest height. */
	static constexpr float LedgeMinHeight = 50.f;
	static constexpr float LedgeMaxHeight = 140.f;

	/** Coming down this much higher (cm) than the ground left is a climb. */
	static constexpr float ClimbRise = 50.f;

	/** How far the far-target look reaches (cm). */
	static constexpr float TargetProbeLength = 8000.f;

	static UControlHintSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bActive; }
	virtual TStatId GetStatId() const override;

	const FControlHintRules& GetRules() const { return Rules; }

	/** The hint on screen, or EControlHint::Count. */
	EControlHint GetShown() const { return Rules.GetShown(); }

	/** The key the shown hint teaches, by binding id: the rules' own, or for swapping guns the slot of a gun not in hand. */
	FName GetShownAction() const;

	/** The shown hint's words as the player's settings make them (holding or toggling to sprint). */
	FText GetShownWords() const;

	/** Forgets what every hint learned and showed, and saves that. */
	void ResetMemory();

	/** Shows a hint now (the console, the HUD photos). */
	void ForceShow(EControlHint Hint);

private:
	// --- Looking at the player (ControlHintSubsystemSenses.cpp) ---

	/** What the player is doing since the last look, DeltaSeconds ago. */
	FControlHintInput Look(float DeltaSeconds);

	/** A ledge of knee to chest height within LedgeReach ahead along Heading. */
	bool ProbeLedge(const ACharacter& Character, const FVector& Heading) const;

	/** The crosshair is on something alive that can be hurt, farther than FControlHintRules::FarTargetDistance. */
	bool ProbeFarTarget(const APlayerController& Controller, const APawn& Pawn, const AWeaponBase& Gun) const;

	/** Two guns of one kind are carried (slots and backpack) and a gunsmith's bench is within BenchDistance. */
	bool IsAtBenchWithPair(const APawn& Pawn, const UWeaponManagerComponent& Weapons);

	// --- The profile's memory (ControlHintSubsystem.cpp) ---

	void Load();
	void Save() const;

	FControlHintRules Rules;
	bool bActive = false;
	float SinceLook = 0.f;

	// What the last look saw, to tell what changed since
	TWeakObjectPtr<APawn> LastPawn;
	FVector LastLocation = FVector::ZeroVector;
	bool bWasOnGround = true;
	double LastGroundZ = 0.0;
	int32 LastGunsCarried = INDEX_NONE;
	int32 LastSlot = INDEX_NONE;
	int32 LastMeleeCount = INDEX_NONE;

	/** The level's benches, found once. */
	TArray<TWeakObjectPtr<AActor>> Benches;
	bool bBenchesFound = false;
};
