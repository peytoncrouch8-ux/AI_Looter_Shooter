// ALockedDoor: a door that opens only with its key carried, or once the story says (Tilly's cellar door).

#include "World/LockedDoor.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "LockedDoor"

ALockedDoor::ALockedDoor()
{
	// A door: opened and shut by a tap, swinging about its hinge.
	UseMode = EInteractablePropUse::Toggle;
	Effect = EInteractablePropEffect::Swing;
	OpenAngle = -90.f;
	SwingSeconds = 0.6f;
	CooldownSeconds = 0.6f;
	Prompt = LOCTEXT("Open", "Open the door");
	PromptWhenOn = LOCTEXT("Close", "Close the door");
	LockedPrompt = LOCTEXT("Locked", "Locked");
}

bool ALockedDoor::IsUnlocked() const
{
	if (!HasLock())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner)
	{
		return false;
	}
	if (!KeyItem.IsNone() && Runner->IsCarrying(KeyItem))
	{
		return true;
	}
	return !UnlockedWhen.IsEmpty() && UnlockedWhen.IsMet(Runner->GetCampaign(), Runner);
}

FInteractionOptions ALockedDoor::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options = Super::GetInteractionOptions(User);
	// Open, it can always be shut; unlocked, it's the door it always was.
	if (IsOn() || IsUnlocked())
	{
		return Options;
	}
	// Locked: still a thing to try, so the prompt says why it won't open.
	Options.bUsable = bEnabled;
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = LockedPrompt;
	Options.HoldPrompt = FText::GetEmpty();
	return Options;
}

bool ALockedDoor::Interact(UInteractionComponent& User, bool bHeld)
{
	if (IsOn() || IsUnlocked())
	{
		return Super::Interact(User, bHeld);
	}
	TryLocked();
	// Nothing was used: the missions hear nothing, and the door stays as it is.
	return false;
}

bool ALockedDoor::TryLocked()
{
	if (IsUnlocked())
	{
		return false;
	}
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastRattle < RattleRest)
	{
		return true;
	}
	LastRattle = Now;
	++Rattles;
	// The latch lifted against its bolt: the same thumb latch the town's doors make behind their walls.
	LooterSound::PlayAt(this, LooterSoundCue::TownLife::Latch, GetInteractionLocation().Get(GetActorLocation()));
	if (!LockedLines.IsEmpty())
	{
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			Captions->Play(LockedLines, ECaptionPlay::Queue);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: locked%s."), *GetActorNameOrLabel(),
		KeyItem.IsNone() ? TEXT("") : *FString::Printf(TEXT(" (its key: %s)"), *KeyItem.ToString()));
	return true;
}

#undef LOCTEXT_NAMESPACE
