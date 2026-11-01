// UAbelCorruptionComponent: the drink. Out over the canyon in "The Hose" he runs a smog line to one of the six shrouded dead
// on the boards at a time (MAINS_2_7.md section 11: "Shoot the line where it meets the body, its collar of smog, 3 hits, to
// break it. A body drunk out heals him 5% and rises as a Restless Unpaid, and the optional fails"). Their spots come from
// the build (BoardsDead: build_area_deck.py, from the biers build_area_main2.py lays them on); their shrouds are the story
// props there, hidden here while a body is drunk out and shown again when the fight starts over.

#include "Bosses/AbelCorruption.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/UnpaidCreature.h"
#include "Story/CaptionSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/StoryProp.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "LooterAbel"

namespace
{
	/** A shroud is the story prop within this of a body's spot (cm). */
	constexpr float ShroudNear = 160.f;
	/** He starts his first line this long after he's settled in the fog (s): the player sees him out there first. */
	constexpr float FirstDrinkAfter = 1.5f;
	/** A drunk body rises this far toward the deck's middle from its bier (cm), standing on the boards. */
	constexpr float RiseBeside = 120.f;
	/** The gold of a body saved, the cyan of a collar hit (linear). */
	const FLinearColor SavedGold(1.f, 0.85f, 0.55f);

	FWeaponFX* DrinkEffects(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}
}

FVector UAbelCorruptionComponent::GetDeadSpot(int32 Index) const
{
	return BoardsDead.IsValidIndex(Index) ? BoardsDead[Index] : FVector::ZeroVector;
}

bool UAbelCorruptionComponent::GetDeadToWalk(const FVector& From, FVector& OutBody) const
{
	bool bFound = false;
	double Best = 0.0;
	for (int32 Index = 0; Index < BoardsDead.Num(); ++Index)
	{
		const double Distance = FVector::DistSquared2D(BoardsDead[Index], From);
		// Not one he's drunk (it's up and fighting), nor one under his feet (no way to turn to it).
		if (IsDrunk(Index) || Distance < 60.0 * 60.0)
		{
			continue;
		}
		if (!bFound || Distance < Best)
		{
			bFound = true;
			Best = Distance;
			OutBody = BoardsDead[Index];
		}
	}
	return bFound;
}

int32 UAbelCorruptionComponent::NumDrunk() const
{
	int32 Count = 0;
	for (const bool bDrunk : Drunk)
	{
		Count += bDrunk ? 1 : 0;
	}
	return Count;
}

void UAbelCorruptionComponent::FindShrouds()
{
	if (bShroudsFound)
	{
		return;
	}
	bShroudsFound = true;
	Shrouds.Init(nullptr, BoardsDead.Num());
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AStoryProp> It(World); It; ++It)
	{
		for (int32 Index = 0; Index < BoardsDead.Num(); ++Index)
		{
			if (!Shrouds[Index].IsValid() && FVector::Dist(It->GetActorLocation(), BoardsDead[Index]) <= ShroudNear)
			{
				Shrouds[Index] = *It;
			}
		}
	}
}

void UAbelCorruptionComponent::TickDrink(float DeltaSeconds)
{
	AAbelKeeper* Abel = GetAbel();
	const UBossComponent* Boss = GetBoss();
	if (!Abel || !Boss)
	{
		return;
	}
	if (Drunk.Num() != BoardsDead.Num())
	{
		Drunk.Init(false, BoardsDead.Num());
	}
	// A drunk body stays gone though the story props read the missions again (theirs say "shown during Main 7").
	for (int32 Index = 0; Index < Drunk.Num(); ++Index)
	{
		AActor* Shroud = Shrouds.IsValidIndex(Index) ? Shrouds[Index].Get() : nullptr;
		if (Drunk[Index] && Shroud && !Shroud->IsHidden())
		{
			Shroud->SetActorHiddenInGame(true);
		}
	}
	// He drinks only from the fog over the canyon, in the real fight (the echo's boards are bare: the six went west).
	const bool bInFog = Abel->GetMove() == EAbelMove::InFog && Boss->IsFighting() && !Abel->IsDead() && !IsRematch() && !BoardsDead.IsEmpty();
	if (Drinking != INDEX_NONE)
	{
		if (!bInFog)
		{
			StopDrink(false);
			return;
		}
		DrinkTime += DeltaSeconds;
		if (CollarHit)
		{
			CollarHit->SetWorldLocation(GetDeadSpot(Drinking));
		}
		if (DrinkTime >= AbelCorruptionRules::DrinkSeconds)
		{
			DrinkOut(Drinking);
		}
		return;
	}
	if (!bInFog)
	{
		DrinkRestLeft = FirstDrinkAfter;
		return;
	}
	DrinkRestLeft -= DeltaSeconds;
	if (DrinkRestLeft <= 0.f)
	{
		const int32 Next = AbelCorruptionRules::NextDrink(Drunk, LastDrunkTried);
		if (Next == INDEX_NONE || !StartDrink(Next))
		{
			DrinkRestLeft = AbelCorruptionRules::DrinkRest;
		}
	}
}

