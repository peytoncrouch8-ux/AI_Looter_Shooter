#include "Weapons/WeaponModelComponent.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/WeaponNotches.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/**
	 * Where a part takes the tally: a row from Start to End in the part's own space (cm; X along the gun toward the muzzle,
	 * Z up, the side faces at +-Y), the cuts Height tall across it, cut only into surfaces at least MinDepth from the
	 * part's middle (its outside, not a window's insides or an ejection port's bolt). The row starts at its rear end.
	 */
	struct FTallyRow
	{
		FVector2f Start = FVector2f::ZeroVector;
		FVector2f End = FVector2f::ZeroVector;
		float Height = 0.f;
		float MinDepth = 0.f;
	};

	/**
	 * The rows, by part mesh, laid out from the guns' Blender scripts (Art/Models/Weapons/<Gun>.py: u along the gun is
	 * +X, v up is +Z, and each part's origin is where it attaches) so they clear each part's plates, rivets, ports,
	 * windows, cuffs and cheek pads. A part that isn't listed gets a row fitted to its size (FitRow): add a row here with
	 * a new stock or body.
	 *
	 * The bullpup's stocks are only butt pads, so its tally goes on the body: on the side plates of the bodies that have
	 * them (behind the grip, under the rivet rows), on the plain shell of the others. The Ranchhand's goes on its stock,
	 * behind the cuff or under the cheek pad; on the pistol grips (Raider, Folding) it runs down the grip.
	 */
	struct FTallyPlacement
	{
		const TCHAR* Mesh;
		FTallyRow Row;
	};

	const FTallyPlacement Placements[] = {
		// Bullpup bodies (origin at the back of the shell, on the bore line).
		{ TEXT("SM_BullpupBody_Standard"), { { 10.0f, -1.45f }, { 18.0f, -1.45f }, 2.6f, 2.4f } },
		{ TEXT("SM_BullpupBody_Heritage"), { { 10.0f, -1.45f }, { 18.0f, -1.45f }, 2.6f, 2.4f } },
		{ TEXT("SM_BullpupBody_Carbon"), { { 10.0f, -1.45f }, { 18.0f, -1.45f }, 2.6f, 2.4f } },
		{ TEXT("SM_BullpupBody_Marksman"), { { 10.0f, -1.45f }, { 18.0f, -1.45f }, 2.6f, 2.4f } },
		{ TEXT("SM_BullpupBody_Armored"), { { 8.4f, -1.3f }, { 18.2f, -1.3f }, 2.8f, 2.9f } },
		{ TEXT("SM_BullpupBody_Salvager"), { { 2.4f, -1.3f }, { 16.4f, -1.3f }, 2.2f, 2.6f } },
		{ TEXT("SM_BullpupBody_Sleek"), { { 8.2f, -0.6f }, { 17.6f, -0.6f }, 2.4f, 2.3f } },
		{ TEXT("SM_BullpupBody_Skeleton"), { { 0.9f, -2.3f }, { 8.2f, -2.3f }, 2.2f, 2.3f } },
		// Ranchhand stocks (origin at the back of the receiver, on the bore line; the stock runs back along -X).
		{ TEXT("SM_RanchhandStock_Field"), { { -36.5f, -2.4f }, { -25.0f, -2.4f }, 2.8f, 1.5f } },
		{ TEXT("SM_RanchhandStock_Saddle"), { { -36.5f, -4.0f }, { -25.0f, -4.0f }, 2.8f, 1.5f } },
		{ TEXT("SM_RanchhandStock_Mule"), { { -36.5f, -2.6f }, { -25.0f, -2.6f }, 2.8f, 1.8f } },
		{ TEXT("SM_RanchhandStock_Thumbhole"), { { -36.5f, -2.6f }, { -25.0f, -2.6f }, 2.8f, 1.6f } },
		{ TEXT("SM_RanchhandStock_Skeleton"), { { -37.4f, -2.6f }, { -30.6f, -2.6f }, 2.6f, 1.6f } },
		{ TEXT("SM_RanchhandStock_Collapsible"), { { -25.5f, -0.9f }, { -14.5f, -0.9f }, 2.6f, 1.3f } },
		{ TEXT("SM_RanchhandStock_Raider"), { { -2.7f, -4.6f }, { -3.9f, -11.0f }, 2.4f, 1.2f } },
		{ TEXT("SM_RanchhandStock_Folding"), { { -2.7f, -4.6f }, { -3.9f, -11.0f }, 2.4f, 1.2f } },
	};

	bool FindRow(const UStaticMesh* Mesh, FTallyRow& OutRow)
	{
		if (!Mesh)
		{
			return false;
		}
		const FString Name = Mesh->GetName();
		for (const FTallyPlacement& Placement : Placements)
		{
			if (Name == Placement.Mesh)
			{
				OutRow = Placement.Row;
				return true;
			}
		}
		return false;
	}

	/**
	 * A row for a part the list doesn't know: along the middle of its longer side view (down it, for a part taller than
	 * long, like a grip), leaving a fifth of it clear at each end, on its outer sides.
	 */
	FTallyRow FitRow(const UStaticMesh& Mesh)
	{
		const FBox3f Box(Mesh.GetBoundingBox());
		const FVector3f Size = Box.GetSize();
		const FVector3f Center = Box.GetCenter();
		FTallyRow Row;
		if (Size.X >= Size.Z)
		{
			Row.Start = FVector2f(Box.Min.X + Size.X * 0.2f, Center.Z);
			Row.End = FVector2f(Box.Max.X - Size.X * 0.2f, Center.Z);
			Row.Height = FMath::Min(Size.Z * 0.3f, 2.8f);
		}
		else
		{
			Row.Start = FVector2f(Center.X, Box.Max.Z - Size.Z * 0.2f);
			Row.End = FVector2f(Center.X, Box.Min.Z + Size.Z * 0.2f);
			Row.Height = FMath::Min(Size.X * 0.3f, 2.8f);
		}
		Row.MinDepth = FMath::Max(FMath::Abs(Box.Min.Y), FMath::Abs(Box.Max.Y)) * 0.75f;
		return Row;
	}
}

