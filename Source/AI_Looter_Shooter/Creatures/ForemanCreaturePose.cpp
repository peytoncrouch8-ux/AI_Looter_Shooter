// AForemanCreature's body on the dead miner's arm layer: his entrance (the pick handle raised high and slammed down), the
// fuse lit in his raised left fist and thrown, the roof's blow driven into a prop (or the floor), knocked to a knee when
// staggered, and his death: knelt where he fell, head bowed, his lamp guttering while he says his last word, then fading.

#include "Creatures/ForemanCreature.h"
#include "Bosses/BossComponent.h"

namespace
{
	FVector Dir(float X, float Y, float Z)
	{
		return FVector(X, Y, Z).GetSafeNormal();
	}

	/** Raised over his head with both hands (the entrance; the roof's heave), and driven down or in. */
	const FVector HighUpperR = Dir(0.1f, 0.3f, 0.95f);
	const FVector HighForeR = Dir(-0.4f, -0.1f, 0.91f);
	const FVector HighUpperL = Dir(0.1f, -0.3f, 0.95f);
	const FVector HighForeL = Dir(-0.4f, 0.15f, 0.9f);
	const FVector SlamUpperR = Dir(0.9f, 0.15f, -0.4f);
	const FVector SlamForeR = Dir(0.7f, -0.05f, -0.71f);
	const FVector SlamUpperL = Dir(0.88f, -0.15f, -0.45f);
	const FVector SlamForeL = Dir(0.68f, 0.1f, -0.72f);
	const FVector DriveUpperR = Dir(0.95f, 0.15f, 0.12f);
	const FVector DriveForeR = Dir(0.97f, -0.1f, 0.08f);
	const FVector DriveUpperL = Dir(0.93f, -0.2f, 0.12f);
	const FVector DriveForeL = Dir(0.96f, 0.15f, 0.08f);
	/** The fuse: lit in his left fist held up high beside his head, the pick handle low in his right. */
	const FVector FuseUpperL = Dir(0.3f, -0.6f, 0.75f);
	const FVector FuseForeL = Dir(0.15f, -0.25f, 0.96f);
	const FVector LowUpperR = Dir(0.3f, 0.25f, -0.92f);
	const FVector LowForeR = Dir(0.8f, -0.2f, 0.55f);
	/** The lob: his left arm flung out forward and up. */
	const FVector ThrowUpperL = Dir(0.85f, -0.2f, 0.45f);
	const FVector ThrowForeL = Dir(0.95f, 0.05f, 0.3f);
	/** Knelt (staggered, or dying): both arms hanging before him. */
	const FVector HangUpperR = Dir(0.25f, 0.22f, -0.94f);
	const FVector HangForeR = Dir(0.55f, -0.05f, -0.83f);
	const FVector HangUpperL = Dir(0.25f, -0.22f, -0.94f);
	const FVector HangForeL = Dir(0.55f, 0.05f, -0.83f);

	/** The slam, the drive and the lob are snapped this long past the wind-up (s). */
	constexpr float SnapSeconds = 0.25f;

	/** Knelt: how far he sinks (cm), bends (degrees) and bows his head, staggered and dying. */
	constexpr float StaggerKneel = 30.f;
	constexpr float DeathKneel = 42.f;

	void Set(ADeadMinerCreature::FMinerArms& Out, const FVector& UpperR, const FVector& ForeR, const FVector& UpperL, const FVector& ForeL,
		float Rate)
	{
		Out.RightUpper = UpperR;
		Out.RightFore = ForeR;
		Out.LeftUpper = UpperL;
		Out.LeftFore = ForeL;
		Out.LeftWeight = 1.f;
		Out.Rate = Rate;
	}
}

