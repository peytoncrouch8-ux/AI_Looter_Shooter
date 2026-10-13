// ULostLootWidget: opening at the window (the input and the mouse taken, nothing fired behind it) and closing (the shutter
// down, the game's input back), staying up only while the player stays at the window, and what it reads: the weapon
// manager, the wallet, the shelves, the chosen ticket.

#include "UI/World/LostLootWidget.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "World/LostPropertyWindow.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The one screen up at a time. */
	TWeakObjectPtr<ULostLootWidget> OpenScreen;
}

ULostLootWidget* ULostLootWidget::OpenFor(APlayerController* InPlayer, ALostPropertyWindow* InWindow)
{
	APawn* Pawn = InPlayer ? InPlayer->GetPawn() : nullptr;
	if (!InPlayer || !InPlayer->IsLocalController() || !Pawn)
	{
		return nullptr;
	}
	if (ULostLootWidget* Already = OpenScreen.Get())
	{
		Already->Close();
	}
	ULostLootWidget* Screen = CreateWidget<ULostLootWidget>(InPlayer, ULostLootWidget::StaticClass());
	if (!Screen)
	{
		return nullptr;
	}
	Screen->Player = InPlayer;
	UWeaponManagerComponent* Weapons = Pawn->FindComponentByClass<UWeaponManagerComponent>();
	// Nothing fires behind the counter.
	if (Weapons)
	{
		Weapons->StopFire();
	}
	Screen->Open(InWindow, Weapons);
	// Over the gameplay HUD (its floating pieces dim under the backdrop), with the keyboard and the mouse.
	Screen->AddToViewport(26);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Screen->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InPlayer->SetInputMode(InputMode);
	InPlayer->SetShowMouseCursor(true);
	OpenScreen = Screen;
	return Screen;
}

ULostLootWidget* ULostLootWidget::GetOpen()
{
	return OpenScreen.Get();
}

void ULostLootWidget::Open(ALostPropertyWindow* InWindow, UWeaponManagerComponent* InManager)
{
	Window = InWindow;
	bAtWindow = InWindow != nullptr;
	Manager = InManager;
	Selected = INDEX_NONE;
	CardTicket = INDEX_NONE;
	bConfirmOpen = false;
	bRefreshWanted = false;
	bStamping = false;
	StampAge = 0.f;
	StatusWords = FText::GetEmpty();
	StatusColor = LooterUI::Color::TextDim();
	ShownDevice = LooterUI::GetMenuDevice();

	// A gun handed in while the screen is up (its five minutes out somewhere) shows at once.
	if (ULostLootSubsystem* Old = BoundLostLoot.Get())
	{
		Old->OnChanged.Remove(StoreChangedHandle);
	}
	ULostLootSubsystem* LostLoot = GetLostLoot();
	BoundLostLoot = LostLoot;
	StoreChangedHandle = LostLoot ? LostLoot->OnChanged.AddUObject(this, &ULostLootWidget::HandleStoreChanged) : FDelegateHandle();

	// The purse as it stands, with no flash.
	PurseTarget = UWalletSubsystem::SilverOf(this);
	ShownSilver = static_cast<float>(PurseTarget);
	PurseFlash = 0.f;

	// Opened before it was first shown, the panel is built as the widget is.
	if (WidgetTree && WidgetTree->RootWidget)
	{
		CloseConfirm();
		BuildPanel();
	}
}

void ULostLootWidget::Close()
{
	if (OpenScreen.Get() == this)
	{
		OpenScreen.Reset();
	}
	if (ULostLootSubsystem* LostLoot = BoundLostLoot.Get())
	{
		LostLoot->OnChanged.Remove(StoreChangedHandle);
	}
	BoundLostLoot.Reset();
	StickRepeat.Stop();
	const bool bWasShown = IsInViewport();
	RemoveFromParent();
	if (APlayerController* PC = Player.Get())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
	if (bWasShown)
	{
		// The shutter comes down at the window; opened from the console, the screen just closes.
		if (const ALostPropertyWindow* At = Window.Get())
		{
			At->PlayShutter(false);
		}
		else
		{
			PlayCue(LooterSoundCue::Close);
		}
	}
}

void ULostLootWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// The screen holds the keyboard (Escape closes it); its rows and buttons never take it.
	SetIsFocusable(true);
}

void ULostLootWidget::NativeDestruct()
{
	if (ULostLootSubsystem* LostLoot = BoundLostLoot.Get())
	{
		LostLoot->OnChanged.Remove(StoreChangedHandle);
	}
	BoundLostLoot.Reset();
	if (OpenScreen.Get() == this)
	{
		OpenScreen.Reset();
	}
	Super::NativeDestruct();
}

bool ULostLootWidget::ShouldClose() const
{
	// A test's screen, or one opened with no player, answers to nobody's feet.
	const APlayerController* PC = Player.Get();
	if (!PC)
	{
		return false;
	}
	const APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		return true;
	}
	const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (Health && Health->IsDead())
	{
		return true;
	}
	// Opened at a window, it stays up while the player stays at it.
	if (bAtWindow)
	{
		const ALostPropertyWindow* At = Window.Get();
		if (!At)
		{
			return true;
		}
		const FVector Spot = At->GetInteractionLocation().Get(At->GetActorLocation());
		return FVector::Dist2D(Pawn->GetActorLocation(), Spot) > StayRadius;
	}
	return false;
}

UWeaponManagerComponent* ULostLootWidget::GetManager() const
{
	if (UWeaponManagerComponent* Held = Manager.Get())
	{
		return Held;
	}
	const APlayerController* PC = Player.Get();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
}

UWalletSubsystem* ULostLootWidget::GetWallet() const
{
	return UWalletSubsystem::Get(this);
}

ULostLootSubsystem* ULostLootWidget::GetLostLoot() const
{
	return ULostLootSubsystem::Get(this);
}

int32 ULostLootWidget::GetPlayerLevel() const
{
	return UWalletSubsystem::PlayerLevelOf(this);
}

const FLostLootEntry* ULostLootWidget::GetChosenEntry() const
{
	const ULostLootSubsystem* LostLoot = GetLostLoot();
	if (!LostLoot || !Rows.IsValidIndex(Selected))
	{
		return nullptr;
	}
	const FLostLootStore& Store = LostLoot->GetStore();
	const int32 Index = Store.FindTicket(Rows[Selected].Ticket);
	return Store.Entries.IsValidIndex(Index) ? &Store.Entries[Index] : nullptr;
}
