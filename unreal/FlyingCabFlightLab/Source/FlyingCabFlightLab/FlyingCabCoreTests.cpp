// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "FlyingCabProgressionSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlyingCabProgressionAccessTest,
	"FlyingCab.Core.Progression.AccessLifecycle",
	EAutomationTestFlags_ApplicationContextMask
		| EAutomationTestFlags::ProductFilter)

bool FFlyingCabProgressionAccessTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UFlyingCabProgressionSubsystem* Progression =
		NewObject<UFlyingCabProgressionSubsystem>(GameInstance);
	TestNotNull(TEXT("Progression subsystem can be created for an isolated test"), Progression);
	if (!Progression)
	{
		return false;
	}

	const FName ServiceAccess(TEXT("Vehicle.Service"));
	TestFalse(TEXT("No permission is granted initially"), Progression->HasAccess(ServiceAccess));
	TestFalse(TEXT("NAME_None cannot be granted"), Progression->GrantAccess(NAME_None));
	TestTrue(TEXT("A new permission is granted once"), Progression->GrantAccess(ServiceAccess));
	TestTrue(TEXT("Granted permission can be queried"), Progression->HasAccess(ServiceAccess));
	TestFalse(TEXT("Duplicate grants are rejected"), Progression->GrantAccess(ServiceAccess));

	Progression->ResetAccess();
	TestFalse(TEXT("Competitive-run reset removes session access"), Progression->HasAccess(ServiceAccess));
	return true;
}

#endif
