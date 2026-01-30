// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/SoftObjectPtr.h"

#include "EqZeroSystemStatics.generated.h"

template <typename T> class TSubclassOf;

class AActor;
class UActorComponent;
class UObject;
struct FFrame;

UCLASS()
class UEqZeroSystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "AssetManager", meta=(DeterminesOutputType=ExpectedAssetType))
	static TSoftObjectPtr<UObject> GetTypedSoftObjectReferenceFromPrimaryAssetId(FPrimaryAssetId PrimaryAssetId, TSubclassOf<UObject> ExpectedAssetType);

	UFUNCTION(BlueprintCallable)
	static FPrimaryAssetId GetPrimaryAssetIdFromUserFacingExperienceName(const FString& AdvertisedExperienceID);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="EqZero", meta = (WorldContext = "WorldContextObject"))
	static void PlayNextGame(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material", meta=(DefaultToSelf="TargetActor"))
	static void SetScalarParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const float ParameterValue, bool bIncludeChildActors = true);

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material", meta=(DefaultToSelf="TargetActor"))
	static void SetVectorParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const FVector ParameterValue, bool bIncludeChildActors = true);

	UFUNCTION(BlueprintCallable, Category = "Rendering|Material", meta=(DefaultToSelf="TargetActor"))
	static void SetColorParameterValueOnAllMeshComponents(AActor* TargetActor, const FName ParameterName, const FLinearColor ParameterValue, bool bIncludeChildActors = true);

	UFUNCTION(BlueprintCallable, Category = "Actor", meta=(DefaultToSelf="TargetActor", ComponentClass="/Script/Engine.ActorComponent", DeterminesOutputType="ComponentClass"))
	static TArray<UActorComponent*> FindComponentsByClass(AActor* TargetActor, TSubclassOf<UActorComponent> ComponentClass, bool bIncludeChildActors = true);
};
