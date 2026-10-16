#pragma once

#include "CoreMinimal.h"
#include "Embers/EmberPower.h"
#include "EmberBorrowedTime.generated.h"

class AActor;
class ACharacter;
class AController;
class UHealthComponent;
class UPlayerProgressionSubsystem;

/**
 * Borrowed Time, Ellis's Debt power (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Five heroes"): for its duration
 * (8 s) the damage the player takes goes into Sexton's ledger instead, to be paid back afterwards.
 *  - Every hurt is healed back at once (UHealthComponent::OnDamaged, as it lands: a blow that would have killed is
 *    healed back before the death is counted) and the damage is added to the debt.
 *  - Every kill (UPlayerProgressionSubsystem::OnKillXP) strikes Strength (0.25) of the debt off, a violet flash of
 *    soul-light at the one killed.
 *  - When it ends the debt is paid back as health drained evenly over PayBackSeconds (3 s; UHealthComponent::Drain,
 *    which never takes the last point: it can't kill). Death forgives it; a scene or a level's end doesn't.
 *  - Heard: Ember.BorrowedTime.Start, and .Settle as the debt is called in. Seen: violet motes round the feet while it
 *    runs, a puff of violet where each hurt is taken back, a dip of the view as it settles.
 *  - It shows nothing on the HUD itself: the socket and the page read GetDebt / GetDebtFraction / IsSettling (the
 *    component's GetActivePower while it runs, GetPowerFor("BorrowedTime") while it settles).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberBorrowedTime : public UEmberPower
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** Health owed right now (while it runs, and while it's paid back). */
	float GetDebt() const { return Debt; }

	/** The debt as a share of the player's most health (0 to 1), for a bar. */
	float GetDebtFraction() const;

	/** Taking hurts into the ledger now. */
	bool IsBorrowing() const { return bBorrowing; }
	/** Paying the debt back now. */
	bool IsSettling() const { return bSettling; }

	/** A kill strikes its share off the debt (the kill's experience calls it; public for the tests). Returns what it struck. */
	float NoteKill(const AActor* Victim);

	/** Pays back DeltaSeconds' share of the debt (its own timer; the tests call it directly). */
	void StepSettle(float DeltaSeconds);

	static constexpr float DefaultStrength = 0.25f;
	static constexpr float DefaultPayBackSeconds = 3.f;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);
	void HandleKillXP(int64 XP, const AActor* Victim);

	void Bind(bool bOn);
	void SchedulePump();
	void Pump();
	void ShowMotes();

	TWeakObjectPtr<ACharacter> Body;
	TWeakObjectPtr<UHealthComponent> Health;
	TWeakObjectPtr<UPlayerProgressionSubsystem> Progression;
	FDelegateHandle KillHandle;

	float Strength = DefaultStrength;
	float PayBackSeconds = DefaultPayBackSeconds;
	FLinearColor Color = FLinearColor(0.58f, 0.46f, 1.f);

	bool bBorrowing = false;
	bool bSettling = false;
	bool bPumping = false;
	float Debt = 0.f;
	/** What the settle pays back each second. */
	float PayRate = 0.f;
	/** The player's health as last told (the health component's change event), so a hurt's real cost is known. */
	float KnownHealth = 0.f;
	float MoteTimer = 0.f;
	FRandomStream Random{ 0xb0270 };
};
