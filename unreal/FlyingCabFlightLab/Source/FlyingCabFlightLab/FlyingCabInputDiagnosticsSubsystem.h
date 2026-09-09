#pragma once

#include "CoreMinimal.h"
#include "FlyingCabInputDiagnosticsBuffer.h"
#include "Subsystems/WorldSubsystem.h"
#include "FlyingCabInputDiagnosticsSubsystem.generated.h"

class FFlyingCabDiagnosticLogObserver;

/** Development-only observer. Never binds, injects, flushes or corrects input. */
UCLASS()
class UFlyingCabInputDiagnosticsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override;

	void RequestDump(const FString& Reason = TEXT("manual"));
	FString GetHistoryText();
	const FString& GetLastScheduledDumpPath() const { return LastDumpPath; }
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
private:
	friend class FFlyingCabDiagnosticLogObserver;
	void ObserveLog(const TCHAR* Text);
	void ObserveActivation(bool bActive);
	void Record(const FString& Text);
	void WritePendingDump();
	void ResetObservations();

	FlyingCabInputDiagnostics::FHistory History;
	FlyingCabInputDiagnostics::FDiscrepancyLatch StaleActions[3];
	FlyingCabInputDiagnostics::FDiscrepancyLatch MissingForce;
	FFlyingCabDiagnosticLogObserver* LogObserver = nullptr;
	FDelegateHandle ActivationHandle;
	TWeakObjectPtr<APawn> TracePawn;
	FString LastState, LastGate, LastForce, LastCabRequest, PendingPayload, LastDumpPath;
	TArray<FString> PendingReasons;
	double NextSampleAt = 0.0, NextDumpAt = 0.0, TransitionGraceUntil = 0.0, PendingIncidentAt = 0.0;
	int32 LastFocus = -2, LastActivation = -2, AutomaticDumps = 0;
	bool bEnabledLastTick = false, bTraceKnown = false, bMissingForce = false, bPendingManual = false, bPendingTruncated = false;
};
