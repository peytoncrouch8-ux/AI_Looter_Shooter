#pragma once

#include "CoreMinimal.h"

class AActor;
class UPointLightComponent;
class UPrimitiveComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UTextureRenderTarget2D;

/**
 * What the off-screen stands share (the loadout's showcase gun, the bestiary's model, the gunsmith's bench's gun): a spot
 * far outside any level, a camera that renders only its own actor into a picture the screen shows, and studio lights on
 * lighting channel 1 that light nothing else.
 */
namespace StageStudio
{
	/**
	 * Far outside any level, so nothing there shadows or lights a stand, and the minimap's top-down bake never sees it. Each
	 * stand has its own index: 0 the loadout's character stand-in (ALoadoutStage, not shown since the 2026-10-08 redesign),
	 * 1 the bestiary's, 2 the bench's, 3 the loadout's showcase gun (ALoadoutGunStage).
	 */
	AI_LOOTER_SHOOTER_API FVector Location(int32 StandIndex);

	/** Seen only by a stand's camera, lit only by its studio lights (channel 1), and never in the way of anything. */
	AI_LOOTER_SHOOTER_API void SetupPrimitive(UPrimitiveComponent* Primitive);

	/**
	 * Renders on demand, into scene color with inverse opacity in alpha (so the screen can cut the model out; it tone maps
	 * the color itself), and only the actors on its show-only list, without sky, fog or effects.
	 */
	AI_LOOTER_SHOOTER_API void SetupCapture(USceneCaptureComponent2D* Capture);

	/** A studio light on channel 1, off until the stand is active. Call from the stand's constructor. */
	AI_LOOTER_SHOOTER_API UPointLightComponent* MakeLight(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, const FVector& Location,
		const FLinearColor& Color, float Candelas, bool bShadows);

	/** The picture a stand renders into: rendered larger than it's shown, with mips to keep it smooth when downsized. */
	AI_LOOTER_SHOOTER_API UTextureRenderTarget2D* MakeRenderTarget(UObject* Outer, FName Name, int32 Width, int32 Height);

	/** Where a world point shows in the capture's picture, as 0..1 across and down. False when it's behind the camera. */
	AI_LOOTER_SHOOTER_API bool ProjectToImage(const USceneCaptureComponent2D* Capture, int32 Width, int32 Height, const FVector& WorldLocation,
		FVector2D& OutUV);
}
