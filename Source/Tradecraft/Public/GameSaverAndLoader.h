// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Chunk.h"
#include "Serialization/Archive.h"
#include "HAL/FileManager.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"
#include "Misc/FileHelper.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Misc/Paths.h"
#include "GameSaverAndLoader.generated.h"

/**
 * 
 */
USTRUCT()
struct FChunk_Ids
{
	GENERATED_BODY()

	TArray<int32> ids;
};

UCLASS()
class TRADECRAFT_API UGameSaverAndLoader : public USaveGame
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> AllSavedGames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SaveSlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UserIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector PlayerPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRotator CameraRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRotator PlayerRotation;

	UPROPERTY()
	TMap<FString, FChunk_Ids> ChunkIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SavedSeed;

	TArray<int32> ItemIds;
	TArray<int32> ItemCounts;

	void SaveLoadChunk(FArchive& Ar, TArray<int32>& ChunkIds);

	void SaveLoadInventory(FArchive& Ar, TArray<int32>& ItemIds, TArray<int32>& ItemCounts);

	// This method is overloaded to work with one array of Chunk Ids or two arrays, item ids and item counts.
	// The load game data is also overloaded.
	bool SaveGameDataToFileCompressed(const FString& FullFilePath, TArray<int32>&  ChunkIds);
	bool SaveGameDataToFileCompressed(const FString& FullFilePath, TArray<int32>& ItemIds, TArray<int32>& ItemCounts);

	bool LoadGameDataFromFileCompressed(const FString& FullFilePath, TArray<int32>& ChunkIds);
	bool LoadGameDataFromFileCompressed(const FString& FullFilePath, TArray<int32>& ItemIds, TArray<int32>& ItemCounts);

	bool VerifyOrCreateDirectory(const FString& FullFilePath);

	bool CreateOrEmptyFile(const FString& FullFilePath);

	bool CheckIfFileExists(FString WorldDirectory, FString ChunkName);

	void DeleteDirectory(FString WorldDirectory);

	void DeleteFile(FString FileName);

	UGameSaverAndLoader();
};