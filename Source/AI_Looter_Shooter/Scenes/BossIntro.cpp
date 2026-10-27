// The boss intros: their rules, names, words, scripts (timing, shots, captions) and the camera's math. The scene itself is
// BossIntroScene.cpp's, each boss's own moments BossIntroBosses.cpp's.

#include "Scenes/BossIntro.h"
#include "AI_Looter_Shooter.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryLineSet.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "LooterBossIntro"

namespace
{
	TAutoConsoleVariable<int32> CVarBossIntro(
		TEXT("Looter.BossIntro"),
		1,
		TEXT("The boss intros (the Gravemother, the Foreman, Abel) as each first fight of a session starts: 1 plays them, 0 never."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarBossIntroReplay(
		TEXT("Looter.BossIntro.Replay"),
		0,
		TEXT("1: a boss intro plays at every fight start, seen or not (never on a vigil stone's rematch). For checks."),
		ECVF_Default);

	FBossIntroPoint At(EBossIntroAnchor Anchor, double X, double Y, double Z)
	{
		FBossIntroPoint Point;
		Point.Anchor = Anchor;
		Point.Offset = FVector(X, Y, Z);
		return Point;
	}

	FBossIntroShot Shot(const TCHAR* Name, float Start, float Duration, const FBossIntroPoint& FromEye, const FBossIntroPoint& ToEye,
		const FBossIntroPoint& FromLook, const FBossIntroPoint& ToLook, float FromFov, float ToFov, bool bTrack = false,
		float FromExposure = 0.f, float ToExposure = 0.f)
	{
		FBossIntroShot Made;
		Made.Name = Name;
		Made.Start = Start;
		Made.Duration = Duration;
		Made.FromEye = FromEye;
		Made.ToEye = ToEye;
		Made.FromLook = FromLook;
		Made.ToLook = ToLook;
		Made.FromFov = FromFov;
		Made.ToFov = ToFov;
		Made.bTrackBoss = bTrack;
		Made.FromExposure = FromExposure;
		Made.ToExposure = ToExposure;
		return Made;
	}

	constexpr EBossIntroAnchor OnBoss = EBossIntroAnchor::Boss;
	constexpr EBossIntroAnchor OnPlayer = EBossIntroAnchor::Player;
	constexpr EBossIntroAnchor OnBurst = EBossIntroAnchor::Burst;

	FBossIntroScript MakeGravemother()
	{
		// The Sink from behind the player's shoulder, on her among the graves (0-4.4); she dives at 2.6 and the ground bursts
		// toward the player (3.0-6.2); low and wide on the floor between them (4.4-10.2) as she bursts out at 6.6, climbs
		// (0.8 s, her brain held) and screams (her roar, her fight's first move); the scene ends as the scream ends.
		FBossIntroScript Script;
		Script.Seconds = 10.2f;
		Script.DiveAt = 2.6f;
		Script.TrailPuffs = 5;
		Script.TrailFrom = 3.0f;
		Script.TrailTo = 6.2f;
		Script.LiveAt = 6.6f;
		Script.CardAt = 6.9f;
		Script.CardSeconds = 3.0f;
		Script.HandBackAt = Script.Seconds - BossIntro::HandBackSeconds;
		Script.Music = BossIntroCue::Gravemother;
		Script.Shots = {
			Shot(TEXT("Den"), 0.f, 4.4f, At(OnPlayer, 150.0, 100.0, 170.0), At(OnPlayer, 20.0, 70.0, 150.0),
				At(OnBoss, 0.0, 0.0, 120.0), At(OnBoss, 0.0, 0.0, 90.0), 62.f, 56.f),
			// Under her she's 1.8x a spider: from 10-12 m her whole body and the climb fit with the floor round her.
			Shot(TEXT("Floor"), 4.4f, Script.Seconds - 4.4f, At(OnBurst, 1250.0, -650.0, 240.0), At(OnBurst, 1000.0, -520.0, 190.0),
				At(OnBurst, 0.0, 0.0, 40.0), At(OnBurst, 0.0, 0.0, 170.0), 64.f, 58.f, /*bTrack*/ true),
		};
		Script.Captions = { { 0.6f, 0 }, { 4.8f, 1 } };
		return Script;
	}

	FBossIntroScript MakeForeman()
	{
		// The stope out of the dark: from the player's eyes, pushing along the drift on his lamp at the fall, the exposure
		// rising from near black (0-5.0), his mutter as he digs; then close from his side (5.0-9.8) as he turns from the fall
		// into his lamp's light, the pick handle up and slammed (his own entrance from 5.6: 1.1 s up, the slam, 0.8 s back).
		FBossIntroScript Script;
		Script.Seconds = 9.8f;
		Script.LiveAt = 5.6f;
		Script.CardAt = 6.2f;
		Script.CardSeconds = 3.0f;
		Script.HandBackAt = Script.Seconds - BossIntro::HandBackSeconds;
		Script.Music = BossIntroCue::Foreman;
		Script.Shots = {
			// His lamp on his bowler sits about 2.2 m up at his 1.5x.
			Shot(TEXT("Drift"), 0.f, 5.0f, At(OnPlayer, 80.0, 40.0, 30.0), At(OnPlayer, -260.0, 30.0, 10.0),
				At(OnBoss, 0.0, 0.0, 215.0), At(OnBoss, 0.0, 0.0, 215.0), 56.f, 46.f, false, -4.0f, -1.5f),
			Shot(TEXT("Lamp"), 5.0f, Script.Seconds - 5.0f, At(OnBoss, 520.0, 300.0, 150.0), At(OnBoss, 440.0, 250.0, 160.0),
				At(OnBoss, 0.0, 0.0, 200.0), At(OnBoss, 0.0, 0.0, 175.0), 50.f, 54.f, /*bTrack*/ true, -1.0f, 0.f),
		};
		Script.Captions = { { 1.0f, 0 } };
		return Script;
	}

	FBossIntroScript MakeAbel()
	{
		// The burial boards at dusk from over the player's shoulder at the Keeper's Gate, down the deck to him among the
		// lantern posts with the sun going behind him (0-5.2); then low before him, looking up at him against the sky
		// (5.2-10.4) as he raises his ghost lantern (his own entrance from 5.6, 1.8 s) and says his fight's first line.
		FBossIntroScript Script;
		Script.Seconds = 10.4f;
		Script.LiveAt = 5.6f;
		Script.CardAt = 5.8f;
		Script.CardSeconds = 3.2f;
		Script.HandBackAt = Script.Seconds - BossIntro::HandBackSeconds;
		Script.bClearCaptions = true;
		Script.Music = BossIntroCue::Abel;
		Script.Shots = {
			Shot(TEXT("Boards"), 0.f, 5.2f, At(OnPlayer, 260.0, -140.0, 280.0), At(OnPlayer, 120.0, -100.0, 230.0),
				At(OnBoss, 0.0, 0.0, 150.0), At(OnBoss, 0.0, 0.0, 170.0), 60.f, 52.f),
			Shot(TEXT("Lantern"), 5.2f, Script.Seconds - 5.2f, At(OnBoss, 480.0, -260.0, 30.0), At(OnBoss, 400.0, -210.0, 50.0),
				At(OnBoss, 0.0, 0.0, 190.0), At(OnBoss, 0.0, 0.0, 205.0), 50.f, 46.f, /*bTrack*/ true),
		};
		Script.Captions = { { 0.8f, 0 }, { 6.4f, BossIntro::OwnLine } };
		return Script;
	}

	/** Each kind's line set's package (the asset is its object of the same name). */
	FString LinesPackage(EBossIntro Kind)
	{
		return FString::Printf(TEXT("/Game/Data/Story/DA_Lines_BossIntro%s"), BossIntro::KindName(Kind));
	}
}

// ---------------------------------------------------------------------------
// Rules and names
// ---------------------------------------------------------------------------

bool BossIntro::IsOn()
{
	return CVarBossIntro.GetValueOnGameThread() != 0;
}

bool BossIntro::IsReplayOn()
{
	return CVarBossIntroReplay.GetValueOnGameThread() != 0;
}

bool BossIntro::IsDue(const FCampaignRecord* Campaign, EBossIntro Kind, bool bRematch)
{
	// A vigil stone's call back is a fight the player chose to have again: straight in, always.
	if (!IsOn() || bRematch)
	{
		return false;
	}
	if (IsReplayOn())
	{
		return true;
	}
	return !Campaign || !Campaign->HasFlag(SeenFlag(Kind));
}

const TCHAR* BossIntro::KindName(EBossIntro Kind)
{
	switch (Kind)
	{
	case EBossIntro::Gravemother:
		return TEXT("Gravemother");
	case EBossIntro::Foreman:
		return TEXT("Foreman");
	case EBossIntro::Abel:
	default:
		return TEXT("Abel");
	}
}

bool BossIntro::ParseKind(const FString& Name, EBossIntro& OutKind)
{
	for (const EBossIntro Kind : { EBossIntro::Gravemother, EBossIntro::Foreman, EBossIntro::Abel })
	{
		if (Name.Equals(KindName(Kind), ESearchCase::IgnoreCase))
		{
			OutKind = Kind;
			return true;
		}
	}
	return false;
}

FName BossIntro::SceneName(EBossIntro Kind)
{
	return FName(*FString::Printf(TEXT("BossIntro.%s"), KindName(Kind)));
}

FName BossIntro::SeenFlag(EBossIntro Kind)
{
	return FName(*FString::Printf(TEXT("BossIntro.Seen.%s"), KindName(Kind)));
}

FString BossIntro::LinesAssetPath(EBossIntro Kind)
{
	const FString Package = LinesPackage(Kind);
	return FString::Printf(TEXT("%s.%s"), *Package, *FPackageName::GetShortName(Package));
}

// ---------------------------------------------------------------------------
// The words and the cards
// ---------------------------------------------------------------------------

const TArray<FStoryLine>& BossIntro::ScriptLines(EBossIntro Kind)
{
	// Short captions said on the move (the talk cut's rule), no family words (the same for every hero). Index = a caption's
	// Line. Hob's are the guide's (his "sunshine"); Teague's mutter sets up his last word ("...Tell the company it was the
	// roof.", ForemanFight::LastWord); Hob's for Abel is Docs/Story.md's own phrase for why the Keeper walks.
	static const TArray<FStoryLine> Gravemother = {
		FStoryLine::Make(LOCTEXT("Hob", "Hob"), LOCTEXT("GravemotherLine0", "Every spider in the Rest answers to her.")),
		FStoryLine::Make(LOCTEXT("Hob", "Hob"), LOCTEXT("GravemotherLine1", "Crack that headstone off her back first, sunshine.")),
	};
	static const TArray<FStoryLine> Foreman = {
		FStoryLine::Make(LOCTEXT("Teague", "Cyrus Teague"), LOCTEXT("ForemanLine0", "Dig. Keep digging. It was the roof.")),
	};
	static const TArray<FStoryLine> Abel = {
		FStoryLine::Make(LOCTEXT("Hob", "Hob"), LOCTEXT("AbelLine0", "A keeper doesn't lie still while his saint is dark.")),
	};
	switch (Kind)
	{
	case EBossIntro::Gravemother:
		return Gravemother;
	case EBossIntro::Foreman:
		return Foreman;
	case EBossIntro::Abel:
	default:
		return Abel;
	}
}

TArray<FStoryLine> BossIntro::Lines(EBossIntro Kind)
{
	TArray<FStoryLine> Words = ScriptLines(Kind);
	// A writer's words win, whole: a set with a line missing or extra would put words on the wrong moment.
	const FString Package = LinesPackage(Kind);
	const UStoryLineSet* Set = FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *LinesAssetPath(Kind)) : nullptr;
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
		UE_LOG(LogLooter, Warning, TEXT("Boss intro: %s has %d lines, the intro has %d: the written words play (create_story_lines.py)."),
			*Package, Set->Lines.Num(), Words.Num());
	}
	return Words;
}

