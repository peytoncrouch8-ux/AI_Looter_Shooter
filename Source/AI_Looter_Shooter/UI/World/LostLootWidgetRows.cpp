// ULostLootWidget: the header (the agent's notice, the shelves' count and bar, the purse), the claim tickets with their stubs,
// and the shelving to buy.

#include "UI/World/LostLootWidget.h"
#include "Economy/WalletSubsystem.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "UI/Vendor/SilverUI.h"
#include "Weapons/WeaponDefinition.h"
#include "World/LostPropertyWindow.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LooterLostLoot"

using namespace LooterUI;

namespace
{
	/** A ticket's gun picture, and the shelves' bar under the count. */
	const FVector2D TicketPicture(78.f, 32.f);
	constexpr float MeterWidth = 170.f;
	constexpr float MeterHeight = 6.f;

	/** The stub's perforation: dashes down the tear line. */
	constexpr int32 PerforationDashes = 7;
	constexpr float DashLength = 4.f;
	constexpr float DashGap = 3.5f;

	/** The punched hole in a stub, where the ticket hung on its hook. */
	constexpr float HoleSize = 9.f;

	FString RarityWord(EWeaponRarity Rarity)
	{
		return UEnum::GetDisplayValueAsText(Rarity).ToString();
	}

	/** "Shotgun": the gun's kind, for a ticket's line. */
	FString KindWord(const FWeaponInstanceData& Gun)
	{
		return Gun.Definition && !Gun.Definition->DisplayName.IsEmpty() ? Gun.Definition->DisplayName.ToString() : FString();
	}
}

// ---------------------------------------------------------------------------
// The header
// ---------------------------------------------------------------------------

UWidget* ULostLootWidget::MakeHeader()
{
	const ULostLootSubsystem* LostLoot = GetLostLoot();
	const FLostLootStore Empty;
	const FLostLootStore& Store = LostLoot ? LostLoot->GetStore() : Empty;
	UHorizontalBox* Head = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// The station agent's notice, pinned by the window.
	{
		const ALostPropertyWindow* At = Window.Get();
		const FText Words = At && !At->Notice.IsEmpty() ? At->Notice : GetDefault<ALostPropertyWindow>()->Notice;
		UVerticalBox* Notice = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Notice->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("NoticeLabel", "Notice").ToString(), 9, Color::Accent(), 300));
		UTextBlock* Body = MakeText(WidgetTree, Words.ToString(), 12, Color::TextDim());
		Body->SetFont(LooterUI::Font(12, /*bBold*/ false));
		Body->SetAutoWrapText(true);
		Notice->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		UHorizontalBoxSlot* NoticeSlot = Head->AddChildToHorizontalBox(Notice);
		NoticeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NoticeSlot->SetVerticalAlignment(VAlign_Center);
		NoticeSlot->SetPadding(FMargin(0.f, 0.f, 32.f, 0.f));
	}

	// The shelves: how full they are.
	UHorizontalBoxSlot* MeterSlot = Head->AddChildToHorizontalBox(MakeShelfMeter(Store.Num(), Store.GetCapacity()));
	MeterSlot->SetVerticalAlignment(VAlign_Center);
	MeterSlot->SetPadding(FMargin(0.f, 0.f, 32.f, 0.f));

	// The purse, for the shelving.
	UVerticalBox* Purse = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Purse->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("YourSilver", "Your silver").ToString(), 9, Color::TextDim(), 250))
		->SetHorizontalAlignment(HAlign_Right);
	UTextBlock* Amount = nullptr;
	UWidget* Line = SilverUI::MakeAmount(WidgetTree, FMath::RoundToInt32(ShownSilver), 24, Color::Text(), false, &Amount);
	Line->SetRenderTransformPivot(FVector2D(1.f, 0.5f));
	Purse->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Right);
	PurseText = Amount;
	PurseLine = Line;
	Head->AddChildToHorizontalBox(Purse)->SetVerticalAlignment(VAlign_Center);
	return Head;
}

UWidget* ULostLootWidget::MakeShelfMeter(int32 Count, int32 Capacity)
{
	const bool bFull = Count >= Capacity;
	UVerticalBox* Meter = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Meter->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("OnShelves", "On the shelves").ToString(), 9, Color::TextDim(), 250))
		->SetHorizontalAlignment(HAlign_Right);
	const FString CountWords = FString::Printf(TEXT("%d / %d"), Count, Capacity);
	Meter->AddChildToVerticalBox(MakeText(WidgetTree, CountWords, 24, bFull ? Color::Worse() : Color::Text()))->SetHorizontalAlignment(HAlign_Right);

	// The bar fills toward full, cyan, red once it is: the next gun in sends the oldest out.
	UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const float Share = Capacity > 0 ? FMath::Clamp(static_cast<float>(Count) / static_cast<float>(Capacity), 0.f, 1.f) : 0.f;
	auto Part = [this, Bar](float Weight, const FLinearColor& PartColor)
	{
		if (Weight < 0.005f)
		{
			return;
		}
		UHorizontalBoxSlot* PartSlot = Bar->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(PartColor)), 0.f, MeterHeight));
		FSlateChildSize Size(ESlateSizeRule::Fill);
		Size.Value = Weight;
		PartSlot->SetSize(Size);
	};
	Part(Share, bFull ? Color::Worse() : Color::CyanText());
	Part(1.f - Share, Color::SegmentOff());
	Meter->AddChildToVerticalBox(MakeSized(WidgetTree, Bar, MeterWidth, MeterHeight))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	if (bFull)
	{
		Meter->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("FullWords", "Full: the oldest goes next").ToString(), 8,
			Color::Worse(), 160))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	return Meter;
}

