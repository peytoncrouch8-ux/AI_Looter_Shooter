#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerSlide.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// The game's pace (the user's call, 2026-10-13: "pace of play is super fast"): the player walks, sprints, crouches, slides
// and walks with the sights up 10% slower, as a factor of its own beside the player's size (Player/PlayerSize.h); the
// creatures whose chase was tuned against the player keep the same odds, the slimes their own pace.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerPaceTest, "Looter.Locomotion.Pace.Speeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerPaceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("The pace: 90%"), LooterPlayerSize::PaceScale, 0.9f);
	TestEqual(TEXT("Its own factor: the size and its speeds are as they were"), LooterPlayerSize::SpeedScale, LooterPlayerSize::Scale);
	TestEqual(TEXT("Every speed: the size's and the pace together"), LooterPlayerSize::MoveScale,
		LooterPlayerSize::SpeedScale * LooterPlayerSize::PaceScale, 1.e-6f);

	// The speeds that come of it (cm/s): a walk of 459, a sprint of 711.45, a slide of 782.6, a crouch of 229.5, each 10%
	// under what the size alone gave (510, 790.5, 869.55, 255).
	const UPlayerLocomotionComponent* Locomotion = GetDefault<UPlayerLocomotionComponent>();
	const float Walk = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::MoveScale;
	const float Sprint = Walk * Locomotion->SprintSpeedMultiplier;
	const float Slide = Sprint * FPlayerSlide::SpeedMultiplier;
	TestEqual(TEXT("Walking: 459"), Walk, 459.f, 0.01f);
	TestEqual(TEXT("Sprinting: 711.45"), Sprint, 711.45f, 0.01f);
	TestEqual(TEXT("Sliding: 782.6"), Slide, 782.595f, 0.01f);
	TestEqual(TEXT("Crouched: 229.5"), Locomotion->CrouchSpeed, 229.5f, 0.01f);
	TestEqual(TEXT("The walk, 10% under the size's own"), Walk / (LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale), 0.9f, 1.e-4f);
	TestEqual(TEXT("The crouch, 10% under the size's own"), Locomotion->CrouchSpeed / (LooterPlayerSize::FullSizeCrouchSpeed * LooterPlayerSize::SpeedScale),
		0.9f, 1.e-4f);

	// The character as the game makes it: walking at the pace (the sprint, the slide and the aim's walk are shares of it),
	// at the same size as before.
	const UClass* Player = LoadClass<ACharacter>(nullptr, TEXT("/Game/Player/BP_LooterCharacter.BP_LooterCharacter_C"));
	const ACharacter* Character = Player ? Player->GetDefaultObject<ACharacter>() : nullptr;
	if (TestNotNull(TEXT("The player character"), Character))
	{
		TestEqual(TEXT("It walks at the pace"), Character->GetCharacterMovement()->MaxWalkSpeed, Walk, 0.01f);
		TestTrue(TEXT("...at the size it had"), Character->GetCapsuleComponent()->GetRelativeScale3D().Equals(FVector(LooterPlayerSize::Scale), 1.e-4));
	}

	// Its run cycle tops out at the full-size jog at its size (510 cm/s): the slower walk is under it, so the blend spaces fit
	// the walk, and only a sprint plays the cycle faster (711.45 / 510, about 1.39).
	const float CycleTop = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::Scale;
	TestTrue(TEXT("The walk is under the run cycle's top"), Walk < CycleTop);
	TestEqual(TEXT("A sprint plays the cycle about 1.39 times as fast"), Sprint / CycleTop, 1.395f, 0.005f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerPaceChasesTest, "Looter.Locomotion.Pace.Chases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerPaceChasesTest::RunTest(const FString& Parameters)
{
	// Every chase tuned against the player's walk keeps its share of it, so a chase is as close as it was: each creature's
	// full-size tuning against the full-size 600 walk.
	const float Walk = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::MoveScale;
	struct FChase
	{
		const TCHAR* Who;
		float Speed;
		float FullSize;
	};
	const FChase Chases[] = {
		{ TEXT("A brown spider"), GetDefault<ASpiderCreature>()->ChaseSpeed, 540.f },
		{ TEXT("A venom spitter"), GetDefault<ASpitterSpider>()->ChaseSpeed, 480.f },
		{ TEXT("An Unpaid"), GetDefault<AUnpaidCreature>()->ChaseSpeed, 470.f },
		{ TEXT("A Bell-Ringer"), GetDefault<ABellRingerCreature>()->ChaseSpeed, 400.f },
		{ TEXT("The Gravemother"), GetDefault<AGravemotherCreature>()->ChaseSpeed, 480.f },
		{ TEXT("The Gravemother's plough"), GetDefault<AGravemotherCreature>()->Furrows.Speed, 900.f },
		{ TEXT("Abel"), GetDefault<AAbelKeeper>()->ChaseSpeed, 430.f },
	};
	for (const FChase& Chase : Chases)
	{
		TestEqual(FString::Printf(TEXT("%s chases at the same share of the player's walk"), Chase.Who), Chase.Speed / Walk,
			Chase.FullSize / LooterPlayerSize::FullSizeWalkSpeed, 1.e-4f);
	}
	// Its own chase is still just under the walk, but in play a regular kind chases Looter.Fight.ChaseScale faster (the fight
	// contract, 2026-10-21: Borderlands normal): a walking player is caught, a sprinting one gets away.
	const float Sprint = Walk * GetDefault<UPlayerLocomotionComponent>()->SprintSpeedMultiplier;
	const float SpiderInPlay = FightRules::ChaseSpeedFor(GetDefault<ASpiderCreature>()->ChaseSpeed, false, FFightTuning().ChaseScale);
	TestTrue(TEXT("A brown spider's own chase is just under the walk"), GetDefault<ASpiderCreature>()->ChaseSpeed < Walk);
	TestTrue(FString::Printf(TEXT("...in play it catches a walking player (%.0f against %.0f)"), SpiderInPlay, Walk), SpiderInPlay > Walk);
	TestTrue(FString::Printf(TEXT("...and a sprinting one gets away (%.0f)"), Sprint), SpiderInPlay < Sprint);

	// The slimes were never slowed with the player (the user's call, 2026-10-08), and the pace leaves them be too.
	TestEqual(TEXT("A slime keeps its own chase"), GetDefault<ASlimeCreature>()->ChaseSpeed, 220.f, 0.01f);
	return true;
}

#endif