FText BossIntro::CardTitle(EBossIntro Kind)
{
	switch (Kind)
	{
	case EBossIntro::Gravemother:
		return LOCTEXT("CardGravemother", "THE GRAVEMOTHER");
	case EBossIntro::Foreman:
		return LOCTEXT("CardForeman", "CYRUS TEAGUE");
	case EBossIntro::Abel:
	default:
		return LOCTEXT("CardAbel", "ABEL RANSOM");
	}
}

FText BossIntro::CardSub(EBossIntro Kind)
{
	switch (Kind)
	{
	case EBossIntro::Gravemother:
		return LOCTEXT("SubGravemother", "MOTHER OF THE SINK'S BROOD");
	case EBossIntro::Foreman:
		return LOCTEXT("SubForeman", "FOREMAN OF THE DEAD SHIFT");
	case EBossIntro::Abel:
	default:
		// His headboard's own words (Docs/Story.md, the cold open's grave).
		return LOCTEXT("SubAbel", "KEEPER OF SAINT ADA");
	}
}

const FBossIntroScript& BossIntro::Script(EBossIntro Kind)
{
	static const FBossIntroScript Gravemother = MakeGravemother();
	static const FBossIntroScript Foreman = MakeForeman();
	static const FBossIntroScript Abel = MakeAbel();
	switch (Kind)
	{
	case EBossIntro::Gravemother:
		return Gravemother;
	case EBossIntro::Foreman:
		return Foreman;
	case EBossIntro::Abel:
	default:
		return Abel;
	}
}

