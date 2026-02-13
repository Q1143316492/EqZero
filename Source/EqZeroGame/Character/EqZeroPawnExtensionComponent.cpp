// Copyright Epic Games, Inc. All Rights Reserved.

#include "EqZeroPawnExtensionComponent.h"

#include "AbilitySystem/EqZeroAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "EqZeroGameplayTags.h"
#include "AbilitySystem/EqZeroAbilityTagRelationshipMapping.h"
#include "EqZeroLogChannels.h"
#include "EqZeroPawnData.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EqZeroPawnExtensionComponent)

class FLifetimeProperty;
class UActorComponent;

const FName UEqZeroPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

UEqZeroPawnExtensionComponent::UEqZeroPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	PawnData = nullptr;
	AbilitySystemComponent = nullptr;
}

void UEqZeroPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEqZeroPawnExtensionComponent, PawnData);
}

void UEqZeroPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

    // 合法性检查 必须挂在pawn上且唯一

	const APawn* Pawn = GetPawn<APawn>();
	ensureAlwaysMsgf((Pawn != nullptr), TEXT("EqZeroPawnExtensionComponent on [%s] can only be added to Pawn actors."), *GetNameSafe(GetOwner()));

	TArray<UActorComponent*> PawnExtensionComponents;
	Pawn->GetComponents(UEqZeroPawnExtensionComponent::StaticClass(), PawnExtensionComponents);
	ensureAlwaysMsgf((PawnExtensionComponents.Num() == 1), TEXT("Only one EqZeroPawnExtensionComponent should exist on [%s]."), *GetNameSafe(GetOwner()));

	RegisterInitStateFeature();
}

void UEqZeroPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

    // 启动状态机
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);
	ensure(TryToChangeInitState(EqZeroGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UEqZeroPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}

void UEqZeroPawnExtensionComponent::SetPawnData(const UEqZeroPawnData* InPawnData)
{
	check(InPawnData);

	APawn* Pawn = GetPawnChecked<APawn>();

	if (Pawn->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogEqZero, Error, TEXT("Trying to set PawnData [%s] on pawn [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(Pawn), *GetNameSafe(PawnData));
		return;
	}

	PawnData = InPawnData;

	Pawn->ForceNetUpdate();

	CheckDefaultInitialization();
}

void UEqZeroPawnExtensionComponent::OnRep_PawnData()
{
	CheckDefaultInitialization();
}

void UEqZeroPawnExtensionComponent::InitializeAbilitySystem(UEqZeroAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
	check(InASC);
	check(InOwnerActor);

	if (AbilitySystemComponent == InASC)
	{
		// ASC没有变化
		return;
	}

	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}

	APawn* Pawn = GetPawnChecked<APawn>();
	AActor* ExistingAvatar = InASC->GetAvatarActor();

	UE_LOG(LogEqZero, Verbose, TEXT("Setting up ASC [%s] on pawn [%s] owner [%s], existing [%s] "), *GetNameSafe(InASC), *GetNameSafe(Pawn), *GetNameSafe(InOwnerActor), *GetNameSafe(ExistingAvatar));
	if ((ExistingAvatar != nullptr) && (ExistingAvatar != Pawn))
	{
		// 已经有 pawn 作为 ASC 的 avatar, 所以我们需要干掉他
		// 如果客户端出现延迟，就可能发生这种情况：他们的新角色在死亡的角色被移除之前就已生成并被控制。
		ensure(!ExistingAvatar->HasAuthority());

		if (UEqZeroPawnExtensionComponent* OtherExtensionComponent = FindPawnExtensionComponent(ExistingAvatar))
		{
			OtherExtensionComponent->UninitializeAbilitySystem();
		}
	}

	AbilitySystemComponent = InASC;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);

	if (ensure(PawnData))
	{
		InASC->SetTagRelationshipMapping(Cast<UEqZeroAbilityTagRelationshipMapping>(PawnData->TagRelationshipMapping));
	}

	OnAbilitySystemInitialized.Broadcast();
}

void UEqZeroPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
	// 如果我们仍然是化身角色，就初始化 ASC（否则，另一个 pawn 在成为化身角色时已经完成了初始化）
	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		FGameplayTagContainer AbilityTypesToIgnore;
		AbilityTypesToIgnore.AddTag(EqZeroGameplayTags::Ability_Behavior_SurvivesDeath);

		AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			// If the ASC doesn't have a valid owner, we need to clear *all* actor info, not just the avatar pairing
			AbilitySystemComponent->ClearActorInfo();
		}

		OnAbilitySystemUninitialized.Broadcast();
	}

	AbilitySystemComponent = nullptr;
}

void UEqZeroPawnExtensionComponent::HandleControllerChanged()
{
	if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()))
	{
		ensure(AbilitySystemComponent->AbilityActorInfo->OwnerActor == AbilitySystemComponent->GetOwnerActor());
		if (AbilitySystemComponent->GetOwnerActor() == nullptr)
		{
			UninitializeAbilitySystem();
		}
		else
		{
			AbilitySystemComponent->RefreshAbilityActorInfo();
		}
	}

	CheckDefaultInitialization();
}

void UEqZeroPawnExtensionComponent::HandlePlayerStateReplicated()
{
	CheckDefaultInitialization();
}

void UEqZeroPawnExtensionComponent::SetupPlayerInputComponent()
{
	CheckDefaultInitialization();
}

void UEqZeroPawnExtensionComponent::CheckDefaultInitialization()
{
	CheckDefaultInitializationForImplementers();

	static const TArray<FGameplayTag> StateChain = {
		EqZeroGameplayTags::InitState_Spawned,
		EqZeroGameplayTags::InitState_DataAvailable,
		EqZeroGameplayTags::InitState_DataInitialized,
		EqZeroGameplayTags::InitState_GameplayReady };

	ContinueInitStateChain(StateChain);
}

bool UEqZeroPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();
	if (!CurrentState.IsValid() && DesiredState == EqZeroGameplayTags::InitState_Spawned)
	{
		if (Pawn)
		{
			return true;
		}
	}
	if (CurrentState == EqZeroGameplayTags::InitState_Spawned && DesiredState == EqZeroGameplayTags::InitState_DataAvailable)
	{
		// Pawn data 加载了，这个会在体验加载后，从PlayerState设置过来
		if (!PawnData)
		{
			return false;
		}

		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		if (bHasAuthority || bIsLocallyControlled)
		{
			// 检查已经 possessed by a controller.
			if (!GetController<AController>())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == EqZeroGameplayTags::InitState_DataAvailable && DesiredState == EqZeroGameplayTags::InitState_DataInitialized)
	{
		// 所有注册的组件都达到了 DataAvailable 状态
		bool Result = Manager->HaveAllFeaturesReachedInitState(Pawn, EqZeroGameplayTags::InitState_DataAvailable);
		return Result;
	}
	else if (CurrentState == EqZeroGameplayTags::InitState_DataInitialized && DesiredState == EqZeroGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UEqZeroPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == EqZeroGameplayTags::InitState_DataInitialized)
	{
	}
}

void UEqZeroPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	// 如果现在有另一个功能处于 DataAvailable 状态，看看我们是否应该过渡到 DataInitialized 状态
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		if (Params.FeatureState == EqZeroGameplayTags::InitState_DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
}

void UEqZeroPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void UEqZeroPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}