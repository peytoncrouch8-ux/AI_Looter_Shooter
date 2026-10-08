#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerSlide.h"
#include "Player/SlideDust.h"
#include "Player/StanceIntent.h"
#include "Player/ViewEase.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "UObject/UnrealType.h"

// The player's stance and slide rules and their settings. Played out on the real character in a test level:
// LocomotionPlayTests.cpp.

namespace
{
	using EStance = FStanceIntent::EStance;

	constexpr float Frame = 1.f / 60.f;

	FStanceIntent MakeIntent(bool bSprintToggle, bool bCrouchToggle)
	{
		FStanceIntent Intent;
		Intent.SetModes(bSprintToggle, bCrouchToggle);
		return Intent;
	}

	/** A component a Blueprint adds (its construction script's template), searched up the Blueprint parents. */
	template <typename T>
	const T* FindBlueprintComponent(const UClass* Class)
	{
		for (const UBlueprintGeneratedClass* Generated = Cast<UBlueprintGeneratedClass>(Class); Generated;
			Generated = Cast<UBlueprintGeneratedClass>(Generated->GetSuperClass()))
		{
			const USimpleConstructionScript* Script = Generated->SimpleConstructionScript;
			for (const USCS_Node* Node : Script ? Script->GetAllNodes() : TArray<USCS_Node*>())
			{
				if (const T* Template = Node ? Cast<T>(Node->ComponentTemplate) : nullptr)
				{
					return Template;
				}
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceHoldModeTest, "Looter.Locomotion.Stance.HoldMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceHoldModeTest::RunTest(const FString& Parameters)
{
	FStanceIntent Intent = MakeIntent(false, false);
	TestFalse(TEXT("Nothing wanted at rest"), Intent.WantsSprint() || Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Holding sprint sprints"), Intent.WantsSprint());
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("Letting go stops"), Intent.WantsSprint());

	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Holding crouch crouches"), Intent.WantsCrouch());
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Sprint pressed while crouching wins"), Intent.WantsSprint());
	TestFalse(TEXT("...and stands up"), Intent.WantsCrouch());
	Intent.Release(EStance::Sprint);
	TestTrue(TEXT("Crouch still held takes over again"), Intent.WantsCrouch());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("All released"), Intent.WantsSprint() || Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.CancelSprintToggle();
	TestTrue(TEXT("Shooting doesn't erase a held sprint key (it resumes after)"), Intent.WantsSprint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceToggleModeTest, "Looter.Locomotion.Stance.ToggleMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceToggleModeTest::RunTest(const FString& Parameters)
{
	FStanceIntent Intent = MakeIntent(true, true);

	Intent.Press(EStance::Sprint);
	Intent.Release(EStance::Sprint);
	TestTrue(TEXT("Toggle sprint stays on after release"), Intent.WantsSprint());
	Intent.Press(EStance::Sprint);
	TestFalse(TEXT("Second press turns it off"), Intent.WantsSprint());

	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	TestTrue(TEXT("Toggle crouch stays on"), Intent.WantsCrouch());
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Sprint from a toggled crouch"), Intent.WantsSprint());
	TestFalse(TEXT("clears the crouch"), Intent.WantsCrouch());
	Intent.CancelSprintToggle();
	TestFalse(TEXT("Stopping/shooting ends a toggled sprint"), Intent.WantsSprint());
	TestFalse(TEXT("and the old crouch doesn't come back"), Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Crouch from a toggled sprint"), Intent.WantsCrouch());
	TestFalse(TEXT("ends the sprint"), Intent.WantsSprint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceMixedModeTest, "Looter.Locomotion.Stance.MixedModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceMixedModeTest::RunTest(const FString& Parameters)
{
	// Toggle sprint, hold crouch.
	FStanceIntent Intent = MakeIntent(true, false);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Held crouch overrides toggled sprint"), Intent.WantsCrouch());
	TestFalse(TEXT("Sprint not wanted while crouch held"), Intent.WantsSprint());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("Toggled sprint was cleared by the crouch"), Intent.WantsSprint());

	// Hold sprint, toggle crouch.
	Intent = MakeIntent(false, true);
	Intent.Press(EStance::Crouch);
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Held sprint overrides toggled crouch"), Intent.WantsSprint());
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("Toggled crouch was cleared by the sprint"), Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.Reset();
	TestFalse(TEXT("Reset (lost control) clears everything"), Intent.WantsSprint() || Intent.WantsCrouch());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionDefaultsTest, "Looter.Locomotion.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLocomotionDefaultsTest::RunTest(const FString& Parameters)
{
	const UPlayerLocomotionComponent* Defaults = GetDefault<UPlayerLocomotionComponent>();
	TestTrue(TEXT("Sprint is faster than walking"), Defaults->SprintSpeedMultiplier > 1.f);
	TestTrue(TEXT("Crouch is slower than walking"), Defaults->CrouchSpeed < LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale);
	TestTrue(TEXT("Crouched capsule is shorter than standing (96)"), Defaults->CrouchedHalfHeight < 96.f);
	TestTrue(TEXT("Crouching tightens spread"), Defaults->CrouchSpreadMultiplier < 1.f);
	TestTrue(TEXT("Crouched head stays under the crouched capsule top"), Defaults->GetCrouchedHeadHeight() < Defaults->CrouchedHalfHeight * 2.f);
	TestEqual(TEXT("Standing spread is untouched"), Defaults->GetSpreadMultiplier(), 1.f);

	// The player jumps 15% higher than the engine's default jump (420 cm/s under 980 cm/s^2 gravity: 90 cm).
	const UClass* Player = LoadClass<ACharacter>(nullptr, TEXT("/Game/Player/BP_LooterCharacter.BP_LooterCharacter_C"));
	const ACharacter* Character = Player ? Player->GetDefaultObject<ACharacter>() : nullptr;
	if (TestNotNull(TEXT("Player character"), Character))
	{
		const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		const float Gravity = FMath::Abs(UPhysicsSettings::Get()->DefaultGravityZ) * Movement->GravityScale;
		TestEqual(TEXT("Jump height (cm)"), FMath::Square(Movement->JumpZVelocity) / (2.f * Gravity), 90.f * 1.15f, 0.5f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSizeTest, "Looter.Locomotion.PlayerSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerSizeTest::RunTest(const FString& Parameters)
{
	// The user's call (2026-10-07): the player 15% smaller and 15% slower, the two together so his strides still cover the
	// ground the animations' do.
	TestEqual(TEXT("Size and speed scale together"), LooterPlayerSize::SpeedScale, LooterPlayerSize::Scale);
	const UClass* Player = LoadClass<ACharacter>(nullptr, TEXT("/Game/Player/BP_LooterCharacter.BP_LooterCharacter_C"));
	const ACharacter* Character = Player ? Player->GetDefaultObject<ACharacter>() : nullptr;
	if (!TestNotNull(TEXT("Player character"), Character))
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestTrue(FString::Printf(TEXT("The whole character is scaled: its capsule, the root (%s)"), *Capsule->GetRelativeScale3D().ToCompactString()),
		Capsule->GetRelativeScale3D().Equals(FVector(LooterPlayerSize::Scale), 1.e-4));
	TestEqual(TEXT("...its standing half height at that size (landings, respawns)"), Character->GetDefaultHalfHeight(),
		Capsule->GetUnscaledCapsuleHalfHeight() * LooterPlayerSize::Scale, 0.01f);
	TestEqual(TEXT("...its eyes at that size"), Character->BaseEyeHeight, GetDefault<APawn>()->BaseEyeHeight * LooterPlayerSize::Scale, 0.01f);

	const float Walk = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale;
	TestEqual(TEXT("Walks at 85% of the full-size 600 cm/s"), Character->GetCharacterMovement()->MaxWalkSpeed, Walk, 0.01f);
	if (const UPlayerLocomotionComponent* Locomotion = FindBlueprintComponent<UPlayerLocomotionComponent>(Player);
		TestNotNull(TEXT("The Blueprint's locomotion component"), Locomotion))
	{
		TestEqual(TEXT("Sprints at 85% of the full-size 930 cm/s"), Walk * Locomotion->SprintSpeedMultiplier, 930.f * LooterPlayerSize::SpeedScale, 0.5f);
		TestEqual(TEXT("Crouches at 85% of the full-size 300 cm/s"), Locomotion->CrouchSpeed,
			LooterPlayerSize::FullSizeCrouchSpeed * LooterPlayerSize::SpeedScale, 0.01f);
	}

	// The body's Anim Blueprints hand their blend spaces the speed in the body's own size (ULooterCharacterAnimInstance),
	// through their ground speed variable: without it the smaller body's feet would slide.
	const TCHAR* AnimBlueprints[] = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/ABP_Rifle.ABP_Rifle_C"),
	};
	for (const TCHAR* Path : AnimBlueprints)
	{
		const UClass* Anim = LoadClass<UAnimInstance>(nullptr, Path);
		const FNumericProperty* Speed = Anim ? CastField<FNumericProperty>(Anim->FindPropertyByName(TEXT("GroundSpeed"))) : nullptr;
		TestTrue(FString::Printf(TEXT("%s has a float GroundSpeed"), Path), Speed && Speed->IsFloatingPoint());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceJumpStandsUpTest, "Looter.Locomotion.Stance.JumpStandsUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceJumpStandsUpTest::RunTest(const FString& Parameters)
{
	// Hold: jump stands up with the crouch key still down; letting go changes nothing, and the next press crouches.
	FStanceIntent Intent = MakeIntent(false, false);
	Intent.Press(EStance::Crouch);
	Intent.StandUp();
	TestFalse(TEXT("Hold: jump stands up with the crouch key still down"), Intent.WantsCrouch());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("...letting go of it leaves them standing"), Intent.WantsCrouch() || Intent.WantsSprint());
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("...and the next press crouches again"), Intent.WantsCrouch());

	// Toggle: jump undoes the toggle, and a single press of crouch crouches again.
	Intent = MakeIntent(false, true);
	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	Intent.StandUp();
	TestFalse(TEXT("Toggle: jump stands up"), Intent.WantsCrouch());
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("...one press of crouch crouches again"), Intent.WantsCrouch());

	// A sprint key held under the crouch takes over once up.
	Intent = MakeIntent(false, false);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	Intent.StandUp();
	TestTrue(TEXT("Standing up with sprint held sprints"), Intent.WantsSprint() && !Intent.WantsCrouch());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceResumeSprintTest, "Looter.Locomotion.Stance.ResumeSprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceResumeSprintTest::RunTest(const FString& Parameters)
{
	// The user's rule: a slide ending with forward held goes back into the sprint, as if its key were down.
	// Hold mode, Shift let go during the slide: the sprint stays on without it until the player stops (or shoots, or aims).
	FStanceIntent Intent = MakeIntent(false, false);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	Intent.Release(EStance::Sprint);
	Intent.ResumeSprint();
	TestTrue(TEXT("Hold, Shift let go: back into the sprint"), Intent.WantsSprint() && !Intent.WantsCrouch());
	Intent.Release(EStance::Sprint);
	TestTrue(TEXT("...which a missed-release check doesn't end (there's no key to let go of)"), Intent.WantsSprint());
	Intent.CancelSprintToggle();
	TestFalse(TEXT("...stopping ends it"), Intent.WantsSprint());

	// Hold mode, Shift still down: it stays a held key, so letting go of Shift ends it.
	Intent = MakeIntent(false, false);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	Intent.ResumeSprint();
	TestTrue(TEXT("Hold, Shift down: sprinting"), Intent.WantsSprint());
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("...until it's let go"), Intent.WantsSprint());

	// The sprint key pressed again takes it over.
	Intent = MakeIntent(false, false);
	Intent.ResumeSprint();
	Intent.Press(EStance::Sprint);
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("A press of Shift takes the kept sprint over, and its release ends it"), Intent.WantsSprint());

	// Toggle crouch: forward held clears the toggled crouch, and it doesn't come back once the sprint ends.
	Intent = MakeIntent(false, true);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	Intent.ResumeSprint();
	TestTrue(TEXT("Toggle crouch: out of it and sprinting"), Intent.WantsSprint() && !Intent.WantsCrouch());
	Intent.Release(EStance::Sprint);
	Intent.CancelSprintToggle();
	TestFalse(TEXT("...and the crouch doesn't come back after"), Intent.WantsCrouch());

	// Toggle sprint: on, as if toggled, until the player stops.
	Intent = MakeIntent(true, false);
	Intent.ResumeSprint();
	TestTrue(TEXT("Toggle sprint: on"), Intent.WantsSprint());
	Intent.CancelSprintToggle();
	TestFalse(TEXT("...until they stop"), Intent.WantsSprint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSlideRulesTest, "Looter.Locomotion.Slide.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerSlideRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A sprint on the ground slides"), FPlayerSlide::CanStart(true, true));
	TestFalse(TEXT("A crouch without a sprint doesn't"), FPlayerSlide::CanStart(false, true));
	TestFalse(TEXT("No slide in the air"), FPlayerSlide::CanStart(true, false));

	// Going in at the player's sprint, across the way they face: the slide follows the run.
	const float Sprint = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale * GetDefault<UPlayerLocomotionComponent>()->SprintSpeedMultiplier;
	const float Crouched = LooterPlayerSize::FullSizeCrouchSpeed * LooterPlayerSize::SpeedScale;
	const float Top = Sprint * FPlayerSlide::SpeedMultiplier;
	FPlayerSlide Slide;
	Slide.Start(FVector(0.0, Sprint, 0.0), FVector::ForwardVector, Top);
	TestEqual(TEXT("10% faster than the sprint going in"), Slide.GetStartSpeed(), Sprint * 1.1f, 0.01f);
	TestEqual(TEXT("...and holding it"), Slide.GetSpeed(), Sprint * 1.1f, 0.01f);
	TestTrue(TEXT("...along the run, not the facing"), Slide.GetDirection().Equals(FVector(0.0, 1.0, 0.0)));

	// Each way out (back into the sprint, on into the crouched walk, or to a stop): it ends on time, easing to the speed
	// the player goes on at, never under it, and with no sudden braking as the ease starts.
	struct FExit
	{
		const TCHAR* Name;
		float Speed;
	};
	const FExit Exits[] = { { TEXT("Into the sprint"), Sprint }, { TEXT("Into the crouched walk"), Crouched }, { TEXT("To a stop"), 0.f } };
	for (const FExit& Exit : Exits)
	{
		Slide.Start(FVector(Sprint, 0.0, 0.0), FVector::ForwardVector, Top);
		float Time = 0.f;
		float Lowest = Top;
		float FirstDrop = -1.f;
		while (Slide.Advance(Frame, Slide.GetSpeed(), true, Exit.Speed) && Time < 5.f)
		{
			Time += Frame;
			if (FirstDrop < 0.f && Slide.GetSpeed() < Top)
			{
				FirstDrop = Top - Slide.GetSpeed();
			}
			Lowest = FMath::Min(Lowest, Slide.GetSpeed());
		}
		Time += Frame;
		TestTrue(FString::Printf(TEXT("%s: ends on time (%.3f s)"), Exit.Name, Time), Slide.GetLastEnd() == FPlayerSlide::EEnd::Time
			&& Time >= FPlayerSlide::Duration - 0.001f && Time <= FPlayerSlide::Duration + Frame + 0.001f);
		TestTrue(FString::Printf(TEXT("%s: eases to %.0f cm/s (%.1f on its last frame)"), Exit.Name, Exit.Speed, Slide.GetSpeed()),
			FMath::IsNearlyEqual(Slide.GetSpeed(), Exit.Speed, (Top - Exit.Speed) * 0.02f + 0.5f));
		TestTrue(FString::Printf(TEXT("%s: never under it on the way (%.1f)"), Exit.Name, Lowest), Lowest >= Exit.Speed - 0.5f);
		// A straight-line ease would drop a twentieth of the way in its first frame; the smooth one, under 2%.
		TestTrue(FString::Printf(TEXT("%s: no sudden braking as it starts (%.1f cm/s in its first frame)"), Exit.Name, FirstDrop),
			FirstDrop >= 0.f && FirstDrop <= (Top - Exit.Speed) * 0.02f + 0.01f);
		TestFalse(FString::Printf(TEXT("%s: over"), Exit.Name), Slide.IsActive());
	}

	Slide.Start(FVector(Sprint, 0.0, 0.0), FVector::ForwardVector, Top);
	TestFalse(TEXT("Running into a wall ends it"), Slide.Advance(Frame, 20.f, true, Crouched));
	TestTrue(TEXT("...as stalled"), Slide.GetLastEnd() == FPlayerSlide::EEnd::Stalled);
	Slide.Start(FVector(Sprint, 0.0, 0.0), FVector::ForwardVector, Top);
	TestFalse(TEXT("Leaving the ground ends it"), Slide.Advance(Frame, Sprint * 1.1f, false, Crouched));
	TestTrue(TEXT("...as airborne"), Slide.GetLastEnd() == FPlayerSlide::EEnd::Airborne);
	Slide.Start(FVector(Sprint, 0.0, 0.0), FVector::ForwardVector, Top);
	Slide.Stop();
	TestTrue(TEXT("A jump stops it"), !Slide.IsActive() && Slide.GetLastEnd() == FPlayerSlide::EEnd::Cancelled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FViewEaseTest, "Looter.Locomotion.ViewEase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FViewEaseTest::RunTest(const FString& Parameters)
{
	// The curve the view's height, its roll and the stance blends ease on: it arrives on time and at rest, and however the
	// target changes, its value and speed carry on from where they were (no snap, no bounce).
	FViewEase Ease;
	Ease.Reset(0.f);
	Ease.SetTarget(10.f, 0.3f);
	float Peak = 0.f;
	float Highest = 0.f;
	// 19 frames: just past the 0.3 s, whatever the frame time's rounding.
	for (int32 Frames = 0; Frames < 19; ++Frames)
	{
		Ease.Advance(Frame);
		Peak = FMath::Max(Peak, Ease.GetVelocity());
		Highest = FMath::Max(Highest, Ease.GetValue());
	}
	TestTrue(FString::Printf(TEXT("Arrives on time (%.4f by 0.3 s)"), Ease.GetValue()), FMath::IsNearlyEqual(Ease.GetValue(), 10.f, 1.e-3f));
	TestTrue(TEXT("...at rest"), Ease.IsSettled() && FMath::IsNearlyZero(Ease.GetVelocity()));
	TestTrue(FString::Printf(TEXT("...never past it (%.4f at most)"), Highest), Highest <= 10.f + 1.e-3f);
	// A minimum-jerk move's top speed is 1.875 times its average.
	TestTrue(FString::Printf(TEXT("...at a top speed of %.1f a second"), Peak), FMath::IsNearlyEqual(Peak, 1.875f * 10.f / 0.3f, 1.f));

	// Turned back part-way: it sets off from where it was at the speed it had.
	Ease.Reset(0.f);
	Ease.SetTarget(10.f, 0.3f);
	for (int32 Frames = 0; Frames < 6; ++Frames)
	{
		Ease.Advance(Frame);
	}
	const float Value = Ease.GetValue();
	const float Speed = Ease.GetVelocity();
	const float Acceleration = Ease.GetAcceleration();
	Ease.SetTarget(0.f, 0.3f);
	TestTrue(TEXT("Turned back: the value carries on"), FMath::IsNearlyEqual(Ease.GetValue(), Value, 1.e-4f));
	TestTrue(TEXT("...and its speed"), FMath::IsNearlyEqual(Ease.GetVelocity(), Speed, 1.e-3f));
	TestTrue(TEXT("...and its acceleration"), FMath::IsNearlyEqual(Ease.GetAcceleration(), Acceleration, 1.e-2f));
	for (int32 Frames = 0; Frames < 60 && !Ease.IsSettled(); ++Frames)
	{
		Ease.Advance(Frame);
	}
	TestTrue(TEXT("...and arrives back at rest"), Ease.IsSettled() && FMath::IsNearlyZero(Ease.GetValue(), 1.e-3f));

	// Shifted: moved at once, same speed, same target.
	Ease.Reset(0.f);
	Ease.SetTarget(10.f, 0.3f);
	for (int32 Frames = 0; Frames < 6; ++Frames)
	{
		Ease.Advance(Frame);
	}
	const float Before = Ease.GetValue();
	const float SpeedBefore = Ease.GetVelocity();
	Ease.Shift(-4.f, 0.25f);
	TestTrue(TEXT("Shifted: moved by the jump"), FMath::IsNearlyEqual(Ease.GetValue(), Before - 4.f, 1.e-4f));
	TestTrue(TEXT("...at the same speed"), FMath::IsNearlyEqual(Ease.GetVelocity(), SpeedBefore, 1.e-3f));
	TestEqual(TEXT("...for the same target"), Ease.GetTarget(), 10.f);

	// A move that needs a harder turn than allowed is stretched to keep within it.
	Ease.Reset(0.f);
	Ease.SetTarget(57.f, 0.1f, 4500.f);
	TestTrue(FString::Printf(TEXT("Limited: stretched (%.3f s for a 0.1 s ask)"), Ease.GetDuration()), Ease.GetDuration() > 0.1f);
	float Hardest = 0.f;
	while (!Ease.IsSettled())
	{
		Ease.Advance(Frame / 4.f);
		Hardest = FMath::Max(Hardest, FMath::Abs(Ease.GetAcceleration()));
	}
	TestTrue(FString::Printf(TEXT("...and kept within it (%.0f at most)"), Hardest), Hardest <= 4500.f * 1.05f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlideDustGroundTest, "Looter.Locomotion.Slide.DustGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlideDustGroundTest::RunTest(const FString& Parameters)
{
	// What the ground's material (or mesh) is called says how much a slide throws up: nothing on water, a little dust and
	// no grit on wood, the most on dirt (and on ground no name speaks for).
	struct FNamed
	{
		const TCHAR* Name;
		ESlideGround Ground;
	};
	const FNamed Names[] = {
		{ TEXT("MI_Water"), ESlideGround::Water }, { TEXT("MI_Waterfall"), ESlideGround::Water },
		{ TEXT("SM_RansomsRest_Water"), ESlideGround::Water }, { TEXT("MI_WoodPlanks"), ESlideGround::Wood },
		{ TEXT("MI_LanternWood"), ESlideGround::Wood }, { TEXT("MI_SkyIslandGrass"), ESlideGround::Grass },
		{ TEXT("MI_Hay"), ESlideGround::Grass }, { TEXT("MI_RockGranite_Ransom"), ESlideGround::Stone },
		{ TEXT("MI_StoneWall"), ESlideGround::Stone }, { TEXT("MI_GroundDirt"), ESlideGround::Dirt },
		{ TEXT("MI_RansomsRestMacro"), ESlideGround::Dirt }, { TEXT("M_Terrain"), ESlideGround::Dirt },
		{ TEXT("M_World"), ESlideGround::None }, { TEXT("BasicShapeMaterial"), ESlideGround::None },
	};
	for (const FNamed& Named : Names)
	{
		TestTrue(FString::Printf(TEXT("%s is %d"), Named.Name, static_cast<int32>(Named.Ground)), FSlideDust::GroundFromName(Named.Name) == Named.Ground);
	}

	TestTrue(TEXT("Nothing on water"), FSlideDust::DustAmount(ESlideGround::Water) == 0.f && FSlideDust::GritAmount(ESlideGround::Water) == 0.f);
	TestTrue(TEXT("Nothing off the ground"), FSlideDust::DustAmount(ESlideGround::None) == 0.f && FSlideDust::GritAmount(ESlideGround::None) == 0.f);
	TestTrue(TEXT("A little dust on wood, no grit"), FSlideDust::DustAmount(ESlideGround::Wood) > 0.f
		&& FSlideDust::DustAmount(ESlideGround::Wood) < FSlideDust::DustAmount(ESlideGround::Dirt) * 0.5f && FSlideDust::GritAmount(ESlideGround::Wood) == 0.f);
	TestTrue(TEXT("The most dust on dirt"), FSlideDust::DustAmount(ESlideGround::Dirt) >= FSlideDust::DustAmount(ESlideGround::Stone)
		&& FSlideDust::DustAmount(ESlideGround::Dirt) >= FSlideDust::DustAmount(ESlideGround::Grass));
	TestTrue(TEXT("The most grit on stone"), FSlideDust::GritAmount(ESlideGround::Stone) >= FSlideDust::GritAmount(ESlideGround::Dirt));
	return true;
}

#endif
