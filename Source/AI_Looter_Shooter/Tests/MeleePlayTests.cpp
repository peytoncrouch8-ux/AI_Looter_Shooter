#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitReaction.h"
#include "Combat/TargetDummy.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootDropComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerMeleeRules.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/MeleeDamageType.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The melee strike in a test level (its rules alone are MeleeTests.cpp): finding the body in front, the stagger and
// hit-stop it gives, the knock back (never over an edge, never a boss), its sounds by body, the player's character
// carrying it, and the reload it cuts short.

namespace
{
	/**
	 * A creature in a test level standing on the ground at Where (its feet), started as play starts it, its movement
	 * given its capsule (a test level doesn't), dropping nothing when it dies.
	 */
	template <typename TCreature>
	TCreature* SpawnCreature(UWorld* World, const FVector& Where, ECreatureRank Rank = ECreatureRank::Basic)
	{
		TCreature* Creature = World->SpawnActor<TCreature>(Where + FVector(0.0, 0.0, 300.0), FRotator::ZeroRotator);
		if (!Creature)
		{
			return nullptr;
		}
		Creature->StartingRank = Rank;
		if (ULootDropComponent* Loot = Creature->template FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		Creature->DispatchBeginPlay();
		const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Creature->SetActorLocation(Where + FVector(0.0, 0.0, HalfHeight + 1.0), false, nullptr, ETeleportType::TeleportPhysics);
		UCharacterMovementComponent* Movement = Creature->GetCharacterMovement();
		Movement->SetUpdatedComponent(Creature->GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Walking);
		// A launch only takes on an active movement (UCharacterMovementComponent::Launch).
		Movement->Activate(true);
		return Creature;
	}

	/** Where a creature stands with its near side Gap cm ahead of the origin along Way (across the ground). */
	FVector InFront(const ACreatureBase* Creature, const FVector& Way, float Gap)
	{
		const float Radius = Creature ? Creature->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.f;
		return Way.GetSafeNormal2D() * (Gap + Radius);
	}

	void PlaceAt(ACreatureBase* Creature, const FVector& Feet)
	{
		const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Creature->SetActorLocation(Feet + FVector(0.0, 0.0, HalfHeight + 1.0), false, nullptr, ETeleportType::TeleportPhysics);
	}

	/** The player's eye over the origin, 140 cm up, looking along Way. */
	FMeleeAim AimAlong(const FVector& Way)
	{
		FMeleeAim Aim;
		Aim.Eye = FVector(0.0, 0.0, 140.0);
		Aim.Forward = Way.GetSafeNormal();
		Aim.FeetZ = 0.0;
		return Aim;
	}

	/**
	 * A strike's damage as its health hears it, inside the strike's scope (UPlayerMeleeComponent::LandStrike): a test
	 * level has no game mode, so a pawn would ignore ApplyPointDamage, and actors' own events need letting through.
	 */
	void Strike(AActor* Victim, float Damage)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		FHitReaction::FMeleeScope MeleeHit;
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, GetDefault<UMeleeDamageType>(), nullptr, nullptr);
	}

	/** A bullet's damage of the same size, for comparison. */
	void Shoot(AActor* Victim, float Damage)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, GetDefault<UDamageType>(), nullptr, nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeStrikePlayTest, "Looter.Melee.Strike",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeStrikePlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	// A floor 40 m across with its top at 0.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	ASpiderCreature* Ahead = SpawnCreature<ASpiderCreature>(World, FVector(0.0, -1500.0, 0.0));
	ASpiderCreature* Behind = SpawnCreature<ASpiderCreature>(World, FVector(0.0, -1200.0, 0.0));
	ASlimeCreature* Aside = SpawnCreature<ASlimeCreature>(World, FVector(0.0, -900.0, 0.0));
	if (!TestNotNull(TEXT("A spider ahead"), Ahead) || !TestNotNull(TEXT("A spider behind"), Behind) || !TestNotNull(TEXT("A slime aside"), Aside))
	{
		return false;
	}
	PlaceAt(Ahead, InFront(Ahead, FVector::ForwardVector, 80.f));
	PlaceAt(Behind, InFront(Behind, -FVector::ForwardVector, 60.f));
	PlaceAt(Aside, InFront(Aside, FVector::RightVector, 60.f));
	const float Reach = FMeleeRules::Reach();

	// The body in front is the one struck; the one behind and the one at the side are not.
	FMeleeContact Contact;
	TestTrue(TEXT("Looking ahead: the spider ahead"), UPlayerMeleeComponent::FindStrikeTarget(World, AimAlong(FVector::ForwardVector), Reach, nullptr, Contact) == Ahead);
	TestEqual(TEXT("...80 cm off"), Contact.Distance, 80.f, 2.f);
	TestTrue(TEXT("Turned right: the slime"), UPlayerMeleeComponent::FindStrikeTarget(World, AimAlong(FVector::RightVector), Reach, nullptr, Contact) == Aside);
	TestNull(TEXT("Turned left: nothing"), UPlayerMeleeComponent::FindStrikeTarget(World, AimAlong(-FVector::RightVector), Reach, nullptr, Contact));
	TestNull(TEXT("Nothing past the reach"), UPlayerMeleeComponent::FindStrikeTarget(World, AimAlong(FVector::ForwardVector), 50.f, nullptr, Contact));

	// The strike: damage, a stagger (light as it is: a gun's hit that size does nothing) and a hit-stop, on that one only.
	const UHealthComponent* AheadHealth = Ahead->FindComponentByClass<UHealthComponent>();
	const UCreatureHitReactionComponent* AheadReaction = Ahead->FindComponentByClass<UCreatureHitReactionComponent>();
	const UCreatureHitReactionComponent* BehindReaction = Behind->FindComponentByClass<UCreatureHitReactionComponent>();
	if (!TestNotNull(TEXT("Its health"), AheadHealth) || !TestNotNull(TEXT("Its hit reactions"), AheadReaction) || !TestNotNull(TEXT("..."), BehindReaction))
	{
		return false;
	}
	const float Before = AheadHealth->GetHealth();
	Strike(Ahead, 10.f);
	TestEqual(TEXT("The strike's damage taken"), AheadHealth->GetHealth(), Before - 10.f, 0.01f);
	TestTrue(TEXT("Staggered by a light strike"), AheadReaction->IsStaggered());
	TestTrue(TEXT("...and stopped for an instant"), AheadReaction->IsHitStopped() && Ahead->CustomTimeDilation == FHitReaction::HitStopDilation);
	const double Now = World->GetTimeSeconds();
	const float MeleeStagger = FHitReaction::StaggerSecondsFor(ECreatureRank::Basic) * FHitReaction::MeleeStaggerScale;
	TestTrue(TEXT("...for half again a heavy hit's stagger"), AheadReaction->GetReaction().IsStaggered(Now + MeleeStagger - 0.01)
		&& !AheadReaction->GetReaction().IsStaggered(Now + MeleeStagger + 0.01));
	Shoot(Behind, 10.f);
	TestFalse(TEXT("A bullet's 10 staggers nothing"), BehindReaction->IsStaggered());
	TestFalse(TEXT("The scope closed with the strike"), FHitReaction::FMeleeScope::IsOpen());

	// The knock: a hop away, a slime a quarter harder than its rank and size alone would give.
	const FVector Knock = UPlayerMeleeComponent::KnockBack(*Ahead, FVector::ForwardVector);
	TestTrue(FString::Printf(TEXT("The spider hops away (%s)"), *Knock.ToCompactString()), Knock.X > 0.0 && Knock.Z > 0.0);
	TestTrue(TEXT("...launched"), Ahead->GetCharacterMovement()->PendingLaunchVelocity.Equals(Knock, 0.01));
	const FVector SlimeKnock = UPlayerMeleeComponent::KnockBack(*Aside, FVector::RightVector);
	const FVector SlimeHeavy = FMeleeRules::KnockVelocity(FVector::RightVector, Aside->GetSizeScale(), Aside->GetRank(), false);
	TestEqual(TEXT("A slime goes a quarter harder"), static_cast<float>(SlimeKnock.Y / SlimeHeavy.Y), FMeleeRules::LightKnockScale, 0.01f);

	// Never off an edge: at the floor's edge, half as far if that lands, else no knock at all.
	ASpiderCreature* Edge = SpawnCreature<ASpiderCreature>(World, FVector(1990.0, 1500.0, 0.0));
	if (TestNotNull(TEXT("A spider at the edge"), Edge))
	{
		const FVector Full = FMeleeRules::KnockVelocity(FVector::ForwardVector, Edge->GetSizeScale(), Edge->GetRank(), false);
		const float Gravity = -Edge->GetCharacterMovement()->GetGravityZ();
		const float Hop = static_cast<float>(Full.X) * 2.f * static_cast<float>(Full.Z) / Gravity;
		TestTrue(TEXT("At the very edge, no knock"), UPlayerMeleeComponent::KnockBack(*Edge, FVector::ForwardVector).IsZero());
		// Three quarters of a hop from the edge: the full one goes over, the half lands.
		PlaceAt(Edge, FVector(2000.0 - Hop * 0.75, 1500.0, 0.0));
		const FVector Half = UPlayerMeleeComponent::KnockBack(*Edge, FVector::ForwardVector);
		TestEqual(TEXT("Near the edge, half as far"), static_cast<float>(Half.X), static_cast<float>(Full.X) * 0.5f, 0.5f);
		TestEqual(TEXT("...as high"), static_cast<float>(Half.Z), static_cast<float>(Full.Z), 0.5f);
	}

	// A boss and a Soulfed monster take the damage only; a corpse and a dummy aren't knocked.
	ASpiderCreature* Boss = SpawnCreature<ASpiderCreature>(World, FVector(-1500.0, 1500.0, 0.0), ECreatureRank::Boss);
	ASpiderCreature* Soulfed = SpawnCreature<ASpiderCreature>(World, FVector(-1500.0, -1500.0, 0.0), ECreatureRank::Legendary);
	if (TestNotNull(TEXT("A boss"), Boss) && TestNotNull(TEXT("A Soulfed spider"), Soulfed))
	{
		TestTrue(TEXT("No knock for a boss"), UPlayerMeleeComponent::KnockBack(*Boss, FVector::ForwardVector).IsZero());
		TestTrue(TEXT("No knock for a Soulfed monster"), UPlayerMeleeComponent::KnockBack(*Soulfed, FVector::ForwardVector).IsZero());
	}
	if (const UHealthComponent* BehindHealth = Behind->FindComponentByClass<UHealthComponent>())
	{
		Strike(Behind, BehindHealth->GetHealth() + 10.f);
		TestTrue(TEXT("Killed"), Behind->IsDead());
		TestTrue(TEXT("No knock for a corpse (the kill's own knock takes it)"), UPlayerMeleeComponent::KnockBack(*Behind, FVector::ForwardVector).IsZero());
	}
	ATargetDummy* Dummy = World->SpawnActor<ATargetDummy>(FVector(1500.0, -1500.0, 0.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A dummy"), Dummy))
	{
		TestTrue(TEXT("No knock for a dummy"), UPlayerMeleeComponent::KnockBack(*Dummy, FVector::ForwardVector).IsZero());
	}

	// What it sounds like: a spider's shell, a slime's gel; nothing of its own for what isn't a creature.
	TestTrue(TEXT("A spider: shell"), UPlayerMeleeComponent::HitCueOf(Ahead) == FName(MeleeCue::HitShell));
	TestTrue(TEXT("A slime: gel"), UPlayerMeleeComponent::HitCueOf(Aside) == FName(MeleeCue::HitGel));
	TestTrue(TEXT("A dummy: the bullets' sound for it"), UPlayerMeleeComponent::HitCueOf(Dummy).IsNone());

	// The player's character carries the strike: its aim is its own eye and look, and it reaches the spider in front.
	UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, FVector(1500.0, 1500.0, 0.0));
	APawn* Player = Locomotion ? Cast<APawn>(Locomotion->GetOwner()) : nullptr;
	UPlayerMeleeComponent* Melee = UPlayerMeleeComponent::Find(Player);
	if (TestNotNull(TEXT("The player"), Player) && TestNotNull(TEXT("The player's character has the strike"), Melee))
	{
		const FMeleeAim Aim = Melee->GetAim();
		const float EyeOverFeet = static_cast<float>(Aim.Eye.Z - Aim.FeetZ);
		TestTrue(FString::Printf(TEXT("Its eye is at the player's height (%.0f cm)"), EyeOverFeet), EyeOverFeet > 100.f && EyeOverFeet < 180.f);
		TestTrue(TEXT("It looks the way it faces"), Aim.Forward.Equals(FVector::ForwardVector, 0.01));
		ASpiderCreature* Close = SpawnCreature<ASpiderCreature>(World, FVector(1500.0, 1100.0, 0.0));
		if (TestNotNull(TEXT("A spider by the player"), Close))
		{
			PlaceAt(Close, FVector(Aim.Eye.X, Aim.Eye.Y, 0.0) + InFront(Close, FVector::ForwardVector, 90.f));
			TestTrue(TEXT("Its strike finds the spider in front"), Melee->FindTargetInReach() == Close);
			TestTrue(TEXT("...as does the hint's wider look"), Melee->FindTargetInReach(2.f) == Close);
		}
		// Nobody controls it: no strike, and nothing counted.
		TestTrue(TEXT("With no player in control, no strike"), Melee->GetBlock() == EMeleeBlock::NoPlayer && !Melee->TryMelee());
		TestFalse(TEXT("...and none counted"), Melee->HasMeleed());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMeleeReloadPlayTest, "Looter.Melee.Reload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMeleeReloadPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();

	// A holder as the game has one (health and a weapon manager), with a rifle in hand five rounds in and rounds to spare.
	APawn* Holder = World->SpawnActor<APawn>();
	if (!TestNotNull(TEXT("A holder"), Holder))
	{
		return false;
	}
	UHealthComponent* Health = NewObject<UHealthComponent>(Holder, TEXT("Health"));
	Health->bShowDamageNumbers = false;
	Holder->AddInstanceComponent(Health);
	Health->RegisterComponent();
	UWeaponManagerComponent* Weapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
	Holder->AddInstanceComponent(Weapons);
	Weapons->RegisterComponent();
	Weapons->MaxWeapons = 3;
	Holder->DispatchBeginPlay();
	FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1);
	Instance.SavedMagazine = 5;
	AWeaponBase* Gun = Weapons->GiveWeapon(Instance);
	if (!TestNotNull(TEXT("A rifle in hand"), Gun))
	{
		return false;
	}
	Weapons->AddAmmo(Gun->GetAmmoType(), 90);

	// Borderlands' way: the strike cuts the reload short, and it starts over once the swing is done.
	Gun->Reload();
	TestTrue(TEXT("Reloading"), Gun->IsReloading());
	TestTrue(TEXT("The swing cuts the reload short"), Gun->BeginMeleeSwing());
	TestTrue(TEXT("...no longer reloading"), !Gun->IsReloading() && Gun->IsMeleeSwinging());
	Gun->Reload();
	TestFalse(TEXT("A reload asked for mid-swing waits"), Gun->IsReloading());
	const int32 Rounds = Gun->GetCurrentMagazine();
	Gun->StartFire();
	TestEqual(TEXT("No shot mid-swing"), Gun->GetCurrentMagazine(), Rounds);
	Gun->StopFire();
	Gun->EndMeleeSwing(true);
	TestTrue(TEXT("The swing over, the reload starts over"), Gun->IsReloading() && !Gun->IsMeleeSwinging());

	// A gun that leaves the hand mid-swing reloads nothing after.
	Gun->BeginMeleeSwing();
	Gun->EndMeleeSwing(false);
	TestFalse(TEXT("Swing ended early: no reload"), Gun->IsReloading());

	// No reload under way: nothing to cut, nothing after.
	TestFalse(TEXT("Nothing to cut short"), Gun->BeginMeleeSwing());
	Gun->EndMeleeSwing(true);
	TestFalse(TEXT("...and no reload after"), Gun->IsReloading());

	// Put away mid-swing, it's free to fire again.
	Gun->BeginMeleeSwing();
	Gun->OnHolstered();
	TestFalse(TEXT("Holstered: the swing let go"), Gun->IsMeleeSwinging());
	return true;
}

#endif
