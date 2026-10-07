#pragma once

#include "CoreMinimal.h"
#include "Story/AmosPoses.h"
#include "Story/StoryCharacter.h"
#include "Story/StoryCondition.h"
#include "AmosWhitlock.generated.h"

class UPoseableMeshComponent;
class USpeakerPointComponent;
class UStaticMeshComponent;

/**
 * Amos Whitlock at his fence in Whitlock Fields (Docs/Areas/RansomsRest.md, Side 2 "Unfinished Business"; Docs/Story.md: "A
 * farmer who died last harvest with his hay half in ... One of the Unpaid, but not angry yet"): a friendly Unpaid only
 * Ellis can see, a story character, never a creature. The build script stands him on his fence's line midway between two
 * posts, by the gate where the fields path passes, facing across the fence into his hayfield (Tools/Unreal/
 * build_area_whitlock.py), tagged Speaker_Amos, there from Main 4's finish (ShownWhen, when Side 2 opens).
 *
 * His body is SK_Amos (Art/Models/Creatures/Amos.py, the user's "hayman") with SM_AmosHat on his hat bone and SM_AmosFork
 * on his fork bone, posed by code from the model's pose table (AmosPoses, on a poseable mesh: no animation runs). Until
 * Side 2 is done he leans on the fence's top rail, his forearms folded on it, his fork against the post at his right; once
 * it's done (SitWhen) "he sits on his fence to wait for the saint to come back": the rail under him, his head turned into
 * the low sun. He settles onto it when his last words have ended, drifting there over SitSeconds; a level begun after
 * Side 2 finds him sitting. His coal is banked low (a dull ember under ash: custom primitive data, as M_Ghost reads it).
 *
 * Talked to (his speaker point at his mouth, his hat's SOCKET_Speaker; topics by the story, from create_story_lines.py),
 * he turns his head to the listener and his jaw moves while his lines play. He's a ghost: nothing meets him but the
 * Interact key's line (a hidden hull about his body), so the player and shots pass through him, and he casts no shadow.
 * Nothing of him ticks unless he's talking, turning back or settling onto the rail.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AAmosWhitlock : public AStoryCharacter
{
	GENERATED_BODY()

public:
	/** The tag the missions and the console find him by. */
	static const FName SpeakerTag;

	/** His side mission ("Unfinished Business"), after which he sits on his fence. */
	static const FName MissionId;

	/** His model and props (Art/Models/Creatures/Amos.py), and the hat's socket at his mouth. */
	static const TCHAR* const ModelPath;
	static const TCHAR* const HatPath;
	static const TCHAR* const ForkPath;
	static const FName SpeakerSocket;

	/** His banked coal's ember (Amos.py's EMBER_COLOR, sRGB #96421F) as the rank color M_Ghost reads, linear. */
	static FLinearColor EmberColor();

	AAmosWhitlock();

	/** The story's conditions: there or not, and leaning or sitting (settling onto the rail when his words are done). */
	virtual void RefreshShown() override;

	/** His head and jaw while he talks, turning back after, and settling onto the rail. */
	virtual void UpdatePose(float DeltaSeconds) override;

	/** Sits him on his fence now (the console, tests): settling over SitSeconds, or bAtOnce in place. He stays sitting. */
	void SitNow(bool bAtOnce = false);

	/** Back to leaning on the rail at once (the console). The story sits him again the next time it looks, if SitWhen holds. */
	void LeanNow();

	/** On the rail and settled. */
	bool IsSitting() const { return Seat == EAmosPose::Sit && !bSettling; }

	/** Drifting from the lean onto the rail. */
	bool IsSettling() const { return bSettling; }

	/** The pose he's in, or settling into: Lean or Sit. */
	EAmosPose GetSeat() const { return Seat; }

	/** The story (or the console) wants him on the rail: SitWhen holds. */
	bool WantsToSit() const;

	/** His model was found (SK_Amos): posed by code, rather than the story character's placeholder. */
	bool HasModel() const;

	/** Where the figure stands in the actor for a seat (the actor is on the fence's line): behind it leaning, on it sitting. */
	FTransform GetSeatTransform(EAmosPose InSeat) const;

	/** Where his head is now (world): where he looks from, and where his words come from. */
	FVector GetHeadLocation() const;

	/** How far his head is turned to whoever he's talking to now (degrees), and how open his jaw is (0-1). */
	float GetHeadYaw() const { return HeadYaw; }
	float GetJawOpen() const { return JawOpen; }

	/** His body: SK_Amos, posed by code (a poseable mesh: no animation runs). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPoseableMeshComponent> Figure;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Fork;

	/** When he sits on his fence: Side 2 finished. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Amos")
	FStoryCondition SitWhen;

	/** How long he takes to settle from the lean onto the rail (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Amos", meta = (ClampMin = "0", Units = "s"))
	float SitSeconds = 2.f;

	/** His coal's ember: how strongly it glows (M_Ghost's rank color alpha: a Basic coal's is 1.8) and its heat (-1 out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Amos", meta = (ClampMin = "0", ClampMax = "2"))
	float EmberStrength = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Amos", meta = (ClampMin = "-1", ClampMax = "0"))
	float EmberHeat = -0.5f;

	/** How far he turns his head to whoever he talks to (degrees), and how fast (degrees a second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Amos", meta = (ClampMin = "0", ClampMax = "80"))
	float HeadTurnLimit = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Amos", meta = (ClampMin = "1"))
	float HeadTurnSpeed = 80.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleTalked(USpeakerPointComponent& Point, AActor* InListener);

	/** The banked ember on his body and props (custom primitive data, as M_Ghost reads it). */
	void ApplyLook();

	/** Puts the figure where the seat (or the settling) has it and poses its bones, with the head and jaw on top. */
	void PoseFigure();

	/** The hull the Interact key's line finds, and the speaker point without the hat's socket, for the seat. */
	void PlaceHull();

	/** Starts settling onto the rail (or sits him there at once). */
	void StartSitting(bool bAtOnce);

	/** Something of him moves: talking, his head or jaw on their way back, settling. */
	bool NeedsTick() const;

	/** Ticks only while something of him moves. */
	void UpdateTicking();

	/** The pose's own turn of his head from his body's front (degrees): the sit looks into the sun. */
	float PoseHeadYaw() const;

	/** The lean, or the sit, or between them while settling (0 leaning, 1 sitting). */
	EAmosPose Seat = EAmosPose::Lean;
	bool bSettling = false;
	float SettleClock = 0.f;

	/** The console sat him: he stays on the rail whatever the story says. */
	bool bSatByHand = false;

	/** Play has begun: from then on the story's changes settle him rather than snap him. */
	bool bBegun = false;

	TWeakObjectPtr<AActor> Listener;
	float HeadYaw = 0.f;
	float JawOpen = 0.f;
	float SpeakClock = 0.f;
};
