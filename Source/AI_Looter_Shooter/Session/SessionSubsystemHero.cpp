// USessionSubsystem: the hero being played. A session plays the hero picked before its new game (ULooterSessionSave::
// HeroId); a play with no session (Looter.PlayLevel, a level straight from the editor) plays Ellis unless Looter.Hero
// named another. The character asks as it begins (UEmberComponent for the powers, UBodyFigureComponent for the body).

#include "Session/SessionSubsystem.h"
#include "Embers/HeroDefinition.h"
#include "Session/SessionSave.h"
#include "Story/HeroLines.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
	/** Looter.Hero's pick for plays with no session: kept for the whole run (across levels and editor plays). */
	FName NoSessionHero;
}

FName USessionSubsystem::GetNoSessionHeroId()
{
	return NoSessionHero.IsNone() ? UHeroDefinition::DefaultId() : NoSessionHero;
}

void USessionSubsystem::SetNoSessionHeroId(FName HeroId)
{
	NoSessionHero = HeroId;
	// Looter.Hero (only with no session played) swaps the words at once, as it puts the body and powers on.
	HeroLines::SetPlayedHero(GetNoSessionHeroId());
}

FName USessionSubsystem::GetPlayedHeroId() const
{
	// A trip without a session carries a save in memory too, but it was never picked: the session's hero only in a session.
	return IsPlayingSession() && Current ? Current->GetHeroId() : GetNoSessionHeroId();
}

const UHeroDefinition* USessionSubsystem::GetPlayedHero() const
{
	if (const UHeroDefinition* Hero = UHeroDefinition::Load(GetPlayedHeroId()))
	{
		return Hero;
	}
	// An id with no definition (a hero taken out of the game): Ellis's, so the player still has a body and powers.
	return UHeroDefinition::Load(UHeroDefinition::DefaultId());
}

const UHeroDefinition* USessionSubsystem::FindPlayedHero(const UObject* WorldContextObject)
{
	const USessionSubsystem* Sessions = Get(WorldContextObject);
	return Sessions ? Sessions->GetPlayedHero() : nullptr;
}
