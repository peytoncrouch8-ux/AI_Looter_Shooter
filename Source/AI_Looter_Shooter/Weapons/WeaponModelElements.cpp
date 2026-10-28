#include "Weapons/WeaponModelComponent.h"
#include "Elements/ElementRules.h"
#include "Components/PrimitiveComponent.h"
#include "HAL/IConsoleManager.h"

// The gun's element on its looks (the user's call, 2026-10-27, "Glow parts take element"): every glowing bit of a gun with an
// element part (vents, barrel slots, sight dots, gauges) glows in ElementRules::FxColor, the rarity accent keeps the rarity's
// colour and the shell its maker's paint; the element part itself wears its element as a finish the gun master draws on its
// own mesh (scorched, gilt, frosted), so no part needs a model of its own and the Tripo parts take it as the scripted ones do.
// The gun's other parts wear the same finish lightly, as stains (ElementSpread of it; the user's call 2026-10-27, "including
// the elemental differences"), so the whole gun reads as its element and the element part still stands out.
// The material side: Tools/Unreal/build_world_materials.py (M_Gun, ELEMENT_FINISH_*).

namespace
{
	TAutoConsoleVariable<bool> CVarElementGlows(TEXT("Looter.Weapons.ElementLook.Glows"), true,
		TEXT("A gun with an element part glows in its element's colour (vents, sight dots, gauges); 0 keeps the art's. Read as a gun is built."));
	TAutoConsoleVariable<bool> CVarElementReticle(TEXT("Looter.Weapons.ElementLook.Reticle"), true,
		TEXT("The optics' reticle (GunReticle) takes the element's colour too; 0 keeps it red."));
	TAutoConsoleVariable<bool> CVarElementFinish(TEXT("Looter.Weapons.ElementLook.Finish"), true,
		TEXT("An element part wears its element's finish (Ember scorched, Soul-light gilt, Grave-salt frosted); 0 shows it as modeled."));
	TAutoConsoleVariable<float> CVarElementAmount(TEXT("Looter.Weapons.ElementLook.Amount"), 1.f,
		TEXT("Scales how far each finish pulls the part's colour to its own (0-1 after scaling)."));
	TAutoConsoleVariable<float> CVarElementSpread(TEXT("Looter.Weapons.ElementLook.Spread"), 0.25f,
		TEXT("How much of the element part's finish the gun's other parts wear as stains (0-1 of its amount and glow). Read as a gun is built."));
	TAutoConsoleVariable<float> CVarEmberGlow(TEXT("Looter.Weapons.ElementLook.EmberGlow"), 4.f,
		TEXT("How bright a scorched part's seams glow (emissive x the Ember hue)."));
	TAutoConsoleVariable<float> CVarSoulGlow(TEXT("Looter.Weapons.ElementLook.SoulGlow"), 0.6f,
		TEXT("How bright a gilt part's faint rim glows (emissive x the Soul-light hue)."));
	TAutoConsoleVariable<float> CVarSaltGlow(TEXT("Looter.Weapons.ElementLook.SaltGlow"), 2.5f,
		TEXT("How bright a frosted part's lit salt crystals glow (emissive x the Grave-salt hue)."));

	/** The finishes' colours (linear) and how far each pulls the part to it, before Looter.Weapons.ElementLook.Amount. */
	const FLinearColor ScorchedTint(0.075f, 0.058f, 0.05f);   // soot-dark heat-scored metal
	constexpr float ScorchedAmount = 0.85f;
	const FLinearColor GiltTint(0.92f, 0.68f, 0.3f);          // pale gold, a little paler than the brass fittings
	constexpr float GiltAmount = 0.8f;
	const FLinearColor FrostedTint(0.6f, 0.7f, 0.75f);        // pale, cold, salt-crusted
	constexpr float FrostedAmount = 0.75f;

