// A loot chest tier's show (ChestShow.h; the user's "escalating show", 2026-10-28): its pose by tier as the show and the
// lid come on, its moments (sounds and code-drawn light, FWeaponFX), its glow slots' dynamic instances, and tier 5's
// fountain of loot thrown one piece at a time.

#include "Loot/ChestShow.h"
#include "Loot/Chest.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Shields/ShieldPickup.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr int32 NumGlows = static_cast<int32>(EChestGlow::Num);

	const FName GlowParam(TEXT("Glow"));
	const FName ColorParam(TEXT("Color"));
	const FName LightSocket(TEXT("Light"));
	const FName OrbSocket(TEXT("Orb"));
	const FName GemSocket(TEXT("Gem"));

	/** Tier 3's lock gone green, the colour of "open" (a little warmer than tier 1's latch light). */
	const FLinearColor LockGreen(0.1f, 1.f, 0.22f);

	/** Each moment's share of the show, by tier (ChestShow::MomentsBetween). */
	struct FMomentAt
	{
		uint32 Moment;
		float Share;
	};
	const FMomentAt Tier1Moments[] = { { ChestShowMoment::LatchLeft, 0.f }, { ChestShowMoment::LatchRight, 0.25f }, { ChestShowMoment::LightFlare, 0.95f } };
	const FMomentAt Tier2Moments[] = { { ChestShowMoment::LatchLeft, 0.f }, { ChestShowMoment::LatchRight, 0.35f }, { ChestShowMoment::LightFlare, 0.95f } };
	const FMomentAt Tier3Moments[] = { { ChestShowMoment::LockSpin, 0.f }, { ChestShowMoment::LockOpen, 0.78f } };
	const FMomentAt Tier4Moments[] = { { ChestShowMoment::OrbPulse, 0.f }, { ChestShowMoment::TrimSweep, 0.4f } };
	const FMomentAt Tier5Moments[] = { { ChestShowMoment::CrestUnfold, 0.f }, { ChestShowMoment::Burst, 0.72f } };

	TConstArrayView<FMomentAt> MomentsOf(int32 Tier)
	{
		switch (Tier)
		{
		case 1: return MakeArrayView(Tier1Moments);
		case 2: return MakeArrayView(Tier2Moments);
		case 3: return MakeArrayView(Tier3Moments);
		case 4: return MakeArrayView(Tier4Moments);
		case 5: return MakeArrayView(Tier5Moments);
		default: return {};
		}
	}

	/** Where T is between A and B (0 to 1). */
	float Ramp(float T, float A, float B)
	{
		return FMath::Clamp((T - A) / FMath::Max(B - A, KINDA_SMALL_NUMBER), 0.f, 1.f);
	}

	float Smooth(float T)
	{
		return FMath::SmoothStep(0.f, 1.f, T);
	}

	/** Out past the end and back (a sprung latch, a crest locking into place): 0 at 0, about 1.1 at its height, 1 at 1. */
	float BackOut(float T)
	{
		constexpr float C1 = 1.70158f;
		constexpr float C3 = C1 + 1.f;
		const float U = T - 1.f;
		return 1.f + C3 * U * U * U + C1 * U * U;
	}

	/** A slot's colour before its instance is made (the stand-in, a test level): ChestTiers.py's. */
	FLinearColor DefaultColor(int32 Tier, EChestGlow Glow)
	{
		switch (Glow)
		{
		case EChestGlow::Trim: return FLinearColor(0.69f, 0.39f, 0.07f);
		case EChestGlow::Gem: return FLinearColor(1.f, 0.65f, 0.15f);
		default: break;
		}
		switch (Tier)
		{
		case 1: return FLinearColor(0.07f, 1.f, 0.14f);
		case 2: return FLinearColor(1.f, 0.52f, 0.04f);
		case 3: return FLinearColor(0.05f, 0.2f, 1.f);
		case 4: return FLinearColor(0.43f, 0.07f, 1.f);
		default: return FLinearColor(1.f, 0.38f, 0.03f);
		}
	}

	FWeaponFX* EffectsOf(const UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(const_cast<UWorld*>(World));
		return &Effects;
	}

	/** A socket's place on a part with a model, else Fallback. */
	FVector SocketOr(const UStaticMeshComponent* Part, FName Socket, const FVector& Fallback)
	{
		return Part && Part->GetStaticMesh() && Part->DoesSocketExist(Socket) ? Part->GetSocketLocation(Socket) : Fallback;
	}
}

