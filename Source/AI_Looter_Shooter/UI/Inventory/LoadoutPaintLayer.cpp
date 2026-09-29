#include "UI/Inventory/LoadoutPaintLayer.h"
#include "Widgets/SLeafWidget.h"

class SLoadoutPaintLayer : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLoadoutPaintLayer) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, ULoadoutPaintLayer* InOwner)
	{
		Owner = InOwner;
		// What it draws moves every frame (the stand-in animates and turns).
		ForceVolatile(true);
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (const ULoadoutPaintLayer* Layer = Owner.Get())
		{
			Layer->Paint(AllottedGeometry, OutDrawElements, LayerId);
		}
		return LayerId + 1;
	}

private:
	TWeakObjectPtr<ULoadoutPaintLayer> Owner;
};

void ULoadoutPaintLayer::Paint(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const
{
	if (Painter)
	{
		Painter(Geometry, Elements, LayerId);
	}
}

TSharedRef<SWidget> ULoadoutPaintLayer::RebuildWidget()
{
	TSharedRef<SLoadoutPaintLayer> Widget = SNew(SLoadoutPaintLayer, this);
	Layer = Widget;
	return Widget;
}

void ULoadoutPaintLayer::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Layer.Reset();
}
