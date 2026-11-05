// ALamplighterShip in Main 6, "The Pilot Light" (2026-11-01, lane MAIN67): handed to the level's APilotSortie for the
// flight off the ledge to Vane's barge over the chapel and home again (her pose its, every frame, from her own tick), her
// starboard rail opened for the plank to the barge, and back on the ledge anchored at the end. And gone from the Rest
// after Main 7 (GoneWhen: she lifts off dark at dawn to meet Sully at the next region).

#include "World/LamplighterShip.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Story/WardenCrewMember.h"
#include "World/LandfallMarker.h"
#include "World/LandfallWinch.h"
#include "World/LanternFlame.h"
#include "World/PilotSortie.h"
#include "World/PilotSortieRules.h"
#include "Components/BoxComponent.h"

bool ALamplighterShip::BeginSortie(APilotSortie& Driver)
{
	if (bGone || !bBegun)
	{
		return false;
	}
	if (Flight == EShipFlight::Sortie && SortieDriver.Get() == &Driver)
	{
		return true;
	}
	// Nothing of Landfall's fight is left to drive (a jump straight here from the console).
	ResetFight();
	SortieDriver = &Driver;
	bSortieKeepsAboard = true;
	bAnchored = false;
	SlideClock = 0.f;
	Slid = 0.f;
	EnterFlight(EShipFlight::Sortie);
	// The winch's chain comes in with her anchor (the winch shows it only for a ship moored and anchored).
	if (ALandfallWinch* Winch = FindOrSpawnWinch())
	{
		Winch->SetShip(this);
	}
	// The Great Lantern leans for the nearest soul still burning: Abel's, in the barge's furnace.
	LanternTargetBeforeSortie = LanternTarget;
	LanternTarget = PilotLight::BargeTag();
	LanternTargetActor.Reset();
	LanternSearchWait = 0.f;
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: the Lamplighter is the sortie's (%s)."), *Driver.GetActorNameOrLabel());
	return true;
}

void ALamplighterShip::AdvanceSortie(float DeltaSeconds)
{
	if (APilotSortie* Driver = SortieDriver.Get())
	{
		Driver->DriveShip(DeltaSeconds);
	}
}

void ALamplighterShip::SetSortiePose(const FTransform& Pose, const FTransform& Calm)
{
	// Not a teleport: whoever stands on her deck rides the move, as on her course in Landfall.
	FTransform Placed = Pose;
	Placed.SetScale3D(GetActorScale3D());
	SetActorTransform(Placed, /*bSweep*/ false, nullptr, ETeleportType::None);
	CalmTransform = Calm;
	CalmTransform.SetScale3D(GetActorScale3D());
}

void ALamplighterShip::SetSortieKeepsAboard(bool bKeep)
{
	bSortieKeepsAboard = bKeep;
	OffDeckFor = 0.f;
}

void ALamplighterShip::SetSortieSavesHeld(bool bHold)
{
	// The same hold as Landfall's flight: one reason for the session, one release.
	bSortieSavesHeld = bHold;
	HoldSaves(bHold);
}

// OpenStarboardRail lives with her rails (LamplighterShipParts.cpp): it opens the stretches round the plank's hinge by tag.

void ALamplighterShip::EndSortie()
{
	SortieDriver.Reset();
	bSortieKeepsAboard = true;
	bSortieSavesHeld = false;
	OpenStarboardRail(false);
	if (!LanternTargetBeforeSortie.IsNone())
	{
		LanternTarget = LanternTargetBeforeSortie;
	}
	LanternTargetActor.Reset();
	LanternSearchWait = 0.f;
	// Down on the ledge where she always lies, the anchor down at once: Junie drops it as she settles (no winch to wind
	// again; the player has just come home from a fight).
	bAnchored = false;
	EnterFlight(EShipFlight::Moored);
	SetAnchored(/*bPlayed*/ true);
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: the Lamplighter is home on the ledge, anchored."));
}

bool ALamplighterShip::IsGoneInStory() const
{
	if (GoneWhen.IsEmpty())
	{
		return false;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && GoneWhen.IsMet(Runner->GetCampaign(), Runner);
}

void ALamplighterShip::MakeGone()
{
	bGone = true;
	bBegun = true;
	HoldSaves(false);
	ResetFight();
	for (AWardenCrewMember* Member : Crew)
	{
		if (IsValid(Member))
		{
			Member->Destroy();
		}
	}
	Crew.Reset();
	if (IsValid(LanternFlame))
	{
		LanternFlame->Destroy();
	}
	LanternFlame = nullptr;
	if (IsValid(ProwMarker))
	{
		ProwMarker->Destroy();
	}
	ProwMarker = nullptr;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	UE_LOG(LogLooter, Log, TEXT("The Lamplighter left the Rest dark at dawn (GoneWhen): no ship on the ledge."));
}
