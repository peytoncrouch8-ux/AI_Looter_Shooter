// The cast of Looter.CastShots (CastShotScene.h): who is photographed, how each state is made, which bones the numbers
// watch, and where the camera goes. CastShotSceneSpots.cpp finds the ground they're photographed on. Developer builds only.

#include "Dev/CastShotScene.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Bosses/AbelKeeper.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Loot/LootDropComponent.h"
#include "Story/AbelOnBoard.h"
#include "Story/AmosWhitlock.h"
#include "Story/HobBird.h"
#include "Story/MisterSexton.h"
#include "Story/SpeakerPoint.h"
#include "Story/StoryCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The camera's view across (degrees): a portrait lens, little distortion. */
	constexpr float FieldOfView = 40.f;

	/** Puts the player on the ground Distance along Ahead from Feet (or nearby round it), standing; false where there's none. */
	bool PutPlayer(UWorld& World, APawn& Player, const FVector& Feet, const FVector& Ahead, float Distance)
	{
		for (const float Turn : { 0.f, 30.f, -30.f, 60.f, -60.f, 90.f, -90.f })
		{
			FVector Ground;
			const FVector Spot = Feet + Ahead.RotateAngleAxis(Turn, FVector::UpVector) * Distance;
			if (!CastShotScene::GroundAt(World, Spot, Ground) || FMath::Abs(Ground.Z - Feet.Z) > 0.6 * Distance
				|| UEncounterSubsystem::IsSheltered(&World, Ground))
			{
				continue;
			}
			const float HalfHeight = Player.GetRootComponent()->Bounds.BoxExtent.Z;
			Player.SetActorLocation(Ground + FVector(0.0, 0.0, HalfHeight + 5.0), false, nullptr, ETeleportType::TeleportPhysics);
			CastShotScene::StopMoving(Player);
			return true;
		}
		return false;
	}

	void HitFrom(ACreatureBase& Creature, const FVector& From, float Damage)
	{
		const FVector Middle = Creature.GetActorLocation();
		const FVector Way = (Middle - From).GetSafeNormal();
		FHitResult Hit;
		Hit.ImpactPoint = Hit.Location = Middle - Way * 25.0;
		UGameplayStatics::ApplyPointDamage(&Creature, Damage, Way, Hit, nullptr, nullptr, UDamageType::StaticClass());
	}
}

const TCHAR* CastShotScene::StateName(EState State)
{
	switch (State)
	{
	case EState::Idle: return TEXT("idle");
	case EState::Walk: return TEXT("walk");
	case EState::Chase: return TEXT("chase");
	case EState::Windup: return TEXT("windup");
	case EState::Hurt: return TEXT("hurt");
	case EState::Death: return TEXT("death");
	case EState::InPlace: return TEXT("asplaced");
	case EState::Lean: return TEXT("lean");
	case EState::Sit: return TEXT("sit");
	case EState::Pack: return TEXT("closing");
	default: return TEXT("state");
	}
}

const TCHAR* CastShotScene::PlaceName(EPlace Place)
{
	return Place == EPlace::Flat ? TEXT("flat") : (Place == EPlace::Slope ? TEXT("slope") : TEXT("level"));
}

const TCHAR* CastShotScene::AngleName(int32 Angle)
{
	static const TCHAR* Names[AngleCount] = { TEXT("front"), TEXT("side"), TEXT("back") };
	return Names[FMath::Clamp(Angle, 0, AngleCount - 1)];
}

