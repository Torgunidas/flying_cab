#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_Base.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "PropertyEditorModule.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Toolkits/SimpleAssetEditor.h"
#include "ToolMenus.h"
#include "ScopedTransaction.h"
#include "Misc/MessageDialog.h"
#include "Misc/DataValidation.h"
#include "Framework/Application/SlateApplication.h"
#include "FlyingCabNarrativeFactories.h"
#include "FlyingCabDialoguePreview.h"
#include "FlyingCabDialogueDefinition.h"
#include "FlyingCabNpcDefinition.h"
#include "FlyingCabNarrativeSettings.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabQuestCatalog.h"
#include "FlyingCabQuestHubData.h"
#include "FlyingCabCityData.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FlyingCabNarrativeEditor"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabNarrativeEditor, Log, All);

namespace
{
	// After `git pull` the editor loads the existing Binaries/<Platform> modules without any prompt:
	// it only compares their BuildId with the engine, never source timestamps (audit 2026-09-10).
	// Compare them here and tell the user, otherwise the game silently runs old code.
	void WarnIfCompiledModulesAreStale()
	{
		IFileManager& Files = IFileManager::Get();
		FString NewestBinary;
		FDateTime NewestBinaryTime = FDateTime::MinValue();
		for (const TCHAR* Module : {TEXT("FlyingCabFlightLab"), TEXT("FlyingCabNarrativeEditor")})
		{
			const FString File = FModuleManager::Get().GetModuleFilename(Module);
			const FDateTime Time = File.IsEmpty() ? FDateTime::MinValue() : Files.GetTimeStamp(*File);
			if (Time > NewestBinaryTime) { NewestBinaryTime = Time; NewestBinary = File; }
		}
		if (NewestBinaryTime == FDateTime::MinValue()) return;
		TArray<FString> Sources;
		Files.FindFilesRecursive(Sources, *FPaths::GameSourceDir(), TEXT("*"), true, false);
		FString NewestSource;
		FDateTime NewestSourceTime = FDateTime::MinValue();
		for (const FString& File : Sources)
		{
			const FString Extension = FPaths::GetExtension(File);
			if (Extension != TEXT("cpp") && Extension != TEXT("h") && Extension != TEXT("cs") && Extension != TEXT("inl")) continue;
			const FDateTime Time = Files.GetTimeStamp(*File);
			if (Time > NewestSourceTime) { NewestSourceTime = Time; NewestSource = File; }
		}
		if (NewestSourceTime <= NewestBinaryTime)
		{
			UE_LOG(LogFlyingCabNarrativeEditor, Display, TEXT("Compiled modules are current: %s built %s, newest source %s."),
				*FPaths::GetCleanFilename(NewestBinary), *NewestBinaryTime.ToString(), *NewestSourceTime.ToString());
			return;
		}
		const FString Message = FString::Printf(
			TEXT("Source is newer than the compiled game modules: %s changed %s, but %s was built %s. Close the editor and run scripts/sync-mac.sh or scripts/Sync-Windows.ps1, otherwise the game runs old code."),
			*FPaths::GetCleanFilename(NewestSource), *NewestSourceTime.ToString(), *FPaths::GetCleanFilename(NewestBinary), *NewestBinaryTime.ToString());
		UE_LOG(LogFlyingCabNarrativeEditor, Warning, TEXT("%s"), *Message);
		if (IsRunningCommandlet() || FApp::IsUnattended() || !FSlateApplication::IsInitialized()) return;
		FNotificationInfo Info(FText::FromString(Message));
		Info.ExpireDuration = 60.f;
		Info.FadeOutDuration = 2.f;
		Info.bUseSuccessFailIcons = true;
		if (TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info))
		{
			Item->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}
}

namespace
{
	using FNameOption = TSharedPtr<TPair<FName, FText>>;

