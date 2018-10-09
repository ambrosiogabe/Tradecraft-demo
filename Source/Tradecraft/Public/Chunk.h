// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Chunk.generated.h"

struct FMeshSection
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FColor> VertexColors;

	int32 elem_id = 0;
};

struct FChunk_Block_Properties
{
	int32 Current_Health = 0;
	int32 id = 0;
};


UCLASS(Blueprintable)
class TRADECRAFT_API AChunk : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AChunk();

public:
	/// Generated Functions
	virtual void OnConstruction(const FTransform & Transform) override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Functions Used in the game
	void GenerateData();

	void SetLocation(FVector position, int32 seed);

	void LoadChunkValues(TArray<int32> ids);

	void GenerateLoadedChunkInWorld();

	void SetChunkMaterials(TArray<UMaterialInterface *> MaterialsToBeSet);

	void SetBlockHealthValues(TArray<int32> Values);

	void GenerateChunkInWorld();

	void MakeOwner(AActor* Parent);

	void ApplyMaterials();

	int32 BreakBlock(int32 x, int32 y, int32 z);

	void AddBlock(int32 x, int32 y, int32 z, int32 id);

	int32 DealDamage(int32 x, int32 y, int32 z, int32 damage);

	int32 GetBlockId(int32 id);

	// Variables used in the game
	FRandomStream RandomStream;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray <UMaterialInterface *> Materials;

	TArray<FIntVector> TreeCenters;

	int32 WidthOfChunk = 16;

	int32 HeightOfChunk = WidthOfChunk * (WidthOfChunk / 2);

	int32 WidthOfChunkExt;

	int32 HeightOfChunkExt;

	int32 VoxelWidth = 100;

	TArray<int32> Block_Health_Values;

	TArray<FChunk_Block_Properties> ChunkData;


private:
	UProceduralMeshComponent * mesh;

	void UpdateMesh();

	TArray<int32> CalculateNoise();

	TArray<int32> NoiseData;

	FVector ChunkPositionInWorld;

};


