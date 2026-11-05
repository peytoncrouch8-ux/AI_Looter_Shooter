// LandfallRules: Landfall's names, Main 1's steps, how the ship begins from the campaign, and the crew's line sets.

#include "World/LamplighterRules.h"
#include "AI_Looter_Shooter.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/NewGameIntro.h"
#include "Story/StoryLineSet.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	/** Where STORY's line sets live (create_story_lines.py). */
	const TCHAR* LineSetFolder = TEXT("/Game/Data/Story/");

	/** Line sets already warned about as missing, so a play's log says it once each. */
	TSet<FName>& WarnedSets()
	{
		static TSet<FName> Warned;
		return Warned;
	}
}

namespace LandfallRules
{
	FName MissionId() { static const FName Name(TEXT("Main1")); return Name; }
	FName DeckClearEvent() { static const FName Name(TEXT("Landfall.DeckClear")); return Name; }
	FName ProwTag() { static const FName Name(TEXT("Lamplighter_Prow")); return Name; }
	FName GrappleTag() { static const FName Name(TEXT("Landfall_Grapple")); return Name; }
	FName WinchTag() { static const FName Name(TEXT("Landfall_AnchorWinch")); return Name; }
	FName SpyglassTag() { static const FName Name(TEXT("Landfall_Spyglass")); return Name; }
	FName BargeTag() { static const FName Name(TEXT("Landfall_Barge")); return Name; }
	FName LandingTag() { static const FName Name(TEXT("Landing_Lamplighter")); return Name; }
	FName HobPerchTag() { static const FName Name(TEXT("Lamplighter_HobPerch")); return Name; }
	FName LanternTargetTag() { static const FName Name(TEXT("Speaker_Delia")); return Name; }

	FName BegunFlag() { static const FName Name(TEXT("Landfall.Begun")); return Name; }
	FName AnchoredFlag() { static const FName Name(TEXT("Landfall.Anchored")); return Name; }
	FName RevealedFlag() { static const FName Name(TEXT("Landfall.Revealed")); return Name; }
	FName SaveHold() { static const FName Name(TEXT("Landfall")); return Name; }

	FName DeckLines() { static const FName Name(TEXT("DA_Lines_LandfallDeck")); return Name; }
	FName VaneHornLines() { static const FName Name(TEXT("DA_Lines_LandfallVaneHorn")); return Name; }
	FName BoarderLines() { static const FName Name(TEXT("DA_Lines_LandfallBoarders")); return Name; }
	FName GrappleLines() { static const FName Name(TEXT("DA_Lines_LandfallGrapples")); return Name; }
	FName LandingLines() { static const FName Name(TEXT("DA_Lines_LandfallLanding")); return Name; }
	FName AnchorLines() { static const FName Name(TEXT("DA_Lines_LandfallAnchor")); return Name; }
	FName PointLines() { static const FName Name(TEXT("DA_Lines_LandfallPoint")); return Name; }
	FName VaneValleyLines() { static const FName Name(TEXT("DA_Lines_LandfallVaneValley")); return Name; }
	FName TrailLines() { static const FName Name(TEXT("DA_Lines_LandfallTrail")); return Name; }

	ELandfallStart StartFor(const FCampaignRecord& Campaign)
	{
		const FName Main = MissionId();
		if (Campaign.HasCompleted(Main))
		{
			return ELandfallStart::Moored;
		}
		if (Campaign.ActiveMission == Main)
		{
			// Main 1 running in a save that never flew: the old grave start. One that flew and was saved before she was down
			// (Save & Quit mid-air; the autosaves are held) flies again from the start: nothing of a flight is in a save.
			if (!Campaign.HasFlag(BegunFlag()))
			{
				return ELandfallStart::MooredOldSave;
			}
			return Campaign.ActiveMissionStep <= StepLanding ? ELandfallStart::Flight : ELandfallStart::Moored;
		}
		// Main 1 not begun yet: the story starts on her deck.
		return ELandfallStart::Flight;
	}

