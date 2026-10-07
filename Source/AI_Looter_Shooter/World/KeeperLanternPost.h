#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "KeeperLanternPost.generated.h"

class AKeeperLanternPost;
class UBoxComponent;
class UMaterialInterface;
class UMissionRunner;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnKeeperLanternRelit, AKeeperLanternPost& /*Post*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnKeepersLanternLit, AKeeperLanternPost& /*Post*/);

/**
 * One of the three keeper's lantern posts on the burial boards deck at Gravewind Point (Docs/Areas/RansomsRest.md, "The boss:
 * Abel Ransom, the Keeper"; Art/Models/Props/BurialDeck.py's KeeperLanternPost: a 3.2 m post with an iron lantern hanging
 * from its arm, SOCKET_Light at its flame, SOCKET_Interact at the lantern, SOCKET_Hang the hook under the arm). The build
 * script stands one on each of the deck's SOCKET_LanternPost_1..3 (Tools/Unreal/build_area_deck.py), tagged LanternPost_Deck.
 *
 *  - Its own lantern is lit: the post's LanternGlow glass. In Abel's second phase it goes dark (SetDark: the glass shows the
 *    house trim's window glass, as the Keeper's Lantern's does while dark), and holding Interact for 1.5 s ("Relight the
 *    lantern") lights it again and tells Abel (OnRelit): relit, the three drag him back out of the fog.
 *  - The keeper's post by the entrance (bKeepersPost, also tagged LanternPost_Keeper) takes the Keeper's Lantern: a tap
 *    ("Hang the Keeper's Lantern") while HangWhen holds (Main 6's second step) and Ellis has it (AKeepersLantern::IsTaken)
 *    hangs SM_KeepersLantern dark from SOCKET_Hang by its SOCKET_Grip; the player's interaction component tells the
 *    missions. After the fight Abel lights it from his ghost light (LightKeepersLantern): its glass glows, a shadowless
 *    light comes on in it, and it leans on its hook toward FlameBearing (north-east, over the ridges, toward Ned: Main 7's
 *    way), as a lantern leans toward a saint's light.
 *
 * What the story keeps of it is read from the campaign record, not saved apart: the Keeper's Lantern hangs there from
 * Main 6's third step on (HungWhen) and is lit and leaning once Main 6 is finished (LitWhen). A box round the post's lantern
 * is what the Interact key's line finds. It never ticks; its light is off unless bCastsLight (the deck's lanterns are
 * emissive glass, and the area keeps to three shadowless lights in view).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AKeeperLanternPost : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** Every post on the deck, and the keeper's post by the entrance (where Main 6 hangs the lantern). */
	static const FName DeckTag;
	static const FName KeepersPostTag;

	AKeeperLanternPost();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	virtual void OnConstruction(const FTransform& Transform) override;

	// --- Its own lantern (Abel's second phase) ---

	/** Puts its lantern out (Abel's bell) or lights it again at once (his fight over or started over): no event either way. */
	void SetDark(bool bInDark);
	bool IsDark() const { return bDark; }

	/** Lights it again as a held Interact does (the console, a test): OnRelit hears it. False when it's lit already. */
	bool Relight(AActor* ByWhom);

	/** Someone relit it (Abel counts them). */
	FOnKeeperLanternRelit OnRelit;

	// --- The Keeper's Lantern (the keeper's post) ---

	/** It can take the lantern now: the keeper's post, nothing hung yet, HangWhen holds and Ellis has the lantern. */
	bool CanHang() const;

	/** Hangs the Keeper's Lantern on it, dark, as a tap of Interact does. False when it can't now (bForce: whatever the story says). */
	bool Hang(AActor* ByWhom, bool bForce = false);

	bool IsKeepersLanternHung() const { return bHung; }

	/** Abel lights it from his ghost light: its glass glows, its light comes on, it leans toward FlameBearing. Hangs it first if needed. */
	void LightKeepersLantern();

	bool IsKeepersLanternLit() const { return bLanternLit; }

	/**
	 * Abel has just lit the Keeper's Lantern (in his scene, or its end state): what shows its flame hears it here (Main 7's
	 * leaning flame). A level loaded after Main 6 finds it lit from the start without this (LitWhen).
	 */
	FOnKeepersLanternLit OnKeepersLanternLit;

	/** The Keeper's Lantern on its hook: what its flame hangs in. */
	UStaticMeshComponent* GetKeepersLanternMesh() const { return KeepersLantern; }

	/** Which way its flame leans (flat, world): FlameBearing's compass direction (X north, Y east). */
	FVector GetLeanDirection() const;

	/** The Keeper's Lantern's globe, where it's lit (world); the hook's place before it's hung. */
	FVector GetKeepersLanternGlobe() const;

	/** Reads the story again: the lantern hung or not, lit or not, as the campaign record has it. */
	void RefreshStory();

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The post with its own lantern (SM_KeeperLanternPost). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Post;

	/** Round the post's lantern: what the Interact key's line finds; it blocks nothing else. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> Grip;

	/** The post's own lantern's light, only with bCastsLight. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> PostLight;

	/** The Keeper's Lantern on the hook (SM_KeepersLantern), hidden until it's hung. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> KeepersLantern;

	/** Its light once Abel has lit it: shadowless, small. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> KeepersLight;

	// --- Settings ---

	/** The keeper's post, by the deck's entrance: it takes the Keeper's Lantern. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post")
	bool bKeepersPost = false;

	/** The lantern can be hung while this holds (Main 6's second step, from 0: 1). Empty: whenever Ellis has it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post")
	FStoryCondition HangWhen;

	/** It hangs there from the start while any of these holds (Main 6 from its third step; after Main 6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post")
	TArray<FStoryCondition> HungWhen;

	/** It's lit and leaning from the start while this holds (after Main 6). Empty: only Abel lights it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post")
	FStoryCondition LitWhen;

	/** How long Interact is held to relight a dark lantern (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post", meta = (ClampMin = "0.1", Units = "s"))
	float RelightSeconds = 1.5f;

	/** How far from the player's eyes it can be used (cm): the lantern hangs over a man's head. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 350.f;

	/** The compass bearing the lit Keeper's Lantern leans toward (0 north, 90 east): north-east, over the ridges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Keeper's Lantern", meta = (ClampMin = "0", ClampMax = "360"))
	float FlameBearing = 45.f;

	/** How far it leans on its hook (degrees): plainly more than any wind would hold it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Keeper's Lantern", meta = (ClampMin = "0", ClampMax = "45"))
	float LeanDegrees = 16.f;

	/** The models' glass slot (LanternGlow), and what it shows while dark: the house trim's window glass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Glass")
	FName GlassSlot = TEXT("LanternGlow");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Glass")
	TObjectPtr<UMaterialInterface> DarkGlass;

	/** The post's own lantern casts a real light (off: its glass glows alone; at most three shadowless lights in view). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Light")
	bool bCastsLight = false;

	/** The sockets: the post's lantern's flame and middle, its hook; the Keeper's Lantern's grip and flame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Sockets")
	FName LightSocket = TEXT("Light");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Sockets")
	FName InteractSocket = TEXT("Interact");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Sockets")
	FName HangSocket = TEXT("Hang");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern Post|Sockets")
	FName GripSocket = TEXT("Grip");

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Puts the box, the lights and the hung lantern where the post's sockets are (or where BurialDeck.py builds them). */
	void FitToModel();
	/** The glass of Mesh lit or dark; Lit keeps the slot's own glow material the first time it's seen. */
	void ApplyGlass(UStaticMeshComponent* Mesh, bool bLit, TObjectPtr<UMaterialInterface>& Lit);
	void ApplyLantern();
	void HandleMissionsChanged();
	bool StoryHolds(const FStoryCondition& Condition) const;

	/** The post's and the Keeper's Lantern's own glow materials, kept for lighting them again. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PostGlow;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LanternGlow;

	bool bDark = false;
	bool bHung = false;
	bool bLanternLit = false;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