void AForemanCreature::WantedArms(FMinerArms& Out) const
{
	if (IsDead())
	{
		// Knelt where he fell, head bowed over his hands (the Unpaid's own slump nods it the rest of the way).
		Set(Out, HangUpperR, HangForeR, HangUpperL, HangForeL, 3.f);
		Out.Kneel = DeathKneel;
		Out.Bow = 20.f;
		Out.HeadPitch = 16.f;
		return;
	}
	if (bStaggeredNow)
	{
		// Knocked to a knee, his coal bared.
		Set(Out, HangUpperR, HangForeR, HangUpperL, HangForeL, 14.f);
		Out.Kneel = StaggerKneel;
		Out.Bow = 18.f;
		Out.HeadPitch = 14.f;
		return;
	}
	if (GetCreatureState() == ECreatureState::Attack)
	{
		const float In = GetStateTime();
		const bool bWinding = In < AttackWindup;
		const float Snap = In < AttackWindup + SnapSeconds ? 26.f : 6.f;
		switch (Move)
		{
		case EForemanMove::Intro:
			if (bWinding)
			{
				Set(Out, HighUpperR, HighForeR, HighUpperL, HighForeL, 6.f);
				Out.Bow = -8.f;
				Out.HeadPitch = -10.f;
			}
			else
			{
				Set(Out, SlamUpperR, SlamForeR, SlamUpperL, SlamForeL, Snap);
				Out.Bow = 18.f;
			}
			return;
		case EForemanMove::Fuse:
			if (bWinding)
			{
				Set(Out, LowUpperR, LowForeR, FuseUpperL, FuseForeL, 7.f);
				Out.Bow = -4.f;
			}
			else
			{
				Set(Out, LowUpperR, LowForeR, ThrowUpperL, ThrowForeL, Snap);
				Out.Bow = 10.f;
			}
			return;
		case EForemanMove::Roof:
			if (bWinding)
			{
				Set(Out, HighUpperR, HighForeR, HighUpperL, HighForeL, 8.f);
				Out.Bow = -6.f;
			}
			else if (StruckProp.IsValid())
			{
				// Driven level into the timber.
				Set(Out, DriveUpperR, DriveForeR, DriveUpperL, DriveForeL, Snap);
				Out.Bow = 12.f;
			}
			else
			{
				Set(Out, SlamUpperR, SlamForeR, SlamUpperL, SlamForeL, Snap);
				Out.Bow = 18.f;
			}
			return;
		default:
			break;
		}
	}
	// His swing, his hunt, his pick handle on his shoulder, his digging at the fall: the dead miners' own.
	Super::WantedArms(Out);
}

bool AForemanCreature::HoldsPickOnLine() const
{
	// His handle swings along his forearm in his swing, his entrance's slam and the roof's blow; lighting and throwing a
	// fuse, and knocked to a knee, it stays in his fist as the model holds it.
	if (GetCreatureState() != ECreatureState::Attack || bStaggeredNow)
	{
		return false;
	}
	return Move == EForemanMove::Swing || Move == EForemanMove::Intro || Move == EForemanMove::Roof;
}

bool AForemanCreature::WantsToWork() const
{
	// Digging at the fall while he waits for his fight; never in it.
	return !(Boss && Boss->IsFighting()) && Super::WantsToWork();
}

float AForemanCreature::WantedLampFlare() const
{
	// His lamp flares with his entrance and his swing's heave (the tell), not with the fuse (its own sparks are the tell).
	if (GetCreatureState() == ECreatureState::Attack && Move == EForemanMove::Intro)
	{
		return FMath::Clamp(GetStateTime() / FMath::Max(AttackWindup, 0.01f), 0.f, 1.f);
	}
	return Move == EForemanMove::Swing ? Super::WantedLampFlare() : 0.f;
}

void AForemanCreature::UpdateDeathLook(float DeltaSeconds)
{
	// He kneels and says his last word while his lamp gutters; then he fades, his coal going out last.
	const float Fading = FMath::Clamp((DeathTime - KneelSeconds) / FMath::Max(FadeSeconds, 0.01f), 0.f, 1.f);
	PhaseAmount = Fading;
	Heat = FMath::Lerp(-0.25f, -1.f, Fading);
	const float Gutter = 0.55f + 0.45f * FMath::Abs(FMath::Sin(DeathTime * 7.3f) * FMath::Sin(DeathTime * 2.1f));
	LampBrightness = Foreman::OwnLampBrightness * Gutter * (1.f - Fading);
	// As he fades, he breaks into drifting ash as his dead do (cosmetic: his kneel and his last word are his fight's show).
	ShedDeathAsh(DeltaSeconds);
}
