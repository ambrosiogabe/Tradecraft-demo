// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chunk.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "GameSaverAndLoader.h"
#include "Kismet/GameplayStatics.h"
#include "GameSaverAndLoader.h"
#include "Misc/Paths.h"
#include "MinecraftWorld.generated.h"

USTRUCT(BlueprintType)
struct FBlock_Properties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Damage_Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> Can_Break;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Current_Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 XP_Value;
};

UCLASS(Blueprintable)
class TRADECRAFT_API AMinecraftWorld : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMinecraftWorld();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void BuildChunkAt(FVector pos);

	void RemoveOldChunks();

	void BuildNearPlayer();

	void RecursivelyBuildWorld(FVector pos, int32 radius);

	bool IsSavedWorld = false;

	int32 ChunkRange = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 seed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> Worlds_To_Delete;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WorldName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString WorldDirectory;

	AActor* Player = nullptr;

	static FString BuildChunkName(FVector pos);

	int32 BreakOrAddBlock(FVector pos, bool addBlock, int32 id);

	UFUNCTION(BlueprintCallable)
		int32 BreakBlock(FVector pos);

	UFUNCTION(BlueprintCallable)
		int32 DealDamage(FVector pos, int32 damage);

	UFUNCTION(BlueprintCallable)
		void AddBlock(FVector pos, int32 id);

	UFUNCTION(BlueprintCallable)
		int32 GetTotalHealth(FVector pos);

	UFUNCTION(BlueprintCallable)
		int32 GetBlockAtPos(FVector pos);

	UFUNCTION(BlueprintCallable)
		void ExitAndSave(TArray<int32> ItemIds, TArray<int32> ItemCounts);

	UFUNCTION(BlueprintCallable)
		TArray<int32> LoadItemCounts();

	UFUNCTION(BlueprintCallable)
		bool IsWorldLoaded();

	UFUNCTION(BlueprintCallable)
		void SetWorldLoaded(bool loaded);

	UFUNCTION(BlueprintCallable)
		TArray<int32> LoadItemIds();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UMaterialInterface *> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBlock_Properties> Block_Props;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UGameInstance* GameInstance;

	TArray<int32> Block_Health_Values;

	bool WorldIsLoaded = false;

private:
	UPROPERTY()
	TMap<FString, AChunk*> Chunks;

	UPROPERTY()
	TArray<FString> ToRemove;

	// ********************************************************************************************************
	int32 ChunkWidth = 16; // IMPORTANT MAKE SURE THIS EQUALS WIDTHOFCHUNK IN CHUNK.CPP!!!!!!!!!!!!!!!!!!!!!!!!!
	// ********************************************************************************************************

	FVector LastBuildPosition;

	UGameSaverAndLoader* SaveGameInstance;

	TArray<int32> ItemIds;
	TArray<int32> ItemCounts;

	bool removingChunks = false;
};