// ---------------------------------------------------------------------------
// The camera's math
// ---------------------------------------------------------------------------

FBossIntroFrame FBossIntroFrame::Make(const FVector& BossFeet, const FVector& PlayerEye, const FVector& BurstFeet, const FVector& BossFacing)
{
	FBossIntroFrame Frame;
	Frame.Boss = BossFeet;
	Frame.Player = PlayerEye;
	Frame.Burst = BurstFeet;
	FVector Forward = PlayerEye - BossFeet;
	Forward.Z = 0.0;
	if (!Forward.Normalize())
	{
		// The player right over it: the way the boss faces is the way to frame it from.
		Forward = FVector(BossFacing.X, BossFacing.Y, 0.0);
		if (!Forward.Normalize())
		{
			Forward = FVector::ForwardVector;
		}
	}
	Frame.Forward = Forward;
	Frame.Right = FVector::CrossProduct(FVector::UpVector, Forward);
	return Frame;
}

FVector FBossIntroFrame::Resolve(const FBossIntroPoint& Point) const
{
	const FVector& Origin = Point.Anchor == EBossIntroAnchor::Player ? Player : (Point.Anchor == EBossIntroAnchor::Burst ? Burst : Boss);
	return Origin + Forward * Point.Offset.X + Right * Point.Offset.Y + FVector::UpVector * Point.Offset.Z;
}

