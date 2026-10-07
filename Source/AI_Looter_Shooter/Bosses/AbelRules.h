#pragma once

#include "CoreMinimal.h"
#include "Bosses/BossTypes.h"
#include "Story/StoryLine.h"
#include "AbelRules.generated.h"

class AActor;
class ULightComponent;
class UPrimitiveComponent;

/**
 * The Gravewind in Abel's last phase (Docs/Areas/RansomsRest.md, "Let me go": "wisps and mild gusts toward the deck's open
 * end. A fall off the deck is caught by Hob ... fall recovery puts the player back on the deck within a second").
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FAbelGustRules
{
	GENERATED_BODY()

	/** The first gust comes this long after the wind rises, then one every Every seconds (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "0", Units = "s"))
	float FirstAfter = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "1", Units = "s"))
	float Every = 7.f;

	/** Each gust lasts this long, rising and dying over Ramp at its ends (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "0.2", Units = "s"))
	float Seconds = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "0", Units = "s"))
	float Ramp = 0.35f;

	/**
	 * How fast a gust carries the player toward the open end at its height (cm/s): mild, well under a walk, so they can
	 * lean into it; a player standing at the end is carried off.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "0", Units = "cm/s"))
	float Push = 260.f;

	/**
	 * Past the open end the wind pours down off the point: a falling player goes down at least this fast (cm/s), so fall
	 * recovery's outside rule (5 m below the last safe spot) brings them back well within a second.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust", meta = (ClampMin = "0", Units = "cm/s"))
	float Downdraft = 650.f;

	/** Wisps streaming over the deck while the wind blows (one instanced mesh: no translucency, no shadow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gust|Look", meta = (ClampMin = "0", ClampMax = "64"))
	int32 Wisps = 28;
};

/** How Abel fights between the dark saint's pulls in his last phase ("Between pulls he fights faster"). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FAbelPace
{
	GENERATED_BODY()

	/** His attacks' cooldown and wind-up, his chase and his lunge, against his own. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pace", meta = (ClampMin = "0.1", ClampMax = "1"))
	float Cooldown = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pace", meta = (ClampMin = "0.3", ClampMax = "1"))
	float Windup = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pace", meta = (ClampMin = "1", ClampMax = "3"))
	float Chase = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pace", meta = (ClampMin = "1", ClampMax = "3"))
	float Lunge = 1.2f;
};

/**
 * Abel's fight as data and plain rules (Docs/Areas/RansomsRest.md, "The boss: Abel Ransom, the Keeper"), so the tests check
 * them without a fight; AAbelKeeper plays them. His moments that only he has (the grief, the buckshot's flare, the bell,
 * the Gravewind, the walk into the wind) are Custom events of his boss component's phases, which his own code answers.
 */
namespace AbelRules
{
	/** His health against an Unpaid's of his level ("about 40x an Unpaid of his level, about 10,500 at level 9"). */
	inline constexpr float HealthScale = 40.f;

	/** His size: the Unpaid's rig at 1.3 (his model is built at 1; the Boss rank adds none). */
	inline constexpr float BodyScale = 1.3f;

	/** The phases' shares of health: "You brought them here" from full, "The bell" at 60%, "Let me go" at 25%. */
	inline constexpr float BellShare = 0.6f;
	inline constexpr float WindShare = 0.25f;

	/** Phase one: two Unpaid rise every 25 s, at most 4; he grieves every 12 s, for 3 s. */
	inline constexpr float RiseEvery = 25.f;
	inline constexpr int32 RiseCount = 2;
	inline constexpr int32 RiseMaxAlive = 4;
	inline constexpr float GrieveEvery = 12.f;
	inline constexpr float GrieveSeconds = 3.f;

	/** Phase two: 8 Unpaid in two waves of four, the second this long after the first. */
	inline constexpr int32 BellWave = 4;
	inline constexpr float SecondWaveAfter = 22.f;

	/** The lanterns: three, each relit by holding Interact this long. */
	inline constexpr int32 Lanterns = 3;
	inline constexpr float RelightSeconds = 1.5f;

	/** Phase three: he tries to walk off into the wind this often; each pull stuns him this long, his coal open. */
	inline constexpr float WalkOffEvery = 10.f;
	inline constexpr float PullStunSeconds = 3.f;