	void ApplyStart(FCampaignRecord& Campaign, ELandfallStart Start)
	{
		// Whichever way she begins, the old opening (the Rest's intro, the cold open, the grave) is behind this story for good.
		Campaign.bColdOpenSeen = true;
		Campaign.SetFlag(NewGameIntro::SeenFlag());
		Campaign.SetFlag(BegunFlag());
		const FName Main = MissionId();
		const bool bMainRunning = Campaign.ActiveMission == Main && !Campaign.HasCompleted(Main);
		switch (Start)
		{
		case ELandfallStart::Flight:
			if (bMainRunning && Campaign.ActiveMissionStep > StepProw)
			{
				// Saved mid-flight: the flight replays whole, so its first step comes back too.
				Campaign.ActiveMissionStep = StepProw;
			}
			break;
		case ELandfallStart::MooredOldSave:
			Campaign.SetFlag(AnchoredFlag());
			if (bMainRunning)
			{
				// The old steps meant other things: their optional keys would count for Landfall's (Abel's hat at step 7).
				Campaign.ClearOptionals(Main);
				Campaign.ActiveMissionStep = OldSaveStep;
				Campaign.ReadyMissions.Remove(Main);
			}
			break;
		case ELandfallStart::Moored:
			if (IsAnchoredIn(Campaign))
			{
				Campaign.SetFlag(AnchoredFlag());
			}
			break;
		}
	}

	bool IsAnchoredIn(const FCampaignRecord& Campaign)
	{
		const FName Main = MissionId();
		return Campaign.HasFlag(AnchoredFlag()) || Campaign.HasCompleted(Main)
			|| (Campaign.ActiveMission == Main && Campaign.ActiveMissionStep > StepAnchor);
	}

	int32 MainStepIn(const FCampaignRecord& Campaign)
	{
		const FName Main = MissionId();
		return Campaign.ActiveMission == Main && !Campaign.HasCompleted(Main) ? Campaign.ActiveMissionStep : INDEX_NONE;
	}

	UStoryLineSet* LoadLineSet(FName SetName)
	{
		if (SetName.IsNone())
		{
			return nullptr;
		}
		const FString Package = FString(LineSetFolder) + SetName.ToString();
		const FString ObjectPath = Package + TEXT(".") + SetName.ToString();
		// Loaded already (held by whoever speaks it): no look at the disk. The package check costs a file lookup each call.
		// (Not one the garbage collector has already let go and is about to destroy.)
		UStoryLineSet* Loaded = FindObject<UStoryLineSet>(nullptr, *ObjectPath);
		if (IsValid(Loaded) && !Loaded->IsUnreachable())
		{
			return Loaded;
		}
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *ObjectPath) : nullptr;
	}

	TArray<FStoryLine> LoadLines(FName SetName)
	{
		if (SetName.IsNone())
		{
			return TArray<FStoryLine>();
		}
		const UStoryLineSet* Set = LoadLineSet(SetName);
		if (!Set)
		{
			if (!WarnedSets().Contains(SetName))
			{
				WarnedSets().Add(SetName);
				UE_LOG(LogLooter, Warning, TEXT("Landfall: no line set %s yet (Tools/Unreal/create_story_lines.py makes it), so nobody says it."),
					*SetName.ToString());
			}
			return TArray<FStoryLine>();
		}
		return Set->Lines;
	}

	int32 Say(const UObject* Context, FName SetName, bool bInterrupt)
	{
		const TArray<FStoryLine> Lines = LoadLines(SetName);
		UCaptionSubsystem* Captions = Lines.IsEmpty() ? nullptr : UCaptionSubsystem::Get(Context);
		return Captions ? Captions->Play(Lines, bInterrupt ? ECaptionPlay::Interrupt : ECaptionPlay::Queue) : INDEX_NONE;
	}
}
