// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineHotfixManager.h"
#include "Misc/CoreDelegates.h"
#include "Containers/Ticker.h"
#include "EqZeroHotfixManager.generated.h"

/**
 * UEqZeroHotfixManager
 * 
 * Manages runtime hotfixing of game configuration files (INI), device profiles, and assets.
 * Extends UOnlineHotfixManager to provide game-specific hotfix behavior.
 * 
 * Key Features:
 * - Downloads and applies hotfix files from cloud storage (e.g., INI files, JSON files)
 * - Supports device profile hotfixing with automatic reapplication
 * - Patches assets from INI files
 * - Tracks pending game hotfixes and notifies via multicast delegate
 * - Reloads network DDoS detection config after hotfix completion
 * 
 * Common Use Cases:
 * - Change game balance parameters without requiring a client patch
 * - Fix bugs in configuration files
 * - Update device-specific settings for different hardware
 * - Modify asset references or properties
 * 
 * Note: Hotfixing is disabled in editor builds to prevent interference with development.
 */
UCLASS()
class UEqZeroHotfixManager : public UOnlineHotfixManager
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPendingGameHotfix, bool);
	FOnPendingGameHotfix OnPendingGameHotfixChanged;

	UEqZeroHotfixManager();
	virtual ~UEqZeroHotfixManager();

	/** Request to patch assets from INI files (can be called when new plugins are loaded) */
	void RequestPatchAssetsFromIniFiles();

protected:
	/** Called when the hotfix process completes (success or failure) */
	void OnHotfixCompleted(EHotfixResult HotfixResult);

	/** Returns the local directory where hotfixed files are cached */
	virtual FString GetCachedDirectory() override
	{
		return FPaths::ProjectPersistentDownloadDir() / TEXT("Hotfix/");
	}

	/** Start the hotfix download and application process */
	virtual void StartHotfixProcess() override;

	/** Determine if a specific cloud file should be processed for hotfixing */
	virtual bool WantsHotfixProcessing(const struct FCloudFileHeader& FileHeader) override;
	
	/** Apply the hotfix processing for a downloaded file */
	virtual bool ApplyHotfixProcessing(const struct FCloudFileHeader& FileHeader) override;
	
	/** Determine if we should warn about missing assets when patching from INI */
	virtual bool ShouldWarnAboutMissingWhenPatchingFromIni(const FString& AssetPath) const override;
	
	/** Patch assets based on INI file configurations */
	virtual void PatchAssetsFromIniFiles() override;

	/** Called when hotfix availability is checked, before downloading */
	virtual void OnHotfixAvailablityCheck(const TArray<FCloudFileHeader>& PendingChangedFiles, const TArray<FCloudFileHeader>& PendingRemoveFiles) override;
	
	/** Apply hotfix to a specific INI file */
	virtual bool HotfixIniFile(const FString& FileName, const FString& IniData) override;

private:
#if !UE_BUILD_SHIPPING
	// Error reporting for debugging
	FDelegateHandle OnScreenMessageHandle;
	void GetOnScreenMessages(TMultiMap<FCoreDelegates::EOnScreenMessageSeverity, FText>& OutMessages);
#endif // !UE_BUILD_SHIPPING

private:
	/** Get the current game instance (typed outer) */
	template<typename T>
	T* GetGameInstance() const
	{
		return GetTypedOuter<T>();
	}

	void Init() override;

private:
	/** Handle for deferred RequestPatchAssets call */
	FTSTicker::FDelegateHandle RequestPatchAssetsHandle;
	
	/** Handle for hotfix completion delegate */
	FDelegateHandle HotfixCompleteDelegateHandle;

	/** True if there's a pending Game.ini hotfix that hasn't been applied yet */
	bool bHasPendingGameHotfix = false;
	
	/** True if there's a pending device profile hotfix that needs reapplication */
	bool bHasPendingDeviceProfileHotfix = false;

	/** Counter tracking how many times Game.ini has been hotfixed this session */
	static int32 GameHotfixCounter;
};
