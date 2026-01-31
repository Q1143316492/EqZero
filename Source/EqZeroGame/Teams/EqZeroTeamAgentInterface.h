// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericTeamAgentInterface.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"

#include "EqZeroTeamAgentInterface.generated.h"

template <typename InterfaceType> class TScriptInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEqZeroTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, int32, OldTeamID, int32, NewTeamID);

inline int32 GenericTeamIdToInteger(FGenericTeamId ID)
{
	return (ID == FGenericTeamId::NoTeam) ? INDEX_NONE : (int32)ID;
}

inline FGenericTeamId IntegerToGenericTeamId(int32 ID)
{
	return (ID == INDEX_NONE) ? FGenericTeamId::NoTeam : FGenericTeamId((uint8)ID);
}

/** Interface for actors which can be associated with teams */
UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UEqZeroTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IEqZeroTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_IINTERFACE_BODY()

	virtual FOnEqZeroTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() { return nullptr; }

	static EQZEROGAME_API void ConditionalBroadcastTeamChanged(TScriptInterface<IEqZeroTeamAgentInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID);

	FOnEqZeroTeamIndexChangedDelegate& GetTeamChangedDelegateChecked()
	{
		FOnEqZeroTeamIndexChangedDelegate* Result = GetOnTeamIndexChangedDelegate();
		check(Result);
		return *Result;
	}
};