	/** The buckshot: a lantern flare this long before the pellets leave, every this many seconds (faster in the wind). */
	inline constexpr float FlareSeconds = 1.f;
	inline constexpr float BuckshotEvery = 7.f;
	inline constexpr float WindBuckshotEvery = 5.f;

	/** His custom events (FBossPhaseEvent::Name), as his phases send them. */
	AI_LOOTER_SHOOTER_API FName BuckshotEvent();
	AI_LOOTER_SHOOTER_API FName GrieveEvent();
	AI_LOOTER_SHOOTER_API FName BellEvent();
	AI_LOOTER_SHOOTER_API FName GravewindEvent();
	AI_LOOTER_SHOOTER_API FName WalkOffEvent();

	/** His three phases, highest share first, with their events (his name for each under his bar). */
	AI_LOOTER_SHOOTER_API TArray<FBossPhase> MakePhases();

	/** The spectral buckshot: slow, visible pellets in a cone (the pellets leave from his pump's muzzle; the flare is his). */
	AI_LOOTER_SHOOTER_API FBossVolley Buckshot();

	/** Unpaid rising through the deck round the arena's middle (on its boards whatever he's doing), at most MaxAlive at once (0: the cap). */
	AI_LOOTER_SHOOTER_API FBossAddWave RisingUnpaid(int32 Count, int32 MaxAlive);

	/** How far back toward the deck the lanterns have dragged him: a third for each relit (0 out in the fog, 1 home). */
	AI_LOOTER_SHOOTER_API float DragShare(int32 LitLanterns, int32 TotalLanterns);

	/** A glide from From to To at Alpha (0-1), eased in and out, lifted by Lift (cm) at its middle. */
	AI_LOOTER_SHOOTER_API FVector GlideAt(const FVector& From, const FVector& To, float Alpha, float Lift = 0.f);

	/**
	 * How hard the Gravewind blows WindTime seconds after it rose: 0 between gusts, 1 at a gust's height, ramped at its
	 * ends. OutGust is how many gusts have begun (for the tests).
	 */
	AI_LOOTER_SHOOTER_API float GustStrength(const FAbelGustRules& Rules, float WindTime, int32* OutGust = nullptr);

	/** A falling player's upward speed once the wind has them past the open end (cm/s): never slower down than its downdraft. */
	AI_LOOTER_SHOOTER_API double FallSpeedPastEnd(const FAbelGustRules& Rules, double VelocityZ);

	/** His lines in the fight (drafts): at a phase's start (0-2), the bell, and Hob's word on a fall off the deck. */
	AI_LOOTER_SHOOTER_API FStoryLine PhaseLine(int32 Phase);
	AI_LOOTER_SHOOTER_API FStoryLine BellLine();
	AI_LOOTER_SHOOTER_API FStoryLine HobOnFall();

	/**
	 * His ghost light lights what's round him, never him. Its globe hangs a hand's width from his chest, and a light bright
	 * enough to reach the deck's boards is hundreds of times brighter there than the dusk sun: it blew his charcoal coat
	 * out white (the dusk fight's tour shot), where his sheet has a dark coat, a pale face and hands and the gold coal. So
	 * his lantern shines on a lighting channel of its own, which his body and props (hat, lantern, pump) never take: they
	 * keep channel 0, the sun's and the sky's. What should catch the light (the deck and its biers, the lantern posts, his
	 * adds) takes this channel as well as 0. Channel 1 is the loadout and bestiary stands' (StageStudio), so it's 2.
	 */
	inline constexpr int32 GhostLightChannel = 2;

	/** Puts a light on the ghost light's channel alone (his lantern's, and Pa's on his board). */
	AI_LOOTER_SHOOTER_API void ShineOnGhostChannel(ULightComponent& Light);

	/** True when a light shines on the ghost light's channel and no other. */
	AI_LOOTER_SHOOTER_API bool IsOnGhostChannel(const ULightComponent& Light);

	/** Lets the ghost light reach a part (or every part of an actor), which keeps its other channels (0: the sun and sky). */
	AI_LOOTER_SHOOTER_API void LetGhostLightReach(UPrimitiveComponent& Part);
	AI_LOOTER_SHOOTER_API void LetGhostLightReach(AActor& Actor);

	/** True when the ghost light reaches a part. */
	AI_LOOTER_SHOOTER_API bool DoesGhostLightReach(const UPrimitiveComponent& Part);
}
