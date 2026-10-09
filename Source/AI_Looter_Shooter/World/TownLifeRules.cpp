#include "World/TownLifeRules.h"
#include "Audio/LooterSoundCues.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

namespace
{
	const FName Ransom(TEXT("Ransom"));
	const FName Bright(TEXT("Bright"));
	const FName Pruitt(TEXT("Pruitt"));
	const FName CottageNorth(TEXT("CottageNorth"));
	const FName CottageSouth(TEXT("CottageSouth"));
	const FName Cottage(TEXT("Cottage"));

	/** The missions the mutters follow, by id as the campaign record keeps them. */
	const FName Main3(TEXT("Main3"));
	const FName Main4(TEXT("Main4"));
	const FName Main6(TEXT("Main6"));

	/** How far the story has come for a mutter: the town afraid, then the gate won, the bell rung, Abel at rest. */
	enum class EStage : uint8
	{
		Afraid,
		GateWon,
		BellRung,
		AbelAtRest,
	};

	struct FMutter
	{
		FName Household;
		EStage Stage;
		const TCHAR* Speaker;
		const TCHAR* Text;
	};

	// Townsfolk behind their doors as the corpse walks past (original lines, written for the game). Nobody is seen, and
	// nobody here is a named character of the story: the storekeeper's family, a couple and an old father, a mother and
	// her child. They know only what a town would: Tilly dressed Ellis, the gate was fought, the bell rang, Abel is quiet.
	const FMutter Mutters[] = {
		{ Pruitt, EStage::Afraid, TEXT("A woman, inside"), TEXT("Bar the door, Hollis. It's coming up the street.") },
		{ Pruitt, EStage::Afraid, TEXT("A man, inside"), TEXT("Lord. That's the Ransom child. Was the Ransom child.") },
		{ Pruitt, EStage::GateWon, TEXT("A man, inside"), TEXT("Tilly Bright says it put down the ones at the gate.") },
		{ Pruitt, EStage::GateWon, TEXT("A woman, inside"), TEXT("Still a corpse, Hollis. Grateful don't change that.") },
		{ Pruitt, EStage::BellRung, TEXT("A woman, inside"), TEXT("That was the chapel bell. First time in a week.") },
		{ Pruitt, EStage::AbelAtRest, TEXT("A man, inside"), TEXT("They say Abel sits quiet out on Gravewind Point now. The child did that.") },

		{ CottageNorth, EStage::Afraid, TEXT("A woman, inside"), TEXT("Away from the window, Walt. It goes by if you let it.") },
		{ CottageNorth, EStage::Afraid, TEXT("An old man, inside"), TEXT("Ransoms. Always did come home at the wrong hour.") },
		{ CottageNorth, EStage::GateWon, TEXT("An old man, inside"), TEXT("Dead shooting the dead at the gate. Never thought I'd live to see it.") },
		{ CottageNorth, EStage::BellRung, TEXT("A woman, inside"), TEXT("Father Aldana says it's helping. Father Aldana says a lot of things.") },
		{ CottageNorth, EStage::AbelAtRest, TEXT("A woman, inside"), TEXT("Set a plate on the step tonight, Walt. Like Delia does.") },

		{ CottageSouth, EStage::Afraid, TEXT("A woman, inside"), TEXT("Hush now. The dead hear their names.") },
		{ CottageSouth, EStage::Afraid, TEXT("A child, inside"), TEXT("Mama, is that the one Miss Tilly dressed?") },
		{ CottageSouth, EStage::GateWon, TEXT("A child, inside"), TEXT("It's fighting the bad ones, Mama. I watched.") },
		{ CottageSouth, EStage::BellRung, TEXT("A child, inside"), TEXT("Did the bell send them home, Mama?") },
		{ CottageSouth, EStage::AbelAtRest, TEXT("A child, inside"), TEXT("I waved at it, Mama. It waved back.") },
	};

	TArray<TownLifeRules::FHousehold> MakeHouseholds()
	{
		using namespace LooterSoundCue::TownLife;
		const FName V(Voices), C(Cough), M(MusicBox), L(Latch), K(Creak), H(Hush), W(Workshop);
		return {
			{ Ransom, { K, K, L, C }, 1.18f, false },
			{ Bright, { W, W, K, L }, 1.1f, false },
			{ Pruitt, { V, V, C, H, K, L }, 1.f, true },
			{ CottageNorth, { V, C, C, K, L }, 0.9f, true },
			{ CottageSouth, { M, M, H, V, K, L }, 1.08f, true },
			{ Cottage, { V, C, K, L }, 1.f, true },
		};
	}
}

const TArray<TownLifeRules::FHousehold>& TownLifeRules::Households()
{
	static const TArray<FHousehold> All = MakeHouseholds();
	return All;
}

const TownLifeRules::FHousehold* TownLifeRules::FindHousehold(FName Id)
{
	return Households().FindByPredicate([Id](const FHousehold& Each) { return Each.Id == Id; });
}