	TArray<FNameOption> GetEvents()
	{
		TArray<FNameOption> Options;
		auto Add = [&Options](FName Id, const TCHAR* Name) { Options.Add(MakeShared<TPair<FName, FText>>(Id, FText::FromString(Name))); };
		Add(FlyingCabQuestEvents::PassengerDelivered, TEXT("Deliver passengers"));
		Add(FlyingCabQuestEvents::PassengerPickedUp, TEXT("Pick up passengers (filter: destination district)"));
		Add(FlyingCabQuestEvents::CreditsEarned, TEXT("Earn credits (all income since this step starts)"));
		Add(FlyingCabQuestEvents::FuelPurchased, TEXT("Buy fuel (units)"));
		Add(FlyingCabQuestEvents::RepairPurchased, TEXT("Repair vehicle (units)"));
		Add(FlyingCabQuestEvents::VehicleEntered, TEXT("Enter a vehicle"));
		Add(FlyingCabQuestEvents::VehicleExited, TEXT("Exit a vehicle"));
		Add(FlyingCabQuestEvents::NearMiss, TEXT("Earn a near-miss bonus"));
		Add(FlyingCabQuestEvents::InteractionCompleted, TEXT("Use an object / finish an NPC conversation"));
		Add(FlyingCabQuestEvents::QuestGiverInteracted, TEXT("Finish an NPC conversation"));
		Add(FlyingCabQuestEvents::AccessGranted, TEXT("Claim or confirm access at a terminal"));
		if (auto* Catalog = GetDefault<UFlyingCabNarrativeSettings>()->QuestCatalog.LoadSynchronous())
			for (FName Id : Catalog->AllowedCustomEventIds) if (!Id.IsNone()) Add(Id, *FString::Printf(TEXT("Custom: %s"), *Id.ToString()));
		return Options;
	}

	TSharedRef<SWidget> NamePicker(const TSharedRef<IPropertyHandle>& Property, TArray<FNameOption> Options, TFunction<void(FName)> AfterChange = {})
	{
		// Combo widgets retain their options independently from the temporary Details customization.
		auto SharedOptions = MakeShared<TArray<FNameOption>>(MoveTemp(Options));
		return SNew(SComboBox<FNameOption>).OptionsSource(&SharedOptions.Get())
			.OnGenerateWidget_Lambda([](FNameOption Item) { return SNew(STextBlock).Text(Item->Value); })
			.OnSelectionChanged_Lambda([Property, SharedOptions, AfterChange](FNameOption Item, ESelectInfo::Type)
			{
				if (Item) { Property->SetValue(Item->Key); if (AfterChange) AfterChange(Item->Key); }
			})
			[SNew(STextBlock).Text_Lambda([Property, SharedOptions]()
			{
				FName Value; if (Property->GetValue(Value) != FPropertyAccess::Success) return LOCTEXT("Multiple", "Multiple values");
				for (const auto& Item : *SharedOptions) if (Item->Key == Value) return Item->Value;
				return Value.IsNone() ? LOCTEXT("Choose", "Choose...") : FText::FromName(Value);
			})];
	}

	TArray<FNameOption> NodeOptions(const TSharedRef<IPropertyHandle>& Property)
	{
		TArray<FNameOption> Options;
		Options.Add(MakeShared<TPair<FName, FText>>(NAME_None, LOCTEXT("EndConversation", "End conversation")));
		TArray<UObject*> Outers; Property->GetOuterObjects(Outers);
		for (UObject* Outer : Outers)
			if (auto* Dialogue = Cast<UFlyingCabDialogueDefinition>(Outer))
				for (const auto& Node : Dialogue->Nodes)
					Options.Add(MakeShared<TPair<FName, FText>>(Node.NodeId, FText::FromString(Node.NodeId.ToString() + TEXT(" — ") + Node.Text.ToString().Replace(TEXT("\n"), TEXT(" ")).Left(65))));
		return Options;
	}

