// Developer console commands for enemy hit reactions you can feel (HITS, 2026-10-28; not in shipping builds):
//  - Looter.Hits.Test <zone> [damage] [crit] [kind]: the nearest living creature (of kind, a class-name word: Spider,
//    Spitter, Unpaid, Miner, Slime...) takes one hit as the player's shot from the view, on a bone of that zone (body, head,
//    abdomen, leg, arm, core, shroud, weak), with the gun in hand as the causer; the log names the bone and what it did.
//  - Looter.Hits.Show [1|0]: text over every creature near the player: its last hit's zone, weight and side, its poise
//    against its break, its wound stage and its stagger pose.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitPose.h"
#include "Combat/HitReaction.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureHitZones.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Containers/Ticker.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Looter.Hits.Show draws over creatures this near the player (cm): a fight's worth, not the whole level. */
	constexpr float ShowReach = 6000.f;

	/** The running Looter.Hits.Show (none: off). */
	FTSTicker::FDelegateHandle ShowTicker;

	/** The game world the command is for: the one it was typed in, or the running play session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	bool ParseZone(const FString& Word, ECreatureHitZone& OutZone)
	{
		for (const ECreatureHitZone Zone : { ECreatureHitZone::Body, ECreatureHitZone::Head, ECreatureHitZone::Abdomen, ECreatureHitZone::Leg,
			ECreatureHitZone::Arm, ECreatureHitZone::Core, ECreatureHitZone::Shroud, ECreatureHitZone::Weak })
		{
			if (Word.Equals(HitFeel::ZoneName(Zone), ESearchCase::IgnoreCase))
			{
				OutZone = Zone;
				return true;
			}
		}
		return false;
	}

	/** Where to put a shot on Zone of Creature: a bone with a hit body (or a hit zone part of its own), and its point. */
	bool FindTarget(const ACreatureBase& Creature, ECreatureHitZone Zone, bool bCritical, UPrimitiveComponent*& OutComponent, FName& OutBone,
		FVector& OutPoint)
	{
		USkeletalMeshComponent* Mesh = Creature.GetMesh();
		const USkeletalMesh* Model = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
		if (Model)
		{
			// A bone the shot could really have met (a hull on it) first; any bone of the zone else.
			FName Fallback;
			const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
			for (int32 Index = 0; Index < Skeleton.GetNum(); ++Index)
			{
				const FName Bone = Skeleton.GetBoneName(Index);
				if (CreatureHitZones::ZoneOf(Creature, Bone, bCritical) != Zone)
				{
					continue;
				}
				if (Mesh->GetBodyInstance(Bone))
				{
					OutComponent = Mesh;
					OutBone = Bone;
					OutPoint = Mesh->GetBoneLocation(Bone);
					return true;
				}
				Fallback = Fallback.IsNone() ? Bone : Fallback;
			}
			if (!Fallback.IsNone())
			{
				OutComponent = Mesh;
				OutBone = Fallback;
				OutPoint = Mesh->GetBoneLocation(Fallback);
				return true;
			}
		}
		// A hit zone of its own (a lamp, a bell).
		TArray<UPrimitiveComponent*> Parts;
		Creature.GetComponents(Parts);
		for (UPrimitiveComponent* Part : Parts)
		{
			if (Part != Mesh && CreatureHitZones::ZoneOfHit(Creature, Part, NAME_None, bCritical) == Zone && Part->IsCollisionEnabled())
			{
				OutComponent = Part;
				OutBone = NAME_None;
				OutPoint = Part->GetComponentLocation();
				return true;
			}
		}
		// A body hit with no model: its middle.
		if (Zone == ECreatureHitZone::Body)
		{
			OutComponent = Mesh;
			OutBone = NAME_None;
			OutPoint = Creature.GetActorLocation();
			return true;
		}
		return false;
	}

	void HitsTest(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		ECreatureHitZone Zone = ECreatureHitZone::Body;
		if (!Pawn || Args.Num() < 1 || !ParseZone(Args[0], Zone))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Hits.Test <body|head|abdomen|leg|arm|core|shroud|weak> [damage] [crit] [kind]: start the game, name a zone."));
			return;
		}
		float Damage = 20.f;
		bool bCritical = Zone == ECreatureHitZone::Core;
		FString Kind;
		for (int32 Index = 1; Index < Args.Num(); ++Index)
		{
			if (Args[Index].IsNumeric())
			{
				Damage = FMath::Max(FCString::Atof(*Args[Index]), 1.f);
			}
			else if (Args[Index].Equals(TEXT("crit"), ESearchCase::IgnoreCase))
			{
				bCritical = true;
			}
			else
			{
				Kind = Args[Index];
			}
		}
		ACreatureBase* Nearest = nullptr;
		double NearestSquared = TNumericLimits<double>::Max();
		for (TActorIterator<ACreatureBase> It(GameWorld); It; ++It)
		{
			const double Squared = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
			const bool bKind = Kind.IsEmpty() || It->GetClass()->GetName().Contains(Kind);
			if (!It->IsDead() && bKind && Squared < NearestSquared)
			{
				Nearest = *It;
				NearestSquared = Squared;
			}
		}
		UPrimitiveComponent* Part = nullptr;
		FName Bone;
		FVector Point;
		if (!Nearest || !FindTarget(*Nearest, Zone, bCritical, Part, Bone, Point))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Hits.Test: no living creature%s with a %s zone."), Kind.IsEmpty() ? TEXT("") : *(TEXT(" of kind ") + Kind),
				HitFeel::ZoneName(Zone));
			return;
		}
		// From the view, as the player's own shot, the gun in hand as its causer (so the hit knows its gun).
		FVector ViewPoint;
		FRotator ViewTurn;
		Controller->GetPlayerViewPoint(ViewPoint, ViewTurn);
		const FVector Way = (Point - ViewPoint).GetSafeNormal();
		const UWeaponManagerComponent* Weapons = Pawn->FindComponentByClass<UWeaponManagerComponent>();
		AActor* Causer = Weapons && Weapons->GetActiveWeapon() ? static_cast<AActor*>(Weapons->GetActiveWeapon()) : static_cast<AActor*>(Pawn);
		FHitResult Hit(Nearest, Part, Point, -Way);
		Hit.BoneName = Bone;
		Hit.TraceStart = ViewPoint;
		Hit.TraceEnd = Point + Way * 100.f;
		Hit.Distance = static_cast<float>(FVector::Dist(ViewPoint, Point));
		UGameplayStatics::ApplyPointDamage(Nearest, Damage, Way, Hit, Controller, Causer,
			bCritical ? UWeaponCritDamageType::StaticClass() : UWeaponDamageType::StaticClass());
		const UCreatureHitReactionComponent* Reactions = Nearest->FindComponentByClass<UCreatureHitReactionComponent>();
		const FCreatureHit* Taken = Reactions ? &Reactions->GetLastTakenHit() : nullptr;
		UE_LOG(LogLooter, Log, TEXT("Looter.Hits.Test: %s took %.0f%s on bone %s: %s hit on the %s from the %s, %.0f cm off its axis%s."),
			*Nearest->GetName(), Damage, bCritical ? TEXT(" crit") : TEXT(""), *Bone.ToString(), Taken ? HitFeel::WeightName(Taken->Weight) : TEXT("?"),
			Taken ? HitFeel::ZoneName(Taken->Zone) : TEXT("?"), Taken ? HitFeel::SideName(Taken->Side) : TEXT("?"), Taken ? Taken->LateralOffset : 0.f,
			Nearest->GetStaggerAlpha() > 0.f ? TEXT(", staggered") : TEXT(""));
	}

	/** One frame of Looter.Hits.Show: a line over each creature near the player in every game world. */
	bool ShowTick(float DeltaTime)
	{
#if ENABLE_DRAW_DEBUG
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			const APlayerController* Controller = World && World->IsGameWorld() ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			if (!Pawn)
			{
				continue;
			}
			const double Now = World->GetTimeSeconds();
			for (TActorIterator<ACreatureBase> It(World); It; ++It)
			{
				const ACreatureBase& Creature = **It;
				const UCreatureHitReactionComponent* Reactions = Creature.FindComponentByClass<UCreatureHitReactionComponent>();
				const UHealthComponent* Life = Creature.FindComponentByClass<UHealthComponent>();
				if (Creature.IsDead() || !Reactions || !Life
					|| FVector::DistSquared(Creature.GetActorLocation(), Pawn->GetActorLocation()) > FMath::Square(ShowReach))
				{
					continue;
				}
				const FCreatureHit& Last = Reactions->GetLastTakenHit();
				const FHitReaction& Rules = Reactions->GetReaction();
				const float Break = Rules.GetFeel().PoiseBreak * FHitReaction::PoiseScaleFor(Creature.GetRank());
				const FString Text = FString::Printf(TEXT("%s %s %s | poise %.2f/%.2f | wound %d | stagger %.2f"), HitFeel::WeightName(Last.Weight),
					HitFeel::ZoneName(Last.Zone), HitFeel::SideName(Last.Side), Rules.GetPoise(Now), Break, WoundStage(Life->GetHealthPercent()),
					Creature.GetStaggerAlpha());
				const float Over = Creature.GetCapsuleComponent() ? Creature.GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 40.f : 120.f;
				DrawDebugString(World, Creature.GetActorLocation() + FVector(0.f, 0.f, Over), Text, nullptr, FColor::White, 0.f, true);
			}
		}
