#include "Combat/BulletSubsystem.h"
#include "Weapons/ElementVfxTuning.h"
#include "Weapons/WeaponFX.h"

// UBulletSubsystem: the trail an element round leaves behind its tracer ("Elements matter" lane F, 2026-10-29): Cinder heat
// shimmer and embers, Thunder a forked spark line, Rot a dripping spatter, Wardbreak a frost line, Hallow a ribbon of light
// (FWeaponFX::SpawnElementTrail). The tracer itself keeps its tint (TracerColor, leaning 55% to the element since 2026-10-22).

namespace
{
	/**
	 * Laid from this far out (cm; clear of the eye, as the smoke thread is), in pieces of at most TrailPiece cm, fading in over
	 * TrailFadeIn; a round lays at most TrailPiecesAFrame pieces a frame (an instant shot's whole way would be one burst).
	 */
	constexpr float TrailStart = 120.f;
	constexpr float TrailPiece = 120.f;
	constexpr float TrailFadeIn = 300.f;
	constexpr int32 TrailPiecesAFrame = 24;
}

void UBulletSubsystem::LayElementTrail(FBullet& Bullet)
{
	const FBulletShot& Shot = Bullet.Shot;
	// The first rounds of a volley only (a shotgun's first two pellets, as the smoke thread), a gun's own (not a bounce or a
	// twin's second), and a fast gun's every other round.
	if (Shot.Element == ELooterElement::None || !Bullet.bThread || Shot.EffectGeneration > 0)
	{
		return;
	}
	const int32 Every = FElementVfxTuning::TrailEvery(Bullet.Kind);
	if (Every > 1 && Bullet.RoundsLeft % Every != 0)
	{
		return;
	}
	const FElementVfxTuning Tuning = FElementVfxTuning::Live();
	if (Tuning.Trail <= 0.f)
	{
		return;
	}
	const float End = FMath::Min(Bullet.Traveled, Tuning.TrailReach);
	float From = FMath::Max(Bullet.TrailLaid, TrailStart);
	for (int32 Piece = 0; Piece < TrailPiecesAFrame && From < End - 1.f; ++Piece)
	{
		const float To = FMath::Min(From + TrailPiece, End);
		const float FadeIn = FMath::Clamp((From - TrailStart) / TrailFadeIn, 0.3f, 1.f);
		Effects.SpawnElementTrail(Shot.Element, TracerPoint(Shot, From), TracerPoint(Shot, To), FadeIn * Tuning.Trail);
		From = To;
	}
	Bullet.TrailLaid = FMath::Max(Bullet.TrailLaid, FMath::Min(From, End));
}
