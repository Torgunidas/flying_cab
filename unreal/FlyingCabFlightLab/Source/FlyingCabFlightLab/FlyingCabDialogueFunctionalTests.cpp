#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "FlyingCabGameMode.h"
#include "FlyingCabPlayerController.h"
#include "FlyingCabCharacter.h"
#include "FlyingCabQuestGiver.h"
#include "FlyingCabQuestSubsystem.h"
#include "FlyingCabQuestDefinition.h"
#include "FlyingCabDialogueSession.h"
#include "FlyingCabDialogueWidget.h"
#include "InputKeyEventArgs.h"
#include "InputCoreTypes.h"

namespace
{
	class FVerifyNpcConversation : public IAutomationLatentCommand
	{
	public:
		explicit FVerifyNpcConversation(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds() + 20) {}
		virtual bool Update() override
		{
			auto* World = AutomationCommon::GetAnyGameWorld();
			auto* PC = World ? Cast<AFlyingCabPlayerController>(World->GetFirstPlayerController()) : nullptr;
			auto* GM = World ? World->GetAuthGameMode<AFlyingCabGameMode>() : nullptr;
			if (FPlatformTime::Seconds() > Deadline)
			{
				if (PC) PC->CloseDialogue();
				Test->AddError(FString::Printf(TEXT("NPC conversation timed out at phase %d"), Phase)); return true;
			}
			if (!PC || !GM) return false;
			auto* Quests = World->GetGameInstance()->GetSubsystem<UFlyingCabQuestSubsystem>();
			if (!Quests) return false;
			if (Phase == 0)
			{
				PC->StartRunMode(EFlyingCabRunMode::Freeroam);
				PC->RequestContextInteraction(); // canonical deferred exit from the cab
				++Phase; return false;
			}
			auto* Character = Cast<AFlyingCabCharacter>(PC->GetPawn());
			if (!Character) return false;
			if (Phase == 1)
			{
				for (TActorIterator<AFlyingCabQuestGiver> It(World); It; ++It)
					if (It->GetNpcId() == TEXT("QuestGiver.Mike")) { Mike = *It; break; }
				if (!Mike.IsValid()) return false;
				Test->TestNotNull(TEXT("Mike has an editable profile"), Mike->GetNpcProfile());
				Character->SetActorLocation(Mike->GetActorLocation() + FVector(-100,0,50));
				PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Pressed, 1));
				++Phase; return false;
			}
			if (Phase == 2)
			{
				PC->RequestContextInteraction();
				++Phase; return false;
			}
			if (Phase == 3)
			{
				if (!PC->IsDialogueOpen()) return false;
				Test->TestTrue(TEXT("Conversation pauses the world"), World->IsPaused());
				Test->TestTrue(TEXT("Conversation suppresses gameplay"), PC->IsGameplayInputSuppressed());
				Test->TestEqual(TEXT("Opening Q does not accept the quest"), Quests->GetQuestStatus(TEXT("Quest.NightshiftContract")), EFlyingCabQuestStatus::Inactive);
				VerifyPresentation(PC);
				PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::A, IE_Released, 0));
				PC->CloseDialogue(); ++Phase; return false;
			}
			if (Phase == 4)
			{
				Test->TestFalse(TEXT("Close resumes gameplay"), World->IsPaused());
				Test->TestFalse(TEXT("Close clears conversation mode"), PC->IsDialogueOpen());
				if (PC->IsGameplayInputSuppressed()) return false; // one canonical transition frame
				Test->TestTrue(TEXT("Released A stays neutral"), FMath::IsNearlyZero(Character->GetTestKeyboardHorizontalInput()));
				Test->TestTrue(TEXT("Reopen at same NPC"), PC->OpenDialogue(Mike.Get()));
				auto* Session = PC->GetDialogueSession();
				if (!Session) { Test->AddError(TEXT("Missing conversation session")); return true; }
				Session->ChooseOption(0, Session->GetView().Revision);
				Session->ChooseOption(0, Session->GetView().Revision);
				Test->TestEqual(TEXT("Explicit answer accepts"), Quests->GetQuestStatus(TEXT("Quest.NightshiftContract")), EFlyingCabQuestStatus::Active);
				PC->CloseDialogue();
				Quests->RecordEvent(FlyingCabQuestEvents::PassengerDelivered, NAME_None, 2);
				++Phase; return false;
			}
			if (Phase == 5)
			{
				const int32 CreditsBefore = GM->GetCredits();
				PC->OpenDialogue(Mike.Get());
				auto* Session = PC->GetDialogueSession();
				if (!Session) return false;
				Session->ChooseOption(0, Session->GetView().Revision);
				const int32 Revision = Session->GetView().Revision;
				Test->TestTrue(TEXT("Turn-in answer succeeds"), Session->ChooseOption(0, Revision));
				Test->TestFalse(TEXT("Repeated turn-in click rejected"), Session->ChooseOption(0, Revision));
				Test->TestEqual(TEXT("Actual economy receives one quest reward"), GM->GetCredits(), CreditsBefore + Mike->GetQuestDefinition()->Reward.Credits);
				PC->CloseDialogue();
				PC->OpenDialogue(Mike.Get());
				Mike->Destroy();
				Test->TestFalse(TEXT("Losing the NPC closes the modal"), PC->IsDialogueOpen());
				Test->TestFalse(TEXT("Losing the NPC resumes the world"), World->IsPaused());
				return true;
			}
			return true;
		}
	private:
		/** The line must type itself in, and no answer may be taken before it has arrived. */
		void VerifyPresentation(AFlyingCabPlayerController* PC)
		{
			auto* Widget = PC->GetDialogueWidget();
			auto* Session = PC->GetDialogueSession();
			Test->TestNotNull(TEXT("The conversation view exists"), Widget);
			if (!Widget || !Session) return;
			Test->TestFalse(TEXT("Opening does not show the line at once"), Widget->IsRevealComplete());
			for (int32 Step = 0; Step < 40 && Widget->GetTotalCharacterCount() == 0; ++Step)
			{
				Widget->AdvancePresentation(0.02f);
			}
			Test->TestTrue(TEXT("The line is wrapped and starts typing"), Widget->GetTotalCharacterCount() > 0);
			Test->TestTrue(TEXT("The line types in rather than appearing whole"),
				Widget->GetRevealedCharacterCount() < Widget->GetTotalCharacterCount());
			Test->TestEqual(TEXT("No answer arrives while the line types"), Widget->GetRevealedOptionCount(), 0);
			const int32 RevisionBefore = Session->GetView().Revision;
			Widget->HandleDialogueKey(EKeys::One);
			Test->TestEqual(TEXT("A number key before the answer arrives changes nothing"),
				Session->GetView().Revision, RevisionBefore);
			Widget->SkipReveal();
			Test->TestEqual(TEXT("Skipping shows the whole line"),
				Widget->GetRevealedCharacterCount(), Widget->GetTotalCharacterCount());
			for (int32 Step = 0; Step < 40 && !Widget->IsRevealComplete(); ++Step)
			{
				Widget->AdvancePresentation(0.02f);
			}
			Test->TestTrue(TEXT("A skipped line brings the answers right away"), Widget->IsRevealComplete());
			Test->TestTrue(TEXT("Every answer is on screen"), Widget->GetRevealedOptionCount() > 0);
		}

		FAutomationTestBase* Test;
		double Deadline;
		int32 Phase = 0;
		TWeakObjectPtr<AFlyingCabQuestGiver> Mike;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabNpcConversationPIETest,
	"FlyingCab.Functional.PIE.NpcConversation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabNpcConversationPIETest::RunTest(const FString&)
{
	if (!AutomationOpenMap(TEXT("/Game/Maps/FlightLab"), true)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyNpcConversation(this));
	return true;
}
#endif
