#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "ReadableProp.generated.h"

class UMissionRunner;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Something to read or look at closely (the keepers' roll in the nave, the wages chalked inside the Whitlock barn's door,
 * Tilly's father's orders slate, Sheriff Odell's report, Ruth Calder's journal and code book, the Foreman's payroll book,
 * the boards where it happened). Interact reads it: its words open as a document on the offer card's reading mode (Title
 * over Text, a worn-brass edge), and what's said about it after (Ellis, Hob) plays as captions once the card is put away.
 * With no Text it's only looked at: the lines play at once. Either way the missions hear Readable.<ReadableId> (EventFor),
 * so an optional objective counts it (an Event objective), and the Interact event as any use.
 *
 * Its model is the dressing's or its own Mesh (set by the build script); it's there while ShownWhen holds (empty: always),
 * hidden and off the Interact key otherwise. A step's reading (the mission gates, 2026-10-27) is read only while UsableWhen
 * holds (from its step; the eleven's tags in their step alone, so the prompt goes once they're taken), seen but never
 * focused otherwise, and with bHoldToRead it takes a short hold (MissionGates::HoldSeconds), not a tap. Lore: neither. Never ticks. Painted words follow the played hero (Story/HeroLines): as play
 * begins its own Mesh becomes the hero's in HeroMeshes (the headboard carved with their name), else OtherMesh for any hero
 * but Ellis (the keepers' roll with no ELLIS under ABEL), else stays as placed. Its words are swapped where they're shown.
 *
 * From a build script (Python):
 *     roll = actors.spawn_actor_from_class(unreal.ReadableProp, unreal.Vector(x, y, z))
 *     roll.set_editor_property('readable_id', 'KeepersRoll')
 *     roll.set_editor_property('title', unreal.Text("The keepers' roll"))
 *     roll.set_editor_property('text', unreal.Text('ADA. ... ABEL. ...'))
 *     roll.set_editor_property('lines', [story_line('Hob', "Heir to the family business, sunshine. Funny how that goes.")])
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AReadableProp : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag every readable carries. */
	static const FName ReadableTag;

	AReadableProp();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	/** What it is, grown to at least a hand's size round it (InteractionFocus::GrownBox). */
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	/**
	 * Reads it for Player (their pawn or controller), as a tap of Interact does: the card (or only the lines), the event.
	 * False when it isn't there to read, its step hasn't come (UsableWhen), or a menu is up.
	 */
	UFUNCTION(BlueprintCallable, Category = "Readable")
	bool Read(AActor* Player);

	/** It's there to read now (ShownWhen). */
	bool ShouldShow() const;

	/** Its step lets it be read now (UsableWhen; empty: always). Shown, it's read. */
	bool CanRead() const;

	bool IsShown() const { return bShownNow; }

	/** Shows or hides it as its story stands (the runner's changes call it; so can tests). */
	void Refresh();

	/** "Readable.KeepersRoll": the event reading it sends. */
	static FName EventFor(FName ReadableId);

	/** "Readable_KeepersRoll": the tag it carries, so an objective's arrow can point at it. */
	static FName TagFor(FName ReadableId);

	/** The event it sends: Readable.<ReadableId>, or Readable.<actor's name> without an id. */
	FName GetReadEvent() const;

	/** Its lines said after reading, each naming its speaker (none named: the title's). */
	TArray<FStoryLine> GetLinesToSay() const;

	/** The model the hero HeroId sees here: HeroMeshes' own, else OtherMesh for any hero but Ellis; null: Mesh as placed. */
	UStaticMesh* PickMeshFor(FName HeroId) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** What it is when it brings its own model (a book, a slate); empty when the dressing's is it. Found by the Interact key's line. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** What the missions call it: reading it sends Readable.<ReadableId> ("KeepersRoll", "WagesChalk", "RuthJournal"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FName ReadableId;

	/** What it is, over its words ("The keepers' roll", "Sheriff Odell's report"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FText Title;

	/** A small line over the title, what kind of thing it is ("Painted on a plank in the nave", "Chalk on the barn door"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FText Heading;

	/** What's written on it, shown as a document. Empty: nothing to read, only to look at (its lines say the rest). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable", meta = (MultiLine = true))
	FText Text;

	/** What's said about it once it's read (Ellis, Hob), as captions; a line with no speaker in Title's name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	TArray<FStoryLine> Lines;

	/** The prompt's words ("Read", "Look"). Empty: Read, or Look when there's nothing written. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FText Prompt;

	/** How far from the player's eyes it can be read (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 250.f;

	/** When it's there to read (the sheriff's hat once Main 5 reaches the den). Empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FStoryCondition ShownWhen;

	/**
	 * When it can be read, while it's shown: any of these holds (a step's reading: Back Pay's payroll book from its step and
	 * after the mission; the eleven's tags in their step alone). Empty: whenever it's shown (lore, any time). Before then
	 * it's seen but never focused. Python: set_editor_property('usable_when', [condition, ...]).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	TArray<FStoryCondition> UsableWhen;

	/**
	 * A step's reading: a short hold of Interact (MissionGates::HoldSeconds), not a tap, so a stray press never does the
	 * step. Off (lore): a tap. Python: set_editor_property('hold_to_read', True).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	bool bHoldToRead = false;

	/**
	 * The story flag reading it sets (FCampaignRecord::StoryFlags), after its event: a Ledger page's Ledger.Page.<Town> opens
	 * the town's page; a letter's Letter.Deacon.<Where>. None: none. Reading it again sets nothing new.
	 * Python: set_editor_property('sets_flag', 'Ledger.Page.TheNeedles').
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable")
	FName SetsFlag;

	/**
	 * A hero's own model in place of Mesh, by UHeroDefinition::Id: the headboard carved with their name
	 * (SM_Headboard_<Id>; build_area_story.py's place_headboards). Python: set_editor_property('hero_meshes', {...}).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable|Heroes")
	TMap<FName, TObjectPtr<UStaticMesh>> HeroMeshes;

	/**
	 * Any hero but Ellis with no model of their own in HeroMeshes sees this one (SM_KeepersRoll_Bare). None: Mesh as
	 * placed. Only for a readable that brings its own Mesh. Python: set_editor_property('other_mesh', mesh).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Readable|Heroes")
	TObjectPtr<UStaticMesh> OtherMesh;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleRunnerChanged();
	void SetShown(bool bShown);

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
	bool bShownNow = true;
};
