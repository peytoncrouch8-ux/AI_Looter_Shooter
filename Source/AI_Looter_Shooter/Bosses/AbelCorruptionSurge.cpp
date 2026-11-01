// UAbelCorruptionComponent: the fog surge in "Keeper" (MAINS_2_7.md section 11: "The smog from the wreck below pours up over
// the deck: the fog thickens to about 15 m of sight (a height-fog density change; the coal, his lantern and the keeper's
// post's lantern are what to aim by)"). The level's own height fog is thickened and brought in close, eased in and given
// back eased out; no volumetrics, nothing new drawn (120 fps on the reference PC).

#include "Bosses/AbelCorruption.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelKeeper.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "LooterAbel"

UExponentialHeightFogComponent* UAbelCorruptionComponent::FindFog()
{
	if (UExponentialHeightFogComponent* Found = Fog.Get())
	{
		return Found;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		if (UExponentialHeightFogComponent* Component = It->GetComponent())
		{
			Fog = Component;
			return Component;
		}
	}
	return nullptr;
}

void UAbelCorruptionComponent::SetSurge(bool bOn)
{
	if (bSurgeWanted == bOn)
	{
		return;
	}
	bSurgeWanted = bOn;
	if (bOn)
	{
		// The wreck's smog climbing the Rim: a low roar under it, Hob's word the first time.
		if (AAbelKeeper* Abel = GetAbel())
		{
			LooterSound::PlayAt(Abel, LooterSoundCue::AbelWindRise, Abel->GetActorLocation(), 0.8f, 0.8f);
		}
		if (!bSurgeLineSaid)
		{
			bSurgeLineSaid = true;
			static const FText Hob = LOCTEXT("Hob", "Hob");
			Say(TEXT("DA_Lines_AbelSurge"), { FStoryLine::Make(Hob, LOCTEXT("Surge", "Can't see a thing. Aim for the coal, sunshine."), 3.f) });
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the fog surge %s."), *GetNameSafe(GetOwner()), bOn ? TEXT("pours up over the deck") : TEXT("thins"));
}

void UAbelCorruptionComponent::TickSurge(float DeltaSeconds)
{
	if (!bSurgeWanted && SurgeShare <= 0.f && !bFogSaved)
	{
		return;
	}
	SurgeShare = AbelCorruptionRules::StepSurge(SurgeShare, bSurgeWanted, DeltaSeconds);
	UExponentialHeightFogComponent* Level = FindFog();
	if (!Level)
	{
		return;
	}
	if (!bFogSaved)
	{
		// The level's own fog as its lighting state left it: what it's given back to.
		bFogSaved = true;
		OwnFogDensity = Level->FogDensity;
		OwnFogStart = Level->StartDistance;
	}
	Level->SetFogDensity(AbelCorruptionRules::SurgeDensityAt(OwnFogDensity, SurgeShare));
	Level->SetStartDistance(AbelCorruptionRules::SurgeStartAt(OwnFogStart, SurgeShare));
	if (!bSurgeWanted && SurgeShare <= 0.f)
	{
		// Given back exactly as it was.
		Level->SetFogDensity(OwnFogDensity);
		Level->SetStartDistance(OwnFogStart);
		bFogSaved = false;
	}
}

#undef LOCTEXT_NAMESPACE
