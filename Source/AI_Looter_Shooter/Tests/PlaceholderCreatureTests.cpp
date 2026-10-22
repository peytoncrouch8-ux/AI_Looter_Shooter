#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Bestiary/BestiaryEntry.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/PlaceholderCreature.h"
#include "Creatures/PlaceholderCreatureDefinition.h"
#include "Creatures/PlaceholderRules.h"
#include "Loot/LootDropComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Modules/ModuleManager.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float PlaceholderStep = 1.f / 30.f;

	/** A kind made for the test (not an asset): a hound-sized melee one, or a ranged rifleman. */
	UPlaceholderCreatureDefinition* MakeKind(EPlaceholderRole Role)
	{
		UPlaceholderCreatureDefinition* Kind = NewObject<UPlaceholderCreatureDefinition>(GetTransientPackage());
		Kind->DisplayName = FText::FromString(Role == EPlaceholderRole::Ranged ? TEXT("Test Lookout") : TEXT("Test Hound"));
		Kind->PlainType = FText::FromString(Role == EPlaceholderRole::Ranged ? TEXT("bandit") : TEXT("wolf"));
		Kind->Role = Role;
		if (Role == EPlaceholderRole::Ranged)
		{
			Kind->Length = 60.f;
			Kind->Width = 55.f;
			Kind->Height = 175.f;
			Kind->HeadSize = 32.f;
			Kind->HeadHeight = 0.92f;
		}
		else
		{
			Kind->Length = 120.f;
			Kind->Width = 50.f;
			Kind->Height = 75.f;
			Kind->HeadSize = 34.f;
			Kind->HeadHeight = 0.75f;
			Kind->HeadForward = 0.9f;
		}
		Kind->MaxHealth = 140.f;
		Kind->AttackDamage = 9.f;
		Kind->XPReward = 12;
		return Kind;
	}

	/** A placeholder of Kind spawned in play at Feet and begun, its kind written in as the editor script writes a BP's. */
	APlaceholderCreature* SpawnPlaceholder(UWorld* World, UPlaceholderCreatureDefinition* Kind, const FVector& Feet)
	{
		const FTransform At(FRotator::ZeroRotator, Feet + FVector(0.0, 0.0, 120.0));
		APlaceholderCreature* Creature = World->SpawnActorDeferred<APlaceholderCreature>(APlaceholderCreature::StaticClass(), At, nullptr,
			nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Creature)
		{
			return nullptr;
		}
		Creature->Definition = Kind;
		APlaceholderCreature::BakeDefinition(Creature);
		Creature->bRespawns = false;
		Creature->FinishSpawning(At);
		if (!Creature->HasActorBegunPlay())
		{
			Creature->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Creature);
		return Creature;
	}

	void SpawnPlaceholderFloor(UWorld* World)
	{
		LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlaceholderRulesTest, "Looter.Creatures.Placeholder.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlaceholderRulesTest::RunTest(const FString& Parameters)
{
	// Its tag: the name, then the plain type in brackets (the user's call); either alone when the other is missing.
	TestEqual(TEXT("Name (type)"), PlaceholderRules::TagName(FText::FromString(TEXT("Knocker")), FText::FromString(TEXT("goblin"))).ToString(),
		FString(TEXT("Knocker (goblin)")));
	TestEqual(TEXT("No type: the name"), PlaceholderRules::TagName(FText::FromString(TEXT("Knocker")), FText::GetEmpty()).ToString(),
		FString(TEXT("Knocker")));

	// The dome stretched to a kind's size; its capsule under its narrower side, half its height.
	const FVector Stretch = PlaceholderRules::BodyStretch(PlaceholderRules::BlobFoot * 2.f, PlaceholderRules::BlobFoot, PlaceholderRules::BlobHeight * 3.f);
	TestTrue(TEXT("Stretched per axis"), Stretch.Equals(FVector(2.f, 1.f, 3.f), 0.001));
	float Radius = 0.f;
	float Half = 0.f;
	PlaceholderRules::CapsuleFor(60.f, 55.f, 175.f, Radius, Half);
	TestTrue(TEXT("A man's capsule: narrow and tall"), Radius < 30.f && FMath::IsNearlyEqual(Half, 87.5f));
	PlaceholderRules::CapsuleFor(70.f, 35.f, 30.f, Radius, Half);
	TestTrue(TEXT("A rat's: never thinner than 18 cm, never shorter than wide"), Radius >= 18.f && Half >= Radius);
	TestTrue(TEXT("The head at its share of the height, forward of the middle"),
		PlaceholderRules::HeadSpot(100.f, 80.f, 0.75f, 0.9f).Equals(FVector(45.f, 0.f, 60.f), 0.01));
	for (const float Z : { 0.3f, 0.72f, 1.f, 1.22f })
	{
		const FVector Scale = PlaceholderRules::SquashScale(Z);
		TestEqual(FString::Printf(TEXT("Squash %.2f keeps the volume"), Z), static_cast<float>(Scale.X * Scale.Y * Scale.Z), 1.f, 0.001f);
	}

	// Its pose by role: the tell reads (a blow squashes down, a shot rears up, a flyer drops), the strike lunges, death
	// slumps and then sinks, an entrance starts under the ground.
	FPlaceholderPoseInput In;
	In.Length = 100.f;
	In.Height = 80.f;
	In.bAttacking = true;
	In.Windup = 0.6f;
	In.Recovery = 0.5f;
	In.AttackTime = 0.59f;
	TestTrue(TEXT("A blow's tell squashes it down"), PlaceholderRules::PoseAt(In).Squash < 0.8f);
	In.Role = EPlaceholderRole::Big;
	TestTrue(TEXT("...a big kind's lower still"), PlaceholderRules::PoseAt(In).Squash < 0.68f);
	In.Role = EPlaceholderRole::Ranged;
	In.bShot = true;
	TestTrue(TEXT("A shot's tell rears it up"), PlaceholderRules::PoseAt(In).Squash > 1.1f);
	In.bShot = false;
	In.Role = EPlaceholderRole::Melee;
	In.AttackTime = 0.62f;
	const FPlaceholderPose Strike = PlaceholderRules::PoseAt(In);
	TestTrue(TEXT("The strike stretches and lunges"), Strike.Squash > 1.1f && Strike.Lunge > 20.f);
	In.AttackTime = 1.2f;
	TestTrue(TEXT("...and is back by its recovery's end"), FMath::Abs(PlaceholderRules::PoseAt(In).Lunge) < 0.5f);

	FPlaceholderPoseInput Flyer;
	Flyer.Role = EPlaceholderRole::Flyer;
	Flyer.Hover = 150.f;
	TestTrue(TEXT("A flyer rides its hover"), PlaceholderRules::PoseAt(Flyer).Lift > 130.f);
	Flyer.bAttacking = true;
	Flyer.AttackTime = Flyer.Windup * 0.99f;
	TestTrue(TEXT("...and dives to the ground by its strike"), PlaceholderRules::PoseAt(Flyer).Lift < 20.f);

	FPlaceholderPoseInput Dead;
	Dead.bDead = true;
	Dead.Height = 80.f;
	Dead.CorpseTime = 6.f;
	Dead.DeadTime = 1.f;
	TestTrue(TEXT("Dead it lies flat"), PlaceholderRules::PoseAt(Dead).Squash < 0.35f && PlaceholderRules::PoseAt(Dead).Lift > -1.f);
	Dead.DeadTime = 9.f;
	TestTrue(TEXT("...then sinks into the ground"), PlaceholderRules::PoseAt(Dead).Lift < -40.f);
	FPlaceholderPoseInput Entering;
	Entering.Height = 80.f;
	Entering.EntranceLeft = 1.f;
	TestTrue(TEXT("Rising in, it starts under the ground"), PlaceholderRules::PoseAt(Entering).Lift < -70.f);

	// A shot leads a moving target, flat; the keep: none outside its hold, back off too close, its spot, else hold.
	const FVector Aim = PlaceholderRules::LeadAim(FVector::ZeroVector, FVector(2000.0, 0.0, 0.0), FVector(0.0, 400.0, 300.0), 2000.f, 0.5f);
	TestTrue(TEXT("Led half its flight, along the ground"), Aim.Equals(FVector(2000.0, 200.0, 0.0), 0.5));
	TestTrue(TEXT("Outside its hold: its brain's"), PlaceholderRules::ChooseKeep(2000.f, 800.f, 1500.f, true, 500.f, 120.f) == EPlaceholderKeep::None);
	TestTrue(TEXT("Too close: backs off"), PlaceholderRules::ChooseKeep(500.f, 800.f, 1500.f, true, 500.f, 120.f) == EPlaceholderKeep::BackOff);
	TestTrue(TEXT("In its band, away from its spot: goes there"), PlaceholderRules::ChooseKeep(1000.f, 800.f, 1500.f, true, 500.f, 120.f) == EPlaceholderKeep::ToSpot);
	TestTrue(TEXT("...at it: holds"), PlaceholderRules::ChooseKeep(1000.f, 800.f, 1500.f, true, 50.f, 120.f) == EPlaceholderKeep::Hold);
	TestTrue(TEXT("A ranged kind's brain stops at CloseInTo"), FMath::IsNearlyEqual(PlaceholderRules::RangedAttackRange(1500.f) * 0.75f, 1500.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlaceholderBodyTest, "Looter.Creatures.Placeholder.Body",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlaceholderBodyTest::RunTest(const FString& Parameters)
{
	// A placeholder's body: its tag says what it stands for, its capsule and clay fit its kind's size, the dome and its head
	// take shots (the head is the critical spot), pawns walk through, and it pays like a real creature: its kind's
	// experience, a loot drop on death, and a rank's multipliers on top (the user's call, 2026-10-22).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnPlaceholderFloor(World);
	UPlaceholderCreatureDefinition* Kind = MakeKind(EPlaceholderRole::Melee);
	APlaceholderCreature* Hound = SpawnPlaceholder(World, Kind, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A placeholder hound"), Hound))
	{
		return false;
	}
	TestEqual(TEXT("Its tag: name (type)"), Hound->DisplayName.ToString(), FString(TEXT("Test Hound (wolf)")));
	TArray<FString> Differences;
	TestTrue(TEXT("Its numbers are its kind's"), Hound->MatchesDefinition(Differences));
	for (const FString& Difference : Differences)
	{
		AddError(Difference);
	}
	float Radius = 0.f;
	float Half = 0.f;
	PlaceholderRules::CapsuleFor(Kind->Length, Kind->Width, Kind->Height, Radius, Half);
	TestEqual(TEXT("Its capsule's height for its kind"), Hound->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), Half, 0.5f);
	// One frame of its body as the game poses it (a test level never ticks its animation): the stretch rides on the body bone.
	Hound->Tick(PlaceholderStep);
	Hound->GetMesh()->TickAnimation(PlaceholderStep, false);
	Hound->GetMesh()->RefreshBoneTransforms(nullptr);
	TestTrue(TEXT("The mesh itself unscaled (the bone carries the stretch)"), Hound->GetMesh()->GetRelativeScale3D().Equals(FVector::OneVector, 0.001));
	const float Tall = static_cast<float>(Hound->GetMesh()->Bounds.BoxExtent.Z * 2.0);
	TestTrue(FString::Printf(TEXT("The clay stretched to its height (%.0f cm, its kind %.0f)"), Tall, Kind->Height), FMath::Abs(Tall - Kind->Height) < Kind->Height * 0.35f);
	TestNotNull(TEXT("A clay look in its kind's colour"), Hound->GetClayLook());

	USkeletalMeshComponent* Body = Hound->GetMesh();
	UStaticMeshComponent* Head = Hound->GetHead();
	for (const UPrimitiveComponent* Zone : { static_cast<const UPrimitiveComponent*>(Body), static_cast<const UPrimitiveComponent*>(Head) })
	{
		TestTrue(FString::Printf(TEXT("%s blocks shots"), *GetNameSafe(Zone)), Zone->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
		TestTrue(FString::Printf(TEXT("%s lets pawns through"), *GetNameSafe(Zone)), Zone->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	}
	FHitResult OnHead;
	OnHead.Component = Head;
	FHitResult OnBody;
	OnBody.Component = Body;
	TestTrue(TEXT("The head is the critical spot"), Hound->IsCriticalSpot(OnHead));
	TestFalse(TEXT("...the body isn't"), Hound->IsCriticalSpot(OnBody));
	// Its head stands where its kind says, round however the body is stretched.
	const FVector HeadAt = Hound->GetActorTransform().InverseTransformPosition(Head->GetComponentLocation())
		+ FVector(0.0, 0.0, Half);
	const FVector Wanted = PlaceholderRules::HeadSpot(Kind->Length, Kind->Height, Kind->HeadHeight, Kind->HeadForward);
	TestTrue(FString::Printf(TEXT("Its head at (%.0f, %.0f, %.0f), wanted (%.0f, %.0f, %.0f)"), HeadAt.X, HeadAt.Y, HeadAt.Z, Wanted.X, Wanted.Y, Wanted.Z),
		FVector::Dist(HeadAt, Wanted) < 15.0);
	const FVector HeadScale = Head->GetComponentScale();
	TestTrue(TEXT("...and round"), FMath::IsNearlyEqual(HeadScale.X, HeadScale.Z, 0.05 * HeadScale.Z)
		&& FMath::IsNearlyEqual(HeadScale.Y, HeadScale.Z, 0.05 * HeadScale.Z));

	// It pays as any creature: its loot drops on death (the default table, or its rank's), its kind's experience, and a
	// Restless one is tougher and worth more.
	const ULootDropComponent* Loot = Hound->FindComponentByClass<ULootDropComponent>();
	TestNotNull(TEXT("It carries a loot drop"), Loot);
	TestEqual(TEXT("Its kind's experience"), Hound->XPReward, Kind->XPReward);
	TestTrue(TEXT("...which a kill pays"), UPlayerProgressionSubsystem::GetLevelRules().KillXP(Hound->XPReward, 1, 1) > 0);
	const float BasicHealth = Hound->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
	const int32 BasicXP = Hound->XPReward;
	Hound->SetRank(ECreatureRank::Rare);
	TestTrue(TEXT("Restless: tougher"), Hound->FindComponentByClass<UHealthComponent>()->GetMaxHealth() > BasicHealth * 1.5f);
	TestTrue(TEXT("...and worth more"), Hound->XPReward > BasicXP);
	TestTrue(TEXT("A melee kind waits on the ring"), Hound->GetFightRole() == EFightRole::Melee);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlaceholderShotsTest, "Looter.Creatures.Placeholder.Shots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlaceholderShotsTest::RunTest(const FString& Parameters)
{
	// A ranged placeholder holds a band, takes ranged turns and shoots pellets after its tell; a player up close gets bitten.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	SpawnPlaceholderFloor(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1400.0, 0.0, 89.0));
	UPlaceholderCreatureDefinition* Kind = MakeKind(EPlaceholderRole::Ranged);
	APlaceholderCreature* Lookout = SpawnPlaceholder(World, Kind, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("A placeholder lookout"), Lookout))
	{
		return false;
	}
	float Near = 0.f;
	float Far = 0.f;
	TestTrue(TEXT("A ranged kind: its band"), Lookout->GetFightRole() == EFightRole::Ranged && Lookout->GetFightBand(Near, Far) && Near < Far);
	TestEqual(TEXT("...its brain stops at CloseInTo"), Lookout->GetAttackRange() * 0.75f, Kind->CloseInTo, 1.f);

	Lookout->AlertTo(Player);
	float AttackAt = -1.f;
	float ShotAt = -1.f;
	for (float Time = 0.f; Time < 6.f && ShotAt < 0.f; Time += PlaceholderStep)
	{
		Lookout->Tick(PlaceholderStep);
		if (AttackAt < 0.f && Lookout->GetCreatureState() == ECreatureState::Attack)
		{
			AttackAt = Time;
		}
		if (Lookout->GetShotsFired() > 0)
		{
			ShotAt = Time;
		}
	}
	TestTrue(TEXT("It winds up a shot"), AttackAt >= 0.f);
	TestTrue(FString::Printf(TEXT("...and fires after its tell (%.2f s into it)"), ShotAt - AttackAt),
		ShotAt >= 0.f && ShotAt - AttackAt >= Kind->AttackWindup - 2.f * PlaceholderStep);
	TestTrue(TEXT("...its turn a ranged one"), Lookout->GetAttackTokenRole() == EFightRole::Ranged);

	// Up close it bites (a melee turn) rather than shoots.
	Player->SetActorLocation(FVector(150.0, 0.0, 89.0));
	TestTrue(TEXT("A player at its feet: a bite, a melee turn"), Lookout->GetAttackTokenRole() == EFightRole::Melee);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlaceholderPagesTest, "Looter.Creatures.Placeholder.Pages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlaceholderPagesTest::RunTest(const FString& Parameters)
{
	// Every placeholder kind made (DA_Placeholder_<Name>) has its BP, whose defaults hold its numbers (the script's
	// BakeDefinition), and a bestiary page about that BP named as its tag is ("Name (type)") and marked a placeholder.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> Kinds;
	Registry.GetAssetsByClass(UPlaceholderCreatureDefinition::StaticClass()->GetClassPathName(), Kinds, true);
	if (Kinds.IsEmpty())
	{
		AddInfo(TEXT("No placeholder kinds made yet (Tools/Unreal/create_placeholder_creatures.py)."));
		return true;
	}
	const TArray<UBestiaryEntry*> Pages = UBestiaryEntry::LoadAll();
	for (const FAssetData& Asset : Kinds)
	{
		const UPlaceholderCreatureDefinition* Kind = Cast<UPlaceholderCreatureDefinition>(Asset.GetAsset());
		if (!Kind)
		{
			continue;
		}
		const FString Name = Kind->GetName();
		TArray<FString> Problems;
		TestTrue(FString::Printf(TEXT("%s: its numbers make sense"), *Name), Kind->IsSound(Problems));
		for (const FString& Problem : Problems)
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Name, *Problem));
		}
		const UBestiaryEntry* const* Page = Pages.FindByPredicate([Kind](const UBestiaryEntry* Entry)
		{
			const UClass* Class = Entry->ActorClass.LoadSynchronous();
			const APlaceholderCreature* Defaults = Class && Class->IsChildOf(APlaceholderCreature::StaticClass())
				? Class->GetDefaultObject<APlaceholderCreature>() : nullptr;
			return Defaults && Defaults->Definition == Kind;
		});
		if (!TestNotNull(FString::Printf(TEXT("%s: a bestiary page about its BP"), *Name), Page))
		{
			continue;
		}
		const APlaceholderCreature* Defaults = (*Page)->ActorClass.Get()->GetDefaultObject<APlaceholderCreature>();
		TArray<FString> Differences;
		TestTrue(FString::Printf(TEXT("%s: its BP's defaults are its numbers (run the script again if not)"), *Name), Defaults->MatchesDefinition(Differences));
		for (const FString& Difference : Differences)
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Name, *Difference));
		}
		TestEqual(FString::Printf(TEXT("%s: its page's name is its tag's"), *Name), (*Page)->DisplayName.ToString(), Kind->GetTagName().ToString());
		TestTrue(FString::Printf(TEXT("%s: its page says it's a placeholder"), *Name),
			(*Page)->Notes.ContainsByPredicate([](const FText& Note) { return Note.ToString().Contains(TEXT("Placeholder")); }));
		TestNotNull(FString::Printf(TEXT("%s: a model on the stand"), *Name), (*Page)->LoadPreviewMesh());
	}
	return true;
}

#endif
