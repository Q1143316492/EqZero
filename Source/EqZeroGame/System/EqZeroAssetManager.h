// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/AssetManager.h"
#include "EqZeroAssetManagerStartupJob.h"
#include "Templates/SubclassOf.h"
#include "EqZeroAssetManager.generated.h"

class UPrimaryDataAsset;
class UEqZeroGameData;
class UEqZeroPawnData;

struct FEqZeroBundles
{
	static const FName Equipped;
};

/**
 * UEqZeroAssetManager
 *
 *	Game implementation of the asset manager that overrides functionality and stores game-specific types.
 *	It is expected that most games will want to override AssetManager as it provides a good place for game-specific loading logic.
 *	This class is used by setting 'AssetManagerClassName' in DefaultEngine.ini.
 *  资源管理器的游戏实现，它会重写功能并存储特定于游戏的类型。
 *  预计大多数游戏会想要重写 AssetManager，因为它为特定于游戏的加载逻辑提供了一个很好的位置。
 *  此类通过在 DefaultEngine.ini 中设置 “AssetManagerClassName” 来使用。
 * 
 *  这个类通过重写 StartInitialLoading 和 PreBeginPIE 让初始化的时候就加载一些资源
 */
UCLASS(Config = Game)
class EQZEROGAME_API UEqZeroAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:

	UEqZeroAssetManager();

	static UEqZeroAssetManager& Get();

	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	// 打印当前加载的资源列表
	static void DumpLoadedAssets();

	const UEqZeroGameData& GetGameData();
	const UEqZeroPawnData* GetDefaultPawnData() const;

protected:
	template <typename GameDataClass>
	const GameDataClass& GetOrLoadTypedGameData(const TSoftObjectPtr<GameDataClass>& DataPath)
	{
		if (TObjectPtr<UPrimaryDataAsset> const * pResult = GameDataMap.Find(GameDataClass::StaticClass()))
		{
			return *CastChecked<GameDataClass>(*pResult);
		}

		// 如果需要，会进行阻塞加载
		return *CastChecked<const GameDataClass>(LoadGameDataOfClass(GameDataClass::StaticClass(), DataPath, GameDataClass::StaticClass()->GetFName()));
	}

	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	static bool ShouldLogAssetLoads();

	// 线程安全地添加已加载的资源以保持在内存中。
	void AddLoadedAsset(const UObject* Asset);

	//~UAssetManager interface
	virtual void StartInitialLoading() override;
#if WITH_EDITOR
	virtual void PreBeginPIE(bool bStartSimulate) override;
#endif
	//~End of UAssetManager interface

	UPrimaryDataAsset* LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);

protected:
	UPROPERTY(Config)
	TSoftObjectPtr<UEqZeroGameData> EqZeroGameDataPath;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;

	UPROPERTY(Config)
	TSoftObjectPtr<UEqZeroPawnData> DefaultPawnData;

private:
	// Flushes the StartupJobs array. Processes all startup work.
	void DoAllStartupJobs();

	// Sets up the ability system
	void InitializeGameplayCueManager();

	// Called periodically during loads, could be used to feed the status to a loading screen
	void UpdateInitialGameContentLoadPercent(float GameContentPercent);

	// The list of tasks to execute on startup. Used to track startup progress.
	TArray<FEqZeroAssetManagerStartupJob> StartupJobs;

private:
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	// Used for a scope lock when modifying the list of load assets.
	FCriticalSection LoadedAssetsCritical;
};


template<typename AssetType>
AssetType* UEqZeroAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

template<typename AssetType>
TSubclassOf<AssetType> UEqZeroAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}
