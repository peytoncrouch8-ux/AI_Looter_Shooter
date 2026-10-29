#include "Weapons/GunHeftTuning.h"
#include "Weapons/GunfeelKickTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Gunfeel.Heft.* and Looter.Gunfeel.Move.* variables (GunHeftTuning.h) and the plain rules that read them. Each
// accessor reads its variable when asked, never caching it, so a change in play shows on the next frame.

namespace
{
	namespace Def = GunHeft::Default;

	TAutoConsoleVariable<float> CVarWeightRevolver(TEXT("Looter.Gunfeel.Heft.Revolver"), Def::WeightRevolver,
		TEXT("The revolver's weight in the hands (1 = today's feel; under 1 lighter, over 1 heavier)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightSMG(TEXT("Looter.Gunfeel.Heft.SMG"), Def::WeightSMG,
		TEXT("The SMG's weight in the hands."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightRifle(TEXT("Looter.Gunfeel.Heft.Rifle"), Def::WeightRifle,
		TEXT("The rifle's weight in the hands."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightShotgun(TEXT("Looter.Gunfeel.Heft.Shotgun"), Def::WeightShotgun,
		TEXT("The shotgun's weight in the hands."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightSniper(TEXT("Looter.Gunfeel.Heft.Sniper"), Def::WeightSniper,
		TEXT("The sniper rifle's weight in the hands."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightLauncher(TEXT("Looter.Gunfeel.Heft.Launcher"), Def::WeightLauncher,
		TEXT("The launcher's weight in the hands."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWeightLMG(TEXT("Looter.Gunfeel.Heft.LMG"), Def::WeightLMG,
		TEXT("The LMG's weight in the hands."), ECVF_Default);
	TAutoConsoleVariable<float> CVarScale(TEXT("Looter.Gunfeel.Heft.Scale"), Def::Scale,
		TEXT("Every weight's distance from 1 times this (0: no heft at all)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarMoveRevolver(TEXT("Looter.Gunfeel.Move.Revolver"), Def::MoveRevolver,
		TEXT("Walking speed on foot with the revolver in hand times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMoveSMG(TEXT("Looter.Gunfeel.Move.SMG"), Def::MoveSMG,
		TEXT("Walking speed on foot with the SMG in hand times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMoveSniper(TEXT("Looter.Gunfeel.Move.Sniper"), Def::MoveSniper,
		TEXT("Walking speed on foot with the sniper rifle in hand times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMoveLMG(TEXT("Looter.Gunfeel.Move.LMG"), Def::MoveLMG,
		TEXT("Walking speed on foot with the LMG in hand times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMoveLauncher(TEXT("Looter.Gunfeel.Move.Launcher"), Def::MoveLauncher,
		TEXT("Walking speed on foot with the launcher in hand times this."), ECVF_Default);

	/** No weight goes under this: a feather-light gun would lag backwards-fast and its bob would flip sign. */
	constexpr float LightestWeight = 0.2f;
	/** The walking speed times a kind's variable never goes under this. */
	constexpr float SlowestMove = 0.5f;
}

GunHeft::FGunHeft GunHeft::FromWeight(float Weight, float Share)
{
	FGunHeft Heft;
	// Share 0 is heft off: weight 1 and no push, whatever weight was asked for, so the numbers are today's exactly.
	const float W = Share > 0.f ? FMath::Max(Weight, LightestWeight) : 1.f;
	Heft.Weight = W;
	Heft.LagGain = Default::LagGain * W;
	Heft.LagClampYaw = Default::LagClampYaw * W;
	Heft.LagClampPitch = Default::LagClampPitch * W;
	Heft.LagInterp = Default::LagInterp / FMath::Pow(W, Default::LagInterpExponent);
	Heft.Bob = 1.f + Default::BobShare * (W - 1.f);
	Heft.Breath = 1.f + Default::BreathShare * (W - 1.f);
	Heft.JumpLag = Default::JumpLag * W;
	Heft.LandKick = W;
	Heft.ArrivalPush = Share > 0.f ? Default::ArrivalPush * W * Share : 0.f;
	return Heft;
}

float GunHeft::ArrivalAt(const FGunHeft& Heft, float SecondsSinceArrival)
{
	if (Heft.ArrivalPush <= 0.f || SecondsSinceArrival < 0.f || SecondsSinceArrival >= Default::ArrivalSeconds)
	{
		return 0.f;
	}
	// No pop: the push rises over the first fifth (a smooth step) and eases out over the rest, down to 0 at the end.
	constexpr float RiseShare = 0.2f;
	const float U = SecondsSinceArrival / Default::ArrivalSeconds;
	if (U < RiseShare)
	{
		return Heft.ArrivalPush * FMath::SmoothStep(0.f, 1.f, U / RiseShare);
	}
	const float Left = (1.f - U) / (1.f - RiseShare);
	return Heft.ArrivalPush * Left * Left;
}

float GunHeft::Share()
{
	return GunfeelKick::IdentityOn() ? FMath::Max(CVarScale.GetValueOnGameThread(), 0.f) : 0.f;
}

float GunHeft::RawWeight(EWeaponKind Kind)
{
	switch (Kind)
	{
	case EWeaponKind::Revolver: return CVarWeightRevolver.GetValueOnGameThread();
	case EWeaponKind::SMG: return CVarWeightSMG.GetValueOnGameThread();
	case EWeaponKind::Rifle: return CVarWeightRifle.GetValueOnGameThread();
	case EWeaponKind::Shotgun: return CVarWeightShotgun.GetValueOnGameThread();
	case EWeaponKind::Sniper: return CVarWeightSniper.GetValueOnGameThread();
	case EWeaponKind::Launcher: return CVarWeightLauncher.GetValueOnGameThread();
	case EWeaponKind::LMG: return CVarWeightLMG.GetValueOnGameThread();
	default: return 1.f;
	}
}

float GunHeft::Weight(EWeaponKind Kind)
{
	return 1.f + (FMath::Max(RawWeight(Kind), LightestWeight) - 1.f) * Share();
}

GunHeft::FGunHeft GunHeft::For(EWeaponKind Kind)
{
	return FromWeight(Weight(Kind), Share());
}

float GunHeft::MoveScale(EWeaponKind Kind)
{
	if (!GunfeelKick::IdentityOn())
	{
		return 1.f;
	}
	switch (Kind)
	{
	case EWeaponKind::Revolver: return FMath::Max(CVarMoveRevolver.GetValueOnGameThread(), SlowestMove);
	case EWeaponKind::SMG: return FMath::Max(CVarMoveSMG.GetValueOnGameThread(), SlowestMove);
	case EWeaponKind::Sniper: return FMath::Max(CVarMoveSniper.GetValueOnGameThread(), SlowestMove);
	case EWeaponKind::LMG: return FMath::Max(CVarMoveLMG.GetValueOnGameThread(), SlowestMove);
	case EWeaponKind::Launcher: return FMath::Max(CVarMoveLauncher.GetValueOnGameThread(), SlowestMove);
	default: return 1.f;
	}
}
