// UAbelCorruptionComponent: his smog-black look (M_Ghost's own slots darkened a third at a time, the furnace coal, the black
// lantern's cold light) and the smog streaming off him. Split from AbelCorruption.cpp.

#include "Bosses/AbelCorruption.h"
#include "Bosses/AbelKeeper.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Weapons/WeaponFX.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** The smog off him: a puff this often at full corruption (s), its life, sizes and lift (cm/s), its soot-violet. */
	constexpr float SmogEvery = 0.07f;
	constexpr float SmogLife = 1.7f;
	constexpr float SmogStart = 26.f;
	constexpr float SmogEnd = 120.f;
	constexpr float SmogLift = 85.f;
	constexpr float SmogOpacity = 0.55f;
	const FLinearColor SmogColor(0.035f, 0.025f, 0.05f);
	/** Puffs only while a player is this near (cm): nobody sees smog off a man across the canyon. */
	constexpr float SmogSeenWithin = 7000.f;
	/** The bones the smog pours off (his rig's), the chest twice so it's thickest round the coal. */
	const TCHAR* const SmogBones[] = { TEXT("spine_02"), TEXT("spine_02"), TEXT("head"), TEXT("clavicle_l"), TEXT("clavicle_r"),
		TEXT("hand_l"), TEXT("hand_r"), TEXT("lantern") };

	/** His black lantern's light while it's corrupted: cold, low, a cyan glint (linear), against his ghost light's own. */
	const FLinearColor BlackLanternLight(0.35f, 0.85f, 1.f);

	/** The look's slots: the coal's crust has its own glow and keeps it. */
	bool IsCoalSlot(const UMaterialInterface* Material)
	{
		return Material && Material->GetName().Contains(TEXT("Coal"));
	}
}

void UAbelCorruptionComponent::MakeLook()
{
	AAbelKeeper* Abel = GetAbel();
	if (!Abel || !LookMaterials.IsEmpty())
	{
		return;
	}
	// His body, hat, lantern and pump (M_Ghost and its instances): a dynamic instance of each slot (not the coal's), its own
	// Tint kept to give back a third at a time. No new assets: the smog is the same material, darkened.
	TArray<UMeshComponent*> Parts;
	Parts.Add(Abel->GetMesh());
	Parts.Add(Abel->GetHat());
	Parts.Add(Abel->Lantern.Get());
	Parts.Add(Abel->Pump.Get());
	for (UMeshComponent* Part : Parts)
	{
		if (!Part)
		{
			continue;
		}
		for (int32 Slot = 0; Slot < Part->GetNumMaterials(); ++Slot)
		{
			UMaterialInterface* Own = Part->GetMaterial(Slot);
			if (!Own || IsCoalSlot(Own))
			{
				continue;
			}
			UMaterialInstanceDynamic* Made = Part->CreateDynamicMaterialInstance(Slot, Own);
			if (!Made)
			{
				continue;
			}
			FLinearColor Tint = FLinearColor::White;
			Made->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("Tint")), Tint);
			LookMaterials.Add(Made);
			OwnTints.Add(Tint);
			LookIsLantern.Add(Part == Abel->Lantern.Get());
		}
	}
}

void UAbelCorruptionComponent::ApplyLook(bool bForce)
{
	AAbelKeeper* Abel = GetAbel();
	if (!Abel || (!bForce && FMath::IsNearlyEqual(CorruptionShown, ShownLook, 0.004f)))
	{
		return;
	}
	ShownLook = CorruptionShown;
	for (int32 Index = 0; Index < LookMaterials.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Made = LookMaterials[Index];
		if (!Made)
		{
			continue;
		}
		// His lantern stays black while any smog is on him (the furnace's lantern, with a cyan glint in it); freed, his own.
		const float Share = LookIsLantern.IsValidIndex(Index) && LookIsLantern[Index] ? (CorruptionShown > 0.01f ? 1.f : 0.f) : CorruptionShown;
		Made->SetVectorParameterValue(TEXT("Tint"), AbelCorruptionRules::SmogTint(OwnTints[Index], Share));
	}
	// His coal: the furnace's orange behind his black ribs, the saint's gold-white once he's freed (the look's rank color).
	const FLinearColor Coal = AbelCorruptionRules::CoalColor(CorruptionShown);
	const FVector4 CoalData(Coal.R, Coal.G, Coal.B, Coal.A);
	for (UPrimitiveComponent* Part : { static_cast<UPrimitiveComponent*>(Abel->GetMesh()), static_cast<UPrimitiveComponent*>(Abel->GetHat()),
		static_cast<UPrimitiveComponent*>(Abel->Lantern.Get()), static_cast<UPrimitiveComponent*>(Abel->Pump.Get()) })
	{
		if (Part)
		{
			Part->SetCustomPrimitiveDataVector4(UnpaidLook::RankColorIndex, CoalData);
		}
	}
	// His lantern's light: a cold cyan glint while it's black, his ghost light's own once freed.
	if (Abel->LanternLight)
	{
		const FLinearColor Ghost = FLinearColor::FromSRGBColor(FColor(0xFF, 0xF0, 0xD6));
		Abel->LanternLight->SetLightColor(CorruptionShown > 0.01f ? BlackLanternLight : Ghost);
	}
}

void UAbelCorruptionComponent::EmitSmog(float DeltaSeconds)
{
	AAbelKeeper* Abel = GetAbel();
	UWorld* World = GetWorld();
	if (!Abel || !World || !World->IsGameWorld() || !Abel->IsPresent() || CorruptionShown <= 0.02f || Abel->IsHidden())
	{
		return;
	}
	const APawn* Player = GetPlayer();
	if (!Player || FVector::Dist(Player->GetActorLocation(), Abel->GetActorLocation()) > SmogSeenWithin)
	{
		return;
	}
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	USkeletalMeshComponent* Body = Abel->GetMesh();
	if (!Bullets || !Body)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	// "Soul smog streaming off him like smoke off a wet fire": as much as is left on him, pooled puffs off his bones, leaning
	// with the Gravewind when it blows.
	SmogClock += DeltaSeconds * CorruptionShown;
	const FVector Wind = Abel->IsWindBlowing() ? Abel->GetSunsetDirection() * (120.f + 260.f * Abel->GetGust()) : FVector::ZeroVector;
	int32 Guard = 0;
	while (SmogClock >= SmogEvery && Guard++ < 4)
	{
		SmogClock -= SmogEvery;
		const FName Bone(SmogBones[FMath::RandRange(0, UE_ARRAY_COUNT(SmogBones) - 1)]);
		const FVector From = Body->DoesSocketExist(Bone) ? Body->GetSocketLocation(Bone) : Abel->GetActorLocation();
		const FVector Drift = FVector(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f), SmogLift) + Wind;
		const float Size = Abel->GetSizeScale();
		Effects.SpawnDustPuff(From, Drift, SmogColor, SmogOpacity * FMath::Max(CorruptionShown, 0.35f), SmogStart * Size, SmogEnd * Size, SmogLife);
	}
}
