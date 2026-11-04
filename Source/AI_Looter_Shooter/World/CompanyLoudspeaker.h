#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Story/StoryCondition.h"
#include "Story/StoryLine.h"
#include "CompanyLoudspeaker.generated.h"

class AController;
class UCapsuleComponent;
class UHealthComponent;
class USceneComponent;
class UStaticMeshComponent;
class UStoryLineSet;

/**
 * One thing Vane says from a loudspeaker (ACompanyLoudspeaker::Cues): said once a visit, the first time its story holds
 * while the player is in earshot (the town gate's "Main Street is closed for assessment" as Main 3's fight begins).
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FLoudspeakerCue
{
	GENERATED_BODY()

	/** When it's said (read from the campaign record; an empty condition: as soon as the player is in earshot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loudspeaker")
	FStoryCondition When;

	/** What Vane says (a line set of /Game/Data/Story: Tools/Unreal/create_story_lines.py). None: nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loudspeaker")
	TObjectPtr<UStoryLineSet> Lines;
};

/**
 * A Vane Consolidated loudspeaker post (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 1: "its horns can be shot
 * (sparks, a crackle, Vane's line cut mid-word)"): a 4 m pole with two brass horns and a VC plate, at the Keeper's Gate
 * (Main 2), the town gate (Main 3) and on Main Street. Vane talks from it through UVaneBroadcast (the radio crackle, his
 * captions, the hiss), either when the story asks (Cues: once a visit each, the first time its condition holds with the
 * player in earshot, and only into a quiet moment so he never drops what someone else is saying) or when a thing in the
 * world tells it to (Broadcast: the hose-cart's valve shut on the boards).
 *
 * Its horns are a target: a few shots (HornsHealth) silence it for the visit. Silenced, the horns burst in sparks with a
 * crackle, and if Vane was on the air from it he's cut off mid-word ("I'm adding it to your bi-"). Missions count it by
 * its tag as a kill (UMissionKillNamedObjective, Main 2's optional "Silence Vane's loudspeaker"). It comes back whole the
 * next time the level loads (the Company mends its horns): nothing is saved.
 *
 * With bShowPost off it's a voice only (no post, no horns, nothing to shoot): where another lane's post already stands
 * (WORLD's instanced posts on Main Street) the voice is placed at its horns. Its look: ModelPath, the user's Tripo post
 * (SM_LoudspeakerPost, TRIPO_MODELS.md 18: 4 m, its horns and plate on it facing +X, socket Horns), its foot set on the
 * ground from its bounds and its horns' target at the socket (the barge's horns, 2026-11-04: the stand-in's pale post was
 * out of the Company's palette). Without it the stand-in: an existing fence post stretched to 4.2 m, two engine cones in
 * brass, a black plate. Tagged Obstacle (the minimap) and LoudspeakerTag.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACompanyLoudspeaker : public AActor
{
	GENERATED_BODY()

public:
	ACompanyLoudspeaker();

	/** Every loudspeaker carries it. */
	static const FName LoudspeakerTag;

	/** The Tripo model (one path): used in place of the stand-in's post, horns and plate when it's in the project. */
	static const TCHAR* const ModelPath;

	/**
	 * Vane says Lines from here now (the crackle at its horns, then UVaneBroadcast: his captions under the hiss). False when
	 * it's silenced or Lines is empty: a silenced horn says nothing.
	 */
	bool Broadcast(const TArray<FStoryLine>& Lines);

	/** Its horns shot out (By, or nobody): sparks, the crackle, Vane cut off if he was talking from here. False if already. */
	bool Silence(AController* By);

	bool IsSilenced() const { return bSilenced; }

	/** Vane is on the air from this one now. */
	bool IsOnAir() const;

	/** Looks at its cues now: says the first one due (the timer's work, twice a second; the tests call it). */
	void CheckCues();

	/** The loudspeaker carrying Tag in Context's level (the Keeper's Gate's: Loudspeaker_KeepersGate), or null. */
	static ACompanyLoudspeaker* FindTagged(const UObject* Context, FName Tag);

	/**
	 * Words cut off mid-word, as a horn shot out cuts Vane: about three quarters of them, ending inside a word, with a dash
	 * ("I'm adding it to your bill." -> "I'm adding it to your bi-", the dash an em dash). Short words keep a letter or two.
	 */
	static FString CutOff(const FString& Words);

	/** Its middle where the horns are (the crackle's and the sparks' spot). */
	FVector GetHornsLocation() const;

	// --- Its parts ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The pole (what the player bumps into). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Post;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HornLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HornRight;

	/** The VC plate under the horns. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Plate;

	/** What shots hit (only the Weapon channel): round both horns, a little bigger than them, so a near miss counts. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> HornsHit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> Health;

	// --- What it says, and how it takes a shot ---

	/** Said once a visit each, in order of need: the first whose condition holds with the player in earshot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loudspeaker")
	TArray<FLoudspeakerCue> Cues;

	/** How near (cm) the player must be for a cue to be said from here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loudspeaker", meta = (ClampMin = "0", Units = "cm"))
	float EarshotRadius = 6000.f;

	/** A cue due waits this long (s) at most for the captions to fall quiet before Vane breaks in anyway. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loudspeaker", meta = (ClampMin = "0", Units = "s"))
	float QuietWaitSeconds = 6.f;

	/** Its post, horns and plate stand (and can be shot). Off: a voice only, nothing seen or hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loudspeaker")
	bool bShowPost = true;

	/** The damage its horns take before they're silenced (a few shots from a gun of the area's band). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loudspeaker", meta = (ClampMin = "1"))
	float HornsHealth = 60.f;

	/** The pole's height (cm): the stand-in post is stretched to it; the horns sit 40 cm under its top. */
	static constexpr float PostHeight = 420.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Shows the stand-in or the Tripo model, or nothing (a voice only). */
	void ApplyLook();

	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** Each cue said this visit (by index), and when each due one began waiting for quiet (world seconds; <0: not waiting). */
	TArray<bool> CueSaid;
	TArray<double> CueDueSince;
	FTimerHandle CueTimer;
	bool bSilenced = false;
	/** It began the broadcast on the air now (UVaneBroadcast's one voice: a later one from elsewhere takes over). */
	bool bOnAir = false;
};
