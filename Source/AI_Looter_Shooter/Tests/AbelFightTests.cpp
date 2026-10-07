#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossSeal.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/UnpaidCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SitWithPa.h"
#include "Story/AbelOnBoard.h"
#include "Tests/AbelTestWorld.h"
#include "World/FallRecoveryTracker.h"
#include "World/KeeperLanternPost.h"
#include "World/PlayableBoundary.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"

// Abel's fight in play (Docs/Areas/RansomsRest.md, "The boss"): his phases and their moments, the lanterns, his fight
// starting over, the fog wall, the Gravewind and a fall off the deck, and his kneel and the scene after.

using namespace AbelTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelPhasesTest, "Looter.Bosses.Abel.Phases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelPhasesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	TArray<AKeeperLanternPost*> Posts;
	AAbelKeeper* Abel = SpawnAbel(World, false, &Posts);
	UEnemyProjectileSubsystem* Shots = World->GetSubsystem<UEnemyProjectileSubsystem>();
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel && Shots ? StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss)
	{
		return false;
	}
	UHealthComponent* Health = Boss->GetBossHealth();
	TestFalse(TEXT("His fight on, he can be hurt"), Health->bInvulnerable);
	TestEqual(TEXT("\"You brought them here\""), Boss->GetPhase(), 0);

	// The buckshot: a one-second lantern flare, then the pellets from the pump.
	const float FlareAt = RunUntil(Abel, 6.f, [Abel]() { return Abel->GetMove() == EAbelMove::Flare; });
	TestTrue(FString::Printf(TEXT("4 s in, his lantern flares (%.2f s)"), FlareAt), FlareAt >= 3.9f && FlareAt <= 4.2f);
	TestTrue(TEXT("...his flare pose"), Abel->GetPoseNow() == EAbelPose::Flare);
	Run(Abel, 0.5f);
	TestEqual(TEXT("Half a second into the flare, nothing has left the pump"), Shots->NumShotsFrom(Abel), 0);
	const float ShotAt = RunUntil(Abel, 1.f, [Abel]() { return Abel->GetMove() == EAbelMove::Fire; });
	TestTrue(TEXT("A second after the flare began, the shot"), ShotAt >= 0.f && ShotAt <= 0.6f);
	TestEqual(TEXT("...a cone of buckshot in flight"), Shots->NumShotsFrom(Abel), Abel->Buckshot.Pellets);
	TestTrue(TEXT("...his shot pose"), Abel->GetPoseNow() == EAbelPose::Fire);

	// Every 12 s he turns to the sunset for 3 s, his coal open (after a shot under way, if one is).
	const float GriefAt = RunUntil(Abel, 10.f, [Abel]() { return Abel->GetMove() == EAbelMove::Grieve; });
	TestTrue(TEXT("He grieves"), GriefAt >= 0.f);
	TestTrue(TEXT("...his coal open, turned to the sunset"), Abel->IsCoalOpen() && Abel->GetPoseNow() == EAbelPose::Sunset);
	Run(Abel, Abel->GrieveSeconds + 0.1f);
	TestFalse(TEXT("Three seconds on, guarded again"), Abel->IsCoalOpen());

	// Two Unpaid rise through the boards every 25 s, at most four.
	RunUntil(Abel, 30.f, [Boss]() { return Boss->NumAliveAdds() > 0; });
	const TArray<ACreatureBase*> Risen = Boss->GetAliveAdds();
	TestEqual(TEXT("Two Unpaid rise"), Risen.Num(), 2);
	TestTrue(TEXT("...fading in through the boards"), Risen.Num() > 0 && Cast<AUnpaidCreature>(Risen[0]) && Cast<AUnpaidCreature>(Risen[0])->GetPhase() > 0.5f);
	Run(Abel, 25.5f);
	TestEqual(TEXT("25 s on, two more"), Boss->NumAliveAdds(), 4);
	Run(Abel, 25.5f);
	TestEqual(TEXT("...and never more than four"), Boss->NumAliveAdds(), 4);

	// "The bell" at 60%: out over the canyon into the fog, out of reach, the lanterns dark, 8 Unpaid in two waves.
	HurtTo(Abel, AbelRules::BellShare);
	TestEqual(TEXT("At 60%: \"The bell\""), Boss->GetPhase(), 1);
	TestTrue(TEXT("...he drifts out, and can't be hurt"), Abel->GetMove() == EAbelMove::DriftOut && Boss->IsUntargetable());
	TestEqual(TEXT("...the three lanterns go dark"), Abel->NumLitLanterns(), 0);
	Run(Abel, Abel->DriftSeconds + 0.1f);
	TestTrue(TEXT("He hangs in the fog over the canyon"), Abel->GetMove() == EAbelMove::InFog
		&& FVector::Dist(Abel->GetActorLocation(), Abel->GetFogSpot()) < 120.0);
	const float InFog = Health->GetHealth();
	BossTestWorld::Hurt(Abel, 1000.f);
	TestEqual(TEXT("...where a shot takes nothing"), Health->GetHealth(), InFog);
	TestEqual(TEXT("The first wave rose (four more, ten at most)"), Boss->NumAliveAdds(), 8);
	Run(Abel, AbelRules::SecondWaveAfter);
	TestEqual(TEXT("...the second, to the fight's ten"), Boss->NumAliveAdds(), 10);

	// Each lantern relit drags him a third of the way back; all three, and he's on the deck, stunned, his coal open.
	const FVector Fog = Abel->GetActorLocation();
	Posts[0]->Relight(Player);
	Run(Abel, 2.f);
	const double Back = FVector::Dist2D(Fog, Abel->GetActorLocation());
	const double Whole = FVector::Dist2D(Abel->GetFogSpot(), Abel->GetHome().GetLocation());
	TestTrue(FString::Printf(TEXT("One relit: a third of the way back (%.0f of %.0f cm)"), Back, Whole), Back > Whole * 0.2 && Back < Whole * 0.45);
	TestTrue(TEXT("...still in the fog"), Abel->IsOutInFog() && Boss->IsUntargetable());
	Posts[1]->Relight(Player);
	Posts[2]->Relight(Player);
	TestTrue(TEXT("All three relit: dragged back"), Abel->GetMove() == EAbelMove::DragBack);
	Run(Abel, Abel->DragBackSeconds + 0.1f);
	TestTrue(TEXT("On the deck again, at his spot"), FVector::Dist2D(Abel->GetActorLocation(), Abel->GetHome().GetLocation()) < 50.0);
	TestTrue(TEXT("...hurtable, stunned, his coal open"), !Boss->IsUntargetable() && Abel->GetMove() == EAbelMove::Pulled && Abel->IsCoalOpen());
	RunUntil(Abel, Abel->StunSeconds + 1.f, [Abel]() { return Abel->GetMove() == EAbelMove::None; });

	// "Let me go" at 25%: the Gravewind; he walks off into it and the dark saint pulls him back; between pulls, faster.
	BossTestWorld::KillAdds(*Boss);
	HurtTo(Abel, AbelRules::WindShare);
	TestEqual(TEXT("At 25%: \"Let me go\""), Boss->GetPhase(), 2);
	TestTrue(TEXT("...the Gravewind blows"), Abel->IsWindBlowing());
	const float WalkAt = RunUntil(Abel, 8.f, [Abel]() { return Abel->GetMove() == EAbelMove::WalkOff; });
	TestTrue(TEXT("He walks off into the wind"), WalkAt >= 0.f && Abel->GetPoseNow() == EAbelPose::Sunset);
	const FVector Sunset = Abel->GetSunsetDirection();
	const float PullAt = RunUntil(Abel, Abel->WalkOffSeconds + 1.f, [Abel]() { return Abel->GetMove() == EAbelMove::Pulled; });
	TestTrue(TEXT("...and is pulled back"), PullAt >= 0.f);
	TestTrue(TEXT("...never past the open end"), FVector::DotProduct(Abel->GetActorLocation() - Abel->GetHome().GetLocation(), Sunset) < Abel->OpenEndDistance);
	TestTrue(TEXT("...stunned, his coal open"), Abel->IsCoalOpen());
	const float CooldownBefore = Abel->AttackCooldown;
	RunUntil(Abel, Abel->StunSeconds + 1.f, [Abel]() { return Abel->GetMove() == EAbelMove::None; });
	TestTrue(TEXT("Between pulls he fights faster"), Abel->IsHastened() && Abel->AttackCooldown < CooldownBefore && Abel->ChaseSpeed > 430.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelLanternsTest, "Looter.Bosses.Abel.Lanterns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelLanternsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AKeeperLanternPost* Post = SpawnPost(World, FVector::ZeroVector, false);
	AKeeperLanternPost* Keepers = SpawnPost(World, FVector(0.0, 1000.0, 0.0), true);
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(-300.0, 0.0, 120.0));
	UInteractionComponent* Interaction = Player ? NewObject<UInteractionComponent>(Player, TEXT("Interaction")) : nullptr;
	if (!Post || !Keepers || !Interaction)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Player->AddInstanceComponent(Interaction);
	Interaction->RegisterComponent();
	int32 Relit = 0;
	Post->OnRelit.AddLambda([&Relit](AKeeperLanternPost&) { ++Relit; });

	// Lit, there's nothing to do with it.
	TestFalse(TEXT("A lit lantern can't be used"), Post->GetInteractionOptions(*Interaction).CanUse());
	// Dark (Abel's bell): a hold of 1.5 s relights it; a tap doesn't.
	Post->SetDark(true);
	const FInteractionOptions Dark = Post->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("Dark: hold Interact 1.5 s to relight"), Dark.CanUse() && Dark.bHold && !Dark.bTap
		&& FMath::IsNearlyEqual(Dark.HoldSeconds, AbelRules::RelightSeconds) && Dark.HoldPrompt.ToString() == TEXT("Relight the lantern"));
	TestFalse(TEXT("...a tap does nothing"), Post->Interact(*Interaction, false));
	TestTrue(TEXT("...the hold relights it"), Post->Interact(*Interaction, true) && !Post->IsDark());
	TestEqual(TEXT("...and says so, once"), Relit, 1);
	TestFalse(TEXT("Lit again, holding does nothing more"), Post->Interact(*Interaction, true));
	TestEqual(TEXT("...still once"), Relit, 1);

	// The keeper's post: nothing to hang without the story (Ellis hasn't the lantern here); hung, it's dark until lit.
	TestFalse(TEXT("No lantern to hang outside Main 6"), Keepers->CanHang());
	TestFalse(TEXT("...nor on the other posts"), Post->CanHang());
	TestTrue(TEXT("Hung (as the story would)"), Keepers->Hang(Player, /*bForce*/ true) && Keepers->IsKeepersLanternHung());
	TestTrue(TEXT("...on the hook, dark"), Keepers->KeepersLantern->IsVisible() && !Keepers->IsKeepersLanternLit() && !Keepers->KeepersLight->IsVisible());
	const FVector UpHung = Keepers->KeepersLantern->GetUpVector();
	TestTrue(TEXT("...hanging straight"), UpHung.Equals(FVector::UpVector, 0.01));

	// Lit by Abel: its glass glows, its light on, leaning north-east (Main 7's way).
	Keepers->LightKeepersLantern();
	TestTrue(TEXT("Lit, with its light"), Keepers->IsKeepersLanternLit() && Keepers->KeepersLight->IsVisible());
	const FVector Lean = Keepers->GetLeanDirection();
	TestTrue(TEXT("It leans north-east"), Lean.Equals(FVector(UE_INV_SQRT_2, UE_INV_SQRT_2, 0.0), 0.001));
	const FVector Up = Keepers->KeepersLantern->GetUpVector();
	const double Tilt = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Up, FVector::UpVector)));
	TestTrue(FString::Printf(TEXT("...plainly, on its hook (%.1f degrees)"), Tilt), FMath::IsNearlyEqual(Tilt, static_cast<double>(Keepers->LeanDegrees), 1.0));
	TestTrue(TEXT("...its body drawn toward the bearing"), FVector::DotProduct((-Up).GetSafeNormal2D(), Lean) > 0.99);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelResetTest, "Looter.Bosses.Abel.Reset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelResetTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	TArray<AKeeperLanternPost*> Posts;
	AAbelKeeper* Abel = SpawnAbel(World, false, &Posts);
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel ? StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss)
	{
		return false;
	}
	// Into the second phase: out in the fog, the lanterns dark, adds up.
	HurtTo(Abel, AbelRules::BellShare);
	Run(Abel, 3.f);
	TestTrue(TEXT("In the fog, the lanterns dark, adds risen"), Abel->IsOutInFog() && Abel->NumLitLanterns() == 0 && Boss->NumAliveAdds() > 0);

	// The player falls: it all starts over.
	BossTestWorld::Kill(Player);
	UHealthComponent* Health = Boss->GetBossHealth();
	TestFalse(TEXT("The player's death starts the fight over"), Boss->IsFighting());
	TestTrue(TEXT("...he's home, healed, waiting and unhurt by anything"), FVector::Dist2D(Abel->GetActorLocation(), Abel->GetHome().GetLocation()) < 1.0
		&& FMath::IsNearlyEqual(Health->GetHealthPercent(), 1.f) && Abel->IsPassive() && Health->bInvulnerable);
	TestTrue(TEXT("...out of the fog, no moment under way"), !Abel->IsOutInFog() && Abel->GetMove() == EAbelMove::None);
	TestEqual(TEXT("...his adds gone"), Boss->NumAliveAdds(), 0);
	TestEqual(TEXT("...the lanterns burning"), Abel->NumLitLanterns(), 3);
	TestTrue(TEXT("...no wind, his own pace"), !Abel->IsWindBlowing() && !Abel->IsHastened());
	TestTrue(TEXT("...a player back on the deck starts it again"), FMath::IsNearlyEqual(Boss->EngageRadius, Abel->DeckRadius));

	// And it does start again, from the first phase.
	ACharacter* Again = BossTestWorld::SpawnPlayer(World, FVector(-600.0, 0.0, 120.0));
	Boss->StartFight(Again);
	TestTrue(TEXT("Again: the first phase, hurtable"), Boss->IsFighting() && Boss->GetPhase() == 0 && !Health->bInvulnerable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelSealTest, "Looter.Bosses.Abel.Seal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelSealTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	// The Keeper's Gate: a wall across the neck 15 m behind his spot, the deck ahead of it (+X).
	ABossSeal* Gate = World->SpawnActor<ABossSeal>(FVector(-1500.0, -1000.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Abel"), Abel) || !TestNotNull(TEXT("The gate's wall"), Gate))
	{
		return false;
	}
	Gate->Shape = EBossSealShape::Gate;
	Gate->GateEnd = FVector(0.0, 2000.0, 0.0);
	Abel->GetBoss()->Seal = Gate;
	ACharacter* Player = nullptr;
	UBossComponent* Boss = StartFight(*this, World, Abel, Player);
	if (!Boss)
	{
		return false;
	}
	TestTrue(TEXT("The player and Abel inside: the fog wall closes across the gate"), Gate->IsRaised());

	// Put back outside it (fall recovery's safe spot behind the gate), the player is shut out: after a moment, it starts over.
	Player->SetActorLocation(FVector(-1900.0, 0.0, 120.0));
	const float Over = RunUntil(Abel, 5.f, [Boss]() { return !Boss->IsFighting(); });
	TestTrue(FString::Printf(TEXT("Shut out, the fight starts over (%.1f s)"), Over), Over > 2.5f && Over < 4.f);
	TestFalse(TEXT("...and the wall drops"), Gate->IsRaised());

	// Won, it drops too.
	Player->SetActorLocation(FVector(-700.0, 0.0, 120.0));
	Boss->StartFight(Player);
	TestTrue(TEXT("Back in: closed again"), Gate->IsRaised());
	BossTestWorld::Kill(Abel);
	TestTrue(TEXT("He's beaten: the wall drops"), Boss->IsWon() && !Gate->IsRaised());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelGustTest, "Looter.Bosses.Abel.Gust",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelGustTest::RunTest(const FString& Parameters)
{
	// A gust off the deck: the deck's open end is its west edge (+X here), the player standing a meter from it as a gust
	// comes; carried over it, falling, the wind takes them down; fall recovery's outside rule brings them back to the deck
	// within a second of leaving it.
	const FPlayableBoundary Deck({ FVector2D(-1250.0, -900.0), FVector2D(1250.0, -900.0), FVector2D(1250.0, 900.0), FVector2D(-1250.0, 900.0) },
		{ false, true, false, false });
	const FAbelGustRules Rules;
	auto FallOff = [&Rules, &Deck](bool bDowndraft, float& OutSeconds, TOptional<EFallRecoveryReason>& OutReason, FFallRecoveryTracker& Tracker)
	{
		constexpr float Step = 1.f / 60.f;
		FFallRecoveryTracker::FRules Recovery;
		FVector Where(1150.0, 0.0, 90.0);
		double FallSpeed = 0.0;
		int32 LeftOn = INDEX_NONE;
		for (int32 Index = 1; Index <= 600; ++Index)
		{
			const float Time = Rules.FirstAfter + Index * Step;
			Where.X += Rules.Push * AbelRules::GustStrength(Rules, Time) * Step;
			const bool bOnDeck = Where.X <= 1250.0;
			const bool bOutside = !Deck.Contains(FVector2D(Where.X, Where.Y));
			if (!bOnDeck)
			{
				FallSpeed -= 980.0 * Step;
				if (bDowndraft && bOutside)
				{
					FallSpeed = AbelRules::FallSpeedPastEnd(Rules, FallSpeed);
				}
				Where.Z += FallSpeed * Step;
			}
			if (LeftOn == INDEX_NONE && bOutside)
			{
				LeftOn = Index;
			}
			OutReason = Tracker.Update(Step, Where, FRotator::ZeroRotator, bOnDeck, &Deck, Recovery);
			if (OutReason.IsSet())
			{
				OutSeconds = LeftOn == INDEX_NONE ? -1.f : (Index - LeftOn) * Step;
				return;
			}
		}
		OutSeconds = -1.f;
	};
	float Seconds = 0.f;
	TOptional<EFallRecoveryReason> Reason;
	FFallRecoveryTracker Tracker;
	FallOff(true, Seconds, Reason, Tracker);
	TestTrue(TEXT("A gust carries the player off the deck's open end, and they're brought back"), Reason.IsSet() && *Reason == EFallRecoveryReason::OffOpenEdge);
	TestTrue(FString::Printf(TEXT("...within a second of leaving it (%.2f s)"), Seconds), Seconds >= 0.f && Seconds <= 1.f);
	TestTrue(TEXT("...to a safe spot on the deck"), Deck.Contains(FVector2D(Tracker.GetSafeLocation().X, Tracker.GetSafeLocation().Y)));
	float Slower = 0.f;
	TOptional<EFallRecoveryReason> Plain;
	FFallRecoveryTracker Untouched;
	FallOff(false, Slower, Plain, Untouched);
	TestTrue(FString::Printf(TEXT("The downdraft makes it quicker (%.2f s against %.2f s)"), Seconds, Slower), Slower > Seconds);

	// In a test level: the wind's gust carries the player toward the open end; past it, a falling player is taken down.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	ACharacter* Player = nullptr;
	if (!Abel || !StartFight(*this, World, Abel, Player))
	{
		return false;
	}
	Abel->ForceGust();
	TestTrue(TEXT("The wind blows"), Abel->IsWindBlowing());
	const FVector Before = Player->GetActorLocation();
	for (int32 Step = 0; Step < 20; ++Step)
	{
		Abel->TickWind(Frame);
	}
	const FVector Carried = Player->GetActorLocation() - Before;
	TestTrue(FString::Printf(TEXT("A gust carries the player toward the open end (%s)"), *Carried.ToCompactString()), Carried.X > 100.0
		&& FMath::Abs(Carried.Y) < 1.0 && Carried.X < Rules.Push * 1.2);
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = FVector::ZeroVector;
	Abel->TickWind(Frame);
	TestEqual(TEXT("Over the deck, a fall is its own"), Movement->Velocity.Z, 0.0);
	Player->SetActorLocation(Abel->GetHome().GetLocation() + Abel->GetSunsetDirection() * (Abel->OpenEndDistance + 150.0));
	Abel->TickWind(Frame);
	TestTrue(TEXT("Past the open end, the wind takes them down"), Movement->Velocity.Z <= -Rules.Downdraft + 0.5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelKneelTest, "Looter.Bosses.Abel.Kneel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelKneelTest::RunTest(const FString& Parameters)
{
	// Played, skipped, and with scenes off: each ends the same, the lantern lit and leaning, Pa on his board.
	for (int32 Way = 0; Way < 3; ++Way)
	{
		const TCHAR* Name = Way == 0 ? TEXT("Played") : Way == 1 ? TEXT("Skipped") : TEXT("Scenes off");
		FScenesSetting Scenes(Way == 2 ? 0 : 2);
		FTestWorldWrapper WorldWrapper;
		if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return false;
		}
		UWorld* World = WorldWrapper.GetTestWorld();
		TArray<AKeeperLanternPost*> Posts;
		AAbelOnBoard* OnBoard = nullptr;
		AAbelKeeper* Abel = SpawnAbel(World, false, &Posts, &OnBoard);
		USceneSubsystem* Played = World->GetSubsystem<USceneSubsystem>();
		ACharacter* Player = nullptr;
		UBossComponent* Boss = Abel && Played ? StartFight(*this, World, Abel, Player) : nullptr;
		if (!Boss)
		{
			return false;
		}
		Run(Abel, 26.f);
		BossTestWorld::Kill(Abel);
		TestTrue(FString::Printf(TEXT("%s: at zero the fight is won, his adds gone"), Name), Boss->IsWon() && Boss->NumAliveAdds() == 0);
		TestTrue(FString::Printf(TEXT("%s: he kneels"), Name), Abel->GetMove() == EAbelMove::Kneel && Abel->GetPoseNow() == EAbelPose::Kneel);
		Run(Abel, 1.5f);
		const TArray<float>& Look = Abel->GetMesh()->GetCustomPrimitiveData().Data;
		TestTrue(FString::Printf(TEXT("%s: he doesn't fade; his coal sinks to an ember"), Name), Abel->GetPhase() == 0.f && Abel->IsPresent()
			&& Look.IsValidIndex(UnpaidLook::HeatIndex) && Look[UnpaidLook::HeatIndex] < -0.3f);
		AKeeperLanternPost* Keepers = Abel->GetKeepersPost();
		TestTrue(FString::Printf(TEXT("%s: the Keeper's Lantern not lit yet"), Name), Keepers && !Keepers->IsKeepersLanternLit());

		Run(Abel, 1.f);
		if (Way == 0)
		{
			TestTrue(TEXT("Played: the scene plays once his kneel settles"), Played->IsPlaying() && Played->GetPlayingName() == SitWithPa::SceneName());
			for (float Time = 0.f; Time < SitWithPa::Duration() + 1.f; Time += Frame)
			{
				Played->Tick(Frame);
				Abel->Tick(Frame);
				if (FMath::IsNearlyEqual(Time, 26.f, Frame * 0.5f))
				{
					TestFalse(TEXT("Played: before he raises his light, the lantern is dark"), Keepers->IsKeepersLanternLit());
				}
			}
		}
		else if (Way == 1)
		{
			TestTrue(TEXT("Skipped: the scene plays"), Played->IsPlaying());
			Played->SkipScene();
		}
		TestFalse(FString::Printf(TEXT("%s: the scene is over"), Name), Played->IsPlaying());
		TestTrue(FString::Printf(TEXT("%s: Main 6 hears it played"), Name), Played->HasPlayed(SitWithPa::SceneName()));
		TestTrue(FString::Printf(TEXT("%s: the Keeper's Lantern hangs lit, leaning north-east"), Name), Keepers->IsKeepersLanternHung()
			&& Keepers->IsKeepersLanternLit() && Keepers->GetLeanDirection().X > 0.6 && Keepers->GetLeanDirection().Y > 0.6);
		TestTrue(FString::Printf(TEXT("%s: Pa sits on his board"), Name), OnBoard->IsShown() && Abel->IsEndingDone() && !Abel->IsPresent()
			&& Abel->IsHidden());
		if (OnBoard->Figure->GetSkinnedAsset())
		{
			TestTrue(FString::Printf(TEXT("%s: ...in the sit"), Name), OnBoard->IsSeated());
		}
	}
	return true;
}

#endif
