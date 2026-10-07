#pragma once

#include "CoreMinimal.h"
#include "Story/StoryCharacter.h"
#include "AbelOnBoard.generated.h"

class UPointLightComponent;
class UPoseableMeshComponent;
class USpeakerPointComponent;
class UStaticMeshComponent;

/**
 * Abel on his burial board after his fight, for the rest of the game (Docs/Areas/RansomsRest.md: "He sits on his board
 * facing the sunset"; "Abel isn't fought again; he's a friend now", with new lines after each ember): a story character,
 * not a creature (Abel the boss is AAbelKeeper, Bosses/). The build script puts him on his bier's SOCKET_Sit, facing as the
 * socket faces (the sunset), tagged Speaker_Abel (Tools/Unreal/build_area_deck.py), shown after Main 6 (ShownWhen).
 *
 * His body is SK_Abel posed once in the pose table's sit (AbelPoses::Solve, on a poseable mesh: no animation runs) at his
 * 1.3, with his hat, ghost lantern and spectral pump on their bones, his coal sunk to a gold ember, and his ghost light's
 * shadowless glow at the lantern's SOCKET_Light. Talked to (his speaker point at his head; topics by the story, from
 * create_story_lines.py), he turns his head to the listener and his jaw moves while his lines play; otherwise nothing of him
 * moves. A hidden hull about his seated size stops the player and finds the Interact line; shots and pellets pass through.
 *
 * The scene after the fight (SitWithPa) shows him the moment the boss sits down on the board (ShowNow), before Main 6 is
 * recorded finished; from then on the story keeps him there.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AAbelOnBoard : public AStoryCharacter
{
	GENERATED_BODY()

public:
	/** The tag the missions and the console find him by. */
	static const FName SpeakerTag;

	/** His model and props (Art/Models/Creatures/Abel.py). */
	static const TCHAR* const ModelPath;
	static const TCHAR* const HatPath;
	static const TCHAR* const LanternPath;
	static const TCHAR* const PumpPath;

	AAbelOnBoard();

	virtual void RefreshShown() override;
	virtual void UpdatePose(float DeltaSeconds) override;

	/** Placed or edited: sat on his board in the editor too, as play shows him. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Shows him sat on his board now, whatever the story says until it next looks (the scene that sits him down). */
	void ShowNow();

	/** Forgets the scene's word: there only if the story says so (the console putting the boss back for another try). */
	void ForgetScene();

	/** Puts the seated pose on his body (again). False without his model. */
	bool ApplySeat();

	/** His body is posed in the sit. */
	bool IsSeated() const { return bSeated; }

	/** Where his head is in the sit (world): where he looks from, and where his speaker point is. */
	FVector GetHeadLocation() const;

	/** His body: SK_Abel, posed once in the sit by code (a poseable mesh: no animation runs). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPoseableMeshComponent> Figure;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lantern;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Pump;

	/** His ghost light at the lantern's globe: small, pale, never a shadow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> LanternLight;

	/** His size against his model (built at 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel", meta = (ClampMin = "0.5", ClampMax = "2"))
	float BodyScale = 1.3f;

	/** His coal sunk to an ember (M_Ghost's flare: -1 out, 0 as it burns). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel", meta = (ClampMin = "-1", ClampMax = "0"))
	float EmberHeat = -0.65f;

	/** How far he turns his head to whoever he talks to (degrees), and how fast (degrees a second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel", meta = (ClampMin = "0", ClampMax = "80"))
	float HeadTurnLimit = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abel", meta = (ClampMin = "1"))
	float HeadTurnSpeed = 70.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleTalked(USpeakerPointComponent& Point, AActor* Listener);
	/** His head turned by Yaw and his jaw open by Jaw (degrees), on the sit. */
	void PoseHead(float Yaw, float Jaw);
	/** The ember and the Boss rank's gold on his body and props (custom primitive data, as M_Ghost reads it). */
	void ApplyEmber();

	/** The sit as solved for his skeleton (component space, by bone index), and his head's and jaw's bones. */
	TArray<FTransform> Seated;
	TArray<FTransform> Rest;
	int32 HeadIndex = INDEX_NONE;
	int32 JawIndex = INDEX_NONE;
	bool bSeated = false;

	bool bShownByScene = false;
	TWeakObjectPtr<AActor> Listener;
	float HeadYaw = 0.f;
	float JawOpen = 0.f;
	float SpeakClock = 0.f;
};
