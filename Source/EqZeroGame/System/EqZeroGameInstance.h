// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonGameInstance.h"
#include "InputActionValue.h"
#include "EqZeroGameInstance.generated.h"

namespace puerts { class FJsEnv; }

class AEqZeroPlayerController;
class UObject;

UCLASS(MinimalAPI, Config = Game)
class UEqZeroGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:

	UEqZeroGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	EQZEROGAME_API AEqZeroPlayerController* GetPrimaryPlayerController() const;

	EQZEROGAME_API virtual bool CanJoinRequestedSession() const override;
	EQZEROGAME_API virtual void HandlerUserInitialized(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext) override;

	// 服务器 收到客户端的加密请求，返回加密密钥
	EQZEROGAME_API virtual void ReceivedNetworkEncryptionToken(const FString& EncryptionToken, const FOnEncryptionKeyResponse& Delegate) override;

	// 客户端 收到服务器确认后，设置客户端的加密密钥
	EQZEROGAME_API virtual void ReceivedNetworkEncryptionAck(const FOnEncryptionKeyResponse& Delegate) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "EqZero|Hero")
	EQZEROGAME_API void OnNativeInputAction(FGameplayTag InputTag, const FInputActionValue& InputActionValue);
protected:

	EQZEROGAME_API virtual void Init() override;
	EQZEROGAME_API virtual void Shutdown() override;

	// 客户端在连接服务器前将 ?EncryptionToken=xxx 添加到 URL
	EQZEROGAME_API void OnPreClientTravelToSession(FString& URL);

	/** 
	 * 不安全，调试用的，正式的用https从服务器拿
	 */
	TArray<uint8> DebugTestEncryptionKey;

private:
	TSharedPtr<puerts::FJsEnv> GameScript;
};
