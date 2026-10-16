// UPlayerHUDWidget: the equipped ember power's socket (UHudEmberSocketWidget, left of the weapon slots): following the
// pawn's ember component, its denied presses (a shake or a dim) and its state (a pop as it comes off cooldown).

#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/HUD/HudEmberSocketWidget.h"
#include "Embers/EmberComponent.h"
#include "Settings/KeyBindingSubsystem.h"

namespace
{
	/** Seconds the weapon column stays fully visible after the power does something (PlayerHUDWidget.cpp's ActivityHold). */
	constexpr float EmberActivityHold = 3.f;
}

void UPlayerHUDWidget::BindEmbers(UEmberComponent* Embers)
{
	if (BoundEmbers.Get() == Embers)
	{
		return;
	}
	if (UEmberComponent* Old = BoundEmbers.Get())
	{
		Old->OnDenied.Remove(EmberDeniedHandle);
		Old->OnStateChanged.Remove(EmberStateHandle);
	}
	EmberDeniedHandle.Reset();
	EmberStateHandle.Reset();
	if (Embers)
	{
		EmberDeniedHandle = Embers->OnDenied.AddUObject(this, &UPlayerHUDWidget::HandleEmberDenied);
		EmberStateHandle = Embers->OnStateChanged.AddUObject(this, &UPlayerHUDWidget::HandleEmberState);
	}
	BoundEmbers = Embers;
}

void UPlayerHUDWidget::UpdateEmbers(float DeltaTime)
{
	if (EmberSocket)
	{
		EmberSocket->Update(BoundEmbers.Get(), BoundKeyName(UKeyBindingSubsystem::EmberBindingId(), TEXT("Q")), DeltaTime);
	}
	// It stands apart from the weapon cluster (shown with no gun in hand too), but fades back with it while it shows.
	if (EmberArea && WeaponCluster)
	{
		const bool bClusterShown = WeaponCluster->GetVisibility() != ESlateVisibility::Collapsed;
		EmberArea->SetRenderOpacity(bClusterShown ? WeaponCluster->GetRenderOpacity() : 1.f);
	}
}

void UPlayerHUDWidget::HandleEmberDenied(EEmberDenied Why)
{
	// The component plays the denied click itself: the socket only shakes or dims, and the column comes forward to show it.
	WeaponActivity = FMath::Max(WeaponActivity, EmberActivityHold);
	if (EmberSocket)
	{
		EmberSocket->Flash(Why);
	}
}

void UPlayerHUDWidget::HandleEmberState(EEmberState NewState)
{
	// A use, its end and coming off cooldown are all worth bringing the column forward for.
	WeaponActivity = FMath::Max(WeaponActivity, EmberActivityHold);
	if (EmberSocket)
	{
		EmberSocket->NotifyState(NewState);
	}
}
