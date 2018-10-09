// Fill out your copyright notice in the Description page of Project Settings.

#include "GameSaverAndLoader.h"

UGameSaverAndLoader::UGameSaverAndLoader()
{
	SaveSlotName = TEXT("Test Save Slot");
	UserIndex = 0;
}

void UGameSaverAndLoader::SaveLoadChunk(FArchive& Ar, TArray<int32>& ChunkIds)
{
	Ar << ChunkIds;
}

void UGameSaverAndLoader::SaveLoadInventory(FArchive& Ar, TArray<int32>& ItemIds, TArray<int32>& ItemCounts)
{
	Ar << ItemIds;
	Ar << ItemCounts;
}

bool UGameSaverAndLoader::SaveGameDataToFileCompressed(const FString& FullFilePath, TArray<int32>& ItemIds, TArray<int32>& ItemCounts)
{
	FBufferArchive ToBinary;
	SaveLoadInventory(ToBinary, ItemIds, ItemCounts);

	// Compress the file
	TArray<uint8> CompressedData;
	FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);

	Compressor << ToBinary;
	Compressor.Flush();

	if (FFileHelper::SaveArrayToFile(CompressedData, *FullFilePath))
	{
		Compressor.FlushCache();
		CompressedData.Empty();

		ToBinary.FlushCache();
		ToBinary.Empty();

		ToBinary.Close();

		return true;
	}
	else
	{
		Compressor.FlushCache();
		CompressedData.Empty();

		ToBinary.FlushCache();
		ToBinary.Empty();

		ToBinary.Close();
		UE_LOG(LogTemp, Warning, TEXT("File Could not be saved."));
		return false;
	}
}

bool UGameSaverAndLoader::SaveGameDataToFileCompressed(const FString& FullFilePath, TArray<int32>&  ChunkIds) 
{
	FBufferArchive ToBinary;
	SaveLoadChunk(ToBinary, ChunkIds);

	// Compress the file
	TArray<uint8> CompressedData;
	FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);

	Compressor << ToBinary;
	Compressor.Flush();

	if (FFileHelper::SaveArrayToFile(CompressedData, *FullFilePath)) 
	{
		Compressor.FlushCache();
		CompressedData.Empty();

		ToBinary.FlushCache();
		ToBinary.Empty();

		ToBinary.Close();

		return true;
	}
	else
	{
		Compressor.FlushCache();
		CompressedData.Empty();

		ToBinary.FlushCache();
		ToBinary.Empty();

		ToBinary.Close();
		UE_LOG(LogTemp, Warning, TEXT("File Could not be saved."));
		return false;
	}
}

bool UGameSaverAndLoader::LoadGameDataFromFileCompressed(const FString& FullFilePath, TArray<int32>&  ChunkIds)
{
	TArray<uint8> CompressedData;
	if (!FFileHelper::LoadFileToArray(CompressedData, *FullFilePath)) 
	{
		UE_LOG(LogTemp, Warning, TEXT("File Helper:: Invalid File"));
		return false;
	}

	FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);

	if (Decompressor.GetError()) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Decompressor error: File was not compressed"));
		return false;
	}

	FBufferArchive DecompressedBinaryArray;
	Decompressor << DecompressedBinaryArray;

	FMemoryReader FromBinary = FMemoryReader(DecompressedBinaryArray, true);
	FromBinary.Seek(0);
	SaveLoadChunk(FromBinary, ChunkIds);

	CompressedData.Empty();
	Decompressor.FlushCache();
	FromBinary.FlushCache();

	DecompressedBinaryArray.Empty();
	DecompressedBinaryArray.Close();

	return true;
}


bool UGameSaverAndLoader::LoadGameDataFromFileCompressed(const FString& FullFilePath, TArray<int32>&  ItemIds, TArray<int32>& ItemCounts)
{
	TArray<uint8> CompressedData;
	if (!FFileHelper::LoadFileToArray(CompressedData, *FullFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("File Path:: %s"), *FullFilePath);
		UE_LOG(LogTemp, Warning, TEXT("File Helper:: Invalid File"));
		return false;
	}

	FArchiveLoadCompressedProxy Decompressor = FArchiveLoadCompressedProxy(CompressedData, ECompressionFlags::COMPRESS_ZLIB);

	if (Decompressor.GetError())
	{
		UE_LOG(LogTemp, Warning, TEXT("Decompressor error: File was not compressed"));
		return false;
	}

	FBufferArchive DecompressedBinaryArray;
	Decompressor << DecompressedBinaryArray;

	FMemoryReader FromBinary = FMemoryReader(DecompressedBinaryArray, true);
	FromBinary.Seek(0);
	SaveLoadInventory(FromBinary, ItemIds, ItemCounts);

	CompressedData.Empty();
	Decompressor.FlushCache();
	FromBinary.FlushCache();

	DecompressedBinaryArray.Empty();
	DecompressedBinaryArray.Close();

	return true;
}

bool UGameSaverAndLoader::VerifyOrCreateDirectory(const FString& FullFilePath)
{
	if (!IFileManager::Get().DirectoryExists(*FullFilePath))
	{
		IFileManager::Get().MakeDirectory(*FullFilePath);
		if (!IFileManager::Get().DirectoryExists(*FullFilePath)) return false;
	}
	return true;
}

bool UGameSaverAndLoader::CreateOrEmptyFile(const FString& FullFilePath)
{
	if (!IFileManager::Get().FileExists(*FullFilePath)) 
	{
		IFileManager::Get().CreateFileWriter(*FullFilePath);
		if (!IFileManager::Get().FileExists(*FullFilePath)) return false;
	}
	else 
	{
		IFileManager::Get().Delete(*FullFilePath);
		IFileManager::Get().CreateFileWriter(*FullFilePath);
		if (!IFileManager::Get().FileExists(*FullFilePath)) return false;
	}
	return true;
}

bool UGameSaverAndLoader::CheckIfFileExists(FString WorldDirectory, FString ChunkName)
{
	FString PathName = FPaths::Combine(WorldDirectory, ChunkName);
	if (IFileManager::Get().FileExists(*PathName)) return true;
	return false;
}

void UGameSaverAndLoader::DeleteFile(FString FileName) 
{
	IFileManager::Get().Delete(*FileName);
}

void UGameSaverAndLoader::DeleteDirectory(FString WorldDirectory) 
{
	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *WorldDirectory);
	for (FString elem : FoundFiles) 
	{
		FString Path = FPaths::Combine(WorldDirectory, elem);
		IFileManager::Get().Delete(*Path);
	}
	IFileManager::Get().DeleteDirectory(*WorldDirectory);
}





