#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/Animation/LooterStancePose.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerFlaskMotion.h"
#include "Player/PlayerFlaskRules.h"
#include "Player/PlayerFlaskSave.h"
#include "Session/SessionSave.h"
#include "Settings/KeyBindingSubsystem.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UnrealType.h"

// The soul flask's rules with no world (Docs/Archive/Work/2026-10-30-SoulFlask/CONTRACT.md): the numbers, the soul a kill gives,
// the heal's curve, pouring soul in, the gate, the motion (the flask, the gun's dip, the hand round it, the third-person
// arm), the key and the save's struct. The drink in a test level is PlayerFlaskPlayTests.cpp.

namespace
{
	/** The most a hand or the flask may move in one 2 ms step of the drink (cm): more reads as a jump. */
	constexpr float MaxStep = 6.f;
	constexpr int32 Steps = 550;

	/** The view's edges at 90 degrees across and 16:9: a point (X, Y, Z) shows where |Y| < X and |Z| < ViewHalfHeight X. */
	constexpr float ViewHalfHeight = 0.5625f;

	/** Where the drawn flask shows on screen, over its points that show: its highest and its rightmost (Z / X, Y / X). */
	struct FFlaskOnScreen
	{
		bool bShows = false;
		float Top = -10.f;
		float Right = -10.f;
	};