FName TownLifeRules::HouseholdForMesh(const FString& MeshName)
{
	if (MeshName.Equals(TEXT("SM_Farmhouse_Ransom"), ESearchCase::IgnoreCase))
	{
		return Ransom;
	}
	if (MeshName.Equals(TEXT("SM_FalseFront_Undertaker"), ESearchCase::IgnoreCase))
	{
		return Bright;
	}
	if (MeshName.Equals(TEXT("SM_FalseFront_Store"), ESearchCase::IgnoreCase))
	{
		return Pruitt;
	}
	if (MeshName.Equals(TEXT("SM_Cottage_Ransom"), ESearchCase::IgnoreCase))
	{
		return Cottage;
	}
	return NAME_None;
}

FName TownLifeRules::HouseholdForHouse(const AActor* House)
{
	if (!House)
	{
		return NAME_None;
	}
	TInlineComponentArray<UStaticMeshComponent*> Meshes(House);
	for (const UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->GetStaticMesh())
		{
			return HouseholdForMesh(Mesh->GetStaticMesh()->GetName());
		}
	}
	return NAME_None;
}

bool TownLifeRules::IsVoice(FName Cue)
{
	using namespace LooterSoundCue::TownLife;
	return Cue == FName(Voices) || Cue == FName(Cough) || Cue == FName(Hush);
}

TArray<FSpeakerTopic> TownLifeRules::MutterTopicsFor(FName Household)
{
	// The latest part of the story first: a speaker point says the first topic that holds.
	const struct
	{
		EStage Stage;
		FName After;
	} Stages[] = {
		{ EStage::AbelAtRest, Main6 },
		{ EStage::BellRung, Main4 },
		{ EStage::GateWon, Main3 },
		{ EStage::Afraid, NAME_None },
	};
	TArray<FSpeakerTopic> Topics;
	for (const auto& Stage : Stages)
	{
		FSpeakerTopic Topic;
		if (!Stage.After.IsNone())
		{
			Topic.When.AfterMissions.Add(Stage.After);
		}
		for (const FMutter& Mutter : Mutters)
		{
			if (Mutter.Household == Household && Mutter.Stage == Stage.Stage)
			{
				Topic.Lines.Add(FStoryLine::Make(FText::FromString(Mutter.Speaker), FText::FromString(Mutter.Text)));
			}
		}
		if (!Topic.Lines.IsEmpty())
		{
			Topics.Add(MoveTemp(Topic));
		}
	}
	return Topics;
}

// ---------------------------------------------------------------------------
// The clock
// ---------------------------------------------------------------------------

void FTownLifeClock::MarkNear(FName Household, double Now)
{
	LastNear.Add(Household, Now);
}

bool FTownLifeClock::IsFreshVisit(FName Household, double Now) const
{
	const double* Last = LastNear.Find(Household);
	return !Last || Now - *Last >= TownLifeRules::AwaySeconds;
}

bool FTownLifeClock::WantsSound(FName Household, double Now, bool bFresh, FRandomStream& Random) const
{
	const double* HouseReady = HouseNext.Find(Household);
	if (Now < NextSound || (HouseReady && Now < *HouseReady))
	{
		return false;
	}
	return Random.FRand() < (bFresh ? TownLifeRules::FirstChance : TownLifeRules::AgainChance);
}

FName FTownLifeClock::PickSound(const TownLifeRules::FHousehold& Household, FRandomStream& Random)
{
	const int32 Count = Household.Sounds.Num();
	if (Count == 0)
	{
		return NAME_None;
	}
	const int32* Last = LastPick.Find(Household.Id);
	int32 Pick = Random.RandRange(0, Count - 1);
	// Never the same thing twice running from one house (a cue listed twice counts as the same thing).
	if (Last && Household.Sounds.IsValidIndex(*Last))
	{
		const FName LastCue = Household.Sounds[*Last];
		for (int32 Tries = 0; Tries < Count && Household.Sounds[Pick] == LastCue; ++Tries)
		{
			Pick = (Pick + 1) % Count;
		}
	}
	LastPick.Add(Household.Id, Pick);
	return Household.Sounds[Pick];
}

void FTownLifeClock::Played(FName Household, double Now, FRandomStream& Random)
{
	NextSound = Now + Random.FRandRange(TownLifeRules::GapMin, TownLifeRules::GapMax);
	HouseNext.Add(Household, Now + Random.FRandRange(TownLifeRules::HouseRestMin, TownLifeRules::HouseRestMax));
}

bool FTownLifeClock::WantsDog(double Now, FRandomStream& Random) const
{
	return Now >= NextDog && Random.FRand() < TownLifeRules::DogChance;
}

void FTownLifeClock::DogBarked(double Now, FRandomStream& Random)
{
	NextDog = Now + Random.FRandRange(TownLifeRules::DogRestMin, TownLifeRules::DogRestMax);
}

void FTownLifeClock::Muttered(double Now, FRandomStream& Random)
{
	NextMutter = Now + Random.FRandRange(TownLifeRules::MutterGapMin, TownLifeRules::MutterGapMax);
}
