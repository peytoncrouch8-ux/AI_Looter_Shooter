#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Player/PlayerMeleeRules.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "World/BreakableDebris.h"
#include "World/BreakableKinds.h"
#include "World/BreakableProp.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The lootable world's crates and barrels (Docs/Polish/BorderlandsComparison.md, item 10): ABreakableProp broken by shots
// or one melee strike into its pre-broken pieces (UBreakableDebrisSubsystem), sometimes leaving ammo or a soul-mote, kept
// broken by the session, placed by Tools/Unreal/build_area_loot.py.

namespace
{
	constexpr EBreakableKind Kinds[] = { EBreakableKind::SlattedCrate, EBreakableKind::PackingCrate, EBreakableKind::Barrel };

	/** A breakable of a kind at Where, as the build script sets it up, begun. */
	ABreakableProp* SpawnProp(UWorld* World, EBreakableKind Kind, const FVector& Where, FName Id = NAME_None)
	{
		const FTransform At(FRotator::ZeroRotator, Where);
		ABreakableProp* Prop = World->SpawnActorDeferred<ABreakableProp>(ABreakableProp::StaticClass(), At);
		if (!Prop)
		{
			return nullptr;
		}
		Prop->Kind = Kind;
		Prop->BreakableId = Id;
		Prop->FinishSpawning(At);
		Prop->DispatchBeginPlay();
		return Prop;
	}

