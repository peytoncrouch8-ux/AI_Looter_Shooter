#include "Bounties/BountyTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Bounty.* cvars: their defaults are FBountyTuning's own field values, so the rules' tests (a default-made
// tuning) and a fresh game agree.

namespace
{
	const FBountyTuning BountyDefaults;

	TAutoConsoleVariable<int32> CVarEnabled(TEXT("Looter.Bounty.Enabled"), BountyDefaults.bEnabled ? 1 : 0,
		TEXT("The bounty board posts and its fights spawn (0: shut)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRotateMinutes(TEXT("Looter.Bounty.RotateMinutes"), BountyDefaults.RotateMinutes,
		TEXT("Minutes of play between the bounty board's rotations (the postings not taken come down, new ones go up)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRefillMinutes(TEXT("Looter.Bounty.RefillMinutes"), BountyDefaults.RefillMinutes,
		TEXT("Minutes of play before a paid-out slot takes a new posting."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarPushWhenLeft(TEXT("Looter.Bounty.PushWhenLeft"), BountyDefaults.PushWhenLeft,
		TEXT("A bounty fight's next push comes once this many of the last are left standing."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSpringRadius(TEXT("Looter.Bounty.SpringRadius"), BountyDefaults.SpringRadius,
		TEXT("A bounty fight springs on the player coming this near (cm) its arena's middle."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHealthScale(TEXT("Looter.Bounty.HealthScale"), BountyDefaults.HealthScale,
		TEXT("Every bounty creature's health against an area creature of its kind, rank and level (1.25: a quarter more)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarGunsWanted(TEXT("Looter.Bounty.RewardGunsWanted"), BountyDefaults.RewardGuns[0],
		TEXT("Guns handed over at the board for a Wanted bounty."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarGunsDangerous(TEXT("Looter.Bounty.RewardGunsDangerous"), BountyDefaults.RewardGuns[1],
		TEXT("Guns handed over at the board for a Dangerous bounty."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarGunsDeadly(TEXT("Looter.Bounty.RewardGunsDeadly"), BountyDefaults.RewardGuns[2],
		TEXT("Guns handed over at the board for a Deadly bounty."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRewardLuck(TEXT("Looter.Bounty.RewardLuck"), BountyDefaults.RewardLuck,
		TEXT("The luck of a bounty's reward guns (a Strongbox's is 1)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarRewardAmmo(TEXT("Looter.Bounty.RewardAmmo"), BountyDefaults.RewardAmmo,
		TEXT("Ammo pickups handed over with a bounty's guns (a chest's 1.5 boxes each)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarPurseWanted(TEXT("Looter.Bounty.PurseWanted"), BountyDefaults.Purse[0],
		TEXT("A Wanted bounty's purse at level 1 (silver; grown with the arena's level)."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarPurseDangerous(TEXT("Looter.Bounty.PurseDangerous"), BountyDefaults.Purse[1],
		TEXT("A Dangerous bounty's purse at level 1."), ECVF_Default);
	TAutoConsoleVariable<int32> CVarPurseDeadly(TEXT("Looter.Bounty.PurseDeadly"), BountyDefaults.Purse[2],
		TEXT("A Deadly bounty's purse at level 1."), ECVF_Default);
	TAutoConsoleVariable<float> CVarXPWanted(TEXT("Looter.Bounty.XPShareWanted"), BountyDefaults.XPShare[0],
		TEXT("A Wanted bounty's experience: this share of the step from its arena's top level to the next."), ECVF_Default);
	TAutoConsoleVariable<float> CVarXPDangerous(TEXT("Looter.Bounty.XPShareDangerous"), BountyDefaults.XPShare[1],
		TEXT("A Dangerous bounty's experience share."), ECVF_Default);
	TAutoConsoleVariable<float> CVarXPDeadly(TEXT("Looter.Bounty.XPShareDeadly"), BountyDefaults.XPShare[2],
		TEXT("A Deadly bounty's experience share."), ECVF_Default);
	TAutoConsoleVariable<float> CVarChimeRange(TEXT("Looter.Bounty.PostedChimeRange"), BountyDefaults.PostedChimeRange,
		TEXT("A new posting chimes when the player is this near the board (cm)."), ECVF_Default);
}

FBountyTuning FBountyTuning::Current()
{
	FBountyTuning Made;
	Made.bEnabled = CVarEnabled.GetValueOnGameThread() != 0;
	// A rotation or a refill quicker than a fight would only churn the board.
	Made.RotateMinutes = FMath::Max(CVarRotateMinutes.GetValueOnGameThread(), 1.f);
	Made.RefillMinutes = FMath::Max(CVarRefillMinutes.GetValueOnGameThread(), 0.f);
	Made.PushWhenLeft = FMath::Clamp(CVarPushWhenLeft.GetValueOnGameThread(), 0, 8);
	Made.SpringRadius = FMath::Max(CVarSpringRadius.GetValueOnGameThread(), 300.f);
	// Never below a tenth: a bounty creature always has some health to take.
	Made.HealthScale = FMath::Clamp(CVarHealthScale.GetValueOnGameThread(), 0.1f, 10.f);
	Made.RewardGuns[0] = FMath::Clamp(CVarGunsWanted.GetValueOnGameThread(), 0, 6);
	Made.RewardGuns[1] = FMath::Clamp(CVarGunsDangerous.GetValueOnGameThread(), 0, 6);
	Made.RewardGuns[2] = FMath::Clamp(CVarGunsDeadly.GetValueOnGameThread(), 0, 6);
	Made.RewardLuck = FMath::Max(CVarRewardLuck.GetValueOnGameThread(), 0.f);
	Made.RewardAmmo = FMath::Clamp(CVarRewardAmmo.GetValueOnGameThread(), 0, 6);
	Made.Purse[0] = FMath::Max(CVarPurseWanted.GetValueOnGameThread(), 0);
	Made.Purse[1] = FMath::Max(CVarPurseDangerous.GetValueOnGameThread(), 0);
	Made.Purse[2] = FMath::Max(CVarPurseDeadly.GetValueOnGameThread(), 0);
	Made.XPShare[0] = FMath::Max(CVarXPWanted.GetValueOnGameThread(), 0.f);
	Made.XPShare[1] = FMath::Max(CVarXPDangerous.GetValueOnGameThread(), 0.f);
	Made.XPShare[2] = FMath::Max(CVarXPDeadly.GetValueOnGameThread(), 0.f);
	Made.PostedChimeRange = FMath::Max(CVarChimeRange.GetValueOnGameThread(), 0.f);
	return Made;
}
