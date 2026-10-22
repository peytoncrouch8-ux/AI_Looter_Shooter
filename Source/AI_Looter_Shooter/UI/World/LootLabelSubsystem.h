#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LootLabelSubsystem.generated.h"

class UUserWidget;

/**
 * Keeps the floating labels of loot lying near each other apart on screen: guns' (AWeaponBase, UWeaponLabelWidget) and
 * wards' (AShieldPickup, UShieldLabelWidget) alike, so at an opened chest a ward's label never draws over a gun's card
 * (play-test 2026-10-22). Every frame it projects the labels showing for the local player, lays them out with
 * LootLabelLayout::Arrange (the looked-at one stays put, the others step just above or below it) and eases each widget's
 * render translation there. The labels' own components and visibility stay their owners'. Looter.LootLabels.* cvars.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULootLabelSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

private:
	/** Puts a label back where its component draws it. */
	void Release(UUserWidget* Widget);

	/** The shift each label shows now (widget units, negative up), eased toward the layout's. */
	TMap<TWeakObjectPtr<UUserWidget>, float> Shown;
};
