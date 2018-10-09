// Fill out your copyright notice in the Description page of Project Settings.

#include "Chunk.h"
#include "EngineUtils.h"
#include "Math/UnrealMathUtility.h"
#include "SimplexNoiseLibrary.h"
#include "MinecraftWorld.h"


const TArray<int32> bTriangles = { 2, 1, 0, 0, 3, 2 };
const FVector NormalsUp[] = { FVector::UpVector, FVector::UpVector, FVector::UpVector, FVector::UpVector };
const FVector NormalsDown[] = { -FVector::UpVector, -FVector::UpVector, -FVector::UpVector, -FVector::UpVector };
const FVector NormalsForward[] = { FVector::ForwardVector, FVector::ForwardVector, FVector::ForwardVector, FVector::ForwardVector };
const FVector NormalsBack[] = { -FVector::ForwardVector, -FVector::ForwardVector, -FVector::ForwardVector, -FVector::ForwardVector };
const FVector NormalsRight[] = { FVector::RightVector, FVector::RightVector, FVector::RightVector, FVector::RightVector };
const FVector NormalsLeft[] = { -FVector::RightVector, -FVector::RightVector, -FVector::RightVector, -FVector::RightVector };
const FVector2D bUVs[] = { FVector2D(0., 0.), FVector2D(0., 1.), FVector2D(1., 1.), FVector2D(1., 0.) };
const FVector bMask[] = { FVector(0., 0., 1.), FVector(0., 0., -1.), FVector(0., 1., 0.), FVector(0., -1., 0.), FVector(1., 0., 0.), FVector(-1., 0., 0) };

// Sets default values
AChunk::AChunk()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	// PrimaryActorTick.bCanEverTick = true;
}

// Called every frame
void AChunk::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AChunk::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);

	mesh = NewObject<UProceduralMeshComponent>(this, FName("Generated Mesh"));
	mesh->RegisterComponent();

	RootComponent = mesh;
	RootComponent->SetWorldTransform(FTransform());

	WidthOfChunkExt = WidthOfChunk + 2;
	HeightOfChunkExt = WidthOfChunkExt * WidthOfChunkExt;
	ChunkData.Init(FChunk_Block_Properties(), (WidthOfChunkExt) * (WidthOfChunkExt) * (HeightOfChunk));

	mesh->bUseAsyncCooking = true;
}

void AChunk::GenerateChunkInWorld()
{
	NoiseData = CalculateNoise();
	GenerateData();
	UpdateMesh();
	ApplyMaterials();
}

void AChunk::GenerateLoadedChunkInWorld() 
{
	UpdateMesh();
	ApplyMaterials();
}

void AChunk::SetChunkMaterials(TArray<UMaterialInterface*> MaterialsToBeSet)
{
	Materials = MaterialsToBeSet;
}

void AChunk::SetBlockHealthValues(TArray<int32> Values) 
{
	Block_Health_Values.Init(0, Values.Num());
	for (int i = 0; i < Block_Health_Values.Num(); i++)
	{
		Block_Health_Values[i] = Values[i];
	}
}

void AChunk::SetLocation(FVector position, int32 seed)
{
	SetActorLocation(position);
	USimplexNoiseLibrary::setNoiseSeed(seed);
	RandomStream.Initialize(seed);
	FString NewName = AMinecraftWorld::BuildChunkName(FVector(position.X / 100, position.Y / 100, position.Z / 100));
	Rename(*NewName);
}

void AChunk::MakeOwner(AActor* Parent)
{
	this->SetOwner(Parent);
}

void AChunk::ApplyMaterials()
{
	int s = 0;
	while (s < Materials.Num())
	{
		mesh->SetMaterial(s, Materials[s]);
		s++;
	}
}

void AChunk::LoadChunkValues(TArray<int32> ids) 
{
	for (int32 i = 0; i < ChunkData.Num(); i++) 
	{
		if (i < ids.Num()) 
		{
			ChunkData[i].id = ids[i];
		}
	}
	UpdateMesh();
	ApplyMaterials();
}

