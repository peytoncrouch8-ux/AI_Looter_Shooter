#include "Weapons/MakerSoundFlavor.h"
#include "Affixes/WeaponMakers.h"
#include "Audio/LooterSoundRules.h"
#include "HAL/IConsoleManager.h"
#include "Weapons/WeaponBase.h"

// The makers' sound flavours and their Looter.Gunfeel.Sound.Maker.* cvars. Each maker's cvars are made from its shipped
// numbers when the module loads (as WeaponMakerTuning does), so they're in the console from the start and a key's cvars
// always match the table. Small moves on purpose: a few percent of pitch and a few dB of a layer is enough to tell the
// makers apart shot after shot, and little enough that a Saint Maud shotgun is still a shotgun.

namespace
{
	FMakerSoundFlavor MakeFlavor(float Pitch, float TailPitch, float Core, float Mech, float Low, float Tail, float MechJitter,
		float Extra)
	{
		FMakerSoundFlavor Flavor;
		Flavor.Pitch = Pitch;
		Flavor.TailPitch = TailPitch;
		Flavor.Core = Core;
		Flavor.Mech = Mech;
		Flavor.Low = Low;
		Flavor.Tail = Tail;
		Flavor.MechJitter = MechJitter;
		Flavor.Extra = Extra;
		return Flavor;
	}

	const TArray<MakerSoundFlavor::FShipped>& Table()
	{
		// MakeFlavor(Pitch, TailPitch, Core, Mech, Low, Tail, MechJitter, Extra)
		static const TArray<MakerSoundFlavor::FShipped> Shipped = {
			// Heavy: railroad iron. A tone lower (7%) with more chest thump and a heavier clunk of the action; the land's
			// answer a shade duller and fuller.
			{ TEXT("Roundhouse"), MakeFlavor(0.93f, 0.97f, 1.f, 1.15f, 1.3f, 1.1f, 0.f, 0.f), nullptr },
			// Ringing: a blessed cast barrel. A touch brighter, the tail louder and brighter, the barrel's own ring after
			// the shot; less thump and action so the ring carries.
			{ TEXT("SaintMaud"), MakeFlavor(1.04f, 1.03f, 1.f, 0.85f, 0.9f, 1.35f, 0.f, 1.f), MakerSoundCue::SaintMaudRing },
			// Rattly: salvage bolted together. The action half again as loud and never the same twice (7% either way),
			// loose bits rattling with it, a thinner thump.
			{ TEXT("Windfall"), MakeFlavor(1.02f, 1.f, 0.95f, 1.5f, 0.85f, 1.f, 0.07f, 1.f), MakerSoundCue::WindfallRattle },
			// Clean: an instrument maker's work. Its own pitch, a quiet tight action and a short, quiet tail: the shot
			// itself, nothing hanging on.
			{ TEXT("Meridian"), MakeFlavor(1.f, 1.f, 1.f, 0.75f, 0.95f, 0.75f, 0.f, 0.f), nullptr },
		};
		return Shipped;
	}

	struct FField
	{
		const TCHAR* Name;
		float FMakerSoundFlavor::* Member;
		const TCHAR* Help;
	};

	const FField Fields[] = {
		{ TEXT("Pitch"), &FMakerSoundFlavor::Pitch, TEXT("the shot's pitch (Core, Mech, Low, Distant) times this.") },
		{ TEXT("TailPitch"), &FMakerSoundFlavor::TailPitch, TEXT("the tail's pitch on top of Pitch.") },
		{ TEXT("Core"), &FMakerSoundFlavor::Core, TEXT("the punch's volume times this.") },
		{ TEXT("Mech"), &FMakerSoundFlavor::Mech, TEXT("the action's volume times this.") },
		{ TEXT("Low"), &FMakerSoundFlavor::Low, TEXT("the chest thump's volume times this.") },
		{ TEXT("Tail"), &FMakerSoundFlavor::Tail, TEXT("the tail's volume times this.") },
		{ TEXT("MechJitter"), &FMakerSoundFlavor::MechJitter, TEXT("the action's pitch moves up to this much either way each shot.") },
		{ TEXT("Extra"), &FMakerSoundFlavor::Extra, TEXT("its own layer (ring, rattle) at this volume (0 = none).") },
	};

	struct FMakerVars
	{
		FName Key;
		TArray<IConsoleVariable*> Fields;
	};

	struct FFlavorCVars
	{
		TArray<FMakerVars> Makers;
		TArray<FString> Names;

		FFlavorCVars()
		{
			IConsoleManager& Console = IConsoleManager::Get();
			for (const MakerSoundFlavor::FShipped& Shipped : Table())
			{
				FMakerVars& Made = Makers.AddDefaulted_GetRef();
				Made.Key = FName(Shipped.Key);
				for (const FField& Field : Fields)
				{
					const FString& Name = Names.Add_GetRef(
						FString::Printf(TEXT("Looter.Gunfeel.Sound.Maker.%s.%s"), Shipped.Key, Field.Name));
					Made.Fields.Add(Console.RegisterConsoleVariable(*Name, Shipped.Flavor.*Field.Member,
						*FString::Printf(TEXT("%s's shot: %s"), Shipped.Key, Field.Help), ECVF_Default));
				}
			}
		}

		~FFlavorCVars()
		{
			// As TAutoConsoleVariable does: a reloaded module registers them afresh.
			IConsoleManager& Console = IConsoleManager::Get();
			for (const FString& Name : Names)
			{
				Console.UnregisterConsoleObject(*Name, false);
			}
		}