// ---------------------------------------------------------------------------
// The pose and the moments, by tier
// ---------------------------------------------------------------------------

FChestShowPose ChestShow::PoseAt(int32 Tier, float PreShare, float LidShare)
{
	FChestShowPose Pose;
	const float S = FMath::Clamp(PreShare, 0.f, 1.f);
	const float L = FMath::Clamp(LidShare, 0.f, 1.f);
	float* Glow = Pose.Glow;
	// Tiers 3-5: the inside lights as the lid lifts (dim while shut: nobody sees it, and it isn't fully dark when they do).
	if (Tier >= 3)
	{
		Glow[static_cast<int32>(EChestGlow::Inner)] = FMath::Lerp(0.15f, 1.3f, Smooth(L));
	}
	switch (Tier)
	{
	case 1:
	case 2:
	{
		// The left latch, then the right, each sprung out past its stop and back; the front light flicks bright as the
		// second goes and fades back as the lid swings.
		const bool bOne = Tier == 1;
		Pose.LatchLeft = LatchOpenPitch * BackOut(Ramp(S, 0.f, bOne ? 0.65f : 0.6f));
		Pose.LatchRight = LatchOpenPitch * BackOut(Ramp(S, bOne ? 0.25f : 0.35f, bOne ? 0.9f : 0.95f));
		const float Flare = S < 1.f ? Smooth(Ramp(S, 0.75f, 1.f)) : 1.f - Smooth(Ramp(L, 0.f, 0.6f));
		Glow[static_cast<int32>(EChestGlow::Main)] = 1.f + 2.5f * Flare;
		break;
	}
	case 3:
	{
		// The ring spins up and slows to a stop, brightening; then it goes green with a flash that fades as the lid lifts.
		Pose.LockRoll = LockTurnDegrees * Smooth(Ramp(S, 0.f, 0.78f));
		Pose.LockGreen = Smooth(Ramp(S, 0.78f, 0.9f));
		const float Flash = Ramp(S, 0.78f, 0.85f) * (1.f - 0.6f * Ramp(S, 0.85f, 1.f)) * (1.f - Smooth(Ramp(L, 0.f, 0.5f)));
		Glow[static_cast<int32>(EChestGlow::LockRing)] = 1.f + 1.2f * Ramp(S, 0.f, 0.78f) * (1.f - Pose.LockGreen) + 0.3f * Pose.LockGreen
			+ 2.f * Flash;
		break;
	}
	case 4:
	{
		// The orb pulses twice, then holds bright; the trim lights from left to right after the first pulse and stays lit.
		const float P = Ramp(S, 0.f, 0.6f);
		const float Pulse = P < 1.f ? FMath::Square(FMath::Sin(2.f * UE_PI * P)) : 0.f;
		const float Hold = Smooth(Ramp(S, 0.55f, 0.8f));
		Glow[static_cast<int32>(EChestGlow::Main)] = 1.f + 2.2f * Pulse + Hold * (1.f - 0.6f * Smooth(L));
		Pose.OrbHalo = FMath::Max(Pulse, 0.6f * Hold * (1.f - L));
		const float Lit = Smooth(Ramp(S, 0.4f, 1.f));
		Glow[static_cast<int32>(EChestGlow::Trim)] = 1.f + (9.f - 3.f * Smooth(L)) * Lit;
		Pose.Sweep = S > 0.4f && S < 1.f ? Ramp(S, 0.4f, 1.f) : -1.f;
		break;
	}
	case 5:
	{
		// The crest swings out and up and locks; its gem gathers light, then bursts (Burst, at 0.72) and settles warm.
		Pose.CrestPitch = CrestOpenPitch * BackOut(Ramp(S, 0.f, 0.65f));
		const float Build = Smooth(Ramp(S, 0.f, 0.72f));
		const float Burst = S >= 0.72f ? 1.f - Smooth(Ramp(S, 0.72f, 1.f)) : 0.f;
		Glow[static_cast<int32>(EChestGlow::Gem)] = 1.f + 1.5f * Build * (1.f - 0.6f * Smooth(L)) + 3.f * Burst;
		Glow[static_cast<int32>(EChestGlow::Main)] = 1.f + 1.5f * Burst + 0.5f * Smooth(L);
		break;
	}
	default:
		break;
	}
	return Pose;
}

uint32 ChestShow::MomentsBetween(int32 Tier, float FromShare, float ToShare)
{
	uint32 Due = 0;
	for (const FMomentAt& Each : MomentsOf(Tier))
	{
		if (FromShare < Each.Share && Each.Share <= ToShare)
		{
			Due |= Each.Moment;
		}
	}
	return Due;
}