bool UAbelCorruptionComponent::StartDrink(int32 Index)
{
	AAbelKeeper* Abel = GetAbel();
	if (!Abel || !BoardsDead.IsValidIndex(Index) || IsDrunk(Index) || Drinking != INDEX_NONE || !Abel->IsOutInFog() || IsRematch())
	{
		return false;
	}
	FindShrouds();
	Drinking = Index;
	LastDrunkTried = Index;
	DrinkTime = 0.f;
	CollarHits = 0;
	const FVector Body = GetDeadSpot(Index);
	if (CollarHit)
	{
		CollarHit->SetWorldLocation(Body);
		CollarHit->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	// The smog's bite on the body, and its drinking heard on it from now on (the hose's sounds: his is a hose of smog).
	LooterSound::PlayAt(Abel, LooterSoundCue::DredgerHoseLatch, Body);
	LooterSound::Stop(DrinkSound.Get(), 0.1f);
	DrinkSound = LooterSound::Start(Abel, LooterSoundCue::DredgerHoseDrain, CollarHit.Get());
	if (!bDrinkLineSaid)
	{
		bDrinkLineSaid = true;
		static const FText Hob = LOCTEXT("Hob", "Hob");
		Say(TEXT("DA_Lines_AbelDrink"), { FStoryLine::Make(Hob, LOCTEXT("DrinkHint", "He's drinking the dead! Shoot the smog where it bites!"), 3.f) });
	}
	UE_LOG(LogLooter, Log, TEXT("%s: a smog line to the dead on bier %d (%d of %d drunk)."), *Abel->GetActorNameOrLabel(), Index, NumDrunk(), BoardsDead.Num());
	return true;
}

bool UAbelCorruptionComponent::HitCollar()
{
	if (Drinking == INDEX_NONE)
	{
		return false;
	}
	++CollarHits;
	const FVector Body = GetDeadSpot(Drinking);
	if (FWeaponFX* Effects = DrinkEffects(GetWorld()))
	{
		Effects->SpawnSoulTatters(Body + FVector(0.0, 0.0, 30.0), FVector::UpVector, DredgerLook::SoulCyan, 30.f, 3 + CollarHits);
	}
	if (CollarHits < AbelCorruptionRules::CollarHits)
	{
		// A bite in the smog: a hard tink, a little higher each time (two more, one more).
		LooterSound::PlayAt(GetOwner(), LooterSoundCue::ImpactMetal, Body, 0.7f, 1.1f + 0.15f * static_cast<float>(CollarHits));
		return true;
	}
	StopDrink(true);
	return true;
}

void UAbelCorruptionComponent::StopDrink(bool bBroken)
{
	if (Drinking == INDEX_NONE)
	{
		return;
	}
	const FVector Body = GetDeadSpot(Drinking);
	Drinking = INDEX_NONE;
	DrinkTime = 0.f;
	CollarHits = 0;
	DrinkRestLeft = AbelCorruptionRules::DrinkRest;
	if (CollarHit)
	{
		CollarHit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	LooterSound::Stop(DrinkSound.Get(), 0.25f);
	DrinkSound.Reset();
	if (!bBroken)
	{
		return;
	}
	// Broken off the body: the snap, the soul it was drinking settling back into its shroud in a breath of gold.
	LooterSound::PlayAt(GetOwner(), LooterSoundCue::DredgerHoseSnap, Body);
	LooterSound::PlayAt(GetOwner(), LooterSoundCue::Element::SoulLightApply, Body, 0.8f);
	if (FWeaponFX* Effects = DrinkEffects(GetWorld()))
	{
		Effects->SpawnStarburst(Body + FVector(0.0, 0.0, 20.0), 90.f, SavedGold, 4.f, 0.5f, 8);
		Effects->SpawnFlash(Body, FVector::UpVector, 0.5f);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the smog line broken off a body on the boards."), *GetNameSafe(GetOwner()));
}

void UAbelCorruptionComponent::DrinkOut(int32 Index)
{
	AAbelKeeper* Abel = GetAbel();
	UBossComponent* Boss = GetBoss();
	UWorld* World = GetWorld();
	if (!Abel || !Boss || !World || !Drunk.IsValidIndex(Index))
	{
		return;
	}
	StopDrink(false);
	Drunk[Index] = true;
	const FVector Body = GetDeadSpot(Index);
	// He heals on what he drank (5% of his health), his black lantern flaring with it.
	if (UHealthComponent* Health = Boss->GetBossHealth())
	{
		Health->Heal(Health->GetMaxHealth() * AbelCorruptionRules::DrinkHeal);
	}
	LooterSound::PlayAt(Abel, LooterSoundCue::AbelFogFlare, Abel->GetLanternGlobe());
	// The body's shroud goes, and what's left of it rises Restless off its bier, one of his adds (gone with the fight).
	FindShrouds();
	if (AActor* Shroud = Shrouds.IsValidIndex(Index) ? Shrouds[Index].Get() : nullptr)
	{
		Shroud->SetActorHiddenInGame(true);
	}
	const FVector Middle = Boss->GetSpot();
	FVector Toward = (Middle - Body).GetSafeNormal2D();
	// His spot is his middle over the boards: the boards are a half height under it.
	const float FeetZ = static_cast<float>(Middle.Z) - Abel->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet(Body.X + Toward.X * RiseBeside, Body.Y + Toward.Y * RiseBeside, FeetZ);
	ACreatureBase::FRuntimeSpawn Spawn;
	Spawn.Rank = ECreatureRank::Rare;
	Spawn.bMayBeTouched = false;
	if (ACreatureBase* Risen = ACreatureBase::SpawnAtRuntime(World, AUnpaidCreature::StaticClass(), Feet, static_cast<float>(Toward.Rotation().Yaw), Spawn))
	{
		// In a level that isn't playing yet (a test level), it starts as play would start it (its rank and health are taken
		// at BeginPlay), as a boss's own adds do (UBossComponent's waves).
		if (!Risen->HasActorBegunPlay())
		{
			Risen->DispatchBeginPlay();
		}
		Boss->AdoptAdd(Risen);
		if (AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(Risen))
		{
			Unpaid->RiseIn();
		}
		AbelRules::LetGhostLightReach(*Risen);
	}
	LooterSound::PlayAt(Abel, LooterSoundCue::UnpaidRise, Feet);
	if (FWeaponFX* Effects = DrinkEffects(World))
	{
		Effects->SpawnSoulTatters(Body, FVector::UpVector, FLinearColor(0.05f, 0.035f, 0.07f), 60.f, 10);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the dead on bier %d drunk out: he heals, it rises Restless (%d of %d drunk; the six not all saved)."),
		*Abel->GetActorNameOrLabel(), Index, NumDrunk(), BoardsDead.Num());
}

void UAbelCorruptionComponent::RestoreDead()
{
	for (int32 Index = 0; Index < Drunk.Num(); ++Index)
	{
		if (!Drunk[Index])
		{
			continue;
		}
		Drunk[Index] = false;
		if (AStoryProp* Shroud = Shrouds.IsValidIndex(Index) ? Cast<AStoryProp>(Shrouds[Index].Get()) : nullptr)
		{
			// Back as the story has it (shown during Main 7).
			Shroud->SetActorHiddenInGame(false);
			Shroud->Refresh();
		}
	}
	LastDrunkTried = INDEX_NONE;
	bDrinkLineSaid = false;
}

#undef LOCTEXT_NAMESPACE