#endif
		return true;
	}

	void HitsShow(const TArray<FString>& Args)
	{
		const bool bOn = Args.Num() == 0 ? !ShowTicker.IsValid() : FCString::Atoi(*Args[0]) != 0;
		if (ShowTicker.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(ShowTicker);
			ShowTicker.Reset();
		}
		if (bOn)
		{
			ShowTicker = FTSTicker::GetCoreTicker().AddTicker(TEXT("Looter.Hits.Show"), 0.f, &ShowTick);
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.Hits.Show %s."), bOn ? TEXT("on") : TEXT("off"));
	}

	FAutoConsoleCommandWithWorldAndArgs HitsTestCommand(
		TEXT("Looter.Hits.Test"),
		TEXT("Looter.Hits.Test <body|head|abdomen|leg|arm|core|shroud|weak> [damage, 20] [crit] [kind]: the nearest living creature (of ")
		TEXT("kind: a class-name word, Spider, Unpaid, Slime...) takes one hit as the player's shot from the view on a bone of that zone, ")
		TEXT("the gun in hand its causer; the log says the bone, the weight, zone and side it took."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&HitsTest));

	FAutoConsoleCommand HitsShowCommand(
		TEXT("Looter.Hits.Show"),
		TEXT("Looter.Hits.Show [1|0]: text over every creature near the player: its last hit's weight, zone and side, its poise ")
		TEXT("against its break, its wound stage and its stagger pose (no number: toggles)."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HitsShow));
}

#endif