uint32 ChestShow::AllMoments(int32 Tier)
{
	return MomentsBetween(Tier, -1.f, 1.f);
}

bool ChestShow::Fountains(int32 Tier)
{
	return Tier >= 5;
}

FName ChestShow::GlowSlot(int32 Tier, EChestGlow Glow)
{
	if (Tier < 1 || Tier > FChestKindInfo::NumTiers)
	{
		return NAME_None;
	}
	switch (Glow)
	{
	case EChestGlow::Main: return FName(*FString::Printf(TEXT("ChestTier%dGlow"), Tier));
	case EChestGlow::LockRing: return Tier == 3 ? FName(TEXT("ChestTier3LockGlow")) : NAME_None;
	case EChestGlow::Trim: return Tier == 4 ? FName(TEXT("ChestTier4TrimGlow")) : NAME_None;
	case EChestGlow::Gem: return Tier == 5 ? FName(TEXT("ChestTier5GemGlow")) : NAME_None;
	case EChestGlow::Inner: return Tier >= 3 ? FName(*FString::Printf(TEXT("ChestTier%dInnerGlow"), Tier)) : NAME_None;
	default: return NAME_None;
	}
}

// ---------------------------------------------------------------------------
// On the chest
// ---------------------------------------------------------------------------

void AChest::MakeShowLooks()
{
	const FChestKindInfo Info = GetKindInfo();
	if (Show.bLooksMade || Info.PreMotion != EChestPreMotion::Show)
	{
		return;
	}
	Show.bLooksMade = true;
	Show.Looks.SetNum(NumGlows);
	UStaticMeshComponent* Parts[] = { Body, Lid, LatchLeft, LatchRight, Lock, Crest };
	for (int32 Index = 0; Index < NumGlows; ++Index)
	{
		const EChestGlow Glow = static_cast<EChestGlow>(Index);
		Show.BaseGlow[Index] = 1.f;
		Show.BaseColor[Index] = DefaultColor(Info.Tier, Glow);
		const FName Slot = ChestShow::GlowSlot(Info.Tier, Glow);
		if (Slot.IsNone())
		{
			continue;
		}
		// One instance per slot, worn by every part that has it (tier 4's orb glow on the body and the lid, tier 3's strips
		// on the body and the lock), so they always match.
		for (UStaticMeshComponent* Part : Parts)
		{
			const int32 Material = Part && Part->GetStaticMesh() ? Part->GetMaterialIndex(Slot) : INDEX_NONE;
			if (Material == INDEX_NONE)
			{
				continue;
			}
			if (!Show.Looks[Index])
			{
				UMaterialInterface* Imported = Part->GetMaterial(Material);
				if (!Imported)
				{
					continue;
				}
				float Value = 1.f;
				FLinearColor Color;
				if (Imported->GetScalarParameterValue(FMaterialParameterInfo(GlowParam), Value))
				{
					Show.BaseGlow[Index] = Value;
				}
				if (Imported->GetVectorParameterValue(FMaterialParameterInfo(ColorParam), Color))
				{
					Show.BaseColor[Index] = Color;
				}
				Show.Looks[Index] = UMaterialInstanceDynamic::Create(Imported, this);
			}
			Part->SetMaterial(Material, Show.Looks[Index]);
		}
	}
}

void AChest::UpdateShow()
{
	const FChestKindInfo Info = GetKindInfo();
	if (Info.PreMotion != EChestPreMotion::Show)
	{
		return;
	}
	Show.Pose = ChestShow::PoseAt(Info.Tier, PreShare, LidShare);
	const FChestShowPose& Pose = Show.Pose;
	// Each piece about its own pivot (its socket): the latches and the crest pitched, the lock rolled.
	LatchLeft->SetRelativeRotation(FRotator(Pose.LatchLeft, 0.f, 0.f).Quaternion());
	LatchRight->SetRelativeRotation(FRotator(Pose.LatchRight, 0.f, 0.f).Quaternion());
	Lock->SetRelativeRotation(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Pose.LockRoll)));
	Crest->SetRelativeRotation(FRotator(Pose.CrestPitch, 0.f, 0.f).Quaternion());
	for (int32 Index = 0; Index < Show.Looks.Num() && Index < NumGlows; ++Index)
	{
		UMaterialInstanceDynamic* Look = Show.Looks[Index];
		if (!Look)
		{
			continue;
		}
		Look->SetScalarParameterValue(GlowParam, Show.BaseGlow[Index] * Pose.Glow[Index]);
		if (static_cast<EChestGlow>(Index) == EChestGlow::LockRing)
		{
			Look->SetVectorParameterValue(ColorParam, FLinearColor::LerpUsingHSV(Show.BaseColor[Index], LockGreen, Pose.LockGreen));
		}
	}
}

