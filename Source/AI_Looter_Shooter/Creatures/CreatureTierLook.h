#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Elements/LooterElement.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/CreatureTrait.h"

class ACreatureBase;

/** Which body a creature wears its tier on (CreatureTierLook::BodyOf): the glow lives in a different part of each. */
enum class ECreatureTierBody : uint8
{
	/** Anything else: no glow of its own, only the Ethereal's ghost-rim. */
	Other,
	/** The spiders (the Spitter too): their eyes, a patch of the spider atlas. */
	Spider,
	/** The meadow slime: its core, inside the gel. */
	Slime,
	/** The Unpaid's rig (dead miners, the Bell-Ringer): the coal burns in the tier's colour already (M_Ghost). */
	Unpaid
};

/**
 * What a creature's tier shell shows (UCreatureTierLookComponent wears it): the body's mesh drawn once more in
 * M_CreatureTier, on a follower of the body's pose, so a hit flash or an ember power's tint (the body's overlay slot)
 * never takes it away mid-fight. Eyes, core and rim are masks in the material; this is which are on and where.
 */
struct FCreatureTierShell
{
	/** It wears a shell at all. */
	bool bWorn = false;

	/** Its eyes glow: everything inside EyeMin..EyeMax of its UV 0 (the spider atlas's 'eye' patch). */
	bool bEyes = false;
	FVector2f EyeMin = FVector2f::ZeroVector;
	FVector2f EyeMax = FVector2f::ZeroVector;

	/** Its core glows: the inside parts (vertex red under a half) within CoreRadius of CoreCenter (cm, the mesh at rest). */
	bool bCore = false;
	FVector3f CoreCenter = FVector3f::ZeroVector;
	float CoreRadius = 0.f;
	/** The core takes only the inside parts (vertex red under a half); off, every face within CoreRadius (the slime's gel lit round its core). */
	bool bInsideOnly = true;

	/** The Ethereal's faint red ghost-rim, shimmering up the body. */
	bool bRim = false;

	/**
	 * It fades with the body's vertex alpha, as the Unpaid's shroud does (M_Ghost: 1 solid, 0 gone). Only an Unpaid's: on
	 * every other model that alpha is baked occlusion (M_World's), which would put the glow out in every crease.
	 */
	bool bShroudFade = false;

	/** How bright the eyes or core burn (M_CreatureTier's Glow: an emissive multiple of the tier's colour). */
	float Glow = 0.f;

	/**
	 * The element a Touched creature glows in (ElementRules::FxColor) instead of its tier's colour; None for everyone else.
	 * Part of the shared look's Key, so two Touched spiders of other elements never share a material.
	 */
	ELooterElement Touched = ELooterElement::None;

	/** The shared look's name (kind and tier): one material instance for every creature that looks alike. */
	FName Key;
};

/** The tag's look at a tier (UCreatureHealthBarWidget::SetTier). */
struct FCreatureTagTier
{
	/** Hallowed and Ethereal: a frame round the bar in the tier's colour, with clamps at its ends. */
	bool bThickFrame = false;
	FLinearColor FrameColor = FLinearColor::White;
	/** Its rank sting is the top tiers' notice: a bigger, longer pop of the word and a flare of the frame. */
	bool bTopTierFlash = false;
};

/** A top tier's death in the world (UCreatureTierFXSubsystem plays it); every length in seconds, every size in cm. */
struct FCreatureTierDeathShow
{
	bool bValid = false;
	/** The whole show. */
	float Seconds = 0.f;
	/** The soul-light pillar: its height and radius once it has shot up, its glow (a loot beam's is 2 to 3). */
	float PillarHeight = 0.f;
	float PillarRadius = 0.f;
	float PillarGlow = 0.f;
	/** The white-hot thread inside it. */
	float CoreRadius = 0.f;
	float CoreGlow = 0.f;
	/** The ring of light racing out over the ground, and how long it runs. */
	float RingRadius = 0.f;
	float RingSeconds = 0.f;
	/** Motes of soul-light rising round the pillar. */
	int32 Motes = 0;
	/** The soul-light burst over the body's own, as a multiple of the body's size. */
	float BurstScale = 1.f;
	/** The loot's pop: how long after the death, and the glints thrown out with it. */
	float PopDelay = 0.f;
	int32 Glints = 0;
	/** The Ethereal's: the pillar flickers like a ghost's light, and a second ring follows the first. */
	bool bFlicker = false;
	bool bSecondRing = false;
};