void AChunk::UpdateMesh()
{
	TArray<FMeshSection> MeshSections;
	MeshSections.SetNum(Materials.Num());
	int Triangle_Num = 0;
	int TotalTriangles = 0;

	for (int x = 0; x < WidthOfChunk; x++) {

		for (int y = 0; y < WidthOfChunk; y++) {

			for (int z = 0; z < HeightOfChunk; z++) {

				int32 index = z + ((y + 1) * HeightOfChunk) + ((x + 1) * WidthOfChunkExt * HeightOfChunk);

				if (index >= ChunkData.Num() || index < 0)
				{
					UE_LOG(LogTemp, Error, TEXT("Index out of bounds: %d. Chunk Size is: %d"), index, ChunkData.Num());
					break;
				}

				int32 CurrentBlock = ChunkData[index].id;
				int32 meshIndex = CurrentBlock;

				if (meshIndex >= MeshSections.Num() || meshIndex < 0) 
				{
					UE_LOG(LogTemp, Warning, TEXT("There are no materials in Minecraft World."), meshIndex, MeshSections.Num());
					break;
				}

				TArray<FVector> &Vertices = MeshSections[meshIndex].Vertices;
				TArray<int32> &Triangles = MeshSections[meshIndex].Triangles;
				TArray<FVector> &Normals = MeshSections[meshIndex].Normals;
				TArray<FVector2D> &UVs = MeshSections[meshIndex].UVs;
				TArray<FProcMeshTangent> &Tangents = MeshSections[meshIndex].Tangents;
				TArray<FColor> &VertexColors = MeshSections[meshIndex].VertexColors;
				int32 elementID = MeshSections[meshIndex].elem_id;

				//All possible vertices
				FVector p0 = FVector((x * 100) - 50.f, (y * 100) - 50.f, (z * 100) + 50.f);
				FVector p1 = FVector((x * 100) + 50.f, (y * 100) - 50.f, (z * 100) + 50.f);
				FVector p2 = FVector((x * 100) + 50.f, (y * 100) - 50.f, (z * 100) - 50.f);
				FVector p3 = FVector((x * 100) - 50.f, (y * 100) - 50.f, (z * 100) - 50.f);
				FVector p4 = FVector((x * 100) - 50.f, (y * 100) + 50.f, (z * 100) + 50.f);
				FVector p5 = FVector((x * 100) + 50.f, (y * 100) + 50.f, (z * 100) + 50.f);
				FVector p6 = FVector((x * 100) + 50.f, (y * 100) + 50.f, (z * 100) - 50.f);
				FVector p7 = FVector((x * 100) - 50.f, (y * 100) + 50.f, (z * 100) - 50.f);

				if(CurrentBlock >= 0 && CurrentBlock < Block_Health_Values.Num())
					ChunkData[index].Current_Health = Block_Health_Values[CurrentBlock];

				// Triangle Number helps us keep track of which triangle we are on
				int Triangle_Num = 0;

				if (CurrentBlock != 0)
				{
					for (int i = 0; i < 6; i++)
					{
						int newIndex = (z + bMask[i].Z) + ((y + bMask[i].Y + 1) * HeightOfChunk) + ((x + bMask[i].X + 1) * WidthOfChunkExt * HeightOfChunk);

						bool flag = false;
						if (newIndex < ChunkData.Num() && newIndex >= 0)
							if (ChunkData[newIndex].id == 0 || ChunkData[newIndex].id == 5) flag = true;	// if see through or none

						if (flag)
						{
							Triangles.Add(bTriangles[0] + Triangle_Num + elementID);
							Triangles.Add(bTriangles[1] + Triangle_Num + elementID);
							Triangles.Add(bTriangles[2] + Triangle_Num + elementID);
							Triangles.Add(bTriangles[3] + Triangle_Num + elementID);
							Triangles.Add(bTriangles[4] + Triangle_Num + elementID);
							Triangles.Add(bTriangles[5] + Triangle_Num + elementID);
							Triangle_Num += 4; // Increment the Triangle Number by four since we have just added four vertices, hence the triangles on this side

							switch (i)
							{
							case 0: // Top Face of cube 
							{
								Vertices.Add(p4);
								Vertices.Add(p0);
								Vertices.Add(p1);
								Vertices.Add(p5);

								Normals.Append(NormalsUp, ARRAY_COUNT(NormalsUp));
								break;
							}
							case 1: // Bottom Face of Cube
							{
								Vertices.Add(p2);
								Vertices.Add(p3);
								Vertices.Add(p7);
								Vertices.Add(p6);

								Normals.Append(NormalsDown, ARRAY_COUNT(NormalsDown));
								break;
							}
							case 2: // Front Face of Cube, Forward
							{
								Vertices.Add(p5);
								Vertices.Add(p6);
								Vertices.Add(p7);
								Vertices.Add(p4);

								Normals.Append(NormalsForward, ARRAY_COUNT(NormalsForward));
								break;
							}
							case 3: // Back Face of Cube
							{
								Vertices.Add(p0);
								Vertices.Add(p3);
								Vertices.Add(p2);
								Vertices.Add(p1);

								Normals.Append(NormalsBack, ARRAY_COUNT(NormalsBack));
								break;
							}
							case 4:
							{
								Vertices.Add(p1);
								Vertices.Add(p2);
								Vertices.Add(p6);
								Vertices.Add(p5);

								Normals.Append(NormalsRight, ARRAY_COUNT(NormalsRight));
								break;
							}
							case 5:
							{
								Vertices.Add(p4);
								Vertices.Add(p7);
								Vertices.Add(p3);
								Vertices.Add(p0);

								Normals.Append(NormalsLeft, ARRAY_COUNT(NormalsLeft));
								break;
							}
							}
							//Finish Switch Statement

							//Add UVs
							UVs.Append(bUVs, ARRAY_COUNT(bUVs));
							FColor color = FColor(255, 255, 255, i);
							VertexColors.Add(color); VertexColors.Add(color); VertexColors.Add(color); VertexColors.Add(color);
						}
					}
					TotalTriangles += Triangle_Num;
					MeshSections[meshIndex].elem_id += Triangle_Num;
				}
			}
		}
	}

	mesh->ClearAllMeshSections();
	for (int i = 0; i < MeshSections.Num(); i++)
	{

		if (MeshSections[i].Vertices.Num() > 0)
		{
			mesh->CreateMeshSection(i, MeshSections[i].Vertices, MeshSections[i].Triangles, MeshSections[i].Normals, MeshSections[i].UVs, MeshSections[i].VertexColors, MeshSections[i].Tangents, true);
		}
	}
	ApplyMaterials();
}