bool UWeaponModelComponent::HasTallyRow(const UStaticMesh* Mesh)
{
	FTallyRow Row;
	return FindRow(Mesh, Row);
}

void UWeaponModelComponent::ChooseNotchPart(UStaticMeshComponent* Stock, UStaticMeshComponent* Body)
{
	NotchPart = nullptr;
	FTallyRow Row;
	// A listed stock first, then a listed body (the bullpup's stocks are pads, so its body is listed instead)...
	for (UStaticMeshComponent* Candidate : { Stock, Body })
	{
		if (Candidate && FindRow(Candidate->GetStaticMesh(), Row))
		{
			NotchPart = Candidate;
			break;
		}
	}
	// ...else a row fitted to the stock, or to the body of a gun without one.
	if (!NotchPart)
	{
		UStaticMeshComponent* Fallback = Stock ? Stock : Body;
		if (Fallback && Fallback->GetStaticMesh())
		{
			NotchPart = Fallback;
			Row = FitRow(*Fallback->GetStaticMesh());
			UE_LOG(LogLooter, Verbose, TEXT("%s has no tally row of its own: one fitted to its size"), *Fallback->GetStaticMesh()->GetName());
		}
	}
	NotchRow = FVector4f(Row.Start.X, Row.Start.Y, Row.End.X, Row.End.Y);
	NotchCut = FVector2f(Row.Height, Row.MinDepth);
}

void UWeaponModelComponent::ShowNotches(const FWeaponInstanceData& Instance)
{
	const int32 Marks = WeaponNotches::Marks(Instance.Kills);
	// The soul-light: faint in the gun's rarity color, the game's one color code, once it has killed a thousand.
	const bool bSoulForged = WeaponNotches::TierFor(Instance.Kills) == ENotchTier::SoulForged;
	const FLinearColor Soul = bSoulForged ? UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity) : FLinearColor::Black;
	const FVector4 SoulData(Soul.R, Soul.G, Soul.B, bSoulForged ? SoulLightGlow : 0.f);
	for (UStaticMeshComponent* Part : Parts)
	{
		if (!Part)
		{
			continue;
		}
		const bool bTally = Part == NotchPart && Marks > 0;
		Part->SetCustomPrimitiveDataFloat(NotchMarksDataIndex, bTally ? static_cast<float>(Marks) : 0.f);
		if (bTally)
		{
			Part->SetCustomPrimitiveDataVector4(NotchRowDataIndex, FVector4(NotchRow.X, NotchRow.Y, NotchRow.Z, NotchRow.W));
			Part->SetCustomPrimitiveDataFloat(NotchHeightDataIndex, NotchCut.X);
			Part->SetCustomPrimitiveDataFloat(NotchDepthDataIndex, NotchCut.Y);
		}
		Part->SetCustomPrimitiveDataVector4(SoulLightDataIndex, SoulData);
	}
}
