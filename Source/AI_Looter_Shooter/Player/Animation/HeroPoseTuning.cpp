#include "Player/Animation/HeroPoseTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Pose.* console variables (a development build only; shipping plays the data as it is) and the aim's tables.

namespace
{
#if !UE_BUILD_SHIPPING
	TAutoConsoleVariable<int32> CVarPoseOn(TEXT("Looter.Pose.On"), 1,
		TEXT("1 each hero's own idle stance (FHeroIdle); 0 Ellis's weary idle with open hands on every hero, as before 2026-10-22."));
	TAutoConsoleVariable<float> CVarPoseCurl(TEXT("Looter.Pose.Curl"), 1.f,
		TEXT("Scales every idle hand's rest curl (1 the hero's; 0 the animation's open hand)."));
	TAutoConsoleVariable<int32> CVarPoseAim(TEXT("Looter.Pose.Aim"), 1,
		TEXT("Third person: 1 aiming down the sights raises the gun to the eye (FHeroAim); 0 keeps the hip hold, as before 2026-10-22."));
	TAutoConsoleVariable<float> CVarPoseAimHead(TEXT("Looter.Pose.Aim.Head"), 1.f,
		TEXT("Scales the head's part of the aim (down onto the gun, canted to its shoulder)."));
	TAutoConsoleVariable<float> CVarEyeAhead(TEXT("Looter.Pose.Aim.EyeAhead"), 0.f, TEXT("Moves the eye the sight lines up with ahead (cm, added)."));
	TAutoConsoleVariable<float> CVarEyeRight(TEXT("Looter.Pose.Aim.EyeRight"), 0.f, TEXT("Moves the eye the sight lines up with right (cm, added)."));
	TAutoConsoleVariable<float> CVarEyeUp(TEXT("Looter.Pose.Aim.EyeUp"), 0.f, TEXT("Moves the eye the sight lines up with up (cm, added)."));

	TAutoConsoleVariable<float> CVarGripRifle(TEXT("Looter.Pose.Aim.Grip.Rifle"), 0.f, TEXT("Aimed: the rifle's grip this far ahead of the eye (cm; 0 the table's)."));
	TAutoConsoleVariable<float> CVarGripShotgun(TEXT("Looter.Pose.Aim.Grip.Shotgun"), 0.f, TEXT("Aimed: the shotgun's grip ahead of the eye (cm; 0 the table's)."));
	TAutoConsoleVariable<float> CVarGripRevolver(TEXT("Looter.Pose.Aim.Grip.Revolver"), 0.f, TEXT("Aimed: the six-gun's grip ahead of the eye (cm; 0 the table's)."));
	TAutoConsoleVariable<float> CVarGripSMG(TEXT("Looter.Pose.Aim.Grip.SMG"), 0.f, TEXT("Aimed: the SMG's grip ahead of the eye (cm; 0 the table's)."));
	TAutoConsoleVariable<float> CVarGripSniper(TEXT("Looter.Pose.Aim.Grip.Sniper"), 0.f, TEXT("Aimed: the sniper rifle's grip ahead of the eye (cm; 0 the table's)."));
	TAutoConsoleVariable<float> CVarGripLMG(TEXT("Looter.Pose.Aim.Grip.LMG"), 0.f, TEXT("Aimed: the LMG's grip ahead of the eye (cm; 0 the table's)."));
	TAutoConsoleVariable<float> CVarGripLauncher(TEXT("Looter.Pose.Aim.Grip.Launcher"), 0.f, TEXT("Aimed: the launcher's grip ahead of the eye (cm; 0 the table's)."));

	const TAutoConsoleVariable<float>* GripCVar(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Rifle: return &CVarGripRifle;
		case EWeaponKind::Shotgun: return &CVarGripShotgun;
		case EWeaponKind::Revolver: return &CVarGripRevolver;
		case EWeaponKind::SMG: return &CVarGripSMG;
		case EWeaponKind::Sniper: return &CVarGripSniper;
		case EWeaponKind::LMG: return &CVarGripLMG;
		case EWeaponKind::Launcher: return &CVarGripLauncher;
		default: return nullptr;
		}
	}
#endif
}

FHeroIdle HeroPoseTuning::Idle(const FHeroIdle& Style)
{
#if !UE_BUILD_SHIPPING
	if (CVarPoseOn.GetValueOnAnyThread() == 0)
	{
		FHeroIdle Old;
		Old.RestCurl = 0.f;
		return Old;
	}
	FHeroIdle Tuned = Style;
	Tuned.RestCurl *= FMath::Max(CVarPoseCurl.GetValueOnAnyThread(), 0.f);
	return Tuned;
#else
	return Style;
#endif
}

FHeroAim HeroPoseTuning::Aim(const FHeroAim& Style)
{
	FHeroAim Tuned = Style;
#if !UE_BUILD_SHIPPING
	if (CVarPoseAim.GetValueOnAnyThread() == 0)
	{
		Tuned.Raise = 0.f;
	}
	const float Head = CVarPoseAimHead.GetValueOnAnyThread();
	Tuned.HeadDown *= Head;
	Tuned.HeadCant *= Head;
#endif
	return Tuned;
}

float HeroPoseTuning::DefaultGripAhead(EWeaponKind Kind)
{
	// The eye over the stock's comb, the pistol grip a hand's length ahead of it (at 22 the support hand fell 5-8 cm short of
	// the foregrip: the mannequin's arm is ~54 cm shoulder to wrist); a six-gun out at most of the arms' reach, the elbows a
	// little bent.
	switch (Kind)
	{
	case EWeaponKind::Shotgun: return 15.f;
	case EWeaponKind::Revolver: return 40.f;
	case EWeaponKind::SMG: return 13.f;
	case EWeaponKind::Sniper: return 14.f;
	case EWeaponKind::LMG: return 14.f;
	case EWeaponKind::Launcher: return 12.f;
	default: return 14.f;
	}
}

float HeroPoseTuning::GripAhead(EWeaponKind Kind)
{
#if !UE_BUILD_SHIPPING
	if (const TAutoConsoleVariable<float>* Tuned = GripCVar(Kind))
	{
		const float Value = Tuned->GetValueOnAnyThread();
		if (!FMath::IsNearlyZero(Value))
		{
			return Value;
		}
	}
#endif
	return DefaultGripAhead(Kind);
}

FVector HeroPoseTuning::DefaultEyeFromHead()
{
	// SKM_Manny's head bone sits at the top of the neck; the right eye ~9 cm ahead of it, ~10 up and 3.5 to the right.
	return FVector(9.f, 3.5f, 10.f);
}

FVector HeroPoseTuning::EyeFromHead()
{
	FVector Eye = DefaultEyeFromHead();
#if !UE_BUILD_SHIPPING
	Eye += FVector(CVarEyeAhead.GetValueOnAnyThread(), CVarEyeRight.GetValueOnAnyThread(), CVarEyeUp.GetValueOnAnyThread());
#endif
	return Eye;
}
