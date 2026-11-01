// SaintAdaWakes: the world's side of Saint Ada waking (who's in it, the end state, the Unpaid going quiet). The scene's
// timeline is SaintAdaWakes.cpp.

#include "Scenes/SaintAdaWakes.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/AbelOnBoard.h"
#include "Weapons/WeaponFX.h"
#include "World/CarriedLantern.h"
#include "World/ChapelBell.h"
#include "World/LamplighterRules.h"
#include "World/CarriedLanternRules.h"
#include "World/ChapelReliquary.h"
#include "World/KeeperLanternPost.h"
#include "World/LightingStateSubsystem.h"
#include "World/ReliquaryLantern.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** The clean dusk once the smog has burnt off (layout.json level.environment.states). */
	const FName CleanDusk(TEXT("Dusk"));
	/** The speakers' tags the scene finds people by. */
	const FName AbelTag(TEXT("Speaker_Abel"));
	const FName FinchTag(TEXT("Speaker_FinchChapel"));
	/** The Unpaid that go quiet: this far from the Reliquary (cm), the whole valley the player can see from the knoll. */
	constexpr float QuietReach = 30000.f;
	/** The six dead's spots round the deck's middle when the deck's own isn't known: a ring this wide, this high (cm). */
	constexpr float DeadRing = 420.f;
	constexpr float DeadUp = 110.f;
	const FLinearColor SoulCyan(0.55f, 0.92f, 1.f);

	template <typename T>
	T* Nearest(const UWorld* World, const FVector& To, FName Tag)
	{
		T* Best = nullptr;
		double BestDistance = TNumericLimits<double>::Max();
		for (TActorIterator<T> It(World); It; ++It)
		{
			if (!Tag.IsNone() && !It->ActorHasTag(Tag))
			{
				continue;
			}
			const double Distance = FVector::DistSquared(It->GetActorLocation(), To);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = *It;
			}
		}
		return Best;
	}
}

namespace SaintAdaWakes
{
	TArray<FStoryLine> Lines()
	{
		TArray<FStoryLine> Made = LandfallRules::LoadLines(GivenToTheWind::WakesLines());
		if (Made.Num() >= 4)
		{
			return Made;
		}
		// The design's words (MAINS_2_7.md section 11), until the line set is made.
		const FText Abel = NSLOCTEXT("LooterSaintAda", "Abel", "Abel");
		const FText Delia = NSLOCTEXT("LooterSaintAda", "Delia", "Delia Ransom");
		return {
			FStoryLine::Make(Abel, NSLOCTEXT("LooterSaintAda", "Knees", "Ma. You came up the hill. Your knees."), 3.2f),
			FStoryLine::Make(Delia, NSLOCTEXT("LooterSaintAda", "Door", "Hush. You held the door."), 2.6f),
			FStoryLine::Make(Abel, NSLOCTEXT("LooterSaintAda", "Keep", "I'll keep her now. From in here. She won't go out again."), 3.6f),
			FStoryLine::Make(Abel, NSLOCTEXT("LooterSaintAda", "Warden", "Warden. Whatever they took me for, there's more of it. Go and get them back."), 4.2f),
		};
	}

	void RingHerBell(UWorld* World)
	{
		for (TActorIterator<AChapelBell> It(World); It; ++It)
		{
			if (!It->ActorHasTag(AChapelBell::BellTag))
			{
				continue;
			}
			if (!It->Ring(nullptr))
			{
				// Her bell won't swing for its own rule (Main 4's): its toll sounds from the tower all the same.
				LooterSound::PlayAt(*It, LooterSoundCue::ChapelBellToll, It->GetActorLocation());
			}
			return;
		}
	}

	AAbelOnBoard* FindAbelSoul(const AReliquaryLantern& Rest)
	{
		return Nearest<AAbelOnBoard>(Rest.GetWorld(), Rest.GetActorLocation(), AbelTag);
	}

	AActor* FindDelia(const AReliquaryLantern& Rest)
	{
		return Nearest<AActor>(Rest.GetWorld(), Rest.GetActorLocation(), GivenToTheWind::DeliaChapelTag());
	}

	AActor* FindFinch(const AReliquaryLantern& Rest)
	{
		return Nearest<AActor>(Rest.GetWorld(), Rest.GetActorLocation(), FinchTag);
	}

	bool FindPoint(const UWorld* World, FVector& OutPoint)
	{
		const AActor* Point = World ? Nearest<AActor>(World, FVector::ZeroVector, GivenToTheWind::WaitPlaceTag()) : nullptr;
		if (!Point)
		{
			return false;
		}
		OutPoint = Point->GetActorLocation();
		return true;
	}

