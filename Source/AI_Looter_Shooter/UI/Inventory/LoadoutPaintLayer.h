#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "LoadoutPaintLayer.generated.h"

class FSlateWindowElementList;

/** A see-through layer that its owner draws lines and shapes on (the inventory stands' rings). */
UCLASS()
class AI_LOOTER_SHOOTER_API ULoadoutPaintLayer : public UWidget
{
	GENERATED_BODY()

public:
	using FPainter = TFunction<void(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId)>;

	void SetPainter(FPainter InPainter) { Painter = MoveTemp(InPainter); }
	void Paint(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	FPainter Painter;
	TSharedPtr<SWidget> Layer;
};
