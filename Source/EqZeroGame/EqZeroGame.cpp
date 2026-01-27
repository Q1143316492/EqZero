// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroGame.h"
#include "Modules/ModuleManager.h"


/**
 * FEqZeroGameModule
 */
class FEqZeroGameModule : public FDefaultGameModuleImpl
{
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FEqZeroGameModule, EqZeroGame, "EqZeroGame");