		const FMakerVars* Find(FName Key) const
		{
			return Makers.FindByPredicate([Key](const FMakerVars& Entry) { return Entry.Key == Key; });
		}
	};

	const FFlavorCVars& Vars()
	{
		static FFlavorCVars Made;
		return Made;
	}

	// Made when the module loads, so the cvars are in the console before any gun fires.
	struct FRegisterAtLoad
	{
		FRegisterAtLoad() { Vars(); }
	} RegisterAtLoad;

	TAutoConsoleVariable<float> CVarScale(TEXT("Looter.Gunfeel.Sound.Maker.Scale"), 1.f,
		TEXT("Every gun maker's sound flavour times this (0: every gun sounds plain, for comparisons; 2: twice as strong)."),
		ECVF_Default);

	/** A multiplier moved toward 1 by Scale. */
	float Toward(float Value, float InScale, float Min, float Max)
	{
		return FMath::Clamp(1.f + (Value - 1.f) * InScale, Min, Max);
	}

	FName MakerKeyOf(const AWeaponBase& Weapon)
	{
		const FWeaponMaker* Maker = WeaponMakers::Of(Weapon.GetInstance());
		return Maker ? Maker->Key : NAME_None;
	}
}

TConstArrayView<MakerSoundFlavor::FShipped> MakerSoundFlavor::Shipped()
{
	return Table();
}

const MakerSoundFlavor::FShipped* MakerSoundFlavor::ShippedFor(FName Key)
{
	if (Key.IsNone())
	{
		return nullptr;
	}
	return Table().FindByPredicate([Key](const FShipped& Entry) { return Key == FName(Entry.Key); });
}

FMakerSoundFlavor MakerSoundFlavor::ShippedFlavor(FName Key)
{
	const FShipped* Entry = ShippedFor(Key);
	return Entry ? Entry->Flavor : FMakerSoundFlavor();
}

float MakerSoundFlavor::Scale()
{
	return FMath::Max(CVarScale.GetValueOnGameThread(), 0.f);
}

FMakerSoundFlavor MakerSoundFlavor::For(FName Key)
{
	if (Key.IsNone())
	{
		return FMakerSoundFlavor();
	}
	const FMakerVars* Maker = Vars().Find(Key);
	if (!Maker)
	{
		return Scaled(ShippedFlavor(Key), Scale());
	}
	FMakerSoundFlavor Live;
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Fields)) && Index < Maker->Fields.Num(); ++Index)
	{
		if (Maker->Fields[Index])
		{
			Live.*Fields[Index].Member = Maker->Fields[Index]->GetFloat();
		}
	}
	return Scaled(Live, Scale());
}

FMakerSoundFlavor MakerSoundFlavor::Of(const AWeaponBase& Weapon)
{
	return For(MakerKeyOf(Weapon));
}

FName MakerSoundFlavor::ExtraCueFor(FName Key)
{
	const FShipped* Entry = ShippedFor(Key);
	return Entry && Entry->ExtraCue ? FName(Entry->ExtraCue) : NAME_None;
}

FName MakerSoundFlavor::ExtraCueOf(const AWeaponBase& Weapon)
{
	return ExtraCueFor(MakerKeyOf(Weapon));
}

FMakerSoundFlavor MakerSoundFlavor::Scaled(const FMakerSoundFlavor& Flavor, float InScale)
{
	const float S = FMath::Max(InScale, 0.f);
	FMakerSoundFlavor Out;
	Out.Pitch = Toward(Flavor.Pitch, S, 0.5f, 2.f);
	Out.TailPitch = Toward(Flavor.TailPitch, S, 0.5f, 2.f);
	Out.Core = Toward(Flavor.Core, S, 0.f, 4.f);
	Out.Mech = Toward(Flavor.Mech, S, 0.f, 4.f);
	Out.Low = Toward(Flavor.Low, S, 0.f, 4.f);
	Out.Tail = Toward(Flavor.Tail, S, 0.f, 4.f);
	Out.MechJitter = FMath::Clamp(Flavor.MechJitter * S, 0.f, 0.3f);
	Out.Extra = FMath::Clamp(Flavor.Extra * S, 0.f, 4.f);
	return Out;
}

bool MakerSoundFlavor::IsPlain(const FMakerSoundFlavor& Flavor)
{
	return FMath::IsNearlyEqual(Flavor.Pitch, 1.f) && FMath::IsNearlyEqual(Flavor.TailPitch, 1.f)
		&& FMath::IsNearlyEqual(Flavor.Core, 1.f) && FMath::IsNearlyEqual(Flavor.Mech, 1.f)
		&& FMath::IsNearlyEqual(Flavor.Low, 1.f) && FMath::IsNearlyEqual(Flavor.Tail, 1.f)
		&& FMath::IsNearlyZero(Flavor.MechJitter) && FMath::IsNearlyZero(Flavor.Extra);
}

float MakerSoundFlavor::MechPitch(const FMakerSoundFlavor& Flavor, float Roll)
{
	return LooterSoundRules::JitteredPitch(Flavor.Pitch, Flavor.MechJitter, Roll);
}

float MakerSoundFlavor::TailPitchOf(const FMakerSoundFlavor& Flavor)
{
	return Flavor.Pitch * Flavor.TailPitch;
}
