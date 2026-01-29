// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericTeamAgentInterface.h"
#include "UObject/Object.h"

#include "EqZeroTeamAgentInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEqZeroTeamIndexChangedDelegate, UObject*, ObjectChangingTeam, FGenericTeamId, OldTeamID, FGenericTeamId, NewTeamID);


UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UEqZeroTeamAgentInterface : public UGenericTeamAgentInterface
{
	GENERATED_BODY()
};

class EQZEROGAME_API IEqZeroTeamAgentInterface : public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	virtual FOnEqZeroTeamIndexChangedDelegate& GetTeamChangedDelegateChecked() = 0;
};
