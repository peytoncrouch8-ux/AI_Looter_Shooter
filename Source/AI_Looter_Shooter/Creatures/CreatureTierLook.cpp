#include "Creatures/CreatureTierLook.h"
#include "Audio/LooterSoundCues.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTouched.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Elements/ElementRules.h"

namespace
{
	// The contract's cues (LooterSoundCues.h's Tier block). A cue with no sounds yet plays nothing, so these are safe
	// before their recipes are in the bank.
	const FName TierNoticeHallowedCue(LooterSoundCue::Tier::NoticeHallowed);
	const FName TierNoticeEtherealCue(LooterSoundCue::Tier::NoticeEthereal);
	const FName TierDeathHallowedCue(LooterSoundCue::Tier::DeathHallowed);
	const FName TierDeathEtherealCue(LooterSoundCue::Tier::DeathEthereal);
	const FName TierPopHallowedCue(LooterSoundCue::Tier::LootPopHallowed);
	const FName TierPopEtherealCue(LooterSoundCue::Tier::LootPopEthereal);

	/** The eyes' and the core's glow by tier (Regular, Restless, Gravebound, Hallowed, Ethereal): each tier burns hotter. */
	constexpr float TierEyeGlow[] = { 0.f, 7.f, 9.f, 11.f, 13.f };
	/**
	 * The slime's heart: the gel lit from within round its core. Not the core's own faces: the gel hides what's inside it
	 * from a translucent shell, and SK_Slime's inside parts don't keep their vertex red (PIE, 2026-10-20: the core mask lit
	 * nothing). The gel's whole middle glows instead, so a softer glow than an eye's reads as strongly.
	 */
	constexpr float TierCoreGlow[] = { 0.f, 1.5f, 2.f, 2.5f, 3.f };

	/** Slime.py's core: its middle (creature space, cm: on the body's axis, 30 up from the ground), and how far round it the gel glows. */
	const FVector3f SlimeCoreCenter(0.f, 0.f, 30.f);
	constexpr float SlimeCoreRadius = 50.f;

	/** The coal's glow at the top tiers: yellow reads bright already; the Ethereal's red is a dark colour and needs more. */
	constexpr float HallowedCoalShare = 1.25f;
	constexpr float EtherealCoalShare = 1.6f;

	/** A tier-coloured lamp keeps a little white in it, so it still reads as a flame and not a gem. */
	constexpr float LampWhiteShare = 0.2f;

	const TCHAR* BodyName(ECreatureTierBody Body)
	{
		switch (Body)
		{
		case ECreatureTierBody::Spider: return TEXT("Spider");
		case ECreatureTierBody::Slime: return TEXT("Slime");
		case ECreatureTierBody::Unpaid: return TEXT("Unpaid");
		default: return TEXT("Other");
		}
	}

	float TierTable(const float (&Table)[5], int32 Tier)
	{
		return Tier >= 0 && Tier < 5 ? Table[Tier] : 0.f;
	}
}

int32 CreatureTierLook::TierOf(ECreatureRank Rank)
{
	return LooterRanks::TierIndex(Rank);
}

bool CreatureTierLook::HasBodyGlow(ECreatureRank Rank)
{
	return TierOf(Rank) >= 1;
}

bool CreatureTierLook::WearsShell(ECreatureRank Rank, ELooterElement Touched)
{
	// A Touched creature is only ever a tier (Basic, Restless, Gravebound): the one tier without a glow borrows its element's.
	return HasBodyGlow(Rank) || (Touched != ELooterElement::None && TierOf(Rank) == 0);
}

FText CreatureTierLook::TagName(const FText& Base, ELooterElement Touched)
{
	if (Touched == ELooterElement::None)
	{
		return Base;
	}
	const FString Word = CreatureTouched::Word(Touched);
	return Word.IsEmpty() ? Base : FText::FromString(FString::Printf(TEXT("%s %s"), *Word, *Base.ToString()));
}

bool CreatureTierLook::HasGhostRim(ECreatureRank Rank)
{
	return Rank == ECreatureRank::Ethereal;
}

bool CreatureTierLook::IsTopTier(ECreatureRank Rank)
{
	return Rank == ECreatureRank::Hallowed || Rank == ECreatureRank::Ethereal;
}

bool CreatureTierLook::ShowsTraitLine(ECreatureRank Rank)
{
	return TierOf(Rank) >= 2;
}

FLinearColor CreatureTierLook::GlowColor(ECreatureRank Rank)
{
	return UCreatureRankSettings::Get(Rank).GlowColor;
}

FLinearColor CreatureTierLook::TagColor(ECreatureRank Rank)
{
	return UCreatureRankSettings::Get(Rank).Color;
}