float BossIntro::Ease(float Alpha)
{
	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	return A * A * A * (A * (A * 6.f - 15.f) + 10.f);
}

FBossIntroView BossIntro::ViewAlong(const FBossIntroShot& Shot, const FBossIntroFrame& Frame, float Alpha, const FVector& BossShift)
{
	const float A = Ease(Alpha);
	FBossIntroView View;
	View.Location = FMath::Lerp(Frame.Resolve(Shot.FromEye), Frame.Resolve(Shot.ToEye), static_cast<double>(A));
	FVector Look = FMath::Lerp(Frame.Resolve(Shot.FromLook), Frame.Resolve(Shot.ToLook), static_cast<double>(A));
	if (Shot.bTrackBoss)
	{
		Look += BossShift;
	}
	View.Rotation = (Look - View.Location).Rotation();
	View.Rotation.Roll = 0.f;
	View.Fov = FMath::Lerp(Shot.FromFov, Shot.ToFov, A);
	View.Exposure = FMath::Lerp(Shot.FromExposure, Shot.ToExposure, A);
	return View;
}

FVector BossIntro::BoomClip(const FVector& Look, const FVector& Eye, float HitShare, float Margin, float Shortest)
{
	const FVector Along = Eye - Look;
	const double Length = Along.Size();
	if (Length <= KINDA_SMALL_NUMBER || HitShare >= 1.f)
	{
		return Eye;
	}
	// In front of what's in the way, but never so near that the boss fills the lens (a wall right behind it).
	const double Kept = FMath::Max(static_cast<double>(FMath::Clamp(HitShare, 0.f, 1.f)) * Length - Margin, FMath::Min<double>(Shortest, Length));
	return Look + Along / Length * Kept;
}

FVector BossIntro::BurstAlong(const FVector& HerFeet, const FVector& PlayerFeet, const FVector& Home)
{
	FVector Toward = PlayerFeet - HerFeet;
	Toward.Z = 0.0;
	const double Gap = Toward.Size();
	if (Gap <= KINDA_SMALL_NUMBER)
	{
		return HerFeet;
	}
	Toward /= Gap;
	// A share of the way, kept off the player: never past where she stands (a player right on her: she bursts where she is).
	const double Along = FMath::Clamp(FMath::Min(Gap * BurstShare, Gap - BurstMinFromPlayer), 0.0, Gap);
	FVector Spot = HerFeet + Toward * Along;
	// On her own ground: pulled back toward her home when it would leave it.
	FVector FromHome = Spot - Home;
	FromHome.Z = 0.0;
	if (FromHome.Size() > BurstMaxFromHome)
	{
		Spot = FVector(Home.X, Home.Y, Spot.Z) + FromHome.GetSafeNormal() * BurstMaxFromHome;
	}
	Spot.Z = HerFeet.Z;
	return Spot;
}

#undef LOCTEXT_NAMESPACE
