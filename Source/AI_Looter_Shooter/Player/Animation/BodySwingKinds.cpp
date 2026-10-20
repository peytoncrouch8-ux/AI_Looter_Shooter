#include "Player/Animation/BodySwing.h"

// FBodySwing's tuning table: how each kind of swing chain swings (SettingsFor), and which kind a chain is by its name and
// the bone it hangs from (KindOf). The binding and the pose are BodySwing.cpp's, the step BodySwingPoints.cpp's.

FBodySwingSettings FBodySwing::SettingsFor(const FString& ChainName)
{
	FBodySwingSettings Settings;
	// Gravity is a small share: the animated pose already hangs, and the spring would hold a full share's sag off it
	// (g x share / stiffness^2: 2 cm on the skirt at 0.08).
	if (ChainName.StartsWith(TEXT("hook")))
	{
		// Crane's iron hook on four links against his chest (KindOf): a pendulum still (full gravity), but short, heavy and
		// lying on him, so it goes with the body (Carry) and settles soon. With the lantern's settings it swung out level at
		// every start and stop and kept swinging through his run (watched 2026-10-20).
		Settings.Stiffness = 7.f;
		Settings.Damping = 5.5f;
		Settings.Gravity = 1.f;
		Settings.Bulk = 1.5f;
		Settings.Carry = 0.75f;
	}
	else if (ChainName.StartsWith(TEXT("longhair")))
	{
		// Crane's long lank hair down his back and in front of his shoulders (KindOf): it lay whipping side to side through
		// his run (up to 50 degrees) and flung out on a start; held a little by his back and coat, it goes partly with him.
		Settings.Stiffness = 9.f;
		Settings.Damping = 5.f;
		Settings.Gravity = 0.05f;
		Settings.Carry = 0.35f;
	}
	else if (ChainName.StartsWith(TEXT("skirt")))
	{
		// The duster's heavy oilskin: swings wide on a turn or a stop and takes a moment to settle.
		Settings.Stiffness = 7.f;
		Settings.Damping = 3.5f;
		Settings.Gravity = 0.08f;
	}
	else if (ChainName.StartsWith(TEXT("cape")))
	{
		// Short and held by the shoulders: a lift and a flutter.
		Settings.Stiffness = 12.f;
		Settings.Damping = 5.f;
		Settings.Gravity = 0.05f;
		// Fluted and stiff, it stands off the arms further than the coat's cloth does.
		Settings.Bulk = 6.f;
	}
	else if (ChainName.StartsWith(TEXT("bandana")) || ChainName.StartsWith(TEXT("hair")))
	{
		Settings.Stiffness = 9.f;
		Settings.Damping = 3.5f;
		Settings.Gravity = 0.05f;
	}
	else if (ChainName.StartsWith(TEXT("lantern")))
	{
		// A pendulum on its wire: gravity swings it (it hangs straight down at rest, so there's no sag), the spring
		// barely holds it, slow to settle.
		Settings.Stiffness = 2.f;
		Settings.Damping = 1.2f;
		Settings.Gravity = 1.f;
		// Its glass and cage round the wire: 4.3 cm across, half of it each side, and a little.
		Settings.Bulk = 2.5f;
	}
	return Settings;
}

FString FBodySwing::KindOf(const FString& ChainName, const FString& ParentBone)
{
	const FString Parent = ParentBone.ToLower();
	if (ChainName.StartsWith(TEXT("lantern")) && Parent.StartsWith(TEXT("spine")))
	{
		return TEXT("hook");
	}
	if (ChainName.StartsWith(TEXT("hair")) && Parent.StartsWith(TEXT("neck")))
	{
		return TEXT("longhair");
	}
	return ChainName;
}

