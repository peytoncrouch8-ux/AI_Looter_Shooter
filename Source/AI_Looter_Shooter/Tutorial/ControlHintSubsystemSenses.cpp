// UControlHintSubsystem: what it sees of the player at each look, for FControlHintRules. Everything is read from the
// player's own components (movement, locomotion, view, weapons, interaction), the HUD, the missions and the captions; the
// looks into the world (a ledge, a target, a bench: ControlHintSubsystemProbes.cpp) only run while their hint could still
// show, so a player who knows the game costs a few comparisons a look.

#include "Tutorial/ControlHintSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/MissionTargets.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerMeleeComponent.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Scenes/SceneSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
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

	/** The melee hint looks twice as far as a strike reaches (UPlayerMeleeComponent::FindTargetInReach), so it comes a step early. */
	constexpr float MeleeHintReachScale = 2.f;
}

FControlHintInput UControlHintSubsystem::Look(float DeltaSeconds)
{
	FControlHintInput In;
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
	In.bEnabled = !Settings || Settings->AreControlHintsShown();
	In.bCaptionsOn = IsSomeoneTalking(World);

	// The menus the hints teach are seen whatever else holds the player: the loadout, the Ledger, the Missions page, the map.
	const ALooterHUD* HUD = ALooterHUD::FindFor(Controller);
	const bool bInventory = HUD && HUD->IsInventoryOpen();
	const EInventoryPage Page = HUD ? HUD->GetInventoryPage() : EInventoryPage::Loadout;
	In.bInventoryOpen = bInventory && Page == EInventoryPage::Loadout;
	In.bLedgerOpen = bInventory && Page == EInventoryPage::Bestiary;
	In.bMissionsOpen = bInventory && Page == EInventoryPage::Missions;
	In.bMapOpen = bInventory && Page == EInventoryPage::Map;
	In.bBenchOpen = HUD && HUD->IsBenchOpen();

	// What the Interact key used since the last look, menus or not.
	In.bInteracted = bUsedSinceLook;
	In.bHeldUse = bHeldSinceLook;
	bUsedSinceLook = false;
	bHeldSinceLook = false;

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
		LastThrowCount = INDEX_NONE;
		LastGrenades = INDEX_NONE;
		LastGun.Reset();
		LastMagazine = INDEX_NONE;
		BindInteraction(Character);
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
	In.bCrouching = In.bInControl && Locomotion && Locomotion->IsCrouching() && !Locomotion->IsSliding();
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

	// --- The gun in hand: low, reloading, and fired (the trigger held, or a round gone from the same gun's magazine) ---
	UWeaponManagerComponent* Weapons = Character->FindComponentByClass<UWeaponManagerComponent>();
	const AWeaponBase* Gun = Weapons ? Weapons->GetActiveWeapon() : nullptr;
	In.bGunInHand = Gun != nullptr;
	const int32 Magazine = Gun ? Gun->GetCurrentMagazine() : INDEX_NONE;
	if (Gun)
	{
		const int32 MagazineSize = FMath::Max(Gun->GetStats().MagazineSize, 1);
		const int32 LowAt = FMath::FloorToInt32(static_cast<float>(MagazineSize) * FControlHintRules::LowMagazineShare);
		In.bReloading = Gun->IsReloading();
		In.bMagazineLow = Gun->GetReserveAmmo() > 0 && Gun->GetCurrentMagazine() <= LowAt;
		const bool bRoundGone = LastGun.Get() == Gun && LastMagazine != INDEX_NONE && Magazine < LastMagazine && !In.bReloading;
		In.bFired = (In.bInControl && Gun->WantsToFire()) || bRoundGone;
	}
	LastGun = Gun;
	LastMagazine = Magazine;
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

	// --- Throwing: a throw since the last look teaches the key (0.8 s apart at the quickest, so no look misses one) ---
	const UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(Character);
	const int32 ThrowCount = Throw ? Throw->GetThrowCount() : 0;
	In.bThrew = LastThrowCount != INDEX_NONE && ThrowCount > LastThrowCount;
	LastThrowCount = ThrowCount;
	// Grenades picked up (a salt tin) before the first throw: the hint's moment even with no crowd about. A count put back
	// as the level begins is no pickup: the first look of a pawn only notes it.
	const int32 Grenades = Throw ? Throw->GetGrenades() : 0;
	In.bGrenadesGained = In.bInControl && LastGrenades != INDEX_NONE && Grenades > LastGrenades && Throw && !Throw->HasThrown();
	LastGrenades = Grenades;

	// --- The Interact key: something to use in front (its prompt is up), by a press, a hold or both ---
	const UInteractionComponent* Interaction = BoundInteraction.Get();
	if (In.bInControl && Interaction && Interaction->GetFocusedActor())
	{
		const FInteractionOptions& Options = Interaction->GetFocusedOptions();
		In.bTapFocused = Options.bUsable && Options.bTap;
		In.bHoldFocused = Options.bUsable && Options.bHold;
	}

	// --- The missions: how many are going, the page the tracked one asks for, how far off its objective is ---
	if (In.bInControl)
	{
		LookAtMissions(*Character, In);
	}

	// --- The probes, only while their hint could still show ---
	if (In.bInControl)
	{
		if (!Rules.IsRetired(EControlHint::Grenade) && Throw && Throw->GetGrenades() > 0 && !Throw->HasThrown())
		{
			// A crowd worth a grenade: two or more live creatures ahead, in sight, within a throw.
			In.bCrowdAhead = Throw->CountTargetsAhead() >= 2;
		}
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
		// Something under the crosshair: to fire at (before the first shot), to aim at from afar, to crouch for.
		const bool bWantsTarget = !Rules.IsRetired(EControlHint::Fire) || (!Rules.IsRetired(EControlHint::Aim) && !In.bAiming)
			|| (!Rules.IsRetired(EControlHint::Crouch) && In.bAiming);
		if (bWantsTarget && Gun)
		{
			const float Distance = ProbeTarget(*Controller, *Character, *Gun);
			In.bTargetInSights = Distance >= 0.f;
			In.bFarTarget = Distance > FControlHintRules::FarTargetDistance;
		}
		if (!Rules.IsRetired(EControlHint::Bench) && Weapons && Carried >= 2)
		{
			In.bAtBenchWithPair = IsAtBenchWithPair(*Character, *Weapons);
		}
	}
	return In;
}