	class FObjectiveDetails : public IPropertyTypeCustomization
	{
	public:
		virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Property, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils&) override
		{
			Row.NameContent()[Property->CreatePropertyNameWidget()];
		}
		virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Property, IDetailChildrenBuilder& Children, IPropertyTypeCustomizationUtils& Utils) override
		{
			auto Event = Property->GetChildHandle(TEXT("EventId")).ToSharedRef();
			auto Target = Property->GetChildHandle(TEXT("TargetId")).ToSharedRef();
			auto Description = Property->GetChildHandle(TEXT("Description")).ToSharedRef();
			auto Utilities = Utils.GetPropertyUtilities();
			Children.AddCustomRow(LOCTEXT("ObjectiveType", "Objective type"))
			.NameContent()[SNew(STextBlock).Text(LOCTEXT("ObjectiveType", "Objective type"))]
			.ValueContent().MinDesiredWidth(310)[NamePicker(Event, GetEvents(), [Target, Description, Utilities](FName NewEvent)
			{
				Target->SetValue(NAME_None);
				FText Current; Description->GetValue(Current);
				if (Current.IsEmpty())
					for (const auto& Option : GetEvents()) if (Option->Key == NewEvent) { Description->SetValue(Option->Value); break; }
				Utilities->RequestRefresh();
			})];
			Children.AddProperty(Description);
			FName Id; Event->GetValue(Id);
			TArray<FNameOption> Targets;
			Targets.Add(MakeShared<TPair<FName, FText>>(NAME_None, LOCTEXT("AnyTarget", "Any / no filter")));
			if (Id == FlyingCabQuestEvents::PassengerDelivered || Id == FlyingCabQuestEvents::PassengerPickedUp)
				for (const auto& District : FlyingCabCityData::GetDistricts()) Targets.Add(MakeShared<TPair<FName, FText>>(District.DistrictId, FText::FromString(District.DisplayName)));
			else if (Id == FlyingCabQuestEvents::QuestGiverInteracted || Id == FlyingCabQuestEvents::InteractionCompleted)
				for (const auto& Hub : FlyingCabQuestHubData::GetQuestHubs(GEditor ? GEditor->GetEditorWorldContext().World() : nullptr))
					Targets.Add(MakeShared<TPair<FName, FText>>(Hub.HubId, FText::FromString(Hub.DisplayName)));
			if (Targets.Num() > 1)
				Children.AddCustomRow(LOCTEXT("Target", "Target"))
				.NameContent()[SNew(STextBlock).Text(LOCTEXT("Target", "Target"))]
				.ValueContent().MinDesiredWidth(310)[NamePicker(Target, MoveTemp(Targets))];
			Children.AddProperty(Property->GetChildHandle(TEXT("RequiredCount")).ToSharedRef());
			const FText Hint = Id == FlyingCabQuestEvents::CreditsEarned
				? LOCTEXT("EarnHint", "Counts positive income, including quest rewards, after this step starts. Spending does not reduce progress.")
				: LOCTEXT("SequenceHint", "Steps run in order. Events before this step becomes active do not count. Target None accepts any source.");
			Children.AddCustomRow(Hint).WholeRowContent()[SNew(STextBlock).Text(Hint).AutoWrapText(true)];
			Children.AddProperty(Target).DisplayName(LOCTEXT("RawTarget", "Target ID (advanced / custom objects)"));
			Children.AddProperty(Property->GetChildHandle(TEXT("ObjectiveId")).ToSharedRef());
		}
	};

	class FDialogueLinkDetails : public IPropertyTypeCustomization
	{
	public:
		virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Property, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils&) override
		{ Row.NameContent()[Property->CreatePropertyNameWidget()]; }
		virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Property, IDetailChildrenBuilder& Children, IPropertyTypeCustomizationUtils&) override
		{
			uint32 Count = 0; Property->GetNumChildren(Count);
			for (uint32 I = 0; I < Count; ++I)
			{
				auto Child = Property->GetChildHandle(I).ToSharedRef();
				const FName Name = Child->GetProperty()->GetFName();
				if (Name == TEXT("NextNodeId") || Name == TEXT("NodeId"))
					Children.AddCustomRow(LOCTEXT("GoTo", "Go to"))
					.NameContent()[SNew(STextBlock).Text(LOCTEXT("GoTo", "Go to"))]
					.ValueContent().MinDesiredWidth(310)[NamePicker(Child, NodeOptions(Child))];
				else Children.AddProperty(Child);
			}
		}
	};

	void OpenAsset(UObject* Asset)
	{
		if (Asset && GEditor) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
	}

	void ValidateAsset(UObject* Asset)
	{
		if (!Asset) return;
		FDataValidationContext Context;
		Asset->IsDataValid(Context);
		TArray<FString> Messages;
		for (const auto& Issue : Context.GetIssues()) Messages.Add(Issue.Message.ToString());
		const FText Text = Messages.IsEmpty() ? LOCTEXT("Valid", "Validation passed.") : FText::FromString(FString::Join(Messages, TEXT("\n\n")));
		FMessageDialog::Open(EAppMsgType::Ok, Text);
	}

	class FNarrativeDetails : public IDetailCustomization
	{
	public:
		virtual void CustomizeDetails(IDetailLayoutBuilder& Layout) override
		{
			TArray<TWeakObjectPtr<UObject>> Objects; Layout.GetObjectsBeingCustomized(Objects);
			if (Objects.Num() != 1 || !Objects[0].IsValid()) return;
			const TWeakObjectPtr<UObject> Asset = Objects[0];
			auto& Tools = Layout.EditCategory(TEXT("Authoring"), LOCTEXT("Authoring", "Flying Cab — Authoring"), ECategoryPriority::Important);
			auto Buttons = SNew(SHorizontalBox);
			Buttons->AddSlot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).Text(LOCTEXT("Validate", "Validate"))
				.OnClicked_Lambda([Asset]() { ValidateAsset(Asset.Get()); return FReply::Handled(); })];
			if (auto* Quest = Cast<UFlyingCabQuestDefinition>(Asset.Get()))
			{
				const TWeakObjectPtr<UFlyingCabQuestDefinition> WeakQuest = Quest;
				Buttons->AddSlot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).Text(LOCTEXT("RegisterQuest", "Add to catalog"))
					.OnClicked_Lambda([WeakQuest]()
					{
						auto* Catalog = GetDefault<UFlyingCabNarrativeSettings>()->QuestCatalog.LoadSynchronous();
						if (Catalog && WeakQuest.IsValid())
						{
							const FScopedTransaction Transaction(LOCTEXT("RegisterQuestTransaction", "Add quest to catalog"));
							Catalog->Modify(); Catalog->Quests.AddUnique(WeakQuest.Get()); Catalog->MarkPackageDirty();
							FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Registered", "Quest added. Save the quest and catalog, then assign the quest in an NPC profile topic."));
						}
						else FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("MissingCatalog", "Select a valid quest catalog in Project Settings / Flying Cab Narrative."));
						return FReply::Handled();
					})];
				auto TurnIn = Layout.GetProperty(GET_MEMBER_NAME_CHECKED(UFlyingCabQuestDefinition, TurnInNpcId));
				Layout.HideProperty(TurnIn);
				TArray<FNameOption> Npcs; Npcs.Add(MakeShared<TPair<FName,FText>>(NAME_None, LOCTEXT("AnyNpc", "Any NPC offering this quest")));
				for (const auto& Hub : FlyingCabQuestHubData::GetQuestHubs(GEditor->GetEditorWorldContext().World()))
					Npcs.Add(MakeShared<TPair<FName,FText>>(Hub.HubId, FText::FromString(Hub.DisplayName)));
				Layout.EditCategory(TEXT("Quest|Flow")).AddCustomRow(LOCTEXT("TurnInNpc", "Turn in at"))
				.Visibility(TAttribute<EVisibility>::CreateLambda([WeakQuest]() { return WeakQuest.IsValid() && WeakQuest->bRequiresTurnIn ? EVisibility::Visible : EVisibility::Collapsed; }))
				.NameContent()[SNew(STextBlock).Text(LOCTEXT("TurnInNpc", "Turn in at"))]
				.ValueContent().MinDesiredWidth(310)[NamePicker(TurnIn, MoveTemp(Npcs))];
				Tools.AddCustomRow(LOCTEXT("QuestHelp", "Workflow")).WholeRowContent()[SNew(STextBlock).AutoWrapText(true)
					.Text(LOCTEXT("QuestWorkflow", "1. Fill in title and objectives.  2. Add to catalog and save.  3. Add a topic to Mike, Jack or your own NPC profile. Existing event types only; each objective starts after the previous one."))];
			}
			if (Cast<UFlyingCabNpcDefinition>(Asset.Get()) || Cast<UFlyingCabDialogueDefinition>(Asset.Get()))
			{
				Buttons->AddSlot().AutoWidth()[SNew(SButton).Text(LOCTEXT("PreviewConversation", "Preview conversation"))
					.OnClicked_Lambda([Asset]() { if (Asset.IsValid()) ShowFlyingCabDialoguePreview(Asset.Get()); return FReply::Handled(); })];
			}
			if (Cast<UFlyingCabDialogueDefinition>(Asset.Get()))
			{
				auto Entry = Layout.GetProperty(GET_MEMBER_NAME_CHECKED(UFlyingCabDialogueDefinition, EntryNodeId));
				Layout.HideProperty(Entry);
				Layout.EditCategory(TEXT("Dialogue")).AddCustomRow(LOCTEXT("DefaultEntry", "Default start"))
				.NameContent()[SNew(STextBlock).Text(LOCTEXT("DefaultEntry", "Default start"))]
				.ValueContent().MinDesiredWidth(310)[NamePicker(Entry, NodeOptions(Entry))];
				Tools.AddCustomRow(LOCTEXT("DialogueHelp", "Text placeholders")).WholeRowContent()[SNew(STextBlock).AutoWrapText(true)
					.Text(LOCTEXT("DialogueWorkflow", "Quest references left empty use the NPC topic's quest. Text placeholders: {QuestTitle}, {QuestDescription}, {Objective}, {Progress}, {Required}, {RewardCredits}. Entry rules are checked in order. No match uses the default start."))];
			}
			if (Cast<UFlyingCabNpcDefinition>(Asset.Get()))
				Tools.AddCustomRow(LOCTEXT("NpcHelp", "Topics")).WholeRowContent()[SNew(STextBlock).AutoWrapText(true)
					.Text(LOCTEXT("NpcWorkflow", "Add one topic per quest or conversation. A quest without a conversation asset uses the ready-made accept/remind/turn-in dialogue. A topic without a quest needs a title and a conversation. Add this profile to the NPC roster or a placed FlyingCabQuestGiver."))];
			Tools.AddCustomRow(LOCTEXT("Actions", "Actions")).WholeRowContent()[Buttons];
		}
	};

	class FNarrativeAssetActions : public FAssetTypeActions_Base
	{
	public:
		FNarrativeAssetActions(UClass* InClass, FText InName, FColor InColor) : Class(InClass), Name(InName), Color(InColor) {}
		virtual FText GetName() const override { return Name; }
		virtual FColor GetTypeColor() const override { return Color; }
		virtual UClass* GetSupportedClass() const override { return Class; }
		virtual uint32 GetCategories() override { return FlyingCabNarrativeAssetCategory; }
		virtual void OpenAssetEditor(const TArray<UObject*>& Objects, TSharedPtr<IToolkitHost> Host) override
		{ FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, Host, Objects); }
	private:
		UClass* Class;
		FText Name;
		FColor Color;
	};

	void CreateNarrativeAsset(UFactory* Factory, const FString& Name, const FString& Folder)
	{
		auto& Assets = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		OpenAsset(Assets.CreateAssetWithDialog(Name, Folder, Factory->SupportedClass, Factory));
	}

	void ShowAuthoringHub()
	{
		auto Content = SNew(SVerticalBox);
		Content->AddSlot().AutoHeight().Padding(0,0,0,16)[SNew(STextBlock).Text(LOCTEXT("HubIntro", "QUESTS & CONVERSATIONS\nCreate a quest, add it to the catalog, then assign it to a topic on an NPC profile.")).AutoWrapText(true)];
		auto Button = [&Content](const FText& Label, TFunction<void()> Action)
		{
			Content->AddSlot().AutoHeight().Padding(0,0,0,10)[SNew(SButton).Text(Label).ContentPadding(FMargin(16,10)).OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })];
		};
		Button(LOCTEXT("NewQuest", "New quest"), []() { CreateNarrativeAsset(NewObject<UFlyingCabQuestFactory>(), TEXT("DA_Quest_New"), TEXT("/Game/Data/Quests")); });
		Button(LOCTEXT("NewNpc", "New NPC profile"), []() { CreateNarrativeAsset(NewObject<UFlyingCabNpcFactory>(), TEXT("DA_NPC_New"), TEXT("/Game/Data/Narrative")); });
		Button(LOCTEXT("NewQuestDialogue", "New conversation — quest template"), []() { CreateNarrativeAsset(NewObject<UFlyingCabDialogueFactory>(), TEXT("DA_Dialogue_New"), TEXT("/Game/Data/Narrative")); });
		Button(LOCTEXT("NewPlainDialogue", "New conversation — small talk"), []() { auto* Factory = NewObject<UFlyingCabDialogueFactory>(); Factory->bQuestTemplate = false; CreateNarrativeAsset(Factory, TEXT("DA_Dialogue_SmallTalk"), TEXT("/Game/Data/Narrative")); });
		Button(LOCTEXT("OpenCatalog", "Open quest catalog"), []() { OpenAsset(GetDefault<UFlyingCabNarrativeSettings>()->QuestCatalog.LoadSynchronous()); });
		Button(LOCTEXT("OpenRoster", "Open NPC roster (Mike, Jack and placements)"), []() { OpenAsset(UFlyingCabNpcRoster::LoadDefaultAsset()); });
		FSlateApplication::Get().AddWindow(SNew(SWindow).Title(LOCTEXT("HubTitle", "Flying Cab — Quest & Dialogue tools")).ClientSize(FVector2D(570,490))
			[SNew(SBox).Padding(24)[Content]]);
	}
}

class FFlyingCabNarrativeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		auto& Assets = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		FlyingCabNarrativeAssetCategory = Assets.RegisterAdvancedAssetCategory(TEXT("FlyingCab"), LOCTEXT("FlyingCabCategory", "Flying Cab"));
		AssetActions = {
			MakeShared<FNarrativeAssetActions>(UFlyingCabQuestDefinition::StaticClass(), LOCTEXT("QuestType", "Quest"), FColor(255,190,65)),
			MakeShared<FNarrativeAssetActions>(UFlyingCabDialogueDefinition::StaticClass(), LOCTEXT("DialogueType", "Conversation"), FColor(80,180,230)),
			MakeShared<FNarrativeAssetActions>(UFlyingCabNpcDefinition::StaticClass(), LOCTEXT("NpcType", "NPC profile"), FColor(160,115,230)),
			MakeShared<FNarrativeAssetActions>(UFlyingCabNpcRoster::StaticClass(), LOCTEXT("RosterType", "NPC roster"), FColor(120,190,155))};
		for (const auto& Action : AssetActions) Assets.RegisterAssetTypeActions(Action.ToSharedRef());
		auto& Properties = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		Properties.RegisterCustomPropertyTypeLayout(TEXT("FlyingCabQuestObjectiveDefinition"), FOnGetPropertyTypeCustomizationInstance::CreateLambda([]() { return MakeShared<FObjectiveDetails>(); }));
		for (const FName Type : {FName(TEXT("FlyingCabDialogueChoice")), FName(TEXT("FlyingCabDialogueEntry"))})
			Properties.RegisterCustomPropertyTypeLayout(Type, FOnGetPropertyTypeCustomizationInstance::CreateLambda([]() { return MakeShared<FDialogueLinkDetails>(); }));
		for (const FName Class : {FName(TEXT("FlyingCabQuestDefinition")), FName(TEXT("FlyingCabDialogueDefinition")), FName(TEXT("FlyingCabNpcDefinition")), FName(TEXT("FlyingCabNpcRoster"))})
			Properties.RegisterCustomClassLayout(Class, FOnGetDetailCustomizationInstance::CreateLambda([]() { return MakeShared<FNarrativeDetails>(); }));
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FFlyingCabNarrativeEditorModule::RegisterMenus));
		StaleModuleCheckHandle = FCoreDelegates::OnFEngineLoopInitComplete.AddStatic(&WarnIfCompiledModulesAreStale);
	}
	void RegisterMenus()
	{
		FToolMenuOwnerScoped Owner(this);
		auto* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
		Menu->FindOrAddSection(TEXT("FlyingCabNarrative")).AddMenuEntry(TEXT("FlyingCabNarrativeTools"),
			LOCTEXT("ToolsMenu", "Flying Cab — Quest & Dialogue tools"), LOCTEXT("ToolsTip", "Create and configure quests, NPCs and conversations."),
			FSlateIcon(), FUIAction(FExecuteAction::CreateStatic(&ShowAuthoringHub)));
	}
	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnFEngineLoopInitComplete.Remove(StaleModuleCheckHandle);
		UToolMenus::UnRegisterStartupCallback(this); UToolMenus::UnregisterOwner(this);
		if (auto* Properties = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
		{
			for (FName Type : {FName(TEXT("FlyingCabQuestObjectiveDefinition")), FName(TEXT("FlyingCabDialogueChoice")), FName(TEXT("FlyingCabDialogueEntry"))}) Properties->UnregisterCustomPropertyTypeLayout(Type);
			for (FName Class : {FName(TEXT("FlyingCabQuestDefinition")), FName(TEXT("FlyingCabDialogueDefinition")), FName(TEXT("FlyingCabNpcDefinition")), FName(TEXT("FlyingCabNpcRoster"))}) Properties->UnregisterCustomClassLayout(Class);
		}
		if (auto* Assets = FModuleManager::GetModulePtr<FAssetToolsModule>(TEXT("AssetTools")))
			for (auto& Action : AssetActions) Assets->Get().UnregisterAssetTypeActions(Action.ToSharedRef());
		AssetActions.Reset();
	}
private:
	TArray<TSharedPtr<IAssetTypeActions>> AssetActions;
	FDelegateHandle StaleModuleCheckHandle;
};

IMPLEMENT_MODULE(FFlyingCabNarrativeEditorModule, FlyingCabNarrativeEditor)
#undef LOCTEXT_NAMESPACE
