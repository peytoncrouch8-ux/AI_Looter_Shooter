#pragma once

#include "CoreMinimal.h"
#include "UnpaidRigBones.generated.h"

/** One arm's bones in SK_Unpaid, from the shoulder out. */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FUnpaidArmBones
{
	GENERATED_BODY()

	FUnpaidArmBones() = default;

	/** The arm on one side, named as Unpaid.py names it: Side is "l" or "r". */
	explicit FUnpaidArmBones(const TCHAR* Side);

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName UpperArm;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName LowerArm;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Hand;

	/** A bone per finger, thumb first and little finger last: they curl together and fan out round the middle one. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> Fingers;
};

/**
 * The bones AUnpaidCreature poses, by their names in SK_Unpaid (Art/Models/Creatures/Unpaid.py). If the model names one
 * differently, set the name in Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.UnpaidCreature] (Rig=(...)): no code
 * change. The pelvis, spine, chest, neck, head and the arms down to the hands are needed; a jaw, coal, finger or tail bone
 * the model lacks is simply left at rest.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FUnpaidRigBones
{
	GENERATED_BODY()

	FUnpaidRigBones();

	/** Carries the body: it hovers and leans here. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Pelvis;

	/** The waist and the chest. A shot on the chest's (or the waist's, or the coal's) hit zone may find the coal. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Spine;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Chest;

	/** Where the coal burns through the chest, at about 1.3 m: the crit spot is found round it. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Coal;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Neck;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Head;

	/** Drops for the shriek. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Jaw;

	/** Carries the hat, which is a mesh of its own (SM_UnpaidHat) and follows the head. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	FName Hat;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FUnpaidArmBones LeftArm;

	UPROPERTY(EditAnywhere, Category = "Rig")
	FUnpaidArmBones RightArm;

	/** The shroud's chain, top first, each bone the child of the one before. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> Shroud;

	/** The short chains of the outer strips at its sides, top first. */
	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> LeftStrip;

	UPROPERTY(EditAnywhere, Category = "Rig")
	TArray<FName> RightStrip;
};
