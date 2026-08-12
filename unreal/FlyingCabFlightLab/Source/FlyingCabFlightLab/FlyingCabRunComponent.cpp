// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabRunComponent.h"

#include "FlyingCabEconomyAsset.h"
#include "FlyingCabScoreSaveGame.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabRun, Log, All);

namespace
{
	const FString TimeAttackSaveSlot = TEXT("FlyingCabTimeAttackScores");
	constexpr int32 TimeAttackSaveUserIndex = 0;
}

UFlyingCabRunComponent::UFlyingCabRunComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlyingCabRunComponent::Configure(const UFlyingCabEconomyAsset* Config)
{
	if (Config)
	{
		TimeAttackTargetCredits = FMath::Max(1, Config->TimeAttackTargetCredits);
	}
}

void UFlyingCabRunComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnTimeAttackCompleted.Clear();
	Super::EndPlay(EndPlayReason);
}

bool UFlyingCabRunComponent::StartRun(EFlyingCabRunMode Mode)
{
	if (Mode == EFlyingCabRunMode::None || bRunActive)
	{
		return false;
	}

	CurrentRunMode = Mode;
	bRunActive = true;
	bRunCompleted = false;
	RunStartWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	RunCompletedDeliveries = 0;
	RunDeliveryCreditsEarned = 0;
	RunNearMissCount = 0;
	RunNearMissCreditsEarned = 0;
	RunFuelCreditsSpent = 0;
	RunRepairCreditsSpent = 0;
	RunTowCount = 0;
	RunTowCreditsSpent = 0;
	return true;
}

bool UFlyingCabRunComponent::CheckTimeAttackGoal(int32 CurrentCredits)
{
	if (!bRunActive || CurrentRunMode != EFlyingCabRunMode::TimeAttack
		|| CurrentCredits < TimeAttackTargetCredits)
	{
		return false;
	}

	const FFlyingCabTimeAttackResult Result = FinishTimeAttack(CurrentCredits);
	OnTimeAttackCompleted.Broadcast(Result);
	return true;
}

void UFlyingCabRunComponent::RecordDelivery(int32 CreditsEarned)
{
	if (bRunActive)
	{
		++RunCompletedDeliveries;
		RunDeliveryCreditsEarned += FMath::Max(0, CreditsEarned);
	}
}

void UFlyingCabRunComponent::RecordNearMiss(int32 CreditsEarned)
{
	if (bRunActive)
	{
		++RunNearMissCount;
		RunNearMissCreditsEarned += FMath::Max(0, CreditsEarned);
	}
}

void UFlyingCabRunComponent::RecordFuelPurchase(int32 CreditsSpent)
{
	if (bRunActive)
	{
		RunFuelCreditsSpent += FMath::Max(0, CreditsSpent);
	}
}

void UFlyingCabRunComponent::RecordRepairPurchase(int32 CreditsSpent)
{
	if (bRunActive)
	{
		RunRepairCreditsSpent += FMath::Max(0, CreditsSpent);
	}
}

void UFlyingCabRunComponent::RecordTow(int32 CreditsSpent)
{
	if (bRunActive)
	{
		++RunTowCount;
		RunTowCreditsSpent += FMath::Max(0, CreditsSpent);
	}
}

float UFlyingCabRunComponent::GetElapsedSeconds() const
{
	if (CurrentRunMode != EFlyingCabRunMode::TimeAttack || !GetWorld())
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - RunStartWorldTime);
}

TArray<float> UFlyingCabRunComponent::GetBestTimeAttackTimes() const
{
	if (!UGameplayStatics::DoesSaveGameExist(TimeAttackSaveSlot, TimeAttackSaveUserIndex))
	{
		return {};
	}

	const UFlyingCabScoreSaveGame* SaveGame = Cast<UFlyingCabScoreSaveGame>(
		UGameplayStatics::LoadGameFromSlot(TimeAttackSaveSlot, TimeAttackSaveUserIndex));
	return SaveGame
		? NormalizeLeaderboard(SaveGame->BestTimeAttackSeconds, TimeAttackLeaderboardSize)
		: TArray<float>();
}

TArray<float> UFlyingCabRunComponent::NormalizeLeaderboard(
	TArray<float> Times,
	int32 MaxEntries)
{
	Times.RemoveAll([](float Seconds) { return Seconds <= 0.0f; });
	Times.Sort();
	const int32 EffectiveMaxEntries = FMath::Max(0, MaxEntries);
	if (Times.Num() > EffectiveMaxEntries)
	{
		Times.SetNum(EffectiveMaxEntries);
	}
	return Times;
}

FFlyingCabTimeAttackResult UFlyingCabRunComponent::FinishTimeAttack(int32 FinalCredits)
{
	FFlyingCabTimeAttackResult Result;
	if (!bRunActive || bRunCompleted)
	{
		return Result;
	}

	Result.ElapsedSeconds = GetElapsedSeconds();
	Result.FinalCredits = FMath::Max(0, FinalCredits);
	Result.TargetCredits = TimeAttackTargetCredits;
	Result.CompletedDeliveries = RunCompletedDeliveries;
	Result.DeliveryCreditsEarned = RunDeliveryCreditsEarned;
	Result.NearMissCount = RunNearMissCount;
	Result.NearMissCreditsEarned = RunNearMissCreditsEarned;
	Result.FuelCreditsSpent = RunFuelCreditsSpent;
	Result.RepairCreditsSpent = RunRepairCreditsSpent;
	Result.TowCount = RunTowCount;
	Result.TowCreditsSpent = RunTowCreditsSpent;

	bRunActive = false;
	bRunCompleted = true;
	SaveTimeAttackScore(Result.ElapsedSeconds);
	return Result;
}

void UFlyingCabRunComponent::SaveTimeAttackScore(float ElapsedSeconds)
{
	if (ElapsedSeconds <= 0.0f)
	{
		return;
	}

	UFlyingCabScoreSaveGame* SaveGame = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(TimeAttackSaveSlot, TimeAttackSaveUserIndex))
	{
		SaveGame = Cast<UFlyingCabScoreSaveGame>(
			UGameplayStatics::LoadGameFromSlot(TimeAttackSaveSlot, TimeAttackSaveUserIndex));
	}
	if (!SaveGame)
	{
		SaveGame = Cast<UFlyingCabScoreSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UFlyingCabScoreSaveGame::StaticClass()));
	}
	if (!SaveGame)
	{
		UE_LOG(LogFlyingCabRun, Warning, TEXT("Could not create Time Attack score save."));
		return;
	}

	SaveGame->BestTimeAttackSeconds.Add(ElapsedSeconds);
	SaveGame->BestTimeAttackSeconds = NormalizeLeaderboard(
		MoveTemp(SaveGame->BestTimeAttackSeconds),
		TimeAttackLeaderboardSize);
	if (!UGameplayStatics::SaveGameToSlot(
		SaveGame,
		TimeAttackSaveSlot,
		TimeAttackSaveUserIndex))
	{
		UE_LOG(LogFlyingCabRun, Warning, TEXT("Could not persist Time Attack leaderboard."));
	}
}