	/** The optics' reticle: an emissive slot without "Glow" in its name. */
	const FName ReticleSlot(TEXT("GunReticle"));
	/** The gun kit's glows are named Gun<Thing>Glow (Tools/Blender/looter_guns.py glow()). */
	const TCHAR* const KitPrefix = TEXT("Gun");
	const TCHAR* const GlowWord = TEXT("Glow");
}

bool UWeaponModelComponent::IsElementGlowSlot(FName Slot, FName RarityGlowSlot)
{
	if (Slot.IsNone())
	{
		return false;
	}
	const FString Name = Slot.ToString();
	// The rarity accent keeps the rarity's colour, a part's own copy of it too ("<slot>_<part>", as the paints' copies).
	if (Slot == RarityGlowSlot || (!RarityGlowSlot.IsNone() && Name.StartsWith(RarityGlowSlot.ToString() + TEXT("_"))))
	{
		return false;
	}
	if (Slot == ReticleSlot)
	{
		return CVarElementReticle.GetValueOnGameThread();
	}
	return Name.StartsWith(KitPrefix) && Name.Contains(GlowWord);
}

bool UWeaponModelComponent::ElementGlowsOn()
{
	return CVarElementGlows.GetValueOnGameThread();
}

FWeaponElementFinish UWeaponModelComponent::ElementFinishFor(ELooterElement Element)
{
	FWeaponElementFinish Finish;
	if (Element == ELooterElement::None || !CVarElementFinish.GetValueOnGameThread())
	{
		return Finish;
	}
	const float Scale = FMath::Max(CVarElementAmount.GetValueOnGameThread(), 0.f);
	Finish.Glow = ElementRules::FxColor(Element);
	switch (Element)
	{
	case ELooterElement::Ember:
		Finish.Kind = FinishScorched;
		Finish.Tint = ScorchedTint;
		Finish.Amount = ScorchedAmount;
		Finish.GlowStrength = CVarEmberGlow.GetValueOnGameThread();
		break;
	case ELooterElement::SoulLight:
		Finish.Kind = FinishGilt;
		Finish.Tint = GiltTint;
		Finish.Amount = GiltAmount;
		Finish.GlowStrength = CVarSoulGlow.GetValueOnGameThread();
		break;
	case ELooterElement::GraveSalt:
		Finish.Kind = FinishFrosted;
		Finish.Tint = FrostedTint;
		Finish.Amount = FrostedAmount;
		Finish.GlowStrength = CVarSaltGlow.GetValueOnGameThread();
		break;
	default:
		return FWeaponElementFinish();
	}
	Finish.Amount = FMath::Clamp(Finish.Amount * Scale, 0.f, 1.f);
	Finish.GlowStrength = FMath::Max(Finish.GlowStrength, 0.f);
	return Finish;
}

void UWeaponModelComponent::ShowElementFinish(UPrimitiveComponent& Part, ELooterElement Element, float AmountScale)
{
	FWeaponElementFinish Finish = ElementFinishFor(Element);
	// The other parts' stains: the same kind (the gun master draws it as soot streaks, gilt veins or salt rime by its amount),
	// lighter and dimmer. Nothing at all once scaled to zero.
	const float Scale = FMath::Clamp(AmountScale, 0.f, 1.f);
	if (Scale <= 0.f)
	{
		Finish = FWeaponElementFinish();
	}
	Finish.Amount *= Scale;
	Finish.GlowStrength *= Scale;
	Part.SetCustomPrimitiveDataFloat(ElementFinishDataIndex, Finish.Kind);
	Part.SetCustomPrimitiveDataVector4(ElementTintDataIndex, FVector4(Finish.Tint.R, Finish.Tint.G, Finish.Tint.B, Finish.Amount));
	Part.SetCustomPrimitiveDataVector4(ElementGlowDataIndex, FVector4(Finish.Glow.R, Finish.Glow.G, Finish.Glow.B, Finish.GlowStrength));
}

float UWeaponModelComponent::ElementSpread()
{
	return FMath::Clamp(CVarElementSpread.GetValueOnGameThread(), 0.f, 1.f);
}
