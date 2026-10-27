#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "StartupLogosSettings.generated.h"

/**
 * The startup cards' words and pictures (UStartupLogosWidget), set without code: Project Settings > Game > Startup Logos,
 * saved to Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.StartupLogosSettings]. The studio's name is a
 * placeholder the user renames later (2026-10-27); never a real studio's or company's name until it's theirs.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Startup Logos"))
class AI_LOOTER_SHOOTER_API UStartupLogosSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UStartupLogosSettings& Get();

	/** The studio card's name, set big under its mark. */
	UPROPERTY(Config, EditAnywhere, Category = "Studio")
	FString StudioName = TEXT("PLACEHOLDER STUDIO");

	/** A small line under the studio's name; empty shows none. */
	UPROPERTY(Config, EditAnywhere, Category = "Studio")
	FString StudioTagline;

	UPROPERTY(Config, EditAnywhere, Category = "Studio")
	bool bShowStudioCard = true;

	/**
	 * The engine card's picture: Epic's official Unreal Engine logo, imported as it comes (white on transparent). Until
	 * it's imported the card shows a plain "Powered by Unreal Engine" in the kit's type. Shown unaltered, at its own
	 * shape, on the plain dark ground with clear space round it (Epic's logo rules): never tinted, cropped or animated
	 * beyond its fades. Packaged builds cook /Game/UI/Logos (DirectoriesToAlwaysCook), since only this path names it.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Engine", meta = (AllowedClasses = "/Script/Engine.Texture2D"))
	FSoftObjectPath EngineLogo = FSoftObjectPath(TEXT("/Game/UI/Logos/T_UnrealEngineLogo.T_UnrealEngineLogo"));

	/** How tall the logo shows on a 1080-line screen (its width follows its own shape). */
	UPROPERTY(Config, EditAnywhere, Category = "Engine", meta = (ClampMin = "40", ClampMax = "600"))
	float EngineLogoHeight = 220.f;

	/**
	 * "POWERED BY" set over the logo. Off for an official lockup that already says it; the plain wordmark shown before
	 * the logo is imported always says it.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Engine")
	bool bEngineCaption = false;

	UPROPERTY(Config, EditAnywhere, Category = "Engine")
	bool bShowEngineCard = true;
};