ECreatureTierBody CreatureTierLook::BodyOf(const ACreatureBase& Creature)
{
	// Most specific first: the dead miners and the Bell-Ringer are built on the Unpaid, the Spitter on the spider.
	if (Creature.IsA<AUnpaidCreature>())
	{
		return ECreatureTierBody::Unpaid;
	}
	if (Creature.IsA<ASlimeCreature>())
	{
		return ECreatureTierBody::Slime;
	}
	if (Creature.IsA<ASpiderCreature>())
	{
		return ECreatureTierBody::Spider;
	}
	return ECreatureTierBody::Other;
}

void CreatureTierLook::SpiderEyePatch(FVector2f& OutMin, FVector2f& OutMax)
{
	// Spider.py: 'eye': (0.75, 0.8125, 0.0, 0.125) as (u0, u1, v0, v1), v up; Unreal's V runs down (V = 1 - v).
	constexpr float Inset = 0.002f;
	OutMin = FVector2f(0.75f + Inset, 1.f - 0.125f + Inset);
	OutMax = FVector2f(0.8125f - Inset, 1.f - Inset);
}

FLinearColor CreatureTierLook::ShellColor(const FCreatureTierShell& Shell, ECreatureRank Rank)
{
	return Shell.Touched != ELooterElement::None ? ElementRules::FxColor(Shell.Touched) : GlowColor(Rank);
}

FCreatureTierShell CreatureTierLook::ShellFor(ECreatureTierBody Body, ECreatureRank Rank, ELooterElement Touched)
{
	FCreatureTierShell Shell;
	const int32 OwnTier = TierOf(Rank);
	// A Basic Touched creature burns at the Restless glow; its tier (the tag's frame, its word) stays Basic's.
	const bool bTouched = Touched != ELooterElement::None && OwnTier >= 0;
	const int32 Tier = bTouched ? FMath::Max(OwnTier, 1) : OwnTier;
	if (Tier < 1)
	{
		return Shell;
	}
	Shell.Touched = bTouched ? Touched : ELooterElement::None;
	Shell.bRim = HasGhostRim(Rank);
	Shell.bShroudFade = Body == ECreatureTierBody::Unpaid;
	switch (Body)
	{
	case ECreatureTierBody::Spider:
		Shell.bEyes = true;
		SpiderEyePatch(Shell.EyeMin, Shell.EyeMax);
		Shell.Glow = TierTable(TierEyeGlow, Tier);
		break;
	case ECreatureTierBody::Slime:
		Shell.bCore = true;
		Shell.bInsideOnly = false;
		Shell.CoreCenter = SlimeCoreCenter;
		Shell.CoreRadius = SlimeCoreRadius;
		Shell.Glow = TierTable(TierCoreGlow, Tier);
		break;
	default:
		// The Unpaid's coal burns in M_Ghost already (CoalGlow); anything else has no glowing part of its own.
		break;
	}
	// A Touched body with nothing of its own to light (the Unpaid's coal burns in the tier's colour already) shows the rim, so
	// its element still reads on it.
	if (bTouched && !Shell.bEyes && !Shell.bCore)
	{
		Shell.bRim = true;
	}
	Shell.bWorn = Shell.bEyes || Shell.bCore || Shell.bRim;
	if (Shell.bWorn)
	{
		Shell.Key = bTouched ? FName(*FString::Printf(TEXT("%s_%d_T%d"), BodyName(Body), Tier, static_cast<int32>(Touched)))
			: FName(*FString::Printf(TEXT("%s_%d"), BodyName(Body), Tier));
	}
	return Shell;
}

float CreatureTierLook::CoalGlow(ECreatureRank Rank, float OwnGlow)
{
	switch (Rank)
	{
	case ECreatureRank::Hallowed: return OwnGlow * HallowedCoalShare;
	case ECreatureRank::Ethereal: return OwnGlow * EtherealCoalShare;
	default: return OwnGlow;
	}
}

FLinearColor CreatureTierLook::LampColor(ECreatureRank Rank, const FLinearColor& OwnColor)
{
	if (!HasBodyGlow(Rank))
	{
		return OwnColor;
	}
	return FMath::Lerp(GlowColor(Rank), FLinearColor::White, LampWhiteShare);
}

FCreatureTagTier CreatureTierLook::TagFor(ECreatureRank Rank)
{
	FCreatureTagTier Tag;
	Tag.bThickFrame = IsTopTier(Rank);
	Tag.bTopTierFlash = IsTopTier(Rank);
	Tag.FrameColor = Tag.bThickFrame ? TagColor(Rank) : FLinearColor::White;
	return Tag;
}

