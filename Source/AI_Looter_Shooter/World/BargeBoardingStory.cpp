// ABargeBoarding: her place in the story (Main 6's steps, read from the mission runner or forced by the console and the
// tests), her state (hanging, away, dying, falling, fallen) and her presence in the level, and the beats each step begins.

#include "World/BargeBoarding.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/BargeFurnace.h"
#include "World/BargeHoseDrum.h"
#include "World/BargeRules.h"
#include "World/FurnaceDraftValve.h"
#include "World/LamplighterRules.h"
#include "World/LandfallMarker.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

namespace
{
	/** The steps' line sets (create_story_lines.py, lane MAIN67's contract). */
	const FName FinchDimmingLines(TEXT("DA_Lines_Main6FinchDimming"));
	const FName NedRadioLines(TEXT("DA_Lines_Main6NedRadio"));
	const FName HallLines(TEXT("DA_Lines_Main6Hall"));
}

bool ABargeBoarding::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

void ABargeBoarding::HandleMissionsChanged()
{
	RefreshStory();
}

void ABargeBoarding::ForceStep(int32 Step)
{
	bForced = Step != INDEX_NONE;
	ForcedStep = Step;
	bStoryRead = false;
	if (bForced && (State == EBargeState::Away || State == EBargeState::Fallen || Step < BargeRules::EscapeStep))
	{
		// Forced back into the sky or back before the escape (the console after a fall, a replay): whole again.
		SetState(EBargeState::Hanging);
		SetActorTransform(RestTransform());
		DyingClock = 0.f;
		bStackDown = false;
		bStackHurtDone = false;
		DropStack(0.f);
		if (Furnace)
		{
			Furnace->SetCageBurst(false);
		}
		// Her lights back on (BeginDying put them out); the view's shown again at the next tick.
		LightView = INDEX_NONE;
		for (const TArray<TObjectPtr<UPointLightComponent>>* Lights : { &DeckLamps, &Lanterns })
		{
			for (UPointLightComponent* Lamp : *Lights)
			{
				if (Lamp)
				{
					Lamp->SetVisibility(true);
				}
			}
		}
	}
	RefreshStory();
}

void ABargeBoarding::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bFell = Runner && Runner->HasFlag(BargeRules::BargeFellFlag());
	int32 Step = INDEX_NONE;
	if (bForced)
	{
		Step = ForcedStep;
	}
	else if (Runner && Runner->IsRunning(BargeRules::Mission()))
	{
		Step = Runner->GetStep(BargeRules::Mission());
	}
	const bool bWanted = bForced || (!bFell && StoryHolds(ShownWhen));
	if (!bWanted || (bFell && !bForced))
	{
		if (State != EBargeState::Fallen)
		{
			SetState(bFell ? EBargeState::Fallen : EBargeState::Away);
		}
		StoryStep = Step;
		bStoryRead = true;
		return;
	}
	if (State == EBargeState::Away || State == EBargeState::Fallen)
	{
		SetState(EBargeState::Hanging);
	}
	const bool bLive = bStoryRead && Step != StoryStep;
	const bool bChanged = !bStoryRead || Step != StoryStep;
	StoryStep = Step;
	bStoryRead = true;
	if (!bChanged)
	{
		return;
	}
	StepClock = 0.f;

	// The valves: open until the furnace's step, as the step's count starts over on a load; all shut past it.
	for (AFurnaceDraftValve* Valve : Valves)
	{
		if (!Valve)
		{
			continue;
		}
		if (Step >= BargeRules::CageStep)
		{
			Valve->SetShutNow(true);
		}
		else if (Step < BargeRules::FurnaceStep || !bLive)
		{
			Valve->SetShutNow(false);
		}
	}
	if (Furnace)
	{
		Furnace->SetValvesShut(CountValvesShut());
		// The ring heats from the office's step (the hall alive under the catwalk) until the furnace is cold.
		Furnace->SetRingRunning(Step >= BargeRules::ManifestStep && Step <= BargeRules::FurnaceStep);
		if (Step >= BargeRules::EscapeStep)
		{
			Furnace->SetCageBurst(true);
			Furnace->PutOut();
		}
	}
	if (HoseDrum)
	{
		HoseDrum->SetJammedNow(IsDrumJammed());
		HoseDrum->SetTurning(Step < BargeRules::CageStep);
	}
	bHoseFrozen = IsDrumJammed();

	if (Step == BargeRules::EscapeStep)
	{
		BeginDying();
	}
	else if (Step >= BargeRules::ReadyStep && State != EBargeState::Falling)
	{
		// Past the fall with no scene to have played it (a skip lost, a load): she's in the canyon.
		FinishFall();
	}
	if (bLive)
	{
		OnStepBegan(Step);
	}
}

void ABargeBoarding::OnStepBegan(int32 Step)
{
	if (Step == BargeRules::ManifestStep && !IsDrumJammed())
	{
		// Through the office's door: the saint dimming, on Finch's tube.
		LandfallRules::Say(this, FinchDimmingLines);
	}
	else if (Step == BargeRules::FurnaceStep)
	{
		// The manifest taken: Ned's call crackles out of the office's radio; then the hall wakes (the Stoker, Vane).
		if (Radio)
		{
			LooterSound::PlayAt(this, LooterSoundCue::VaneRadioIn, Radio->GetComponentLocation());
		}
		LandfallRules::Say(this, NedRadioLines);
		LandfallRules::Say(this, HallLines);
		if (!IsDrumJammed())
		{
			LandfallRules::Say(this, FinchDimmingLines);
		}
	}
}

void ABargeBoarding::SetState(EBargeState NewState)
{
	if (State == NewState)
	{
		return;
	}
	const EBargeState Was = State;
	State = NewState;
	ApplyPresence(NewState != EBargeState::Away && NewState != EBargeState::Fallen);
	UE_LOG(LogLooter, Log, TEXT("%s: the barge is %s (was %s)."), *GetActorNameOrLabel(), *UEnum::GetValueAsString(NewState),
		*UEnum::GetValueAsString(Was));
}

void ABargeBoarding::ApplyPresence(bool bShown)
{
	// Out of the story she's out of the world: nothing seen, nothing to stand on, nothing ticking.
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	SetActorTickEnabled(bShown);
	TArray<AActor*> Pieces = { Furnace.Get(), HoseDrum.Get(), HatchMarker.Get() };
	for (AFurnaceDraftValve* Valve : Valves)
	{
		Pieces.Add(Valve);
	}
	for (AActor* Piece : Pieces)
	{
		if (IsValid(Piece))
		{
			Piece->SetActorHiddenInGame(!bShown);
			Piece->SetActorEnableCollision(bShown);
			Piece->SetActorTickEnabled(bShown && Piece->PrimaryActorTick.bStartWithTickEnabled);
		}
	}
	if (bShown && !FurnaceDrone.IsValid() && Furnace && GetWorld() && GetWorld()->IsGameWorld())
	{
		// Her furnace's breath under the deck: heard round her, louder in the hall.
		FurnaceDrone = LooterSound::Start(this, LooterSoundCue::SmogAmbience, Furnace->GetRootComponent(), NAME_None, 0.9f, 0.7f);
	}
	else if (!bShown)
	{
		LooterSound::Stop(FurnaceDrone.Get(), 1.f);
		FurnaceDrone.Reset();
	}
}

