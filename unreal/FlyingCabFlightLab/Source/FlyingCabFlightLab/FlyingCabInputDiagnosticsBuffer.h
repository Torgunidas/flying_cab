#pragma once

#include "CoreMinimal.h"

namespace FlyingCabInputDiagnostics
{
/** Bounded, game-thread-only evidence. This class has no access to gameplay input. */
class FHistory
{
public:
	explicit FHistory(int32 Capacity = 2048) { Entries.SetNum(FMath::Max(1, Capacity)); }
	void Add(double Now, const FString& Text)
	{
		Prune(Now);
		if (Count == Entries.Num()) { Pop(); ++CapacityDrops; }
		FEntry& Entry = Entries[(Head + Count++) % Entries.Num()];
		Entry.Time = Now;
		Entry.Text = Text.Len() > 2048 ? Text.Left(2048) + TEXT(" [truncated]") : Text;
	}
	FString Export(double Now)
	{
		Prune(Now);
		FString Result = FString::Printf(TEXT("history_seconds=5 samples=%d capacity_drops_total=%d\n"), Count, CapacityDrops);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FEntry& Entry = Entries[(Head + Index) % Entries.Num()];
			Result += FString::Printf(TEXT("%+.6fs %s\n"), Entry.Time - Now, *Entry.Text);
		}
		return Result;
	}
	void Reset() { Head = Count = CapacityDrops = 0; for (FEntry& Entry : Entries) { Entry.Text.Reset(); } }
private:
	struct FEntry { double Time = 0.0; FString Text; };
	void Pop() { Entries[Head].Text.Reset(); Head = (Head + 1) % Entries.Num(); --Count; }
	void Prune(double Now) { while (Count && Now - Entries[Head].Time > 5.0) { Pop(); } }
	TArray<FEntry> Entries;
	int32 Head = 0, Count = 0, CapacityDrops = 0;
};

/** Reports a sustained observation once. Never returns or alters a control value. */
struct FDiscrepancyLatch
{
	bool Observe(bool bMismatch, double Now)
	{
		if (!bMismatch) { Frames = 0; bReported = false; return false; }
		if (Frames == 0) { Started = Now; }
		Frames = FMath::Min(Frames + 1, 2);
		if (Frames >= 2 && Now - Started >= 0.1 && !bReported)
		{
			bReported = true;
			return true;
		}
		return false;
	}
	int32 Frames = 0;
	double Started = 0.0;
	bool bReported = false;
};
}