	/**
	 * A blow of Damage on it from the -X side, as AActor::TakeDamage hands a shot's to its listeners (the point, then the
	 * damage). The engine drops actors' own events in a test level until play begins, so they're let through here, as the
	 * other tests do (BossTestWorld::Hurt).
	 */
	void Hit(ABreakableProp* Prop, float Damage)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		const FVector Point = Prop->GetActorLocation() + FVector(-40.0, 0.0, 30.0);
		const UDamageType* Type = GetDefault<UDamageType>();
		Prop->OnTakePointDamage.Broadcast(Prop, Damage, nullptr, Point, Prop->Body, NAME_None, FVector(1.0, 0.0, 0.0), Type, nullptr);
		Prop->OnTakeAnyDamage.Broadcast(Prop, Damage, Type, nullptr, nullptr);
	}

	/** Moves the pieces on for Seconds, a frame at a time (a test level never ticks). */
	void RunDebris(UBreakableDebrisSubsystem* Debris, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Debris->Advance(Frame);
		}
	}

	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakablesKindsTest, "Looter.Loot.Breakables.Kinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBreakablesKindsTest::RunTest(const FString& Parameters)
{
	// Each kind: one melee strike always breaks it (the least a level-1 strike does), a single level-1 round never does;
	// its models are named as Lootables.py names them; what a break gives is never a gun, at most one ammo pickup at a
	// kill's 18-36 rounds leaning toward the breaking gun, at its kind's chance.
	const float LeastStrike = FMeleeRules::BaseDamage * 0.9f;
	for (const EBreakableKind Kind : Kinds)
	{
		const FBreakableKindInfo Info = FBreakableKindInfo::Get(Kind);
		const FString Name = UEnum::GetDisplayValueAsText(Kind).ToString();
		TestTrue(FString::Printf(TEXT("%s: one strike breaks it (%.0f health under the least strike, %.0f)"), *Name, Info.Health, LeastStrike),
			Info.Health > 0.f && Info.Health < LeastStrike);
		TestTrue(FString::Printf(TEXT("%s: one plain level-1 round (20, up to 22) doesn't"), *Name), Info.Health > 22.f);
		TestTrue(FString::Printf(TEXT("%s: pieces to throw, a stump and a sound"), *Name), Info.PieceCount >= 5 && Info.StumpPath && Info.BreakCue);
		TestTrue(FString::Printf(TEXT("%s: its first piece's path, as Lootables.py names it"), *Name),
			Info.PiecePath(0).StartsWith(TEXT("/Game/Art/Props/SM_Break_")) && Info.PiecePath(0).EndsWith(TEXT("_1")));
		TestTrue(FString::Printf(TEXT("%s: chances that read as sometimes"), *Name), Info.AmmoChance > 0.2f && Info.AmmoChance < 0.6f
			&& Info.MoteChance > 0.f && Info.MoteChance < 0.15f);
	}
	TestEqual(TEXT("A piece's object path"), FBreakableKindInfo::Get(EBreakableKind::Barrel).PiecePath(2),
		FString(TEXT("/Game/Art/Props/SM_Break_BarrelA_3.SM_Break_BarrelA_3")));

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ABreakableProp* Crate = SpawnProp(TestLevel.GetTestWorld(), EBreakableKind::SlattedCrate, FVector(300.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A crate"), Crate))
	{
		return false;
	}
	const ULootTable* Table = Crate->MakeLootTable();
	TestTrue(TEXT("Its loot: never a gun"), Table->WeaponDropChance == 0.f && Table->MaxWeaponDrops == 0);
	TestTrue(TEXT("...one ammo pickup at its chance"), Table->MinAmmoDrops == 1 && Table->MaxAmmoDrops == 1
		&& FMath::IsNearlyEqual(Table->AmmoDropChance, Crate->GetKindInfo().AmmoChance));
	TestTrue(TEXT("...a kill's 18-36 rounds, leaning toward the gun that broke it"), Table->AmmoAmountMin == LooterLoot::KillAmmoAmountMin
		&& Table->AmmoAmountMax == LooterLoot::KillAmmoAmountMax && Table->KillWeaponAmmoBias > 1.f);
	// Counted over many breaks from a seed: the ammo comes about as often as its chance says, never a gun; the mote likewise.
	FRandomStream Random(0xb4ea6);
	constexpr int32 Breaks = 4000;
	int32 WithAmmo = 0;
	int32 Guns = 0;
	int32 Motes = 0;
	for (int32 Index = 0; Index < Breaks; ++Index)
	{
		const FLootRoll Roll = ULootLibrary::RollLoot(Table, 1, 0.f, Random, EAmmoType::AssaultRifle, /*bWeapons*/ false);
		WithAmmo += Roll.Ammo.IsEmpty() ? 0 : 1;
		Guns += Roll.Weapons.Num();
		Motes += LooterBreakables::RollMote(Crate->GetKindInfo(), Random) ? 1 : 0;
	}
	const float AmmoShare = static_cast<float>(WithAmmo) / Breaks;
	const float MoteShare = static_cast<float>(Motes) / Breaks;
	TestTrue(FString::Printf(TEXT("Ammo from %.1f%% of breaks (its chance %.0f%%)"), AmmoShare * 100.f, Crate->GetKindInfo().AmmoChance * 100.f),
		FMath::Abs(AmmoShare - Crate->GetKindInfo().AmmoChance) < 0.03f);
	TestTrue(FString::Printf(TEXT("A soul-mote from %.1f%% (its chance %.0f%%)"), MoteShare * 100.f, Crate->GetKindInfo().MoteChance * 100.f),
		FMath::Abs(MoteShare - Crate->GetKindInfo().MoteChance) < 0.02f);
	TestEqual(TEXT("No gun ever"), Guns, 0);

	// What the shots and the strike need of it: solid to them (world dynamic, blocking), with health and no numbers over it.
	TestTrue(TEXT("Solid, world dynamic, the root the hits name"), Crate->Body == Crate->GetRootComponent()
		&& Crate->Body->GetCollisionObjectType() == ECC_WorldDynamic && Crate->Body->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		&& Crate->Body->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
	TestTrue(TEXT("Health at its kind's, no damage numbers"), Crate->Health && FMath::IsNearlyEqual(Crate->Health->GetMaxHealth(),
		Crate->GetKindInfo().Health) && !Crate->Health->bShowDamageNumbers);
	TestTrue(TEXT("Tagged Breakable and Obstacle"), Crate->ActorHasTag(FName(LooterBreakables::Tag)) && Crate->ActorHasTag(FName(TEXT("Obstacle"))));
	TestFalse(TEXT("Idle, it never ticks"), Crate->IsActorTickEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakablesBreakTest, "Looter.Loot.Breakables.Break",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBreakablesBreakTest::RunTest(const FString& Parameters)
{
	// A shot that doesn't break it rocks it away from the blow and back, ticking only meanwhile; the blow that empties its
	// health breaks it: the body gone (no collision), the stump left, its pieces thrown out of where they stood, landing,
	// lying and shrinking away, their components given back to the pool and used again by the next break.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UBreakableDebrisSubsystem* Debris = UBreakableDebrisSubsystem::Get(World);
	ABreakableProp* Barrel = SpawnProp(World, EBreakableKind::Barrel, FVector(300.0, 0.0, 0.0));
	ABreakableProp* Crate = SpawnProp(World, EBreakableKind::SlattedCrate, FVector(300.0, 400.0, 0.0));
	if (!TestTrue(TEXT("Two props and the debris"), Barrel && Crate && Debris))
	{
		return false;
	}
	const FTransform Rest = Barrel->GetActorTransform();

	Hit(Barrel, 20.f);
	TestFalse(TEXT("A round doesn't break it"), Barrel->IsBroken());
	TestTrue(TEXT("...it rocks, ticking while it does"), Barrel->IsActorTickEnabled());
	Barrel->Advance(0.05f);
	TestTrue(FString::Printf(TEXT("...tipped away from the blow (%.1f degrees)"), Barrel->GetShudder()), Barrel->GetShudder() > 0.5f
		&& Barrel->GetActorUpVector().X > 0.0);
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Barrel->Advance(1.f / 60.f);
	}
	TestTrue(TEXT("...and back where it stood, still"), !Barrel->IsActorTickEnabled() && Barrel->GetShudder() == 0.f
		&& Barrel->GetActorTransform().Equals(Rest, 0.01));

	Hit(Barrel, Barrel->Health->GetHealth() + 1.f);
	TestTrue(TEXT("The blow that empties its health breaks it"), Barrel->IsBroken());
	TestTrue(TEXT("...its body gone, nothing to hit"), !Barrel->Body->IsVisible() && Barrel->Body->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestFalse(TEXT("...no longer an obstacle"), Barrel->ActorHasTag(TEXT("Obstacle")));
	if (Barrel->StumpMesh)
	{
		TestTrue(TEXT("...its stump left standing"), Barrel->Stump->IsVisible() && Barrel->Stump->GetStaticMesh() == Barrel->StumpMesh);
	}
	else
	{
		AddWarning(TEXT("Lootables.py's stumps aren't imported in this checkout: no stump to see."));
	}
	TestFalse(TEXT("A broken one doesn't break again"), Barrel->Break(nullptr, FVector::ForwardVector));

	const int32 Pieces = Barrel->PieceMeshes.Num();
	if (Pieces == 0)
	{
		AddWarning(TEXT("Lootables.py's pieces aren't imported in this checkout: no pieces thrown, the debris isn't checked."));
		return true;
	}
	TestEqual(TEXT("Its pieces thrown"), Debris->NumPieces(), Pieces);
	TestEqual(TEXT("...each on a pooled component"), Debris->NumComponents(), Pieces);
	RunDebris(Debris, 0.15f);
	TestEqual(TEXT("...flying, none down yet"), Debris->NumLying(), 0);
	RunDebris(Debris, 2.f);
	TestEqual(TEXT("...all down on the ground, lying"), Debris->NumLying(), Pieces);
	RunDebris(Debris, LooterBreakables::PieceLifeMax + LooterBreakables::ShrinkSeconds + 0.2f);
	TestEqual(TEXT("...shrunk away"), Debris->NumPieces(), 0);
	TestFalse(TEXT("...and the debris has nothing to tick"), Debris->IsTickable());

	Crate->Break(nullptr, FVector(0.0, 1.0, 0.0));
	TestEqual(TEXT("The next break's pieces"), Debris->NumPieces(), Crate->PieceMeshes.Num());
	TestTrue(TEXT("...on the same components where it can"), Debris->NumComponents() <= FMath::Max(Pieces, Crate->PieceMeshes.Num()));
	RunDebris(Debris, 0.2f);
	TestTrue(TEXT("...rising out of where it stood"), Debris->NumLying() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakablesPoolTest, "Looter.Loot.Breakables.Pool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBreakablesPoolTest::RunTest(const FString& Parameters)
{
	// However many break at once, the pieces out never pass the pool's cap: the oldest give theirs up.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UBreakableDebrisSubsystem* Debris = UBreakableDebrisSubsystem::Get(World);
	if (!TestNotNull(TEXT("The debris"), Debris))
	{
		return false;
	}
	TArray<ABreakableProp*> Props;
	for (int32 Index = 0; Index < 12; ++Index)
	{
		Props.Add(SpawnProp(World, Kinds[Index % 3], FVector(300.0 + 200.0 * (Index / 4), 200.0 * (Index % 4), 0.0)));
	}
	if (Props[0] && Props[0]->PieceMeshes.IsEmpty())
	{
		AddWarning(TEXT("Lootables.py's pieces aren't imported in this checkout: the pool isn't checked."));
		return true;
	}
	for (ABreakableProp* Prop : Props)
	{
		if (Prop)
		{
			Prop->Break(nullptr, FVector::ForwardVector);
			Debris->Advance(0.1f);
		}
	}
	TestTrue(FString::Printf(TEXT("12 breaks at once: %d pieces out, at most %d"), Debris->NumPieces(), UBreakableDebrisSubsystem::MaxPieces),
		Debris->NumPieces() <= UBreakableDebrisSubsystem::MaxPieces && Debris->NumComponents() <= UBreakableDebrisSubsystem::MaxPieces);
	RunDebris(Debris, 10.f);
	TestEqual(TEXT("...and all gone in a while"), Debris->NumPieces(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBreakablesSaveTest, "Looter.Loot.Breakables.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBreakablesSaveTest::RunTest(const FString& Parameters)
{
	// A broken prop stays broken: the session keeps it by its id with the map's world, through the save file, and the level
	// played again has it as a stump, giving nothing; one left whole stays whole.
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper AgainLevel;
	if (!TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview) && AgainLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	const FName BrokenId(TEXT("Breakable_Test_001"));
	const FName WholeId(TEXT("Breakable_Test_002"));
	UWorld* Played = PlayedLevel.GetTestWorld();
	UWorld* Again = AgainLevel.GetTestWorld();
	ABreakableProp* Broken = SpawnProp(Played, EBreakableKind::PackingCrate, FVector(300.0, 0.0, 0.0), BrokenId);
	ABreakableProp* Whole = SpawnProp(Played, EBreakableKind::Barrel, FVector(300.0, 300.0, 0.0), WholeId);
	if (!TestTrue(TEXT("The level's props"), Broken && Whole))
	{
		return false;
	}
	Hit(Broken, 500.f);
	TestTrue(TEXT("One broken"), Broken->IsBroken() && !Whole->IsBroken());

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Played, *Save);
	const FString PlayedMap = USessionSubsystem::MapOf(Played);
	const FSavedMapWorld* Kept = Save->FindWorld(PlayedMap);
	TestTrue(TEXT("The broken one is kept by its id, and only it"), Kept && Kept->BrokenProps.Num() == 1 && Kept->BrokenProps[0] == BrokenId);
	ULooterSessionSave* Back = WriteAndRead(Save);
	const FSavedMapWorld* Read = Back ? Back->FindWorld(PlayedMap) : nullptr;
	if (!TestTrue(TEXT("Through the save file"), Read && Read->BrokenProps.Contains(BrokenId)))
	{
		return false;
	}

	ABreakableProp* BrokenAgain = SpawnProp(Again, EBreakableKind::PackingCrate, FVector(300.0, 0.0, 0.0), BrokenId);
	ABreakableProp* WholeAgain = SpawnProp(Again, EBreakableKind::Barrel, FVector(300.0, 300.0, 0.0), WholeId);
	if (!TestTrue(TEXT("The level's props again, whole"), BrokenAgain && WholeAgain && !BrokenAgain->IsBroken()))
	{
		return false;
	}
	const FSavedMapWorld PlayedWorld = *Read;
	Back->FindOrAddWorld(USessionSubsystem::MapOf(Again)) = PlayedWorld;
	UBreakableDebrisSubsystem* Debris = UBreakableDebrisSubsystem::Get(Again);
	USessionSubsystem::RestoreWorld(Again, *Back);
	TestTrue(TEXT("The broken one is a stump again"), BrokenAgain->IsBroken() && !BrokenAgain->Body->IsVisible()
		&& BrokenAgain->Body->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("...quietly: no pieces thrown"), !Debris || Debris->NumPieces() == 0);
	TestTrue(TEXT("...and it gives nothing"), BrokenAgain->GetDroppedLoot().IsEmpty());
	TestFalse(TEXT("The other stays whole"), WholeAgain->IsBroken());
	BrokenAgain->Mend();
	TestTrue(TEXT("Mended (the console): whole and at full health"), !BrokenAgain->IsBroken() && BrokenAgain->Body->IsVisible()
		&& FMath::IsNearlyEqual(BrokenAgain->Health->GetHealth(), BrokenAgain->Health->GetMaxHealth()));
	return true;
}

#endif