void AChunk::GenerateData()
{
	for (int x = 0; x < WidthOfChunkExt; x++)
	{
		for (int y = 0; y < WidthOfChunkExt; y++)
		{
			for (int z = 0; z < HeightOfChunk; z++)
			{
				int32 index = z + (y * HeightOfChunk) + (x * WidthOfChunkExt * HeightOfChunk);
				int32 noiseIndex = y + (x * WidthOfChunkExt);

				if (index >= ChunkData.Num())
				{
					UE_LOG(LogTemp, Error, TEXT("Index out of bounds: %d. Chunk Size is: %d"), index, ChunkData.Num());
					break;
				}

				if (noiseIndex >= NoiseData.Num())
				{
					UE_LOG(LogTemp, Error, TEXT("Index out of bounds: %d. Noise Size is: %d"), noiseIndex, NoiseData.Num());
					break;
				}

				if (z == 30 + NoiseData[noiseIndex]) { ChunkData[index].id = 1; } // Grass Block
				else if (z == 29 + NoiseData[noiseIndex]) { ChunkData[index].id = 2; } // Dirt Block
				else if (z < 29 + NoiseData[noiseIndex]) { ChunkData[index].id = 3; } // Stone Block
				else if (z < 15 + NoiseData[noiseIndex] && ChunkData[index].id == 0) { ChunkData[index].id = 7; }
				else { ChunkData[index].id = 0; }
			}
		}
	}

	for (int x = 3; x < WidthOfChunk - 3; x++)
	{
		for (int y = 2; y < WidthOfChunk - 2; y++)
		{
			for (int z = 0; z < HeightOfChunk; z++)
			{
				int32 index = z + (y * HeightOfChunk) + (x * WidthOfChunkExt * HeightOfChunk);

				if (y + (x * WidthOfChunkExt) >= NoiseData.Num() || y + (x * WidthOfChunkExt) < 0) 
				{
					UE_LOG(LogTemp, Warning, TEXT("ERROR: AT LINE 301"));
					break;
				}

				if (z == 31 + NoiseData[y + (x * WidthOfChunkExt)] && RandomStream.FRand() < 0.03) { TreeCenters.Add(FIntVector(x, y, z)); } // Tree
			}
		}
	}

	for (int i = 0; i < TreeCenters.Num(); i++)
	{
		int height = (int)(RandomStream.FRand() * 4) + 4;
		FIntVector pos = TreeCenters[i];

		int rand_x = (int)(RandomStream.FRand() * 1) + 2;
		int rand_y = (int)(RandomStream.FRand() * 1) + 2;
		int rand_z = (int)(RandomStream.FRand() * 1) + 2;

		float radius = FVector(rand_x, rand_y, rand_z).Size();

		for (int x = pos.X - radius; x < pos.X + radius; x++)
		{
			for (int y = pos.Y - radius; y < pos.Y + radius; y++)
			{
				for (int z = pos.Z - radius; z < pos.Z + radius; z++)
				{
					int realPosZ = z + height - rand_z;

					if (FVector::Dist(FVector(pos.X, pos.Y, pos.Z + height), FVector(x, y, realPosZ)) <= radius)
					{
						int32 index = (realPosZ)+((y + 1) * HeightOfChunk) + ((x + 1) * WidthOfChunkExt * HeightOfChunk);

						if (RandomStream.FRand() < 0.8 && ChunkData[index].id == 0) // Only Add leaves if there is an empty block there
							ChunkData[index].id = 5;
					}
				}
			}
		}

		if (height + pos.Z < HeightOfChunk - 1)
		{
			for (int j = 0; j < height; j++)
			{
				int32 index = (pos.Z + j) + (pos.Y * HeightOfChunk) + (pos.X * WidthOfChunkExt * HeightOfChunk);
				ChunkData[index].id = 4;
			}
		}
	}
}

