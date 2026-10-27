#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's hide shrugging hits off (the user's calls, 2026-10-27: "you need to break the tombstone on her back
// before you can damage her"; "I can just sit above the sink and shoot her without her aggro'ing to me": rim shots do
// nothing), and the playtest's tuning (1.6x health, 1.25x hits, staggers as easy to earn as before). AGravemotherCreature's
// GravemotherCreatureShrug.cpp, the stone's glow (GravemotherStoneMark.cpp), GravemotherTuning.

#include "Bosses/BossComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherFight.h"
#include "Creatures/GravemotherStone.h"
#include "Creatures/GravemotherTuning.h"
#include "Creatures/HuntingGround.h"
#include "Missions/ObjectiveHighlightSubsystem.h"
#include "Tests/BossTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "Tests/AutomationCommon.h"

using namespace GravemotherTestWorld;

namespace
{
	/** A shot on Target at Where from the -X side, as a bullet hands it over. */
	FPointDamageEvent ShotAt(AActor& Target, UPrimitiveComponent* Part, const FVector& Where, float Damage)
	{
		const FHitResult Hit(&Target, Part, Where, FVector(-1.0, 0.0, 0.0));
		return FPointDamageEvent(Damage, Hit, FVector(1.0, 0.0, 0.0), UDamageType::StaticClass());
	}

	/** A player stand-in at Where with a controller to shoot through. */
	AAIController* SpawnShooter(UWorld* World, const FVector& Where)
	{
		ACharacter* Player = BossTestWorld::SpawnPlayer(World, Where);
		AAIController* Controller = Player ? World->SpawnActor<AAIController>() : nullptr;
		if (Controller)
		{
			Controller->SetPawn(Player);
		}
		return Controller;
	}

