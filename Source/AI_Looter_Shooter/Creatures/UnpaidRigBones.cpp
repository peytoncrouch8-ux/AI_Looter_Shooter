// The Unpaid's bone names (UnpaidRigBones.h), as Art/Models/Creatures/Unpaid.py names them.

#include "Creatures/UnpaidRigBones.h"

FUnpaidArmBones::FUnpaidArmBones(const TCHAR* Side)
	: UpperArm(*FString::Printf(TEXT("upperarm_%s"), Side))
	, LowerArm(*FString::Printf(TEXT("lowerarm_%s"), Side))
	, Hand(*FString::Printf(TEXT("hand_%s"), Side))
{
	for (const TCHAR* Finger : { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
	{
		Fingers.Emplace(*FString::Printf(TEXT("%s_01_%s"), Finger, Side));
	}
}

FUnpaidRigBones::FUnpaidRigBones()
	: Pelvis(TEXT("pelvis"))
	, Spine(TEXT("spine_01"))
	, Chest(TEXT("spine_02"))
	, Coal(TEXT("coal"))
	, Neck(TEXT("neck"))
	, Head(TEXT("head"))
	, Jaw(TEXT("jaw"))
	, Hat(TEXT("hat"))
	, LeftArm(TEXT("l"))
	, RightArm(TEXT("r"))
{
	for (int32 Link = 1; Link <= 5; ++Link)
	{
		Shroud.Emplace(*FString::Printf(TEXT("tail_%02d"), Link));
	}
	for (int32 Link = 1; Link <= 2; ++Link)
	{
		LeftStrip.Emplace(*FString::Printf(TEXT("tail_l_%02d"), Link));
		RightStrip.Emplace(*FString::Printf(TEXT("tail_r_%02d"), Link));
	}
}