// ---------------------------------------------------------------------------
// The tickets
// ---------------------------------------------------------------------------

FText ULostLootWidget::AgoWords(double HandedInAt) const
{
	const ULostLootSubsystem* LostLoot = GetLostLoot();
	const double Now = LostLoot ? LostLoot->GetClock() : -1.0;
	// A ticket from another play without a session (its clock started over) says nothing rather than something wrong.
	if (HandedInAt < 0.0 || Now < HandedInAt)
	{
		return FText::GetEmpty();
	}
	const double Seconds = Now - HandedInAt;
	if (Seconds < 60.0)
	{
		return LOCTEXT("JustNow", "just now");
	}
	return FText::Format(LOCTEXT("Ago", "{0} ago"), FText::FromString(USessionSubsystem::FormatPlayTime(Seconds)));
}

UWidget* ULostLootWidget::MakeStub(const FLostLootEntry& Entry)
{
	// The stub the player keeps: the punched hole it hung by, NO. and the number, as a railway's claim check is printed.
	UVerticalBox* Stub = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UImage* Hole = MakeImage(WidgetTree, CircleBrush(Color::Backdrop(), Color::RowLine(), 1.f));
	Stub->AddChildToVerticalBox(MakeSized(WidgetTree, Hole, HoleSize, HoleSize))->SetHorizontalAlignment(HAlign_Center);
	Stub->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("No", "No.").ToString(), 8, Color::TextDim(), 200))
		->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	UTextBlock* Number = MakeText(WidgetTree, FLostLootStore::TicketText(Entry.Ticket), 16, Color::Accent(), false, 60);
	Stub->AddChildToVerticalBox(Number);

	// Then the tear line down the card.
	UVerticalBox* Perforation = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	for (int32 Dash = 0; Dash < PerforationDashes; ++Dash)
	{
		Perforation->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::RowLine())), 1.5f, DashLength))
			->SetPadding(FMargin(0.f, Dash > 0 ? DashGap : 0.f, 0.f, 0.f));
	}

	UHorizontalBox* Holder = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Holder->AddChildToHorizontalBox(MakeSized(WidgetTree, Stub, LostLootLayout::StubWidth - 14.f))->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* TearSlot = Holder->AddChildToHorizontalBox(Perforation);
	TearSlot->SetVerticalAlignment(VAlign_Center);
	TearSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	return Holder;
}

UWidget* ULostLootWidget::MakeTicketRow(int32 RowIndex, const FLostLootEntry& Entry, bool bNextToGo, bool bFull)
{
	FRow& Row = Rows.AddDefaulted_GetRef();
	Row.Ticket = Entry.Ticket;
	const FWeaponInstanceData& Gun = Entry.Weapon;

	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(MakeStub(Entry))->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* PictureSlot = Line->AddChildToHorizontalBox(LoadoutParts::MakeGunPicture(WidgetTree, Gun, TicketPicture));
	PictureSlot->SetVerticalAlignment(VAlign_Center);
	PictureSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

	// Its name in its rarity's colour; level, rarity and kind; where it was found and when it came in.
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Info->AddChildToVerticalBox(LoadoutParts::MakeGunNameLine(WidgetTree, Gun, 13, LooterWeaponText::Color(Gun), 40, 250.f, 10));
	const FString Kind = KindWord(Gun);
	const FString Detail = Kind.IsEmpty()
		? FText::Format(LOCTEXT("TicketDetail", "Level {0} · {1}"), FText::AsNumber(Gun.Level), FText::FromString(RarityWord(Gun.Rarity))).ToString()
		: FText::Format(LOCTEXT("TicketDetailKind", "Level {0} · {1} · {2}"), FText::AsNumber(Gun.Level), FText::FromString(RarityWord(Gun.Rarity)),
			FText::FromString(Kind)).ToString();
	Info->AddChildToVerticalBox(MakeText(WidgetTree, Detail, 10, Color::TextDim(), true, 100))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	const FText Ago = AgoWords(Entry.HandedInAt);
	const FText Found = Ago.IsEmpty() ? LostLootRules::ReasonWords(Entry.Reason, Entry.Map)
		: FText::Format(LOCTEXT("FoundAgo", "{0} · {1}"), LostLootRules::ReasonWords(Entry.Reason, Entry.Map), Ago);
	UTextBlock* FoundText = MakeText(WidgetTree, Found.ToString(), 10, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.8f));
	FoundText->SetFont(LooterUI::Font(10, /*bBold*/ false));
	FoundText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	Info->AddChildToVerticalBox(FoundText)->SetPadding(FMargin(0.f, 1.f, 0.f, 0.f));
	UHorizontalBoxSlot* InfoSlot = Line->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);

	// NEXT TO GO on the one that makes room next: red once the shelves are full and it really is on its way out.
	if (bNextToGo)
	{
		UHorizontalBoxSlot* TagSlot = Line->AddChildToHorizontalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("NextToGo", "Next to go").ToString(), 9,
			bFull ? Color::Worse() : Color::TextDim(), 160));
		TagSlot->SetVerticalAlignment(VAlign_Center);
		TagSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	}

	UOverlay* Card = LoadoutParts::MakeCard(WidgetTree, Line, FMargin(12.f, 6.f, 14.f, 6.f), 1.5f, Row.Fill, Row.Line);
	USizeBox* Sized = MakeSized(WidgetTree, Card, 0.f);
	Sized->SetMinDesiredHeight(LostLootLayout::RowHeight);
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	// The screen sounds its own choosing and claims.
	Button->bPlaysSounds = false;
	Button->SetupContent(Sized, LostLootActions::Ticket, RowIndex, EButtonKind::Bare);
	Button->OnButtonClicked.BindUObject(this, &ULostLootWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &ULostLootWidget::HandleHovered);
	Row.Button = Button;
	return Button;
}

