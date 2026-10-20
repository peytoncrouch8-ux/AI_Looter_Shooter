#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberPropActor.generated.h"

class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Something an ember power sets down in the world for a while (Grave Ward's lantern, Tripwire's two stakes): one of the
 * game's baked props (never a mesh made in play), shown but never touched (no collision: nothing walks into it, no shot
 * stops on it), with an optional small light (movable, no shadows). The power that set it lights it and takes it away
 * (Remove).
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AEmberPropActor : public AActor
{
	GENERATED_BODY()

public:
	AEmberPropActor();

	/**
	 * A prop of the model at MeshPath (an object path; nothing shown when it isn't in the game) standing on Ground facing
	 * Yaw, scaled so: its lowest point on the ground. Null without a world.
	 */
	static AEmberPropActor* Place(UWorld* World, const TCHAR* MeshPath, const FVector& Ground, float Yaw, const FVector& Scale);

	/** A light Offset over the prop's ground, in Color, Candela bright, reaching Radius cm. */
	void SetLight(const FLinearColor& Color, float Candela, float Radius, const FVector& Offset);
	/** The light's brightness now (shown only when the quality allows power lights). */
	void SetLightIntensity(float Candela);

	/** Whether a power may light the ground: High and Epic only (an extra light costs ~0.3 ms on Medium). */
	static bool LightsAllowed();

	/** The prop's model loaded (shown); false where the asset isn't in the game (the power still works, drawn without it). */
	bool HasModel() const;

	/** Where its top is (the lantern's glass, a stake's head): the model's bounds' top over its ground. */
	FVector GetTop() const;

	/** Takes it away now. */
	void Remove();

	/** The model at Path, or null without a warning while it isn't in the game. Looked for again while missing. */
	static UStaticMesh* FindMesh(const TCHAR* Path);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Model;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> Light;
};
