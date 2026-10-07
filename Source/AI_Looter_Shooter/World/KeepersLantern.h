#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "KeepersLantern.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;
struct FCampaignRecord;

/**
 * Abel's lantern where the spiders hoarded it, in the webbing on the Sink's floor (Docs/Areas/RansomsRest.md, Main 5 "The
 * Keeper's Lantern": "Take the Keeper's Lantern from the webbing. It is dark."; Main 4: "Your father's fell in the dark,
 * whole. Spiders hoard anything a saint has touched. Look in the Sink."). A cobweb tangle (Art/Models/Props/Sink.py's
 * Web_Snare: no collision, no shadow) with SM_KeepersLantern (Art/Models/Props/BurialDeck.py) hanging from its
 * SOCKET_Lantern by the lantern's SOCKET_Grip.
 *
 * It is dark: no light of its own, and its glass (the LanternGlow slot) shows the house trim's window glass instead of the
 * glow, as BurialDeck.py's notes ask, until Main 6 has Abel light it (SetLit). A tap of Interact takes it ("Take the
 * Keeper's Lantern") while TakeWhen holds (Main 5's third step); the player's interaction component tells the missions (an
 * Interact on the actor tagged Lantern_Keeper), and it leaves the snare for good: the empty snare stays.
 *
 * Whether Ellis has it is read from the campaign record, not saved apart (IsTaken): Main 5 past its taking step, or
 * finished. So a session loaded on Main 5's last step, or any time after, finds the snare empty, and one loaded before the
 * lantern was taken finds it hanging there again; Main 6 carries it from there. A small box round the lantern is what the
 * Interact key's line finds (it blocks nothing else). It never ticks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AKeepersLantern : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag missions find it by (Main 5's "Take the Keeper's Lantern"). */
	static const FName LanternTag;

	/** The mission it's taken in (Main 5, "The Keeper's Lantern") and its step that asks for it (counted from 0). */
	static const FName Mission;
	static constexpr int32 TakeStep = 2;

	/**
	 * Ellis has taken it from the webbing: Main 5 is past its taking step, or finished. Runner adds the mission running in
	 * this level (in tests), as FStoryCondition does. Main 6 carries it from here to the keeper's post.
	 */
	static bool IsTaken(const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr);

	/** The same in WorldContextObject's level, from its mission runner's campaign record; no without one. */
	static bool IsTakenIn(const UObject* WorldContextObject);

	AKeepersLantern();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	/** Taken for good: the story keeps it so (IsTaken), and a lasting objective may count it. */
	virtual bool IsUsedUp() const override { return bTaken; }

	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * Takes it from the webbing as a tap of Interact does (the console). bForce takes it whatever TakeWhen says. Missions
	 * hear of it from the player's interaction component, not from here. False when it's gone already or can't be taken now.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lantern")
	bool Take(AActor* ByWhom, bool bForce = false);

	/** It hangs in the webbing and TakeWhen holds (empty: any time). */
	UFUNCTION(BlueprintPure, Category = "Lantern")
	bool CanTake() const;

	/** It still hangs in the snare. */
	UFUNCTION(BlueprintPure, Category = "Lantern")
	bool IsHanging() const { return !bTaken; }

	/** The lantern's glass slot: GlassSlot by name, else the slot whose material is named for it; INDEX_NONE without one. */
	int32 FindGlassSlot() const;

	/** Its glass lit (the LanternGlow it was made with) or dark (DarkGlass). There is never a light: it's an emissive glass. */
	void SetLit(bool bInLit);
	bool IsLit() const { return bLit; }

	/** Reads its story again: gone once the story says it's taken, back in the snare when the story went back before its step. */
	void RefreshStory();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The cobweb tangle it hangs from (SM_Web_Snare), its pivot at the tangle's middle, its front (+X) away from the wall. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Snare;

	/** The Keeper's Lantern (SM_KeepersLantern), hung by its grip from the snare's cord. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lantern;

	/** Round the lantern: what the Interact key's line finds; it blocks nothing else. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Grip;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern")
	FText Prompt;

	/** It can be taken only while this holds (Main 5's third step, from 0: 2). Empty: any time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern")
	FStoryCondition TakeWhen;

	/** How far from the player's eyes it can be taken (cm): it hangs at chest height in the webbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 300.f;

	/** The snare's socket the lantern hangs from, and the lantern's own the cord holds it by. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern")
	FName HangSocket = TEXT("Lantern");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern")
	FName GripSocket = TEXT("Grip");

	/** The lantern's glass slot (BurialDeck.py's LanternGlow), and what it shows while dark: the trim's window glass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern|Glass")
	FName GlassSlot = TEXT("LanternGlow");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern|Glass")
	TObjectPtr<UMaterialInterface> DarkGlass;

	/** Its glass glows (Abel lights it in Main 6). It's dark in the Sink. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern|Glass")
	bool bLit = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Hangs the lantern by its grip from the snare's socket (or where Sink.py's cord ends), and the box round it. */
	void HangLantern();

	/** Its glass lit or dark, as bLit says. */
	void ApplyGlass();

	/** Shows it in the snare (and findable), or puts it away for good. */
	void ShowInSnare(bool bShown);

	void HandleMissionsChanged();

	/** The glass slot's own material (the glow), kept for lighting it again. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LitGlass;

	bool bTaken = false;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
