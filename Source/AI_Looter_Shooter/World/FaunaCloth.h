#pragma once

#include "CoreMinimal.h"
#include "World/FaunaActor.h"
#include "FaunaCloth.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/** One hung piece's swing (in play only). */
struct FFaunaClothSwing
{
	/** Swung out along the wind (degrees about the line) and how fast. */
	float Angle = 0.f;
	float Speed = 0.f;
	/** Its own place in the gusts and in its flutter. */
	float Phase = 0.f;
	float Flutter = 0.f;
};

/**
 * Washing moving in the wind (World/FaunaActor.h): sheets, shirts and towels pegged to a line, each swinging out along
 * the gusts on its own damped spring and fluttering, so a laundry line reads as a lived-in yard. Tools/Unreal/
 * build_area_fauna.py hangs pieces on the empty back line of every laundry line in the level (Skyreach's town); the
 * front line's washing is part of the line's model and stays still. Each mesh's pieces are one instanced component;
 * nothing moves unless the line is on screen and near.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFaunaCloth : public AFaunaActor
{
	GENERATED_BODY()

public:
	AFaunaCloth();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override { return true; }

	const TArray<FFaunaClothSwing>& GetSwings() const { return Swings; }

	/** Builds its pieces now (as play begins it), for the tests' worlds. */
	void SetupCloth();

	/** The pieces and where each hangs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth")
	TArray<FFaunaClothPiece> Pieces;

	/** The cloth meshes the pieces name, each hanging from its pivot (the middle of its top edge). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth")
	TArray<TObjectPtr<UStaticMesh>> Meshes;

	/** The way the wind blows (degrees; the smoke's lean), how far a gust swings a piece out and how long one lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth", meta = (Units = "Degrees"))
	float WindYaw = 19.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth", meta = (ClampMin = "0", ClampMax = "60", Units = "Degrees"))
	float SwingDegrees = 22.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth", meta = (ClampMin = "0", ClampMax = "20", Units = "Degrees"))
	float FlutterDegrees = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth", meta = (ClampMin = "0.5", Units = "s"))
	float GustSeconds = 5.f;

	/** A gust's snap (FaunaCues.h), now and then while a player is near. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth|Sound")
	FName FlapCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth|Sound", meta = (ClampMin = "0", Units = "cm"))
	float SoundRange = 1500.f;

protected:
	virtual void BeginPlay() override;

private:
	void PushTransforms();

	TArray<FFaunaClothSwing> Swings;
	/** Each mesh's component, and each piece's place in its component. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Parts;
	TArray<int32> PieceInstance;
	TArray<TArray<FTransform>> Poses;
	FRandomStream Random;
	float WindTime = 0.f;
	float SoundTimer = 0.f;
	FVector Center = FVector::ZeroVector;
	float Reach = 0.f;
	bool bReady = false;
};