void UControlHintSubsystem::LookAtMissions(const APawn& Pawn, FControlHintInput& In) const
{
	const UWorld* World = GetWorld();
	const UMissionSubsystem* Book = World ? World->GetSubsystem<UMissionSubsystem>() : nullptr;
	if (!Book)
	{
		return;
	}
	In.MissionsGoing = Book->GetMissions().Num();
	FVector Waypoint = FVector::ZeroVector;
	if (!Rules.IsRetired(EControlHint::Map) && Book->GetTrackedWaypoint(Waypoint))
	{
		In.bFarWaypoint = FVector::Dist2D(Waypoint, Pawn.GetActorLocation()) > FControlHintRules::MapWaypointDistance;
	}

	// The page the tracked mission's step asks to open (Main 2's "Open the Ledger"), read from its objectives.
	if (Rules.IsRetired(EControlHint::Inventory) && Rules.IsRetired(EControlHint::Ledger) && Rules.IsRetired(EControlHint::Missions))
	{
		return;
	}
	const UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	const FName Tracked = Runner && Runner->IsActive() ? Runner->GetTrackedMission() : NAME_None;
	const UMissionDefinition* Mission = Tracked.IsNone() ? nullptr : Runner->FindDefinition(Tracked);
	const int32 Step = Mission ? Runner->GetStep(Tracked) : INDEX_NONE;
	if (!Mission || !Mission->Steps.IsValidIndex(Step))
	{
		return;
	}
	for (int32 Index = 0; Index < Mission->Steps[Step].Objectives.Num(); ++Index)
	{
		const UMissionOpenPageObjective* Open = Cast<UMissionOpenPageObjective>(Mission->GetObjective(Step, Index));
		if (!Open)
		{
			continue;
		}
		switch (Open->Page)
		{
		case EMissionPage::Bestiary: In.bAskedLedger = true; break;
		case EMissionPage::Missions: In.bAskedMissions = true; break;
		case EMissionPage::Loadout:
		case EMissionPage::Any:      In.bAskedInventory = true; break;
		}
	}
}

void UControlHintSubsystem::BindInteraction(APawn* Pawn)
{
	UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	if (Interaction == BoundInteraction.Get())
	{
		return;
	}
	if (UInteractionComponent* Old = BoundInteraction.Get())
	{
		Old->OnInteracted.RemoveDynamic(this, &UControlHintSubsystem::HandleInteracted);
	}
	BoundInteraction = Interaction;
	if (Interaction)
	{
		Interaction->OnInteracted.AddUniqueDynamic(this, &UControlHintSubsystem::HandleInteracted);
	}
	bUsedSinceLook = false;
	bHeldSinceLook = false;
}

void UControlHintSubsystem::HandleInteracted(AActor* Target, bool bHeld)
{
	// Taken at the next look: a tap teaches the key, a hold run its full time teaches holding it too.
	bUsedSinceLook = true;
	bHeldSinceLook = bHeldSinceLook || bHeld;
}
