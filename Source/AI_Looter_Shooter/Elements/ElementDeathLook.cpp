#include "Elements/ElementDeathLook.h"
#include "Weapons/WeaponFX.h"

// The elements' deaths (see the header): FWeaponFX's element looks, placed over the whole body.

namespace
{
	/** Pieces on a body of the spider's size (40 cm round) for each element. */
	constexpr float EmberAura = 28.f;
	constexpr float SoulAura = 8.f;
	constexpr float SaltAura = 34.f;
	/** Thunder's pieces are mostly three-bead crawling arcs, so fewer of them; Rot's are single drips, bubbles and wisps. */
	constexpr float ThunderAura = 14.f;
	constexpr float RotAura = 22.f;
}

int32 ElementDeathLook::AuraCount(ELooterElement Element, float BodyScale)
{
	float Base = 0.f;
	switch (Element)
	{
	case ELooterElement::Ember: Base = EmberAura; break;
	case ELooterElement::SoulLight: Base = SoulAura; break;
	case ELooterElement::GraveSalt: Base = SaltAura; break;
	case ELooterElement::Thunder: Base = ThunderAura; break;
	case ELooterElement::Rot: Base = RotAura; break;
	default: return 0;
	}
	return FMath::Max(1, FMath::RoundToInt32(Base * FMath::Sqrt(FMath::Clamp(BodyScale, 0.5f, 3.f))));
}

void ElementDeathLook::Play(FWeaponFX& Effects, ELooterElement Element, const FVector& Center, float Radius, float HalfHeight,
	const FVector& Camera)
{
	if (Element == ELooterElement::None)
	{
		return;
	}
	const float BodyScale = FMath::Clamp(Radius / 40.f, 0.5f, 3.f);
	const int32 Aura = AuraCount(Element, BodyScale) / (Effects.NumParticles() > ParticleBudget ? 2 : 1);
	const FVector Up = FVector::UpVector;
	// The glows sit on the body's side toward the eye: at its heart the body would hide them.
	const FVector Front = Camera.IsZero() ? FVector::ZeroVector : (Camera - Center).GetSafeNormal() * Radius * 0.8f;
	const FVector Heart = Center + Front + Up * HalfHeight * 0.2f;
	const FVector Foot = Center + Front * 0.5f - Up * HalfHeight * 0.45f;
	switch (Element)
	{
	case ELooterElement::Ember:
		// Embers breaking off all over it, then the whole body going up from its foot in a fountain and a curl of smoke.
		Effects.SpawnElementAura(Element, Center, Radius, HalfHeight, Aura, 1.6f);
		Effects.SpawnElementBurst(Element, Foot, Radius * 2.5f, true);
		break;
	case ELooterElement::SoulLight:
		// A burst of light from its heart, motes drawn up out of the body.
		Effects.SpawnElementBurst(Element, Heart, Radius * 3.f, true);
		Effects.SpawnElementAura(Element, Center, Radius, HalfHeight, Aura, 1.4f);
		break;
	case ELooterElement::GraveSalt:
		// Crystals shed from the whole body, then what is left of it flung out and falling.
		Effects.SpawnElementAura(Element, Center, Radius, HalfHeight, Aura, 1.4f);
		Effects.SpawnElementBurst(Element, Center + Front * 0.5f, Radius * 2.5f, true);
		break;
	case ELooterElement::Thunder:
		// A flash from its heart and sparks flung out, then sparks raining down over the body (the kill burst's), arcs
		// crackling over it as the last of the charge runs out.
		Effects.SpawnElementBurst(Element, Heart, Radius * 2.5f, true);
		Effects.SpawnElementAura(Element, Center, Radius, HalfHeight, Aura, 1.6f);
		break;
	case ELooterElement::Rot:
		// Drips running off the whole body, then it collapses low in a wet burst and a green cloud spreading over the ground.
		Effects.SpawnElementAura(Element, Center, Radius, HalfHeight, Aura, 1.4f);
		Effects.SpawnElementBurst(Element, Center + Front * 0.5f - Up * HalfHeight * 0.25f, Radius * 2.6f, true);
		break;
	default:
		break;
	}
}
