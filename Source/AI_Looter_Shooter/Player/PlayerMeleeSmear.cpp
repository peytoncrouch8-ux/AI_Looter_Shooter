#include "Player/PlayerMeleeComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "Weapons/WeaponFXMelee.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

// UPlayerMeleeComponent's swing smear (round 5, 2026-10-27: the user's pick from a video, "swing smear + hit feel"): where
// the tool's head passed through the strike, kept for a moment and drawn as a glowing band in the tool's tint that thins
// and fades in about 0.12 s (MeleeFX::DrawSmear on FWeaponFX's streaks: no assets, a few dozen quads at most).

namespace
{
	TAutoConsoleVariable<float> CVarSmearInner(TEXT("Looter.Melee.Smear.Inner"), 0.6f,
		TEXT("Where the smear's inner edge runs, from the hand (0) to the tool's tip (1)."));

	const FName TipSocket(TEXT("Tip"));
	const FName GripSocket(TEXT("Grip"));
	const FName ForearmSocket(TEXT("Forearm"));
	/** At most this many places kept (two frames' worth past the smear's life at 120 fps). */
	constexpr int32 MaxSmearSamples = 40;
}

void UPlayerMeleeComponent::UpdateSmear()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	const float Life = MeleeFX::SmearSeconds();
	Smear.RemoveAll([Now, Life](const MeleeFX::FSmearSample& Sample) { return Now - Sample.Time > Life; });

	// Laid down while the head is swung hard (the drive and the follow-through), not while the blow holds it still.
	if (bSwinging && HoldTime <= 0.f && MeleeMotion::IsSmearing(SwingTime))
	{
		MeleeFX::FSmearSample Sample;
		FVector Way;
		if (FindToolSocket(TipSocket, Sample.Tip, Way, &Sample.Scale))
		{
			FVector Hand;
			if (!FindToolSocket(GripSocket, Hand, Way) && !FindToolSocket(ForearmSocket, Hand, Way))
			{
				Hand = Sample.Tip;
			}
			Sample.Inner = FMath::Lerp(Hand, Sample.Tip, FMath::Clamp(CVarSmearInner.GetValueOnGameThread(), 0.f, 1.f));
			Sample.Time = Now;
			if (Smear.Num() >= MaxSmearSamples)
			{
				Smear.RemoveAt(0);
			}
			Smear.Add(Sample);
		}
	}
	if (Smear.Num() < 2)
	{
		return;
	}
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	MeleeFX::DrawSmear(Effects, Smear, Now, MeleeFX::SmearColor(Tool.Kind));
}