	/**
	 * The drawn flask at Pose, measured along its axis with its outline's radius there (the foot's rim, the belly as wide as
	 * its widest, the neck), up, down and to either side of the axis on screen.
	 */
	FFlaskOnScreen MeasureOnScreen(const FThrowPose& Pose)
	{
		using namespace FlaskMotion;
		const FVector Up = Pose.Rotation.Quaternion().GetUpVector();
		const FVector Lift = FVector::VectorPlaneProject(FVector::UpVector, Up).GetSafeNormal();
		const FVector Side = FVector::VectorPlaneProject(FVector::RightVector, Up).GetSafeNormal();
		FFlaskOnScreen Out;
		constexpr int32 Slices = 20;
		for (int32 Slice = 0; Slice <= Slices; ++Slice)
		{
			const float Share = static_cast<float>(Slice) / Slices;
			const float Reach = Share < 0.05f ? HeldFootRadius : (Share < 0.65f ? HeldRadius : HeldRadius * 0.3f);
			const FVector Axis = Pose.Offset + Up * (HeldMouthUp * Share);
			for (const FVector& Edge : { Axis + Lift * Reach, Axis - Lift * Reach, Axis + Side * Reach, Axis - Side * Reach })
			{
				if (Edge.X <= 1.0)
				{
					continue;
				}
				const float Height = static_cast<float>(Edge.Z / Edge.X);
				const float Across = static_cast<float>(Edge.Y / Edge.X);
				if (Height < -ViewHalfHeight || Height > ViewHalfHeight || FMath::Abs(Across) > 1.f)
				{
					continue;
				}
				Out.bShows = true;
				Out.Top = FMath::Max(Out.Top, Height);
				Out.Right = FMath::Max(Out.Right, Across);
			}
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskRulesTest, "Looter.Flask.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskRulesTest::RunTest(const FString& Parameters)
{
	// The contract's numbers.
	TestEqual(TEXT("Three charges"), FFlaskRules::MaxCharges, 3);
	TestEqual(TEXT("100 soul a charge"), FFlaskRules::SoulPerCharge, 100.f);
	TestTrue(TEXT("A 1.1 s drink, the heal from 0.35 s over 0.8 s, 35% of the most"), FMath::IsNearlyEqual(FFlaskRules::DrinkSeconds, 1.1f)
		&& FMath::IsNearlyEqual(FFlaskRules::SipAt, 0.35f) && FMath::IsNearlyEqual(FFlaskRules::HealSeconds, 0.8f)
		&& FMath::IsNearlyEqual(FFlaskRules::HealShare, 0.35f));
	TestTrue(TEXT("0.4 s between drinks, 0.75 speed, 34 soul a mote, 25 at least in a boss fight"), FMath::IsNearlyEqual(FFlaskRules::RepeatSeconds, 0.4f)
		&& FMath::IsNearlyEqual(FFlaskRules::MoveScale, 0.75f) && FMath::IsNearlyEqual(FFlaskRules::MoteSoul, 34.f)
		&& FMath::IsNearlyEqual(FFlaskRules::BossFightKillSoul, 25.f));

	// The soul a kill gives, by rank; a boss the whole flask; a boss fight's floor.
	const TPair<ECreatureRank, float> Table[] = { { ECreatureRank::Basic, 15.f }, { ECreatureRank::Rare, 30.f }, { ECreatureRank::Epic, 45.f },
		{ ECreatureRank::Hallowed, 60.f }, { ECreatureRank::Ethereal, 60.f }, { ECreatureRank::Legendary, 100.f }, { ECreatureRank::Boss, 300.f } };
	for (const TPair<ECreatureRank, float>& Row : Table)
	{
		TestEqual(FString::Printf(TEXT("Rank %d's kill"), static_cast<int32>(Row.Key)), FFlaskRules::KillSoul(Row.Key, false, false), Row.Value);
	}
	TestEqual(TEXT("A boss (its component) fills all three"), FFlaskRules::KillSoul(ECreatureRank::Basic, true, false), 300.f);
	TestEqual(TEXT("A Basic helper in a boss fight: 25"), FFlaskRules::KillSoul(ECreatureRank::Basic, false, true), 25.f);
	TestEqual(TEXT("A Gravebound helper keeps its own 45"), FFlaskRules::KillSoul(ECreatureRank::Epic, false, true), 45.f);
	TestEqual(TEXT("The boss itself, in its fight: 300"), FFlaskRules::KillSoul(ECreatureRank::Basic, true, true), 300.f);

	// The heal's curve: nothing before the sip, all of it by the window's end, smooth and rising between.
	TestEqual(TEXT("No heal at the press"), FFlaskRules::HealShareAt(0.f), 0.f);
	TestEqual(TEXT("...nor just before the sip"), FFlaskRules::HealShareAt(FFlaskRules::SipAt - 0.01f), 0.f);
	TestEqual(TEXT("All of it by SipAt + HealSeconds"), FFlaskRules::HealShareAt(FFlaskRules::SipAt + FFlaskRules::HealSeconds), 1.f);
	TestEqual(TEXT("Half at the window's middle"), FFlaskRules::HealShareAt(FFlaskRules::SipAt + FFlaskRules::HealSeconds * 0.5f), 0.5f, 0.001f);
	bool bRising = true;
	float Biggest = 0.f;
	float Last = 0.f;
	for (int32 Step = 1; Step <= 200; ++Step)
	{
		const float Now = FFlaskRules::HealShareAt(1.3f * Step / 200.f);
		bRising &= Now >= Last;
		Biggest = FMath::Max(Biggest, Now - Last);
		Last = Now;
	}
	TestTrue(TEXT("It only rises"), bRising);
	TestTrue(FString::Printf(TEXT("A gulp, not a switch (%.3f at most a 6.5 ms step)"), Biggest), Biggest < 0.02f);

	// Pouring soul in: overflow carries on, a full flask keeps none, several charges at once.
	int32 Charges = 0;
	float Soul = 90.f;
	TestEqual(TEXT("90 + 45: one charge filled"), FFlaskRules::Pour(Charges, Soul, 45.f), 1);
	TestTrue(TEXT("...35 carried into the next"), Charges == 1 && FMath::IsNearlyEqual(Soul, 35.f));
	TestEqual(TEXT("A boss's 300 from there: two more, the rest let go"), FFlaskRules::Pour(Charges, Soul, 300.f), 2);
	TestTrue(TEXT("...full, nothing kept"), Charges == 3 && Soul == 0.f);
	TestEqual(TEXT("Full: none filled"), FFlaskRules::Pour(Charges, Soul, 60.f), 0);
	TestTrue(TEXT("...and still none kept"), Charges == 3 && Soul == 0.f);
	Charges = 2;
	Soul = 0.f;
	TestEqual(TEXT("Under a charge's worth: none filled"), FFlaskRules::Pour(Charges, Soul, 34.f), 0);
	TestTrue(TEXT("...the soul kept"), Charges == 2 && FMath::IsNearlyEqual(Soul, 34.f));
	TestEqual(TEXT("Nothing poured, nothing changed"), FFlaskRules::Pour(Charges, Soul, -5.f), 0);
	TestTrue(TEXT("...as it was"), Charges == 2 && FMath::IsNearlyEqual(Soul, 34.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskGateTest, "Looter.Flask.Gate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskGateTest::RunTest(const FString& Parameters)
{
	FFlaskGateInput Ready;
	Ready.Now = 100.0;
	Ready.Charges = 2;
	TestTrue(TEXT("Hurt with a charge: it drinks"), FFlaskRules::WhyBlocked(Ready) == EFlaskBlock::None);

	auto Check = [this, &Ready](const TCHAR* What, TFunctionRef<void(FFlaskGateInput&)> Change, EFlaskBlock Expected)
	{
		FFlaskGateInput In = Ready;
		Change(In);
		TestTrue(What, FFlaskRules::WhyBlocked(In) == Expected);
	};
	Check(TEXT("No charge: empty"), [](FFlaskGateInput& In) { In.Charges = 0; }, EFlaskBlock::Empty);
	Check(TEXT("Health full"), [](FFlaskGateInput& In) { In.bFullHealth = true; }, EFlaskBlock::FullHealth);
	Check(TEXT("Empty is told before full health"), [](FFlaskGateInput& In) { In.Charges = 0; In.bFullHealth = true; }, EFlaskBlock::Empty);
	Check(TEXT("Already drinking"), [](FFlaskGateInput& In) { In.bDrinking = true; }, EFlaskBlock::Busy);
	Check(TEXT("Within 0.4 s of the last drink's end"), [](FFlaskGateInput& In) { In.LastDrinkEnd = In.Now - 0.3; }, EFlaskBlock::Busy);
	Check(TEXT("...free 0.4 s after"), [](FFlaskGateInput& In) { In.LastDrinkEnd = In.Now - 0.41; }, EFlaskBlock::None);
	Check(TEXT("The hands busy (a throw, a strike, a cast, a swim)"), [](FFlaskGateInput& In) { In.bHandsBusy = true; }, EFlaskBlock::Busy);
	Check(TEXT("Busy is quiet, before empty"), [](FFlaskGateInput& In) { In.bHandsBusy = true; In.Charges = 0; }, EFlaskBlock::Busy);
	Check(TEXT("Dead"), [](FFlaskGateInput& In) { In.bAlive = false; In.bMenuOpen = true; }, EFlaskBlock::Dead);
	Check(TEXT("In a scene"), [](FFlaskGateInput& In) { In.bInScene = true; In.bMenuOpen = true; }, EFlaskBlock::Scene);
	Check(TEXT("Behind a menu"), [](FFlaskGateInput& In) { In.bMenuOpen = true; In.bTraversing = true; }, EFlaskBlock::Menu);
	Check(TEXT("Mid-mantle or vault"), [](FFlaskGateInput& In) { In.bTraversing = true; In.bDrinking = true; }, EFlaskBlock::Moving);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskMotionTest, "Looter.Flask.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskMotionTest::RunTest(const FString& Parameters)
{
	using namespace FlaskMotion;
	bool bShown = true;
	FlaskPose(0.f, bShown);
	TestFalse(TEXT("Not shown before the press"), bShown);
	FlaskPose(LowerSeconds, bShown);
	TestFalse(TEXT("Put away by LowerSeconds"), bShown);
	const FThrowPose AtSip = FlaskPose(FFlaskRules::SipAt, bShown);
	TestTrue(TEXT("In the hand at the sip"), bShown);
	// At the sip (the 2026-10-30 play-check: it filled 40% of the view over the crosshair, its neck at the eye): the neck runs
	// down and back out of the bottom of the view toward the mouth, the foot tilted up and away, the flask low and left.
	const FVector Mouth = MouthOf(AtSip);
	const FVector Neck = AtSip.Rotation.Quaternion().GetUpVector();
	TestTrue(FString::Printf(TEXT("Its mouth under the bottom of the view (%s)"), *Mouth.ToCompactString()), Mouth.Z < -ViewHalfHeight * Mouth.X);
	TestTrue(TEXT("...its neck pointing down and back toward the mouth"), Neck.X < -0.3 && Neck.Z < -0.3);
	TestTrue(TEXT("...its foot up and away from the mouth"), AtSip.Offset.X > Mouth.X && AtSip.Offset.Z > Mouth.Z);
	TestTrue(TEXT("Its start is out of sight, low under the view"), MouthOf(StartPose()).Z < -ViewHalfHeight * MouthOf(StartPose()).X);
	TestTrue(TEXT("Tipped up through the pour: its foot rises above its mouth"), TippedPose().Offset.Z > MouthOf(TippedPose()).Z);
	TestTrue(TEXT("Drawn small (the view's share of its size)"), ViewScale > 0.3f && ViewScale < 1.f);

	// Through the whole drink: never above the bottom fifth of the view (its top under -0.32 X: at most about a fifth of the
	// view's height shows), never right of the left third's edge, so never over the crosshair or the middle; and it shows.
	float Highest = -10.f;
	float Rightmost = -10.f;
	bool bSeen = false;
	for (int32 Step = 1; Step < Steps; ++Step)
	{
		const FFlaskOnScreen Screen = MeasureOnScreen(FlaskPose(FFlaskRules::DrinkSeconds * Step / Steps, bShown));
		if (bShown && Screen.bShows)
		{
			bSeen = true;
			Highest = FMath::Max(Highest, Screen.Top);
			Rightmost = FMath::Max(Rightmost, Screen.Right);
		}
	}
	const FFlaskOnScreen AtSipScreen = MeasureOnScreen(AtSip);
	TestTrue(TEXT("Seen at the sip"), AtSipScreen.bShows);
	TestTrue(TEXT("Seen through the drink"), bSeen);
	TestTrue(FString::Printf(TEXT("A fifth of the view at most (its top at %.3f X; %.0f%% of the height shows)"), Highest,
		(ViewHalfHeight + Highest) / (2.f * ViewHalfHeight) * 100.f), Highest < -0.32f);
	TestTrue(FString::Printf(TEXT("Left of the middle third, never at the crosshair (its right edge at %.3f X)"), Rightmost), Rightmost < -0.3f);

	// The gun: at rest before and after, all the way down while the flask is up.
	TestEqual(TEXT("The gun at rest at the press"), GunDipShare(0.f), 0.f);
	TestEqual(TEXT("...and at the end"), GunDipShare(FFlaskRules::DrinkSeconds), 0.f);
	TestEqual(TEXT("All the way down at the sip"), GunDipShare(FFlaskRules::SipAt), 1.f);
	TestTrue(TEXT("Its deepest dip is the throw's"), GunPose(0.5f).Offset.Equals(ThrowMotion::GunDipPose().Offset, 0.01));

	// The flask and the hand never jump, and the hand is round the flask while it's up, on the gun before and after.
	const FHandPlacement OnGun = FHandPlacement::Make(FVector(40.f, -4.f, -14.f), FVector::ForwardVector, FVector::RightVector,
		FHandFingers::Of(EHandGrip::Forend));
	TestTrue(TEXT("The hand on the gun at the press"), FirstPersonHandsMotion::SupportInDrink(OnGun, 0.f).Hold.Equals(OnGun.Hold, 0.01));
	TestTrue(TEXT("...and at the end"), FirstPersonHandsMotion::SupportInDrink(OnGun, FFlaskRules::DrinkSeconds).Hold.Equals(OnGun.Hold, 0.01));
	for (const float Time : { RaiseSeconds, FFlaskRules::SipAt, 0.6f, TipEndSeconds })
	{
		const FThrowPose Pose = FlaskPose(Time, bShown);
		TestTrue(FString::Printf(TEXT("Round the flask %.2f s in"), Time),
			FirstPersonHandsMotion::SupportInDrink(OnGun, Time).Hold.Equals(FirstPersonHandsMotion::OnFlask(Time).Hold, 0.01)
			&& FVector::Dist(FirstPersonHandsMotion::OnFlask(Time).Hold, MiddleOf(Pose)) < HeldRadius);
		// Cradled from below: the palm faces up into the belly, the knuckles run on round its right, the wrist to the left.
		const FHandPlacement OnFlask = FirstPersonHandsMotion::OnFlask(Time);
		TestTrue(FString::Printf(TEXT("...from below, %.2f s in"), Time), OnFlask.GetPalmDir().Z > 0.3 && OnFlask.GetFingersDir().Y > 0.3
			&& OnFlask.Hold.Z < MiddleOf(Pose).Z);
	}
	float HandStep = 0.f;
	float FlaskStep = 0.f;
	FVector LastHand = FirstPersonHandsMotion::SupportInDrink(OnGun, 0.f).Hold;
	FVector LastFlask = FlaskPose(0.f, bShown).Offset;
	for (int32 Step = 1; Step <= Steps; ++Step)
	{
		const float Time = FFlaskRules::DrinkSeconds * Step / Steps;
		const FVector Hand = FirstPersonHandsMotion::SupportInDrink(OnGun, Time).Hold;
		const FVector Flask = FlaskPose(Time, bShown).Offset;
		HandStep = FMath::Max(HandStep, static_cast<float>(FVector::Dist(Hand, LastHand)));
		FlaskStep = FMath::Max(FlaskStep, static_cast<float>(FVector::Dist(Flask, LastFlask)));
		LastHand = Hand;
		LastFlask = Flask;
	}
	TestTrue(FString::Printf(TEXT("The hand never jumps (%.2f cm at most per 2 ms)"), HandStep), HandStep <= MaxStep);
	TestTrue(FString::Printf(TEXT("Nor does the flask (%.2f cm)"), FlaskStep), FlaskStep <= MaxStep);

	// The third-person arm on the same clock: nothing outside the drink, the arm on it while the flask is up.
	TestFalse(TEXT("Third person: no drink at 0"), LooterStancePose::DescribeDrink(0.f).IsActive());
	TestFalse(TEXT("...nor at the end"), LooterStancePose::DescribeDrink(FFlaskRules::DrinkSeconds).IsActive());
	const FLooterThrowStance Mid = LooterStancePose::DescribeDrink(0.6f);
	TestTrue(TEXT("...the arm up and the gun dipped mid-drink"), Mid.Arm > 0.99f && Mid.GunDip > 0.99f);
	TestTrue(TEXT("...the hand by the mouth (up from the shoulder, toward the middle)"), Mid.Hand.Z > 0.f && Mid.Hand.Y < 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlaskKeyAndSaveTest, "Looter.Flask.KeyAndSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlaskKeyAndSaveTest::RunTest(const FString& Parameters)
{
	// The key: "Flask", H, left on the D-pad, clear of every other pad button the game uses in play.
	TestTrue(TEXT("The binding is \"Flask\""), UKeyBindingSubsystem::FlaskBindingId() == FName(TEXT("Flask")));
	TestTrue(TEXT("On H"), UKeyBindingSubsystem::DefaultFlaskKey() == EKeys::H);
	TestTrue(TEXT("...and left on the D-pad"), UKeyBindingSubsystem::DefaultFlaskGamepadKey() == EKeys::Gamepad_DPad_Left);
	for (const FKey& Taken : { UKeyBindingSubsystem::DefaultGrenadeKey(), UKeyBindingSubsystem::DefaultMeleeKey(), UKeyBindingSubsystem::DefaultEmberKey(),
		UKeyBindingSubsystem::DefaultMapKey(), UKeyBindingSubsystem::DefaultDropWeaponKey(), FKey(EKeys::E), FKey(EKeys::R), FKey(EKeys::F5) })
	{
		TestTrue(FString::Printf(TEXT("H isn't %s"), *Taken.ToString()), UKeyBindingSubsystem::DefaultFlaskKey() != Taken);
	}
	for (const FKey& Taken : { UKeyBindingSubsystem::DefaultGrenadeGamepadKey(), UKeyBindingSubsystem::DefaultMeleeGamepadKey(),
		UKeyBindingSubsystem::DefaultEmberGamepadKey(), UKeyBindingSubsystem::DefaultMapGamepadKey(), FKey(EKeys::Gamepad_DPad_Up),
		FKey(EKeys::Gamepad_Special_Left), FKey(EKeys::Gamepad_Special_Right), FKey(EKeys::Gamepad_LeftThumbstick), FKey(EKeys::Gamepad_LeftTrigger),
		FKey(EKeys::Gamepad_RightTrigger), FKey(EKeys::Gamepad_FaceButton_Bottom), FKey(EKeys::Gamepad_FaceButton_Right),
		FKey(EKeys::Gamepad_FaceButton_Left), FKey(EKeys::Gamepad_FaceButton_Top) })
	{
		TestTrue(FString::Printf(TEXT("The pad's flask button isn't %s"), *Taken.ToString()), UKeyBindingSubsystem::DefaultFlaskGamepadKey() != Taken);
	}
	UInputMappingContext* Context = NewObject<UInputMappingContext>(NewObject<ULooterKeyBindingsSave>());
	const UInputAction* Action = UKeyBindingSubsystem::AddFlaskAction(Context, *Context);
	if (TestNotNull(TEXT("The flask action"), Action))
	{
		for (const FKey& Key : { UKeyBindingSubsystem::DefaultFlaskKey(), UKeyBindingSubsystem::DefaultFlaskGamepadKey() })
		{
			TestTrue(FString::Printf(TEXT("Mapped to %s"), *Key.ToString()), Context->GetMappings().ContainsByPredicate([Action, &Key](const FEnhancedActionKeyMapping& Mapping)
			{
				return Mapping.Action == Action && Mapping.Key == Key;
			}));
		}
	}

	// The save: three saved fields that come back as they went; a fresh one (a save from before the flask) reads full; the
	// session keeps one.
	const UScriptStruct* Struct = FFlaskSave::StaticStruct();
	TestNotNull(TEXT("The charges are saved"), FindFProperty<FIntProperty>(Struct, TEXT("Charges")));
	TestNotNull(TEXT("The soul is saved"), FindFProperty<FFloatProperty>(Struct, TEXT("Soul")));
	TestNotNull(TEXT("Whether it was saved is saved"), FindFProperty<FBoolProperty>(Struct, TEXT("bSaved")));
	TestNotNull(TEXT("The session keeps the flask"), FindFProperty<FStructProperty>(ULooterSessionSave::StaticClass(), TEXT("Flask")));
	FFlaskSave Saved;
	Saved.Charges = 1;
	Saved.Soul = 40.f;
	Saved.bSaved = true;
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	FObjectAndNameAsStringProxyArchive WriteProxy(Writer, false);
	FFlaskSave::StaticStruct()->SerializeItem(WriteProxy, &Saved, nullptr);
	FFlaskSave Read;
	FMemoryReader Reader(Bytes, true);
	FObjectAndNameAsStringProxyArchive ReadProxy(Reader, true);
	FFlaskSave::StaticStruct()->SerializeItem(ReadProxy, &Read, nullptr);
	TestTrue(TEXT("Read back as saved"), Read.Charges == 1 && FMath::IsNearlyEqual(Read.Soul, 40.f) && Read.bSaved);
	const FFlaskSave Fresh;
	TestTrue(TEXT("A save from before the flask: not saved, full"), !Fresh.bSaved && Fresh.Charges == FFlaskRules::MaxCharges && Fresh.Soul == 0.f);
	return true;
}

#endif
