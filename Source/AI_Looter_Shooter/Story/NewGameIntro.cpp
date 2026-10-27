// The new game's intro: its rules, its words and its captions' timing (the shots are NewGameIntroShots.cpp's, the Rest
// scene NewGameIntroRest.cpp's, the planet part UNewGameIntroSubsystem's).

#include "Story/NewGameIntro.h"
#include "AI_Looter_Shooter.h"
#include "Session/CampaignRecord.h"
#include "Story/HeroLines.h"
#include "Story/StoryLineSet.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "LooterNewGameIntro"

namespace
{
	TAutoConsoleVariable<int32> CVarIntro(
		TEXT("Looter.Intro"),
		1,
		TEXT("The new game's intro (Teropa from orbit, then Ransom's Rest) before the cold open: 1 plays it, 0 passes it."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarLetterbox(
		TEXT("Looter.Intro.Letterbox"),
		0.1f,
		TEXT("The intro's letterbox: each bar's share of the screen's height (0.1 frames 16:9 near 2:1; 0 none)."),
		ECVF_Default);

	/** The line set's package (the asset is its object of the same name). */
	const TCHAR* LinesPackage = TEXT("/Game/Data/Story/DA_Lines_NewGameIntro");
}

FName NewGameIntro::SceneName()
{
	static const FName Name(TEXT("NewGameIntro"));
	return Name;
}

FName NewGameIntro::SeenFlag()
{
	static const FName Flag(TEXT("Intro.Seen"));
	return Flag;
}

FString NewGameIntro::LinesAssetPath()
{
	return FString::Printf(TEXT("%s.%s"), LinesPackage, *FPackageName::GetShortName(LinesPackage));
}

FName NewGameIntro::PlanetTag()
{
	static const FName Tag(TEXT("MenuPlanet"));
	return Tag;
}

bool NewGameIntro::IsOn()
{
	return CVarIntro.GetValueOnGameThread() != 0;
}

float NewGameIntro::LetterboxFraction()
{
	// More than a fifth of the screen each would leave a strip: the shots are framed for about 2:1.
	return FMath::Clamp(CVarLetterbox.GetValueOnGameThread(), 0.f, 0.2f);
}

bool NewGameIntro::IsDue(const FCampaignRecord& Campaign)
{
	// The cold open's own rule (UColdOpenSubsystem::IsDue) and not yet seen: a session past its cold open never sees it.
	return Campaign.bFirstCastOff && !Campaign.bColdOpenSeen && !Campaign.HasFlag(SeenFlag());
}

const TArray<FStoryLine>& NewGameIntro::ScriptLines()
{
	// Docs/Story.md's facts in short captions (the talk cut's rule): The world (Teropa, the Reaches, the Rim, the dead
	// given to the wind, the saints), the Dunne Gang (seven towns, seven embers, the Rest the seventh, seven days ago), the
	// Unpaid (the dead lost the light and drifted home), the cold open (the player rode in with the gang) and the logline.
	// No family words: the same for every hero (Pipeline/Story.md, "Writing new lines"). Index = the captions' Line.
	static const TArray<FStoryLine> Lines = {
		// The planet part (or Westward).
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L0", "One sun, one moon, wide seas, and one great continent.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L1", "Its railroad runs west to the Rim, and no farther.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L2", "Out here the dead are given to the wind.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L3", "At dusk the Gravewind carries them west, down the Sundown Road.")),
		// Ransom's Rest.
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L4", "Every town keeps a saint, whose light shows its dead the way across.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L5", "Seven days ago the Dunne Gang robbed Saint Ada, and the Rest went dark.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L6", "Seven towns. Seven saints. Not one light left burning.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L7", "The dead lost the light on the road, and drifted home.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L8", "You rode in with that gang.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L9", "Seven outlaws killed you.")),
		FStoryLine::Make(FText::GetEmpty(), LOCTEXT("L10", "Something sent you back to bring them in.")),
	};
	return Lines;
}

TArray<FStoryLine> NewGameIntro::Lines()
{
	TArray<FStoryLine> Words = ScriptLines();
	// A writer's words win, whole: a set with a line missing or extra would put words on the wrong shot.
	const UStoryLineSet* Set = FPackageName::DoesPackageExist(LinesPackage) ? LoadObject<UStoryLineSet>(nullptr, *LinesAssetPath()) : nullptr;
	if (Set && Set->Lines.Num() == Words.Num())
	{
		for (int32 Index = 0; Index < Words.Num(); ++Index)
		{
			if (!Set->Lines[Index].Text.IsEmpty())
			{
				Words[Index].Text = Set->Lines[Index].Text;
			}
		}
	}
	else if (Set)
	{
		UE_LOG(LogLooter, Warning, TEXT("Intro: %s has %d lines, the intro has %d slots: the written words play (create_story_lines.py)."),
			LinesPackage, Set->Lines.Num(), Words.Num());
	}
	return HeroLines::ResolveLines(Words);
}

const TArray<FIntroCaption>& NewGameIntro::PlanetCaptions()
{
	// One thing on screen at a time: a card, then its lines, each held at least a reader's time (FStoryLine::GetSeconds),
	// all done before the white rises at PlanetWhiteAt.
	static const TArray<FIntroCaption> Captions = {
		{ 1.0f, 3.8f, EIntroCard::Teropa },
		{ 5.0f, 4.4f, EIntroCard::None, 0 },
		{ 9.6f, 4.1f, EIntroCard::None, 1 },
		{ 13.9f, 3.4f, EIntroCard::Reaches },
		{ 17.5f, 3.5f, EIntroCard::None, 2 },
		{ 21.1f, 4.9f, EIntroCard::None, 3 },
	};
	return Captions;
}

const TArray<FIntroCaption>& NewGameIntro::RestCaptions()
{
	// Shots (RestMoves): over the Rim 0-15, the chapel 15-27, the farm 27-37, the grave 37-50.5. Nothing within a cut's
	// dip; the white rises at RestWhiteAt.
	static const TArray<FIntroCaption> Captions = {
		{ 2.0f, 3.8f, EIntroCard::Rest },
		{ 6.0f, 5.3f, EIntroCard::None, 4 },
		{ 15.4f, 5.6f, EIntroCard::None, 5 },
		{ 21.2f, 4.4f, EIntroCard::None, 6 },
		{ 27.6f, 4.4f, EIntroCard::None, 7 },
		{ 32.4f, 3.0f, EIntroCard::None, 8 },
		{ 37.6f, 3.8f, EIntroCard::Hero },
		{ 41.6f, 2.7f, EIntroCard::None, 9 },
		{ 44.4f, 3.6f, EIntroCard::None, 10 },
	};
	return Captions;
}

FText NewGameIntro::CardTitle(EIntroCard Card)
{
	switch (Card)
	{
	case EIntroCard::Teropa:
		return LOCTEXT("CardTeropa", "TEROPA");
	case EIntroCard::Reaches:
		return LOCTEXT("CardReaches", "THE REACHES");
	case EIntroCard::Rest:
		return LOCTEXT("CardRest", "RANSOM'S REST");
	default:
		return FText::GetEmpty();
	}
}

FText NewGameIntro::CardSub(EIntroCard Card)
{
	switch (Card)
	{
	case EIntroCard::Reaches:
		return LOCTEXT("SubReaches", "THE WESTERN FRONTIER");
	case EIntroCard::Rest:
		return LOCTEXT("SubRest", "A FARM VALLEY ON THE RIM");
	default:
		return FText::GetEmpty();
	}
}

float NewGameIntro::RestSceneSeconds(bool bWestward)
{
	return (bWestward ? PlanetSeconds : 0.f) + RestSeconds;
}

#undef LOCTEXT_NAMESPACE
