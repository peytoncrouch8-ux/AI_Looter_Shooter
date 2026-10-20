#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTierLook.h"
#include "CreatureTierLookComponent.generated.h"

class ACreatureBase;
class AController;
class UCreatureHealthBarWidget;
class UHealthComponent;
class UPrimitiveComponent;
class USkeletalMeshComponent;

/**
 * A creature's tier as the player sees it (CreatureTierLook has the rules; Saved/Tiers/DESIGN.md, "Readable and
 * rewarding"). Made on a creature the first time its rank is a tier above Regular (Refresh, from ACreatureBase's rank
 * being applied), never on a Regular one, a Legendary monster or a boss:
 *  - its shell: the body's mesh once more on a follower of its pose (SetLeaderPoseComponent), every slot in the shared
 *    M_CreatureTier look for its kind and tier (UCreatureTierFXSubsystem::GetShellLook): the spiders' eyes and the slime's
 *    core glowing in the tier's colour, and the Ethereal's faint red ghost-rim shimmering up its body. A follower, not the
 *    body's overlay slot, so a hit flash or an ember power's tint never takes it away mid-fight (they skip it: IsTierShell).
 *    No shadow, no collision, no light: one more translucent draw of the body, for the few creatures above Regular;
 *  - its flare (the shell's custom primitive data, CreatureTierLook::FlareIndex): the glow flares as it turns on the
 *    player (its rank sting; a top tier's harder) and as it dies, then goes out with the body (an Unpaid's phase-step and
 *    dissolve fade it too). Its traits scale it (UCreatureTraitComponent::GetGlowScale: Vengeful's enrage doubles it, a
 *    Grave-burst's swell brightens it, Mending breathes it), on the shell and on an Unpaid's coal alike; anything else may
 *    hold a flare of its own on top (SetHeldFlare);
 *  - its death, a top tier's: the show in its colour (UCreatureTierFXSubsystem::PlayDeath: the burst, the ring, the
 *    pillar of soul-light, the motes, the loot's pop and both sounds), a frame after the death, or once a Grave-burst's
 *    swell is over (its soul goes with the blast); its glow holds while it swells, then goes out;
 *  - its tag's tier (DressTag, from its tag's frame): the thick frame and the top tiers' flash, and the trait line.
 * The Unpaid's coal and the dead miner's lamp take the tier's colour in their own code (CreatureTierLook::CoalGlow,
 * LampColor). Ticks only while it has something to drive: a shell, a death show waiting, an Unpaid's coal with traits.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureTierLookComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureTierLookComponent();

	/** Creature's rank was just applied (ACreatureBase::ApplyRank): its look follows, made on its first tier above Regular. */
	static void Refresh(ACreatureBase& Creature);

	/**
	 * Its tag dressed for its tier (the frame, the top tiers' flash, the trait line) and a Warded one's ward bar
	 * (UCreatureTraitComponent::GetWardFraction): cheap every frame its tag shows.
	 */
	static void DressTag(const ACreatureBase& Creature, UCreatureHealthBarWidget& Tag);

	/** Actor's tier look, if it has one. */
	static UCreatureTierLookComponent* Find(const AActor* Actor);

	/** Whether Component is a tier shell: the hit flash, an ember power's tint and the like pass it by. */
	static bool IsTierShell(const UPrimitiveComponent* Component);

	/** The tag every shell carries. */
	static const FName ShellTag;

	/** Its look at the creature's rank now: the shell worn, changed or taken off, its glow lit again (a respawn's). */
	void ApplyTier();

	/** Moves its flare and fade on and writes them to the shell (its tick; a test level, which doesn't tick, calls it). */
	void Advance(float DeltaSeconds);

	/** A flare held on top of its own (0 for none): a trait's moment, such as Vengeful's enrage. */
	void SetHeldFlare(float Amount);

	/** The tag's trait line now (the traits' words; empty with none), made again only when its traits change. */
	FText TraitLineNow();

	USkeletalMeshComponent* GetShell() const { return Shell; }
	const FCreatureTierShell& GetShellSpec() const { return ShellSpec; }
	ECreatureRank GetShownRank() const { return ShownRank; }
	/** The flare and fade last written to the shell. */
	float GetFlare() const { return ShownFlare; }
	float GetFade() const { return ShownFade; }
	/** Its death's show has been played this life, or is waiting (its next frame, or a Grave-burst's swell). */
	bool HasShownDeath() const { return bDeathShown; }
	bool IsDeathShowWaiting() const { return bDeathShowPending; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleDeath(AController* Killer);

	ACreatureBase* GetCreature() const;
	void BindHealth();
	/** Puts the shell on (made the first time) in the look ShellSpec asks for; off if it can't be (no model, no material). */
	void WearShell();
	void DropShell();
	/** Writes Flare and Fade to the shell when they've changed (or bForce). */
	void WriteLook(float Flare, float Fade, bool bForce);
	/** An Unpaid's coal and ember edge at Scale times its tier's glow (its traits'), written when it changes. */
	void ScaleCoal(float Scale);
	/** Ticks while there's a shell to drive, a death show waiting, or an Unpaid's coal its traits may brighten. */
	void UpdateTicking();

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Shell;

	FCreatureTierShell ShellSpec;
	ECreatureRank ShownRank = ECreatureRank::Basic;
	TWeakObjectPtr<UHealthComponent> Health;
	/** Seconds since it died (below zero: alive; held at 0 while a Grave-burst swells), and its show: played, waiting. */
	float DeathAge = -1.f;
	bool bDeathShown = false;
	bool bDeathShowPending = false;
	/** An Unpaid's body (dead miners, the Bell-Ringer): its phase-step fades the shell with it, its coal takes the traits' glow. */
	bool bFollowsPhase = false;
	float HeldFlare = 0.f;
	float ShownFlare = -1.f;
	float ShownFade = -1.f;
	/** The coal's scale last written (1: as its rank set it). */
	float ShownCoalScale = 1.f;

	/** The traits its trait line was made for, and the line. */
	TArray<ECreatureTrait> LineTraits;
	FText Line;
	bool bLineMade = false;
};