	/** Sets a cvar for the rest of a scope, then puts it back. */
	struct FCVarScope
	{
		FCVarScope(const TCHAR* Name, float Value)
			: Var(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Var)
			{
				Was = Var->GetFloat();
				Var->Set(Value, ECVF_SetByCode);
			}
		}
		~FCVarScope()
		{
			if (Var)
			{
				Var->Set(Was, ECVF_SetByCode);
			}
		}
		IConsoleVariable* Var = nullptr;
		float Was = 0.f;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherShrugStoneTest, "Looter.Creatures.Gravemother.Shrug.Stone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherShrugStoneTest::RunTest(const FString& Parameters)
{
	// While her headstone stands nothing hurts her: a shot, a blast, a blow glance off (no health, no number), she turns on
	// the shooter all the same, the stone glows as the target a moment, and after three Hob says to crack it off. Bullets
	// landing on her then show her own look (the own-look tag on the part struck), and once it's off they hurt as ever.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World, FVector::ZeroVector, 3000.0);
	AAIController* Shooter = SpawnShooter(World, FVector(1200.0, 0.0, 90.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector, 5);
	AGravemotherStone* Stone = Mother ? Mother->GetStone() : nullptr;
	if (!TestNotNull(TEXT("Shooter"), Shooter) || !TestNotNull(TEXT("Her headstone"), Stone))
	{
		return false;
	}
	const UHealthComponent* Hers = Mother->FindComponentByClass<UHealthComponent>();
	TestEqual(TEXT("With the stone on, a hit is shrugged off: the stone"), Mother->ShrugFor(Shooter), EGravemotherShrug::Stone);

	const float Before = Hers->GetHealth();
	const FVector OnHer = Mother->GetActorLocation() - FVector(150.0, 0.0, 0.0);
	const float Taken = Mother->TakeDamage(200.f, ShotAt(*Mother, Mother->GetMesh(), OnHer, 200.f), Shooter, nullptr);
	TestEqual(TEXT("A shot on her takes nothing"), Taken, 0.f);
	TestEqual(TEXT("...her health untouched"), Hers->GetHealth(), Before);
	TestEqual(TEXT("...counted as shrugged"), Mother->GetShrugCount(EGravemotherShrug::Stone), 1);
	TestEqual(TEXT("...she turns on the shooter as a hit would turn her"), Mother->GetCreatureState(), ECreatureState::Chase);
	TestTrue(TEXT("...and the stone glows as the target"), Stone->IsMarked());
	const bool bHasLook = LoadObject<UMaterialInterface>(nullptr, UObjectiveHighlightSubsystem::MaterialPath, nullptr, LOAD_NoWarn) != nullptr;
	if (bHasLook)
	{
		TestNotNull(TEXT("...the highlight drawn over it"), Stone->GetSlab()->GetOverlayMaterial());
	}
	Stone->Advance(0.1f);
	TestTrue(TEXT("...coming up, pulsing"), Stone->GetMarkStrength() > 0.f && Stone->GetMarkStrength() <= 1.f);
	for (float Time = 0.f; Time < GravemotherTuning::MarkSeconds() + 0.2f; Time += Step)
	{
		Stone->Advance(Step);
	}
	TestTrue(TEXT("...and fading out after its time"), !Stone->IsMarked() && Stone->GetSlab()->GetOverlayMaterial() == nullptr);

	// A grave-salt jar's blast is shrugged off too.
	FRadialDamageEvent Blast;
	Blast.Origin = Mother->GetActorLocation() + FVector(-300.0, 0.0, 0.0);
	Blast.DamageTypeClass = UDamageType::StaticClass();
	TestEqual(TEXT("A blast takes nothing"), Mother->TakeDamage(300.f, Blast, Shooter, nullptr), 0.f);
	TestEqual(TEXT("...her health untouched"), Hers->GetHealth(), Before);

	// The third: Hob's hint (once).
	const bool bHintAtThree = GravemotherTuning::HintAfter() == GravemotherTuning::DefaultHintAfter;
	if (bHintAtThree)
	{
		TestFalse(TEXT("No hint after two"), Mother->WasHinted(EGravemotherShrug::Stone));
	}
	Mother->TakeDamage(50.f, ShotAt(*Mother, Mother->GetMesh(), OnHer, 50.f), Shooter, nullptr);
	if (bHintAtThree)
	{
		TestTrue(TEXT("Hob's hint after three"), Mother->WasHinted(EGravemotherShrug::Stone));
	}
	TestFalse(TEXT("Hob's line about the stone has words"), GravemotherFight::ShrugLine(EGravemotherShrug::Stone).Text.IsEmpty());
	TestFalse(TEXT("...and her bar a callout"), GravemotherFight::ShrugCallout(EGravemotherShrug::Stone).IsEmpty());

	// Bullets: a shot she'll shrug off shows her own look (the part struck carries the own-look tag as the look is chosen).
	const FName OwnLook(UBulletSubsystem::OwnLookTag);
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	FBulletShot Shot;
	Shot.Instigator = Shooter;
	FHitResult Hit(Mother, Mother->GetMesh(), OnHer, FVector(-1.0, 0.0, 0.0));
	Hit.BoneName = AGravemotherCreature::AbdomenBone;
	FBulletLanding Landing;
	if (TestNotNull(TEXT("The bullets"), Bullets))
	{
		Bullets->OnBulletLanding.Broadcast(Shot, Hit, Landing);
		TestTrue(TEXT("A bullet landing on her while the stone stands: her own look"), Mother->GetMesh()->ComponentHasTag(OwnLook));
	}

	// The stone off: hits hurt again, and bullets show her wounds as ever.
	Stone->Break(false);
	TestEqual(TEXT("Stone off: a hit hurts"), Mother->ShrugFor(Shooter), EGravemotherShrug::None);
	TestFalse(TEXT("...and its glow is gone with it"), Stone->IsMarked());
	if (Bullets)
	{
		Bullets->OnBulletLanding.Broadcast(Shot, Hit, Landing);
		TestFalse(TEXT("...a bullet landing on her shows a wound's look"), Mother->GetMesh()->ComponentHasTag(OwnLook));
	}

	// The rule's cvar: off, the stone guards nothing.
	Mother->ResetToHome();
	TestTrue(TEXT("Whole again for a new life"), Mother->IsStoneOn());
	{
		FCVarScope Off(TEXT("Looter.Gravemother.StoneGuards"), 0.f);
		TestEqual(TEXT("Looter.Gravemother.StoneGuards 0: the stone guards nothing"), Mother->ShrugFor(Shooter), EGravemotherShrug::None);
	}
	TestEqual(TEXT("...back on"), Mother->ShrugFor(Shooter), EGravemotherShrug::Stone);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherShrugRimTest, "Looter.Creatures.Gravemother.Shrug.Rim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherShrugRimTest::RunTest(const FString& Parameters)
{
	// Her lair's hunting ground (36 m round the den's mouth, 3 m up or down: build_area_den.py): a shooter off it (the Sink's
	// rim, 12 m up; across the field) can't hurt her or her stone, stone or none; one on it hurts her once the stone is off.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World, FVector::ZeroVector, 3000.0);
	AAIController* Floor = SpawnShooter(World, FVector(1200.0, 0.0, 90.0));
	AAIController* Rim = SpawnShooter(World, FVector(1200.0, 600.0, 1200.0));
	AAIController* Far = SpawnShooter(World, FVector(5000.0, 0.0, 90.0));
	// Jumping off a berm (2.6 m) a little past her ground's height: still on it for her hits (the slack).
	AAIController* Jumper = SpawnShooter(World, FVector(1200.0, -600.0, 480.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector, 5);
	AGravemotherStone* Stone = Mother ? Mother->GetStone() : nullptr;
	if (!TestNotNull(TEXT("Shooters"), Floor) || !TestNotNull(TEXT("..."), Rim) || !TestNotNull(TEXT("..."), Far)
		|| !TestNotNull(TEXT("..."), Jumper) || !TestNotNull(TEXT("Her headstone"), Stone))
	{
		return false;
	}
	Mother->HuntingGround = FHuntingGround::MakeAround(FVector::ZeroVector, 3600.f, {}, 200.f, 300.f);

	TestEqual(TEXT("On her ground, with the stone on: the stone"), Mother->ShrugFor(Floor), EGravemotherShrug::Stone);
	TestEqual(TEXT("On the rim: off her ground (the one to tell)"), Mother->ShrugFor(Rim), EGravemotherShrug::OffGround);
	TestEqual(TEXT("Past her ground's reach: off it"), Mother->ShrugFor(Far), EGravemotherShrug::OffGround);
	if (FMath::IsNearlyEqual(GravemotherTuning::RimSlack(), GravemotherTuning::DefaultRimSlack))
	{
		TestEqual(TEXT("Jumping off a berm, 4.8 m up (her ground's 3 m and the slack's 3): still on it"), Mother->ShrugFor(Jumper),
			EGravemotherShrug::Stone);
	}

	// A rim shot on the stone glances off too: the stone keeps its health, she counts it.
	const UHealthComponent* Its = Stone->GetHealthComponent();
	const float StoneBefore = Its->GetHealth();
	const FVector OnStone = Stone->GetSlab()->Bounds.Origin;
	TestEqual(TEXT("A rim shot on the stone takes nothing"), Stone->TakeDamage(80.f, ShotAt(*Stone, Stone->GetSlab(), OnStone, 80.f), Rim, nullptr), 0.f);
	TestEqual(TEXT("...the stone untouched"), Its->GetHealth(), StoneBefore);
	TestEqual(TEXT("...shrugged as off her ground"), Mother->GetShrugCount(EGravemotherShrug::OffGround), 1);
	TestFalse(TEXT("...not the stone's glow: the answer is to come down"), Stone->IsMarked());
	{
		// From the floor it takes the hit, as it always did (the engine's events let run, as BossTestWorld::Hurt does).
		FEditorScriptExecutionGuard RunActorEvents;
		Stone->TakeDamage(80.f, ShotAt(*Stone, Stone->GetSlab(), OnStone, 80.f), Floor, nullptr);
		TestTrue(TEXT("A shot on the stone from her ground hurts it"), Its->GetHealth() < StoneBefore);
	}

	// The stone off: the floor's shots hurt; the rim's still don't.
	Stone->Break(false);
	TestEqual(TEXT("Stone off, on her ground: it hurts"), Mother->ShrugFor(Floor), EGravemotherShrug::None);
	TestEqual(TEXT("...from the rim: still nothing"), Mother->ShrugFor(Rim), EGravemotherShrug::OffGround);
	const UHealthComponent* Hers = Mother->FindComponentByClass<UHealthComponent>();
	const float Before = Hers->GetHealth();
	TestEqual(TEXT("A rim shot on her takes nothing"), Mother->TakeDamage(300.f, ShotAt(*Mother, Mother->GetMesh(),
		Mother->GetActorLocation() - FVector(150.0, 0.0, 0.0), 300.f), Rim, nullptr), 0.f);
	TestEqual(TEXT("...her health untouched"), Hers->GetHealth(), Before);
	TestEqual(TEXT("...two shrugged from off her ground"), Mother->GetShrugCount(EGravemotherShrug::OffGround), 2);
	TestFalse(TEXT("Hob's line about the rim has words"), GravemotherFight::ShrugLine(EGravemotherShrug::OffGround).Text.IsEmpty());
	TestTrue(TEXT("...and nothing for a hit that hurts"), GravemotherFight::ShrugLine(EGravemotherShrug::None).Text.IsEmpty());

	// The rule's cvar.
	{
		FCVarScope Off(TEXT("Looter.Gravemother.RimShrug"), 0.f);
		TestEqual(TEXT("Looter.Gravemother.RimShrug 0: the rim's shots hurt"), Mother->ShrugFor(Rim), EGravemotherShrug::None);
	}
	// Without a ground of her own (the console's, a test's) there's no rim.
	Mother->HuntingGround = FHuntingGround();
	TestEqual(TEXT("No hunting ground: no rim rule"), Mother->ShrugFor(Far), EGravemotherShrug::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherShrugTuningTest, "Looter.Creatures.Gravemother.Shrug.Tuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherShrugTuningTest::RunTest(const FString& Parameters)
{
	// The user's "~1.6x health and harder hits": her class carries 1.6x the first tuning's 300 and 1.25x its 12 (the Ledger
	// reads the class); the cvars move one spawned after; a stagger takes the same crits as before (7% of the old health).
	const AGravemotherCreature* Defaults = GetDefault<AGravemotherCreature>();
	const UHealthComponent* DefaultHealth = Defaults->FindComponentByClass<UHealthComponent>();
	if (!TestNotNull(TEXT("Her class's health"), DefaultHealth))
	{
		return false;
	}
	TestNearlyEqual(TEXT("Her class: 300 x 1.6"), DefaultHealth->MaxHealth, 300.f * GravemotherTuning::DefaultHealthScale, 0.01f);
	TestNearlyEqual(TEXT("...her bite 12 x 1.25 (her Furrow, quake and clod come from it)"), Defaults->AttackDamage,
		12.f * GravemotherTuning::DefaultDamageScale, 0.001f);
	TestNearlyEqual(TEXT("Her stagger takes the same crits as before (7% of 300)"),
		GravemotherFight::MakeStagger().CritShare * DefaultHealth->MaxHealth, 0.07f * 300.f, 0.5f);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AGravemotherCreature* AsSet = SpawnMother(World, FVector::ZeroVector, 4);
	AGravemotherCreature* Doubled = nullptr;
	{
		FCVarScope Health(TEXT("Looter.Gravemother.HealthScale"), GravemotherTuning::HealthScale() * 2.f);
		FCVarScope Damage(TEXT("Looter.Gravemother.DamageScale"), GravemotherTuning::DamageScale() * 2.f);
		Doubled = SpawnMother(World, FVector(0.0, 3000.0, 0.0), 4);
	}
	if (!TestNotNull(TEXT("Her as the cvars stand"), AsSet) || !TestNotNull(TEXT("Her with them doubled"), Doubled))
	{
		return false;
	}
	const float Health = AsSet->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
	TestNearlyEqual(TEXT("The health cvar doubled: twice the health"), Doubled->FindComponentByClass<UHealthComponent>()->GetMaxHealth(), Health * 2.f, 1.f);
	TestNearlyEqual(TEXT("The damage cvar doubled: twice the hit"), Doubled->AttackDamage, AsSet->AttackDamage * 2.f, 0.01f);
	TestNearlyEqual(TEXT("...her stone a fifth of her health either way"), Doubled->GetStone()->GetHealthComponent()->GetMaxHealth(),
		Doubled->FindComponentByClass<UHealthComponent>()->GetMaxHealth() * Doubled->StoneSettings.HealthShare, 1.f);
	return true;
}

#endif
