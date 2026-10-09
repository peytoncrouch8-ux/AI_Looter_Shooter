// UControlHintSubsystem: what it sees of the player at each look, for FControlHintRules. Everything is read from the
// player's own components (movement, locomotion, view, weapons) and the HUD; the three probes (a ledge, a far target, a
// bench) only run while their hint could still show, so a player who knows the game costs a few comparisons a look.

#include "Tutorial/ControlHintSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionTargets.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Scenes/SceneSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/GunsmithBench.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Faster than this between looks (cm/s) is no walk: a respawn, a trip's landing, a scene putting the player somewhere. */
	constexpr float TeleportSpeed = 3000.f;

	/** Moving at least this fast (cm/s) on the ground is walking, not shuffling into place. */
	constexpr float WalkSpeed = 100.f;

	/** Leaving the ground this fast upward (cm/s) is a jump, not a step off an edge. */
	constexpr float JumpSpeed = 150.f;

	/** The ledge probe's two heights over the feet (cm): blocked at the knee, open over the chest. */
	constexpr float KneeHeight = 40.f;
	constexpr float OverChestHeight = 170.f;

	/** A face steeper than this (its normal's Z) is a wall or a fence, not a slope; a top flatter than TopFlatness is one to stand on. */
	constexpr float WallSteepness = 0.5f;
	constexpr float TopFlatness = 0.7f;

	/** How far past the ledge's face the look for its top comes down (cm). */
	constexpr float TopInset = 20.f;

	/** The melee hint looks twice as far as a strike reaches (UPlayerMeleeComponent::FindTargetInReach), so it comes a step early. */
	constexpr float MeleeHintReachScale = 2.f;

	/** What the ledge probe sees: anything solid placed in the level, static or moving. */
	FCollisionObjectQueryParams SolidObjects()
	{
		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_WorldStatic);
		Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
		return Objects;
	}
}