TArray<CastShotScene::FSubject> CastShotScene::Subjects(UWorld& World, TConstArrayView<FString> Only)
{
	const TArray<EPlace> BothPlaces = { EPlace::Flat, EPlace::Slope };
	const TArray<EState> CreatureStates = { EState::Idle, EState::Walk, EState::Chase, EState::Windup, EState::Hurt, EState::Death };
	TArray<FSubject> All;
	auto Creature = [&](const TCHAR* Name, UClass* Class, ECreatureRank Rank, bool bSpiderling, int32 Pack)
	{
		FSubject& Subject = All.AddDefaulted_GetRef();
		Subject.Name = Name;
		Subject.Class = Class;
		Subject.Rank = Rank;
		Subject.bSpiderling = bSpiderling;
		Subject.PackCount = Pack;
		Subject.Places = Pack > 0 ? TArray<EPlace>{ EPlace::Flat } : BothPlaces;
		Subject.States = Pack > 0 ? TArray<EState>{ EState::Pack } : CreatureStates;
	};
	Creature(TEXT("spider"), ASpiderCreature::StaticClass(), ECreatureRank::Basic, false, 0);
	Creature(TEXT("spider-restless"), ASpiderCreature::StaticClass(), ECreatureRank::Rare, false, 0);
	Creature(TEXT("spider-gravebound"), ASpiderCreature::StaticClass(), ECreatureRank::Epic, false, 0);
	Creature(TEXT("spiderling"), ASpiderCreature::StaticClass(), ECreatureRank::Basic, true, 0);
	Creature(TEXT("gravemother"), AGravemotherCreature::StaticClass(), ECreatureRank::Legendary, false, 0);
	Creature(TEXT("slime"), ASlimeCreature::StaticClass(), ECreatureRank::Basic, false, 0);
	Creature(TEXT("slime-restless"), ASlimeCreature::StaticClass(), ECreatureRank::Rare, false, 0);
	Creature(TEXT("unpaid"), AUnpaidCreature::StaticClass(), ECreatureRank::Basic, false, 0);
	Creature(TEXT("unpaid-restless"), AUnpaidCreature::StaticClass(), ECreatureRank::Rare, false, 0);
	Creature(TEXT("unpaid-gravebound"), AUnpaidCreature::StaticClass(), ECreatureRank::Epic, false, 0);
	Creature(TEXT("spider-pack"), ASpiderCreature::StaticClass(), ECreatureRank::Basic, false, 5);
	Creature(TEXT("unpaid-pack"), AUnpaidCreature::StaticClass(), ECreatureRank::Basic, false, 6);
	Creature(TEXT("slime-pack"), ASlimeCreature::StaticClass(), ECreatureRank::Basic, false, 4);

	FSubject& Player = All.AddDefaulted_GetRef();
	Player.Name = TEXT("player");
	Player.bPlayer = true;
	Player.Places = BothPlaces;
	Player.States = { EState::Idle, EState::Walk };

	// The story's characters, and Abel, where the level has them.
	TMap<FString, int32> Seen;
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		AActor* Actor = *It;
		FString Name;
		TArray<EState> States = { EState::InPlace };
		if (Actor->IsA<AMisterSexton>())
		{
			Name = TEXT("sexton");
		}
		else if (Actor->IsA<AAmosWhitlock>())
		{
			Name = TEXT("amos");
			States = { EState::Lean, EState::Sit };
		}
		else if (Actor->IsA<AHobBird>())
		{
			Name = TEXT("hob");
		}
		else if (Actor->IsA<AAbelOnBoard>())
		{
			Name = TEXT("abel-board");
		}
		else if (Actor->IsA<AStoryCharacter>())
		{
			Name = TEXT("story");
		}
		else if (Actor->IsA<AAbelKeeper>())
		{
			Name = TEXT("abel");
		}
		else if (Actor->IsA<ASpeakerPoint>() && (Actor->ActorHasTag(TEXT("Speaker_Delia")) || Actor->ActorHasTag(TEXT("Speaker_Tilly"))))
		{
			Name = Actor->ActorHasTag(TEXT("Speaker_Delia")) ? TEXT("delia-door") : TEXT("tilly-window");
		}
		if (Name.IsEmpty())
		{
			continue;
		}
		const int32 Count = Seen.FindOrAdd(Name)++;
		FSubject& Character = All.AddDefaulted_GetRef();
		Character.Name = Count > 0 ? FString::Printf(TEXT("%s%d"), *Name, Count + 1) : Name;
		Character.Found = Actor;
		Character.Places = { EPlace::InPlace };
		Character.States = States;
	}

	if (!Only.IsEmpty())
	{
		All.RemoveAll([Only](const FSubject& Subject)
		{
			return !Only.ContainsByPredicate([&Subject](const FString& Prefix) { return Subject.Name.StartsWith(Prefix, ESearchCase::IgnoreCase); });
		});
	}
	return All;
}