// ---------------------------------------------------------------------------
// More shelving
// ---------------------------------------------------------------------------

UWidget* ULostLootWidget::MakeUpgradeRow()
{
	const ULostLootSubsystem* LostLoot = GetLostLoot();
	const int32 Upgrades = LostLoot ? LostLoot->GetStore().Upgrades : 0;
	const int32 Capacity = LostLootRules::Capacity(Upgrades);
	const bool bAtMost = Upgrades >= LostLootRules::MaxUpgrades;

	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UWidget* Right = nullptr;
	if (bAtMost)
	{
		Info->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("AtMost", "The shelves are at their most").ToString(), 13, Color::TextDim(), 80));
		Info->AddChildToVerticalBox(MakeText(WidgetTree, FText::Format(LOCTEXT("AtMostDetail", "Room for {0} guns: every corner of the depot holds them."),
			FText::AsNumber(Capacity)).ToString(), 10, Color::TextDim()))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	}
	else
	{
		const int32 Price = LostLootRules::UpgradePrice(Upgrades, GetPlayerLevel());
		const UWalletSubsystem* Wallet = GetWallet();
		Info->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("MoreShelving", "More shelving").ToString(), 13, Color::Accent(), 80));
		Info->AddChildToVerticalBox(MakeText(WidgetTree, FText::Format(LOCTEXT("ShelvingDetail", "{0} · room for {1} guns, {2} now"),
			LostLootRules::UpgradeName(Upgrades), FText::AsNumber(LostLootRules::Capacity(Upgrades + 1)), FText::AsNumber(Capacity)).ToString(), 10,
			Color::TextDim()))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		Right = SilverUI::MakeAmount(WidgetTree, Price, 15, Wallet && Wallet->CanAfford(Price) ? Color::Text() : Color::Worse(), false);
	}
	UHorizontalBoxSlot* InfoSlot = Line->AddChildToHorizontalBox(Info);
	InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	InfoSlot->SetVerticalAlignment(VAlign_Center);
	if (Right)
	{
		UHorizontalBoxSlot* RightSlot = Line->AddChildToHorizontalBox(Right);
		RightSlot->SetVerticalAlignment(VAlign_Center);
		RightSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	}

	UImage* Fill = nullptr;
	UImage* Edge = nullptr;
	UOverlay* Card = LoadoutParts::MakeCard(WidgetTree, Line, FMargin(14.f, 8.f, 14.f, 8.f), 1.5f, Fill, Edge);
	if (Fill && Edge)
	{
		Fill->SetColorAndOpacity(bAtMost ? LoadoutParts::Colors::EmptyFill() : Color::Row());
		Edge->SetColorAndOpacity(bAtMost ? Color::RowLine() : Color::Accent() * FLinearColor(1.f, 1.f, 1.f, 0.6f));
	}
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->bPlaysSounds = false;
	Button->SetupContent(MakeSized(WidgetTree, Card, 0.f, 58.f), LostLootActions::Upgrade, 0, EButtonKind::Bare);
	Button->OnButtonClicked.BindUObject(this, &ULostLootWidget::HandleButton);
	return Button;
}

#undef LOCTEXT_NAMESPACE