FControlHintInput UControlHintSubsystem::Look(float DeltaSeconds)
{
	FControlHintInput In;
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
	In.bEnabled = !Settings || Settings->AreControlHintsShown();

	// The menus the hints teach are seen whatever else holds the player.
	const ALooterHUD* HUD = ALooterHUD::FindFor(Controller);
	In.bInventoryOpen = HUD && HUD->IsInventoryOpen();
	In.bBenchOpen = HUD && HUD->IsBenchOpen();

	ACharacter* Character = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
	if (!Character)
	{
		LastPawn.Reset();
		return In;
	}
	const FVector Location = Character->GetActorLocation();
	if (LastPawn.Get() != Character)
	{
		// A new pawn (the level's first, a respawn): its readings start here, so arriving isn't a walk, a pickup or a swap.
		LastPawn = Character;
		LastLocation = Location;
		bWasOnGround = true;
		LastGroundZ = Location.Z;
		LastGunsCarried = INDEX_NONE;
		LastSlot = INDEX_NONE;
		LastMeleeCount = INDEX_NONE;
	}

	const UHealthComponent* Health = Character->FindComponentByClass<UHealthComponent>();
	const bool bAlive = !Health || !Health->IsDead();
	In.bInControl = bAlive && !(HUD && HUD->IsMenuOpen()) && !USceneSubsystem::HidesGameplayHUD(World) && !World->IsPaused();

	// --- Moving ---
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const bool bOnGround = Movement && Movement->IsMovingOnGround();
	const float Speed = Movement ? static_cast<float>(Movement->Velocity.Size2D()) : 0.f;
	const float Step = static_cast<float>(FVector::Dist2D(Location, LastLocation));
	LastLocation = Location;
	if (In.bInControl && bOnGround && Step <= TeleportSpeed * FMath::Max(DeltaSeconds, UE_KINDA_SMALL_NUMBER))
	{
		In.Moved = Step;
	}
	const UPlayerLocomotionComponent* Locomotion = Character->FindComponentByClass<UPlayerLocomotionComponent>();
	In.bSprinting = In.bInControl && bOnGround && Speed > WalkSpeed && Locomotion && Locomotion->GetSprintAlpha() > 0.5f;
	In.bSliding = In.bInControl && Locomotion && Locomotion->IsSliding();
	In.bWalking = In.bInControl && bOnGround && Speed > WalkSpeed && !In.bSprinting;

	// --- Jumping and climbing: off the ground going up, or down again a good step higher than where it was left ---
	if (In.bInControl)
	{
		const bool bTookOff = bWasOnGround && !bOnGround && Movement && Movement->Velocity.Z > JumpSpeed;
		const bool bClimbed = !bWasOnGround && bOnGround && Location.Z - LastGroundZ >= ClimbRise;
		In.bJumped = bTookOff || bClimbed || Character->bPressedJump;
	}
	if (bOnGround)
	{
		LastGroundZ = Location.Z;
	}
	bWasOnGround = bOnGround;

	// --- The gun in hand ---
	UWeaponManagerComponent* Weapons = Character->FindComponentByClass<UWeaponManagerComponent>();
	const AWeaponBase* Gun = Weapons ? Weapons->GetActiveWeapon() : nullptr;
	In.bGunInHand = Gun != nullptr;
	if (Gun)
	{
		const int32 MagazineSize = FMath::Max(Gun->GetStats().MagazineSize, 1);
		const int32 LowAt = FMath::FloorToInt32(static_cast<float>(MagazineSize) * FControlHintRules::LowMagazineShare);
		In.bReloading = Gun->IsReloading();
		In.bMagazineLow = Gun->GetReserveAmmo() > 0 && Gun->GetCurrentMagazine() <= LowAt;
	}
	const UPlayerViewComponent* View = Character->FindComponentByClass<UPlayerViewComponent>();
	In.bAiming = View && View->IsAiming();

	// --- Guns carried: a pickup past the first, and a swap that wasn't one ---
	In.GunsEquipped = Weapons ? Weapons->GetWeapons().Num() : 0;
	const int32 Carried = In.GunsEquipped + (Weapons ? Weapons->GetBackpack().Num() : 0);
	In.bPickedUpGun = LastGunsCarried != INDEX_NONE && Carried > LastGunsCarried && Carried >= 2;
	const int32 Slot = Weapons ? Weapons->GetActiveSlot() : INDEX_NONE;
	// A gun taken in hand as it's picked up (holding Interact) changes the slot too, but teaches nothing about swapping.
	In.bSwapped = LastSlot != INDEX_NONE && Slot != INDEX_NONE && Slot != LastSlot && In.GunsEquipped >= 2 && !In.bPickedUpGun
		&& Carried == LastGunsCarried;
	LastGunsCarried = Carried;
	LastSlot = Slot;

	// --- Striking: a strike since the last look teaches the key (the swing is 0.6 s apart, so no look misses one) ---
	const UPlayerMeleeComponent* Melee = UPlayerMeleeComponent::Find(Character);
	const int32 MeleeCount = Melee ? Melee->GetMeleeCount() : 0;
	In.bMeleed = LastMeleeCount != INDEX_NONE && MeleeCount > LastMeleeCount;
	LastMeleeCount = MeleeCount;

	// --- The probes, only while their hint could still show ---
	if (In.bInControl)
	{
		if (!Rules.IsRetired(EControlHint::Melee) && Melee && !Melee->HasMeleed())
		{
			// A live creature close in front, in the strike's cone and in sight (the strike's own look, at twice its reach).
			const AActor* Close = Melee->FindTargetInReach(MeleeHintReachScale);
			In.bCloseTarget = Close && Close->IsA<ACreatureBase>() && MissionTargets::IsAlive(*Close);
		}
		if (!Rules.IsRetired(EControlHint::Jump) && bOnGround && Speed > WalkSpeed && Movement)
		{
			In.bLedgeAhead = ProbeLedge(*Character, Movement->Velocity);
		}
		if (!Rules.IsRetired(EControlHint::Aim) && Gun && !In.bAiming)
		{
			In.bFarTarget = ProbeFarTarget(*Controller, *Character, *Gun);
		}
		if (!Rules.IsRetired(EControlHint::Bench) && Weapons && Carried >= 2)
		{
			In.bAtBenchWithPair = IsAtBenchWithPair(*Character, *Weapons);
		}
	}
	return In;
}

