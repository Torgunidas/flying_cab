#if WITH_DEV_AUTOMATION_TESTS
#include "FlyingCabInputDiagnosticsBuffer.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabInputHistoryTest,
	"FlyingCab.Core.Input.DiagnosticsHistory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabInputHistoryTest::RunTest(const FString&)
{
	using namespace FlyingCabInputDiagnostics;
	FHistory History(3);
	History.Add(0.0, TEXT("expired"));
	History.Add(2.0, TEXT("event_first"));
	History.Add(3.0, TEXT("event_second"));
	History.Add(4.0, TEXT("event_third"));
	FString Dump = History.Export(5.0);
	TestFalse(TEXT("Capacity evicts oldest"), Dump.Contains(TEXT("expired")));
	TestTrue(TEXT("Capacity eviction is disclosed"), Dump.Contains(TEXT("capacity_drops_total=1")));
	TestTrue(TEXT("Wrapped entries stay chronological"), Dump.Find(TEXT("event_first")) < Dump.Find(TEXT("event_second"))
		&& Dump.Find(TEXT("event_second")) < Dump.Find(TEXT("event_third")));
	Dump = History.Export(8.0);
	TestFalse(TEXT("Old evidence expires without new events"), Dump.Contains(TEXT("event_first")));
	TestTrue(TEXT("Exactly five seconds retained"), Dump.Contains(TEXT("event_second")));
	TestTrue(TEXT("Whole ring expires"), History.Export(10.0).Contains(TEXT("samples=0")));
	History.Add(11.0, FString::ChrN(3000, TEXT('x')));
	TestTrue(TEXT("Oversized event is bounded and marked"), History.Export(11.0).Contains(TEXT("[truncated]")));
	History.Reset();
	TestTrue(TEXT("Reset clears evidence"), History.Export(11.0).Contains(TEXT("samples=0")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyingCabInputDiagnosticLatchTest,
	"FlyingCab.Core.Input.DiagnosticsPassiveLatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FFlyingCabInputDiagnosticLatchTest::RunTest(const FString&)
{
	FlyingCabInputDiagnostics::FDiscrepancyLatch Latch;
	TestFalse(TEXT("Transient first frame is not an incident"), Latch.Observe(true, 0.0));
	TestFalse(TEXT("Frame boundary noise is not an incident"), Latch.Observe(true, 0.01));
	TestTrue(TEXT("Persistent discrepancy is reported"), Latch.Observe(true, 0.11));
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		if (Latch.Observe(true, 1.0 + Index)) { AddError(TEXT("Repeated incident during same discrepancy")); break; }
	}
	TestFalse(TEXT("Neutral observation rearms"), Latch.Observe(false, 2000.0));
	TestFalse(TEXT("New incident starts fresh"), Latch.Observe(true, 2001.0));
	TestTrue(TEXT("New persistent incident reported"), Latch.Observe(true, 2001.2));
	return true;
}
#endif
