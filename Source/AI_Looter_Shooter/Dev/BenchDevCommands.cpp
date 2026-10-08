// Developer console commands for the gunsmith's bench and its parts box (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Bench/BenchRules.h"
#include "UI/HUD/LooterHUD.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponPartSwap.h"
#include "World/GunsmithBench.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	APawn* FindPlayerPawn(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		return Controller ? Controller->GetPawn() : nullptr;
	}

	UWeaponManagerComponent* FindPlayerWeapons(UWorld* World)
	{
		const APawn* Pawn = FindPlayerPawn(World);
		return Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	}

	/** The weapon definition whose asset name contains the text ("Shotgun" finds DA_PumpShotgun). */
	UWeaponDefinition* FindDefinition(const FString& Text)
	{
		TArray<FAssetData> Assets;
		IAssetRegistry::GetChecked().GetAssetsByClass(UWeaponDefinition::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Contains(Text))
			{
				return Cast<UWeaponDefinition>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	/** The kind of gun a part is added for: the one named, or else the gun in hand's (else the first gun carried). */
	UWeaponDefinition* KindFor(const UWeaponManagerComponent& Weapons, const FString* Named)
	{
		if (Named)
		{
			return FindDefinition(*Named);
		}
		if (const AWeaponBase* InHand = Weapons.GetActiveWeapon())
		{
			return InHand->GetInstance().Definition;
		}
		const TArray<FCarriedGun> Carried = BenchRules::CarriedGuns(Weapons);
		const FWeaponInstanceData* First = Carried.IsEmpty() ? nullptr : Weapons.FindCarriedGun(Carried[0]);
		return First ? First->Definition.Get() : nullptr;
	}

	/** Looter.Bench.Open: the bench's screen, with no bench. */
	void OpenScreen(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		ALooterHUD* HUD = Controller ? Cast<ALooterHUD>(Controller->GetHUD()) : nullptr;
		if (!HUD)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Bench.Open: no player HUD (start the game first)."));
			return;
		}
		if (!HUD->OpenBench(nullptr))
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Bench.Open: the bench's screen can't open now (the pause menu is up, or the player carries nothing)."));
		}
	}

	/** Looter.Bench.Spawn: a bench two meters in front of the player, its front toward them. */
	void SpawnBench(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APawn* Pawn = FindPlayerPawn(GameWorld);
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Bench.Spawn: no player (start the game first)."));
			return;
		}
		const FVector Forward = FRotator(0.0, Pawn->GetViewRotation().Yaw, 0.0).Vector();
		const FVector Feet = Pawn->GetActorLocation() - FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight());
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const AGunsmithBench* Bench = GameWorld->SpawnActor<AGunsmithBench>(Feet + Forward * 200.0, (-Forward).Rotation(), Params);
		UE_LOG(LogLooter, Log, TEXT("Looter.Bench.Spawn: %s in front of the player (%s; not saved)."), Bench ? *Bench->GetName() : TEXT("nothing"),
			Bench && Bench->HasModel() ? TEXT("its model") : TEXT("plain shapes until SM_GunsmithBench is imported"));
	}

	void ListBox(const UWeaponManagerComponent& Weapons)
	{
		const TArray<FBoxedWeaponPart>& Box = Weapons.GetPartsBox();
		UE_LOG(LogLooter, Log, TEXT("Looter.Bench.Box: %d / %d parts."), Box.Num(), UWeaponManagerComponent::MaxBoxedParts);
		for (int32 Index = 0; Index < Box.Num(); ++Index)
		{
			const FBoxedWeaponPart& Part = Box[Index];
			UE_LOG(LogLooter, Log, TEXT("  %2d  %s  %s=%s  (%s)"), Index, Part.Definition ? *Part.Definition->GetName() : TEXT("(no kind)"),
				*Part.Slot.ToString(), *Part.Key.ToString(), *BenchRules::PartName(Part));
		}
	}

	/** Adds the part Key in SlotName of Kind, or every part of that slot (or of every slot) for "all". */
	int32 AddParts(UWeaponManagerComponent& Weapons, UWeaponDefinition& Kind, const FString& SlotName, const FString& Key)
	{
		int32 Added = 0;
		for (const FWeaponPartSlot& Slot : Kind.Parts)
		{
			if (SlotName != TEXT("all") && !Slot.Name.ToString().Equals(SlotName, ESearchCase::IgnoreCase))
			{
				continue;
			}
			for (const FWeaponPartOption& Option : Slot.Options)
			{
				if (Key != TEXT("all") && !Option.Key.ToString().Equals(Key, ESearchCase::IgnoreCase))
				{
					continue;
				}
				FBoxedWeaponPart Part;
				Part.Definition = &Kind;
				Part.Slot = Slot.Name;
				Part.Key = Option.Key;
				if (!Weapons.AddToPartsBox(Part))
				{
					UE_LOG(LogLooter, Warning, TEXT("Looter.Bench.Box: the box is full (%d parts)."), UWeaponManagerComponent::MaxBoxedParts);
					return Added;
				}
				++Added;
			}
		}
		return Added;
	}

	/** Looter.Bench.Box [list|clear|add <slot|all> <key|all> [kind]]: the player's parts box. */
	void BoxCommand(const TArray<FString>& Args, UWorld* World)
	{
		UWeaponManagerComponent* Weapons = FindPlayerWeapons(FindGameWorld(World));
		if (!Weapons)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Bench.Box: no player with weapons (start the game first)."));
			return;
		}
		const FString Verb = Args.Num() > 0 ? Args[0].ToLower() : FString(TEXT("list"));
		if (Verb == TEXT("list"))
		{
			ListBox(*Weapons);
			return;
		}
		if (Verb == TEXT("clear"))
		{
			Weapons->ClearPartsBox();
			UE_LOG(LogLooter, Log, TEXT("Looter.Bench.Box: emptied."));
			return;
		}
		if (Verb == TEXT("add") && Args.Num() >= 3)
		{
			UWeaponDefinition* Kind = KindFor(*Weapons, Args.Num() > 3 ? &Args[3] : nullptr);
			if (!Kind)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Bench.Box: no kind of gun%s (name one, or carry a gun)."),
					Args.Num() > 3 ? *FString::Printf(TEXT(" matches '%s'"), *Args[3]) : TEXT(""));
				return;
			}
			const int32 Added = AddParts(*Weapons, *Kind, Args[1], Args[2].ToLower() == TEXT("all") ? FString(TEXT("all")) : Args[2]);
			UE_LOG(LogLooter, Log, TEXT("Looter.Bench.Box: added %d %s part(s); %d / %d in the box."), Added, *Kind->GetName(),
				Weapons->GetPartsBox().Num(), UWeaponManagerComponent::MaxBoxedParts);
			if (Added == 0)
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.Bench.Box: %s has no part %s=%s."), *Kind->GetName(), *Args[1], *Args[2]);
			}
			return;
		}
		UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Bench.Box [list|clear|add <slot|all> <key|all> [kind]]"));
	}

	FAutoConsoleCommandWithWorldAndArgs OpenScreenCommand(
		TEXT("Looter.Bench.Open"),
		TEXT("Looter.Bench.Open: opens the gunsmith's bench's screen for the player's guns, with no bench."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&OpenScreen));

	FAutoConsoleCommandWithWorldAndArgs SpawnBenchCommand(
		TEXT("Looter.Bench.Spawn"),
		TEXT("Looter.Bench.Spawn: a gunsmith's bench two meters in front of the player, its front toward them (not saved)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnBench));

	FAutoConsoleCommandWithWorldAndArgs BoxCommandRegistration(
		TEXT("Looter.Bench.Box"),
		TEXT("Looter.Bench.Box [list|clear|add <slot|all> <key|all> [kind]]: lists or empties the player's parts box, or adds parts to it ")
		TEXT("(of the gun in hand's kind unless a kind is named: \"add Barrel Marksman\", \"add all all Shotgun\")."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&BoxCommand));
}

#endif