bool UControlHintSubsystem::ProbeLedge(const ACharacter& Character, const FVector& Heading) const
{
	const UWorld* World = GetWorld();
	const FVector Forward = Heading.GetSafeNormal2D();
	if (!World || Forward.IsNearlyZero())
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
	const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 30.f;
	const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 80.f;
	const FVector Feet = Character.GetActorLocation() - FVector(0.0, 0.0, HalfHeight);
	const FVector Ahead = Forward * (Radius + LedgeReach);
	// The level's solid pieces: volumes, the playable area's invisible walls and the scatter's grass are no ledges.
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("ControlHintLedge"), &Character, /*bTraceComplex*/ false);
	const FCollisionObjectQueryParams Objects = SolidObjects();

	// Something steep at knee height ahead (a wall, a fence, a crate's side; not a slope)...
	FHitResult Low;
	const FVector LowStart = Feet + FVector(0.0, 0.0, KneeHeight);
	if (!World->LineTraceSingleByObjectType(Low, LowStart, LowStart + Ahead, Objects, Params) || Low.ImpactNormal.Z > WallSteepness)
	{
		return false;
	}
	// ...open over the chest (a building's wall would block it too)...
	FHitResult High;
	const FVector HighStart = Feet + FVector(0.0, 0.0, OverChestHeight);
	if (World->LineTraceSingleByObjectType(High, HighStart, HighStart + Ahead, Objects, Params))
	{
		return false;
	}
	// ...and a top to stand on just past its face, from knee to chest height.
	const FVector Over = Low.ImpactPoint + Forward * TopInset;
	FHitResult Top;
	if (!World->LineTraceSingleByObjectType(Top, FVector(Over.X, Over.Y, HighStart.Z), FVector(Over.X, Over.Y, LowStart.Z - 10.0), Objects, Params))
	{
		return false;
	}
	const float Height = static_cast<float>(Top.ImpactPoint.Z - Feet.Z);
	return Top.ImpactNormal.Z >= TopFlatness && Height >= LedgeMinHeight && Height <= LedgeMaxHeight;
}

bool UControlHintSubsystem::ProbeFarTarget(const APlayerController& Controller, const APawn& Pawn, const AWeaponBase& Gun) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	// Along the crosshair, on the gun's own channel: what a shot would hit (pawn capsules let it through to the bodies).
	FVector Eye = FVector::ZeroVector;
	FRotator Facing = FRotator::ZeroRotator;
	Controller.GetPlayerViewPoint(Eye, Facing);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ControlHintTarget), /*bTraceComplex*/ false, &Pawn);
	Params.AddIgnoredActor(&Gun);
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Eye, Eye + Facing.Vector() * TargetProbeLength, Gun.TraceChannel, Params))
	{
		return false;
	}
	const AActor* Target = Hit.GetActor();
	if (!Target || Target == &Pawn || !Target->FindComponentByClass<UHealthComponent>() || !MissionTargets::IsAlive(*Target))
	{
		return false;
	}
	return FVector::Dist(Pawn.GetActorLocation(), Hit.ImpactPoint) > FControlHintRules::FarTargetDistance;
}

bool UControlHintSubsystem::IsAtBenchWithPair(const APawn& Pawn, const UWeaponManagerComponent& Weapons)
{
	// Two guns of one kind among the slots and the backpack: the bench trades parts between guns of a kind.
	TArray<const UObject*, TInlineAllocator<16>> Kinds;
	bool bPair = false;
	auto Count = [&Kinds, &bPair](const UObject* Kind)
	{
		if (Kind)
		{
			bPair |= Kinds.Contains(Kind);
			Kinds.Add(Kind);
		}
	};
	for (const AWeaponBase* Gun : Weapons.GetWeapons())
	{
		Count(Gun ? Gun->GetInstance().Definition.Get() : nullptr);
	}
	for (const FWeaponInstanceData& Item : Weapons.GetBackpack())
	{
		Count(Item.Definition.Get());
	}
	if (!bPair)
	{
		return false;
	}
	if (!bBenchesFound)
	{
		// Benches are placed with the level and never move: found once.
		bBenchesFound = true;
		for (TActorIterator<AGunsmithBench> It(GetWorld()); It; ++It)
		{
			Benches.Add(*It);
		}
	}
	const FVector Where = Pawn.GetActorLocation();
	for (const TWeakObjectPtr<AActor>& Bench : Benches)
	{
		if (Bench.IsValid() && FVector::Dist(Bench->GetActorLocation(), Where) <= BenchDistance)
		{
			return true;
		}
	}
	return false;
}
