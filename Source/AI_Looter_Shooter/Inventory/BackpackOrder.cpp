// FBackpackOrder: how the backpack's guns and spare wards interleave, in the order they went in.

#include "Inventory/BackpackOrder.h"

void FBackpackOrder::Add(EBackpackItem Kind)
{
	Items.Add(Kind);
}

int32 FBackpackOrder::RemoveAt(EBackpackItem Kind, int32 Index)
{
	const int32 Position = PositionOf(Kind, Index);
	if (Position != INDEX_NONE)
	{
		Items.RemoveAt(Position);
	}
	return Position;
}

void FBackpackOrder::Insert(EBackpackItem Kind, int32 Index, int32 Position)
{
	if (Position >= 0 && Position <= Items.Num())
	{
		// The place a thing just came out of is right only while the same number of its kind stand before it.
		int32 Before = 0;
		for (int32 Entry = 0; Entry < Position; ++Entry)
		{
			Before += Items[Entry] == Kind ? 1 : 0;
		}
		if (Before == Index)
		{
			Items.Insert(Kind, Position);
			return;
		}
	}
	const int32 Was = PositionOf(Kind, Index);
	if (Was != INDEX_NONE)
	{
		Items.Insert(Kind, Was);
	}
	else
	{
		Items.Add(Kind);
	}
}

int32 FBackpackOrder::Count(EBackpackItem Kind) const
{
	int32 Found = 0;
	for (const EBackpackItem Item : Items)
	{
		Found += Item == Kind ? 1 : 0;
	}
	return Found;
}

int32 FBackpackOrder::PositionOf(EBackpackItem Kind, int32 Index) const
{
	if (Index < 0)
	{
		return INDEX_NONE;
	}
	int32 Seen = 0;
	for (int32 Position = 0; Position < Items.Num(); ++Position)
	{
		if (Items[Position] == Kind)
		{
			if (Seen == Index)
			{
				return Position;
			}
			++Seen;
		}
	}
	return INDEX_NONE;
}

bool FBackpackOrder::Reconcile(int32 NumGuns, int32 NumWards)
{
	bool bChanged = false;
	auto Fit = [this, &bChanged](EBackpackItem Kind, int32 Wanted)
	{
		int32 Have = Count(Kind);
		// Too many: the last of that kind go (a path that took one out without saying which).
		for (int32 Position = Items.Num() - 1; Position >= 0 && Have > Wanted; --Position)
		{
			if (Items[Position] == Kind)
			{
				Items.RemoveAt(Position);
				--Have;
				bChanged = true;
			}
		}
		// Too few: the missing ones went in last.
		for (; Have < FMath::Max(Wanted, 0); ++Have)
		{
			Items.Add(Kind);
			bChanged = true;
		}
	};
	Fit(EBackpackItem::Gun, NumGuns);
	Fit(EBackpackItem::Ward, NumWards);
	return bChanged;
}

TArray<FBackpackItem> FBackpackOrder::Entries() const
{
	TArray<FBackpackItem> Out;
	Out.Reserve(Items.Num());
	int32 Guns = 0;
	int32 Wards = 0;
	for (const EBackpackItem Item : Items)
	{
		FBackpackItem& Entry = Out.AddDefaulted_GetRef();
		Entry.Kind = Item;
		Entry.Index = Item == EBackpackItem::Gun ? Guns++ : Wards++;
	}
	return Out;
}