/**
 * How the enemy tiers look and sound (Saved/Tiers/DESIGN.md, "Readable and rewarding"; the user's 2026-10-20 approval):
 * plain rules, so the tests check them without a level. Regular shows nothing new; Restless and up glow in the tier's
 * colour (spider eyes, slime core, the Unpaid's coal, the dead miner's lamp); Gravebound and up name their traits under
 * the tag's name; Hallowed and Ethereal get the thick frame, a notice sting with a flare, the big death in their colour
 * with a pillar of soul-light, a loot pop with its own sound and the kill's XP in their colour; Ethereal alone the faint
 * red ghost-rim. Colours come from the rank settings (FCreatureRankInfo::Color for the tag, GlowColor for the body).
 */
namespace CreatureTierLook
{
	/** The tier (0 Regular .. 4 Ethereal), INDEX_NONE for a Legendary monster or a boss (LooterRanks::TierIndex). */
	AI_LOOTER_SHOOTER_API int32 TierOf(ECreatureRank Rank);

	/** Restless and up: the body glows in the tier's colour. */
	AI_LOOTER_SHOOTER_API bool HasBodyGlow(ECreatureRank Rank);

	/**
	 * Whether a creature of Rank that is Touched by Touched (None: not Touched) wears a tier shell: any tier's glow as before,
	 * and a Touched Basic one too (its element's glow stands in for the tier it hasn't). A Legendary monster or a boss never.
	 */
	AI_LOOTER_SHOOTER_API bool WearsShell(ECreatureRank Rank, ELooterElement Touched);

	/**
	 * The name on the tag: Base, with the Touched word in front for a Touched creature ("Cinder-touched Brown Spider";
	 * CreatureTouched::Word), else Base as it is. The rank word stays its own, before it ("Restless Cinder-touched Brown Spider").
	 */
	AI_LOOTER_SHOOTER_API FText TagName(const FText& Base, ELooterElement Touched);

	/** Ethereal: the faint red ghost-rim. */
	AI_LOOTER_SHOOTER_API bool HasGhostRim(ECreatureRank Rank);

	/** Hallowed and Ethereal: the sting, the thick frame, the big death, the loot pop, the coloured XP. */
	AI_LOOTER_SHOOTER_API bool IsTopTier(ECreatureRank Rank);

	/** Gravebound and up: the trait line under the tag's name. */
	AI_LOOTER_SHOOTER_API bool ShowsTraitLine(ECreatureRank Rank);

	/** The body's glow colour (FCreatureRankInfo::GlowColor), and the tag's word colour (FCreatureRankInfo::Color). */
	AI_LOOTER_SHOOTER_API FLinearColor GlowColor(ECreatureRank Rank);
	AI_LOOTER_SHOOTER_API FLinearColor TagColor(ECreatureRank Rank);

	/** Which body Creature wears its tier on, by class (most specific first: a dead miner is an Unpaid). */
	AI_LOOTER_SHOOTER_API ECreatureTierBody BodyOf(const ACreatureBase& Creature);

	/**
	 * What the shell shows for that body at Rank (bWorn false: no shell). A Touched creature (Touched not None) wears one even
	 * at Basic, burning at the Restless glow, with its element's colour and a key of its own; its rank keeps the tag's frame
	 * and word (those come from TagFor, never from the shell). A body with no eyes or core to light shows the faint rim
	 * instead, so a Touched dead miner still glows.
	 */
	AI_LOOTER_SHOOTER_API FCreatureTierShell ShellFor(ECreatureTierBody Body, ECreatureRank Rank, ELooterElement Touched = ELooterElement::None);

	/** The colour a shell glows in: the Touched element's (ElementRules::FxColor), else Rank's GlowColor. */
	AI_LOOTER_SHOOTER_API FLinearColor ShellColor(const FCreatureTierShell& Shell, ECreatureRank Rank);