void AChest::DrawShowFrame() const
{
	const FChestKindInfo Info = GetKindInfo();
	const FChestShowPose& Pose = Show.Pose;
	if (Info.Tier != 4 && Info.Tier != 5)
	{
		return;
	}
	FWeaponFX* Effects = EffectsOf(GetWorld());
	if (!Effects)
	{
		return;
	}
	const FVector Front = GetInteractionLocation().Get(GetActorLocation());
	const FVector Up = GetActorUpVector();
	auto ColorOf = [this, &Info](EChestGlow Glow)
	{
		return Show.bLooksMade ? Show.BaseColor[static_cast<int32>(Glow)] : DefaultColor(Info.Tier, Glow);
	};
	if (Info.Tier == 4)
	{
		// The orb's halo as it pulses (drawn this frame only: the show ticks while it plays).
		if (Pose.OrbHalo > 0.01f)
		{
			const FVector Orb = SocketOr(Lid, OrbSocket, SocketOr(Body, OrbSocket, Front + Up * 12.0));
			Effects->AddGlow(Orb, 14.f + 16.f * Pose.OrbHalo, ColorOf(EChestGlow::Main), 1.5f + 3.5f * Pose.OrbHalo);
		}
		// The sweep's spark running along the trim at the seam, left to right as the player faces it, a short tail behind.
		if (Pose.Sweep >= 0.f)
		{
			const float HalfWidth = Body->GetStaticMesh() ? static_cast<float>(Body->GetStaticMesh()->GetBounds().BoxExtent.Y) * 0.9f : 40.f;
			const FVector Side = GetActorRightVector();
			const FVector Spark = Front + Side * FMath::Lerp(-HalfWidth, HalfWidth, Pose.Sweep);
			const FLinearColor Gold = ColorOf(EChestGlow::Trim);
			Effects->AddGlow(Spark, 10.f, Gold, 6.f);
			Effects->AddStreak(Spark - Side * 14.0, Spark, 2.5f, Gold, 4.f);
		}
		return;
	}
	// Tier 5: the gem gathering light as the crest unfolds, until it bursts.
	if (State == EChestState::Unlocking && PreShare < 0.72f)
	{
		const float Build = FMath::Clamp(PreShare / 0.72f, 0.f, 1.f);
		const FVector Gem = SocketOr(Crest, GemSocket, Front + Up * 20.0);
		Effects->AddGlow(Gem, 6.f + 16.f * Build, ColorOf(EChestGlow::Gem), 1.f + 4.f * Build);
	}
}

