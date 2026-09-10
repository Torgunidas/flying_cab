#include "FlyingCabDialoguePreview.h"
#include "FlyingCabDialogueSession.h"
#include "FlyingCabNpcDefinition.h"
#include "FlyingCabQuestDefinition.h"
#include "Framework/Application/SlateApplication.h"
#include "PropertyCustomizationHelpers.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

class SFlyingCabDialoguePreview : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlyingCabDialoguePreview) {} SLATE_ARGUMENT(UObject*, Asset) SLATE_END_ARGS()
	void Construct(const FArguments& Args)
	{
		Session.Reset(NewObject<UFlyingCabDialogueSession>());
		if (auto* Npc = Cast<UFlyingCabNpcDefinition>(Args._Asset)) Profile.Reset(Npc);
		else
		{
			Profile.Reset(NewObject<UFlyingCabNpcDefinition>());
			Profile->NpcId = TEXT("Npc.Preview"); Profile->DisplayName = NSLOCTEXT("FlyingCab", "PreviewNpc", "Preview NPC");
			FFlyingCabNpcTopic Topic; Topic.Title = NSLOCTEXT("FlyingCab", "PreviewTopic", "Preview conversation");
			Topic.Dialogue = Cast<UFlyingCabDialogueDefinition>(Args._Asset);
			Profile->Topics.Add(Topic);
			bStandalone = true;
		}
		for (int32 I = 0; I < 4; ++I) Statuses.Add(MakeShared<EFlyingCabQuestStatus>(static_cast<EFlyingCabQuestStatus>(I)));
		ChildSlot
		[
			SNew(SBox).Padding(24)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
				[SNew(STextBlock).Text(NSLOCTEXT("FlyingCab", "PreviewSafety", "CONVERSATION PREVIEW\nChoices simulate quest changes. Game state and assets are never changed.")).AutoWrapText(true)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)[SNew(STextBlock).Text(NSLOCTEXT("FlyingCab", "PreviewStateLabel", "Starting quest state"))]
					+ SHorizontalBox::Slot().FillWidth(1)
					[SNew(SComboBox<TSharedPtr<EFlyingCabQuestStatus>>).OptionsSource(&Statuses)
						.OnGenerateWidget_Lambda([](auto Item) { return SNew(STextBlock).Text(StaticEnum<EFlyingCabQuestStatus>()->GetDisplayNameTextByValue(static_cast<int64>(*Item))); })
						.OnSelectionChanged_Lambda([this](auto Item, ESelectInfo::Type) { if (Item) { Status = *Item; Restart(); } })
						[SNew(STextBlock).Text_Lambda([this]() { return StaticEnum<EFlyingCabQuestStatus>()->GetDisplayNameTextByValue(static_cast<int64>(Status)); })]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12,0,0,0)[SNew(SButton).Text(NSLOCTEXT("FlyingCab", "RestartPreview", "Restart preview")).OnClicked_Lambda([this]() { Restart(); return FReply::Handled(); })]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
				[SNew(SObjectPropertyEntryBox).AllowedClass(UFlyingCabQuestDefinition::StaticClass())
					.Visibility(bStandalone ? EVisibility::Visible : EVisibility::Collapsed)
					.ObjectPath_Lambda([this]() { return Profile->Topics[0].Quest ? Profile->Topics[0].Quest->GetPathName() : FString(); })
					.OnObjectChanged_Lambda([this](const FAssetData& Data) { Profile->Topics[0].Quest = Cast<UFlyingCabQuestDefinition>(Data.GetAsset()); Restart(); })]
				+ SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox) + SScrollBox::Slot()[SAssignNew(Body, SVerticalBox)]]
			]
		];
		Restart();
	}
private:
	void Restart()
	{
		if (Session->StartPreview(Profile.Get(), Status) && bStandalone) Session->ChooseOption(0, Session->GetView().Revision);
		Refresh();
	}
	void Refresh()
	{
		Body->ClearChildren(); const auto View = Session->GetView();
		Body->AddSlot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text(View.Speaker).Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))];
		Body->AddSlot().AutoHeight().Padding(0,0,0,20)[SNew(STextBlock).Text(View.Text).AutoWrapText(true)];
		if (!View.Feedback.IsEmpty()) Body->AddSlot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text(View.Feedback).AutoWrapText(true).ColorAndOpacity(FLinearColor(0.95f,0.7f,0.3f))];
		if (!Session->IsActive())
		{
			Body->AddSlot().AutoHeight()[SNew(STextBlock).Text(NSLOCTEXT("FlyingCab", "PreviewEnded", "Conversation ended. Restart to try another path."))]; return;
		}
		for (int32 I = 0; I < View.Options.Num(); ++I)
		{
			const auto& Option = View.Options[I];
			const FText Text = Option.bEnabled ? Option.Text : FText::Format(NSLOCTEXT("FlyingCab", "PreviewDisabled", "{0}\nUnavailable: {1}"), Option.Text, Option.UnavailableReason);
			Body->AddSlot().AutoHeight().Padding(0,0,0,10)
			[SNew(SButton).IsEnabled(Option.bEnabled).ContentPadding(FMargin(14,10))
				.OnClicked_Lambda([this, I, Revision = View.Revision]() { Session->ChooseOption(I, Revision); Refresh(); return FReply::Handled(); })
				[SNew(STextBlock).Text(Text).AutoWrapText(true)]];
		}
	}
	TStrongObjectPtr<UFlyingCabNpcDefinition> Profile;
	TStrongObjectPtr<UFlyingCabDialogueSession> Session;
	TSharedPtr<SVerticalBox> Body;
	TArray<TSharedPtr<EFlyingCabQuestStatus>> Statuses;
	EFlyingCabQuestStatus Status = EFlyingCabQuestStatus::Inactive;
	bool bStandalone = false;
};

void ShowFlyingCabDialoguePreview(UObject* Asset)
{
	FSlateApplication::Get().AddWindow(SNew(SWindow).Title(NSLOCTEXT("FlyingCab", "PreviewTitle", "Flying Cab — Conversation preview")).ClientSize(FVector2D(760, 660))
		[SNew(SFlyingCabDialoguePreview).Asset(Asset)]);
}