	/**
	 * The spider atlas's 'eye' patch in UV 0 as Unreal reads it (Spider.py's ATLAS 'eye' is u 0.75-0.8125, v 0-0.125 in
	 * Blender, whose v runs up; the FBX import flips it: V = 1 - v). A hair inside its edges, off its neighbours.
	 */
	AI_LOOTER_SHOOTER_API void SpiderEyePatch(FVector2f& OutMin, FVector2f& OutMax);

	/** How hard an Unpaid's coal and ember edge glow at Rank, from its kind's own strength (UnpaidRules' CoalGlow). */
	AI_LOOTER_SHOOTER_API float CoalGlow(ECreatureRank Rank, float OwnGlow);

	/** A dead miner's lamp flame and its light at Rank: its own warm colour for Regular, else the tier's, kept a flame's. */
	AI_LOOTER_SHOOTER_API FLinearColor LampColor(ECreatureRank Rank, const FLinearColor& OwnColor);

	/** The tag's look at Rank. */
	AI_LOOTER_SHOOTER_API FCreatureTagTier TagFor(ECreatureRank Rank);

	/** The top tiers' sounds (the contract's cue names; NAME_None below them): the notice sting, the death, the loot pop. */
	AI_LOOTER_SHOOTER_API FName NoticeCue(ECreatureRank Rank);
	AI_LOOTER_SHOOTER_API FName DeathCue(ECreatureRank Rank);
	AI_LOOTER_SHOOTER_API FName PopCue(ECreatureRank Rank);

	/**
	 * Whether Rank's sting gives way to Other's sounded a moment ago (UEncounterSettings::RankStingRest): two creatures
	 * turning together sting once, but a top tier's own sting never gives way to a lower one's (a Restless that turned with
	 * a Hallowed doesn't swallow its sting).
	 */
	AI_LOOTER_SHOOTER_API bool StingGivesWayTo(ECreatureRank Rank, ECreatureRank Other);

	/** A trait's word on the tag ("Fleet", "Grave-burst"): the traits' own (CreatureTraitRules::Word). */
	AI_LOOTER_SHOOTER_API FString TraitWord(ECreatureTrait Trait);

	/**
	 * The shell's flare that makes its glow GlowScale times its own (UCreatureTraitComponent::GetGlowScale: Vengeful's
	 * enrage, a Grave-burst's swell, Mending's breath); never below none.
	 */
	AI_LOOTER_SHOOTER_API float TraitFlare(float GlowScale);

	/** The tag's trait line: the traits' words, upper case, a dot between ("FLEET · WARDED · PHASING"); empty with none. */
	AI_LOOTER_SHOOTER_API FText TraitLine(TConstArrayView<ECreatureTrait> Traits);

	/** A top tier's death show (bValid false below Hallowed). */
	AI_LOOTER_SHOOTER_API FCreatureTierDeathShow DeathShowFor(ECreatureRank Rank);

	/** The kill's "+N XP" in the tier's colour (Hallowed and Ethereal), a size up from a kill's own. */
	AI_LOOTER_SHOOTER_API bool TintsKillXP(ECreatureRank Rank);

	// --- The shell's flare and fade (M_CreatureTier's Flare and Fade, through the shell's custom primitive data) ---

	/** The shell's custom primitive data: the flare (0 at rest; up to 2) and the fade (1 shown, 0 gone). */
	inline constexpr int32 FlareIndex = 0;
	inline constexpr int32 FadeIndex = 1;
	/** M_CreatureTier's FlareGain (build_tier_material.py's default): a flare of 1 makes it this much brighter again. */
	inline constexpr float ShellFlareGain = 2.f;

	/** The flare as the creature turns on the player (its rank sting, Age seconds ago): a top tier's is the bigger. */
	inline constexpr float NoticeFlareSeconds = 0.9f;
	AI_LOOTER_SHOOTER_API float NoticeFlareAt(float Age, bool bTopTier);

	/** Its death, Since seconds ago: the glow flares as its soul goes, then goes out (Fade to 0 by DeathFadeSeconds). */
	inline constexpr float DeathFadeSeconds = 0.85f;
	AI_LOOTER_SHOOTER_API float DeathFlareAt(float Since);
	AI_LOOTER_SHOOTER_API float DeathFadeAt(float Since);
}