void AChest::PlayShowMoment(uint32 Moment)
{
	++Show.MomentsPlayed;
	const FChestKindInfo Info = GetKindInfo();
	UWorld* World = GetWorld();
	FWeaponFX* Effects = EffectsOf(World);
	const FVector Front = GetInteractionLocation().Get(GetActorLocation());
	const FVector Up = GetActorUpVector();
	auto ColorOf = [this, &Info](EChestGlow Glow)
	{
		return Show.bLooksMade ? Show.BaseColor[static_cast<int32>(Glow)] : DefaultColor(Info.Tier, Glow);
	};
	auto At = [&Front](const UStaticMeshComponent* Piece)
	{
		return Piece && Piece->GetStaticMesh() ? Piece->GetComponentLocation() : Front;
	};
	switch (Moment)
	{
	case ChestShowMoment::LatchLeft:
	case ChestShowMoment::LatchRight:
		// A sprung latch snapping open; tier 1's tin a touch higher than tier 2's.
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::LatchPop, At(Moment == ChestShowMoment::LatchLeft ? LatchLeft : LatchRight), 1.f,
			Info.Tier == 1 ? 1.06f : 0.97f);
		break;
	case ChestShowMoment::LightFlare:
		if (Effects)
		{
			Effects->SpawnStarburst(SocketOr(Body, LightSocket, Front) + GetActorForwardVector() * 2.0, 16.f, ColorOf(EChestGlow::Main), 5.f, 0.22f, 6);
		}
		break;
	case ChestShowMoment::LockSpin:
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::LockSpin, At(Lock));
		break;
	case ChestShowMoment::LockOpen:
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::LockOpen, At(Lock));
		if (Effects)
		{
			const FVector Where = At(Lock) + GetActorForwardVector() * 2.0;
			Effects->SpawnStarburst(Where, 26.f, LockGreen, 7.f, 0.28f, 8);
			Effects->SpawnShockRing(Where, GetActorForwardVector(), 4.f, 28.f, 3.f, LockGreen, 4.f, 0.35f);
		}
		break;
	case ChestShowMoment::OrbPulse:
	{
		const FVector Orb = SocketOr(Lid, OrbSocket, SocketOr(Body, OrbSocket, Front + Up * 12.0));
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::OrbPulse, Orb);
		if (Effects)
		{
			Effects->SpawnShockRing(Orb, FVector::ZeroVector, 6.f, 45.f, 4.f, ColorOf(EChestGlow::Main), 4.f, 0.5f, /*bFacing*/ true);
		}
		break;
	}
	case ChestShowMoment::TrimSweep:
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::TrimSweep, Front);
		break;
	case ChestShowMoment::CrestUnfold:
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::CrestUnfold, At(Crest));
		break;
	case ChestShowMoment::Burst:
	{
		// The crest locked out: a burst of the gem's gold, a ring of it over the ground, and the short rising tone with it.
		const FVector Gem = SocketOr(Crest, GemSocket, Front + Up * 20.0);
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::Burst, Gem);
		LooterSound::PlayAt(this, LooterSoundCue::ChestTiers::Rise, Gem);
		if (Effects)
		{
			const FLinearColor Gold = ColorOf(EChestGlow::Gem);
			Effects->SpawnStarburst(Gem, 90.f, Gold, 12.f, 0.35f, 12);
			Effects->SpawnFireball(Gem, Up * 60.0, 20.f, 110.f, Gold, 2.2f, 0.6f);
			Effects->SpawnShockRing(Gem, FVector::ZeroVector, 10.f, 170.f, 10.f, Gold, 5.f, 0.5f, /*bFacing*/ true);
			Effects->SpawnShockRing(GetActorLocation() + Up * 4.0, Up, 30.f, 220.f, 16.f, Gold, 5.f, 0.7f);
			Effects->SpawnLootMotes(Gem, 60.f, Gold, 14);
		}
		break;
	}
	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Tier 5's fountain
// ---------------------------------------------------------------------------

void AChest::AdvanceFountain(float DeltaSeconds)
{
	Show.FountainClock += DeltaSeconds;
	while (HasFountainLeft() && Show.FountainClock >= ChestShow::FountainFirst + Show.FountainThrown * ChestShow::FountainEvery)
	{
		ThrowFountainPiece(Show.FountainThrown++);
	}
}

void AChest::ThrowPendingLoot()
{
	const UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}
	while (HasFountainLeft())
	{
		ThrowFountainPiece(Show.FountainThrown++);
	}
}

void AChest::ThrowFountainPiece(int32 Index)
{
	UWorld* World = GetWorld();
	if (!World || !Show.Fountain.IsValidIndex(Index))
	{
		return;
	}
	const FChestFountainPiece& Piece = Show.Fountain[Index];
	const FVector From = GetLootOrigin();
	EWeaponRarity Rarity = EWeaponRarity::Common;
	if (Piece.bWard)
	{
		if (AShieldPickup* Ward = AShieldPickup::SpawnWard(World, Piece.Ward, From))
		{
			Ward->Toss(Piece.Velocity);
			DroppedLoot.Add(Ward);
		}
		Rarity = Piece.Ward.Rarity;
	}
	else
	{
		const FTransform Spawn(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), From);
		if (AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(this, Piece.Weapon, Spawn))
		{
			Gun->Toss(Piece.Velocity);
			ULootFanfareSubsystem::ExpectLanding(Gun);
			DroppedLoot.Add(Gun);
		}
		Rarity = Piece.Weapon.Rarity;
	}
	// Each piece seen and heard leaving, a little brighter by its rarity (as a boss's shower pops).
	if (FWeaponFX* Effects = EffectsOf(World))
	{
		Effects->SpawnFlash(From, Piece.Velocity.GetSafeNormal(), 0.45f);
	}
	LooterSound::PlayAt(this, LooterSoundCue::BossLootPop, From, 1.f, 1.f + 0.06f * static_cast<float>(Rarity));
}