ACreatureBase* CastShotScene::Spawn(UWorld& World, const FSubject& Subject, const FVector& Feet, float Yaw)
{
	if (!Subject.Class || !Subject.Class->IsChildOf(ACreatureBase::StaticClass()))
	{
		return nullptr;
	}
	ACreatureBase::FRuntimeSpawn Spawning = Subject.bSpiderling ? AGravemotherCreature::MakeSpiderlingSpawn() : ACreatureBase::FRuntimeSpawn();
	Spawning.Rank = Subject.Rank;
	ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(&World, Subject.Class, Feet, Yaw, Spawning);
	if (!Creature)
	{
		return nullptr;
	}
	if (Subject.bSpiderling)
	{
		Creature->DisplayName = AGravemotherCreature::SpiderlingName();
	}
	if (ULootDropComponent* Loot = Creature->FindComponentByClass<ULootDropComponent>())
	{
		Loot->bDropOnDeath = false;
	}
	Creature->SetPassive(true);
	Creature->DevPutInState(ECreatureState::Idle);
	return Creature;
}

float CastShotScene::Start(UWorld& World, ACreatureBase& Creature, EState State, APawn& Player, const FVector& Ahead)
{
	const FVector Middle = Creature.GetActorLocation();
	const FVector Feet = Middle - FVector(0.0, 0.0, Creature.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	const bool bUnpaid = Creature.IsA<AUnpaidCreature>();
	switch (State)
	{
	case EState::Idle:
		return 1.f;
	case EState::Walk:
	{
		FVector Goal = Feet + Ahead * 700.0;
		GroundAt(World, Goal, Goal);
		Creature.DevPutInState(ECreatureState::Wander, nullptr, Goal);
		return 1.4f;
	}
	case EState::Chase:
		if (!PutPlayer(World, Player, Feet, Ahead, 1200.f))
		{
			return -1.f;
		}
		Creature.SetPassive(false);
		Creature.DevPutInState(ECreatureState::Chase, &Player);
		return 1.4f;
	case EState::Windup:
		// Its prey in reach ahead: the attack's tell, most of the way through.
		if (!PutPlayer(World, Player, Feet, Creature.GetActorForwardVector(), FMath::Max(Creature.GetAttackRange() * 0.7f, 120.f)))
		{
			return -1.f;
		}
		Creature.SetPassive(false);
		Creature.DevPutInState(ECreatureState::Attack, &Player);
		return 0.7f * Creature.AttackWindup;
	case EState::Hurt:
		// A shot from ahead and to its left, as a gun's bullet lands (no one to blame: it doesn't turn on anyone).
		HitFrom(Creature, Middle + Creature.GetActorForwardVector().RotateAngleAxis(-35.f, FVector::UpVector) * 600.0 + FVector(0.0, 0.0, 40.0), 1.f);
		return 0.06f;
	case EState::Death:
		HitFrom(Creature, Middle + Creature.GetActorForwardVector() * 600.0, 1.0e7f);
		// The Unpaid starts to dissolve 0.35 s after it dies.
		return bUnpaid ? 0.3f : 0.45f;
	default:
		return -1.f;
	}
}

CastShotScene::FBodyParts CastShotScene::PartsOf(const AActor& Subject)
{
	FBodyParts Parts;
	if (Subject.IsA<ASpiderCreature>())
	{
		for (int32 Pair = 0; Pair < 4; ++Pair)
		{
			for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
			{
				Parts.Feet.Add(*FString::Printf(TEXT("foot_%d_%s"), Pair, Side));
				Parts.Limbs.Add(*FString::Printf(TEXT("femur_%d_%s"), Pair, Side));
				Parts.Limbs.Add(*FString::Printf(TEXT("tibia_%d_%s"), Pair, Side));
			}
		}
		Parts.PlantedHeight = 3.f;
		Parts.Body = { TEXT("body"), TEXT("abdomen"), TEXT("head") };
	}
	else if (Subject.IsA<ASlimeCreature>())
	{
		Parts.bSlimeFoot = true;
	}
	else
	{
		// The Unpaid's rig (Abel and Amos are built like it) and the player's mannequin share their arms' names.
		for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
		{
			Parts.Limbs.Add(*FString::Printf(TEXT("upperarm_%s"), Side));
			Parts.Limbs.Add(*FString::Printf(TEXT("lowerarm_%s"), Side));
			Parts.Limbs.Add(*FString::Printf(TEXT("hand_%s"), Side));
		}
		Parts.Body = { TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05"),
			TEXT("tail_01"), TEXT("tail_02"), TEXT("thigh_l"), TEXT("thigh_r") };
		Parts.Trailing = { TEXT("tail_03"), TEXT("tail_04"), TEXT("tail_05"), TEXT("tail_l_02"), TEXT("tail_r_02") };
		if (Subject.IsA<ACharacter>() && !Subject.IsA<ACreatureBase>())
		{
			// The mannequin: the balls of its feet touch the ground, the ankles stand a little over it.
			Parts.Feet = { TEXT("ball_l"), TEXT("ball_r") };
			Parts.PlantedHeight = 6.f;
		}
	}
	return Parts;
}

UPrimitiveComponent* CastShotScene::BodyOf(const AActor& Subject)
{
	if (const ACharacter* Character = Cast<ACharacter>(&Subject))
	{
		USkinnedMeshComponent* Mesh = Character->GetMesh();
		if (Mesh && Mesh->GetSkinnedAsset())
		{
			return Mesh;
		}
	}
	TInlineComponentArray<USkinnedMeshComponent*> Skinned(&Subject);
	for (USkinnedMeshComponent* Mesh : Skinned)
	{
		if (Mesh && Mesh->GetSkinnedAsset() && Mesh->IsVisible())
		{
			return Mesh;
		}
	}
	UStaticMeshComponent* Biggest = nullptr;
	TInlineComponentArray<UStaticMeshComponent*> Statics(&Subject);
	for (UStaticMeshComponent* Mesh : Statics)
	{
		if (Mesh && Mesh->GetStaticMesh() && Mesh->IsVisible() && (!Biggest || Mesh->Bounds.SphereRadius > Biggest->Bounds.SphereRadius))
		{
			Biggest = Mesh;
		}
	}
	return Biggest;
}

FBox CastShotScene::BoundsOf(TConstArrayView<const AActor*> Subjects)
{
	FBox Box(ForceInit);
	for (const AActor* Subject : Subjects)
	{
		const UPrimitiveComponent* Body = Subject ? BodyOf(*Subject) : nullptr;
		if (Body)
		{
			Box += Body->Bounds.GetBox();
		}
		else if (Subject)
		{
			Box += Subject->GetComponentsBoundingBox(true);
		}
	}
	// Nothing to see (a door with no leaf of its own): a person's size where it stands.
	if (!Box.IsValid && !Subjects.IsEmpty() && Subjects[0])
	{
		Box = FBox::BuildAABB(Subjects[0]->GetActorLocation(), FVector(60.0, 60.0, 100.0));
	}
	return Box;
}

void CastShotScene::Frame(UWorld& World, ACameraActor& Camera, const FBox& Bounds, float FacingYaw, int32 Angle,
	TConstArrayView<const AActor*> Ignored)
{
	// Round the subject (degrees from its facing) and up from level, for each angle.
	static const float Around[AngleCount] = { 35.f, 95.f, 205.f };
	static const float Elevation[AngleCount] = { 12.f, 6.f, 32.f };
	const int32 Pick = FMath::Clamp(Angle, 0, AngleCount - 1);
	const FVector Center = Bounds.GetCenter();
	const FVector Extent = Bounds.GetExtent();
	// Far enough for its width across the view and its height up it (a 16:9 view is narrower up than across).
	const float Across = FMath::Tan(FMath::DegreesToRadians(FieldOfView * 0.5f));
	const float Up = Across * 9.f / 16.f;
	const float Distance = 1.25f * FMath::Max3(static_cast<float>(FMath::Max(Extent.X, Extent.Y)) / Across, static_cast<float>(Extent.Z) / Up, 150.f);
	const FVector Direction = FRotator(Elevation[Pick], FacingYaw + Around[Pick], 0.f).Vector();
	FVector Eye = Center + Direction * Distance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CastShotCamera), false);
	for (const AActor* Each : Ignored)
	{
		Params.AddIgnoredActor(Each);
	}
	FHitResult Hit;
	if (World.LineTraceSingleByChannel(Hit, Center, Eye, ECC_Visibility, Params))
	{
		// A wall or the hill in the way: in front of it, but never inside the subject.
		Eye = Center + Direction * FMath::Max(static_cast<float>(Hit.Distance) - 30.f, static_cast<float>(Extent.Size()) * 1.1f);
	}
	Camera.SetActorLocationAndRotation(Eye, (Center - Eye).Rotation());
	Camera.GetCameraComponent()->SetFieldOfView(FieldOfView);
}

#endif