	TArray<FVector> DeadOnTheBoards(const UWorld* World)
	{
		// The deck's middle from its three lantern posts (the six biers with shrouds are round it), then six spots round it.
		TArray<FVector> Spots;
		FVector Middle = FVector::ZeroVector;
		int32 Posts = 0;
		if (World)
		{
			for (TActorIterator<AKeeperLanternPost> It(World); It; ++It)
			{
				if (It->ActorHasTag(AKeeperLanternPost::DeckTag))
				{
					Middle += It->GetActorLocation();
					++Posts;
				}
			}
		}
		if (Posts == 0)
		{
			FVector Point;
			if (!FindPoint(World, Point))
			{
				return Spots;
			}
			Middle = Point;
		}
		else
		{
			Middle /= static_cast<double>(Posts);
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(Index) / 6.f + 0.4f;
			Spots.Add(Middle + FVector(FMath::Cos(Angle) * DeadRing, FMath::Sin(Angle) * DeadRing, DeadUp));
		}
		return Spots;
	}

	int32 QuietUnpaid(UWorld* World, const FVector& Center, float Radius, TArray<TWeakObjectPtr<AActor>>* OutSinking)
	{
		if (!World)
		{
			return 0;
		}
		UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
		FWeaponFX* Effects = nullptr;
		if (Bullets)
		{
			Effects = &Bullets->GetEffects();
			Effects->Initialize(World);
		}
		TArray<AUnpaidCreature*> Quieted;
		for (TActorIterator<AUnpaidCreature> It(World); It; ++It)
		{
			AUnpaidCreature* Unpaid = *It;
			if (!IsValid(Unpaid) || Unpaid->IsDead() || Unpaid->IsHidden()
				|| FVector::DistSquared(Unpaid->GetActorLocation(), Center) > static_cast<double>(Radius) * Radius)
			{
				continue;
			}
			Quieted.Add(Unpaid);
		}
		for (AUnpaidCreature* Unpaid : Quieted)
		{
			if (!Unpaid->HasActorBegunPlay())
			{
				// Never risen (a placed one not begun yet, a test level's): simply gone.
				Unpaid->Destroy();
				continue;
			}
			// Saint Ada's light holds the Rest's dead in their graves again: they stop, their soul-light sighs out, and down.
			Unpaid->SetPassive(true);
			Unpaid->SetCanBeDamaged(false);
			Unpaid->SetActorEnableCollision(false);
			if (UCharacterMovementComponent* Movement = Unpaid->GetCharacterMovement())
			{
				Movement->StopMovementImmediately();
				Movement->DisableMovement();
			}
			if (Effects)
			{
				Effects->SpawnSoulTatters(Unpaid->GetActorLocation(), FVector::UpVector, SoulCyan, 50.f, 4);
			}
			if (OutSinking)
			{
				LooterSound::PlayAt(Unpaid, LooterSoundCue::UnpaidDissolve, Unpaid->GetActorLocation(), 0.7f);
				OutSinking->Add(Unpaid);
			}
			else
			{
				Unpaid->Destroy();
			}
		}
		if (!Quieted.IsEmpty())
		{
			UE_LOG(LogLooter, Log, TEXT("Saint Ada wakes: %d Unpaid go quiet and sink into their graves."), Quieted.Num());
		}
		return Quieted.Num();
	}

	void ApplyEndState(AReliquaryLantern& Rest, bool bInstant)
	{
		UWorld* World = Rest.GetWorld();
		if (UMissionRunner* Runner = UMissionRunner::Get(&Rest))
		{
			// For good: the Reliquary lit, Abel's soul there, the coal and its souls gone, the shrouds off the boards.
			Runner->SetFlag(GivenToTheWind::SaintAdaAwakeFlag());
		}
		Rest.ShowLantern(true, 1.f);
		if (AChapelReliquary* Saint = Rest.GetReliquary())
		{
			Saint->SetLitNow(true);
			Saint->SetBlaze(0.f);
		}
		if (UCarriedLanternSubsystem* Carried = UCarriedLanternSubsystem::Get(&Rest))
		{
			Carried->SetDown();
			Carried->ReleaseFollowers(DeadOnTheBoards(World));
		}
		if (AAbelOnBoard* Abel = FindAbelSoul(Rest))
		{
			Abel->ShowNow();
		}
		if (ULightingStateSubsystem* Light = ULightingStateSubsystem::Get(&Rest))
		{
			Light->SetState(CleanDusk, bInstant ? ELightingSwitch::Instant : ELightingSwitch::Blend, bInstant ? 0.f : 4.f);
		}
		// At once (no scene, or a skip): the dead near are simply gone; the played scene lowers its own into their graves.
		if (bInstant)
		{
			QuietUnpaid(World, Rest.GetActorLocation(), QuietReach, nullptr);
		}
	}

	void FinishAtOnce(AReliquaryLantern& Rest)
	{
		ApplyEndState(Rest, /*bInstant*/ true);
		if (USceneSubsystem* Scenes = USceneSubsystem::Get(&Rest))
		{
			Scenes->MarkPlayed(SceneName());
		}
		UE_LOG(LogLooter, Log, TEXT("Saint Ada wakes (no scene): the Reliquary lit, the smog gone."));
	}
}
