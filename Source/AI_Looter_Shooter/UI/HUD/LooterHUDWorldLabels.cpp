// ALooterHUD: the labels drawn over the world (creature tags, loot labels, damage numbers, barks, the givers' ! and ?),
// hidden under menus and scenes.

#include "UI/HUD/LooterHUD.h"
#include "UI/World/GiverMarkSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Slate/SGameLayerManager.h"

namespace
{
	/** The layer UWidgetComponent puts every screen-space widget component of a player on (its default SharedLayerName). */
	const FName WorldLabelsLayer(TEXT("WidgetComponentScreenLayer"));
}

bool ALooterHUD::ShowWorldLabels(IGameLayerManager& Layers, ULocalPlayer* Player, bool bVisible)
{
	const TSharedPtr<IGameLayer> Layer = Player ? Layers.FindLayerForPlayer(Player, WorldLabelsLayer) : nullptr;
	if (!Layer.IsValid())
	{
		return false;
	}
	const TSharedRef<SWidget> Widget = Layer->AsWidget();
	const EVisibility Wanted = bVisible ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	if (Widget->GetVisibility() != Wanted)
	{
		Widget->SetVisibility(Wanted);
	}
	return true;
}

void ALooterHUD::SetWorldLabelsVisible(bool bVisible)
{
	// Creature tags, loot labels and damage numbers are widget components drawn in screen space: the engine puts them on
	// one layer of their own, in front of every menu. A menu covers the game, so the layer goes with it. The layer is the
	// viewport's and outlives the level: a Save & Quit (from the pause menu) or a trip (from the map or a station board)
	// leaves with it hidden, and a HUD that remembered "shown" for itself never showed it again in the next level, so no
	// damage number, tag or label was seen from then on. Asked every frame, it follows what the layer shows now. The local
	// player is the game's first when the controller is already going (the HUD's EndPlay as the level changes).
	APlayerController* PC = GetOwningPlayerController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LocalPlayer)
	{
		const UGameInstance* GameInstance = GetGameInstance();
		LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	}
	const UGameViewportClient* Viewport = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
	const TSharedPtr<IGameLayerManager> LayerManager = Viewport ? Viewport->GetGameLayerManager() : nullptr;
	if (LayerManager.IsValid())
	{
		ShowWorldLabels(*LayerManager, LocalPlayer, bVisible);
	}
	// The givers' ! and ? float on the same layer (AGiverMarkActor), so they go with it; a mission newly offered nearby
	// waits to chime until they can be seen again.
	if (UGiverMarkSubsystem* GiverMarks = UGiverMarkSubsystem::Get(this))
	{
		GiverMarks->SetLabelsShown(bVisible);
	}
}
