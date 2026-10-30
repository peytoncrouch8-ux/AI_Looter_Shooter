#include "Weapons/BulletMarks.h"

// The element rounds' marks ("Elements matter" lane F, 2026-10-29; the user's pick: "a mark where element shots land on ground
// and walls (scorch, burn, acid, salt frost, gilt)"): the atlas's styles 15-19 (Art/Textures/BulletMarks/make_bullet_marks.py
// Scorch, Burn, Acid, Frost, Gilt), on any surface that takes a mark. The decal can't glow (DBuffer); the brief flare on a fresh
// one (embers, sparks, drips, glints, motes) is FWeaponFX::SpawnElementMark's.

FBulletMarkLook FBulletMarks::ElementLookOf(ELooterElement Element, EImpactMaterial Material)
{
	const FBulletMarkLook Surface = LookOf(Material);
	if (Surface.Style < 0)
	{
		return { INDEX_NONE, 0.f };
	}
	// Each a rifle round's radius in cm (make_bullet_marks.py RADIUS_CM), never smaller than the surface's own hole would be.
	FBulletMarkLook Look;
	switch (Element)
	{
	case ELooterElement::Ember: Look = { FirstElementStyle, 9.f }; break;
	case ELooterElement::Thunder: Look = { FirstElementStyle + 1, 8.5f }; break;
	case ELooterElement::Rot: Look = { FirstElementStyle + 2, 9.f }; break;
	case ELooterElement::GraveSalt: Look = { FirstElementStyle + 3, 9.5f }; break;
	case ELooterElement::SoulLight: Look = { FirstElementStyle + 4, 8.f }; break;
	default: return { INDEX_NONE, 0.f };
	}
	Look.Radius = FMath::Max(Look.Radius, Surface.Radius);
	return Look;
}