TArray<int32> AChunk::CalculateNoise()
{
	TArray<int32> noises;
	noises.Init(0, WidthOfChunkExt * WidthOfChunkExt);

	int32 ChunkXIndex = (int)(GetActorLocation().X / 100);
	int32 ChunkYIndex = (int)(GetActorLocation().Y / 100);
	for (int32 x = -1; x <= WidthOfChunk; x++)
	{
		for (int32 y = -1; y <= WidthOfChunk; y++)
		{
			float noiseValue =
				USimplexNoiseLibrary::SimplexNoise2D((ChunkXIndex + x) * 0.01f, (ChunkYIndex + y + 1) * 0.01f) * 4 +
				USimplexNoiseLibrary::SimplexNoise2D((ChunkXIndex + x) * 0.01f, (ChunkYIndex + y + 1) * 0.01f) * 8 +
				USimplexNoiseLibrary::SimplexNoise2D((ChunkXIndex + x) * 0.01f, (ChunkYIndex + y + 1) * 0.01f) * 16 +
				FMath::Clamp(USimplexNoiseLibrary::SimplexNoise2D((ChunkXIndex + x) * 0.05f, (ChunkYIndex + y + 1) * 0.05f), 0.0f, 5.0f) * 4; // clamp 0-5

			if ((y + 1) + ((x + 1) * WidthOfChunkExt) < noises.Num())
				noises[(y + 1) + ((x + 1) * WidthOfChunkExt)] = (FMath::FloorToInt(noiseValue));
		}
	}
	return noises;
}

int32 AChunk::DealDamage(int32 x, int32 y, int32 z, int32 damage) 
{
	int32 index = z + (y * HeightOfChunk) + (x * HeightOfChunk * WidthOfChunkExt);

	if (index < ChunkData.Num() && index >= 0)
	{
		ChunkData[index].Current_Health -= damage;
		return ChunkData[index].Current_Health;
	}
	return 1;
}

int32 AChunk::BreakBlock(int32 x, int32 y, int32 z)
{
	int32 index = z + (y * HeightOfChunk) + (x * HeightOfChunk * WidthOfChunkExt);
	int32 tmp = 0;

	if (index < ChunkData.Num() && index >= 0)
	{
		tmp = ChunkData[index].id;
		ChunkData[index].id = 0;
		UpdateMesh();
	}
	return tmp;
}

void AChunk::AddBlock(int32 x, int32 y, int32 z, int32 id)
{
	int32 index = z + (y * HeightOfChunk) + (x * HeightOfChunk * WidthOfChunkExt);

	if (index < ChunkData.Num() && index >= 0)
	{
		ChunkData[index].id = id;
		UpdateMesh();
	}
}

int32 AChunk::GetBlockId(int32 id) 
{
	if (id >= 0 && id < ChunkData.Num())
		return ChunkData[id].id;
	return 0;
}
