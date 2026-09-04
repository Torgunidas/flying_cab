// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlyingCabProgressionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogFlyingCabProgression, Log, All);

bool UFlyingCabProgressionSubsystem::HasAccess(FName AccessId) const
{
	return !AccessId.IsNone() && GrantedAccess.Contains(AccessId);
}

bool UFlyingCabProgressionSubsystem::GrantAccess(FName AccessId)
{
	if (AccessId.IsNone() || GrantedAccess.Contains(AccessId))
	{
		return false;
	}

	GrantedAccess.Add(AccessId);
	UE_LOG(LogFlyingCabProgression, Display, TEXT("Access granted: %s."), *AccessId.ToString());
	return true;
}

bool UFlyingCabProgressionSubsystem::RevokeAccess(FName AccessId)
{
	if (AccessId.IsNone() || GrantedAccess.Remove(AccessId) == 0)
	{
		return false;
	}

	UE_LOG(LogFlyingCabProgression, Display, TEXT("Access revoked: %s."), *AccessId.ToString());
	return true;
}

void UFlyingCabProgressionSubsystem::ResetAccess()
{
	GrantedAccess.Reset();
	UE_LOG(LogFlyingCabProgression, Display, TEXT("Session access reset for a new competitive run."));
}