FName CreatureTierLook::NoticeCue(ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Hallowed: return TierNoticeHallowedCue;
	case ECreatureRank::Ethereal: return TierNoticeEtherealCue;
	default: return NAME_None;
	}
}

FName CreatureTierLook::DeathCue(ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Hallowed: return TierDeathHallowedCue;
	case ECreatureRank::Ethereal: return TierDeathEtherealCue;
	default: return NAME_None;
	}
}

FName CreatureTierLook::PopCue(ECreatureRank Rank)
{
	switch (Rank)
	{
	case ECreatureRank::Hallowed: return TierPopHallowedCue;
	case ECreatureRank::Ethereal: return TierPopEtherealCue;
	default: return NAME_None;
	}
}

bool CreatureTierLook::StingGivesWayTo(ECreatureRank Rank, ECreatureRank Other)
{
	return !IsTopTier(Rank) || IsTopTier(Other);
}

FString CreatureTierLook::TraitWord(ECreatureTrait Trait)
{
	// The traits' own words (CreatureTraitRules::Word, the design's names): one table for the tag and the traits' code.
	return CreatureTraitRules::Word(Trait).ToString();
}

float CreatureTierLook::TraitFlare(float GlowScale)
{
	// M_CreatureTier brightens by 1 + Flare x FlareGain: a glow GlowScale times its own is that flare.
	return FMath::Max(GlowScale - 1.f, 0.f) / ShellFlareGain;
}

FText CreatureTierLook::TraitLine(TConstArrayView<ECreatureTrait> Traits)
{
	TArray<FString> Words;
	for (const ECreatureTrait Trait : Traits)
	{
		const FString Word = TraitWord(Trait);
		if (!Word.IsEmpty())
		{
			Words.Add(Word.ToUpper());
		}
	}
	// A middle dot between, as the HUD's own lists run ("FLEET · WARDED").
	return FText::FromString(FString::Join(Words, TEXT(" · ")));
}

FCreatureTierDeathShow CreatureTierLook::DeathShowFor(ECreatureRank Rank)
{
	FCreatureTierDeathShow Show;
	if (Rank == ECreatureRank::Hallowed)
	{
		// Saint-gold: a steady pillar twice a man's height and more, a ring out past the fight, the soul going up in motes.
		Show.bValid = true;
		Show.Seconds = 1.7f;
		Show.PillarHeight = 900.f;
		Show.PillarRadius = 24.f;
		Show.PillarGlow = 5.f;
		Show.CoreRadius = 7.f;
		Show.CoreGlow = 9.f;
		Show.RingRadius = 420.f;
		Show.RingSeconds = 0.7f;
		Show.Motes = 24;
		Show.BurstScale = 1.5f;
		Show.PopDelay = 0.15f;
		Show.Glints = 18;
	}
	else if (Rank == ECreatureRank::Ethereal)
	{
		// Red, taller and longer, and never quite steady: a ghost's light, a second ring after the first.
		Show.bValid = true;
		Show.Seconds = 2.1f;
		Show.PillarHeight = 1300.f;
		Show.PillarRadius = 30.f;
		Show.PillarGlow = 6.f;
		Show.CoreRadius = 9.f;
		Show.CoreGlow = 11.f;
		Show.RingRadius = 540.f;
		Show.RingSeconds = 0.8f;
		Show.Motes = 36;
		Show.BurstScale = 1.7f;
		Show.PopDelay = 0.18f;
		Show.Glints = 24;
		Show.bFlicker = true;
		Show.bSecondRing = true;
	}
	return Show;
}

bool CreatureTierLook::TintsKillXP(ECreatureRank Rank)
{
	return IsTopTier(Rank);
}

float CreatureTierLook::NoticeFlareAt(float Age, bool bTopTier)
{
	if (Age < 0.f || Age >= NoticeFlareSeconds)
	{
		return 0.f;
	}
	// Up at once (a few frames), then easing out over the rest: a flare, not a swell.
	constexpr float Rise = 0.06f;
	const float Peak = bTopTier ? 1.6f : 0.8f;
	const float Up = FMath::Clamp(Age / Rise, 0.f, 1.f);
	const float Left = 1.f - Age / NoticeFlareSeconds;
	return Peak * Up * Left * Left;
}

float CreatureTierLook::DeathFlareAt(float Since)
{
	if (Since < 0.f || Since >= DeathFadeSeconds)
	{
		return 0.f;
	}
	return 1.8f * FMath::Exp(-Since / 0.25f);
}

float CreatureTierLook::DeathFadeAt(float Since)
{
	if (Since < 0.f)
	{
		return 1.f;
	}
	return 1.f - FMath::SmoothStep(0.15f, DeathFadeSeconds, Since);
}
