#pragma once

#include "CoreMinimal.h"
#include "LightingState.generated.h"

/**
 * One way a level can be lit (Day, Dusk), as Tools/Unreal/build_area_environment.py writes it from layout.json
 * level.environment: the sun by compass bearing and elevation, with its intensity, color temperature and shadows; the
 * sky light; the height fog's density and colors; the exposure; and the tints the material parameter collection
 * MPC_Lighting carries to what can't see the light itself (the unlit backdrop, the painted clouds). Lengths are cm,
 * colors linear. ALightingStates holds a level's states and ULightingStateSubsystem switches between them.
 *
 * The defaults are the tutorial island's afternoon exactly as the build script places it (its DEFAULTS), so a default
 * state and a level built without states agree.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FLightingState
{
	GENERATED_BODY()

	/** What scripts and the console call it (Day, Dusk); matched ignoring case. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting")
	FName Name = FName(TEXT("Day"));

	/** The compass bearing the sun stands at, in degrees: 0 north (+X), 90 east (+Y), 247.5 west-southwest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Sun")
	float SunBearing = 292.f;

	/** How high the sun stands over the horizon, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Sun", meta = (ClampMin = "-90", ClampMax = "90"))
	float SunElevation = 38.f;

	/** The sun's illuminance (lux). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Sun", meta = (ClampMin = "0"))
	float SunIntensity = 7.f;

	/** The sun's color temperature (kelvin): lower is warmer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Sun", meta = (ClampMin = "1700", ClampMax = "12000"))
	float SunTemperature = 5300.f;

	/**
	 * How far the sun's cascaded shadows reach from the camera (cm), before the quality preset scales it (70% on
	 * Medium). A low sun throws long shadows across more casters, so dusk caps it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Sun", meta = (ClampMin = "0"))
	float ShadowDistance = 10000.f;

	/** How many cascades share that distance (the quality preset caps it: two on Medium). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Sun", meta = (ClampMin = "0", ClampMax = "4"))
	int32 ShadowCascades = 2;

	/** The sky light's intensity. Its color comes from capturing the sky, which every switch does again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Sky", meta = (ClampMin = "0"))
	float SkyIntensity = 1.2f;

	/** The height fog's density. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Fog", meta = (ClampMin = "0"))
	float FogDensity = 0.03f;

	/** The haze's own color (on the tutorial island, the blue filling the void under it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Fog")
	FLinearColor FogInscattering = FLinearColor(0.20f, 0.29f, 0.44f);

	/** The haze's glow toward the sun; black for none (the tutorial island has none). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Fog")
	FLinearColor FogDirectionalInscattering = FLinearColor::Black;

	/** How tightly that glow gathers around the sun: higher is tighter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Fog", meta = (ClampMin = "2", ClampMax = "64"))
	float FogDirectionalExponent = 4.f;

	/** How far from the camera the glow starts (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Fog", meta = (ClampMin = "0"))
	float FogDirectionalStartDistance = 10000.f;

	/**
	 * Multiplies every backdrop layer's own tint (MPC_Lighting's BackdropTint, which M_Backdrop reads). The silhouettes
	 * are unlit and never see the sun go down, so dusk darkens them here; white leaves them as built.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Unlit")
	FLinearColor BackdropTint = FLinearColor::White;

	/** Multiplies the painted clouds' colors (MPC_Lighting's CloudTint); white leaves them as built. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Unlit")
	FLinearColor CloudTint = FLinearColor::White;

	/** The exposure compensation of the level's post process volume (EV). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting|Exposure")
	float ExposureBias = 0.4f;

	/** The rotation of a directional light shining from the sun's spot: toward the opposite bearing, downward. */
	FRotator GetSunRotation() const;

	/** The bearing (0 to 360) and elevation, in degrees, of the sun a directional light with this rotation shines from. */
	static void SunAnglesFromRotation(const FRotator& Rotation, float& OutBearing, float& OutElevation);

	/**
	 * The state Alpha (0 to 1) of the way from one state to another, named as the second: the sun turns the shorter way
	 * round, colors and numbers go straight, and the cascade count rounds to the nearer.
	 */
	static FLightingState Blend(const FLightingState& From, const FLightingState& To, float Alpha);

	/** Everything it sets, in a line for the log. */
	FString Describe() const;
};
